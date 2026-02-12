/*******************************************************************************
* Copyright  Faraday Technology Corporation 2010-2011.  All rights reserved.   *
*------------------------------------------------------------------------------*
* Name: gm_memory.h                                                            *
* Description:                                                                 *
*     1: ......                                                                *
* Author: giggs                                                                *
*******************************************************************************/

#ifndef GM_MEMORY_H_
#define GM_MEMORY_H_

#include <sys/types.h>

#define LIBRTMP_MEM_DBG     0
#if LIBRTMP_MEM_DBG
#define malloc(x)       gm_malloc(x, __FUNCTION__)
#define calloc(x, y)    gm_calloc(x, y, __FUNCTION__)
#define realloc(x, y)   gm_realloc(x, y, __FUNCTION__)
#define free(x)         gm_free(x, __FUNCTION__)
#define memcpy(x, y, z) gm_memcpy(x, y, z, __FUNCTION__)
#define memset(x, y, z) gm_memset(x, y, z, __FUNCTION__)
#define memcmp(x, y, z) gm_memcmp(x, y, z, __FUNCTION__)
#define memchr(x, y, z) gm_memchr(x, y, z, __FUNCTION__)
#endif

extern void* gm_malloc(size_t size, const char *func);
extern void* gm_calloc(size_t num, size_t size, const char *func);
extern void* gm_realloc(void* ptr, size_t size, const char *func);
extern void  gm_free(void* ptr, const char *func);
extern void* gm_memcpy(void* destination, const void* source, size_t num, const char *func);
extern void* gm_memset(void* ptr, int value, size_t num, const char *func);
extern int   gm_memcmp(const void* ptr1, const void* ptr2, size_t num, const char *func);
extern void* gm_memchr(void* ptr, int value, size_t num, const char *func);

extern void  print_hex(const char *tittle, const void* data, int len);

#endif /* GM_MEMORY_H_ */
