#if defined(__FREERTOS)
#include "string.h"
#else
#include <linux/syscalls.h>
#endif

#include "vpe_alg.h"
#include "vpe_dev_int.h"
#include "vpe_common_param_int.h"
#include "vpet_api_int.h"
#include "ctl_vpe_isp.h"
#include "kwrap/error_no.h"
#include "vpe_dbg.h"
#include "vpe_uti.h"

extern BOOL vpe_param_id_valid[VPE_ID_MAX_NUM];
USIZE vpe_2dlut_output_size[VPE_ID_MAX_NUM]={0};
USIZE vpe_dctg_output_size[VPE_ID_MAX_NUM] = {0};

static BOOL first_run[VPE_ID_MAX_NUM] = {TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};

//=============================================================================
// global
//=============================================================================
void vpe_set_sharpen_param(VPE_ID id)
{
	ctl_vpe_isp_sharpen_init.enable = vpe_param[id]->sharpen->enable;
	ctl_vpe_isp_sharpen_init.edge_sharp_str1 = vpe_param[id]->sharpen->edge_sharp_str1;
	ctl_vpe_isp_sharpen_init.edge_sharp_str2 = vpe_param[id]->sharpen->edge_sharp_str2;
	ctl_vpe_isp_sharpen_init.flat_sharp_str = vpe_param[id]->sharpen->flat_sharp_str;
	ctl_vpe_isp_sharpen_init.edge_weight_gain = vpe_param[id]->sharpen->edge_weight_gain;
	vpet_reset_param_update(id, VPET_ITEM_SHARPEN_PARAM);
	return ;
}

void vpe_set_dce_ctl_param(VPE_ID id)
{
	ctl_vpe_isp_dce_ctl_init.enable = vpe_param[id]->dce_ctl->enable;
	ctl_vpe_isp_dce_ctl_init.dce_mode = vpe_param[id]->dce_ctl->dce_mode;
	ctl_vpe_isp_dce_ctl_init.lens_radius = vpe_param[id]->dce_ctl->lens_radius;
	vpet_reset_param_update(id, VPET_ITEM_DCE_CTL_PARAM);
	return ;
}

void vpe_set_gdc_param(VPE_ID id)
{
	ctl_vpe_isp_gdc_init.fovgain = vpe_param[id]->gdc->fovgain;
	ctl_vpe_isp_gdc_init.cent_x_ratio = vpe_param[id]->gdc->cent_x_ratio;
	ctl_vpe_isp_gdc_init.cent_y_ratio = vpe_param[id]->gdc->cent_y_ratio;
	memcpy(ctl_vpe_isp_gdc_init.geo_lut, vpe_param[id]->gdc->gdc_lut, sizeof(UINT16) * VPE_GDC_LUT_NUMS);
	vpet_reset_param_update(id, VPET_ITEM_GDC_PARAM);
	return ;
}


void vpe_set_2dlut_param(VPE_ID id)
{
	ctl_vpe_isp_2dlut_init.lut_sz = vpe_param[id]->lut2d->lut_sz;
	memcpy(ctl_vpe_isp_2dlut_init.lut, vpe_param[id]->lut2d->lut, sizeof(UINT32) * VPE_2DLUT_NUM);
	ctl_vpe_isp_2dlut_init.out_size.w = vpe_2dlut_output_size[id].w;
	ctl_vpe_isp_2dlut_init.out_size.h = vpe_2dlut_output_size[id].h;
	vpet_reset_param_update(id, VPET_ITEM_2DLUT_PARAM);
	return ;
}

void vpe_set_drt_param(VPE_ID id)
{
	memcpy(ctl_vpe_isp_cvt_init.cvt_sel, vpe_param[id]->drt->cvt_sel, sizeof(VPE_DRT_PARAM));
	vpet_reset_param_update(id, VPET_ITEM_DRT_PARAM);
	return ;
}

void vpe_set_dctg_param(VPE_ID id)
{
	ctl_vpe_isp_dctg_init.lens_r = vpe_param[id]->dctg->lens_r;
	ctl_vpe_isp_dctg_init.lens_x_st = vpe_param[id]->dctg->lens_x_st;
	ctl_vpe_isp_dctg_init.lens_y_st = vpe_param[id]->dctg->lens_y_st;
	ctl_vpe_isp_dctg_init.lut2d_sz = vpe_param[id]->dctg->lut2d_sz;
	ctl_vpe_isp_dctg_init.mount_type = vpe_param[id]->dctg->mount_type;
	ctl_vpe_isp_dctg_init.phi_ed = vpe_param[id]->dctg->phi_ed;
	ctl_vpe_isp_dctg_init.phi_st = vpe_param[id]->dctg->phi_st;
	ctl_vpe_isp_dctg_init.rot_y = vpe_param[id]->dctg->rot_y;
	ctl_vpe_isp_dctg_init.rot_z = vpe_param[id]->dctg->rot_z;
	ctl_vpe_isp_dctg_init.theta_ed = vpe_param[id]->dctg->theta_ed;
	ctl_vpe_isp_dctg_init.theta_st = vpe_param[id]->dctg->theta_st;
	ctl_vpe_isp_dctg_init.out_size.w = vpe_dctg_output_size[id].w;
	ctl_vpe_isp_dctg_init.out_size.h = vpe_dctg_output_size[id].h;
	vpet_reset_param_update(id, VPET_ITEM_DCTG_PARAM);
	return ;
}

static void vpe_set_flip_rot_ctrl(VPE_ID id)
{
	ctl_vpe_flip_rot_init.flip_rot_mode = vpe_param[id]->flip_rot->flip_rot_mode;
	ctl_vpe_flip_rot_init.rot_manual_param.rot_degree = vpe_param[id]->flip_rot->rot_manual_param.rot_degree;
	ctl_vpe_flip_rot_init.rot_manual_param.flip = vpe_param[id]->flip_rot->rot_manual_param.flip;
	ctl_vpe_flip_rot_init.rot_manual_param.ratio_mode = vpe_param[id]->flip_rot->rot_manual_param.ratio_mode;
	ctl_vpe_flip_rot_init.rot_manual_param.fovbound = vpe_param[id]->flip_rot->rot_manual_param.fovbound;
	ctl_vpe_flip_rot_init.rot_manual_param.boundy = vpe_param[id]->flip_rot->rot_manual_param.boundy;
	ctl_vpe_flip_rot_init.rot_manual_param.boundu = vpe_param[id]->flip_rot->rot_manual_param.boundu;
	ctl_vpe_flip_rot_init.rot_manual_param.boundv = vpe_param[id]->flip_rot->rot_manual_param.boundv;
	vpet_reset_param_update(id, VPET_ITEM_FLIP_ROT_PARAM);
	return;
}

//=============================================================================
// external functions
//=============================================================================
INT32 vpe_api_cb_flow(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param)
{
	CTL_VPE_ISP_IQ_ALL data = {NULL};
	UINT32 dbg_mode = vpe_dbg_get_dbg_mode(id);
	INT32 rt = E_OK;
	CTL_VPE_ISP_CB_PARAM *cb_param = (CTL_VPE_ISP_CB_PARAM *)param;

	if (vpe_param_id_valid[id]!= TRUE) {
		PRINT_VPE_ERR(dbg_mode & VPE_DBG_ERR_MSG, "invalid VPE ID (%d)!! \r\n", id);
		return 0;
	}

	if (evt == ISP_EVENT_PARAM_RST) {
		first_run[id] = TRUE;
	}

	if (first_run[id] == TRUE) {
		vpe_set_sharpen_param(id);
		data.p_sharpen_param = &ctl_vpe_isp_sharpen_init;
	
		vpe_set_dce_ctl_param(id);
		data.p_dce_ctl = &ctl_vpe_isp_dce_ctl_init;

		vpe_set_gdc_param(id);
		data.p_dce_gdc_param = &ctl_vpe_isp_gdc_init;

		if((data.p_dce_ctl->enable == TRUE) && (data.p_dce_ctl->dce_mode == 1)){
			vpe_set_2dlut_param(id);
			data.p_dce_2dlut_param = &ctl_vpe_isp_2dlut_init;
			PRINT_VPE(dbg_mode & VPE_DBG_UPDATE, "VPE(%d) 2dlut enable- !! \r\n", id);
		}

		vpe_set_drt_param(id);
		data.p_yuv_cvt_param = &ctl_vpe_isp_cvt_init;

		vpe_set_dctg_param(id);
		data.p_dctg_param = &ctl_vpe_isp_dctg_init;

		if(data.p_dce_ctl->dce_mode == CTL_VPE_ISP_DCE_MODE_2DLUT_DCTG){
			data.p_dctg_param->enable = 1;
		}else{
			data.p_dctg_param->enable = 0;
		}

		rt = ctl_vpe_isp_set(id, CTL_VPE_ISP_ITEM_IQ_PARAM, (CTL_VPE_ISP_IQ_ALL *)&data);

		if(rt != E_OK){
			PRINT_VPE(dbg_mode & VPE_DBG_UPDATE, "VPE(%d) first set fail- !! \r\n", id);
		}
	}
	
	if (evt != ISP_EVENT_PARAM_RST) {
		first_run[id] = FALSE;
	}
	
	if (first_run[id] == FALSE) {
		vpe_dctg_output_size[id].w = cb_param->dce_in_size.w;
		vpe_dctg_output_size[id].h = cb_param->dce_in_size.h;
	
		if (vpet_get_param_update(id, VPET_ITEM_SHARPEN_PARAM) == TRUE) {
			vpe_set_sharpen_param(id);
			data.p_sharpen_param = &ctl_vpe_isp_sharpen_init;
			PRINT_VPE(dbg_mode & VPE_DBG_UPDATE, "VPE(%d) trig - sharpen update!! \r\n", id);
		} else {
			data.p_sharpen_param = NULL;
		}

		if (vpet_get_param_update(id, VPET_ITEM_DCE_CTL_PARAM) == TRUE) {
			vpe_set_dce_ctl_param(id);
			data.p_dce_ctl = &ctl_vpe_isp_dce_ctl_init;
			PRINT_VPE(dbg_mode & VPE_DBG_UPDATE, "VPE(%d) trig - dce ctl update!! \r\n", id);
		}else {
			data.p_dce_ctl = NULL;
		}

		if (vpet_get_param_update(id, VPET_ITEM_GDC_PARAM) == TRUE) {
			vpe_set_gdc_param(id);
			data.p_dce_gdc_param = &ctl_vpe_isp_gdc_init;
			PRINT_VPE(dbg_mode & VPE_DBG_UPDATE, "VPE(%d) trig - gdc update!! \r\n", id);
		} else {
			data.p_dce_gdc_param = NULL;
		}

		if (vpet_get_param_update(id, VPET_ITEM_2DLUT_PARAM) == TRUE) {
			vpe_set_2dlut_param(id);
			data.p_dce_2dlut_param = &ctl_vpe_isp_2dlut_init;
			PRINT_VPE(dbg_mode & VPE_DBG_UPDATE, "VPE(%d) trig - 2dlut update!! \r\n", id);
		} else {
			data.p_dce_2dlut_param = NULL;
		}

		if (vpet_get_param_update(id, VPET_ITEM_DRT_PARAM) == TRUE) {
			vpe_set_drt_param(id);
			data.p_yuv_cvt_param = &ctl_vpe_isp_cvt_init;
			PRINT_VPE(dbg_mode & VPE_DBG_UPDATE, "VPE(%d) trig - drt update!! \r\n", id);
		} else {
			data.p_yuv_cvt_param = NULL;
		}

		if (vpet_get_param_update(id, VPET_ITEM_DCTG_PARAM) == TRUE) {
			vpe_dctg_output_size[id].w = cb_param->dce_in_size.w;
			vpe_dctg_output_size[id].h = cb_param->dce_in_size.h;
			vpe_set_dctg_param(id);
			data.p_dctg_param = &ctl_vpe_isp_dctg_init;
			PRINT_VPE(dbg_mode & VPE_DBG_UPDATE, "VPE(%d) trig - dctg update!! \r\n", id);
		} else {
			data.p_dctg_param = NULL;
		}

		if (vpe_param[id]->dce_ctl->dce_mode == VPE_ISP_DCE_MODE_2DLUT_DCTG) {
			vpe_set_dctg_param(id);
			data.p_dctg_param = &ctl_vpe_isp_dctg_init;
			data.p_dctg_param->enable = 1;
		} else {
			vpe_set_dctg_param(id);
			data.p_dctg_param = &ctl_vpe_isp_dctg_init;
			data.p_dctg_param->enable = 0;
		}

		if (vpet_get_param_update(id, VPET_ITEM_FLIP_ROT_PARAM) == TRUE) {
			vpe_set_flip_rot_ctrl(id);
			data.p_dce_ctl = &ctl_vpe_isp_dce_ctl_init;
			data.p_flip_rot_ctl = &ctl_vpe_flip_rot_init;
			vpe_set_flip_rot_ctrl(id);
			if (ctl_vpe_isp_dce_ctl_init.enable && (ctl_vpe_flip_rot_init.flip_rot_mode != CTL_VPE_ISP_ROTATE_0)) {
				PRINT_VPE(dbg_mode & VPE_DBG_UPDATE, "vpe flip_rot is turned on, force dce enable to 0 (%d)  \r\n", id);
				ctl_vpe_isp_dce_ctl_init.enable = 0;
			}
			PRINT_VPE(dbg_mode & VPE_DBG_UPDATE, "VPE(%d) trig - flip_rot update!! \r\n", id);
		}

		ctl_vpe_isp_set(id, CTL_VPE_ISP_ITEM_IQ_PARAM, (CTL_VPE_ISP_IQ_ALL *)&data);
	}

	return 0;
}


