#include "ctl_vpe_isp.h"


CTL_VPE_ISP_SHARPEN_PARAM ctl_vpe_isp_sharpen_init = {
	.enable = FALSE,
	.edge_weight_src_sel = FALSE,
	.edge_weight_th = 2,
	.edge_weight_gain = 175,
	.noise_level = 15,
	.noise_curve = {
		0, 38, 46, 51, 54, 57, 59, 61, 62, 62,
		62, 62, 62, 62, 62, 62, 62},
	.blend_inv_gamma = 128,
	.edge_sharp_str1 = 40,
	.edge_sharp_str2 = 40,
	.flat_sharp_str = 20,
	.coring_th = 0,
	.bright_halo_clip = 128,
	.dark_halo_clip = 128,
	.sharpen_out_sel = 0
};

CTL_VPE_ISP_DCE_CTL ctl_vpe_isp_dce_ctl_init = {
	.enable = FALSE,
	.lsb_rand = 0,
	.dce_mode = 1,
	.lens_radius = 0
};

CTL_VPE_ISP_DCE_GDC_PARAM ctl_vpe_isp_gdc_init = {
	.fovbound = 0,
	.boundy = 0,
	.boundu = 0,
	.boundv = 0,
	.xdist_a1 = 4095,
	.ydist_a1 = 4095,
	.fovgain = 1024,
	.geo_lut = {
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535,
			65535, 65535, 65535, 65535, 65535},
	.cent_x_ratio = 500,
	.cent_y_ratio = 500,
	.cent_ratio_base = 1000
};

CTL_VPE_ISP_DCE_2DLUT_PARAM ctl_vpe_isp_2dlut_init = {
	.xofs_i = 0,
	.xofs_f = 0,
	.yofs_i = 0,
	.yofs_f = 0,
	.lut_sz = 5,
	.lut = {0},
	.out_size = {
		.w = 0,
		.h = 0
	}
};

CTL_VPE_ISP_YUV_CVT_PARAM ctl_vpe_isp_cvt_init = {
	.cvt_sel = {0}
};

CTL_VPE_ISP_FLIP_ROT_CTL ctl_vpe_flip_rot_init = 
{
	.flip_rot_mode = CTL_VPE_ISP_ROTATE_0,
	.rot_manual_param = {
		.rot_degree = 0,
		.flip = CTL_VPE_ISP_FLIP_NONE,
		.ratio_mode = CTL_VPE_ISP_ROT_RAT_MANUAL,
		.ratio = 100,
		.fovbound = 1,
		.boundy = 128,
		.boundu = 0,
		.boundv = 0
	}
};

CTL_VPE_ISP_DCTG_PARAM ctl_vpe_isp_dctg_init = {
	.enable = 0,
	.mount_type = 1,
	.lut2d_sz = 3,
	.lens_r = 540,
	.lens_x_st = 420,
	.lens_y_st = 0,
	.theta_st = 51471, //pi/4 = 45 deg
	.theta_ed = 102943, //pi/2 = 90 deg
	.phi_st = -51472,  //-pi/4 = -45 deg
	.phi_ed =  51471,  //pi/4 = 45 deg
	.rot_y = 0,
	.rot_z = 0,
	.out_size = {
		.w = 0,
		.h = 0
	}
};


