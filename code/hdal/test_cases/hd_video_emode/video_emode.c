/**
	@brief Sample code of video record.\n

	@file video_record.c

	@author Boyan Huang

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
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
#include <sys/ioctl.h>
#include "kdrv_vdocdc_ioctl.h"
#else
#include <FreeRTOS_POSIX.h>
#include <FreeRTOS_POSIX/pthread.h>
#include <kwrap/task.h>
#include <kwrap/util.h>
#define sleep(x)    vos_task_delay_ms(1000*x)
#define usleep(x)   vos_task_delay_us(x)
#endif

EXAMFUNC_ENTRY(hd_video_emode, argc, argv)
{		
#if defined(__LINUX)
	int i;
	int fd;
	
	if ((fd = open("/dev/kdrv_h26x", O_RDONLY)) == -1) {
		printf("cannot open /dev/nvt_h26x0\n");
		return -1;
	}

	{
		VENC_CH_INFO ch_info;

		ioctl(fd, VENC_IOC_GET_CH_INFO, &ch_info);

		printf("drv_ver = 0x%08x\r\n", ch_info.drv_ver);
		printf("emode_ver = 0x%08x\r\n", ch_info.emode_ver);
		printf("encode_num = %d, id = ", ch_info.num);	
		
		for (i = 0; i < ch_info.num; i++) {
			printf("%d ", ch_info.id[i]);			
		}
		printf("\r\n");
	}
	
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-r"))
			ioctl(fd, VENC_IOC_RD_CFG, argv[++i]);
		else if (!strcmp(argv[i], "-id")) {
			int tmp = atoi(argv[++i]);
			ioctl(fd, VENC_IOC_SET_ENC_ID, &tmp);
		}
		else if (!strcmp(argv[i], "-w"))
			ioctl(fd, VENC_IOC_WT_CFG, argv[++i]);
		else
			;
	}			
	
	close(fd);
#else
	printf("hd_video_emode on rtos not ready\r\n");
#endif
	return 0;
}

