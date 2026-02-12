
#if defined (__KERNEL__)

/*
#include <linux/version.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
//#include <linux/mm.h>
//#include <asm/uaccess.h>
//#include <asm/atomic.h>
//#include <asm/io.h>
*/

#elif defined (__FREERTOS)

#else

#endif

//#include "frammap/frammap_if.h"
#include "kdrv_videoprocess/kdrv_vpe.h"

#include "vpe_int.h"
#include "vpe_ll_base.h"
#include "vpe_config_base.h"
#include "vpe_shp_base.h"


VPE_REG_DEFINE vpe_shp_reg_def[VPE_REG_SHP_MAXNUM] = {
	{VPE_REG_SHP_EDGE_WEI_SRC_SEL, 0, 0x240, 1, 0, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_SHP_EDGE_WEI_TH, 0, 0x240, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_EDGE_WEI_GAIN, 0, 0x240, 8, 16, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_LV, 0, 0x240, 8, 24, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_SHP_EDGE_SHP_STR_1, 0, 0x244, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_EDGE_SHP_STR_2, 0, 0x244, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_FLAT_SHP_STR, 0, 0x244, 8, 16, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_SHP_CORING_TH, 0, 0x248, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_BLEND_INV_GAMMA, 0, 0x248, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_BRIGHT_HALO_CLIP, 0, 0x248, 8, 16, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_DARK_HALO_CLIP, 0, 0x248, 8, 24, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_SHP_NOISE_CURVE_0, 0, 0x24c, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_1, 0, 0x24c, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_2, 0, 0x24c, 8, 16, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_3, 0, 0x24c, 8, 24, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_SHP_NOISE_CURVE_4, 0, 0x250, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_5, 0, 0x250, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_6, 0, 0x250, 8, 16, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_7, 0, 0x250, 8, 24, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_SHP_NOISE_CURVE_8, 0, 0x254, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_9, 0, 0x254, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_10, 0, 0x254, 8, 16, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_11, 0, 0x254, 8, 24, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_SHP_NOISE_CURVE_12, 0, 0x258, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_13, 0, 0x258, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_14, 0, 0x258, 8, 16, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_SHP_NOISE_CURVE_15, 0, 0x258, 8, 24, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_SHP_NOISE_CURVE_16, 0, 0x25c, 8, 0, 0, 0xff, {0, 0, 0, 0}},
};


inline UINT32 vpe_drv_shp_trans_config_to_regtable(VPE_SHARPEN_CFG *p_shp, UINT32 regIndex)
{
	UINT32 ret = 0;

	if (p_shp != NULL) {
		switch (regIndex) {
		case VPE_REG_SHP_EDGE_WEI_SRC_SEL:
			ret = p_shp->edge_weight_src_sel;
			break;
		case VPE_REG_SHP_EDGE_WEI_TH:
			ret = p_shp->edge_weight_th;
			break;
		case VPE_REG_SHP_EDGE_WEI_GAIN:
			ret = p_shp->edge_weight_gain;
			break;
		case VPE_REG_SHP_NOISE_LV:
			ret = p_shp->noise_level;
			break;
		case VPE_REG_SHP_EDGE_SHP_STR_1:
			ret = p_shp->edge_sharp_str1;
			break;
		case VPE_REG_SHP_EDGE_SHP_STR_2:
			ret = p_shp->edge_sharp_str2;
			break;
		case VPE_REG_SHP_FLAT_SHP_STR:
			ret = p_shp->flat_sharp_str;
			break;
		case VPE_REG_SHP_CORING_TH:
			ret = p_shp->coring_th;
			break;
		case VPE_REG_SHP_BLEND_INV_GAMMA:
			ret = p_shp->blend_inv_gamma;
			break;
		case VPE_REG_SHP_BRIGHT_HALO_CLIP:
			ret = p_shp->bright_halo_clip;
			break;
		case VPE_REG_SHP_DARK_HALO_CLIP:
			ret = p_shp->dark_halo_clip;
			break;

		case VPE_REG_SHP_NOISE_CURVE_0:
		case VPE_REG_SHP_NOISE_CURVE_1:
		case VPE_REG_SHP_NOISE_CURVE_2:
		case VPE_REG_SHP_NOISE_CURVE_3:
		case VPE_REG_SHP_NOISE_CURVE_4:
		case VPE_REG_SHP_NOISE_CURVE_5:
		case VPE_REG_SHP_NOISE_CURVE_6:
		case VPE_REG_SHP_NOISE_CURVE_7:
		case VPE_REG_SHP_NOISE_CURVE_8:
		case VPE_REG_SHP_NOISE_CURVE_9:
		case VPE_REG_SHP_NOISE_CURVE_10:
		case VPE_REG_SHP_NOISE_CURVE_11:
		case VPE_REG_SHP_NOISE_CURVE_12:
		case VPE_REG_SHP_NOISE_CURVE_13:
		case VPE_REG_SHP_NOISE_CURVE_14:
		case VPE_REG_SHP_NOISE_CURVE_15:
		case VPE_REG_SHP_NOISE_CURVE_16:
			ret = p_shp->noise_curve[regIndex - VPE_REG_SHP_NOISE_CURVE_0];
			break;


		}
	} else {
		DBG_ERR("vpe: parameter NULL...\r\n");
	}

	return ret;
}



int vpe_drv_shp_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd)
{
	UINT32 i, j, mask;
	UINT32 *cmduffer;
	UINT32 tmpval = 0, regoffset = 0, regval = 0;

	if ((p_drv_cfg != NULL) && (p_llcmd != NULL)) {
		cmduffer = (UINT32 *)p_llcmd->vaddr;

		for (i = VPE_REG_SHP_START; i <= VPE_REG_SHP_END; i += VPE_REG_OFFSET) {
			tmpval = 0;
			mask = 0;
			regoffset = i;
			for (j = 0; j < VPE_REG_SHP_MAXNUM; j++) {
				if (vpe_shp_reg_def[j].reg_add == i) {
					if (vpe_shp_reg_def[j].numofbits >= VPE_REG_MAXBIT) {
						mask = 0xffffffff;
					} else {
						mask = (1UL << vpe_shp_reg_def[j].numofbits) - 1;
					}

					regval = vpe_drv_shp_trans_config_to_regtable(&(p_drv_cfg->sharpen), j);
					tmpval |= ((UINT32)(regval & mask)) << vpe_shp_reg_def[j].startBit;
				}
			}

			if (mask > 0) {
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, regoffset);
			}
		}

		return 0;
	} else {
		DBG_ERR("vpe: parameter NULL...\r\n");

		return -1;
	}
}

int vpe_drv_shp_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg)
{
	INT32 i = 0;

	if ((p_vpe_info != NULL) && (p_drv_cfg != NULL)) {
		p_drv_cfg->sharpen.edge_weight_src_sel = p_vpe_info->sharpen_param.edge_weight_src_sel;
		p_drv_cfg->sharpen.edge_weight_th = p_vpe_info->sharpen_param.edge_weight_th;
		p_drv_cfg->sharpen.edge_weight_gain = p_vpe_info->sharpen_param.edge_weight_gain;
		p_drv_cfg->sharpen.noise_level = p_vpe_info->sharpen_param.noise_level;

		for (i = 0; i < 17; i++) {
			p_drv_cfg->sharpen.noise_curve[i] = p_vpe_info->sharpen_param.noise_curve[i];
		}

		p_drv_cfg->sharpen.blend_inv_gamma = p_vpe_info->sharpen_param.blend_inv_gamma;
		p_drv_cfg->sharpen.edge_sharp_str1 = p_vpe_info->sharpen_param.edge_sharp_str1;

		p_drv_cfg->sharpen.edge_sharp_str2 = p_vpe_info->sharpen_param.edge_sharp_str2;
		p_drv_cfg->sharpen.flat_sharp_str = p_vpe_info->sharpen_param.flat_sharp_str;
		p_drv_cfg->sharpen.coring_th = p_vpe_info->sharpen_param.coring_th;
		p_drv_cfg->sharpen.bright_halo_clip = p_vpe_info->sharpen_param.bright_halo_clip;
		p_drv_cfg->sharpen.dark_halo_clip = p_vpe_info->sharpen_param.dark_halo_clip;

		return 0;
	} else {
		DBG_ERR("vpe: parameter NULL...\r\n");

		return -1;
	}
}






