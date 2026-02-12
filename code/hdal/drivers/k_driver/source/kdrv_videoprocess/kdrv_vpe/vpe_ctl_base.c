
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
#include "vpe_ctl_base.h"

#if 0

VPE_REG_DEFINE vpe_ctl_pipe_reg_def[VPE_REG_CTL_PIPE_MAXNUM] = {
	{VPE_REG_CTL_PIPE_SHP_PPI, 0, 0x3c0, 4, 0, 0, 0xf, {0, 0, 0, 0}},
	{VPE_REG_CTL_PIPE_SHP_PPO, 0, 0x3c0, 4, 4, 0, 0xf, {0, 0, 0, 0}},
	{VPE_REG_CTL_PIPE_SRC_PPO, 0, 0x3c4, 4, 0, 0, 0xf, {0, 0, 0, 0}},
	{VPE_REG_CTL_PIPE_SCA_PPI, 0, 0x3c4, 4, 4, 0, 0xf, {0, 0, 0, 0}},
};

VPE_REG_DEFINE vpe_ctl_global_reg_def[VPE_REG_CTL_GLO_MAXNUM] = {
	{VPE_REG_CTL_GLO_SHP_EN, 0, 0x4, 1, 2, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_CTL_GLO_DCE_EN, 0, 0x4, 1, 3, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_CTL_GLO_DCTG_EN, 0, 0x4, 1, 4, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_CTL_GLO_DCTG_GAMMA_LUT_EN, 0, 0x4, 1, 5, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_CTL_GLO_DCE_2D_LUT_EN, 0, 0x4, 1, 6, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_CTL_GLO_DEBUG_TYPE, 0, 0x4, 4, 20, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_CTL_GLO_COL_NUM, 0, 0x4, 3, 24, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_CTL_GLO_SRC_FORMAT, 0, 0x8, 3, 10, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_CTL_GLO_SHP_OUT_SEL, 0, 0x8, 1, 17, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_CTL_GLO_SRC_DRT, 0, 0x8, 2, 20, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_CTL_GLO_SRC_UV_SWAP, 0, 0x8, 1, 24, 0, 0x1, {0, 0, 0, 0}},

};

VPE_REG_DEFINE vpe_glo_size_reg_def[VPE_REG_GLO_SIZE_MAXNUM] = {
	{VPE_REG_GLO_SIZE_SRC_WIDTH, 0, 0x10, 14, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_GLO_SIZE_SRC_HEIGHT, 0, 0x10, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_GLO_SIZE_PROC_HEIGHT, 0, 0x1c, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_GLO_SIZE_PRESCA_MERGE_WIDTH, 0, 0x1c, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_GLO_SIZE_PROC_Y_START, 0, 0x20, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_GLO_SIZE_DCE_WIDTH, 0, 0x30, 13, 0, 0, 0x1fff, {0, 0, 0, 0}},
	{VPE_REG_GLO_SIZE_DCE_HEIGHT, 0, 0x30, 13, 16, 0, 0x1fff, {0, 0, 0, 0}},

};


inline UINT32 vpe_drv_ctl_glo_trans_config_to_regtable(VPE_GLO_CTL_CFG  *p_glo_ctl, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {

	case VPE_REG_CTL_GLO_SHP_EN:
		ret = p_glo_ctl->sharpen_en;
		break;
	case VPE_REG_CTL_GLO_DCE_EN:
		ret = p_glo_ctl->dce_en;
		break;
	case VPE_REG_CTL_GLO_DCTG_EN:
		ret = p_glo_ctl->dctg_en;
		break;
	case VPE_REG_CTL_GLO_DCTG_GAMMA_LUT_EN:
		ret = p_glo_ctl->dctg_gamma_lut_en;
		break;
	case VPE_REG_CTL_GLO_DCE_2D_LUT_EN:
		ret = p_glo_ctl->dce_2d_lut_en;
		break;
	case VPE_REG_CTL_GLO_DEBUG_TYPE:
		ret = p_glo_ctl->debug_type;
		break;
	case VPE_REG_CTL_GLO_COL_NUM:
		ret = p_glo_ctl->col_num;
		break;
	case VPE_REG_CTL_GLO_SRC_FORMAT:
		ret = p_glo_ctl->src_format;
		break;
	case VPE_REG_CTL_GLO_SHP_OUT_SEL:
		ret = p_glo_ctl->sharpen_out_sel;
		break;
	case VPE_REG_CTL_GLO_SRC_DRT:
		ret = p_glo_ctl->src_drt;
		break;
	case VPE_REG_CTL_GLO_SRC_UV_SWAP:
		ret = p_glo_ctl->src_uv_swap;
		break;
	}
	return ret;
}

UINT32 vpe_drv_glo_size_trans_config_to_regtable(VPE_GLO_SZ_CFG  *p_glo_sz, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_GLO_SIZE_SRC_WIDTH:
		ret = p_glo_sz->src_width;
		break;
	case VPE_REG_GLO_SIZE_SRC_HEIGHT:
		ret = p_glo_sz->src_height;
		break;
	case VPE_REG_GLO_SIZE_PROC_HEIGHT:
		ret = p_glo_sz->proc_height;
		break;
	case VPE_REG_GLO_SIZE_PRESCA_MERGE_WIDTH:
		ret = p_glo_sz->presca_merge_width;
		break;
	case VPE_REG_GLO_SIZE_PROC_Y_START:
		ret = p_glo_sz->proc_y_start;
		break;
	case VPE_REG_GLO_SIZE_DCE_WIDTH:
		ret = p_glo_sz->dce_width;
		break;
	case VPE_REG_GLO_SIZE_DCE_HEIGHT:
		ret = p_glo_sz->dce_height;
		break;


	}
	return ret;
}

inline UINT32 vpe_drv_ctl_pipe_trans_config_to_regtable(VPE_PIP_CTL_CFG  *p_pip_ctl, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_CTL_PIPE_SHP_PPI:
		ret = p_pip_ctl->shp_ppi_idx;
		break;
	case VPE_REG_CTL_PIPE_SHP_PPO:
		ret = p_pip_ctl->shp_ppo_idx;
		break;
	case VPE_REG_CTL_PIPE_SRC_PPO:
		ret = p_pip_ctl->src_ppo_idx;
		break;
	case VPE_REG_CTL_PIPE_SCA_PPI:
		ret = p_pip_ctl->sca_ppi_idx;
		break;
	}
	return ret;
}



int vpe_drv_ctl_trans_regtable_to_cmdlist(VPE_DRV_CFG  *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd)
{
	UINT32 i, j, mask;
	UINT32 *cmduffer;

	UINT32 tmpval = 0, regval = 0;

	cmduffer = (UINT32 *)p_llcmd->vaddr;

#if 0
	for (i = VPE_REG_CTL_GLO_START; i <= VPE_REG_CTL_GLO_END; i += VPE_REG_OFFSET) {
		tmpval = 0;
		mask = 0;
		for (j = 0; j < VPE_REG_CTL_GLO_MAXNUM; j++) {
			if (vpe_ctl_global_reg_def[j].reg_add == i) {
				if (vpe_ctl_global_reg_def[j].numofbits >= VPE_REG_MAXBIT) {
					mask = 0xffffffff;
				} else {
					mask = (1UL << vpe_ctl_global_reg_def[j].numofbits) - 1;
				}

				regval = vpe_drv_ctl_glo_trans_config_to_regtable(&p_drv_cfg->glo_ctl, j);
				tmpval |= ((UINT32)(regval & mask)) << vpe_ctl_global_reg_def[j].startBit;
			}
		}

		if (mask > 0) {
			cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
			cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, i);
		}
	}
#endif

#if 0
	for (i = VPE_REG_GLO_SIZE_START; i <= VPE_REG_GLO_SIZE_END; i += VPE_REG_OFFSET) {
		tmpval = 0;
		mask = 0;
		for (j = 0; j < VPE_REG_GLO_SIZE_MAXNUM; j++) {
			if (vpe_glo_size_reg_def[j].reg_add == i) {
				if (vpe_glo_size_reg_def[j].numofbits >= VPE_REG_MAXBIT) {
					mask = 0xffffffff;
				} else {
					mask = (1UL << vpe_glo_size_reg_def[j].numofbits) - 1;
				}

				regval = vpe_drv_glo_size_trans_config_to_regtable(&p_drv_cfg->glo_sz, j);
				tmpval |= ((UINT32)(regval & mask)) << vpe_glo_size_reg_def[j].startBit;
			}
		}

		if (mask > 0) {
			cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
			cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, i);
		}
	}
#endif

#if 0
	for (i = VPE_REG_CTL_PIPE_START; i <= VPE_REG_CTL_PIPE_END; i += VPE_REG_OFFSET) {
		tmpval = 0;
		mask = 0;
		for (j = 0; j < VPE_REG_CTL_PIPE_MAXNUM; j++) {
			if (vpe_ctl_pipe_reg_def[j].reg_add == i) {
				if (vpe_ctl_pipe_reg_def[j].numofbits >= VPE_REG_MAXBIT) {
					mask = 0xffffffff;
				} else {
					mask = (1UL << vpe_ctl_pipe_reg_def[j].numofbits) - 1;
				}

				regval = vpe_drv_ctl_pipe_trans_config_to_regtable(&p_drv_cfg->pip_ctl, j);
				tmpval |= ((UINT32)(regval & mask)) << vpe_ctl_pipe_reg_def[j].startBit;
			}
		}

		if (mask > 0) {
			cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
			cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, i);
		}
	}
#endif

	return 0;
}
#endif

int vpe_drv_ctl_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg)
{
	p_drv_cfg->glo_ctl.col_num = p_vpe_info->col_info.col_num;

#if 0
	p_drv_cfg->pip_ctl.shp_ppi_idx = 0;
	p_drv_cfg->pip_ctl.shp_ppo_idx = 1;
	p_drv_cfg->pip_ctl.src_ppo_idx = 0;
	p_drv_cfg->pip_ctl.sca_ppi_idx = 1;
#endif

	p_drv_cfg->glo_ctl.src_format = KDRV_VPE_SRC_TYPE_YUV420;
	p_drv_cfg->glo_ctl.src_uv_swap = p_vpe_info->glb_img_info.src_uv_swap;

	p_drv_cfg->glo_ctl.src_drt = p_vpe_info->glb_img_info.src_drt;

	if (p_vpe_info->func_mode & KDRV_VPE_FUNC_SHP_EN) {
		p_drv_cfg->glo_ctl.sharpen_en = 1;
		p_drv_cfg->glo_ctl.sharpen_out_sel = p_vpe_info->sharpen_param.sharpen_out_sel;
	} else {
		p_drv_cfg->glo_ctl.sharpen_en = 0;
	}

	if (p_vpe_info->func_mode & KDRV_VPE_FUNC_DCE_EN) {
		p_drv_cfg->glo_ctl.dce_en = 1;
	} else {
		p_drv_cfg->glo_ctl.dce_en = 0;
	}

	if (p_vpe_info->func_mode & KDRV_VPE_FUNC_DCTG_EN) {
		p_drv_cfg->glo_ctl.dctg_en = 1;
	} else {
		p_drv_cfg->glo_ctl.dctg_en = 0;
	}

	return 0;
}


void vpe_set_ctl_glo_reg(VPE_GLO_CTL_CFG  *p_glo_ctl)
{
	if (p_glo_ctl != NULL) {
		vpeg->reg_1.bit.vpe_shp_en = p_glo_ctl->sharpen_en;
		vpeg->reg_1.bit.dce_en = p_glo_ctl->dce_en;
		vpeg->reg_1.bit.vpe_dctg_en = p_glo_ctl->dctg_en;
		vpeg->reg_1.bit.vpe_dctg_gma_lut_en = p_glo_ctl->dctg_gamma_lut_en;
		vpeg->reg_1.bit.dce_2d_lut_en = p_glo_ctl->dce_2d_lut_en;
		vpeg->reg_1.bit.vpe_debug_type = p_glo_ctl->debug_type;
		vpeg->reg_1.bit.vpe_col_num = p_glo_ctl->col_num;


		vpeg->reg_2.bit.vpe_src_fmt = p_glo_ctl->src_format;
		vpeg->reg_2.bit.vpe_shp_out_sel = p_glo_ctl->sharpen_out_sel;
		vpeg->reg_2.bit.vpe_src_drt = p_glo_ctl->src_drt;
		vpeg->reg_2.bit.vpe_src_uv_swap = p_glo_ctl->src_uv_swap;
	}
}

void vpe_set_glo_size_reg(VPE_GLO_SZ_CFG *p_glo_sz)
{
	if (p_glo_sz != NULL) {
		vpeg->reg_4.bit.vpe_src_buf_width = p_glo_sz->src_width;
		vpeg->reg_4.bit.vpe_src_buf_height = p_glo_sz->src_height;

		vpeg->reg_7.bit.vpe_proc_height = p_glo_sz->proc_height;
		vpeg->reg_7.bit.presca_merge_width = p_glo_sz->presca_merge_width;

		vpeg->reg_8.bit.vpe_proc_y_start = p_glo_sz->proc_y_start;

		vpeg->reg_12.bit.dce_width = p_glo_sz->dce_width;
		vpeg->reg_12.bit.dce_height = p_glo_sz->dce_height;

		//printk("p_glo_sz->src_width: %d\r\n", p_glo_sz->src_width);
		//printk("p_glo_sz->src_height: %d\r\n", p_glo_sz->src_height);
		//printk("p_glo_sz->proc_height: %d\r\n", p_glo_sz->proc_height);
		//printk("p_glo_sz->presca_merge_width: %d\r\n", p_glo_sz->presca_merge_width);
		//printk("p_glo_sz->proc_y_start: %d\r\n", p_glo_sz->proc_y_start);
		//printk("p_glo_sz->dce_width: %d\r\n", p_glo_sz->dce_width);
		//printk("p_glo_sz->dce_height: %d\r\n", p_glo_sz->dce_height);
	}
}

void vpe_set_ctl_pipe_reg(void)  //VPE_PIP_CTL_CFG  *p_pip_ctl
{
#if 0
	p_drv_cfg->pip_ctl.shp_ppi_idx = 0;
	p_drv_cfg->pip_ctl.shp_ppo_idx = 1;
	p_drv_cfg->pip_ctl.src_ppo_idx = 0;
	p_drv_cfg->pip_ctl.sca_ppi_idx = 1;
#endif

	vpeg->reg_240.bit.shp_ppi_idx = 0;//p_pip_ctl->shp_ppi_idx;
	vpeg->reg_240.bit.shp_ppo_idx = 1;//p_pip_ctl->shp_ppo_idx;

	vpeg->reg_241.bit.src_ppo_idx = 0;//p_pip_ctl->src_ppo_idx;
	vpeg->reg_241.bit.sca_ppi_idx = 1;//p_pip_ctl->sca_ppi_idx;
}



