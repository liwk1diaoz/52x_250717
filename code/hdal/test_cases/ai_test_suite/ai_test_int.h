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


#include "ai_test_push.h"
#include "ai_test_pull.h"
#include "ai_test_net.h"
#include "ai_test_op.h"


/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/

#define AI_RGB_BUFSIZE(w, h)    (ALIGN_CEIL_4((w) * HD_VIDEO_PXLFMT_BPP(HD_VIDEO_PXLFMT_RGB888_PLANAR) / 8) * (h))

/////////////////////////////////////////////////////////

typedef struct CONFIG_OPT {
	CHAR key[256];
	CHAR value[256];
}CONFIG_OPT;

#endif //_AI_TEST_INT_H_
