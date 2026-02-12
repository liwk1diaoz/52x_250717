 #include "vendor_dis.h"
 #include "../include/vendor_dis/vendor_dis_version.h"
 #include "dis_ioctl.h"
 #include "hdal.h"
 #include "kflow_common/nvtmpp.h"
 #include "kflow_common/nvtmpp_ioctl.h"
#if defined(__LINUX)
#include <sys/ioctl.h>
#endif

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)
#define NVTMPP_OPEN(...) 0
#define NVTMPP_IOCTL nvtmpp_ioctl
#define NVTMPP_CLOSE(...)

#define NVTDIS_OPEN(...) 0
#define NVTDIS_IOCTL nvt_dis_ioctl
#define NVTDIS_CLOSE(...)
#endif

#if defined(__LINUX)
#define NVTMPP_OPEN  open
#define NVTMPP_IOCTL ioctl
#define NVTMPP_CLOSE close

#define NVTDIS_OPEN  open
#define NVTDIS_IOCTL ioctl
#define NVTDIS_CLOSE close
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
static BOOL g_vendor_dis_api_init = FALSE;
static INT32 g_vendor_dis_fd = -1;
static INT32 g_vendor_dis_nvtmpp_fd = -1;
#if VENDOR_DIS_IOREMAP_IN_KERNEL
#else
static UINT32 g_vendor_dis_eth_buff_pa = 0;
static UINT32 g_vendor_dis_eth_buff_va = 0;
#endif
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT32 vendor_dis_init(UINT32 func)
{
	INT32 ret;
	VENDOR_DIS_OPENCFG dis_opencfg;
	if (g_vendor_dis_api_init == FALSE) {
		g_vendor_dis_api_init = TRUE;

		// open dis device
		g_vendor_dis_fd= NVTDIS_OPEN("/dev/kdrv_dis0", O_RDWR);
		if (g_vendor_dis_fd < 0) {
			printf("[VENDOR_DIS] Open kdrv_dis fail!\n");
			return HD_ERR_NG;
		}

		// open nvtmpp device
		g_vendor_dis_nvtmpp_fd = NVTMPP_OPEN("/dev/nvtmpp", O_RDWR|O_SYNC);
		if (g_vendor_dis_nvtmpp_fd < 0) {
			printf("open /dev/nvtmpp error\r\n");
			return HD_ERR_SYS;
		}

		if(func){

			ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_OPENCFG, &dis_opencfg);
			if (ret < 0) {
				printf("[VENDOR_DIS] dis opencfg fail! \n");
				return ret;
			}

			ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_OPEN, NULL);
			if (ret < 0) {
				printf("[VENDOR_DIS] dis open fail! \n");
				return ret;
			}
		}

		return 0;
	} else {
		printf("[VENDOR_DIS] dis module is already initialised. \n");
		return -1;
	}
}

INT32 vendor_dis_uninit(UINT32 func)
{
	INT32 ret;
	if (g_vendor_dis_api_init == TRUE) {
		g_vendor_dis_api_init = FALSE;

		if(func){
			ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_CLOSE, NULL);
			if (ret < 0) {
				printf("[VENDOR_DIS] dis close fail! \n");
				return ret;
			}
		}
		if (g_vendor_dis_fd != -1) {
			NVTDIS_CLOSE(g_vendor_dis_fd);
		}
		if (g_vendor_dis_nvtmpp_fd != -1) {
            NVTMPP_CLOSE(g_vendor_dis_nvtmpp_fd);
        }
		return 0;
	} else {
		printf("[VENDOR_DIS] dis module is not initialised yet. \n");
		return -1;
	}
}

INT32 vendor_dis_set_param(VENDOR_DIS_FUNC param_id, void *p_param)
{
	int ret = 0;
	VENDOR_DIS_IMG_IN_DMA_INFO* p_tmp_addr_param;
	VENDOR_DIS_IMG_OUT_DMA_INFO* p_tmp_addr_param2;
	VENDOR_DIS_ETH_IN_BUFFER_INFO* p_tmp_addr_param3;
#if VENDOR_DIS_IOREMAP_IN_KERNEL
#else
	NVTMPP_IOC_VB_PA_TO_VA_S nvtmpp_msg = {0};
#endif
	if (g_vendor_dis_api_init == FALSE) {
		printf("[VENDOR_DIS] dis should be init before set param\n");
		return -1;
	}

	switch (param_id) {
	case VENDOR_DIS_INPUT_INFO:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_SET_IMG_INFO, (VENDOR_DIS_IN_IMG_INFO*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_INPUT_INFO fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_INPUT_ADDR:
		p_tmp_addr_param = (VENDOR_DIS_IMG_IN_DMA_INFO*)p_param;
#if VENDOR_DIS_IOREMAP_IN_KERNEL
#else
		nvtmpp_msg.pa = p_tmp_addr_param->ui_inadd0;
		ret = NVTMPP_IOCTL(g_vendor_dis_nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_INPUT_ADDR fail in get kernel va! \n");
			return ret;
		}
		p_tmp_addr_param->ui_inadd0= nvtmpp_msg.va;

		nvtmpp_msg.pa = p_tmp_addr_param->ui_inadd1;
		ret = NVTMPP_IOCTL(g_vendor_dis_nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_INPUT_ADDR fail in get kernel va! \n");
			return ret;
		}
		p_tmp_addr_param->ui_inadd1= nvtmpp_msg.va;

		nvtmpp_msg.pa = p_tmp_addr_param->ui_inadd2;
		ret = NVTMPP_IOCTL(g_vendor_dis_nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_INPUT_ADDR fail in get kernel va! \n");
			return ret;
		}
		p_tmp_addr_param->ui_inadd2= nvtmpp_msg.va;
#endif
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_SET_IMG_DMA_IN, (VENDOR_DIS_IMG_IN_DMA_INFO*)p_tmp_addr_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_INPUT_ADDR fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_INT_EN:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_SET_INT_EN, (UINT32*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_INT_EN fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_OUTPUT_ADDR:
		p_tmp_addr_param2 = (VENDOR_DIS_IMG_OUT_DMA_INFO*)p_param;
#if VENDOR_DIS_IOREMAP_IN_KERNEL
#else
		nvtmpp_msg.pa = p_tmp_addr_param2->ui_outadd0;
		ret = NVTMPP_IOCTL(g_vendor_dis_nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_OUTPUT_ADDR fail in get kernel va! \n");
			return ret;
		}
		p_tmp_addr_param2->ui_outadd0= nvtmpp_msg.va;

		nvtmpp_msg.pa = p_tmp_addr_param2->ui_outadd1;
		ret = NVTMPP_IOCTL(g_vendor_dis_nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_OUTPUT_ADDR fail in get kernel va! \n");
			return ret;
		}
		p_tmp_addr_param2->ui_outadd1= nvtmpp_msg.va;
#endif
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_SET_IMG_DMA_OUT, (VENDOR_DIS_IMG_OUT_DMA_INFO*)p_tmp_addr_param2);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_OUTPUT_ADDR fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_BLOCKS_DIM_INFO:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_SET_MV_BLOCKS_DIM, (VENDOR_DIS_BLOCKS_DIM*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_BLOCKS_DIM fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_ETH_PARAM_IN:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, ETH_IOC_SET_ETH_PARAM_IN, (VENDOR_DIS_ETH_IN_PARAM*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set ETH_IOC_SET_ETH_PARAM_IN fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_ETH_BUFFER_IN:
		p_tmp_addr_param3 = (VENDOR_DIS_ETH_IN_BUFFER_INFO*)p_param;
#if VENDOR_DIS_IOREMAP_IN_KERNEL
#else
		nvtmpp_msg.pa = p_tmp_addr_param3->ui_inadd;
		ret = NVTMPP_IOCTL(g_vendor_dis_nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set VENDOR_DIS_INPUT_ADDR fail in get kernel va! \n");
			return ret;
		}
		p_tmp_addr_param3->ui_inadd= nvtmpp_msg.va;

		g_vendor_dis_eth_buff_pa = nvtmpp_msg.pa;
		g_vendor_dis_eth_buff_va = nvtmpp_msg.va;
#endif
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, ETH_IOC_SET_ETH_BUFFER, (VENDOR_DIS_ETH_IN_BUFFER_INFO*)p_tmp_addr_param3);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis set ETH_IOC_SET_ETH_BUFFER fail! \n");
			return ret;
		}
		break;
	default:
		printf("[VENDOR_DIS] not support id %d \n", param_id);
		break;
	}

	return 0;
}

INT32 vendor_dis_get_param(VENDOR_DIS_FUNC param_id, void *p_param)
{
	int ret = 0;
#if VENDOR_DIS_IOREMAP_IN_KERNEL
#else
	NVTMPP_IOC_VB_PA_TO_VA_S nvtmpp_msg = {0};
	VENDOR_DIS_MV_IMG_OUT_DMA_INFO* p_tmp_addr_param;
	VENDOR_DIS_ETH_IN_BUFFER_INFO* p_tmp_addr_param2;
#endif
	if (g_vendor_dis_api_init == FALSE) {
		printf("[VENDOR_DIS] dis should be init before get param\n");
		return -1;
	}

	switch (param_id) {
	case VENDOR_DIS_INPUT_INFO:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_GET_IMG_INFO, (VENDOR_DIS_IN_IMG_INFO*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get VENDOR_DIS_INPUT_INFO fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_INPUT_ADDR:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_GET_IMG_DMA_IN, (VENDOR_DIS_IMG_IN_DMA_INFO*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get VENDOR_DIS_INPUT_ADDR fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_INT_EN:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_GET_INT_EN, (UINT32*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get VENDOR_DIS_INT_EN fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_OUTPUT_ADDR:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_GET_IMG_DMA_OUT, (VENDOR_DIS_IMG_OUT_DMA_INFO*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get VENDOR_DIS_OUTPUT_ADDR fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_MV_OUT:
#if VENDOR_DIS_IOREMAP_IN_KERNEL
#else
		p_tmp_addr_param = (VENDOR_DIS_MV_IMG_OUT_DMA_INFO*)p_param;
		nvtmpp_msg.pa = (UINT32)p_tmp_addr_param->p_mvaddr;
		ret = NVTMPP_IOCTL(g_vendor_dis_nvtmpp_fd, NVTMPP_IOC_VB_PA_TO_VA , &nvtmpp_msg);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get VENDOR_DIS_MV_ADDR fail in get kernel va! \n");
			return ret;
		}
		p_tmp_addr_param->p_mvaddr = (VENDOR_DIS_MOTION_INFOR*)nvtmpp_msg.va;
#endif
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_GET_MV_MAP_OUT, (VENDOR_DIS_MV_IMG_OUT_DMA_INFO*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get VENDOR_DIS_MV_ADDR fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_MDS_DIM_INFO:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_GET_MV_MDS_DIM, (VENDOR_DIS_MDS_DIM*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get VENDOR_DIS_MDS_DIM_INFO fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_BLOCKS_DIM_INFO:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_GET_MV_BLOCKS_DIM, (VENDOR_DIS_BLOCKS_DIM*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get VENDOR_DIS_BLOCKS_DIM_INFO fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_ETH_PARAM_IN:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, ETH_IOC_GET_ETH_PARAM_IN, (VENDOR_DIS_ETH_IN_PARAM*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get ETH_IOC_GET_ETH_PARAM_IN fail! \n");
			return ret;
		}
		break;
	case VENDOR_DIS_ETH_BUFFER_IN:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, ETH_IOC_GET_ETH_BUFFER, (VENDOR_DIS_ETH_IN_BUFFER_INFO*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get VENDOR_DIS_ETH_BUFFER_IN fail! \n");
			return ret;
		}
#if VENDOR_DIS_IOREMAP_IN_KERNEL
#else
		p_tmp_addr_param2 = (VENDOR_DIS_ETH_IN_BUFFER_INFO*)p_param;
		p_tmp_addr_param2->ui_inadd  = p_tmp_addr_param2->ui_inadd - g_vendor_dis_eth_buff_va + g_vendor_dis_eth_buff_pa;
#endif
		break;
	case VENDOR_DIS_ETH_PARAM_OUT:
		ret = NVTDIS_IOCTL(g_vendor_dis_fd, ETH_IOC_GET_ETH_PARAM_OUT, (VENDOR_DIS_ETH_OUT_PARAM*)p_param);
		if (ret < 0) {
			printf("[VENDOR_DIS] dis get VENDOR_DIS_ETH_PARAM_OUT fail! \n");
			return ret;
		}
		break;
	default:
		printf("[VENDOR_DIS] not support id %d \n", param_id);
		break;
	}

	return 0;
}

INT32 vendor_dis_trigger(VENDOR_DIS_TRIGGER_PARAM *p_param)
{
	int ret = 0;
	if (g_vendor_dis_api_init == FALSE) {
		printf("[VENDOR_DIS] dis should be init before trigger\n");
		return -1;
	}

	ret = NVTDIS_IOCTL(g_vendor_dis_fd, DIS_IOC_TRIGGER, p_param);
	if (ret < 0) {
		printf("[VENDOR_DIS] dis trigger fail! \n");
		return ret;
	}

	return ret;
}

INT32 vendor_dis_get_kdrv_version(CHAR* kdrv_version)
{
	INT32 ret = 0;

    // open dis device
	g_vendor_dis_fd= NVTDIS_OPEN("/dev/kdrv_dis0", O_RDWR);
	if (g_vendor_dis_fd < 0) {
		printf("[VENDOR_DIS] Open kdrv_dis fail!\n");
		return HD_ERR_NG;
	}

	ret = NVTDIS_IOCTL(g_vendor_dis_fd,  DIS_IOC_GET_VER, kdrv_version);

	if (ret < 0) {
		printf("[VENDOR_DIS] dis get kdrv_version fail! \n");
	}

	NVTDIS_CLOSE(g_vendor_dis_fd);

	return ret;
}

INT32 vendor_dis_get_vendor_version(CHAR* vendor_version)
{
    INT32 ret = 0;
    snprintf(vendor_version, 20, VENDOR_DIS_VERSION);
    return ret;
}


