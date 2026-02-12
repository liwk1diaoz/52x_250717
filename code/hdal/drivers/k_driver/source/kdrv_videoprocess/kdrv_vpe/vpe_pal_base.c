
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
#include "vpe_pal_base.h"

#if 0

VPE_REG_DEFINE vpe_pal_reg_def[VPE_REG_PAL_MAXNUM] = {
	{VPE_REG_PAL_0_Y, 0, 0x260, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_0_CB, 0, 0x260, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_0_CR, 0, 0x260, 8, 16, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_PAL_1_Y, 0, 0x264, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_1_CB, 0, 0x264, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_1_CR, 0, 0x264, 8, 16, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_PAL_2_Y, 0, 0x268, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_2_CB, 0, 0x268, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_2_CR, 0, 0x268, 8, 16, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_PAL_3_Y, 0, 0x26c, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_3_CB, 0, 0x26c, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_3_CR, 0, 0x26c, 8, 16, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_PAL_4_Y, 0, 0x270, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_4_CB, 0, 0x270, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_4_CR, 0, 0x270, 8, 16, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_PAL_5_Y, 0, 0x274, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_5_CB, 0, 0x274, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_5_CR, 0, 0x274, 8, 16, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_PAL_6_Y, 0, 0x278, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_6_CB, 0, 0x278, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_6_CR, 0, 0x278, 8, 16, 0, 0xff, {0, 0, 0, 0}},

	{VPE_REG_PAL_7_Y, 0, 0x27c, 8, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_7_CB, 0, 0x27c, 8, 8, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_PAL_7_CR, 0, 0x27c, 8, 16, 0, 0xff, {0, 0, 0, 0}},
};


inline UINT32 vpe_drv_pal_trans_config_to_regtable(VPE_PALETTE_CFG *p_pal, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_PAL_0_Y:
		ret = p_pal[0].pal_y;
		break;
	case VPE_REG_PAL_0_CB:
		ret = p_pal[0].pal_cb;
		break;
	case VPE_REG_PAL_0_CR:
		ret = p_pal[0].pal_cr;
		break;
	case VPE_REG_PAL_1_Y:
		ret = p_pal[1].pal_y;
		break;
	case VPE_REG_PAL_1_CB:
		ret = p_pal[1].pal_cb;
		break;
	case VPE_REG_PAL_1_CR:
		ret = p_pal[1].pal_cr;
		break;
	case VPE_REG_PAL_2_Y:
		ret = p_pal[2].pal_y;
		break;
	case VPE_REG_PAL_2_CB:
		ret = p_pal[2].pal_cb;
		break;
	case VPE_REG_PAL_2_CR:
		ret = p_pal[2].pal_cr;
		break;
	case VPE_REG_PAL_3_Y:
		ret = p_pal[3].pal_y;
		break;
	case VPE_REG_PAL_3_CB:
		ret = p_pal[3].pal_cb;
		break;
	case VPE_REG_PAL_3_CR:
		ret = p_pal[3].pal_cr;
		break;
	case VPE_REG_PAL_4_Y:
		ret = p_pal[4].pal_y;
		break;
	case VPE_REG_PAL_4_CB:
		ret = p_pal[4].pal_cb;
		break;
	case VPE_REG_PAL_4_CR:
		ret = p_pal[4].pal_cr;
		break;
	case VPE_REG_PAL_5_Y:
		ret = p_pal[5].pal_y;
		break;
	case VPE_REG_PAL_5_CB:
		ret = p_pal[5].pal_cb;
		break;
	case VPE_REG_PAL_5_CR:
		ret = p_pal[5].pal_cr;
		break;
	case VPE_REG_PAL_6_Y:
		ret = p_pal[6].pal_y;
		break;
	case VPE_REG_PAL_6_CB:
		ret = p_pal[6].pal_cb;
		break;
	case VPE_REG_PAL_6_CR:
		ret = p_pal[6].pal_cr;
		break;
	case VPE_REG_PAL_7_Y:
		ret = p_pal[7].pal_y;
		break;
	case VPE_REG_PAL_7_CB:
		ret = p_pal[7].pal_cb;
		break;
	case VPE_REG_PAL_7_CR:
		ret = p_pal[7].pal_cr;
		break;

	}
	return ret;
}




int vpe_drv_pal_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd)
{
	UINT32 i, j, mask;
	UINT32 *cmduffer;
	UINT32 tmpval = 0, regoffset = 0, regval = 0;

	cmduffer = (UINT32 *)p_llcmd->vaddr;

	for (i = VPE_REG_PAL_START; i <= VPE_REG_PAL_END; i += VPE_REG_OFFSET) {
		tmpval = 0;
		mask = 0;
		regoffset = i;
		for (j = 0; j < VPE_REG_PAL_MAXNUM; j++) {
			if (vpe_pal_reg_def[j].reg_add == i) {
				if (vpe_pal_reg_def[j].numofbits >= VPE_REG_MAXBIT) {
					mask = 0xffffffff;
				} else {
					mask = (1UL << vpe_pal_reg_def[j].numofbits) - 1;
				}

				regval = vpe_drv_pal_trans_config_to_regtable(p_drv_cfg->palette, j);
				tmpval |= ((UINT32)(regval & mask)) << vpe_pal_reg_def[j].startBit;
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

int vpe_drv_pal_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg)
{
	INT32 i = 0;

	if ((p_vpe_info != NULL) && (p_drv_cfg != NULL)) {
		for (i = 0; i < 8; i++) {
			p_drv_cfg->palette[i].pal_y = p_vpe_info->palette[i].pal_y;
			p_drv_cfg->palette[i].pal_cb = p_vpe_info->palette[i].pal_cb;
			p_drv_cfg->palette[i].pal_cr = p_vpe_info->palette[i].pal_cr;
		}
	}

	return 0;
}

void vpe_set_pal_reg(VPE_PALETTE_CFG *p_pal_cfg)
{
	if (p_pal_cfg != NULL) {
		vpeg->reg_152.bit.pal0_y  = p_pal_cfg[0].pal_y;
		vpeg->reg_152.bit.pal0_cb = p_pal_cfg[0].pal_cb;
		vpeg->reg_152.bit.pal0_cr = p_pal_cfg[0].pal_cr;

		vpeg->reg_153.bit.pal1_y  = p_pal_cfg[1].pal_y;
		vpeg->reg_153.bit.pal1_cb = p_pal_cfg[1].pal_cb;
		vpeg->reg_153.bit.pal1_cr = p_pal_cfg[1].pal_cr;

		vpeg->reg_154.bit.pal2_y  = p_pal_cfg[2].pal_y;
		vpeg->reg_154.bit.pal2_cb = p_pal_cfg[2].pal_cb;
		vpeg->reg_154.bit.pal2_cr = p_pal_cfg[2].pal_cr;

		vpeg->reg_155.bit.pal3_y  = p_pal_cfg[3].pal_y;
		vpeg->reg_155.bit.pal3_cb = p_pal_cfg[3].pal_cb;
		vpeg->reg_155.bit.pal3_cr = p_pal_cfg[3].pal_cr;

		vpeg->reg_156.bit.pal4_y  = p_pal_cfg[4].pal_y;
		vpeg->reg_156.bit.pal4_cb = p_pal_cfg[4].pal_cb;
		vpeg->reg_156.bit.pal4_cr = p_pal_cfg[4].pal_cr;

		vpeg->reg_157.bit.pal5_y  = p_pal_cfg[5].pal_y;
		vpeg->reg_157.bit.pal5_cb = p_pal_cfg[5].pal_cb;
		vpeg->reg_157.bit.pal5_cr = p_pal_cfg[5].pal_cr;

		vpeg->reg_158.bit.pal6_y  = p_pal_cfg[6].pal_y;
		vpeg->reg_158.bit.pal6_cb = p_pal_cfg[6].pal_cb;
		vpeg->reg_158.bit.pal6_cr = p_pal_cfg[6].pal_cr;

		vpeg->reg_159.bit.pal7_y  = p_pal_cfg[7].pal_y;
		vpeg->reg_159.bit.pal7_cb = p_pal_cfg[7].pal_cb;
		vpeg->reg_159.bit.pal7_cr = p_pal_cfg[7].pal_cr;
	}
}




