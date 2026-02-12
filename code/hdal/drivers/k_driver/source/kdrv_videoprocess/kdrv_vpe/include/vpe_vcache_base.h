#ifndef __VPE_VCACHE_BASE_H__
#define __VPE_VCACHE_BASE_H__


#define VPE_REG_VCACHE_START             0xb0
#define VPE_REG_VCACHE_END               0xbc


enum {
	VPE_REG_VCACHE_IDX_SHT = 0,
	VPE_REG_VCACHE_IDX_SET,
	VPE_REG_VCACHE_IDX_OFT,
	VPE_REG_VCACHE_OFT_MODE,
	VPE_REG_VCACHE_OFT_SHT,
	VPE_REG_VCACHE_SLOT_WIDTH,
	VPE_REG_VCACHE_SLOT_HEIGHT,
	VPE_REG_VCACHE_OFT_WIDTH,
	VPE_REG_VCACHE_OFT_HEIGHT,

	VPE_REG_VCACHE_MAXNUM
};

extern int vpe_drv_vcache_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg);
//extern int vpe_drv_vcache_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd);

extern void vpe_set_vcache_reg(VPE_VCACHE_CFG *p_vcache);

#endif
