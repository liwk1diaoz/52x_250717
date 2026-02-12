#ifndef __VPE_DCTG_BASE_H__
#define __VPE_DCTG_BASE_H__

//#include "vpe_platform.h"


#define VPE_REG_DCTG_START             0x3e0
#define VPE_REG_DCTG_END               0x3fc


enum {
	VPE_REG_DCTG_MOUNT_TYPE = 0,
	VPE_REG_DCTG_RA_EN,
	VPE_REG_DCTG_LUT2D_SZ,
	VPE_REG_DCTG_LENS_R,
	VPE_REG_DCTG_LENS_X_ST,
	VPE_REG_DCTG_LENS_Y_ST,
	VPE_REG_DCTG_THETA_ST,
	VPE_REG_DCTG_THETA_ED,
	VPE_REG_DCTG_PHI_ST,
	VPE_REG_DCTG_PHI_ED,
	VPE_REG_DCTG_ROT_Z,
	VPE_REG_DCTG_ROT_Y,
	VPE_REG_DCTG_MAXNUM
};

extern int vpe_drv_dctg_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg);
//extern int vpe_drv_dctg_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd);

extern void vpe_set_dctg_reg(VPE_DCTG_CFG *p_dctg_cfg);

#endif
