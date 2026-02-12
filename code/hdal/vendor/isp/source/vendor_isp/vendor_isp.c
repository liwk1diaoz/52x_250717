/**
	@brief Header file of vendor isp module.\n
	This file contains the functions which is related to IQ/3A in the chip.

	@file vendor_isp.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "vendor_isp.h"
#if defined(__LINUX)
#include <sys/ioctl.h>
#endif
#define PTHREAD_MUTEX ENABLE
#if (PTHREAD_MUTEX == ENABLE)
#include<pthread.h>

static pthread_mutex_t ae_mutex_lock;
static pthread_mutex_t af_mutex_lock;
static pthread_mutex_t awb_mutex_lock;
static pthread_mutex_t iq_mutex_lock;
static pthread_mutex_t isp_mutex_lock;
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
static INT32 isp_fd = -1;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_isp_init(void)
{
	if (vendor_api_init != FALSE) {
		printf("nvt_isp module is already initialised. \n");
		return HD_ERR_INIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_init(&ae_mutex_lock, NULL);
	pthread_mutex_init(&af_mutex_lock, NULL);
	pthread_mutex_init(&awb_mutex_lock, NULL);
	pthread_mutex_init(&iq_mutex_lock, NULL);
	pthread_mutex_init(&isp_mutex_lock, NULL);
	#endif

	// open isp device
	isp_fd = open("/dev/nvt_isp", O_RDWR);
	if (isp_fd < 0) {
		printf("Open nvt_isp fail!\n");
		return HD_ERR_NG;
	}

	vendor_api_init = TRUE;

	return HD_OK;
}

HD_RESULT vendor_isp_uninit(void)
{
	if (vendor_api_init != TRUE) {
		printf("nvt_isp module is not initialised yet.\n");
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_destroy(&ae_mutex_lock);
	pthread_mutex_destroy(&af_mutex_lock);
	pthread_mutex_destroy(&awb_mutex_lock);
	pthread_mutex_destroy(&iq_mutex_lock);
	pthread_mutex_destroy(&isp_mutex_lock);
	#endif

	if (isp_fd != -1) {
		close(isp_fd);
	}

	vendor_api_init = FALSE;

	return HD_OK;
}

HD_RESULT vendor_isp_get_ae(AET_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&ae_mutex_lock);
	#endif

	switch (item) {
	case AET_ITEM_VERSION:
		ret = ioctl(isp_fd, AE_IOC_GET_VERSION, (UINT32 *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_VERSION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_FREQUENCY:
		ret = ioctl(isp_fd, AE_IOC_GET_FREQUENCY, (AET_FREQUENCY_MODE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_FREQUENCY fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_METER:
		ret = ioctl(isp_fd, AE_IOC_GET_METER, (AET_METER_MODE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_METER fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_EV:
		ret = ioctl(isp_fd, AE_IOC_GET_EV, (AET_EV_OFFSET *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_EV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_ISO:
		ret = ioctl(isp_fd, AE_IOC_GET_ISO, (AET_ISO_VALUE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_ISO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LONGEXP:
		ret = ioctl(isp_fd, AE_IOC_GET_LONGEXP, (AET_LONGEXP_MODE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_LONGEXP fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LONGEXP_EXPT:
		ret = ioctl(isp_fd, AE_IOC_GET_LONGEXP_EXPT, (AET_LONGEXP_EXPT_VALUE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_LONGEXP_EXPT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LONGEXP_ISO:
		ret = ioctl(isp_fd, AE_IOC_GET_LONGEXP_ISO, (AET_LONGEXP_ISO_VALUE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_LONGEXP_ISO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_OPERATION:
		ret = ioctl(isp_fd, AE_IOC_GET_OPERATION, (AET_OPERATION *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_OPERATION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_BASE_ISO:
		ret = ioctl(isp_fd, AE_IOC_GET_BASE_ISO, (UINT32 *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_BASE_ISO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_BASE_GAIN_RATIO:
		ret = ioctl(isp_fd, AE_IOC_GET_BASE_GAIN_RATIO, (UINT32 *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_BASE_GAIN_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_EXPECT_LUM:
		ret = ioctl(isp_fd, AE_IOC_GET_EXPECT_LUM, (AET_EXPECT_LUM *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_EXPECT_LUM fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LA_CLAMP:
		ret = ioctl(isp_fd, AE_IOC_GET_LA_CLAMP, (AET_LA_CLAMP *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_LA_CLAMP fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_OVER_EXPOSURE:
		ret = ioctl(isp_fd, AE_IOC_GET_OVER_EXPOSURE, (AET_OVER_EXPOSURE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_OVER_EXPOSURE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_CONVERGENCE:
		ret = ioctl(isp_fd, AE_IOC_GET_CONVERGENCE, (AET_CONVERGENCE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_CONVERGENCE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_CURVE_GEN_MOVIE:
		ret = ioctl(isp_fd, AE_IOC_GET_CURVE_GEN_MOVIE, (AET_CURVE_GEN_MOVIE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_CURVE_GEN_MOVIE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_METER_WIN:
		ret = ioctl(isp_fd, AE_IOC_GET_METER_WIN, (AET_METER_WINDOW *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_METER_WIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LUM_GAMMA:
		ret = ioctl(isp_fd, AE_IOC_GET_LUM_GAMMA, (AET_LUM_GAMMA *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_LUM_GAMMA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_SHDR:
		ret = ioctl(isp_fd, AE_IOC_GET_SHDR, (AET_SHDR *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_SHDR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_SHDR_HBS:
		ret = ioctl(isp_fd, AE_IOC_GET_SHDR_HBS, (AET_SHDR_HBS *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_SHDR HBS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_IRIS_CFG:
		ret = ioctl(isp_fd, AE_IOC_GET_IRIS_CFG, (AET_IRIS_CFG *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_IRIS_CFG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_CURVE_GEN_PHOTO:
		ret = ioctl(isp_fd, AE_IOC_GET_CURVE_GEN_PHOTO, (AET_CURVE_GEN_PHOTO *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_CURVE_GEN_PHOTO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_MANUAL:
		ret = ioctl(isp_fd, AE_IOC_GET_MANUAL, (AET_MANUAL *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_MANUAL fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_STATUS:
		ret = ioctl(isp_fd, AE_IOC_GET_STATUS, (AET_STATUS_INFO *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_STATUS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_PRIORITY:
		ret = ioctl(isp_fd, AE_IOC_GET_PRIORITY, (AET_PRIORITY *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_PRIORITY fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_ROI_WIN:
		ret = ioctl(isp_fd, AE_IOC_GET_ROI_WIN, (AET_ROI_WIN *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_ROI_WIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_SMART_IR:
		ret = ioctl(isp_fd, AE_IOC_GET_SMART_IR, (AET_SMART_IR *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_SMART_IR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_EXPT_BOUND:
		ret = ioctl(isp_fd, AE_IOC_GET_EXPT_BOUND, (AET_SMART_IR *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_EXPT_BOUND fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_GAIN_BOUND:
		ret = ioctl(isp_fd, AE_IOC_GET_GAIN_BOUND, (AET_SMART_IR *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_GAIN_BOUND fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LA_WIN:
		ret = ioctl(isp_fd, AE_IOC_GET_LA_WIN, (AET_LA_WIN *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_LA_WIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_APERTURE_BOUND:
		ret = ioctl(isp_fd, AE_IOC_GET_APERTURE_BOUND, (AET_APERTURE_BOUND *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_APERTURE_BOUND fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_STITCH_ID:
		ret = ioctl(isp_fd, AE_IOC_GET_STITCH_ID, (AET_STITCH_ID *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_STITCH_ID fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_DCIRIS:
		ret = ioctl(isp_fd, AE_IOC_GET_DCIRIS, (AET_DCIRIS *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_DCIRIS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_NNSC_EXP_RATIO:
		ret = ioctl(isp_fd, AE_IOC_GET_EXP_RATIO, (AET_EXP_RATIO *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_EXP_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_SKIP_LA:
		ret = ioctl(isp_fd, AE_IOC_GET_SKIP_LA, (AET_SKIP_LA *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_SKIP_LA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_EXT_PARAM:
		ret = ioctl(isp_fd, AE_IOC_GET_EXT_PARAM, (AET_EXTEND_PARAM *)p_param);
		if (ret < 0) {
			printf("AE_IOC_GET_EXT_PARAM fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	default:
		printf("vendor_ae_get not support item %d \n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&ae_mutex_lock);
	#endif

	return ret;
}

HD_RESULT vendor_isp_set_ae(AET_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&ae_mutex_lock);
	#endif

	switch (item) {
	case AET_ITEM_RLD_CONFIG:
		ret = ioctl(isp_fd, AE_IOC_RLD_CONFIG, (AET_CFG_INFO *)p_param);
		if (ret < 0) {
			printf("AE_IOC_RLD_CONFIG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_RLD_DTSI:
		ret = ioctl(isp_fd, AE_IOC_RLD_DTSI, (AET_DTSI_INFO *)p_param);
		if (ret < 0) {
			printf("AE_IOC_RLD_DTSI fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_FREQUENCY:
		ret = ioctl(isp_fd, AE_IOC_SET_FREQUENCY, (AET_FREQUENCY_MODE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_FREQUENCY fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_METER:
		ret = ioctl(isp_fd, AE_IOC_SET_METER, (AET_METER_MODE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_METER fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_EV:
		ret = ioctl(isp_fd, AE_IOC_SET_EV, (AET_EV_OFFSET *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_EV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_ISO:
		ret = ioctl(isp_fd, AE_IOC_SET_ISO, (AET_ISO_VALUE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_ISO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LONGEXP:
		ret = ioctl(isp_fd, AE_IOC_SET_LONGEXP, (AET_LONGEXP_MODE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_LONGEXP fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LONGEXP_EXPT:
		ret = ioctl(isp_fd, AE_IOC_SET_LONGEXP_EXPT, (AET_LONGEXP_EXPT_VALUE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_LONGEXP_EXPT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LONGEXP_ISO:
		ret = ioctl(isp_fd, AE_IOC_SET_LONGEXP_ISO, (AET_LONGEXP_ISO_VALUE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_LONGEXP_ISO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_OPERATION:
		ret = ioctl(isp_fd, AE_IOC_SET_OPERATION, (AET_OPERATION *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_OPERATION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_BASE_ISO:
		ret = ioctl(isp_fd, AE_IOC_SET_BASE_ISO, (UINT32 *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_BASE_ISO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_BASE_GAIN_RATIO:
		ret = ioctl(isp_fd, AE_IOC_SET_BASE_GAIN_RATIO, (UINT32 *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_BASE_GAIN_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_EXPECT_LUM:
		ret = ioctl(isp_fd, AE_IOC_SET_EXPECT_LUM, (AET_EXPECT_LUM *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_EXPECT_LUM fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LA_CLAMP:
		ret = ioctl(isp_fd, AE_IOC_SET_LA_CLAMP, (AET_LA_CLAMP *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_LA_CLAMP fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_OVER_EXPOSURE:
		ret = ioctl(isp_fd, AE_IOC_SET_OVER_EXPOSURE, (AET_OVER_EXPOSURE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_OVER_EXPOSURE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_CONVERGENCE:
		ret = ioctl(isp_fd, AE_IOC_SET_CONVERGENCE, (AET_CONVERGENCE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_CONVERGENCE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_CURVE_GEN_MOVIE:
		ret = ioctl(isp_fd, AE_IOC_SET_CURVE_GEN_MOVIE, (AET_CURVE_GEN_MOVIE *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_CURVE_GEN_MOVIE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_METER_WIN:
		ret = ioctl(isp_fd, AE_IOC_SET_METER_WIN, (AET_METER_WINDOW *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SSET_METER_WIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LUM_GAMMA:
		ret = ioctl(isp_fd, AE_IOC_SET_LUM_GAMMA, (AET_LUM_GAMMA *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_LUM_GAMMA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_SHDR:
		ret = ioctl(isp_fd, AE_IOC_SET_SHDR, (AET_SHDR *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_SHDR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_SHDR_HBS:
		ret = ioctl(isp_fd, AE_IOC_SET_SHDR_HBS, (AET_SHDR_HBS *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_SHDR HBS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_IRIS_CFG:
		ret = ioctl(isp_fd, AE_IOC_SET_IRIS_CFG, (AET_IRIS_CFG *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_IRIS_CFG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_CURVE_GEN_PHOTO:
		ret = ioctl(isp_fd, AE_IOC_SET_CURVE_GEN_PHOTO, (AET_CURVE_GEN_PHOTO *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_CURVE_GEN_PHOTO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_MANUAL:
		ret = ioctl(isp_fd, AE_IOC_SET_MANUAL, (AET_MANUAL *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_MANUAL fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_PRIORITY:
		ret = ioctl(isp_fd, AE_IOC_SET_PRIORITY, (AET_PRIORITY *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_PRIORITY fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_ROI_WIN:
		ret = ioctl(isp_fd, AE_IOC_SET_ROI_WIN, (AET_ROI_WIN *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_ROI_WIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_SMART_IR:
		ret = ioctl(isp_fd, AE_IOC_SET_SMART_IR, (AET_SMART_IR *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_SMART_IR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_EXPT_BOUND:
		ret = ioctl(isp_fd, AE_IOC_SET_EXPT_BOUND, (AET_EXPT_BOUND *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_EXPT_BOUND fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_GAIN_BOUND:
		ret = ioctl(isp_fd, AE_IOC_SET_GAIN_BOUND, (AET_GAIN_BOUND *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_GAIN_BOUND fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_LA_WIN:
		ret = ioctl(isp_fd, AE_IOC_SET_LA_WIN, (AET_LA_WIN *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_LA_WIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_APERTURE_BOUND:
		ret = ioctl(isp_fd, AE_IOC_SET_APERTURE_BOUND, (AET_APERTURE_BOUND *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_APERTURE_BOUND fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_STITCH_ID:
		ret = ioctl(isp_fd, AE_IOC_SET_STITCH_ID, (AET_STITCH_ID*)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_STITCH_ID fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_DCIRIS:
		ret = ioctl(isp_fd, AE_IOC_SET_DCIRIS, (AET_DCIRIS*)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_DCIRIS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_NNSC_EXP_RATIO:
		ret = ioctl(isp_fd, AE_IOC_SET_EXP_RATIO, (AET_EXP_RATIO *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_EXP_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_SKIP_LA:
		ret = ioctl(isp_fd, AE_IOC_SET_SKIP_LA, (AET_SKIP_LA *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_SKIP_LA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AET_ITEM_EXT_PARAM:
		ret = ioctl(isp_fd, AE_IOC_SET_EXT_PARAM, (AET_EXTEND_PARAM *)p_param);
		if (ret < 0) {
			printf("AE_IOC_SET_EXT_PARAM fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	default:
		printf("vendor_ae_set not support item %d \n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&ae_mutex_lock);
	#endif

	return ret;
}

HD_RESULT vendor_isp_get_af(AFT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&af_mutex_lock);
	#endif

	switch (item) {
	case AFT_ITEM_VERSION:
		ret = ioctl(isp_fd, AF_IOC_GET_VERSION, (UINT32 *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_VERSION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_OPERATION:
		ret = ioctl(isp_fd, AF_IOC_GET_OPERATION, (AFT_OPERATION *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_OPERATION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_ENABLE:
		ret = ioctl(isp_fd, AF_IOC_GET_ENABLE, (AFT_ENABLE *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_ALG_METHOD:
		ret = ioctl(isp_fd, AF_IOC_GET_ALG_METHOD, (AFT_ALG_METHOD *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_ALG_METHOD fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_SHOT_MODE:
		ret = ioctl(isp_fd, AF_IOC_GET_SHOT_MODE, (AFT_SHOT_MODE *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_SHOT_MODE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_SEARCH_DIR:
		ret = ioctl(isp_fd, AF_IOC_GET_SEARCH_DIR, (AFT_SEARCH_DIR *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_SEARCH_DIR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_SKIP_FRAME:
		ret = ioctl(isp_fd, AF_IOC_GET_SKIP_FRAME, (AFT_SKIP_FRAME *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_SKIP_FRAME fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_THRES:
		ret = ioctl(isp_fd, AF_IOC_GET_THRES, (AFT_THRES *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_THRES fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_STEP_SIZE:
		ret = ioctl(isp_fd, AF_IOC_GET_STEP_SIZE, (AFT_STEP_SIZE *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_STEP_SIZE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_MAX_COUNT:
		ret = ioctl(isp_fd, AF_IOC_GET_MAX_COUNT, (AFT_MAX_COUNT *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_MAX_COUNT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_WIN_WEIGHT:
		ret = ioctl(isp_fd, AF_IOC_GET_WIN_WEIGHT, (AFT_WIN_WEIGHT *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_WIN_WEIGHT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_VA_STA:
		ret = ioctl(isp_fd, AF_IOC_GET_VA_STA, (AFT_VA_STA *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_VA_STA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_EXEC_STATUS:
		ret = ioctl(isp_fd, AF_IOC_GET_EXEC_STS, (AFT_EXEC_STS *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_EXEC_STS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_SEN_RESOLUTION:
		ret = ioctl(isp_fd, AF_IOC_GET_SEN_RESOLUTION, (AFT_SEN_RESO *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_SEN_RESOLUTION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_NIGHT_MODE:
		ret = ioctl(isp_fd, AF_IOC_GET_NIGHT_MODE, (AFT_NIGHT_MODE *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_NIGHT_MODE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_ALG_CMD:
		ret = ioctl(isp_fd, AF_IOC_GET_ALG_CMD, (AFT_ALG_CMD *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_ALG_CMD fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_VA_OPT:
		ret = ioctl(isp_fd, AF_IOC_GET_VA_OPT, (AFT_VA_OPT *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_VA_OPT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_BACKLASH_COMP:
		ret = ioctl(isp_fd, AF_IOC_GET_BACKLASH_COMP, (AFT_BACKLASH_COMP *)p_param);
		if (ret < 0) {
			printf("AF_IOC_GET_BACKLASH_COMP_STS fail! \n");
			ret = HD_ERR_NG;
		}
		break;
	default:
		printf("vendor_af_get not support item %u\n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&af_mutex_lock);
	#endif

	return ret;
}

HD_RESULT vendor_isp_set_af(AFT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&af_mutex_lock);
	#endif

	switch (item) {
	case AFT_ITEM_RLD_CONFIG:
		ret = ioctl(isp_fd, AF_IOC_RLD_CONFIG, (AFT_CFG_INFO *)p_param);
		if (ret < 0) {
			printf("AF_IOC_RLD_CONFIG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_RLD_DTSI:
		ret = ioctl(isp_fd, AF_IOC_RLD_DTSI, (AFT_DTSI_INFO *)p_param);
		if (ret < 0) {
			printf("AF_IOC_RLD_DTSI fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_OPERATION:
		ret = ioctl(isp_fd, AF_IOC_SET_OPERATION, (AFT_OPERATION *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_OPERATION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_ENABLE:
		ret = ioctl(isp_fd, AF_IOC_SET_ENABLE, (AFT_ENABLE *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_ALG_METHOD:
		ret = ioctl(isp_fd, AF_IOC_SET_ALG_METHOD, (AFT_ALG_METHOD *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_ALG_METHOD fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_SHOT_MODE:
		ret = ioctl(isp_fd, AF_IOC_SET_SHOT_MODE, (AFT_SHOT_MODE *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_SHOT_MODE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_SEARCH_DIR:
		ret = ioctl(isp_fd, AF_IOC_SET_SEARCH_DIR, (AFT_SEARCH_DIR *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_SEARCH_DIR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_SKIP_FRAME:
		ret = ioctl(isp_fd, AF_IOC_SET_SKIP_FRAME, (AFT_SKIP_FRAME *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_SKIP_FRAME fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_THRES:
		ret = ioctl(isp_fd, AF_IOC_SET_THRES, (AFT_THRES *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_THRES fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_STEP_SIZE:
		ret = ioctl(isp_fd, AF_IOC_SET_STEP_SIZE, (AFT_STEP_SIZE *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_STEP_SIZE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_MAX_COUNT:
		ret = ioctl(isp_fd, AF_IOC_SET_MAX_COUNT, (AFT_MAX_COUNT *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_MAX_COUNT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_WIN_WEIGHT:
		ret = ioctl(isp_fd, AF_IOC_SET_WIN_WEIGHT, (AFT_WIN_WEIGHT *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_WIN_WEIGHT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_RETRIGGER:
		ret = ioctl(isp_fd, AF_IOC_SET_RETRIGGER, (AFT_RETRIGGER *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_RETRIGGER fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_SEN_RESOLUTION:
		ret = ioctl(isp_fd, AF_IOC_SET_SEN_RESOLUTION, (AFT_SEN_RESO *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_SEN_RESOLUTION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_NIGHT_MODE:
		ret = ioctl(isp_fd, AF_IOC_SET_NIGHT_MODE, (AFT_NIGHT_MODE *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_NIGHT_MODE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_ALG_CMD:
		ret = ioctl(isp_fd, AF_IOC_SET_ALG_CMD, (AFT_ALG_CMD *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_ALG_CMD fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_VA_OPT:
		ret = ioctl(isp_fd, AF_IOC_SET_VA_OPT, (AFT_VA_OPT *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_VA_OPT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AFT_ITEM_BACKLASH_COMP:
		ret = ioctl(isp_fd, AF_IOC_SET_BACKLASH_COMP, (AFT_BACKLASH_COMP *)p_param);
		if (ret < 0) {
			printf("AF_IOC_SET_BACKLASH_COMP_STS fail! \n");
			ret = HD_ERR_NG;
		}
		break;
	default:
		printf("vendor_af_set not support item %u\n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&af_mutex_lock);
	#endif

	return ret;
}

HD_RESULT vendor_isp_get_awb(AWBT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&awb_mutex_lock);
	#endif

	switch (item) {
	case AWBT_ITEM_VERSION:
		ret = ioctl(isp_fd, AWB_IOC_GET_VERSION, (UINT32 *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_VERSION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_SCENE:
		ret = ioctl(isp_fd, AWB_IOC_GET_SCENE, (AWBT_SCENE_MODE *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_SCENE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_WB_RATIO:
		ret = ioctl(isp_fd, AWB_IOC_GET_USER_RATIO, (AWBT_WB_RATIO *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_WB_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_OPERATION:
		ret = ioctl(isp_fd, AWB_IOC_GET_OPERATION, (AWBT_OPERATION *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_OPERATION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_STITCH_ID:
		ret = ioctl(isp_fd, AWB_IOC_GET_STITCH_ID, (AWBT_STITCH_ID *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_STITCH_ID fail! \n");
			ret = HD_ERR_NG;
		}
		break;
	case AWBT_ITEM_CA_TH:
		ret = ioctl(isp_fd, AWB_IOC_GET_CA_TH, (AWBT_CA_TH *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_CA_TH fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_TH:
		ret = ioctl(isp_fd, AWB_IOC_GET_TH, (AWBT_TH *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_TH fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_LV:
		ret = ioctl(isp_fd, AWB_IOC_GET_LV, (AWBT_LV *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_CT_WEIGHT:
		ret = ioctl(isp_fd, AWB_IOC_GET_CT_WEIGHT, (AWBT_CT_WEIGHT *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_CT_WEIGHT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_TARGET:
		ret = ioctl(isp_fd, AWB_IOC_GET_TARGET, (AWBT_TARGET *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_TARGET fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_CT_INFO:
		ret = ioctl(isp_fd, AWB_IOC_GET_CT_INFO, (AWBT_CT_INFO *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_CT_INFO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_MWB_GAIN:
		ret = ioctl(isp_fd, AWB_IOC_GET_MWB_GAIN, (AWBT_MWB_GAIN *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_MWB_GAIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_CONVERGE:
		ret = ioctl(isp_fd, AWB_IOC_GET_CONVERGE, (AWBT_CONVERGE *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_CONVERGE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_EXPAND_BLOCK:
		ret = ioctl(isp_fd, AWB_IOC_GET_EXPAND_BLOCK, (AWBT_EXPAND_BLOCK *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_EXPAND_BLOCK fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_MANUAL:
		ret = ioctl(isp_fd, AWB_IOC_GET_MANUAL, (AWBT_MANUAL *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_MANUAL fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_STATUS:
		ret = ioctl(isp_fd, AWB_IOC_GET_STATUS, (AWBT_STATUS *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_STATUS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_CA:
		ret = ioctl(isp_fd, AWB_IOC_GET_CA, (AWBT_CA *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_CA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_FLAG:
		ret = ioctl(isp_fd, AWB_IOC_GET_FLAG, (AWBT_FLAG *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_FLAG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_LUMA_WEIGHT:
		ret = ioctl(isp_fd, AWB_IOC_GET_LUMA_WEIGHT, (AWBT_LUMA_WEIGHT *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_LUMA_WEIGHT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_KGAIN_RATIO:
		ret = ioctl(isp_fd, AWB_IOC_GET_KGAIN_RATIO, (AWBT_KGAIN_RATIO*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_KGAIN_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_CT_TO_CGAIN:
		ret = ioctl(isp_fd, AWB_IOC_GET_CGAIN_FROM_CT, (AWBT_CT_TO_CGAIN*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_CGAIN_FROM_CT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_WIN:
		ret = ioctl(isp_fd, AWB_IOC_GET_WIN, (AWBT_WIN*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_WIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_EXPAND_CT_ENABLE:
		ret = ioctl(isp_fd, AWB_IOC_GET_EXPAND_CT_ENABLE, (AWBT_EXPAND_CT_ENABLE*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_EXPAND_CT_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_AUTO_WEIGHT_ENABLE:
		ret = ioctl(isp_fd, AWB_IOC_GET_AUTO_WEIGHT_ENABLE, (AWBT_AUTO_WEIGHT_ENABLE*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_AUTO_WEIGHT_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_GRAY_WORLD_ENABLE:
		ret = ioctl(isp_fd, AWB_IOC_GET_GRAY_WORLD_ENABLE, (AWBT_GRAY_WORLD_ENABLE*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_GRAY_WORLD_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_NNSC_GREEN_REMOVE:
		ret = ioctl(isp_fd, AWB_IOC_GET_GREEN_REMOVE, (AWBT_GREEN_REMOVE*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_GREEN_REMOVE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_NNSC_SKIN_REMOVE:
		ret = ioctl(isp_fd, AWB_IOC_GET_SKIN_REMOVE, (AWBT_SKIN_REMOVE*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_GET_SKIN_REMOVE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	default:
		printf("vendor_awb_get not support item %d \n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&awb_mutex_lock);
	#endif

	return ret;
}

HD_RESULT vendor_isp_set_awb(AWBT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&awb_mutex_lock);
	#endif

	switch (item) {
	case AWBT_ITEM_RLD_CONFIG:
		ret = ioctl(isp_fd, AWB_IOC_RLD_CONFIG, (AWBT_CFG_INFO *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_RLD_CONFIG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_RLD_DTSI:
		ret = ioctl(isp_fd, AWB_IOC_RLD_DTSI, (AWBT_DTSI_INFO *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_RLD_DTSI fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_SCENE:
		ret = ioctl(isp_fd, AWB_IOC_SET_SCENE, (AWBT_SCENE_MODE *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_SCENE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_WB_RATIO:
		ret = ioctl(isp_fd, AWB_IOC_SET_USER_RATIO, (AWBT_WB_RATIO *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_WB_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_OPERATION:
		ret = ioctl(isp_fd, AWB_IOC_SET_OPERATION, (AWBT_OPERATION *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_OPERATION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_STITCH_ID:
		ret = ioctl(isp_fd, AWB_IOC_SET_STITCH_ID, (AWBT_STITCH_ID *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_STITCH_ID fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_CA_TH:
		ret = ioctl(isp_fd, AWB_IOC_SET_CA_TH, (AWBT_CA_TH *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_CA_TH fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_TH:
		ret = ioctl(isp_fd, AWB_IOC_SET_TH, (AWBT_TH *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_TH fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_LV:
		ret = ioctl(isp_fd, AWB_IOC_SET_LV, (AWBT_LV *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_CT_WEIGHT:
		ret = ioctl(isp_fd, AWB_IOC_SET_CT_WEIGHT, (AWBT_CT_WEIGHT *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_CT_WEIGHT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_TARGET:
		ret = ioctl(isp_fd, AWB_IOC_SET_TARGET, (AWBT_TARGET *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_TARGET fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_CT_INFO:
		ret = ioctl(isp_fd, AWB_IOC_SET_CT_INFO, (AWBT_CT_INFO *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_CT_INFO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_MWB_GAIN:
		ret = ioctl(isp_fd, AWB_IOC_SET_MWB_GAIN, (AWBT_MWB_GAIN *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_MWB_GAIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_CONVERGE:
		ret = ioctl(isp_fd, AWB_IOC_SET_CONVERGE, (AWBT_CONVERGE *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_CONVERGE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_EXPAND_BLOCK:
		ret = ioctl(isp_fd, AWB_IOC_SET_EXPAND_BLOCK, (AWBT_EXPAND_BLOCK *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_EXPAND_BLOCK fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_MANUAL:
		ret = ioctl(isp_fd, AWB_IOC_SET_MANUAL, (AWBT_MANUAL *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_MANUAL fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_LUMA_WEIGHT:
		ret = ioctl(isp_fd, AWB_IOC_SET_LUMA_WEIGHT, (AWBT_LUMA_WEIGHT *)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_LUMA_WEIGHT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_KGAIN_RATIO:
		ret = ioctl(isp_fd, AWB_IOC_SET_KGAIN_RATIO, (AWBT_KGAIN_RATIO*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_KGAIN_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_WIN:
		ret = ioctl(isp_fd, AWB_IOC_SET_WIN, (AWBT_WIN*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_WIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_EXPAND_CT_ENABLE:
		ret = ioctl(isp_fd, AWB_IOC_SET_EXPAND_CT_ENABLE, (AWBT_EXPAND_CT_ENABLE*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_EXPAND_CT_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_AUTO_WEIGHT_ENABLE:
		ret = ioctl(isp_fd, AWB_IOC_SET_AUTO_WEIGHT_ENABLE, (AWBT_AUTO_WEIGHT_ENABLE*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_AUTO_WEIGHT_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_GRAY_WORLD_ENABLE:
		ret = ioctl(isp_fd, AWB_IOC_SET_GRAY_WORLD_ENABLE, (AWBT_GRAY_WORLD_ENABLE*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_GRAY_WORLD_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_NNSC_GREEN_REMOVE:
		ret = ioctl(isp_fd, AWB_IOC_SET_GREEN_REMOVE, (AWBT_GREEN_REMOVE*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_GREEN_REMOVE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case AWBT_ITEM_NNSC_SKIN_REMOVE:
		ret = ioctl(isp_fd, AWB_IOC_SET_SKIN_REMOVE, (AWBT_SKIN_REMOVE*)p_param);
		if (ret < 0) {
			printf("AWB_IOC_SET_SKIN_REMOVE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	default:
		printf("vendor_awb_set not support item %d \n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&awb_mutex_lock);
	#endif

	return ret;
}

HD_RESULT vendor_isp_get_iq(IQT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&iq_mutex_lock);
	#endif

	switch (item) {
	case IQT_ITEM_VERSION:
		ret = ioctl(isp_fd, IQ_IOC_GET_VERSION, (UINT32 *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_VERSION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NR_LV:
		ret = ioctl(isp_fd, IQ_IOC_GET_NR_LV, (IQT_NR_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_NR_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHARPNESS_LV:
		ret = ioctl(isp_fd, IQ_IOC_GET_SHARPNESS_LV, (IQT_SHARPNESS_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SHARPNESS_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SATURATION_LV:
		ret = ioctl(isp_fd, IQ_IOC_GET_SATURATION_LV, (IQT_SATURATION_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SATURATION_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CONTRAST_LV:
		ret = ioctl(isp_fd, IQ_IOC_GET_CONTRAST_LV, (IQT_CONTRAST_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_CONTRAST_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_BRIGHTNESS_LV:
		ret = ioctl(isp_fd, IQ_IOC_GET_BRIGHTNESS_LV, (IQT_BRIGHTNESS_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_BRIGHTNESS_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NIGHT_MODE:
		ret = ioctl(isp_fd, IQ_IOC_GET_NIGHT_MODE, (IQT_NIGHT_MODE *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_NIGHT_MODE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_YCC_FORMAT:
		ret = ioctl(isp_fd, IQ_IOC_GET_YCC_FORMAT, (IQT_YCC_FORMAT *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_YCC_FORMAT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_OPERATION:
		ret = ioctl(isp_fd, IQ_IOC_GET_OPERATION, (IQT_OPERATION *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_OPERATION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_IMAGEEFFECT:
		ret = ioctl(isp_fd, IQ_IOC_GET_IMAGEEFFECT, (IQT_IMAGEEFFECT *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_IMAGEEFFECT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CCID:
		ret = ioctl(isp_fd, IQ_IOC_GET_CCID, (IQT_CCID *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_CCID fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_HUE_SHIFT:
		ret = ioctl(isp_fd, IQ_IOC_GET_HUE_SHIFT, (IQT_HUE_SHIFT *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_HUE_SHIFT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHDR_TONE_LV:
		ret = ioctl(isp_fd, IQ_IOC_GET_SHDR_TONE_LV, (IQT_SHDR_TONE_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SHDR_TONE_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_3DNR_LV:
		ret = ioctl(isp_fd, IQ_IOC_GET_3DNR_LV, (IQT_3DNR_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_3DNR_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_GAMMA_LV:
		ret = ioctl(isp_fd, IQ_IOC_GET_GAMMA_LV, (IQT_GAMMA_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_GAMMA_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_OB_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_OB, (IQT_OB_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_OB fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_NR, (IQT_NR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_NR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CFA_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_CFA, (IQT_CFA_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_CFA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_VA_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_VA, (IQT_VA_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_VA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_GAMMA_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_GAMMA, (IQT_GAMMA_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_GAMMA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_GAMMA_EXT_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_GAMMA_EXT, (IQT_GAMMA_EXT_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_GAMMA_EXT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CCM_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_CCM, (IQT_CCM_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_CCM fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CCM_EXT_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_CCM_EXT, (IQT_CCM_EXT_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_CCM_EXT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CCM_EXT2_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_CCM_EXT2, (IQT_CCM_EXT2_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_CCM_EXT2 fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_COLOR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_COLOR, (IQT_COLOR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_COLOR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CONTRAST_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_CONTRAST, (IQT_CONTRAST_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_CONTRAST fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_EDGE_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_EDGE, (IQT_EDGE_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_EDGE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_3DNR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_3DNR, (IQT_3DNR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_3DNR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_DPC_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_DPC, (IQT_DPC_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_DPC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_EXPAND_DPC_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_EXPAND_DPC, (IQT_EXPAND_DPC_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_EXPAND_DPC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHADING_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_SHADING, (IQT_SHADING_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SHADING fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_LDC_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_LDC, (IQT_LDC_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_LDC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_PFR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_PFR, (IQT_PFR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_PFR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_WDR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_WDR, (IQT_WDR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_WDR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_DEFOG_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_DEFOG, (IQT_DEFOG_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_DEFOG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHDR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_SHDR, (IQT_SHDR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SHDR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHDR_EXT_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_SHDR_EXT, (IQT_SHDR_EXT_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SHDR_EXT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_RGBIR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_RGBIR, (IQT_RGBIR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_RGBIR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_COMPANDING_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_COMPANDING, (IQT_COMPANDING_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_COMPANDING fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHDR_MODE:
		ret = ioctl(isp_fd, IQ_IOC_GET_SHDR_MODE, (IQT_SHDR_MODE *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SHDR_MODE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_3DNR_MISC_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_3DNR_MISC, (IQT_3DNR_MISC_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_3DNR_MISC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_POST_3DNR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_POST_3DNR, (IQT_POST_3DNR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_POST_3DNR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_DR_LEVEL:
		ret = ioctl(isp_fd, IQ_IOC_GET_DR_LEVEL, (IQT_DR_LEVEL *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_DR_LEVEL fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_RGBIR_ENH_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_RGBIR_ENH, (IQT_RGBIR_ENH_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_RGBIR_ENH fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_RGBIR_ENH_ISO:
		ret = ioctl(isp_fd, IQ_IOC_GET_RGBIR_ENH_ISO, (IQT_RGBIR_ENH_ISO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_RGBIR_ENH_ISO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_MD_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_MD, (IQT_MD_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_MD fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_EDGE_REGION_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_EDGE_REGION, (IQT_EDGE_REGION_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_EDGE_REGION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHADING_INTER_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_SHADING_INTER, (IQT_SHADING_INTER_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SHADING_INTER fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHADING_EXT_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_SHADING_EXT, (IQT_SHADING_EXT_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SHADING_EXT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHADING_VIG_EXT_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_SHADING_VIG_EXT, (IQT_SHADING_VIG_EXT_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SHADING_VIG_EXT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHADING_DITH_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_SHADING_DITH, (IQT_SHADING_DITH_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SHADING_DITH fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CST_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_CST, (IQT_CST_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_CST fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_STRIPE_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_STRIPE, (IQT_STRIPE_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_STRIPE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_LCA_SUB_RATIO_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_GET_LCA_SUB_RATIO, (IQT_LCA_SUB_RATIO_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_LCA_SUB_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NNSC_DARK_ENH_RATIO:
		ret = ioctl(isp_fd, IQ_IOC_GET_DARK_ENH_RATIO, (IQT_DARK_ENH_RATIO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_DARK_ENH_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NNSC_CONTRAST_ENH_RATIO:
		ret = ioctl(isp_fd, IQ_IOC_GET_CONTRAST_ENH_RATIO, (IQT_CONTRAST_ENH_RATIO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_CONTRAST_ENH_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NNSC_GREEN_ENH_RATIO:
		ret = ioctl(isp_fd, IQ_IOC_GET_GREEN_ENH_RATIO, (IQT_GREEN_ENH_RATIO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_GREEN_ENH_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NNSC_SKIN_ENH_RATIO:
		ret = ioctl(isp_fd, IQ_IOC_GET_SKIN_ENH_RATIO, (IQT_SKIN_ENH_RATIO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_GET_SKIN_ENH_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	default:
		printf("vendor_iq_get not support item %d \n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&iq_mutex_lock);
	#endif

	return ret;
}

HD_RESULT vendor_isp_set_iq(IQT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&iq_mutex_lock);
	#endif

	switch (item) {
	case IQT_ITEM_RLD_CONFIG:
		ret = ioctl(isp_fd, IQ_IOC_RLD_CONFIG, (IQT_CFG_INFO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_RLD_CONFIG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_RLD_DTSI:
		ret = ioctl(isp_fd, IQ_IOC_RLD_DTSI, (IQT_DTSI_INFO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_RLD_DTSI fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NR_LV:
		ret = ioctl(isp_fd, IQ_IOC_SET_NR_LV, (IQT_NR_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_NR_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHARPNESS_LV:
		ret = ioctl(isp_fd, IQ_IOC_SET_SHARPNESS_LV, (IQT_SHARPNESS_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SHARPNESS_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SATURATION_LV:
		ret = ioctl(isp_fd, IQ_IOC_SET_SATURATION_LV, (IQT_SATURATION_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SATURATION_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CONTRAST_LV:
		ret = ioctl(isp_fd, IQ_IOC_SET_CONTRAST_LV, (IQT_CONTRAST_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_CONTRAST_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_BRIGHTNESS_LV:
		ret = ioctl(isp_fd, IQ_IOC_SET_BRIGHTNESS_LV, (IQT_BRIGHTNESS_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_BRIGHTNESS_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NIGHT_MODE:
		ret = ioctl(isp_fd, IQ_IOC_SET_NIGHT_MODE, (IQT_NIGHT_MODE *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_NIGHT_MODE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_YCC_FORMAT:
		ret = ioctl(isp_fd, IQ_IOC_SET_YCC_FORMAT, (IQT_YCC_FORMAT *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_YCC_FORMAT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_OPERATION:
		ret = ioctl(isp_fd, IQ_IOC_SET_OPERATION, (IQT_OPERATION *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_OPERATION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_IMAGEEFFECT:
		ret = ioctl(isp_fd, IQ_IOC_SET_IMAGEEFFECT, (IQT_IMAGEEFFECT *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_IMAGEEFFECT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CCID:
		ret = ioctl(isp_fd, IQ_IOC_SET_CCID, (IQT_CCID *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_CCID fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_HUE_SHIFT:
		ret = ioctl(isp_fd, IQ_IOC_SET_HUE_SHIFT, (IQT_HUE_SHIFT *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_HUE_SHIFT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHDR_TONE_LV:
		ret = ioctl(isp_fd, IQ_IOC_SET_SHDR_TONE_LV, (IQT_SHDR_TONE_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SHDR_TONE_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_3DNR_LV:
		ret = ioctl(isp_fd, IQ_IOC_SET_3DNR_LV, (IQT_3DNR_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_3DNR_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_GAMMA_LV:
		ret = ioctl(isp_fd, IQ_IOC_SET_GAMMA_LV, (IQT_GAMMA_LV *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_GAMMA_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_OB_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_OB, (IQT_OB_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_OB fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_NR, (IQT_NR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_NR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CFA_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_CFA, (IQT_CFA_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_CFA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_VA_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_VA, (IQT_VA_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_VA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_GAMMA_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_GAMMA, (IQT_GAMMA_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_GAMMA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_GAMMA_EXT_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_GAMMA_EXT, (IQT_GAMMA_EXT_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_GAMMA_EXT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CCM_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_CCM, (IQT_CCM_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_CCM fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CCM_EXT_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_CCM_EXT, (IQT_CCM_EXT_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_CCM_EXT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CCM_EXT2_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_CCM_EXT2, (IQT_CCM_EXT2_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_CCM_EXT2 fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_COLOR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_COLOR, (IQT_COLOR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_COLOR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CONTRAST_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_CONTRAST, (IQT_CONTRAST_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_CONTRAST fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_EDGE_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_EDGE, (IQT_EDGE_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SSET_EDGE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_3DNR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_3DNR, (IQT_3DNR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_3DNR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_DPC_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_DPC, (IQT_DPC_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_DPC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_EXPAND_DPC_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_EXPAND_DPC, (IQT_EXPAND_DPC_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_EXPAND_DPC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHADING_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_SHADING, (IQT_SHADING_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SHADING fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_LDC_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_LDC, (IQT_LDC_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_LDC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_PFR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_PFR, (IQT_PFR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_PFR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_WDR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_WDR, (IQT_WDR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_WDR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_DEFOG_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_DEFOG, (IQT_DEFOG_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_DEFOG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHDR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_SHDR, (IQT_SHDR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SHDR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHDR_EXT_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_SHDR_EXT, (IQT_SHDR_EXT_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SHDR_EXT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_RGBIR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_RGBIR, (IQT_RGBIR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_RGBIR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_COMPANDING_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_COMPANDING, (IQT_COMPANDING_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_COMPANDING fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHDR_MODE:
		ret = ioctl(isp_fd, IQ_IOC_SET_SHDR_MODE, (IQT_SHDR_MODE *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SHDR_MODE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_3DNR_MISC_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_3DNR_MISC, (IQT_3DNR_MISC_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_3DNR_MISC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_POST_3DNR_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_POST_3DNR, (IQT_POST_3DNR_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_POST_3DNR fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_RGBIR_ENH_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_RGBIR_ENH, (IQT_RGBIR_ENH_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_RGBIR_ENH fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_MD_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_MD, (IQT_MD_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_MD fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_EDGE_REGION_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_EDGE_REGION, (IQT_EDGE_REGION_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SSET_EDGE_REGION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHADING_INTER_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_SHADING_INTER, (IQT_SHADING_INTER_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SHADING_INTER fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHADING_EXT_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_SHADING_EXT, (IQT_SHADING_EXT_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SHADING_EXT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHADING_VIG_EXT_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_SHADING_VIG_EXT, (IQT_SHADING_VIG_EXT_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SHADING_VIG_EXT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_SHADING_DITH_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_SHADING_DITH, (IQT_SHADING_DITH_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SHADING_DITH fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_CST_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_CST, (IQT_CST_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_CST fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_STRIPE_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_STRIPE, (IQT_STRIPE_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_STRIPE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_LCA_SUB_RATIO_PARAM:
		ret = ioctl(isp_fd, IQ_IOC_SET_LCA_SUB_RATIO, (IQT_LCA_SUB_RATIO_PARAM *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_LCA_SUB_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NNSC_DARK_ENH_RATIO:
		ret = ioctl(isp_fd, IQ_IOC_SET_DARK_ENH_RATIO, (IQT_DARK_ENH_RATIO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_DET_DARK_ENH_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NNSC_CONTRAST_ENH_RATIO:
		ret = ioctl(isp_fd, IQ_IOC_SET_CONTRAST_ENH_RATIO, (IQT_CONTRAST_ENH_RATIO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_CONTRAST_ENH_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NNSC_GREEN_ENH_RATIO:
		ret = ioctl(isp_fd, IQ_IOC_SET_GREEN_ENH_RATIO, (IQT_GREEN_ENH_RATIO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_GREEN_ENH_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case IQT_ITEM_NNSC_SKIN_ENH_RATIO:
		ret = ioctl(isp_fd, IQ_IOC_SET_SKIN_ENH_RATIO, (IQT_SKIN_ENH_RATIO *)p_param);
		if (ret < 0) {
			printf("IQ_IOC_SET_SKIN_ENH_RATIO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	default:
		printf("vendor_iq_set not support item %d \n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&iq_mutex_lock);
	#endif

	return ret;
}

HD_RESULT vendor_isp_get_common(ISPT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	if (item != ISPT_ITEM_WAIT_VD) {
		pthread_mutex_lock(&isp_mutex_lock);
	}
	#endif

	switch (item) {
	case ISPT_ITEM_VERSION:
		ret = ioctl(isp_fd, ISP_IOC_GET_VERSION, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_VERSION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_FUNC:
		ret = ioctl(isp_fd, ISP_IOC_GET_FUNC, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_FUNC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_YUV:
		ret = ioctl(isp_fd, ISP_IOC_GET_YUV, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_YUV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_RAW:
		ret = ioctl(isp_fd, ISP_IOC_GET_RAW, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_RAW fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_FRAME:
		ret = ioctl(isp_fd, ISP_IOC_GET_FRAME, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_FRAME fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_INFO:
		ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_INFO, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_SENSOR_INFO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_REG:
		ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_REG, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_SENSOR_REG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_MODE_INFO:
		ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_MODE_INFO, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_SENSOR_MODE_INFO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_CHIP_INFO:
		ret = ioctl(isp_fd, ISP_IOC_GET_CHIP_INFO, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_CHIP_INFO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_CA_DATA:
		ret = ioctl(isp_fd, ISP_IOC_GET_CA_DATA, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_CA_DATA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_LA_DATA:
		ret = ioctl(isp_fd, ISP_IOC_GET_LA_DATA, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_LA_DATA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_VA_DATA:
		ret = ioctl(isp_fd, ISP_IOC_GET_VA_DATA, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_VA_DATA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_VA_INDEP_DATA:
		ret = ioctl(isp_fd, ISP_IOC_GET_VA_INDEP_DATA, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_VA_INDEP_DATA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_WAIT_VD:
		ret = ioctl(isp_fd, ISP_IOC_GET_WAIT_VD, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_WAIT_VD fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_EXPT:
		ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_EXPT, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_SENSOR_EXPT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_GAIN:
		ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_GAIN, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_SENSOR_GAIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_D_GAIN:
		ret = ioctl(isp_fd, ISP_IOC_GET_D_GAIN, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_D_GAIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_C_GAIN:
		ret = ioctl(isp_fd, ISP_IOC_GET_C_GAIN, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_C_GAIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_TOTAL_GAIN:
		ret = ioctl(isp_fd, ISP_IOC_GET_TOTAL_GAIN, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_TOTAL_GAIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_LV:
		ret = ioctl(isp_fd, ISP_IOC_GET_LV, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_CT:
		ret = ioctl(isp_fd, ISP_IOC_GET_CT, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_CT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_MOTOR_IRIS:
		ret = ioctl(isp_fd, ISP_IOC_GET_MOTOR_IRIS, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_MOTOR_IRIS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_MOTOR_FOCUS:
		ret = ioctl(isp_fd, ISP_IOC_GET_MOTOR_FOCUS, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_MOTOR_FOCUS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_MOTOR_ZOOM:
		ret = ioctl(isp_fd, ISP_IOC_GET_MOTOR_ZOOM, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_MOTOR_ZOOM fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_MOTOR_MISC:
		ret = ioctl(isp_fd, ISP_IOC_GET_MOTOR_MISC, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_MOTOR_MISC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_DIRECTION:
		ret = ioctl(isp_fd, ISP_IOC_GET_SENSOR_DIRECTION, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_SENSOR_DIRECTION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_HISTO_DATA:
		ret = ioctl(isp_fd, ISP_IOC_GET_HISTO_DATA, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_HISTO_DATA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_3DNR_STA:
		ret = ioctl(isp_fd, ISP_IOC_GET_3DNR_STA, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_3DNR_STA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_IR_INFO:
		ret = ioctl(isp_fd, ISP_IOC_GET_IR_INFO, (ISPT_IR_INFO *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_IR_INFO fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_MD_DATA:
		ret = ioctl(isp_fd, ISP_IOC_GET_MD_DATA, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_MD_DATA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_MD_STA:
		ret = ioctl(isp_fd, ISP_IOC_GET_MD_STA, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_MD_STA fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_CA_ENABLE:
		ret = ioctl(isp_fd, ISP_IOC_GET_CA_ENABLE, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_CA_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_LA_ENABLE:
		ret = ioctl(isp_fd, ISP_IOC_GET_LA_ENABLE, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_GET_LA_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	default:
		printf("vendor_common_get not support item %d \n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	if (item != ISPT_ITEM_WAIT_VD) {
		pthread_mutex_unlock(&isp_mutex_lock);
	}
	#endif

	return ret;
}

HD_RESULT vendor_isp_set_common(ISPT_ITEM item, VOID *p_param)
{
	INT32 ret = HD_OK;

	if (vendor_api_init == FALSE) {
		return HD_ERR_UNINIT;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_lock(&isp_mutex_lock);
	#endif

	switch (item) {
	case ISPT_ITEM_YUV:
		ret = ioctl(isp_fd, ISP_IOC_SET_YUV, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_YUV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_RAW:
		ret = ioctl(isp_fd, ISP_IOC_SET_RAW, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_RAW fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_REG:
		ret = ioctl(isp_fd, ISP_IOC_SET_SENSOR_REG, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_SENSOR_REG fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_EXPT:
		ret = ioctl(isp_fd, ISP_IOC_SET_SENSOR_EXPT, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_SENSOR_EXPT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_GAIN:
		ret = ioctl(isp_fd, ISP_IOC_SET_SENSOR_GAIN, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_SENSOR_GAIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_D_GAIN:
		ret = ioctl(isp_fd, ISP_IOC_SET_D_GAIN, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_D_GAIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_C_GAIN:
		ret = ioctl(isp_fd, ISP_IOC_SET_C_GAIN, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_C_GAIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_TOTAL_GAIN:
		ret = ioctl(isp_fd, ISP_IOC_SET_TOTAL_GAIN, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_TOTAL_GAIN fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_LV:
		ret = ioctl(isp_fd, ISP_IOC_SET_LV, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_LV fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_CT:
		ret = ioctl(isp_fd, ISP_IOC_SET_CT, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_CT fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_MOTOR_IRIS:
		ret = ioctl(isp_fd, ISP_IOC_SET_MOTOR_IRIS, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_MOTOR_IRIS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_MOTOR_FOCUS:
		ret = ioctl(isp_fd, ISP_IOC_SET_MOTOR_FOCUS, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_MOTOR_FOCUS fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_MOTOR_ZOOM:
		ret = ioctl(isp_fd, ISP_IOC_SET_MOTOR_ZOOM, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_MOTOR_ZOOM fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_MOTOR_MISC:
		ret = ioctl(isp_fd, ISP_IOC_SET_MOTOR_MISC, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_MOTOR_MISC fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_DIRECTION:
		ret = ioctl(isp_fd, ISP_IOC_SET_SENSOR_DIRECTION, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_SENSOR_DIRECTION fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_SLEEP:
		ret = ioctl(isp_fd, ISP_IOC_SET_SENSOR_SLEEP, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_SENSOR_SLEEP fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_SENSOR_WAKEUP:
		ret = ioctl(isp_fd, ISP_IOC_SET_SENSOR_WAKEUP, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_SENSOR_WAKEUP fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_CA_ENABLE:
		ret = ioctl(isp_fd, ISP_IOC_SET_CA_ENABLE, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_CA_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	case ISPT_ITEM_LA_ENABLE:
		ret = ioctl(isp_fd, ISP_IOC_SET_LA_ENABLE, (UINT32 *)p_param);
		if (ret < 0) {
			printf("ISP_IOC_SET_LA_ENABLE fail! \n");
			ret = HD_ERR_NG;
		}
		break;

	default:
		printf("vendor_common_set not support item %d \n", item);
		break;
	}

	#if (PTHREAD_MUTEX == ENABLE)
	pthread_mutex_unlock(&isp_mutex_lock);
	#endif

	return ret;
}

