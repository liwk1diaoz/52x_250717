#ifndef __VPE_PAL_BASE_H__
#define __VPE_PAL_BASE_H__

//#include "vpe_platform.h"


#define VPE_REG_PAL_START             0x260
#define VPE_REG_PAL_END               0x27c


enum {
	VPE_REG_PAL_0_Y = 0,
	VPE_REG_PAL_0_CB,
	VPE_REG_PAL_0_CR,
	VPE_REG_PAL_1_Y,
	VPE_REG_PAL_1_CB,
	VPE_REG_PAL_1_CR,
	VPE_REG_PAL_2_Y,
	VPE_REG_PAL_2_CB,
	VPE_REG_PAL_2_CR,
	VPE_REG_PAL_3_Y,
	VPE_REG_PAL_3_CB,
	VPE_REG_PAL_3_CR,
	VPE_REG_PAL_4_Y,
	VPE_REG_PAL_4_CB,
	VPE_REG_PAL_4_CR,
	VPE_REG_PAL_5_Y,
	VPE_REG_PAL_5_CB,
	VPE_REG_PAL_5_CR,
	VPE_REG_PAL_6_Y,
	VPE_REG_PAL_6_CB,
	VPE_REG_PAL_6_CR,
	VPE_REG_PAL_7_Y,
	VPE_REG_PAL_7_CB,
	VPE_REG_PAL_7_CR,
	VPE_REG_PAL_MAXNUM
};

extern int vpe_drv_pal_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg);
//extern int vpe_drv_pal_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd);


extern void vpe_set_pal_reg(VPE_PALETTE_CFG *p_pal_cfg);

#endif
