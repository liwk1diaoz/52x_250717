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
#include <kwrap/cmdsys.h>
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
	mem_cfg.pool_info[0].blk_cnt = 2;
	mem_cfg.pool_info[0].ddr_id = DDR_ID0;
	
	mem_cfg.pool_info[1].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[1].blk_size = 0x300000;
	mem_cfg.pool_info[1].blk_cnt = 3;
	mem_cfg.pool_info[1].ddr_id = DDR_ID0;	
	
	mem_cfg.pool_info[2].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[2].blk_size = 0x400000;
	mem_cfg.pool_info[2].blk_cnt = 2;
	mem_cfg.pool_info[2].ddr_id = DDR_ID0;		
	
	mem_cfg.pool_info[3].type = HD_COMMON_MEM_OSG_POOL;
	mem_cfg.pool_info[3].blk_size = 0x500000;
	mem_cfg.pool_info[3].blk_cnt = 1;
	mem_cfg.pool_info[3].ddr_id = DDR_ID0;
	
	mem_cfg.pool_info[4].type = HD_COMMON_MEM_GFX_POOL;
	mem_cfg.pool_info[4].blk_size = 0x200000;
	mem_cfg.pool_info[4].blk_cnt = 2;
	mem_cfg.pool_info[4].ddr_id = DDR_ID0;

	
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		DBG_ERR("hd_common_mem_init err: %d\r\n", (int)(ret));
	}
	
	system("cat /proc/hdal/comm/info");
	return ret;
}

static HD_RESULT mem_exit(void)
{
	return hd_common_mem_uninit();
}

#define ADDR_ARRAY_NUM   3
static HD_RESULT test_get_block(UINT32 blk_size, 
								HD_COMMON_MEM_POOL_TYPE pool_type,
								HD_COMMON_MEM_DDR_ID ddr_id,
								UINT32 times)
{
	HD_COMMON_MEM_VB_BLK blk[ADDR_ARRAY_NUM];
	UINT32               i,j;
	UINT32               pa[ADDR_ARRAY_NUM], va[ADDR_ARRAY_NUM];
	HD_RESULT         ret, func_ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- blk_size: 0x%x, ddr_id: %d, times: %d\033[0m\r\n", (unsigned int)(blk_size), (int)(ddr_id), (int)(times));

	if (times > ADDR_ARRAY_NUM) {
		DBG_ERR("Test times over %d.\r\n", (int)(ADDR_ARRAY_NUM));
		return HD_ERR_SYS;
	}
#if defined(__LINUX)
	system("cat /proc/hdal/comm/info");
#else
	nvt_cmdsys_runcmd("nvtmpp info");
#endif
	DBG_IND("\r\nblk_size = 0x%x\r\n", (unsigned int)(blk_size));
	for (j=0; j<times; j++) {	
		blk[j] = hd_common_mem_get_block(pool_type, blk_size, ddr_id);
		if (blk[j] == HD_COMMON_MEM_VB_INVALID_BLK) {
			DBG_ERR("get block fail(blk(%d) %d)\r\n", (int)(j), (int)(blk[j]));
			return HD_ERR_NG;
		}
		DBG_IND("blk[%d] = 0x%x\r\n", (int)(j), (unsigned int)(blk[j]));
		
		
			pa[j] = hd_common_mem_blk2pa(blk[j]);
			if (pa[j] == 0) {
				DBG_ERR("blk2pa fail, blk(%d) = 0x%x\r\n", (int)j, (unsigned int)blk[j]);
				func_ret = HD_ERR_SYS;
				goto blk2pa_err;
			}
			DBG_IND("pa[%d] = 0x%x\r\n", (int)(j), (unsigned int)(pa[j]));
	}
	
	if (pa[0] > 0) {
		
		for (j=0; j<times; j++) {
			va[j] = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa[j], blk_size);
			if (va[j] == 0) {
				func_ret = HD_ERR_SYS;
				goto map_err;
			}
			DBG_IND("va[%d] = 0x%x\r\n", (int)(j), (unsigned int)(va[j]));
			memset((void*)va[j], TEST_PATTERN, blk_size);
			DBG_IND("va[%d] = 0x%x\r\n", (int)(j), (unsigned int)(va[j]));
			/* check data */
			for (i=0; i<blk_size; i++) {			
				if ( *((UINT8 *)va[j] + i) != TEST_PATTERN ) {
					DBG_ERR("Index:%d, Test Pattern:%u != Virtual memory data:%u (with offset %u bytes)\r\n", (int)(j), (unsigned int)(TEST_PATTERN), (unsigned int)(*((UINT8 *)va[j] + i)), (unsigned int)(i));
					goto data_err;
				}
			}
			DBG_IND("Virtual memory data with test pattern are correct!\r\n");		
		}
		
data_err:
		for (j=0; j<times; j++) {
			hd_common_mem_flush_cache((void*)va[j], blk_size);
			hd_common_mem_munmap((void*)va[j], blk_size);
		}
	}
blk2pa_err:
map_err:

#if defined(__LINUX)
        system("cat /proc/hdal/comm/info");
#else
        nvt_cmdsys_runcmd("nvtmpp info");
#endif

	for (j=0; j<times; j++) {
	ret = hd_common_mem_release_block(blk[j]);
		if (HD_OK != ret) {
			DBG_ERR("release blk fail %d index:%d\r\n", (int)(ret), (int)(j));
			return HD_ERR_NG;
		}
	}
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");
	return func_ret;
}

static HD_RESULT function_test(void)
{
	HD_RESULT    ret, end_ret = 0;
	
	DBG_IND("\033[33m[FUNCTION_TEST] Get block size 2MB from common pool\033[0m\r\n");
	ret = test_get_block(0x200000, HD_COMMON_MEM_COMMON_POOL, DDR_ID0, 1);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			
	
	DBG_IND("\033[33m[FUNCTION_TEST] Get block size 3MB from common pool\033[0m\r\n");
	ret = test_get_block(0x300000, HD_COMMON_MEM_COMMON_POOL, DDR_ID0, 1);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			
	
	DBG_IND("\033[33m[FUNCTION_TEST] Get block size 4MB from common pool\033[0m\r\n");
	ret = test_get_block(0x400000, HD_COMMON_MEM_COMMON_POOL, DDR_ID0, 1);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }				
	
	DBG_IND("\033[33m[FUNCTION_TEST] Get block size 5MB from OSG pool\033[0m\r\n");
	ret = test_get_block(0x500000, HD_COMMON_MEM_OSG_POOL, DDR_ID0, 1);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			
		
	DBG_IND("\033[33m[FUNCTION_TEST] Get block size 2MB from GFX pool\033[0m\r\n");
	ret = test_get_block(0x200000, HD_COMMON_MEM_GFX_POOL, DDR_ID0, 1);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			
		
	DBG_IND("\033[33m[FUNCTION_TEST] Get block size 2MB for 3 times from Common pool\033[0m\r\n");
	ret = test_get_block(0x200000, HD_COMMON_MEM_COMMON_POOL, DDR_ID0, 3);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			
	
	return end_ret;
}

#define ADDR_ARRAY_NUM   3
static HD_RESULT test_get_block_err(UINT32 blk_size, 
								HD_COMMON_MEM_POOL_TYPE pool_type,
								HD_COMMON_MEM_DDR_ID ddr_id,
								UINT32 times)
{
	HD_COMMON_MEM_VB_BLK blk[ADDR_ARRAY_NUM];
	UINT32               j;
	//UINT32               pa[ADDR_ARRAY_NUM], va[ADDR_ARRAY_NUM];
	HD_RESULT         ret, func_ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- blk_size: 0x%x, ddr_id: %d, times: %d\033[0m\r\n", (unsigned int)(blk_size), (int)(ddr_id), (int)(times));

	if (times > ADDR_ARRAY_NUM) {
		DBG_ERR("Test times over %d.\r\n", (int)(ADDR_ARRAY_NUM));
		return HD_ERR_SYS;
	}
	system("cat /proc/hdal/comm/info");

	DBG_IND("\r\nblk_size = 0x%x\r\n", (unsigned int)(blk_size));
	for (j=0; j<times; j++) {	
		blk[j] = hd_common_mem_get_block(pool_type, blk_size, ddr_id);
		if (blk[j] == HD_COMMON_MEM_VB_INVALID_BLK) {
			DBG_IND("\033[33mError handle test: PASS (for get block %d times: %d)(Check)\033[0m\r\n", (int)(j+1), (int)(blk[j]));
			func_ret = HD_OK; // error means error handle test success, but need to check!!
			goto block_err;
		}
		DBG_IND("blk[%d] = 0x%x\r\n", (int)(j), (unsigned int)(blk[j]));
		func_ret = HD_ERR_SYS; // no error means error handle test fail
	}

block_err:

	system("cat /proc/hdal/comm/info");

	times = j;
	for (j=0; j<times; j++) {
	ret = hd_common_mem_release_block(blk[j]);
		if (HD_OK != ret) {
			DBG_ERR("release blk fail %d index:%d\r\n", (int)(ret), (int)(j));
			return HD_ERR_NG;
		}
	}
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");
	return func_ret;
}

static HD_RESULT test_get_block_err2(UINT32 blk_size, 
								HD_COMMON_MEM_POOL_TYPE pool_type,
								HD_COMMON_MEM_DDR_ID ddr_id,
								UINT32 free_times)
{
	HD_COMMON_MEM_VB_BLK blk;
	UINT32               i;
	UINT32               pa, va;
	HD_RESULT         ret, func_ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("Test mapping memory: Start...\r\n");
	DBG_IND("\033[33mInput Param -- blk_size: 0x%x, pool_type: %d, ddr_id: %d, free_times:%d \033[0m\r\n", (unsigned int)(blk_size), (int)(pool_type), (int)(ddr_id), (int)(free_times));

	system("cat /proc/hdal/comm/info");
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
		va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, pa, blk_size);
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

	system("cat /proc/hdal/comm/info");

	for (i=0; i<free_times; i++) {
		ret = hd_common_mem_release_block(blk);
		if (HD_OK != ret) {
			DBG_IND("\033[33mError handle test: PASS (for release block %d times: %d)(Check)\033[0m\r\n", (int)(i+1), (int)(blk));			
			return HD_OK;
		}
		func_ret = HD_ERR_SYS;
	}
	
	DBG_IND("Test mapping memory: Done...\r\n");
	DBG_IND("#########################################################################\r\n");
	return func_ret;
}

static HD_RESULT test_get_block_err3(UINT32 blk_size, 
								HD_COMMON_MEM_POOL_TYPE pool_type,
								HD_COMMON_MEM_DDR_ID ddr_id)
{
	HD_COMMON_MEM_VB_BLK blk;
	HD_RESULT         ret, func_ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("Test mapping memory: Start...\r\n");
	DBG_IND("\033[33mInput Param -- blk_size: 0x%x, pool_type: %d, ddr_id: %d\033[0m\r\n", (unsigned int)(blk_size), (int)(pool_type), (int)(ddr_id));
	system("cat /proc/hdal/comm/info");
	
	// Un memory init. 
	ret = mem_exit();
    if(ret != HD_OK) {
        DBG_ERR("mem fail=%d\n", (int)(ret));
		return HD_ERR_SYS;
    }
	
	// Try to get block without memory init.
	DBG_IND("\r\nblk_size = 0x%x\r\n", (unsigned int)(blk_size));
	blk = hd_common_mem_get_block(pool_type, blk_size, ddr_id);
	if (blk == HD_COMMON_MEM_VB_INVALID_BLK) {
		DBG_IND("\033[33mError handle test: PASS (for get block without memory init: %d)(Check)\033[0m\r\n", (int)(blk));			
		func_ret = HD_OK;
		
		//init memory
		ret = mem_init();
		if(ret != HD_OK) {
			DBG_ERR("init fail=%d\r\n", (int)(ret));
			func_ret = HD_ERR_SYS;
			goto err;
		}			
		
		goto err;
	}
	func_ret = HD_ERR_SYS;
	DBG_IND("blk = 0x%x\r\n", (unsigned int)(blk));
	
err:	
	DBG_IND("Test mapping memory: Done...\r\n");
	DBG_IND("#########################################################################\r\n");
	return func_ret;
}

static HD_RESULT error_handle_test(void)
{
	HD_RESULT    ret, end_ret = 0;

	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Get block size 4 MB from common pool for 3 times\033[0m\r\n");
	ret = test_get_block_err(0x400000, HD_COMMON_MEM_COMMON_POOL, DDR_ID0, 3);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Get block size 5 MB from OSG pool for 2 times\033[0m\r\n");
	ret = test_get_block_err(0x500000, HD_COMMON_MEM_OSG_POOL, DDR_ID0, 2);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Get block size 2 MB from GFX pool for 3 times\033[0m\r\n");
	ret = test_get_block_err(0x200000, HD_COMMON_MEM_GFX_POOL, DDR_ID0, 3);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }		
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Free a block twice.\033[0m\r\n");
	ret = test_get_block_err2(0x200000, HD_COMMON_MEM_GFX_POOL, DDR_ID0, 2);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Get block with hd_common_mem not initialized.\033[0m\r\n");
	ret = test_get_block_err3(0x200000, HD_COMMON_MEM_COMMON_POOL, DDR_ID0);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }			
	
	return end_ret;
}
	
EXAMFUNC_ENTRY(hd_common_get_block_test, argc, argv)
{
    HD_RESULT ret;
	INT param;

    if (argc != 2)
    {
		DBG_ERR("Usage:Number of parameter shall be 1.\r\n");
		DBG_IND("\r\nParam 1 to do Function Test.\r\n");
		DBG_IND("Param 2 to do Error Handle\r\n");
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
		printf("init fail=%d\r\n", (int)(ret));
		goto exit;
	}	

	if (param == 1) {
		DBG_IND("FUNCTION_TEST START\r\n");
		ret = function_test();
	    if(ret != HD_OK) {
			DBG_ERR("FUNCTION_TEST FAIL =%d\r\n", (int)(ret));
	    }
		DBG_IND("FUNCTION_TEST OK\r\n");
	} else if (param == 2) {
		DBG_IND("ERROR_HANDLE_TEST START\r\n");
		ret = error_handle_test();
	    if(ret != HD_OK) {
			DBG_ERR("ERROR_HANDLE_TEST FAIL =%d\r\n", (int)(ret));
	    }
		DBG_IND("ERROR_HANDLE_TEST OK\r\n");
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
        DBG_ERR("mem fail=%d\n", (int)(ret));
    }
    //uninit hdal
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        DBG_ERR("common fail=%d\n", (int)(ret));
    }
	return 0;
}
