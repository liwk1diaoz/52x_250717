
#if defined (__KERNEL__)

//#include <linux/version.h>
//#include <linux/module.h>
//#include <linux/seq_file.h>
//#include <linux/proc_fs.h>
//#include <linux/bitops.h>
//#include <linux/interrupt.h>
//#include <linux/mm.h>
//#include <asm/uaccess.h>
//#include <asm/atomic.h>
//#include <asm/io.h>

#elif defined (__FREERTOS)

#else

#endif

//#include "frammap/frammap_if.h"
#include "kdrv_videoprocess/kdrv_vpe.h"

#include "vpe_int.h"
#include "vpe_ll_base.h"
#include "vpe_config_base.h"
#include "vpe_dctg_base.h"

#if 0

VPE_REG_DEFINE vpe_dctg_reg_def[VPE_REG_DCTG_MAXNUM] = {
	{VPE_REG_DCTG_MOUNT_TYPE, 0, 0x3e0, 1, 0, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_DCTG_RA_EN, 0, 0x3e0, 1, 1, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_DCTG_LUT2D_SZ, 0, 0x3e0, 2, 2, 0, 0x3, {0, 0, 0, 0}},
	{VPE_REG_DCTG_LENS_R, 0, 0x3e0, 14, 16, 0, 0x3fff, {0, 0, 0, 0}},

	{VPE_REG_DCTG_LENS_X_ST, 0, 0x3e4, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_DCTG_LENS_Y_ST, 0, 0x3e4, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},

	{VPE_REG_DCTG_THETA_ST, 0, 0x3e8, 20, 0, 0, 0xfffff, {0, 0, 0, 0}},

	{VPE_REG_DCTG_THETA_ED, 0, 0x3ec, 20, 0, 0, 0xfffff, {0, 0, 0, 0}},

	{VPE_REG_DCTG_PHI_ST, 0, 0x3f0, 20, 0, 0, 0xfffff, {0, 0, 0, 0}},

	{VPE_REG_DCTG_PHI_ED, 0, 0x3f4, 20, 0, 0, 0xfffff, {0, 0, 0, 0}},

	{VPE_REG_DCTG_ROT_Z, 0, 0x3f8, 20, 0, 0, 0xfffff, {0, 0, 0, 0}},

	{VPE_REG_DCTG_ROT_Y, 0, 0x3fc, 20, 0, 0, 0xfffff, {0, 0, 0, 0}},
};

inline UINT32 vpe_drv_dctg_trans_config_to_regtable(VPE_DCTG_CFG *p_dctg, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_DCTG_MOUNT_TYPE:
		ret = p_dctg->mount_type;
		break;
	case VPE_REG_DCTG_RA_EN:
		ret = p_dctg->ra_en;
		break;
	case VPE_REG_DCTG_LUT2D_SZ:
		ret = p_dctg->lut2d_sz;
		break;
	case VPE_REG_DCTG_LENS_R:
		ret = p_dctg->lens_r;
		break;
	case VPE_REG_DCTG_LENS_X_ST:
		ret = p_dctg->lens_x_st;
		break;
	case VPE_REG_DCTG_LENS_Y_ST:
		ret = p_dctg->lens_y_st;
		break;
	case VPE_REG_DCTG_THETA_ST:
		ret = p_dctg->theta_st;
		break;
	case VPE_REG_DCTG_THETA_ED:
		ret = p_dctg->theta_ed;
		break;
	case VPE_REG_DCTG_PHI_ST:
		ret = p_dctg->phi_st;
		break;
	case VPE_REG_DCTG_PHI_ED:
		ret = p_dctg->phi_ed;
		break;
	case VPE_REG_DCTG_ROT_Z:
		ret = p_dctg->rot_z;
		break;
	case VPE_REG_DCTG_ROT_Y:
		ret = p_dctg->rot_y;
		break;

	}
	return ret;
}




int vpe_drv_dctg_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd)
{
	UINT32 i, j, mask;
	UINT32 *cmduffer;
	UINT32 tmpval = 0, regoffset = 0, regval = 0;

	cmduffer = (UINT32 *)p_llcmd->vaddr;

	for (i = VPE_REG_DCTG_START; i <= VPE_REG_DCTG_END; i += VPE_REG_OFFSET) {
		tmpval = 0;
		mask = 0;
		regoffset = i;
		for (j = 0; j < VPE_REG_DCTG_MAXNUM; j++) {
			if (vpe_dctg_reg_def[j].reg_add == i) {
				if (vpe_dctg_reg_def[j].numofbits >= VPE_REG_MAXBIT) {
					mask = 0xffffffff;
				} else {
					mask = (1UL << vpe_dctg_reg_def[j].numofbits) - 1;
				}

				regval = vpe_drv_dctg_trans_config_to_regtable(&(p_drv_cfg->dctg), j);
				tmpval |= ((UINT32)(regval & mask)) << vpe_dctg_reg_def[j].startBit;
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

int vpe_drv_dctg_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg)
{
	if ((p_vpe_info != NULL) && (p_drv_cfg != NULL)) {
		p_drv_cfg->dctg.mount_type = p_vpe_info->dctg_param.mount_type;
		p_drv_cfg->dctg.ra_en = p_vpe_info->dctg_param.ra_en;
		p_drv_cfg->dctg.lut2d_sz = p_vpe_info->dctg_param.lut2d_sz;
		p_drv_cfg->dctg.lens_r = p_vpe_info->dctg_param.lens_r;
		p_drv_cfg->dctg.lens_x_st = p_vpe_info->dctg_param.lens_x_st;
		p_drv_cfg->dctg.lens_y_st = p_vpe_info->dctg_param.lens_y_st;

		p_drv_cfg->dctg.theta_st = p_vpe_info->dctg_param.theta_st;
		p_drv_cfg->dctg.theta_ed = p_vpe_info->dctg_param.theta_ed;
		p_drv_cfg->dctg.phi_st = (0xFFFFF & p_vpe_info->dctg_param.phi_st);
		p_drv_cfg->dctg.phi_ed = (0xFFFFF & p_vpe_info->dctg_param.phi_ed);
		p_drv_cfg->dctg.rot_z = (0xFFFFF & p_vpe_info->dctg_param.rot_z);

		p_drv_cfg->dctg.rot_y = (0xFFFFF & p_vpe_info->dctg_param.rot_y);
	}

	return 0;
}


void vpe_set_dctg_reg(VPE_DCTG_CFG *p_dctg_cfg)
{
	if (p_dctg_cfg != NULL) {
		vpeg->reg_248.bit.dctg_mount_type = p_dctg_cfg->mount_type;
		vpeg->reg_248.bit.dctg_ra_en = p_dctg_cfg->ra_en;
		vpeg->reg_248.bit.dctg_lut2d_sz = p_dctg_cfg->lut2d_sz;
		vpeg->reg_248.bit.dctg_lens_r = p_dctg_cfg->lens_r;

		vpeg->reg_249.bit.dctg_lens_x_st = p_dctg_cfg->lens_x_st;
		vpeg->reg_249.bit.dctg_lens_y_st = p_dctg_cfg->lens_y_st;

		vpeg->reg_250.bit.dctg_theta_st = p_dctg_cfg->theta_st;
		vpeg->reg_251.bit.dctg_theta_ed = p_dctg_cfg->theta_ed;

		vpeg->reg_252.bit.dctg_phi_st = p_dctg_cfg->phi_st;
		vpeg->reg_253.bit.dctg_phi_ed = p_dctg_cfg->phi_ed;

		vpeg->reg_254.bit.dctg_rot_z = p_dctg_cfg->rot_z;
		vpeg->reg_255.bit.dctg_rot_y = p_dctg_cfg->rot_y;
	}

}




