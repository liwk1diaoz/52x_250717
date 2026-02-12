/**
        @brief Sample code.\n

        @file hd_logger_test.c

        @author Niven Cho

        @ingroup mhdal

        @note Nothing.

        Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include <stdio.h>
#include <string.h>
#include <kwrap/examsys.h>
#include <pthread.h>
#include <hdal.h>

#if defined(__FREERTOS)
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#endif

extern void hdal_flow_log_p(const char *fmt, ...);

static void *show_message( void *ptr )
{
	char str[256] = {0};
	//coverity[no_escape]
	while(1)
	{
		int len = (rand()%5)+1;
		//coverity[dont_call]
		char ch = (char)('A'+ rand()%26);
		memset(str, ch, len);
		str[len] = 0;
		//hdal_flow_log_p("%d: %s\n",(int)ptr, str);
		hdal_flow_log_p("%s\n", str);
		usleep(1); // for task switch
	}

	pthread_exit((void *)1234);
	return NULL;
}


EXAMFUNC_ENTRY(hd_logger_test, argc, argv)
{
	srand( time(NULL) );
	if (hd_common_init(0) != 0) {
		printf("hd_common_init failed.\n");
		return -1;
	}

	pthread_t thread1;
	int join_ret;

	int i;
	for (i=0;i<100;i++) {
		if (pthread_create(&thread1, NULL, show_message , (void*) i) != 0) {
			printf("thread create failed.\n");
			return -1;
		}
	}
	pthread_join( thread1, (void *)&join_ret);

	return 0;
}