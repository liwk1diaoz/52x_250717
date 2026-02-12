
#ifndef __VPE_CTL_BASE_H__
#define __VPE_CTL_BASE_H__

//#include "vpe_platform.h"


#define VPE_REG_CTL_GLO_START             0x4
#define VPE_REG_CTL_GLO_END               0xc

#define VPE_REG_GLO_SIZE_START             0x10
#define VPE_REG_GLO_SIZE_END               0x30


#define VPE_REG_CTL_PIPE_START             0x3c0
#define VPE_REG_CTL_PIPE_END               0x3c4

enum {
	VPE_REG_CTL_PIPE_SHP_PPI = 0,
	VPE_REG_CTL_PIPE_SHP_PPO,
	VPE_REG_CTL_PIPE_SRC_PPO,
	VPE_REG_CTL_PIPE_SCA_PPI,
	VPE_REG_CTL_PIPE_MAXNUM
};

enum {
	VPE_REG_CTL_GLO_SHP_EN = 0,
	VPE_REG_CTL_GLO_DCE_EN,
	VPE_REG_CTL_GLO_DCTG_EN,
	VPE_REG_CTL_GLO_DCTG_GAMMA_LUT_EN,
	VPE_REG_CTL_GLO_DCE_2D_LUT_EN,
	VPE_REG_CTL_GLO_DEBUG_TYPE,
	VPE_REG_CTL_GLO_COL_NUM,
	VPE_REG_CTL_GLO_SRC_FORMAT,
	VPE_REG_CTL_GLO_SHP_OUT_SEL,
	VPE_REG_CTL_GLO_SRC_DRT,
	VPE_REG_CTL_GLO_SRC_UV_SWAP,
	VPE_REG_CTL_GLO_MAXNUM
};

enum {
	VPE_REG_GLO_SIZE_SRC_WIDTH = 0,
	VPE_REG_GLO_SIZE_SRC_HEIGHT,
	VPE_REG_GLO_SIZE_PROC_HEIGHT,
	VPE_REG_GLO_SIZE_PRESCA_MERGE_WIDTH,
	VPE_REG_GLO_SIZE_PROC_Y_START,
	VPE_REG_GLO_SIZE_DCE_WIDTH,
	VPE_REG_GLO_SIZE_DCE_HEIGHT,
	VPE_REG_GLO_SIZE_MAXNUM
};

//extern int vpe_drv_ctl_trans_regtable_to_cmdlist(VPE_DRV_CFG  *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd);
extern int vpe_drv_ctl_config(KDRV_VPE_CONFIG *p_sca_info, VPE_DRV_CFG *p_drv_cfg);

extern void vpe_set_ctl_glo_reg(VPE_GLO_CTL_CFG  *p_glo_ctl);
extern void vpe_set_glo_size_reg(VPE_GLO_SZ_CFG *p_glo_sz);
extern void vpe_set_ctl_pipe_reg(void);
#endif
