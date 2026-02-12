#include "vendor_ive.h"
#include "hdal.h"
#include "kflow_common/nvtmpp.h"
#include "kflow_common/nvtmpp_ioctl.h"
#include "ive_ioctl.h"
#include "vendor_ive_version.h"
#include <string.h>

#if defined(__LINUX)
#include <sys/ioctl.h>
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
static BOOL ive_api_init;
static INT32 ive_fd = -1;
static INT32 nvtmpp_fd = -1;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT32 vendor_ive_init(void)
{
	INT32 ret;
	VENDOR_IVE_OPENCFG ive_opencfg;
	if (ive_api_init == FALSE) {
		ive_api_init = TRUE;

		// open ive device
		ive_fd= NVTIVE_OPEN("/dev/nvt_ive_module0", O_RDWR);
		if (ive_fd < 0) {
			printf("[VENDOR_IVE] Open kdrv_ive fail!\n");
			return HD_ERR_NG;
		}

		// open nvtmpp device
		nvtmpp_fd = NVTMPP_OPEN("/dev/nvtmpp", O_RDWR|O_SYNC);
		if (nvtmpp_fd < 0) {
			printf("open /dev/nvtmpp error\r\n");
			return HD_ERR_SYS;
		}

		ive_opencfg.ive_clock_sel = 480;
		ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_OPENCFG, &ive_opencfg);
		if (ret < 0) {
			printf("[VENDOR_IVE] ive opencfg fail! \n");
			return ret;
		}

		ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_OPEN, NULL);
		if (ret < 0) {
			printf("[VENDOR_IVE] ive open fail! \n");
			return ret;
		}

		return 0;
	} else {
		printf("[VENDOR_IVE] ive module is already initialised. \n");
		return -1;
	}
}

INT32 vendor_ive_uninit(void)
{
	INT32 ret;
	if (ive_api_init == TRUE) {
		ive_api_init = FALSE;
		ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_CLOSE, NULL);
		if (ret < 0) {
			printf("[VENDOR_IVE] ive close fail! \n");
			return ret;
		}
		if (ive_fd != -1) {
			NVTIVE_CLOSE(ive_fd);
		}
		if (nvtmpp_fd != -1) {
            NVTMPP_CLOSE(nvtmpp_fd);
        }
		return 0;
	} else {
		printf("[VENDOR_IVE] ive module is not initialised yet. \n");
		return -1;
	}
}

INT32 vendor_ive_set_param(VENDOR_IVE_FUNC param_id, void *p_param, UINT32 id)
{
	int ret = 0;
	VENDOR_IVE_IMG_IN_DMA_INFO* tmp_addr_param;
	VENDOR_IVE_OUTSEL_PARAM* tmp_outsel;
	VENDOR_IVE_IRV_PARAM* tmp_irv;
	VENDOR_IVE_FLOWCT_PARAM* tmp_flowct;
#if VENDOR_IVE_IOREMAP_IN_KERNEL
#else
	NVTMPP_IOC_VB_PA_TO_VA_S nvtmpp_msg;
#endif
	unsigned int* tmp_buf = NULL;
	unsigned int* p_id = NULL;

	if (ive_api_init == FALSE) {
		printf("[VENDOR_IVE] ive should be init before set param\n");
		return -1;
	}
	if (p_param == NULL) {
		printf("[VENDOR_IVE] p_param is NULL!\n");
		return -1;
	}


	switch (param_id) {
		case VENDOR_IVE_INPUT_INFO:
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_IN_IMG_INFO) + 4);
			memcpy((void *)tmp_buf, (void *)p_param, sizeof(VENDOR_IVE_IN_IMG_INFO));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_IN_IMG_INFO));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_IMG_INFO, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_INPUT_INFO fail! \n");
				return ret;
			}

			break;
		case VENDOR_IVE_INPUT_ADDR:
			tmp_addr_param = (VENDOR_IVE_IMG_IN_DMA_INFO*)p_param;
#if VENDOR_IVE_IOREMAP_IN_KERNEL
#else
			nvtmpp_msg.pa = tmp_addr_param->addr;
			ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);

			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_INPUT_ADDR fail in get kernel va! \n");
				return ret;
			}
			tmp_addr_param->addr = nvtmpp_msg.va;
#endif
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_IMG_IN_DMA_INFO) + 4);
			memcpy((void *)tmp_buf, (void *)tmp_addr_param, sizeof(VENDOR_IVE_IMG_IN_DMA_INFO));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_IMG_IN_DMA_INFO));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_IMG_DMA_IN, (void *)tmp_buf);

			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_INPUT_ADDR fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_OUTPUT_ADDR:
			tmp_addr_param = (VENDOR_IVE_IMG_OUT_DMA_INFO*)p_param;
#if VENDOR_IVE_IOREMAP_IN_KERNEL
#else
			nvtmpp_msg.pa = tmp_addr_param->addr;
			ret = NVTMPP_IOCTL(nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_OUTPUT_ADDR fail in get kernel va! \n");
				return ret;
			}
			tmp_addr_param->addr = nvtmpp_msg.va;
#endif
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_IMG_OUT_DMA_INFO) + 4);
			memcpy((void *)tmp_buf, (void *)tmp_addr_param, sizeof(VENDOR_IVE_IMG_OUT_DMA_INFO));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_IMG_OUT_DMA_INFO));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_IMG_DMA_OUT, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_OUTPUT_ADDR fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_GENERAL_FILTER:
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_GENERAL_FILTER_PARAM) + 4);
			memcpy((void *)tmp_buf, (void *)p_param, sizeof(VENDOR_IVE_GENERAL_FILTER_PARAM));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_GENERAL_FILTER_PARAM));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_GENERAL_FILTER, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_GENERAL_FILTER fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_MEDIAN_FILTER:
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_MEDIAN_FILTER_PARAM) + 4);
			memcpy((void *)tmp_buf, (void *)p_param, sizeof(VENDOR_IVE_MEDIAN_FILTER_PARAM));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_MEDIAN_FILTER_PARAM));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_MEDIAN_FILTER, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_MEDIAN_FILTER fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_EDGE_FILTER:
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_EDGE_FILTER_PARAM) + 4);
			memcpy((void *)tmp_buf, (void *)p_param, sizeof(VENDOR_IVE_EDGE_FILTER_PARAM));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_EDGE_FILTER_PARAM));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_EDGE_FILTER, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_EDGE_FILTER fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_NON_MAX_SUP:
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_NON_MAX_SUP_PARAM) + 4);
			memcpy((void *)tmp_buf, (void *)p_param, sizeof(VENDOR_IVE_NON_MAX_SUP_PARAM));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_NON_MAX_SUP_PARAM));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_NON_MAX_SUP, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_NON_MAX_SUP fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_THRES_LUT:
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_THRES_LUT_PARAM) + 4);
			memcpy((void *)tmp_buf, (void *)p_param, sizeof(VENDOR_IVE_THRES_LUT_PARAM));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_THRES_LUT_PARAM));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_THRES_LUT, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_THRES_LUT fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_MORPH_FILTER:
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_MORPH_FILTER_PARAM) + 4);
			memcpy((void *)tmp_buf, (void *)p_param, sizeof(VENDOR_IVE_MORPH_FILTER_PARAM));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_MORPH_FILTER_PARAM));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_MORPH_FILTER, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_MORPH_FILTER fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_INTEGRAL_IMG:
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_INTEGRAL_IMG_PARAM) + 4);
			memcpy((void *)tmp_buf, (void *)p_param, sizeof(VENDOR_IVE_INTEGRAL_IMG_PARAM));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_INTEGRAL_IMG_PARAM));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_INTEGRAL_IMG, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set VENDOR_IVE_INTEGRAL_IMG fail! \n");
				return ret;
			}
			break;
        case VENDOR_IVE_OUTSEL:
            tmp_outsel = (VENDOR_IVE_OUTSEL_PARAM*)p_param;
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_OUTSEL_PARAM) + 4);
			memcpy((void *)tmp_buf, (void *)tmp_outsel, sizeof(VENDOR_IVE_OUTSEL_PARAM));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_OUTSEL_PARAM));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_OUTSEL, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set IVE_IOC_SET_OUTSEL fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_IRV:
            tmp_irv = (VENDOR_IVE_IRV_PARAM*)p_param;
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_IRV_PARAM) + 4);
			memcpy((void *)tmp_buf, (void *)tmp_irv, sizeof(VENDOR_IVE_IRV_PARAM));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_IRV_PARAM));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_IRV, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set IVE_IOC_SET_IRV fail! \n");
				return ret;
			}
			break;
	
		case VENDOR_IVE_FLOWCT:
            tmp_flowct = (VENDOR_IVE_FLOWCT_PARAM*)p_param;
			tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_FLOWCT_PARAM) + 4);
			memcpy((void *)tmp_buf, (void *)tmp_flowct, sizeof(VENDOR_IVE_FLOWCT_PARAM));
			p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_FLOWCT_PARAM));
			p_id[0] = id;
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_SET_FLOWCT, (void *)tmp_buf);
			free(tmp_buf);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive set IVE_IOC_SET_FLOWCT fail! \n");
				return ret;
			}
			break;

		default:
			printf("[VENDOR_IVE] not support param id %d \n", param_id);
			break;
	}

	return 0;
}

INT32 vendor_ive_get_param(VENDOR_IVE_FUNC param_id, void *p_param, UINT32 id)
{
	int ret = 0;
	unsigned int* p_id = NULL;
	if (ive_api_init == FALSE) {
		printf("[VENDOR_IVE] ive should be init before get param\n");
		return -1;
	}
	if (p_param == NULL) {
		printf("[VENDOR_IVE] p_param is NULL!\n");
		return -1;
	}
	p_id = (unsigned int*)p_param;
	p_id[0] = id;

	switch (param_id) {
		case VENDOR_IVE_INPUT_INFO:
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_IMG_INFO, (VENDOR_IVE_IN_IMG_INFO*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_INPUT_INFO fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_INPUT_ADDR:
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_IMG_DMA_IN, (VENDOR_IVE_IMG_IN_DMA_INFO*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_INPUT_ADDR fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_OUTPUT_ADDR:
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_IMG_DMA_OUT, (VENDOR_IVE_IMG_OUT_DMA_INFO*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_OUTPUT_ADDR fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_GENERAL_FILTER:
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_GENERAL_FILTER, (VENDOR_IVE_GENERAL_FILTER_PARAM*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_GENERAL_FILTER fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_MEDIAN_FILTER:
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_MEDIAN_FILTER, (VENDOR_IVE_MEDIAN_FILTER_PARAM*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_MEDIAN_FILTER fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_EDGE_FILTER:
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_EDGE_FILTER, (VENDOR_IVE_EDGE_FILTER_PARAM*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_EDGE_FILTER fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_NON_MAX_SUP:
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_NON_MAX_SUP, (VENDOR_IVE_NON_MAX_SUP_PARAM*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_NON_MAX_SUP fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_THRES_LUT:
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_THRES_LUT, (VENDOR_IVE_THRES_LUT_PARAM*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_THRES_LUT fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_MORPH_FILTER:
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_MORPH_FILTER, (VENDOR_IVE_MORPH_FILTER_PARAM*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_MORPH_FILTER fail! \n");
				return ret;
			}
			break;
		case VENDOR_IVE_INTEGRAL_IMG:
			ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_INTEGRAL_IMG, (VENDOR_IVE_INTEGRAL_IMG_PARAM*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_INTEGRAL_IMG fail! \n");
				return ret;
			}
			break;
        case VENDOR_IVE_OUTSEL:
            ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_OUTSEL, (VENDOR_IVE_OUTSEL_PARAM*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_OUTSEL fail! \n");
				return ret;
			}
            break;
		case VENDOR_IVE_IRV:
            ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_IRV, (VENDOR_IVE_IRV_PARAM*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_IRV fail! \n");
				return ret;
			}
            break;

		case VENDOR_IVE_FLOWCT:
            ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_FLOWCT, (VENDOR_IVE_FLOWCT_PARAM*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_FLOWCT fail! \n");
				return ret;
			}
            break;

		case VENDOR_IVE_VERSION:
            ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_GET_VERSION, (CHAR*)p_param);
			if (ret < 0) {
				printf("[VENDOR_IVE] ive get VENDOR_IVE_VERSION fail! \n");
				return ret;
			}
            break;

		default:
			printf("[VENDOR_IVE] not support get param id %d \n", param_id);
			break;
	}

	return 0;
}

INT32 vendor_ive_trigger(VENDOR_IVE_TRIGGER_PARAM *p_param, UINT32 id)
{
	int ret = 0;
	if (ive_api_init == FALSE) {
		printf("[VENDOR_IVE] ive should be init before trigger\n");
		return -1;
	}
	if (p_param == NULL) {
		printf("[VENDOR_IVE] p_param is NULL!\n");
		return -1;
	}
	if (p_param->is_nonblock == 1) {
		printf("[VENDOR_IVE] is_nonblock can't be true in vendor_ive_trigger!\n");
        return -1;
	}

	unsigned int* tmp_buf = (unsigned int*)malloc(sizeof(VENDOR_IVE_TRIGGER_PARAM) + 4);
	memcpy((void *)tmp_buf, (void *)p_param, sizeof(VENDOR_IVE_TRIGGER_PARAM));
	unsigned int* p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(VENDOR_IVE_TRIGGER_PARAM));
	p_id[0] = id;
	ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_TRIGGER, (void *)tmp_buf);
	free(tmp_buf);
	if (ret < 0) {
		printf("[VENDOR_IVE] ive trigger fail! \n");
		return ret;
	}

	return ret;
}

VOID vendor_ive_get_version(void *p_param)
{
	CHAR version_info[32] = VENDOR_IVE_IMPL_VERSION;

	if (p_param == NULL) {
		printf("[VENDOR_IVE] get_version, p_param is NULL fail! \n");
		return;
	}

	memcpy((void *)p_param, (void *)version_info, sizeof(VENDOR_IVE_IMPL_VERSION));

	return;
}

INT32 vendor_ive_trigger_nonblock(UINT32 id)
{
	int ret = 0;
	if (ive_api_init == FALSE) {
		printf("[VENDOR_IVE] ive should be init before trigger\n");
		return -1;
	}
	unsigned int* tmp_buf = (unsigned int*)malloc(4);
	//memcpy((void *)tmp_buf, (void *)p_param, 0);
	unsigned int* p_id = (unsigned int*)((unsigned int)tmp_buf);
	p_id[0] = id;
	ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_TRIGGER_NONBLOCK, (void *)tmp_buf);
	free(tmp_buf);
	if (ret < 0) {
		//printf("[VENDOR_IVE] ive trigger fail! \n");
		return ret;
	}

	return ret;
}

INT32 vendor_ive_waitdone_nonblock(UINT32 *timeout, UINT32 id)
{
	int ret = 0;
	if (ive_api_init == FALSE) {
		printf("[VENDOR_IVE] ive should be init before trigger\n");
		return -1;
	}
	if (timeout == NULL) {
		printf("[VENDOR_IVE] p_param is NULL!\n");
		return -1;
	}
	unsigned int* tmp_buf = (unsigned int*)malloc(sizeof(UINT32) + 4);
	memcpy((void *)tmp_buf, (void *)timeout, sizeof(UINT32));
	unsigned int* p_id = (unsigned int*)((unsigned int)tmp_buf + sizeof(UINT32));
	p_id[0] = id;
	ret = NVTIVE_IOCTL(ive_fd, IVE_IOC_WAITDONE_NONBLOCK, (void *)tmp_buf);
	free(tmp_buf);
	if (ret < 0) {
		//printf("[VENDOR_IVE] ive waitdone fail! \n");
		return ret;
	}

	return ret;
}


