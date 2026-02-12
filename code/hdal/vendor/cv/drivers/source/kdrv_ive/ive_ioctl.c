#include "ive_ioctl.h"
#include "kdrv_ive.h"
#include "ive_platform.h"
#include "ive_dbg.h"
#include "kdrv_ive_version.h"


#if defined(__FREERTOS)
int nvt_ive_ioctl(int fd, unsigned int uiCmd, void *p_arg)
{
	int iRet = 0;
	KDRV_IVE_OPENCFG                  ive_cfg_data            = {0};
	KDRV_IVE_IN_IMG_INFO              ive_imginfo_data        = {0};
	KDRV_IVE_IMG_IN_DMA_INFO          ive_input_addr          = {0};
	KDRV_IVE_IMG_OUT_DMA_INFO         ive_output_addr         = {0};
	KDRV_IVE_GENERAL_FILTER_PARAM     ive_general_filter_data = {0};
	KDRV_IVE_MEDIAN_FILTER_PARAM      ive_median_filter_data  = {0};
	KDRV_IVE_EDGE_FILTER_PARAM        ive_edge_filter_data    = {0};
	KDRV_IVE_NON_MAX_SUP_PARAM        ive_non_max_sup_data    = {0};
	KDRV_IVE_THRES_LUT_PARAM          ive_thres_lut_data      = {0};
	KDRV_IVE_MORPH_FILTER_PARAM       ive_morph_filter_data   = {0};
	KDRV_IVE_INTEGRAL_IMG_PARAM       ive_integral_img_data   = {0};
	KDRV_IVE_IRV_PARAM                ive_irv_data            = {0};
	KDRV_IVE_FLOW_CT_PARAM            ive_flowct_data         = {0};
	CHAR                              version_info[32]        = KDRV_IVE_IMPL_VERSION;

	KDRV_IVE_OUTSEL_PARAM             ive_outsel_data         = {0};
	KDRV_IVE_TRIGGER_PARAM            ive_trigger_param       = {0};
	UINT32							  ive_waitdone_timeout    = 0;
	unsigned int id = 0;

	nvt_dbg(IND, "cmd:%x\n", uiCmd);

	switch(uiCmd) {
		case IVE_IOC_START:
			/*call someone to start operation*/
			DBG_ERR("IVE: IVE_IOC_START has not implemented. TODO\r\n");
			break;

		case IVE_IOC_STOP:
			/*call someone to stop operation*/
			DBG_ERR("IVE: IVE_IOC_STOP has not implemented. TODO\r\n");
			break;

		case IVE_IOC_READ_REG:
			DBG_ERR("IVE: IVE_IOC_READ_REG has not implemented. TODO\r\n");
			break;

		case IVE_IOC_WRITE_REG:
			DBG_ERR("IVE: IVE_IOC_WRITE_REG has not implemented. TODO\r\n");
			break;

		case IVE_IOC_READ_REG_LIST:
			DBG_ERR("IVE: IVE_IOC_READ_REG_LIST has not implemented. TODO\r\n");
			break;

		case IVE_IOC_WRITE_REG_LIST:
			DBG_ERR("IVE: IVE_IOC_WRITE_REG_LIST has not implemented. TODO\r\n");
			break;

		/* Add other operations here */
		// OPEN
		case IVE_IOC_OPEN:
			kdrv_ive_open(0, 0);
			break;
		// CLOSE
		case IVE_IOC_CLOSE:
			kdrv_ive_close(0, 0);
			break;
		// OPENCFG
		case IVE_IOC_OPENCFG:

			memcpy(&ive_cfg_data, p_arg, sizeof(KDRV_IVE_OPENCFG));
			kdrv_ive_set(0, KDRV_IVE_PARAM_IPL_OPENCFG, &ive_cfg_data);

			break;
		// INPUT IMG INFO
		case IVE_IOC_SET_IMG_INFO:
			
			memcpy(&ive_imginfo_data, p_arg, sizeof(KDRV_IVE_IN_IMG_INFO));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_IN_IMG_INFO)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_IN_IMG, &ive_imginfo_data);

			break;
		case IVE_IOC_GET_IMG_INFO:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IPL_IN_IMG, &ive_imginfo_data);
			memcpy(p_arg, &ive_imginfo_data, sizeof(KDRV_IVE_IN_IMG_INFO));

			break;
		// DMA IN
		case IVE_IOC_SET_IMG_DMA_IN:
			
			memcpy(&ive_input_addr, p_arg, sizeof(KDRV_IVE_IMG_IN_DMA_INFO));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_IMG_IN_DMA_INFO)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_IMG_DMA_IN, &ive_input_addr);

			break;
		case IVE_IOC_GET_IMG_DMA_IN:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IPL_IMG_DMA_IN, &ive_input_addr);
			memcpy(p_arg, &ive_input_addr, sizeof(KDRV_IVE_IMG_IN_DMA_INFO));

			break;
		// DMA_OUT
		case IVE_IOC_SET_IMG_DMA_OUT:
			
			memcpy(&ive_output_addr, p_arg, sizeof(KDRV_IVE_IMG_OUT_DMA_INFO));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_IMG_OUT_DMA_INFO)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IPL_IMG_DMA_OUT, &ive_output_addr);

			break;
		case IVE_IOC_GET_IMG_DMA_OUT:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IPL_IMG_DMA_OUT, &ive_output_addr);
			memcpy(p_arg, &ive_output_addr, sizeof(KDRV_IVE_IMG_OUT_DMA_INFO));

			break;
		// GENERAL FILTER
		case IVE_IOC_SET_GENERAL_FILTER:
			
			memcpy(&ive_general_filter_data, p_arg, sizeof(KDRV_IVE_GENERAL_FILTER_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_GENERAL_FILTER_PARAM)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_GENERAL_FILTER, &ive_general_filter_data);

			break;
		case IVE_IOC_GET_GENERAL_FILTER:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_GENERAL_FILTER, &ive_general_filter_data);
			memcpy(p_arg, &ive_general_filter_data, sizeof(KDRV_IVE_GENERAL_FILTER_PARAM));

			break;
		// MEDIAN FILTER
		case IVE_IOC_SET_MEDIAN_FILTER:
			
			memcpy(&ive_median_filter_data, p_arg, sizeof(KDRV_IVE_MEDIAN_FILTER_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_MEDIAN_FILTER_PARAM)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_MEDIAN_FILTER, &ive_median_filter_data);

			break;
		case IVE_IOC_GET_MEDIAN_FILTER:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_MEDIAN_FILTER, &ive_median_filter_data);
			memcpy(p_arg, &ive_median_filter_data, sizeof(KDRV_IVE_MEDIAN_FILTER_PARAM));

			break;
		// EDGE FILTER
		case IVE_IOC_SET_EDGE_FILTER:
			
			memcpy(&ive_edge_filter_data, p_arg, sizeof(KDRV_IVE_EDGE_FILTER_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_EDGE_FILTER_PARAM)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_EDGE_FILTER, &ive_edge_filter_data);

			break;
		case IVE_IOC_GET_EDGE_FILTER:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_EDGE_FILTER, &ive_edge_filter_data);
			memcpy(p_arg, &ive_edge_filter_data, sizeof(KDRV_IVE_EDGE_FILTER_PARAM));

			break;
		// NON_MAX_SUP
		case IVE_IOC_SET_NON_MAX_SUP:
			
			memcpy(&ive_non_max_sup_data, p_arg, sizeof(KDRV_IVE_NON_MAX_SUP_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_NON_MAX_SUP_PARAM)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_NON_MAX_SUP, &ive_non_max_sup_data);
	
			break;
		case IVE_IOC_GET_NON_MAX_SUP:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_NON_MAX_SUP, &ive_non_max_sup_data);
			memcpy(p_arg, &ive_non_max_sup_data, sizeof(KDRV_IVE_NON_MAX_SUP_PARAM));

			break;
		// THRES_LUT
		case IVE_IOC_SET_THRES_LUT:
			
			memcpy(&ive_thres_lut_data, p_arg, sizeof(KDRV_IVE_THRES_LUT_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_THRES_LUT_PARAM)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_THRES_LUT, &ive_thres_lut_data);

			break;
		case IVE_IOC_GET_THRES_LUT:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_THRES_LUT, &ive_thres_lut_data);
			memcpy(p_arg, &ive_thres_lut_data, sizeof(KDRV_IVE_THRES_LUT_PARAM));

			break;
		// MORPH FILTER
		case IVE_IOC_SET_MORPH_FILTER:

			memcpy(&ive_morph_filter_data, p_arg, sizeof(KDRV_IVE_MORPH_FILTER_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_MORPH_FILTER_PARAM)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_MORPH_FILTER, &ive_morph_filter_data);

			break;
		case IVE_IOC_GET_MORPH_FILTER:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_MORPH_FILTER, &ive_morph_filter_data);
			memcpy(p_arg, &ive_morph_filter_data, sizeof(KDRV_IVE_MORPH_FILTER_PARAM));

			break;
		// INTEGRAL IMG
		case IVE_IOC_SET_INTEGRAL_IMG:
			
			memcpy(&ive_integral_img_data, p_arg, sizeof(KDRV_IVE_INTEGRAL_IMG_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_INTEGRAL_IMG_PARAM)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_INTEGRAL_IMG, &ive_integral_img_data);

			break;
		case IVE_IOC_GET_INTEGRAL_IMG:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_INTEGRAL_IMG, &ive_integral_img_data);
			memcpy(p_arg, &ive_integral_img_data, sizeof(KDRV_IVE_INTEGRAL_IMG_PARAM));

			break;
		// ITER REGION VOTE
		/*
		case IVE_IOC_SET_ITER_REGION_VOTE:
			if (copy_from_user(&ive_iter_reg_vote_data, (void __user *)ulArg, sizeof(ive_iter_reg_vote_data))) {
				iRet = -EFAULT;
				goto exit;
			}
			if (copy_from_user(&id, (void __user *)(ulArg+sizeof(ive_iter_reg_vote_data)), sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_ITER_REGION_VOTE, &ive_iter_reg_vote_data);
			break;
		case IVE_IOC_GET_ITER_REGION_VOTE:
			if (copy_from_user(&id, (void __user *)ulArg, sizeof(unsigned int))) {
				iRet = -EFAULT;
				goto exit;
			}
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_ITER_REGION_VOTE, &ive_iter_reg_vote_data);
			iRet = (copy_to_user((void __user *)ulArg, &ive_iter_reg_vote_data, sizeof(ive_iter_reg_vote_data))) ? (-EFAULT) : 0;
			break;
			*/
		// trigger engine
		case IVE_IOC_TRIGGER:
			
			memcpy(&ive_trigger_param, p_arg, sizeof(KDRV_IVE_TRIGGER_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_TRIGGER_PARAM)), sizeof(unsigned int));
			kdrv_ive_trigger(id, &ive_trigger_param, NULL, NULL);

			break;
		// trigger engine
		case IVE_IOC_TRIGGER_NONBLOCK:
			
			memcpy(&id, (void *) ((UINT32)p_arg ), sizeof(unsigned int));
			iRet = kdrv_ive_trigger_nonblock(id);
			if (iRet != 0) {
				iRet = IVE_MSG_ERR;
			}

			break;
		//520-add
		case IVE_IOC_SET_OUTSEL:
		    
			memcpy(&ive_outsel_data, p_arg, sizeof(KDRV_IVE_OUTSEL_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_OUTSEL_PARAM)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IQ_OUTSEL, &ive_outsel_data);

            break;
        case IVE_IOC_GET_OUTSEL:
            
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IQ_OUTSEL, &ive_outsel_data);
			memcpy(p_arg, &ive_outsel_data, sizeof(KDRV_IVE_OUTSEL_PARAM));

            break;
		// IRV
		case IVE_IOC_SET_IRV:
			
			memcpy(&ive_irv_data, p_arg, sizeof(KDRV_IVE_IRV_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_IRV_PARAM)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_IRV, &ive_irv_data);

			break;
		case IVE_IOC_GET_IRV:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_IRV, &ive_irv_data);
			memcpy(p_arg, &ive_irv_data, sizeof(KDRV_IVE_IRV_PARAM));

			break;

		case IVE_IOC_SET_FLOWCT:
			
			memcpy(&ive_flowct_data, p_arg, sizeof(KDRV_IVE_FLOW_CT_PARAM));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(KDRV_IVE_FLOW_CT_PARAM)), sizeof(unsigned int));
			kdrv_ive_set(id, KDRV_IVE_PARAM_FLOWCT, &ive_flowct_data);

			break;

		case IVE_IOC_GET_FLOWCT:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			kdrv_ive_get(id, KDRV_IVE_PARAM_FLOWCT, &ive_flowct_data);
			memcpy(p_arg, &ive_flowct_data, sizeof(KDRV_IVE_FLOW_CT_PARAM));

			break;

		case IVE_IOC_GET_VERSION:
			
			memcpy(&id, p_arg, sizeof(unsigned int));
			memcpy(p_arg, version_info, sizeof(KDRV_IVE_IMPL_VERSION));

			break;
		
		// wait engine done
		case IVE_IOC_WAITDONE_NONBLOCK:
			
			memcpy(&ive_waitdone_timeout, p_arg, sizeof(UINT32));
			memcpy(&id, (void *) ((UINT32)p_arg + sizeof(UINT32)), sizeof(unsigned int));
			iRet = kdrv_ive_waitdone_nonblock(id, &ive_waitdone_timeout);
			if (iRet != 0) {
				if (IVE_MSG_TIMEOUT != iRet) {
					iRet = IVE_MSG_ERR;
				}
			}

			break;


		default :
			break;
	}

	return iRet;
}

void nvt_ive_drv_init_rtos(void)
{
	UINT32 ive_clk_freq;

	ive_clk_freq = ive_platform_get_clock_freq_from_dtsi(0);
	ive_create_resource(NULL, ive_clk_freq);
}

#endif

