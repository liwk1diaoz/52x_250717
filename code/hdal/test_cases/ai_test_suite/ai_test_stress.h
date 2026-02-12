/**
	@brief Source file of vendor ai net test code.

	@file ai_test_util.h

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _AI_TEST_STRESS_H_
#define _AI_TEST_STRESS_H_

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"

#if defined(__LINUX)
#include <pthread.h>                    //for pthread API
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#endif

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/

typedef struct _CPU_STRESS_DATA {
	UINT32                       sleep_us;
	UINT32                       sleep_loop_count;
	UINT32                       stress_level;
	pthread_t                    thread_id;
	int                          exit;
} CPU_STRESS_DATA;

typedef struct _DMA_STRESS_DATA {
	UINT32                       copy_size;
	UINT32                       ddr_id;
	UINT32                       sleep_us;
	UINT32                       copy_times;
	UINT32                       stress_level;
	pthread_t                    thread_id;
	UINT32                       exit;
} DMA_STRESS_DATA;

/*-----------------------------------------------------------------------------*/
/* Function Definitions                                                            */
/*-----------------------------------------------------------------------------*/

int start_cpu_stress_thread(CPU_STRESS_DATA *data);
int stop_cpu_stress_thread(CPU_STRESS_DATA *data);

int start_dma_stress_thread(DMA_STRESS_DATA *data);
int stop_dma_stress_thread(DMA_STRESS_DATA *data);

#endif //_AI_TEST_STRESS_H_

