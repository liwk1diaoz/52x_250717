
#if defined (__KERNEL__)
/*
#include <linux/version.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/io.h>
*/
#elif defined (__FREERTOS)

#else

#endif

//#include "frammap/frammap_if.h"
#include "kdrv_videoprocess/kdrv_vpe.h"

#include "vpe_int.h"
//#include "vpe_ll_base.h"
#include "vpe_config_base.h"
#include "vpe_vcache_base.h"

#if 0
VPE_REG_DEFINE vpe_vcache_reg_def[VPE_REG_VCACHE_MAXNUM] = {
	{VPE_REG_VCACHE_IDX_SHT, 0, 0xb0, 4, 4, 0, 0x4, {0, 0, 0, 0}},
	{VPE_REG_VCACHE_IDX_SET, 0, 0xb0, 1, 12, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_VCACHE_IDX_OFT, 0, 0xb0, 16, 16, 0, 0xffff, {0, 0, 0, 0}},

	{VPE_REG_VCACHE_OFT_MODE, 0, 0xb4, 4, 0, 0, 0x2, {0, 0, 0, 0}},
	{VPE_REG_VCACHE_OFT_SHT, 0, 0xb4, 2, 4, 0, 0x3, {0, 0, 0, 0}},

	{VPE_REG_VCACHE_SLOT_WIDTH, 0, 0xb8, 9, 0, 0, 0x80, {0, 0, 0, 0}},
	{VPE_REG_VCACHE_SLOT_HEIGHT, 0, 0xb8, 9, 16, 0, 0x80, {0, 0, 0, 0}},

	{VPE_REG_VCACHE_OFT_WIDTH, 0, 0xbc, 9, 0, 0, 0x10, {0, 0, 0, 0}},
	{VPE_REG_VCACHE_OFT_HEIGHT, 0, 0xbc, 9, 16, 0, 0x10, {0, 0, 0, 0}},

};

UINT32 vpe_drv_vcache_trans_config_to_regtable(VPE_VCACHE_CFG *p_vcache, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_VCACHE_IDX_SHT:
		ret = p_vcache->idx_sht;
		break;
	case VPE_REG_VCACHE_IDX_SET:
		ret = p_vcache->idx_set;
		break;
	case VPE_REG_VCACHE_IDX_OFT:
		ret = p_vcache->idx_oft;
		break;
	case VPE_REG_VCACHE_OFT_MODE:
		ret = p_vcache->oft_mode;
		break;
	case VPE_REG_VCACHE_OFT_SHT:
		ret = p_vcache->oft_sht;
		break;
	case VPE_REG_VCACHE_SLOT_WIDTH:
		ret = p_vcache->slot_width;
		break;
	case VPE_REG_VCACHE_SLOT_HEIGHT:
		ret = p_vcache->slot_height;
		break;
	case VPE_REG_VCACHE_OFT_WIDTH:
		ret = p_vcache->oft_width;
		break;
	case VPE_REG_VCACHE_OFT_HEIGHT:
		ret = p_vcache->oft_height;
		break;

	}
	return ret;
}
#endif


#if 0
int vpe_drv_vcache_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd)
{
	UINT32 i, j, mask;
	UINT32 *cmduffer;
	UINT32 tmpval = 0, regoffset = 0, regval = 0;

	cmduffer = (UINT32 *)p_llcmd->vaddr;

	for (i = VPE_REG_VCACHE_START; i <= VPE_REG_VCACHE_END; i += VPE_REG_OFFSET) {
		tmpval = 0;
		mask = 0;
		regoffset = i;
		for (j = 0; j < VPE_REG_VCACHE_MAXNUM; j++) {
			if (vpe_vcache_reg_def[j].reg_add == i) {
				if (vpe_vcache_reg_def[j].numofbits >= VPE_REG_MAXBIT) {
					mask = 0xffffffff;
				} else {
					mask = (1UL << vpe_vcache_reg_def[j].numofbits) - 1;
				}

				regval = vpe_drv_vcache_trans_config_to_regtable(&(p_drv_cfg->vcache), j);
				tmpval |= ((UINT32)(regval & mask)) << vpe_vcache_reg_def[j].startBit;
			}
		}

		if (mask > 0) {
			cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
			cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, regoffset);
		}
	}

	return 0;
}
#endif

#if KDRV_VPE_SUPPORT_DEWARP
#define PI 205887
#endif


int vpe_drv_vcache_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg)
{

	if (p_drv_cfg->glo_ctl.dce_en == 1) {
		if((p_vpe_info->dce_param.dc_scenes_sel == KDRV_VPE_DCS_EIS) || (p_vpe_info->dce_param.dc_scenes_sel == KDRV_VPE_DCS_DIS)){
			p_drv_cfg->vcache.idx_set = 0;
			p_drv_cfg->vcache.oft_mode = 0;
			p_drv_cfg->vcache.oft_sht = 2;
			p_drv_cfg->vcache.slot_width = 32;
			p_drv_cfg->vcache.slot_height = 8;
			p_drv_cfg->vcache.oft_width = 8;
			p_drv_cfg->vcache.oft_height = 4;
			p_drv_cfg->vcache.idx_sht = 0;
			p_drv_cfg->vcache.idx_oft = 0;
		}else{
			p_drv_cfg->vcache.idx_set = 0;
			p_drv_cfg->vcache.oft_mode = 1;
			p_drv_cfg->vcache.oft_sht = 1;
			p_drv_cfg->vcache.slot_width = 16;
			p_drv_cfg->vcache.slot_height = 16;
			p_drv_cfg->vcache.oft_width = 8;
			p_drv_cfg->vcache.oft_height = 4;

			p_drv_cfg->vcache.idx_sht = 0;
			p_drv_cfg->vcache.idx_oft = 0;
		}
	} else {
		p_drv_cfg->vcache.idx_set = 0;
		p_drv_cfg->vcache.oft_mode = 0;
		p_drv_cfg->vcache.oft_sht = 3;
		p_drv_cfg->vcache.slot_width = 128;
		p_drv_cfg->vcache.slot_height = 2;
		p_drv_cfg->vcache.oft_width = 16;
		p_drv_cfg->vcache.oft_height = 2;

		p_drv_cfg->vcache.idx_sht = 0;
		p_drv_cfg->vcache.idx_oft = 0;
	}


	return 0;
}



void vpe_set_vcache_reg(VPE_VCACHE_CFG *p_vcache)
{
	vpeg->reg_44.bit.vch_idx_sht = p_vcache->idx_sht;
	vpeg->reg_44.bit.vch_idx_set = p_vcache->idx_set;
	vpeg->reg_44.bit.vch_idx_oft = p_vcache->idx_oft;

	vpeg->reg_45.bit.vch_oft_mode = p_vcache->oft_mode;
	vpeg->reg_45.bit.vch_oft_sht = p_vcache->oft_sht;

	vpeg->reg_46.bit.vch_slot_width = p_vcache->slot_width;
	vpeg->reg_46.bit.vch_slot_height = p_vcache->slot_height;

	vpeg->reg_47.bit.vch_oft_width = p_vcache->oft_width;
	vpeg->reg_47.bit.vch_oft_height = p_vcache->oft_height;
}



