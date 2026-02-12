#include "md_ioctl.h"
#include "kdrv_md.h"
#include "md_platform.h"
#include "kdrv_md_version.h"


#if defined(__FREERTOS)
int nvt_md_ioctl(int fd, unsigned int uiCmd, void *p_arg)
{
    /*
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
    */
    int iRet = 0;
	KDRV_MD_OPENCFG      md_cfg_data = {0};
	KDRV_MD_PARAM        md_param = {0};
    KDRV_MD_TRIGGER_PARAM md_trigger_param = {0};
	KDRV_MD_REG_DATA     md_reg = {0};
    const UINT32 chip = 0, engine = KDRV_CV_ENGINE_MD, channel = 0;
	static UINT32 id = 0;

	nvt_dbg(IND, "cmd:%x\n", uiCmd);

	switch (uiCmd) {
	/* Add other operations here */

	// OPEN
	case MD_IOC_OPEN:
		kdrv_md_open(chip, engine);
        id = KDRV_DEV_ID(chip, engine, channel);
		break;
	// CLOSE
	case MD_IOC_CLOSE:
		kdrv_md_close(chip, engine);
		break;
	// OPENCFG
	case MD_IOC_OPENCFG:
		memcpy(&md_cfg_data, p_arg, sizeof(KDRV_MD_OPENCFG));
        kdrv_md_set(id, KDRV_MD_PARAM_OPENCFG, &md_cfg_data);
		break;
    // SET MODE
	case MD_IOC_SET_PARAM:
		memcpy(&md_param, p_arg, sizeof(KDRV_MD_PARAM));
	    kdrv_md_set(id, KDRV_MD_PARAM_ALL, &md_param);
		break;
	// GET MODE
	case MD_IOC_GET_PARAM:
		kdrv_md_get(id, KDRV_MD_PARAM_ALL, &md_param);
		memcpy(p_arg, &md_param, sizeof(KDRV_MD_PARAM));
		break;
	// trigger engine
	case MD_IOC_TRIGGER:
        memcpy(&md_trigger_param, p_arg, sizeof(KDRV_MD_TRIGGER_PARAM));
		kdrv_md_trigger(id, &md_trigger_param, NULL, NULL);
		break;
	case MD_IOC_GET_REG:
        kdrv_md_get(id, KDRV_MD_PARAM_GET_REG, &md_reg);
		memcpy(p_arg, &md_reg, sizeof(KDRV_MD_REG_DATA));
		break;	
	case MD_IOC_GET_VER: {
		CHAR version_info[32] = KDRV_MD_IMPL_VERSION;
		memcpy(p_arg, version_info, sizeof(KDRV_MD_IMPL_VERSION));
	}	
	default :
		break;
	}
	return iRet;
}

#endif

