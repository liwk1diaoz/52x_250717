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

void* gm_malloc(size_t size, const char *func)
{
    void *addr;

    gm_alloc_cnt++;
    gm_alloc_cnt_max = MAX(gm_alloc_cnt, gm_alloc_cnt_max);
    addr = malloc(size);
    printf("LIBRTMP_MEM_DBG  malloc %d(%d)  0x%08X(%d)  [%s]\n",
           gm_alloc_cnt, gm_alloc_cnt_max, (unsigned int)addr, size, func);
    if (!addr)
    {
        printf("LIBRTMP_MEM_DBG  malloc alloc NULL\n");
        while(1) { printf("LIBRTMP_MEM_DBG ERR!\n"); sleep(1); }
    }
    return addr;
}

void* gm_calloc(size_t num, size_t size, const char *func)
{
    void *addr;

    gm_alloc_cnt++;
    gm_alloc_cnt_max = MAX(gm_alloc_cnt, gm_alloc_cnt_max);
    addr = calloc(num, size);
    printf("LIBRTMP_MEM_DBG  calloc %d(%d)  0x%08X(%d)  [%s]\n",
           gm_alloc_cnt, gm_alloc_cnt_max, (unsigned int)addr, size, func);
    if (!addr)
    {
        printf("LIBRTMP_MEM_DBG  calloc alloc NULL\n");
        while(1) { printf("LIBRTMP_MEM_DBG ERR!\n"); sleep(1); }
    }
    return addr;
}

void* gm_realloc(void* ptr, size_t size, const char *func)
{
    void *addr;

    if (!ptr)
        gm_alloc_cnt++;
    gm_alloc_cnt_max = MAX(gm_alloc_cnt, gm_alloc_cnt_max);
    addr = realloc(ptr, size);
    printf("LIBRTMP_MEM_DBG  realloc %d(%d)  0x%08X->0x%08X(%d)  [%s]\n",
           gm_alloc_cnt, gm_alloc_cnt_max, (unsigned int)ptr, (unsigned int)addr, size, func);
    if (!addr)
    {
        printf("LIBRTMP_MEM_DBG  calloc relloc NULL\n");
        while(1) { printf("LIBRTMP_MEM_DBG ERR!\n"); sleep(1); }
    }
    return addr;
}

void gm_free(void* ptr, const char *func)
{
    if (!(int)ptr)
        return;

    gm_alloc_cnt--;
    printf("LIBRTMP_MEM_DBG  free %d(%d)  0x%08X  [%s]\n",
           gm_alloc_cnt, gm_alloc_cnt_max, (unsigned int)ptr, func);

    free(ptr);
}

void* gm_memcpy(void* destination, const void* source, size_t num, const char *func)
{
    printf("LIBRTMP_MEM_DBG  memcpy  0x%08X  0x%08X  %d [%s]\n",
           (unsigned int)destination, (unsigned int)source, num, func);
    return memcpy(destination, source, num);
}

void* gm_memset(void* ptr, int value, size_t num, const char *func)
{
    printf("LIBRTMP_MEM_DBG  memset  0x%08X  %d  %d [%s]\n",
           (unsigned int)ptr, value, num, func);
    return memset(ptr, value, num);
}

int   gm_memcmp(const void* ptr1, const void* ptr2, size_t num, const char *func)
{
    printf("LIBRTMP_MEM_DBG  memcmp  0x%08X  0x%08X  %d [%s]\n",
           (unsigned int)ptr1, (unsigned int)ptr2, num, func);
    return memcmp(ptr1, ptr2, num);
}

void* gm_memchr(void* ptr, int value, size_t num, const char *func)
{
    printf("LIBRTMP_MEM_DBG  memchr  0x%08X  %d  %d [%s]\n",
           (unsigned int)ptr, value, num, func);
    return memchr (ptr, value, num);
}

void print_hex(const char *tittle, const void* data, int len)
{
    printf("%s : ",tittle);
    const unsigned char * p = (const unsigned char*)data;
    int i = 0;

    for (; i<len; ++i)
    {
        if (i%16 == 0) printf("\n");
        printf("%02X ", *p++);
    }

    printf("\n");
}
