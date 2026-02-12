#include "dis_ioctl.h"
#include "kdrv_dis.h"
#include "kdrv_eth.h"
#include "dis_platform.h"
#include "kdrv_dis_version.h"



#if defined(__FREERTOS)
int nvt_dis_ioctl(int fd, unsigned int uiCmd, void *p_arg)
{
	int iRet = 0;
	KDRV_DIS_OPENCFG                  dis_cfg_data            = {0};
	KDRV_DIS_IN_IMG_INFO              dis_imginfo_data        = {0};
	KDRV_DIS_IN_DMA_INFO              dis_input_addr          = {0};
	KDRV_DIS_OUT_DMA_INFO             dis_output_addr         = {0};
	KDRV_MV_OUT_DMA_INFO              dis_out_mv              = {0};
	UINT32                            dis_int_en              = 0;
	KDRV_DIS_TRIGGER_PARAM            dis_trigger_param       = {0};
	KDRV_MDS_DIM                      dis_mdsdim              = {0};
	KDRV_BLOCKS_DIM                   dis_blksz               = KDRV_DIS_BLKSZ_MAX;

	KDRV_ETH_IN_PARAM                 dis_eth_param           = {0};
	KDRV_ETH_IN_BUFFER_INFO           dis_eth_dma             = {0};
	KDRV_ETH_OUT_PARAM                dis_eth_param_out       = {0};
    CHAR                              version_info[32]        = KDRV_DIS_VERSION;


	nvt_dbg(IND, "cmd:%x\n", uiCmd);

	switch (uiCmd) {
	/* Add other operations here */

	// OPEN
	case DIS_IOC_OPEN:
		kdrv_dis_open(0, 0);
		break;
	// CLOSE
	case DIS_IOC_CLOSE:
		kdrv_dis_close(0, 0);
		break;
	// OPENCFG
	case DIS_IOC_OPENCFG:
		memcpy(&dis_cfg_data, p_arg, sizeof(KDRV_DIS_OPENCFG));
		kdrv_dis_set(0, KDRV_DIS_PARAM_OPENCFG, &dis_cfg_data);
		break;
	// INPUT IMG INFO
	case DIS_IOC_SET_IMG_INFO:
		memcpy(&dis_imginfo_data, p_arg, sizeof(KDRV_DIS_IN_IMG_INFO));
		kdrv_dis_set(0, KDRV_DIS_PARAM_IN_IMG, &dis_imginfo_data);
		break;
	case DIS_IOC_GET_IMG_INFO:
		kdrv_dis_get(0, KDRV_DIS_PARAM_IN_IMG, &dis_imginfo_data);
		memcpy(p_arg, &dis_imginfo_data, sizeof(KDRV_DIS_IN_IMG_INFO));
		break;
	// DMA IN
	case DIS_IOC_SET_IMG_DMA_IN:
		memcpy(&dis_input_addr, p_arg, sizeof(KDRV_DIS_IN_DMA_INFO));
		kdrv_dis_set(0, KDRV_DIS_PARAM_DMA_IN, &dis_input_addr);
		break;
	case DIS_IOC_GET_IMG_DMA_IN:
		kdrv_dis_get(0, KDRV_DIS_PARAM_DMA_IN, &dis_input_addr);
		memcpy(p_arg, &dis_input_addr, sizeof(KDRV_DIS_IN_DMA_INFO));
		break;
	// DMA_OUT
	case DIS_IOC_SET_IMG_DMA_OUT:
		memcpy(&dis_output_addr, p_arg, sizeof(KDRV_DIS_OUT_DMA_INFO));
		kdrv_dis_set(0, KDRV_DIS_PARAM_DMA_OUT, &dis_output_addr);
		break;
	case DIS_IOC_GET_IMG_DMA_OUT:
		kdrv_dis_get(0, KDRV_DIS_PARAM_DMA_OUT, &dis_output_addr);
		memcpy(p_arg, &dis_output_addr, sizeof(KDRV_DIS_OUT_DMA_INFO));
		break;
	// INT_EN_IN
	case DIS_IOC_SET_INT_EN:
		memcpy(&dis_int_en, p_arg, sizeof(UINT32));
		kdrv_dis_set(0, KDRV_DIS_PARAM_INT_EN, &dis_int_en);
		break;
	case DIS_IOC_GET_INT_EN:
		kdrv_dis_get(0, KDRV_DIS_PARAM_INT_EN, &dis_int_en);
		memcpy(p_arg, &dis_int_en, sizeof(UINT32));
		break;
	// MV_OUT
	case DIS_IOC_GET_MV_MAP_OUT:
		memcpy(&dis_out_mv, p_arg, sizeof(KDRV_MV_OUT_DMA_INFO));
		kdrv_dis_get(0, KDRV_DIS_PARAM_MV_OUT, &dis_out_mv);
		break;
	//MDS_DIM
	case DIS_IOC_GET_MV_MDS_DIM:
		kdrv_dis_get(0, KDRV_DIS_PARAM_MV_DIM, &dis_mdsdim);
		memcpy(p_arg, &dis_mdsdim, sizeof(KDRV_MDS_DIM));
		break;
	//BLOCK_DIM
	case DIS_IOC_SET_MV_BLOCKS_DIM:
		memcpy(&dis_blksz, p_arg, sizeof(KDRV_BLOCKS_DIM));
		kdrv_dis_set(0, KDRV_DIS_PARAM_BLOCK_DIM, &dis_blksz);
		break;
	case DIS_IOC_GET_MV_BLOCKS_DIM:
		kdrv_dis_get(0, KDRV_DIS_PARAM_BLOCK_DIM, &dis_blksz);
		memcpy(p_arg, &dis_blksz, sizeof(KDRV_BLOCKS_DIM));
		break;
	//trigger engine
	case DIS_IOC_TRIGGER:
		memcpy(&dis_trigger_param, p_arg, sizeof(KDRV_DIS_TRIGGER_PARAM));
		kdrv_dis_trigger(0, &dis_trigger_param, NULL, NULL);
		break;

	//ETH IN_PARAM
	case ETH_IOC_SET_ETH_PARAM_IN:
		memcpy(&dis_eth_param, p_arg, sizeof(KDRV_ETH_IN_PARAM));
		kdrv_dis_set(0, KDRV_DIS_ETH_PARAM_IN, &dis_eth_param);
		break;
	//ETH IN_PARAM
	case ETH_IOC_GET_ETH_PARAM_IN:
		kdrv_dis_get(0, KDRV_DIS_ETH_PARAM_IN, &dis_eth_param);
		memcpy(p_arg, &dis_eth_param, sizeof(KDRV_ETH_IN_PARAM));
		break;
	//ETH IN_BUFFER
	case ETH_IOC_SET_ETH_BUFFER:
		memcpy(&dis_eth_dma, p_arg, sizeof(KDRV_ETH_IN_BUFFER_INFO));
		kdrv_dis_set(0, KDRV_DIS_ETH_BUFFER_IN, &dis_eth_dma);
		break;
	//ETH IN_BUFFER
	case ETH_IOC_GET_ETH_BUFFER:
		kdrv_dis_get(0, KDRV_DIS_ETH_BUFFER_IN, &dis_eth_dma);
		memcpy(p_arg, &dis_eth_dma, sizeof(KDRV_ETH_IN_BUFFER_INFO));
		break;
	//ETH OUT_PARAM
	case ETH_IOC_GET_ETH_PARAM_OUT:
		kdrv_dis_get(0, KDRV_DIS_ETH_PARAM_OUT, &dis_eth_param_out);
		memcpy(p_arg, &dis_eth_param_out, sizeof(KDRV_ETH_OUT_PARAM));
		break;

    //GET DRIVER VERSION
    case DIS_IOC_GET_VER:
        memcpy(p_arg, &version_info, sizeof(KDRV_DIS_VERSION));
        break;
	default :
		break;
	}
	return iRet;
}

#endif

