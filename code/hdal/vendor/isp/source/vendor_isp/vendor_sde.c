/**
	@brief Header file of vendor sde module.\n
	This file contains the functions which is related to IQ/3A in the chip.

	@file hd_videocapture.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "vendor_sde.h"
#if defined(__LINUX)
#include <sys/ioctl.h>
#endif
#define PTHREAD_MUTEX ENABLE
#if (PTHREAD_MUTEX == ENABLE)
#include<pthread.h>

static pthread_mutex_t mutex_lock;
#endif

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static BOOL vendor_api_init;
static INT32 sde_fd = -1;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_sde_init(void)
{
	if (vendor_api_init != FALSE) {
		printf("nvt_sde module is already initialised. \n");
		return HD_ERR_INIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
		pthread_mutex_init(&mutex_lock, NULL);
	#endif

	// open sde device
	sde_fd = open("/dev/nvt_sde", O_RDWR);
	if (sde_fd < 0) {
		printf("Open nvt_sde fail!\n");
		return HD_ERR_NG;
	}

	vendor_api_init = TRUE;

	return HD_OK;
}

HD_RESULT vendor_sde_uninit(void)
{
	if (vendor_api_init != TRUE) {
		printf("nvt_sde module is not initialised yet.\n");
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_destroy(&mutex_lock);
	#endif

	if (sde_fd != -1) {
		close(sde_fd);
	}

	vendor_api_init = FALSE;

	return HD_OK;
}

HD_RESULT vendor_sde_get_cmd(SDET_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&mutex_lock);
	#endif

	switch (item) {
	case SDET_ITEM_VERSION:
		ret = ioctl(sde_fd, SDE_IOC_GET_VERSION, (UINT32 *)p_param);
		if (ret < 0) {
			printf("SDE_IOC_GET_VERSION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case SDET_ITEM_IO_INFO:
		ret = ioctl(sde_fd, SDE_IOC_GET_IO_INFO, (UINT32 *)p_param);
		if (ret < 0) {
			printf("SDE_IOC_GET_IO_INFO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case SDET_ITEM_CTRL_PARAM:
		ret = ioctl(sde_fd, SDE_IOC_GET_CTRL, (UINT32 *)p_param);
		if (ret < 0) {
			printf("SDE_IOC_GET_CTRL fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	default:
		printf("vendor_common_get not support item %d \n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&mutex_lock);
	#endif

	return ret;
}

HD_RESULT vendor_sde_set_cmd(SDET_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&mutex_lock);
	#endif

	switch (item) {

	case SDET_ITEM_OPEN:
		ret = ioctl(sde_fd, SDE_IOC_OPEN, NULL);
		if (ret < 0) {
			printf("SDE_IOC_OPEN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case SDET_ITEM_CLOSE:
		ret = ioctl(sde_fd, SDE_IOC_CLOSE, NULL);
		if (ret < 0) {
			printf("SDE_IOC_CLOSE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case SDET_ITEM_TRIGGER:
		ret = ioctl(sde_fd, SDE_IOC_TRIGGER, (UINT32 *)p_param);
		if (ret < 0) {
			printf("SDE_IOC_TRIGGER fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case SDET_ITEM_IO_INFO:
		ret = ioctl(sde_fd, SDE_IOC_SET_IO_INFO, (UINT32 *)p_param);
		if (ret < 0) {
			printf("SDE_IOC_SET_IO_INFO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case SDET_ITEM_CTRL_PARAM:
		ret = ioctl(sde_fd, SDE_IOC_SET_CTRL, (UINT32 *)p_param);
		if (ret < 0) {
			printf("SDE_IOC_SET_CTRL fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	default:
		printf("vendor_common_set not support item %d \n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&mutex_lock);
	#endif

	return ret;
}

