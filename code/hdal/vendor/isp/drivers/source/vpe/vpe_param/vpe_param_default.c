#include "kwrap/type.h"
#include "vpe_alg.h"

#include "vpe_param_default.h"

static VPE_SHARPEN_PARAM vpe_sharpen_param = {
	.enable = 0,
	.edge_weight_gain = 175,
	.edge_sharp_str1 = 40,
	.edge_sharp_str2 = 40,
	.flat_sharp_str = 20
};

static VPE_DCE_CTL_PARAM vpe_dce_ctl_param = {
	.enable = FALSE,
	.dce_mode = 0,
	.lens_radius = 0
};

static VPE_GDC_PARAM vpe_gdc_param = {
	.fovgain = 1024,
	.gdc_lut = {
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535},
	.cent_x_ratio = 500,
	.cent_y_ratio = 500,
};

static VPE_2DLUT_PARAM vpe_2dlut_param = {
	.lut_sz = 3,
	.lut = {0},
};

static VPE_DRT_PARAM vpe_drt_param = {
	.cvt_sel = {0},
};

static VPE_DCTG_PARAM vpe_dctg_param = {
	.mount_type = 1,
	.lut2d_sz = 3,
	.lens_r = 540,
	.lens_x_st = 420,
	.lens_y_st = 0,
	.theta_st = 51471, //pi/4 = 45 deg
	.theta_ed = 102943, //pi/2 = 90 deg
	.phi_st = -51472,  //-pi/4 = -45 deg
	.phi_ed = 51471,  //pi/4 = 45 deg
	.rot_z = 0,
	.rot_y = 0
};

static VPE_PARAM_PTR vpe_param_default = {
	&vpe_sharpen_param,
	&vpe_dce_ctl_param,
	&vpe_gdc_param,
	&vpe_2dlut_param,
	&vpe_drt_param,
	&vpe_dctg_param,
};

UINT32 vpe_get_param_default(void)
{
	return (UINT32)(&vpe_param_default);
}
