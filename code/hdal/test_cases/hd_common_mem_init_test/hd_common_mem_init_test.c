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

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- \033[0m\r\n");
	
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
	
	DBG_IND("Check if:\r\n");
	DBG_IND("1.the memory pool size, block count , pool type, ddr id match the setting.\r\n");
	DBG_IND("2.each block is 4k alignment (on Linux OS)\r\n");
#if defined(__LINUX)
        system("cat /proc/hdal/comm/info");
#else
	sleep(1);
	nvt_cmdsys_runcmd("nvtmpp info");
	sleep(1);
#endif


	hd_common_mem_uninit();
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");
	
	return ret;
}

static HD_RESULT function_test(void)
{
	HD_RESULT    ret, end_ret = 0;
	
	DBG_IND("\033[33m[FUNCTION_TEST] Memory init test\033[0m\r\n");
	ret = mem_init();
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }		
	
	return end_ret;
}

static HD_RESULT test_common_pool_max_err(HD_COMMON_MEM_DDR_ID ddr_id, UINT32 num)
{
	UINT32                 i;
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- ddr_id: %d, num: %d\033[0m\r\n", (int)(ddr_id), (int)(num));
	
	for (i=0; i<num; i++) {	
		mem_cfg.pool_info[i].type = HD_COMMON_MEM_COMMON_POOL;
		mem_cfg.pool_info[i].blk_size = 0x1000;
		mem_cfg.pool_info[i].blk_cnt = 1;
		mem_cfg.pool_info[i].ddr_id = ddr_id;
	}

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK == ret) {
		DBG_ERR("Error handle test in max pool num: fail %d\r\n", (int)(ret));
		ret = HD_ERR_SYS;
	} else {
		DBG_IND("\033[33mError handle test: PASS\033[0m\r\n");
		ret = HD_OK;
	}
	
	hd_common_mem_uninit();
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");
	
	return ret;	
}

static HD_RESULT test_common_pool_size_over_hdal_memory_err(HD_COMMON_MEM_DDR_ID ddr_id)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- ddr_id: %d\033[0m\r\n", (int)(ddr_id));
	
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x1F400000; // 500MB
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = ddr_id;

	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK == ret) {
		DBG_ERR("Error handle test in common pool size over hdal memory: fail %d\r\n", (int)(ret));
		ret = HD_ERR_SYS;
	} else {
		DBG_IND("\033[33mError handle test: PASS\033[0m\r\n");
		ret = HD_OK;
	}
	
	hd_common_mem_uninit();
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");
	
	return ret;	
}

static HD_RESULT test_init_repeat_err(HD_COMMON_MEM_DDR_ID ddr_id)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- ddr_id: %d\033[0m\r\n", (int)(ddr_id));
	
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x1000;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = ddr_id;
	
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		DBG_ERR("hd_common_mem_init err: %d\r\n", (int)(ret));
	}		
	
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK == ret) {
		DBG_ERR("Error handle test for memory init repeatly: fail %d\r\n", (int)(ret));
		ret = HD_ERR_SYS;
	} else {
		DBG_IND("\033[33mError handle test: PASS\033[0m\r\n");
		ret = HD_OK;
	}
	
	hd_common_mem_uninit();
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");
	
	return ret;	
}

static HD_RESULT test_uninit_repeat_err(HD_COMMON_MEM_DDR_ID ddr_id)
{
	HD_RESULT              ret;
	HD_COMMON_MEM_INIT_CONFIG mem_cfg = {0};

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- ddr_id: %d\033[0m\r\n", (int)(ddr_id));
	
	mem_cfg.pool_info[0].type = HD_COMMON_MEM_COMMON_POOL;
	mem_cfg.pool_info[0].blk_size = 0x1000;
	mem_cfg.pool_info[0].blk_cnt = 1;
	mem_cfg.pool_info[0].ddr_id = ddr_id;
	
	ret = hd_common_mem_init(&mem_cfg);
	if (HD_OK != ret) {
		DBG_ERR("hd_common_mem_init err: %d\r\n", (int)(ret));
	}		
		
	ret = hd_common_mem_uninit();
	if (HD_OK != ret) {
		DBG_ERR("hd_common_mem_uninit err: %d\r\n", (int)(ret));
	}		
	ret = hd_common_mem_uninit();
	if (HD_OK == ret) {
		DBG_ERR("Error handle test for memory uninit repeatly: fail %d\r\n", (int)(ret));
		ret = HD_ERR_SYS;
	} else {
		DBG_IND("\033[33mError handle test: PASS\033[0m\r\n");
		ret = HD_OK;
	}

	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");
	
	return ret;	
}

static HD_RESULT error_handle_test(void)
{
	HD_RESULT    ret, end_ret = 0;

	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Set common pool exceed max number\033[0m\r\n");
	ret = test_common_pool_max_err(DDR_ID0, 35);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Set common pool size over HDAL memory\033[0m\r\n");
	ret = test_common_pool_size_over_hdal_memory_err(DDR_ID0);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }		
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Memory init repeat\033[0m\r\n");
	ret = test_init_repeat_err(DDR_ID0);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }		
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Memory un-init repeat\033[0m\r\n");
	ret = test_uninit_repeat_err(DDR_ID0);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }		
	
	return end_ret;
}

EXAMFUNC_ENTRY(hd_common_mem_init_test, argc, argv)
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
    //uninit hdal
	ret = hd_common_uninit();
    if(ret != HD_OK) {
        DBG_ERR("common fail=%d\n", (int)(ret));
    }
	return 0;
}
