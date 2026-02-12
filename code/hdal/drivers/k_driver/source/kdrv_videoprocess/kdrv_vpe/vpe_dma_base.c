
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
#include "vpe_dma_base.h"

#include "kdrv_vpe_int.h"

//#include <mach/nvt_pcie.h>

VPE_REG_DEFINE vpe_dma_reg_def[VPE_REG_DMA_MAXNUM] = {
	{VPE_REG_DMA_LLC, 0, 0x300, 32, 0, 0, 0xffffffff, {0, 0, 0, 0}},
	{VPE_REG_DMA_SRC, 0, 0x304, 32, 0, 0, 0xffffffff, {0, 0, 0, 0}},
	{VPE_REG_DMA_DCTG_GAMMA_ADDR, 0, 0x31c, 32, 0, 0, 0xffffffff, {0, 0, 0, 0}},
	{VPE_REG_DMA_DCE_2DLUT_ADDR, 0, 0x320, 32, 0, 0, 0xffffffff, {0, 0, 0, 0}},
	{VPE_REG_DMA_DES_RES0_DP0, 0, 0x340, 32, 0, 0, 0xffffffff, {0, 0, 0, 0}},
	{VPE_REG_DMA_DES_RES1_DP0, 0, 0x34c, 32, 0, 0, 0xffffffff, {0, 0, 0, 0}},
	{VPE_REG_DMA_DES_RES2_DP0, 0, 0x358, 32, 0, 0, 0xffffffff, {0, 0, 0, 0}},
	{VPE_REG_DMA_DES_RES3_DP0, 0, 0x364, 32, 0, 0, 0xffffffff, {0, 0, 0, 0}},

};
inline UINT32 vpe_drv_dma_trans_config_to_regtable(VPE_DMA_CFG *p_dma, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_DMA_LLC:
		ret = p_dma->llc_addr;
		break;
	case VPE_REG_DMA_SRC:
		ret = p_dma->src_addr;
		break;
	case VPE_REG_DMA_DCTG_GAMMA_ADDR:
		ret = p_dma->dctg_gamma_addr;
		break;
	case VPE_REG_DMA_DCE_2DLUT_ADDR:
		ret = p_dma->dce_2dlut_addr;
		break;
	case VPE_REG_DMA_DES_RES0_DP0:
		ret = p_dma->des_res_dp_addr[0];
		break;
	case VPE_REG_DMA_DES_RES1_DP0:
		ret = p_dma->des_res_dp_addr[1];
		break;
	case VPE_REG_DMA_DES_RES2_DP0:
		ret = p_dma->des_res_dp_addr[2];
		break;
	case VPE_REG_DMA_DES_RES3_DP0:
		ret = p_dma->des_res_dp_addr[3];
		break;

	}
	return ret;
}




int vpe_drv_dma_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd)
{
	UINT32 i, j, mask;
	UINT32 *cmduffer;
	UINT32 tmpval = 0, regoffset = 0, regval = 0;

	if ((p_drv_cfg != NULL) && (p_llcmd != NULL)) {

		cmduffer = (UINT32 *)p_llcmd->vaddr;

		for (i = VPE_REG_DMA_START; i <= VPE_REG_DMA_END; i += VPE_REG_OFFSET) {
			tmpval = 0;
			mask = 0;
			regoffset = i;
			for (j = 0; j < VPE_REG_DMA_MAXNUM; j++) {
				if (vpe_dma_reg_def[j].reg_add == i) {
					if (vpe_dma_reg_def[j].numofbits >= VPE_REG_MAXBIT) {
						mask = 0xffffffff;
					} else {
						mask = (1UL << vpe_dma_reg_def[j].numofbits) - 1;
					}

					regval = vpe_drv_dma_trans_config_to_regtable(&(p_drv_cfg->dma), j);
					tmpval |= ((UINT32)(regval & mask)) << vpe_dma_reg_def[j].startBit;
				}
			}

			if (mask > 0) {
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM_VAL(tmpval);
				cmduffer[p_llcmd->cmd_num++] = VPE_LLCMD_NORM(0xf, regoffset);
			}
		}

	}

	return 0;
}

int vpe_drv_dma_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg)
{
	INT32 i = 0, j = 0;

	if ((p_vpe_info != NULL) && (p_drv_cfg != NULL)) {
		p_drv_cfg->dma.src_addr = p_vpe_info->glb_img_info.src_addr;


		for (i = 0; i < KDRV_VPE_MAX_IMG; i++) {
			for (j = 0; j < p_vpe_info->out_img_info[i].out_dup_num; j++) {
				p_drv_cfg->dma.des_res_dp_addr[i] = p_vpe_info->out_img_info[i].out_dup_info.out_addr;
			}
		}
	}

	return 0;
}






