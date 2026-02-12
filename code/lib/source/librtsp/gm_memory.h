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
#include "sysdef.h"

#if LIBRTSP_MEM_DBG
#define malloc(x)       gm_malloc(x, __FUNCTION__)
#define calloc(x, y)    gm_calloc(x, y, __FUNCTION__)
#define free(x)         gm_free(x, __FUNCTION__)
#endif

extern void *gm_malloc(size_t size, char *func);
extern void *gm_calloc(size_t num, size_t size, char *func);
extern void  gm_free(void *ptr, char *func);

#endif /* GM_MEMORY_H_ */
