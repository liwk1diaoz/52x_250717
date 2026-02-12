/**
 * @file hd_common_mem_alloc_test.c
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
#include "hd_type.h"
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
#define TEST_PATTERN          0x33

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
	DBG_IND("Virtual memory data with test pattern are correct!\r\n");			
	
#if defined(__LINUX)
        system("cat /proc/hdal/comm/info");
#else
	sleep(1);
        nvt_cmdsys_runcmd("nvtmpp info");
        sleep(1);
#endif

	ret = hd_common_mem_free(pa, (void *)va);
	if (ret != HD_OK) {
		DBG_ERR("err:free pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
		return HD_ERR_NG;
	}
	DBG_IND("free_mem\r\n\r\n");
#if defined(__LINUX)
        system("cat /proc/hdal/comm/info");
#else        
        nvt_cmdsys_runcmd("nvtmpp info");
        sleep(1);
#endif

	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");	
	
	return HD_OK;
}

static HD_RESULT function_test(void)
{
	HD_RESULT    ret, end_ret = 0;
	
	DBG_IND("\033[33m[FUNCTION_TEST] Allocate 100Bytes\033[0m\r\n");
	ret = test_alloc(DDR_ID0, 100);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }	
	
	DBG_IND("\033[33m[FUNCTION_TEST] Allocate 4KBytes\033[0m\r\n");
	ret = test_alloc(DDR_ID0, 4096);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }	

	DBG_IND("\033[33m[FUNCTION_TEST] Allocate 1MBytes\033[0m\r\n");
	ret = test_alloc(DDR_ID0, 0x100000);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }				
	
	DBG_IND("\033[33m[FUNCTION_TEST] Allocate 2MBytes\033[0m\r\n");
	ret = test_alloc(DDR_ID0, 0x200000);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }				
	
	DBG_IND("\033[33m[FUNCTION_TEST] Allocate 8MBytes\033[0m\r\n");
	ret = test_alloc(DDR_ID0, 0x800000);
	end_ret |= ret;
    if(ret != HD_OK) {
		DBG_ERR("function_test fail=%d\r\n", (int)(ret));
    }					
	
	return end_ret;
}

static HD_RESULT test_alloc_err(HD_COMMON_MEM_DDR_ID ddr_id, UINT32 size)
{
	void                 *va;
	UINT32               pa;
	HD_RESULT            ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- ddr_id: %d, size: %d\033[0m\r\n", (int)(ddr_id), (int)(size));
	
	ret = hd_common_mem_alloc("Test Error", &pa, (void **)&va, size, ddr_id);
	if (HD_OK == ret) {
		DBG_ERR("Error handle test for allocate: fail %d\r\n", (int)(ret));
		ret = HD_ERR_SYS;
	} else {
		DBG_IND("\033[33mError handle test: PASS\033[0m\r\n");
		ret = HD_OK;
	}	
	
	if (ret == HD_ERR_SYS) {
		ret = hd_common_mem_free(pa, (void *)va);
		if (ret != HD_OK) {
			DBG_ERR("err:free pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
			return HD_ERR_NG;
		}
	}
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");	
	
	return ret;
}

#if 1
static HD_RESULT test_alloc_err2(HD_COMMON_MEM_DDR_ID ddr_id, UINT32 size, UINT32 addr_type_null)
{
	void                 *va;
	UINT32               pa;
	HD_RESULT            ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- ddr_id: %d, size: %d, addr_type_null: %d\033[0m\r\n", (int)(ddr_id), (int)(size), (int)(addr_type_null));
	
	if (addr_type_null == 1) {
		ret = hd_common_mem_alloc("Test Error", NULL, (void **)&va, size, ddr_id);
	} else if (addr_type_null == 2) {
		ret = hd_common_mem_alloc("Test Error", &pa, NULL, size, ddr_id);
	} else {
		ret = HD_OK;
	}
	
	if (HD_OK == ret) {
		DBG_ERR("Error handle test for allocate: fail %d\r\n", (int)(ret));
		ret = HD_ERR_SYS;
	} else {
		DBG_IND("\033[33mError handle test: PASS\033[0m\r\n");
		ret = HD_OK;
	}	
	
	if (ret == HD_ERR_SYS) {
		ret = hd_common_mem_free(pa, (void *)va);
		if (ret != HD_OK) {
			DBG_ERR("err:free pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
			return HD_ERR_NG;
		}
	}
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");	
	
	return ret;
}
#endif

static HD_RESULT test_alloc_err3(HD_COMMON_MEM_DDR_ID ddr_id, UINT32 size)
{
	void                 *va;
	UINT32               pa;
	HD_RESULT            ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- ddr_id: %d, size: %d\033[0m\r\n", (int)(ddr_id), (int)(size));
	
	// Un memory init. 
	ret = mem_exit();
    if(ret != HD_OK) {
        DBG_ERR("mem fail=%d\n", (int)(ret));
		return HD_ERR_SYS;
    }
		
	ret = hd_common_mem_alloc("Test Error", &pa, (void **)&va, size, ddr_id);	
	if (HD_OK == ret) {
		DBG_ERR("Error handle test for free an invalid address: fail %d\r\n", (int)(ret));
		ret = HD_ERR_SYS;
	} else {
		DBG_IND("\033[33mError handle test: PASS\033[0m\r\n");
		ret = HD_OK;
	}		
	
	//init memory
	ret = mem_init();
	if(ret != HD_OK) {
		DBG_ERR("init fail=%d\r\n", (int)(ret));
		ret = HD_ERR_SYS;
		goto  err;
	}				
	
err:	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");	
	
	return ret;
}

static HD_RESULT test_alloc_err4(HD_COMMON_MEM_DDR_ID ddr_id, UINT32 size, UINT32 free_type)
{
	void                 *va;
	UINT32               pa;
	HD_RESULT            ret = HD_OK;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- ddr_id: %d, size: %d, free_type: %d\033[0m\r\n", (int)(ddr_id), (int)(size), (int)(free_type));
	
	ret = hd_common_mem_alloc("Test Error", &pa, (void **)&va, size, ddr_id);	
	if (ret != HD_OK) {
		DBG_ERR("err:alloc size 0x%x, ddr %d\r\n", (unsigned int)(size), (int)(ddr_id+1));
		return HD_ERR_NG;
	}	
	
	if (free_type == 1) {
		void *va_test = NULL;
		ret = hd_common_mem_free((UINT32)NULL, va_test);
		if (HD_OK == ret) {
			DBG_ERR("Error handle test for free an invalid address: fail %d\r\n", (int)(ret));
			ret = HD_ERR_SYS;
		} else {
			DBG_IND("\033[33mError handle test: PASS\033[0m\r\n");
			ret = HD_OK;
		}	
	} else if (free_type == 2) {
		ret = hd_common_mem_free(pa, (void *)va);
		if (ret != HD_OK) {
			DBG_ERR("err:free pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
			return HD_ERR_NG;
		}

		ret = hd_common_mem_free(pa, (void *)va);
		if (HD_OK == ret) {
			DBG_ERR("Error handle test for double free memory: fail %d\r\n", (int)(ret));
			ret = HD_ERR_SYS;
		} else {
			DBG_IND("\033[33mError handle test: PASS\033[0m\r\n");
			ret = HD_OK;
		}		
	}
	
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");	
	
	return ret;
}

static HD_RESULT error_handle_test(void)
{
	HD_RESULT    ret, end_ret = 0;

	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Allocate over size (over limit test)\033[0m\r\n");
	ret = test_alloc_err(DDR_ID0, 0x1F400000);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Input invalid ddr id\033[0m\r\n");
	ret = test_alloc_err(20, 0x100000);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }	
	
#if 1
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Input phy_addr is NULL\033[0m\r\n");
	ret = test_alloc_err2(DDR_ID0, 0x100000, 1);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }		
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Input virt_addr is NULL\033[0m\r\n");
	ret = test_alloc_err2(DDR_ID0, 0x100000, 2);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }			
#endif	
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Allocate with hd_common_mem not initialized\033[0m\r\n");	
	ret = test_alloc_err3(DDR_ID0, 0x100000);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }			
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Free an invalid address\033[0m\r\n");
	ret = test_alloc_err4(DDR_ID0, 0x100000, 1);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }		
	
	DBG_IND("\033[33m[ERROR_HANDLE_TEST] Double free memory\033[0m\r\n");
	ret = test_alloc_err4(DDR_ID0, 0x100000, 2);
	end_ret |= ret;					
    if(ret != HD_OK) {
		DBG_ERR("error_handle_test fail=%d\r\n", (int)(ret));
    }		

	return end_ret;
}


static HD_RESULT test_robust(HD_COMMON_MEM_DDR_ID ddr_id, UINT32 size, UINT32 times)
{
	UINT32               i, j;
	void                 *va;
	UINT32               pa;
	HD_RESULT            ret;

	DBG_IND("#########################################################################\r\n");
	DBG_IND("%s: Start...\r\n", __FUNCTION__);	
	DBG_IND("\033[33mInput Param -- ddr_id: %d, size: %d, times: %d\033[0m\r\n", (int)(ddr_id), (int)(size), (int)(times));
	
	for (j=0; j<times; j++) {	
		ret = hd_common_mem_alloc("Test", &pa, (void **)&va, size, ddr_id);
		if (ret != HD_OK) {
			DBG_ERR("err:alloc size 0x%x, ddr %d\r\n", (unsigned int)(size), (int)(ddr_id+1));
			return HD_ERR_NG;
		}
		memset((void*)va, TEST_PATTERN, size);
		/* check data */
		for (i=0; i<size; i++) {			
			if ( *((UINT8 *)va + i) != TEST_PATTERN ) {
				DBG_ERR("Test Pattern:%u != Virtual memory data:%u (with offset %u bytes)\r\n", (unsigned int)(TEST_PATTERN), (unsigned int)(*((UINT8 *)va + i)), (unsigned int)(i));
				ret = hd_common_mem_free(pa, (void *)va);
				if (ret != HD_OK) {
					DBG_ERR("err:free pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
					return HD_ERR_NG;
				}				
				return HD_ERR_SYS;
			}
		}

		ret = hd_common_mem_free(pa, (void *)va);
		if (ret != HD_OK) {
			DBG_ERR("err:free pa = 0x%x, va = 0x%x\r\n", (unsigned int)(pa), (unsigned int)(va));
			return HD_ERR_NG;
		}
	}
	
	system("cat /proc/hdal/comm/info");
	DBG_IND("%s: Done...\r\n", __FUNCTION__);	
	DBG_IND("#########################################################################\r\n");	
	
	return HD_OK;
}

static HD_RESULT robust_test(void)
{
	HD_RESULT    ret;
	
	DBG_IND("\033[33m[ROBUST_TEST] Allocate/Free 1000 times\033[0m\r\n");
	ret = test_robust(DDR_ID0, 0x100000, 1000);
    if(ret != HD_OK) {
		DBG_ERR("robust_test fail=%d\r\n", (int)(ret));
    }		

	return ret;
}

EXAMFUNC_ENTRY(hd_common_mem_alloc_test, argc, argv)
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
	} else if (param == 3) {
		DBG_IND("ROBUST_TEST START\r\n");
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
