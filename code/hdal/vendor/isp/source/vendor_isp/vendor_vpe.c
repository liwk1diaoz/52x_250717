/**
	@brief Header file of vendor vpe module.\n
	This file contains the functions which is related to IQ/3A in the chip.

	@file hd_videocapture.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <kwrap/type.h>
#include "vendor_vpe.h"
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
static INT32 vpe_fd = -1;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_vpe_init(void)
{
	if (vendor_api_init != FALSE) {
		printf("nvt_vpe module is already initialised. \n");
		return HD_ERR_INIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
		pthread_mutex_init(&mutex_lock, NULL);
	#endif

	// open vpe device
	vpe_fd = open("/dev/nvt_vpe", O_RDWR);
	if (vpe_fd < 0) {
		printf("Open nvt_vpe fail!\n");
		return HD_ERR_NG;
	}

	vendor_api_init = TRUE;

	return HD_OK;
}

HD_RESULT vendor_vpe_uninit(void)
{
	if (vendor_api_init != TRUE) {
		printf("nvt_vpe module is not initialised yet.\n");
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_destroy(&mutex_lock);
	#endif

	if (vpe_fd != -1) {
		close(vpe_fd);
	}

	vendor_api_init = FALSE;

	return HD_OK;
}

HD_RESULT vendor_vpe_get_cmd(VPET_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&mutex_lock);
	#endif

	switch (item) {
	case VPET_ITEM_VERSION:
		ret = ioctl(vpe_fd, VPE_IOC_GET_VERSION, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_GET_VERSION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_SHARPEN_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_GET_SHARPEN, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_GET_SHARPEN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_DCE_CTL_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_GET_DCE_CTL, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_GET_DCE_CTL fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_GDC_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_GET_GDC, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_GET_GDC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_2DLUT_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_GET_2DLUT, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_GET_2DLUT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_DRT_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_GET_DRT, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_GET_DRT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_DCTG_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_GET_DCTG, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_GET_DCTG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_FLIP_ROT_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_GET_FLIP_ROT, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_GET_FLIP_ROT fail! \n");
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

HD_RESULT vendor_vpe_set_cmd(VPET_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&mutex_lock);
	#endif

	switch (item) {

	case VPET_ITEM_SHARPEN_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_SET_SHARPEN, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_SET_SHARPEN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_DCE_CTL_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_SET_DCE_CTL, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_SET_DCE_CTL fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_GDC_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_SET_GDC, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_SET_GDC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_2DLUT_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_SET_2DLUT, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_SET_2DLUT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_DRT_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_SET_DRT, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_SET_DRT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_DCTG_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_SET_DCTG, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_SET_DCTG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case VPET_ITEM_FLIP_ROT_PARAM:
		ret = ioctl(vpe_fd, VPE_IOC_SET_FLIP_ROT, (UINT32 *)p_param);
		if (ret < 0) {
			printf("VPE_IOC_SET_FLIP_ROT fail! \n");
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

