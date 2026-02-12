#ifndef __VPE_DMA_BASE_H__
#define __VPE_DMA_BASE_H__

//#include "vpe_platform.h"


#define VPE_REG_DMA_START             0x300
#define VPE_REG_DMA_END               0x364


enum {
	VPE_REG_DMA_LLC = 0,
	VPE_REG_DMA_SRC,
	VPE_REG_DMA_DCTG_GAMMA_ADDR,
	VPE_REG_DMA_DCE_2DLUT_ADDR,
	VPE_REG_DMA_DES_RES0_DP0,
	VPE_REG_DMA_DES_RES1_DP0,
	VPE_REG_DMA_DES_RES2_DP0,
	VPE_REG_DMA_DES_RES3_DP0,
	VPE_REG_DMA_MAXNUM
};

extern int vpe_drv_dma_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg);
extern int vpe_drv_dma_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd);
#endif
