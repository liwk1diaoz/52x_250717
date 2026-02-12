
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

#include "vpe_platform.h"

#include "vpe_int.h"
#include "vpe_ll_base.h"
#include "vpe_config_base.h"
#include "vpe_dce_base.h"




#if 0

VPE_REG_DEFINE vpe_dce_reg_def[VPE_REG_DCE_MAXNUM] = {
	{VPE_REG_DCE_DCE_MODE, 0, 0xc0, 2, 0, 0, 0x3, {0, 0, 0, 0}},
	{VPE_REG_DCE_LUT2D_SZ, 0, 0xc0, 3, 2, 0, 0x3, {0, 0, 0, 0}},
	{VPE_REG_DCE_LSB_RAND, 0, 0xc0, 1, 5, 0, 0x1, {0, 0, 0, 0}},

	{VPE_REG_DCE_FOVBOUND, 0, 0xc4, 1, 0, 0, 0x1, {0, 0, 0, 0}},
	{VPE_REG_DCE_BOUNDY, 0, 0xc4, 10, 16, 0, 0x3ff, {0, 0, 0, 0}},

	{VPE_REG_DCE_BOUNDU, 0, 0xc8, 10, 0, 0, 0x3ff, {0, 0, 0, 0}},
	{VPE_REG_DCE_BOUNDV, 0, 0xc8, 10, 16, 0, 0x3ff, {0, 0, 0, 0}},

	{VPE_REG_DCE_CENT_X_S, 0, 0xcc, 14, 0, 0, 0x3fff, {0, 0, 0, 0}},
	{VPE_REG_DCE_CENT_Y_S, 0, 0xcc, 14, 16, 0, 0x3fff, {0, 0, 0, 0}},

	{VPE_REG_DCE_XDIST, 0, 0xd0, 12, 0, 0, 0xfff, {0, 0, 0, 0}},
	{VPE_REG_DCE_YDIST, 0, 0xd0, 12, 16, 0, 0xfff, {0, 0, 0, 0}},

	{VPE_REG_DCE_NORMFACT, 0, 0xd4, 10, 0, 0, 0xff, {0, 0, 0, 0}},
	{VPE_REG_DCE_NORMBIT, 0, 0xd4, 5, 12, 0, 0x1f, {0, 0, 0, 0}},
	{VPE_REG_DCE_FOVGAIN, 0, 0xd4, 12, 20, 0, 0xfff, {0, 0, 0, 0}},

	{VPE_REG_DCE_HFACT, 0, 0xd8, 24, 0, 0, 0xffffff, {0, 0, 0, 0}},

	{VPE_REG_DCE_VFACT, 0, 0xdc, 24, 0, 0, 0xffffff, {0, 0, 0, 0}},

	{VPE_REG_DCE_XOFS_I, 0, 0xe0, 9, 0, 0, 0x7f, {0, 0, 0, 0}},
	{VPE_REG_DCE_YOFS_I, 0, 0xe0, 9, 16, 0, 0x7f, {0, 0, 0, 0}},

	{VPE_REG_DCE_XOFS_F, 0, 0xe4, 24, 0, 0, 0xffffff, {0, 0, 0, 0}},

	{VPE_REG_DCE_YOFS_F, 0, 0xe8, 24, 0, 0, 0xffffff, {0, 0, 0, 0}},
};


inline UINT32 vpe_drv_dce_trans_config_to_regtable(VPE_DCE_CFG *p_dce, UINT32 regIndex)
{
	UINT32 ret = 0;
	switch (regIndex) {
	case VPE_REG_DCE_DCE_MODE:
		ret = p_dce->dce_mode;
		break;
	case VPE_REG_DCE_LUT2D_SZ:
		ret = p_dce->lut2d_sz;
		break;
	case VPE_REG_DCE_LSB_RAND:
		ret = p_dce->lsb_rand;
		break;
	case VPE_REG_DCE_FOVBOUND:
		ret = p_dce->fovbound;
		break;
	case VPE_REG_DCE_BOUNDY:
		ret = p_dce->boundy;
		break;
	case VPE_REG_DCE_BOUNDU:
		ret = p_dce->boundu;
		break;
	case VPE_REG_DCE_BOUNDV:
		ret = p_dce->boundv;
		break;
	case VPE_REG_DCE_CENT_X_S:
		ret = p_dce->cent_x_s;
		break;
	case VPE_REG_DCE_CENT_Y_S:
		ret = p_dce->cent_y_s;
		break;
	case VPE_REG_DCE_XDIST:
		ret = p_dce->xdist_a1;
		break;
	case VPE_REG_DCE_YDIST:
		ret = p_dce->ydist_a1;
		break;
	case VPE_REG_DCE_NORMFACT:
		ret = p_dce->normfact;
		break;
	case VPE_REG_DCE_NORMBIT:
		ret = p_dce->normbit;
		break;
	case VPE_REG_DCE_FOVGAIN:
		ret = p_dce->fovgain;
		break;
	case VPE_REG_DCE_HFACT:
		ret = p_dce->hfact;
		break;
	case VPE_REG_DCE_VFACT:
		ret = p_dce->vfact;
		break;
	case VPE_REG_DCE_XOFS_I:
		ret = p_dce->xofs_i;
		break;
	case VPE_REG_DCE_XOFS_F:
		ret = p_dce->xofs_f;
		break;
	case VPE_REG_DCE_YOFS_I:
		ret = p_dce->yofs_i;
		break;
	case VPE_REG_DCE_YOFS_F:
		ret = p_dce->yofs_f;
		break;


	}
	return ret;
}




int vpe_drv_dce_trans_regtable_to_cmdlist(VPE_DRV_CFG *p_drv_cfg, VPE_LLCMD_DATA *p_llcmd)
{
	UINT32 i, j, mask;
	UINT32 *cmduffer;
	UINT32 tmpval = 0, regoffset = 0, regval = 0;

	cmduffer = (UINT32 *)p_llcmd->vaddr;

	for (i = VPE_REG_DCE_START; i <= VPE_REG_DCE_END; i += VPE_REG_OFFSET) {
		tmpval = 0;
		mask = 0;
		regoffset = i;
		for (j = 0; j < VPE_REG_DCE_MAXNUM; j++) {
			if (vpe_dce_reg_def[j].reg_add == i) {
				if (vpe_dce_reg_def[j].numofbits >= VPE_REG_MAXBIT) {
					mask = 0xffffffff;
				} else {
					mask = (1UL << vpe_dce_reg_def[j].numofbits) - 1;
				}

				regval = vpe_drv_dce_trans_config_to_regtable(&(p_drv_cfg->dce), j);
				tmpval |= ((UINT32)(regval & mask)) << vpe_dce_reg_def[j].startBit;
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

UINT32 vpe_drv_dce_get_actual_2dlut_size(INT32 lut2d_sz)
{
	UINT32 lut_tap_num = 257;

	switch (lut2d_sz) {
	case VPE_DCE_2DLUT_9_9_GRID:
		lut_tap_num = 9;
		break;
	case VPE_DCE_2DLUT_65_65_GRID:
		lut_tap_num = 65;
		break;
	case VPE_DCE_2DLUT_129_129_GRID:
		lut_tap_num = 129;
		break;
	case VPE_DCE_2DLUT_257_257_GRID:
		lut_tap_num = 257;
		break;
	default:
		lut_tap_num = 257;
		break;
	}

	return lut_tap_num;
}

static UINT64 kdrv_vpe_do_64b_div(UINT64 dividend, UINT64 divisor)
{
#if defined __UITRON || defined __ECOS || defined __FREERTOS
	dividend = (dividend / divisor);
#else
	do_div(dividend, divisor);
#endif
	return dividend;
}

static void vpe_drv_dce_norm_param_cal(KDRV_VPE_CONFIG *p_vpe_info)
{
	UINT64 img_w = 0, img_h = 0;
	UINT64 cen_x = 0, cen_y = 0;
	UINT64 dist_x = 0, dist_sqr_x = 0, dist_y = 0, dist_sqr_y = 0;
	UINT64 GDCi = 0, GDCtmp = 0;
	UINT64 TmpX = 0, TmpY = 0;

	if (p_vpe_info != NULL) {

		img_w = (UINT64)p_vpe_info->dce_param.dewarp_in_width;
		img_h = (UINT64)p_vpe_info->dce_param.dewarp_in_height;
		cen_x = (UINT64)p_vpe_info->dce_param.cent_x_s;
		cen_y = (UINT64)p_vpe_info->dce_param.cent_y_s;

		dist_x = kdrv_vpe_do_64b_div((((cen_x > (img_w - cen_x)) ? cen_x : (img_w - cen_x)) * 120), 100);
		dist_y = kdrv_vpe_do_64b_div((((cen_y > (img_h - cen_y)) ? cen_y : (img_h - cen_y)) * 120), 100);

		dist_sqr_x = dist_x * dist_x;
		dist_sqr_y = dist_y * dist_y;

		TmpX = (dist_sqr_x * ((UINT64)p_vpe_info->dce_param.xdist_a1 + 1)) >> 12;
		TmpY = (dist_sqr_y * ((UINT64)p_vpe_info->dce_param.ydist_a1 + 1)) >> 12;

		GDCtmp = TmpX + TmpY;

		while (((UINT64)1 << GDCi) < GDCtmp) {
			GDCi++;
		}

		p_vpe_info->dce_param.normbit = (UINT32)GDCi;
#if defined __UITRON || defined __ECOS || defined __FREERTOS
		p_vpe_info->dce_param.normfact = (UINT32)((((UINT64)1 << (GDCi + 9))) / GDCtmp);
#else
		{
			UINT64 mod, x;
			x = (((UINT64)1 << (GDCi + 9)));
			mod = do_div(x, GDCtmp);
			p_vpe_info->dce_param.normfact = (UINT32)(x);
		}
#endif
		if (p_vpe_info->dce_param.normfact > 1023) {
			p_vpe_info->dce_param.normfact = 1023;
		}

	}
}

int vpe_drv_dce_config(KDRV_VPE_CONFIG *p_vpe_info, VPE_DRV_CFG *p_drv_cfg, struct vpe_drv_lut2d_info *lut2d)
{
	INT32 i = 0;
	int rt;
	struct KDRV_VPE_ROI dewarp_in; 
	struct KDRV_VPE_ROI dewarp_out; 

	if ((p_vpe_info != NULL) && (p_drv_cfg != NULL)) {
		if (p_drv_cfg->glo_ctl.dce_en) {
			UINT32 lut_tap_num = vpe_drv_dce_get_actual_2dlut_size(p_vpe_info->dce_param.lut2d_sz);

			p_drv_cfg->dce.dce_mode = p_vpe_info->dce_param.dce_mode;
			p_drv_cfg->dce.lut2d_sz = p_vpe_info->dce_param.lut2d_sz;
			p_drv_cfg->dce.lsb_rand = p_vpe_info->dce_param.lsb_rand;
			p_drv_cfg->dce.fovbound = p_vpe_info->dce_param.fovbound;
			p_drv_cfg->dce.boundy = p_vpe_info->dce_param.boundy;
			p_drv_cfg->dce.boundu = p_vpe_info->dce_param.boundu;
			p_drv_cfg->dce.boundv = p_vpe_info->dce_param.boundv;
			p_drv_cfg->dce.cent_x_s = p_vpe_info->dce_param.cent_x_s;
			p_drv_cfg->dce.cent_y_s = p_vpe_info->dce_param.cent_y_s;
			p_drv_cfg->dce.xdist_a1 = p_vpe_info->dce_param.xdist_a1;
			p_drv_cfg->dce.ydist_a1 = p_vpe_info->dce_param.ydist_a1;

			vpe_drv_dce_norm_param_cal(p_vpe_info);
			p_drv_cfg->dce.normfact = p_vpe_info->dce_param.normfact;
			p_drv_cfg->dce.normbit = p_vpe_info->dce_param.normbit;

			p_drv_cfg->dce.fovgain = p_vpe_info->dce_param.fovgain;



			dewarp_out.x = 0;
			dewarp_out.y = 0;
			dewarp_out.w = p_vpe_info->dce_param.dewarp_in_width;
			dewarp_out.h = p_vpe_info->dce_param.dewarp_in_height;
			
			if (((p_vpe_info->dce_param.dce_2d_lut_en == 1)&& (!(p_vpe_info->func_mode & KDRV_VPE_FUNC_DCTG_EN)))||((p_vpe_info->dce_param.dce_2d_lut_en == 0)&& (p_vpe_info->func_mode & KDRV_VPE_FUNC_DCTG_EN))){
				if ((p_vpe_info->dce_param.dewarp_out_width != 0) && (p_vpe_info->dce_param.dewarp_out_height != 0)) {
					//set new dce out size
					dewarp_out.w = p_vpe_info->dce_param.dewarp_out_width;
					dewarp_out.h = p_vpe_info->dce_param.dewarp_out_height;
				}
			} else {
				if ((p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_90) ||
				   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_270) ||
				   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_90_H_FLIP) ||
				   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_270_H_FLIP)) {
					dewarp_out.w = p_vpe_info->dce_param.dewarp_in_height;
					dewarp_out.h = p_vpe_info->dce_param.dewarp_in_width;
				}
			}

			p_drv_cfg->dce.hfact = (UINT32)kdrv_vpe_do_64b_div(((UINT64)(lut_tap_num - 1) << (UINT64)VPE_2DLUT_FRAC_BIT_NUM), (UINT64)(dewarp_out.w - 1));
			p_drv_cfg->dce.vfact = (UINT32)kdrv_vpe_do_64b_div(((UINT64)(lut_tap_num - 1) << (UINT64)VPE_2DLUT_FRAC_BIT_NUM), (UINT64)(dewarp_out.h - 1));

			p_drv_cfg->dce.xofs_i = p_vpe_info->dce_param.xofs_i;
			p_drv_cfg->dce.xofs_f = p_vpe_info->dce_param.xofs_f;
			p_drv_cfg->dce.yofs_i = p_vpe_info->dce_param.yofs_i;
			p_drv_cfg->dce.yofs_f = p_vpe_info->dce_param.yofs_f;

			for (i = 0; i < 65; i++) {
				p_drv_cfg->dce_geo.geo_lut[i] = p_vpe_info->dce_param.geo_lut[i];
			}
			p_drv_cfg->dma.dce_2dlut_addr = p_vpe_info->dce_param.dce_l2d_addr;

			p_drv_cfg->glo_ctl.dce_2d_lut_en = p_vpe_info->dce_param.dce_2d_lut_en;



			if ((p_vpe_info->dce_param.rot != VPE_DRV_DCE_ROT_NONE)&&(p_vpe_info->dce_param.dce_2d_lut_en == 0)) {
				p_vpe_info->dce_param.lut2d_sz = VPE_DCE_2DLUT_9_9_GRID;

				//dce_param.lut2d_sz = DCE_2DLUT_TAB_IDX;
				dewarp_in.x = p_vpe_info->glb_img_info.src_in_x;
				dewarp_in.y = p_vpe_info->glb_img_info.src_in_y;
				dewarp_in.w = p_vpe_info->dce_param.dewarp_in_width;
				dewarp_in.h = p_vpe_info->dce_param.dewarp_in_height;

				if ((p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_90) ||
					   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_270) ||
					   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_90_H_FLIP) ||
					   (p_vpe_info->dce_param.rot == VPE_DRV_DCE_ROT_270_H_FLIP)) {

					//set new dce out size
					if(dewarp_out.w != dewarp_in.h){
						DBG_ERR("VPE: rot %d, dewarp_out_width: %d != dewarp_in_deight: %d !!\r\n", (int)p_vpe_info->dce_param.rot, (int)dewarp_out.w, (int)dewarp_in.h);
						return -1;
					}
					if(dewarp_out.h != dewarp_in.w){
						DBG_ERR("VPE: rot %d, dewarp_out_height: %d != dewarp_in_width: %d !!\r\n", (int)p_vpe_info->dce_param.rot, (int)dewarp_out.h, (int)dewarp_in.w);
						return -1;

					}
					//p_vpe_info->dce_param.dewarp_out_width = p_vpe_info->glb_img_info.src_in_h
					//p_vpe_info->dce_param.dewarp_out_height = p_vpe_info->glb_img_info.src_in_w;
				}
				//set 2d lut tab & force dce_2d_lut_en = 1
				memset(lut2d->va_addr, 0, lut2d->size);
				//DBG_ERR("lut2d %lx %lx %x\n", (unsigned long)lut2d->va_addr, (unsigned long)lut2d->pa_addr, lut2d->size);

				if (p_vpe_info->dce_param.rot != VPE_DRV_DCE_ROT_MANUAL) {
					rt = vpe_drv_dce_gen_rot_2dlut_fix(lut2d->va_addr, lut2d->size, dewarp_in, p_vpe_info->dce_param.rot);
				} else {
				    p_vpe_info->dce_param.rot_param.ratio_mode = VPE_DRV_DCE_ROT_RAT_FIX_ASPECT_RATIO_UP;
					rt = vpe_drv_dce_gen_rot_2dlut_deg(lut2d->va_addr, lut2d->size, dewarp_in, p_vpe_info->dce_param.rot_param);
				}
				if (rt < 0) {
					DBG_ERR("lut2d rot(%d) (%d %d) (%d %d %d %d) apply fail\n", p_vpe_info->dce_param.rot, dewarp_in.w, dewarp_in.h,
										p_vpe_info->dce_param.rot_param.rad, p_vpe_info->dce_param.rot_param.flip,
										p_vpe_info->dce_param.rot_param.ratio_mode, p_vpe_info->dce_param.rot_param.ratio);
					return -1;
				}
#if (COHERENT_CACHE_BUF == 1)
				vos_cpu_dcache_sync((VOS_ADDR)lut2d->va_addr, lut2d->size, VOS_DMA_TO_DEVICE);
#endif


				p_drv_cfg->dma.dce_2dlut_addr = lut2d->pa_addr;

			} 


		}

	}
	return 0;
}


void vpe_set_dce_reg(VPE_DCE_CFG *p_dce_cfg)
{
	if (p_dce_cfg != NULL) {
		vpeg->reg_48.bit.dce_mode = p_dce_cfg->dce_mode;
		vpeg->reg_48.bit.dce_lut2d_sz = p_dce_cfg->lut2d_sz;
		vpeg->reg_48.bit.dce_lsb_rand = p_dce_cfg->lsb_rand;

		vpeg->reg_49.bit.dce_fov_bound = p_dce_cfg->fovbound;
		vpeg->reg_49.bit.dce_bound_y = p_dce_cfg->boundy;
		vpeg->reg_50.bit.dce_bound_u = p_dce_cfg->boundu;
		vpeg->reg_50.bit.dce_bound_v = p_dce_cfg->boundv;

		vpeg->reg_51.bit.dce_cent_x_s = p_dce_cfg->cent_x_s;
		vpeg->reg_51.bit.dce_cent_y_s = p_dce_cfg->cent_y_s;

		vpeg->reg_52.bit.dce_xdist = p_dce_cfg->xdist_a1;
		vpeg->reg_52.bit.dce_ydist = p_dce_cfg->ydist_a1;

		vpeg->reg_53.bit.dce_norm_fact = p_dce_cfg->normfact;
		vpeg->reg_53.bit.dce_norm_bit = p_dce_cfg->normbit;
		vpeg->reg_53.bit.dce_fov_gain = p_dce_cfg->fovgain;

		vpeg->reg_54.bit.dce_hfact = p_dce_cfg->hfact;
		vpeg->reg_55.bit.dce_vfact = p_dce_cfg->vfact;

		vpeg->reg_56.bit.dce_xofs_int = p_dce_cfg->xofs_i;
		vpeg->reg_56.bit.dce_yofs_int = p_dce_cfg->yofs_i;

		vpeg->reg_57.bit.dce_xofs_frc = p_dce_cfg->xofs_f;
		vpeg->reg_58.bit.dce_yofs_frc = p_dce_cfg->yofs_f;
	}
}


void vpe_set_dce_geo_lut_reg(VPE_DRV_CFG *p_drv_cfg)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	T_DCE_GEO_REGISTER32 arg;

	if (p_drv_cfg != NULL) {
		for (i = 0; i < 32; i++) {
			reg_set_val = 0;

			reg_ofs = DCE_GEO_REGISTER0_OFS + (i << 2);

			idx = (i * 2);
			reg_set_val = (p_drv_cfg->dce_geo.geo_lut[idx]) + ((p_drv_cfg->dce_geo.geo_lut[idx + 1]) << 16);

			VPE_SETREG(reg_ofs, reg_set_val);
		}

		arg.reg = VPE_GETREG(DCE_GEO_REGISTER32_OFS);
		arg.bit.GEO_LUT_64 = p_drv_cfg->dce_geo.geo_lut[64];
		VPE_SETREG(DCE_GEO_REGISTER32_OFS, arg.reg);
	}
}



