/**
	@brief Source file of vendor ai net test code.

	@file ai_test_int.h

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _AI_TEST_INT_H_
#define _AI_TEST_INT_H_

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"

// platform dependent
#if defined(__LINUX)
#include <pthread.h>			//for pthread API
#define MAIN(argc, argv) 		int main(int argc, char** argv)
#define GETCHAR()				getchar()
#else
#include <FreeRTOS_POSIX.h>	
#include <FreeRTOS_POSIX/pthread.h> //for pthread API
#include <kwrap/util.h>		//for sleep API
#define sleep(x)    			vos_util_delay_ms(1000*(x))
#define msleep(x)    			vos_util_delay_ms(x)
#define usleep(x)   			vos_util_delay_us(x)
#include <kwrap/examsys.h> 	//for MAIN(), GETCHAR() API
#define MAIN(argc, argv) 		EXAMFUNC_ENTRY(ai_test, argc, argv)
#define GETCHAR()				NVT_EXAMSYS_GETCHAR()
#endif

#define DEBUG_MENU 		1

#define CHKPNT			printf("\033[37mCHK: %s, %s: %d\033[0m\r\n",__FILE__,__func__,__LINE__)
#define DBGH(x)			printf("\033[0;35m%s=0x%08X\033[0m\r\n", #x, x)
#define DBGD(x)			printf("\033[0;35m%s=%d\033[0m\r\n", #x, x)

///////////////////////////////////////////////////////////////////////////////

#define NET_PATH_ID		UINT32
#define IN_PATH_ID		UINT32


#include "ai_test_push.h"
#include "ai_test_pull.h"
#include "ai_test_net.h"
#include "ai_test_op.h"


/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/

#define MAX_CH_NUM              9    // some net(inceptionv2_feature_dc) with input channel = 9
#define AI_RGB_BUFSIZE(w, h)    (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(HD_VIDEO_PXLFMT_RGB888_PLANAR) / 8) * (h))
#define USE_AI2_CPU_LOADING     0
#define NN_RUN_NET_NUM	        1
#define NN_SET_INPUT_NUM	    16

/////////////////////////////////////////////////////////

typedef struct _TEST_NETWORK {

	UINT32 auto_test;

	// (1) input 
	NET_IN_CONFIG net_in_cfg;
	NET_PATH_ID in_path;

	// (2) network 
	//NET_PROC_CONFIG net_proc_cfg;
	NET_PATH_ID net_path;
	pthread_t  proc_thread_id;
	UINT32 proc_start;
	UINT32 proc_exit;
	UINT32 proc_oneshot;

	// (3) operator
	NET_PATH_ID op_path;
	int net_op_opt;
	pthread_t  op_thread_id;
	UINT32 op_start;
	UINT32 op_exit;
	UINT32 op_oneshot;
	UINT32 input_blob_num;

} TEST_NETWORK;

typedef struct CONFIG_OPT {
	CHAR key[256];
	CHAR value[256];
}CONFIG_OPT;

extern HD_RESULT ai_test_network_user_start(TEST_NETWORK *p_stream);
extern HD_RESULT ai_test_network_user_oneshot(TEST_NETWORK *p_stream, const char* name);
extern HD_RESULT ai_test_network_user_multishot(TEST_NETWORK *p_stream, const char* name, UINT32 shot_time_ms);
extern HD_RESULT ai_test_network_user_stop(TEST_NETWORK *p_stream);

extern HD_RESULT ai_test_operator_user_start(TEST_NETWORK *p_stream);
extern HD_RESULT ai_test_operator_user_oneshot(TEST_NETWORK *p_stream);
extern HD_RESULT ai_test_operator_user_stop(TEST_NETWORK *p_stream);


#endif //_AI_TEST_INT_H_
