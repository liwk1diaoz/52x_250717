#ifndef __VPE_DCE_BASE_H__
#define __VPE_DCE_BASE_H__

//#include "vpe_platform.h"
#include "vpe_dce_cal.h"


#define VPE_REG_DCE_START             0xc0
#define VPE_REG_DCE_END               0xe8

#define VPE_2DLUT_FRAC_BIT_NUM 24

typedef enum {
	VPE_DCE_2DLUT_9_9_GRID      = 0,     ///< 9x9 grid 2D lut distortion correction
	VPE_DCE_2DLUT_65_65_GRID    = 3,     ///< 65x65 grid 2D lut distortion correction
	VPE_DCE_2DLUT_129_129_GRID  = 4,     ///< 129x129 grid 2D lut distortion correction
	VPE_DCE_2DLUT_257_257_GRID  = 5,     ///< 257x257 grid 2D lut distortion correction
	ENUM_DUMMY4WORD(VEP_DCE_2DLUT_NUM)
} VPE_DCE_2DLUT_NUM;

enum {
	VPE_REG_DCE_DCE_MODE = 0,
	VPE_REG_DCE_LUT2D_SZ,
	VPE_REG_DCE_LSB_RAND,
	VPE_REG_DCE_FOVBOUND,
	VPE_REG_DCE_BOUNDY,
	VPE_REG_DCE_BOUNDU,
	VPE_REG_DCE_BOUNDV,
	VPE_REG_DCE_CENT_X_S,
	VPE_REG_DCE_CENT_Y_S,
	VPE_REG_DCE_XDIST,
	VPE_REG_DCE_YDIST,
	VPE_REG_DCE_NORMFACT,
	VPE_REG_DCE_NORMBIT,
	VPE_REG_DCE_FOVGAIN,
	VPE_REG_DCE_HFACT,
	VPE_REG_DCE_VFACT,
	VPE_REG_DCE_XOFS_I,
	VPE_REG_DCE_XOFS_F,
	VPE_REG_DCE_YOFS_I,
	VPE_REG_DCE_YOFS_F,
	VPE_REG_DCE_MAXNUM
};

extern int vpe_drv_dce_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg, struct vpe_drv_lut2d_info *lut2d);
//extern int vpe_drv_dce_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd);


extern void vpe_set_dce_reg(VPE_DCE_CFG *p_dce_cfg);
extern void vpe_set_dce_geo_lut_reg(VPE_DRV_CFG *p_drv_cfg);

#endif
