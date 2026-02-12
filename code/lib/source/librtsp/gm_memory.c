/*******************************************************************************
* Copyright  Faraday Technology Corporation 2010-2011.  All rights reserved.   *
*------------------------------------------------------------------------------*
* Name: gm_memory.c                                                            *
* Description:                                                                 *
*     1: ......                                                                *
* Author: giggs                                                                *
*******************************************************************************/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static int gm_alloc_cnt = 0, gm_alloc_cnt_max = 0;

void *gm_malloc(size_t size, char *func)
{
	int addr;

	gm_alloc_cnt++;
	gm_alloc_cnt_max = MAX(gm_alloc_cnt, gm_alloc_cnt_max);
	addr = (int)malloc(size);
	printf("LIBRTSP_MEM_DBG  malloc %d(%d)  0x%08X(%d)  [%s]\n",
		   gm_alloc_cnt, gm_alloc_cnt_max, addr, size, func);
	if (!addr) {
		printf("LIBRTSP_MEM_DBG  malloc alloc NULL\n");
		while (1) {
			printf("LIBRTSP_MEM_DBG ERR!\n");
			sleep(1);
		}
	}
	return (void *)addr;
}

void *gm_calloc(size_t num, size_t size, char *func)
{
	int addr;

	gm_alloc_cnt++;
	gm_alloc_cnt_max = MAX(gm_alloc_cnt, gm_alloc_cnt_max);
	addr = (int)calloc(num, size);
	printf("LIBRTSP_MEM_DBG  calloc %d(%d)  0x%08X(%d)  [%s]\n",
		   gm_alloc_cnt, gm_alloc_cnt_max, addr, size, func);
	if (!addr) {
		printf("LIBRTSP_MEM_DBG  calloc alloc NULL\n");
		while (1) {
			printf("LIBRTSP_MEM_DBG ERR!\n");
			sleep(1);
		}
	}
	return (void *)addr;
}

void gm_free(void *ptr, char *func)
{
	gm_alloc_cnt--;
	printf("LIBRTSP_MEM_DBG  free %d(%d)  0x%08X  [%s]\n",
		   gm_alloc_cnt, gm_alloc_cnt_max, (int)ptr, func);
	if (!(int)ptr) {
		printf("LIBRTSP_MEM_DBG  free NULL\n");
		while (1) {
			printf("LIBRTSP_MEM_DBG ERR!\n");
			sleep(1);
		}
	}
	free(ptr);
}


