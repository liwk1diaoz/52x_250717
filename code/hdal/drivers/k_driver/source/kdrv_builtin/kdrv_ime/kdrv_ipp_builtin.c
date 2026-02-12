#include "kdrv_builtin.h"
#include "nvtmpp_init.h"
#include "kwrap/type.h"
#include "kwrap/file.h"
#include "kwrap/cpu.h"
#include "kwrap/util.h"
#include "ime_platform.h"
#include "ime_reg.h"
#include "ime_lib.h"
#include "ime_dbg.h"
#include "kdrv_ipp_builtin.h"
#include "kdrv_ipp_builtin_int.h"
#include "sie_init.h"

#if defined (__KERNEL__)
#include <plat/nvt-sramctl.h>

#define KDRV_IPP_BUILTIN_NEW_BIND_FLOW (DISABLE)

#define IFE_FB_INT_STS_OFS 				0xC
#define IFE_FB_INT_R_DEC1_ERR_BIT 			0x2
#define IFE_FB_INT_R_DEC2_ERR_BIT 			0x4
#define IFE_FB_INT_RINGBUF_ERR_BIT 			0x100
#define IFE_FB_INT_R_FRAME_ERR_BIT 			0x200
#define IFE_FB_INT_ERR_CHK_BIT 				(IFE_FB_INT_R_DEC1_ERR_BIT | IFE_FB_INT_R_DEC2_ERR_BIT | IFE_FB_INT_RINGBUF_ERR_BIT | IFE_FB_INT_R_FRAME_ERR_BIT)

extern VOID ime_base_reg_init(VOID);
extern void nvt_bootts_add_ts(char *name);

/* isp callback register before init */
static KDRV_IPP_BUILTIN_ISP_CB g_kdrv_ipp_isp_cb = NULL;
static KDRV_IPP_BUILTIN_CTL g_kdrv_ipp_builtin_ctl;

_INLINE BOOL kdrv_ipp_builtin_dtsi_ver(void)
{
	return g_kdrv_ipp_builtin_ctl.dtsi_ver;
}

_INLINE UINT32* kdrv_ipp_builtin_get_dtsi_buf(void)
{
	return g_kdrv_ipp_builtin_ctl.p_dtsi_buf;
}

_INLINE UINT32 kdrv_ipp_builtin_get_reg(UINT32 eng, UINT32 ofs)
{
	return ioread32((void *)(g_kdrv_ipp_builtin_ctl.reg_base[eng] + ofs));
}

_INLINE void kdrv_ipp_builtin_set_reg(UINT32 eng, UINT32 ofs, UINT32 val)
{
	iowrite32(val, (void *)(g_kdrv_ipp_builtin_ctl.reg_base[eng] + ofs));
}

INT32 kdrv_ipp_builtin_get(void *p_hdl, UINT32 param_id, void *p_data)
{
	INT32 ret = 0;

	switch(param_id){


	case KDRV_IPP_PARAM_BUILTIN_GET_IPP_INFO:
	{
		UINT32 i=0;
		KDRV_IPP_BUILTIN_IPP_INFO* info = (KDRV_IPP_BUILTIN_IPP_INFO *)p_data;

		for(i=0; i < g_kdrv_ipp_builtin_ctl.hdl_num; i++){
			if(info->isp_id == g_kdrv_ipp_builtin_ctl.p_hdl[i].isp_id)
				break;
		}

		if(i < g_kdrv_ipp_builtin_ctl.hdl_num )
			info->func_en = g_kdrv_ipp_builtin_ctl.p_hdl[i].func_en;
		else{
			info->func_en = 0;
			DBG_ERR("get isp info error, isp_id:%d\n",info->isp_id);
		}

		break;
	}
	default:
		KDRV_IPP_BUILTIN_DUMP("unknown ipp builtin get case\r\n");
		break;
	}

	return ret;
}
_INLINE UINT32 kdrv_ipp_builtin_yuv_size(KDRV_IPP_BUILTIN_IMG_INFO *path_info)
{
	UINT32 size = 0;

	if (path_info->fmt == KDRV_IPP_BUILTIN_FMT_YUV420 ||
		path_info->fmt == KDRV_IPP_BUILTIN_FMT_YUV420_PLANAR ||
		path_info->fmt == KDRV_IPP_BUILTIN_FMT_NVX2) {
		size = path_info->size.h * path_info->loff[0];
		size += (path_info->size.h * path_info->loff[1]) / 2;
	} else if (path_info->fmt == KDRV_IPP_BUILTIN_FMT_Y8) {
		size = path_info->size.h * path_info->loff[0];
	}

	return size;
}

_INLINE UINT32 kdrv_ipp_builtin_y_size(KDRV_IPP_BUILTIN_IMG_INFO *path_info)
{
	return path_info->size.h * path_info->loff[0];
}

_INLINE UINT32 kdrv_ipp_builtin_uv_size(KDRV_IPP_BUILTIN_IMG_INFO *path_info)
{
	if (path_info->fmt == KDRV_IPP_BUILTIN_FMT_YUV420 ||
		path_info->fmt == KDRV_IPP_BUILTIN_FMT_NVX2) {
		return (path_info->size.h / 2) * path_info->loff[1];
	} else if (path_info->fmt == KDRV_IPP_BUILTIN_FMT_YUV420_PLANAR) {
		return (path_info->size.h / 2) * (path_info->loff[1] / 2);
	}

	return 0;
}

_INLINE KDRV_IPP_BUILTIN_FMT kdrv_ipp_builtin_typecast_fmt(KDRV_IPP_BUILTIN_PATH_ID pid, IME_GET_OUTPATH_INFO *path_info)
{
	if (pid < 4) {
		if (pid == 0) {
			/* IME_PE_ENC_EN, bit26 */
			if (kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IME, 0x04) & (0x4000000)) {
				return KDRV_IPP_BUILTIN_FMT_NVX2;
			}
		}
		if (path_info->out_path_image_format.out_format_sel == IME_OUTPUT_YCC_420) {
			if (path_info->out_path_image_format.out_format_type_sel == IME_OUTPUT_YCC_PLANAR) {
				return KDRV_IPP_BUILTIN_FMT_YUV420_PLANAR;
			} else {
				return KDRV_IPP_BUILTIN_FMT_YUV420;
			}
		} else {
			return KDRV_IPP_BUILTIN_FMT_Y8;
		}
	} else if (pid == 4) {
		/* IME_3DNR_REG_OUT_ENC_EN, bit31 */
		if (kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IME, 0x04) & (0x80000000)) {
			return KDRV_IPP_BUILTIN_FMT_NVX2;
		} else {
			return KDRV_IPP_BUILTIN_FMT_YUV420;
		}
	} else if (pid == 5) {
		return KDRV_IPP_BUILTIN_FMT_Y8;
	}

	return 0;
}

static UINT32 kdrv_ipp_builtin_get_ime_out_flip(UINT32 pid)
{
	UINT32 reg_val;

	reg_val = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IME, 0x08);

	if (pid == KDRV_IPP_BUILTIN_PATH_ID_5) {
		return reg_val & 20000;
	}

	/* path 1~4 */
	return (reg_val & (0x800 << pid));
}

static void kdrv_ipp_builtin_set_ime_out_addr(UINT32 pid, KDRV_IPP_BUILTIN_IMG_INFO *p_info)
{
	UINT32 eng = KDRV_IPP_BUILTIN_IME;
	UINT32 addr[3] = {0};

	addr[0] = p_info->phyaddr[0];
	addr[1] = p_info->phyaddr[1];
	addr[2] = p_info->phyaddr[2];

	if (kdrv_ipp_builtin_get_ime_out_flip(pid)) {
		addr[0] = addr[0] + (kdrv_ipp_builtin_y_size(p_info) - p_info->loff[0]);
		addr[1] = addr[1] + (kdrv_ipp_builtin_uv_size(p_info) - p_info->loff[1]);
		addr[2] = addr[2] + (kdrv_ipp_builtin_uv_size(p_info) - p_info->loff[1]);
	}

	switch (pid) {
	case 0:
		kdrv_ipp_builtin_set_reg(eng, 0x94, addr[0]);
		kdrv_ipp_builtin_set_reg(eng, 0x98, addr[1]);
		kdrv_ipp_builtin_set_reg(eng, 0x9C, addr[2]);
		break;

	case 1:
		kdrv_ipp_builtin_set_reg(eng, 0x11C, addr[0]);
		kdrv_ipp_builtin_set_reg(eng, 0x120, addr[1]);
		break;

	case 2:
		kdrv_ipp_builtin_set_reg(eng, 0x19C, addr[0]);
		kdrv_ipp_builtin_set_reg(eng, 0x1A0, addr[1]);
		break;

	case 3:
		kdrv_ipp_builtin_set_reg(eng, 0x1D8, addr[0]);
		break;

	case 4:
		kdrv_ipp_builtin_set_reg(eng, 0x818, addr[0]);
		kdrv_ipp_builtin_set_reg(eng, 0x81C, addr[1]);
		break;

	default:
		break;
	}
}

void kdrv_ipp_builtin_update_timestamp(void)
{
	/* for direct mode hdal first frame start timestamp */
	KDRV_IPP_BUILTIN_CTL *p_ctl = &g_kdrv_ipp_builtin_ctl;

	if (p_ctl->p_trig_hdl) {
		vos_perf_mark(&p_ctl->p_trig_hdl->fs_timestamp);
	}
}

void kdrv_ipp_builtin_frc(KDRV_IPP_BUILTIN_HDL *p_hdl, KDRV_IPP_BUILTIN_FMD_CB_INFO *p_info)
{
	UINT32 i;
	KDRV_IPP_BUILTIN_IMG_INFO *p_path;
	KDRV_IPP_BUILTIN_RATE_CTL *p_frc;

	for (i = 0; i < KDRV_IPP_BUILTIN_PATH_ID_MAX; i++) {
		p_path = &p_info->out_img[i];
		p_frc = &p_hdl->frc[i];

		if (!p_path->enable) {
			continue;
		}

		if (p_hdl->fs_cnt <= p_frc->skip) {
			/* skip frame */
			p_path->enable = DISABLE;
		} else {
			/* frame rate ctrl */
			if (p_frc->src != p_frc->dst) {
				if ((p_hdl->fs_cnt - p_frc->skip) > p_frc->src &&
					((p_hdl->fs_cnt - p_frc->skip) % p_frc->src) == 0) {
					p_frc->rate_cnt = 0;
				}

				p_frc->rate_cnt += KDRV_IPP_BUILTIN_FRC_BASE;
				if (p_frc->rate_cnt >= p_frc->drop_rate) {
					p_path->enable = DISABLE;
					p_frc->rate_cnt -= p_frc->drop_rate;
				}
			} else if (p_frc->dst == 0) {
				p_path->enable = DISABLE;
			}
		}
	}
}

static INT32 kdrv_ipp_builtin_cal_privacy_mask_line(IPOINT a, IPOINT b, IPOINT cent, INT32 *p_coefs)
{
	INT32 check = 0;

	// 0: >=,  1: <=,  2: >,  3: <
	p_coefs[0] = (b.y - a.y);
	p_coefs[1] = (a.x - b.x);
	p_coefs[2] = (a.x * b.y - b.x * a.y);

	check = (p_coefs[0] * cent.x) + (p_coefs[1] * cent.y);
	if (check >= p_coefs[2]) {
		p_coefs[3] = 0;
	} else if (check <= p_coefs[2]) {
		p_coefs[3] = 1;
	}

	return 0;
}

static INT32 kdrv_ipp_builtin_cfg_privacy_mask_coefs(UINT32 idx, IPOINT *p_coord)
{
	KDRV_IPP_BUILTIN_PRI_MASK_REG_LINE pm_reg_line;
	IPOINT center;
	INT32 line[4][4];
	UINT32 j;
	UINT32 reg_base;

	center.x = (p_coord[0].x + p_coord[1].x + p_coord[2].x + p_coord[3].x) >> 2;
	center.y = (p_coord[0].y + p_coord[1].y + p_coord[2].y + p_coord[3].y) >> 2;

	kdrv_ipp_builtin_cal_privacy_mask_line(p_coord[0], p_coord[3], center, &line[0][0]);
	kdrv_ipp_builtin_cal_privacy_mask_line(p_coord[1], p_coord[0], center, &line[1][0]);
	kdrv_ipp_builtin_cal_privacy_mask_line(p_coord[2], p_coord[3], center, &line[2][0]);
	kdrv_ipp_builtin_cal_privacy_mask_line(p_coord[1], p_coord[2], center, &line[3][0]);

	reg_base = (idx < 4) ? (KDRV_IPP_BUILTIN_PRI_MASK_REG_BASE03 + ((idx % 4) * KDRV_IPP_BUILTIN_PRI_MASK_REG_OFS) + 0x4) :
							(KDRV_IPP_BUILTIN_PRI_MASK_REG_BASE47 + ((idx % 4) * KDRV_IPP_BUILTIN_PRI_MASK_REG_OFS) + 0x4);
	for (j = 0; j < 4; j++) {
		pm_reg_line.val = 0;
		pm_reg_line.bit_set0.comp = line[j][3];
		pm_reg_line.bit_set0.coefa = ((line[j][0] < 0) ? (-1 * line[j][0]) : line[j][0]);
		pm_reg_line.bit_set0.signa = ((line[j][0] < 0) ? 1 : 0);
		pm_reg_line.bit_set0.coefb = ((line[j][1] < 0) ? (-1 * line[j][1]) : line[j][1]);
		pm_reg_line.bit_set0.signb = ((line[j][1] < 0) ? 1 : 0);
		kdrv_ipp_builtin_set_reg(KDRV_IPP_BUILTIN_IME, reg_base + (j * 0x8), pm_reg_line.val);

		pm_reg_line.val = 0;
		pm_reg_line.bit_set1.coefc = ((line[j][2] < 0) ? (-1 * line[j][2]) : line[j][2]);
		pm_reg_line.bit_set1.signc = ((line[j][2] < 0) ? 1 : 0);
		kdrv_ipp_builtin_set_reg(KDRV_IPP_BUILTIN_IME, reg_base + (j * 0x8) + 0x4, pm_reg_line.val);
	}

	return 0;
}

static INT32 kdrv_ipp_builtin_cfg_privacy_mask(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	KDRV_IPP_BUILTIN_PRI_MASK_REG_CTRL pm_reg_ctl;
	UINT32 i, tmp;

	/* config register base on mask */
	for (i = 0; i < KDRV_IPP_BUILTIN_PRI_MASK_NUM; i++) {
		tmp = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IME, 0x8);
		if (p_hdl->mask[i].enable) {
			tmp |= (1 << i);
			kdrv_ipp_builtin_set_reg(KDRV_IPP_BUILTIN_IME, 0x8, tmp);

			/* ctrl */
			tmp = (i < 4) ? (KDRV_IPP_BUILTIN_PRI_MASK_REG_BASE03 + ((i % 4) * KDRV_IPP_BUILTIN_PRI_MASK_REG_OFS)) :
							(KDRV_IPP_BUILTIN_PRI_MASK_REG_BASE47 + ((i % 4) * KDRV_IPP_BUILTIN_PRI_MASK_REG_OFS));
			pm_reg_ctl.val = 0;
			pm_reg_ctl.bit.type = 0;
			pm_reg_ctl.bit.color_y = p_hdl->mask[i].color[0];
			pm_reg_ctl.bit.color_u = p_hdl->mask[i].color[1];
			pm_reg_ctl.bit.color_v = p_hdl->mask[i].color[2];
			pm_reg_ctl.bit.hlw_en = p_hdl->mask[i].hlw_enable;
			kdrv_ipp_builtin_set_reg(KDRV_IPP_BUILTIN_IME, tmp, pm_reg_ctl.val);

			/* alpha */
			tmp = (i < 4) ? 0x4B0 : 0x544;
			pm_reg_ctl.val = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IME, tmp);
			pm_reg_ctl.val &= ~(0xff << (8 * (i % 4)));
			pm_reg_ctl.val |= p_hdl->mask[i].weight << (8 * (i % 4));
			kdrv_ipp_builtin_set_reg(KDRV_IPP_BUILTIN_IME, tmp, pm_reg_ctl.val);

			/* line coefs */
			kdrv_ipp_builtin_cfg_privacy_mask_coefs(i, p_hdl->mask[i].coord);
			if (p_hdl->mask[i].hlw_enable) {
				kdrv_ipp_builtin_cfg_privacy_mask_coefs(i + 1, p_hdl->mask[i].coord2);
			}
		} else {
			tmp &= ~(1 << i);
			kdrv_ipp_builtin_set_reg(KDRV_IPP_BUILTIN_IME, 0x8, tmp);
		}
	}

	return 0;
}

INT32 kdrv_ipp_builtin_cfg_ife(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	UINT32 eng = KDRV_IPP_BUILTIN_IFE;
	UINT32 eng_ctrl;
	KDRV_IPP_BUILTIN_JOB *p_job;

	if (p_hdl->fs_cnt == 0) {
		/* shdr enable, set rinbuf addr for direct mode at first frame */
		if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW &&
			p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_SHDR) {
			kdrv_ipp_builtin_set_reg(eng, 0x38, p_hdl->vprc_shdr_blk.phy_addr);
		}

		/* clear all interrupt */
		kdrv_ipp_builtin_set_reg(eng, IFE_FB_INT_STS_OFS, 0xffffffff);
	}

	if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
		/* frmstart load */
		eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
		eng_ctrl |= 0x10;
		kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
	} else {
		/* d2d mode update input address from p_hdl->p_cur_job */
		if (p_hdl->p_cur_job) {
			p_job = (KDRV_IPP_BUILTIN_JOB *)p_hdl->p_cur_job;
			kdrv_ipp_builtin_set_reg(eng, 0x30, nvtmpp_sys_va2pa(p_job->va[0]));
			if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_SHDR) {
				kdrv_ipp_builtin_set_reg(eng, 0x38, nvtmpp_sys_va2pa(p_job->va[1]));
			}
		}

		/* eng start load */
		eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
		eng_ctrl |= 0x4;
		kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
	}

	return 0;
}

INT32 kdrv_ipp_builtin_cfg_dce(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	UINT32 eng = KDRV_IPP_BUILTIN_DCE;
	UINT32 eng_ctrl;
	UINT32 func_ctrl;
	UINT32 single_out_bit;
	UINT32 buf_idx;
	KDRV_IPP_BUILTIN_JOB *p_job = NULL;

	buf_idx = p_hdl->fs_cnt % 2;
	single_out_bit = 0x10;
	func_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x04);

	/* disable 2dlut for first frame */
	if (p_hdl->fs_cnt == 0) {
		func_ctrl &= ~(0x80000);
	}

	/* wdr */
	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_WDR) {
		if (p_hdl->pri_buf.wdr[0].size > 0) {
			if (p_hdl->fs_cnt == 0) {
				/* disable wdr for first frame */
				func_ctrl &= ~(0x200);

				/* prevent iq misenable, set subin addr to un-used private buffer */
				kdrv_ipp_builtin_set_reg(eng, 0x124, p_hdl->pri_buf.wdr[1].phy_addr);
			} else {
				/* update reference buffer */
				kdrv_ipp_builtin_set_reg(eng, 0x124, p_hdl->last_ctl_info.wdr_addr);
			}

			/* wdr subout */
			kdrv_ipp_builtin_set_reg(eng, 0x12C, p_hdl->pri_buf.wdr[buf_idx].phy_addr);
			func_ctrl |= 0x400;
			single_out_bit |= 0x1;
			p_hdl->last_ctl_info.wdr_addr = p_hdl->pri_buf.wdr[buf_idx].phy_addr;
		}
	}

	/* single out, func ctrl */
	kdrv_ipp_builtin_set_reg(eng, 0x04, func_ctrl);
	kdrv_ipp_builtin_set_reg(eng, 0x18, single_out_bit);

	if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
		/* frmstart load */
		eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
		eng_ctrl |= 0x10;
		kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
	} else  if(p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_CCIR){

		/* ccir d2d mode update input address from p_hdl->p_cur_job */
		if (p_hdl->p_cur_job) {
			p_job = (KDRV_IPP_BUILTIN_JOB *)p_hdl->p_cur_job;
			kdrv_ipp_builtin_set_reg(eng, 0x20, nvtmpp_sys_va2pa(p_job->va[0]));
			kdrv_ipp_builtin_set_reg(eng, 0x28, nvtmpp_sys_va2pa(p_job->va[1]));
		}

		/* eng start load */
		eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
		eng_ctrl |= 0x4;
		kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
	} else {

		/* eng start load */
		eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
		eng_ctrl |= 0x4;
		kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
	}

	return 0;
}

INT32 kdrv_ipp_builtin_cfg_ipe(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	UINT32 eng = KDRV_IPP_BUILTIN_IPE;
	UINT32 eng_ctrl;
	UINT32 func_ctrl;
	UINT32 single_out_bit;
	UINT32 buf_idx;

	buf_idx = p_hdl->fs_cnt % 2;
	single_out_bit = 0x10;
	func_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x08);

	/* disable gamma, yout, wait iq flow enable
		direct mode, disable for first frame
		d2d mode, always disable and wait iq config
	*/
	if ((p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW && p_hdl->fs_cnt == 0) ||
		(p_hdl->flow != KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW)) {
		func_ctrl &= ~(0x6);
	}

	/* enable defog subout, disable defog, lce func */
	if (p_hdl->pri_buf.defog[0].size > 0) {
		if (p_hdl->fs_cnt == 0) {
			/* disable defog, lce for first frame */
			func_ctrl &= ~(0x60);

			/* prevent iq misenable, set subin addr to un-used private buffer */
			kdrv_ipp_builtin_set_reg(eng, 0x2C4, p_hdl->pri_buf.defog[1].phy_addr);
		} else {
			/* update reference buffer */
			kdrv_ipp_builtin_set_reg(eng, 0x2C4, p_hdl->last_ctl_info.defog_addr);
		}

		/* defog subout */
		kdrv_ipp_builtin_set_reg(eng, 0x2CC, p_hdl->pri_buf.defog[buf_idx].phy_addr);
		single_out_bit |= 0x8;
		func_ctrl |= 0x10;
		p_hdl->last_ctl_info.defog_addr = p_hdl->pri_buf.defog[buf_idx].phy_addr;
	}

	/* single out, func ctrl */
	kdrv_ipp_builtin_set_reg(eng, 0x08, func_ctrl);
	kdrv_ipp_builtin_set_reg(eng, 0x0C, single_out_bit);

	if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
		/* frmstart load */
		eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
		eng_ctrl |= 0x10;
		kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
	} else {
		/* eng start load */
		eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
		eng_ctrl |= 0x4;
		kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
	}

	return 0;
}

INT32 kdrv_ipp_builtin_cfg_ime(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	UINT32 eng = KDRV_IPP_BUILTIN_IME;
	UINT32 eng_ctrl;
	UINT32 func_ctrl;
	UINT32 single_out_bit;
	UINT32 buf_idx;
	UINT32 i;
	KDRV_IPP_BUILTIN_IMG_INFO *p_path_info;

	buf_idx = p_hdl->fs_cnt % 2;
	single_out_bit = 0x80000000;
	func_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x04);

	/* set lca output address, enable bypass */
	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_YUV_SUBOUT) {
		UINT32 lca_ctrl;

		if (p_hdl->fs_cnt == 0) {
			/* enable bypass for first frame */
			lca_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x230);
			lca_ctrl |= 0x8;
			kdrv_ipp_builtin_set_reg(eng, 0x230, lca_ctrl);
		} else {
			/* disable bypass */
			lca_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x230);
			if (lca_ctrl & 0x8) {
				lca_ctrl &= ~(0x8);
				kdrv_ipp_builtin_set_reg(eng, 0x230, lca_ctrl);
			}
		}

		/* lca output */
		if (p_hdl->pri_buf.lca[0].size > 0) {
			kdrv_ipp_builtin_set_reg(eng, 0x26C, p_hdl->pri_buf.lca[buf_idx].phy_addr);
			single_out_bit |= 0x1;
			func_ctrl |= 0x2000000;

			p_hdl->last_ctl_info.lca_addr = p_hdl->pri_buf.lca[buf_idx].phy_addr;
		}

		/* lca cent uv */
		kdrv_ipp_builtin_set_reg(eng, 0x234, (p_hdl->last_ctl_info.gray_avg_u | (p_hdl->last_ctl_info.gray_avg_v << 8)));
	}

	/* 3dnr ctrl, no reference image, disable 3dnr at first frame */
	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_3DNR) {
		if (p_hdl->fs_cnt == 0) {
			/* disable 3dnr for first frame */
			func_ctrl &= ~(0x8000000);

			/* prevent iq misenable, set subin addr to un-used private buffer */
			/* ref yuv */
			kdrv_ipp_builtin_set_reg(eng, 0x808, p_hdl->vprc_blk[p_hdl->_3dnr_ref_path][1].phy_addr);
			kdrv_ipp_builtin_set_reg(eng, 0x80C, p_hdl->vprc_blk[p_hdl->_3dnr_ref_path][1].phy_addr);

			/* ref mv, ms */
			kdrv_ipp_builtin_set_reg(eng, 0x824, p_hdl->pri_buf._3dnr_ms[1].phy_addr);
			kdrv_ipp_builtin_set_reg(eng, 0x83C, p_hdl->pri_buf._3dnr_mv[1].phy_addr);
		} else {
			/* update reference buffer */
			/* ref yuv */
			kdrv_ipp_builtin_set_reg(eng, 0x808, p_hdl->last_ctl_info.path_addr_y[p_hdl->_3dnr_ref_path]);
			kdrv_ipp_builtin_set_reg(eng, 0x80C, p_hdl->last_ctl_info.path_addr_u[p_hdl->_3dnr_ref_path]);

			/* ref mv, ms */
			kdrv_ipp_builtin_set_reg(eng, 0x824, p_hdl->last_ctl_info._3dnr_ms_addr);
			kdrv_ipp_builtin_set_reg(eng, 0x83C, p_hdl->last_ctl_info._3dnr_mv_addr);
		}

		/* 3dnr subout */
		if ((p_hdl->pri_buf._3dnr_ms[0].size > 0) &&
			(p_hdl->pri_buf._3dnr_mv[0].size > 0)) {
			kdrv_ipp_builtin_set_reg(eng, 0x82C, p_hdl->pri_buf._3dnr_ms[buf_idx].phy_addr);
			kdrv_ipp_builtin_set_reg(eng, 0x844, p_hdl->pri_buf._3dnr_mv[buf_idx].phy_addr);
			single_out_bit |= 0x14;

			p_hdl->last_ctl_info._3dnr_ms_addr = p_hdl->pri_buf._3dnr_ms[buf_idx].phy_addr;
			p_hdl->last_ctl_info._3dnr_mv_addr = p_hdl->pri_buf._3dnr_mv[buf_idx].phy_addr;
		}
	}

	/* path 1~5 */
	for (i = 0; i <= KDRV_IPP_BUILTIN_PATH_ID_5; i++) {
		p_path_info = &p_hdl->path_info[i];
		if (p_path_info->enable) {
			if (p_hdl->vprc_blk[i][buf_idx].phy_addr) {
				p_path_info->phyaddr[0] = p_hdl->vprc_blk[i][buf_idx].phy_addr;
				p_path_info->phyaddr[1] = p_path_info->phyaddr[0] + kdrv_ipp_builtin_y_size(p_path_info);
				p_path_info->phyaddr[2] = p_path_info->phyaddr[1] + kdrv_ipp_builtin_uv_size(p_path_info);

				kdrv_ipp_builtin_set_ime_out_addr(i, p_path_info);
				if (i == KDRV_IPP_BUILTIN_PATH_ID_5) {
					/* path 5(ref out) */
					func_ctrl |= 0x40000000;
					single_out_bit |= 0x2;
				} else {
					/* path1~4 */
					func_ctrl |= (0x4 << i);
					single_out_bit |= (0x40 << i);
				}
				p_hdl->last_ctl_info.path_addr_y[i] = p_path_info->phyaddr[0];
				p_hdl->last_ctl_info.path_addr_u[i] = p_path_info->phyaddr[1];
				p_hdl->last_ctl_info.path_addr_v[i] = p_path_info->phyaddr[2];
			}
		}
	}

	/* privacy mask */
	kdrv_ipp_builtin_cfg_privacy_mask(p_hdl);

	/* single out, func ctrl */
	kdrv_ipp_builtin_set_reg(eng, 0x04, func_ctrl);
	kdrv_ipp_builtin_set_reg(eng, 0x0C, single_out_bit);

	if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
		/* frmstart load */
		eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
		eng_ctrl |= 0x10;
		kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
	} else {
		/* eng start load */
		eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
		eng_ctrl |= 0x4;
		kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
	}

	return 0;
}

INT32 kdrv_ipp_builtin_cfg_ife2(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	UINT32 eng = KDRV_IPP_BUILTIN_IFE2;
	UINT32 eng_ctrl;
	UINT32 buf_addr;

	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_YUV_SUBOUT) {
		/* update input address
			ime lca last_phy_info update before ife2 config
		*/
		if (p_hdl->last_ctl_info.lca_addr == p_hdl->pri_buf.lca[0].phy_addr) {
			buf_addr = p_hdl->pri_buf.lca[1].phy_addr;
		} else {
			buf_addr = p_hdl->pri_buf.lca[0].phy_addr;
		}
		kdrv_ipp_builtin_set_reg(eng, 0x20, buf_addr);

		/* load bit base on flow */
		if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
			/* frmstart & eng start load */
			eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
			eng_ctrl |= 0x14;
			kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
		} else {
			/* eng start load */
			eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0x0);
			eng_ctrl |= 0x4;
			kdrv_ipp_builtin_set_reg(eng, 0x0, eng_ctrl);
		}
	}

	return 0;
}

UINT32 kdrv_ipp_builtin_get_cfg_eng_bit(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	UINT32 cfg_eng_bit;

	switch (p_hdl->flow) {
	case KDRV_IPP_BUILTIN_FLOW_RAW:
	case KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW:
		if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_YUV_SUBOUT) {
			cfg_eng_bit = (KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IFE) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_DCE) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IPE) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IME) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IFE2));
		} else {
			cfg_eng_bit = (KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IFE) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_DCE) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IPE) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IME));
		}
		break;

	case KDRV_IPP_BUILTIN_FLOW_CCIR:
		if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_YUV_SUBOUT) {
			cfg_eng_bit = (KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_DCE) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IPE) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IME) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IFE2));
		} else {
			cfg_eng_bit = (	KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_DCE) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IPE) |
							KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IME));
		}
		break;

	case KDRV_IPP_BUILTIN_FLOW_IME_D2D:
		cfg_eng_bit = KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IME);
		break;

	default:
		DBG_ERR("unsupport flow %d\r\n", (int)p_hdl->flow);
		return 0;
	}

	return cfg_eng_bit;
}

INT32 kdrv_ipp_builtin_cfg_eng_all(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	CHAR *dtsi_reg_num_tag[KDRV_IPP_BUILTIN_ENG_MAX] = {
		"ife_reg_num",
		"dce_reg_num",
		"ipe_reg_num",
		"ime_reg_num",
		"ife2_reg_num",
	};
	CHAR *dtsi_reg_cfg_tag[KDRV_IPP_BUILTIN_ENG_MAX] = {
		"ife_reg_cfg",
		"dce_reg_cfg",
		"ipe_reg_cfg",
		"ime_reg_cfg",
		"ife2_reg_cfg",
	};
	CHAR regnode[64];
	UINT32 cfg_eng_bit;
	UINT32 eng;
	UINT32 reg_cfg_num = 0;
	UINT32 *p_reg_buf;
	UINT32 ofs;
	UINT32 val;
	UINT32 i;

	cfg_eng_bit = kdrv_ipp_builtin_get_cfg_eng_bit(p_hdl);
	if (cfg_eng_bit == 0) {
		return -1;
	}

	/* config from dtsi
		1. direct mode start
		2. dram mode
	*/
	if ((p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW && p_hdl->fs_cnt == 0) ||
		(p_hdl->flow != KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW)) {
		snprintf(regnode, 64, "%s/register", p_hdl->node);
		p_reg_buf = kdrv_ipp_builtin_get_dtsi_buf();
		if (p_reg_buf == NULL) {
			DBG_ERR("no buffer to read dtsi\r\n");
			return -1;
		}

		for (eng = KDRV_IPP_BUILTIN_IFE; eng < KDRV_IPP_BUILTIN_ENG_MAX; eng++) {
			if ((cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(eng)) == 0) {
				continue;
			}

			if (kdrv_ipp_builtin_plat_read_dtsi_array(regnode, dtsi_reg_num_tag[eng], &reg_cfg_num, 1)) {
				DBG_ERR("read dtsi %s failed\r\n", dtsi_reg_num_tag[eng]);
				return -1;
			}
			if (kdrv_ipp_builtin_plat_read_dtsi_array(regnode, dtsi_reg_cfg_tag[eng], p_reg_buf, (reg_cfg_num * 2))) {
				DBG_ERR("read dtsi %s failed\r\n", dtsi_reg_cfg_tag[eng]);
				return -1;
			}

			for (i = 0; i < reg_cfg_num; i++) {
				ofs = p_reg_buf[i * 2];
				val = p_reg_buf[(i * 2) + 1];

				/* skip engine ctrl regiser, set it at last */
				if (ofs == 0) {
					continue;
				} else {
					/* write config to engine */
					kdrv_ipp_builtin_set_reg(eng, ofs, val);
				}
			}
		}
	}

	/* config engine */
	if (cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IFE)) {
		kdrv_ipp_builtin_cfg_ife(p_hdl);
	}
	if (cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_DCE)) {
		kdrv_ipp_builtin_cfg_dce(p_hdl);
	}
	if (cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IPE)) {
		kdrv_ipp_builtin_cfg_ipe(p_hdl);
	}
	if (cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IME)) {
		kdrv_ipp_builtin_cfg_ime(p_hdl);
	}
	if (cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(KDRV_IPP_BUILTIN_IFE2)) {
		kdrv_ipp_builtin_cfg_ife2(p_hdl);
	}

	/* isp reset event for frm 0(both direct and drame mode)
		trig event for both direct and drame mode
	*/
	if (g_kdrv_ipp_isp_cb) {
		if (p_hdl->fs_cnt == 0) {
			g_kdrv_ipp_isp_cb(p_hdl->isp_id, KDRV_IPP_BUILTIN_ISP_EVENT_RESET);
		} else{
			g_kdrv_ipp_isp_cb(p_hdl->isp_id, KDRV_IPP_BUILTIN_ISP_EVENT_TRIG);
		}
	}

	return 0;
}

INT32 kdrv_ipp_builtin_trig_eng_start(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	KDRV_IPP_BUILTIN_CTL *p_ctl;
	UINT32 cfg_eng_bit;
	UINT32 eng;
	UINT32 val;

	p_ctl = &g_kdrv_ipp_builtin_ctl;
	if (p_ctl->p_trig_hdl != NULL) {
		DBG_ERR("already triggered for hdl 0x%.8x\r\n", (unsigned int)p_ctl->p_trig_hdl);
		return -1;
	}
	p_ctl->p_trig_hdl = p_hdl;

	/* start engine */
	cfg_eng_bit = kdrv_ipp_builtin_get_cfg_eng_bit(p_hdl);

	eng = KDRV_IPP_BUILTIN_IFE2;
	if (cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(eng)) {
		val = kdrv_ipp_builtin_get_reg(eng, 0) | 0x2;
		kdrv_ipp_builtin_set_reg(eng, 0, val);
	}

	eng = KDRV_IPP_BUILTIN_IFE;
	if (cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(eng)) {
		val = kdrv_ipp_builtin_get_reg(eng, 0) | 0x2;
		kdrv_ipp_builtin_set_reg(eng, 0, val);
	}

	eng = KDRV_IPP_BUILTIN_IPE;
	if (cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(eng)) {
		val = kdrv_ipp_builtin_get_reg(eng, 0) | 0x2;
		kdrv_ipp_builtin_set_reg(eng, 0, val);
	}

	eng = KDRV_IPP_BUILTIN_IME;
	if (cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(eng)) {
		val = kdrv_ipp_builtin_get_reg(eng, 0) | 0x2;
		kdrv_ipp_builtin_set_reg(eng, 0, val);
	}

	eng = KDRV_IPP_BUILTIN_DCE;
	if (cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(eng)) {
		val = kdrv_ipp_builtin_get_reg(eng, 0) | 0x2;
		kdrv_ipp_builtin_set_reg(eng, 0, val);
	}

	return 0;
}

INT32 kdrv_ipp_builtin_soft_reset_eng(void)
{
	UINT32 eng_ctrl;
	UINT32 eng;

	for (eng = KDRV_IPP_BUILTIN_IFE; eng < KDRV_IPP_BUILTIN_ENG_MAX; eng++) {
		eng_ctrl = kdrv_ipp_builtin_get_reg(eng, 0);
		eng_ctrl |= 0x1;
		kdrv_ipp_builtin_set_reg(eng, 0, eng_ctrl);
		eng_ctrl &= ~(0x1);
		kdrv_ipp_builtin_set_reg(eng, 0, eng_ctrl);
	}

	return 0;
}

BOOL kdrv_ipp_builtin_check_src_valid(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	KDRV_IPP_BUILTIN_CTL *p_ctl = &g_kdrv_ipp_builtin_ctl;

	if (p_hdl->src_sie_id_bit == 0) {
		return FALSE;
	}

	/* all src id must be valid */
	if ((p_hdl->src_sie_id_bit & p_ctl->valid_src_id_bit) == p_hdl->src_sie_id_bit) {
		return TRUE;
	}

	return FALSE;
}

void kdrv_ipp_builtin_upd_isp_info(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	UINT32 i, tmp;

	/* dce histogram result */
	tmp = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_DCE, 0x4);
	p_hdl->isp_info.hist_rst.enable = (tmp & 0x4000) >> 14;
	if (p_hdl->isp_info.hist_rst.enable) {
		p_hdl->isp_info.hist_rst.sel = (tmp & 0x8000) >> 15;
		for (i = 0; i < 64; i++) {
			tmp = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_DCE, (0x2d4 + (i << 2)));
			p_hdl->isp_info.hist_rst.stcs[i * 2] = tmp & (0xffff);
			p_hdl->isp_info.hist_rst.stcs[(i * 2) + 1] = (tmp >> 16) & (0xffff);
		}
	}

	/* edge statistic */
	tmp = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IPE, 0xdc);
	p_hdl->isp_info.edge_stcs.localmax_max = tmp & 0x3ff;
	p_hdl->isp_info.edge_stcs.coneng_max = (tmp >> 10) & 0x3ff;
	p_hdl->isp_info.edge_stcs.coneng_avg = (tmp >> 20) & 0x3ff;

	/* defog airlight */
	tmp = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IPE, 0x340);
	p_hdl->isp_info.defog_stcs.airlight[0] = tmp & 0x3ff;
	p_hdl->isp_info.defog_stcs.airlight[1] = (tmp >> 16) & 0x3ff;
	tmp = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IPE, 0x344);
	p_hdl->isp_info.defog_stcs.airlight[2] = tmp & 0x3ff;

	/* defog subout addr */
	p_hdl->isp_info.defog_subout_addr = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IPE, 0x2CC);
}

void kdrv_ipp_builtin_upd_lca_gray_avg(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	UINT32 sum = 0;
	UINT32 cnt = 0;

	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_YUV_SUBOUT) {
		cnt = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IFE2, 0x98) & 0xFFFFFFF;
		if (cnt) {
			sum = (kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IFE2, 0x84) & 0x7ffff) +
					((kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IFE2, 0x88) & 0x3ffff) << 19);
			p_hdl->last_ctl_info.gray_avg_u = (sum + (cnt >> 1)) / cnt;

			sum = (kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IFE2, 0x90) & 0x7ffff) +
					((kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IFE2, 0x94) & 0x3ffff) << 19);
			p_hdl->last_ctl_info.gray_avg_v = (sum + (cnt >> 1)) / cnt;
		}
	}
}

#if 0
#endif

void kdrv_ipp_builtin_ime_isr(UINT32 sts)
{
	static BOOL ime_1st_frame_end_skip = FALSE;
	KDRV_IPP_BUILTIN_CTL *p_ctl;
	KDRV_IPP_BUILTIN_HDL *p_hdl;
	KDRV_IPP_BUILTIN_JOB *p_job;
	KDRV_IPP_BUILTIN_FMD_CB_INFO info = {0};
	UINT32 i;

	p_ctl = &g_kdrv_ipp_builtin_ctl;
	p_hdl = p_ctl->p_trig_hdl;
	if (p_hdl == NULL) {
		DBG_ERR("null handle in ime isr\r\n");
		return ;
	}

	if (sts & IME_INTS_FB_FRM_END) {
		p_hdl->out_buf_release_cnt++;
	}

	if (sts & IME_INTE_FRM_END) {
		kdrv_ipp_builtin_debug_set_ts(KDRV_IPP_BUILTIN_DBG_TS_FMD, &p_hdl->dbg);
		p_hdl->fmd_cnt++;

		if(g_kdrv_ipp_isp_cb)
			g_kdrv_ipp_isp_cb(p_hdl->isp_id, KDRV_IPP_BUILTIN_ISP_EVENT_FRM_ED);

		/* log first frame end timestamp */
		if (p_hdl->fs_cnt == 1) {
			if((kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IFE, IFE_FB_INT_STS_OFS) & IFE_FB_INT_ERR_CHK_BIT)) {
				ime_1st_frame_end_skip = TRUE;
				kdrv_ipp_builtin_set_reg(KDRV_IPP_BUILTIN_IFE, IFE_FB_INT_STS_OFS, 0xffffffff);	//clear all inter
			} else {
				nvt_bootts_add_ts("IME"); //end
			}
		} else {
			if (ime_1st_frame_end_skip == TRUE) {
				ime_1st_frame_end_skip = FALSE;
				nvt_bootts_add_ts("IME"); //end
			}
		}

		/* update isp info */
		kdrv_ipp_builtin_upd_isp_info(p_hdl);

		/* update lca gray avg */
		kdrv_ipp_builtin_upd_lca_gray_avg(p_hdl);

		if (ime_1st_frame_end_skip == FALSE) {
			/* frame end callback flow */
			info.name = p_hdl->name;

			if (p_hdl->flow != KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW && p_hdl->p_cur_job) {
				p_job = (KDRV_IPP_BUILTIN_JOB *)p_hdl->p_cur_job;
				if (p_job->buf_ctrl == SIE_BUILTIN_HEADER_CTL_PUSH) {
					p_hdl->out_buf_release_cnt++;
				}
			}
			info.release_flg = (p_hdl->out_buf_release_cnt > 0) ? 1 : 0;
			for (i = 0; i < KDRV_IPP_BUILTIN_PATH_ID_MAX; i++) {
				info.out_img[i] = kdrv_ipp_builtin_get_path_info(info.name, i);

				/* lock 3dnr last buffer for dram mode */
				if (p_hdl->out_buf_release_cnt == 2) {
					if ((p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_3DNR) && (p_hdl->flow != KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) &&
						i == p_hdl->_3dnr_ref_path && info.out_img[i].enable && info.out_img[i].addr[0]) {
						KDRV_IPP_BUILTIN_DUMP("FASTBOOT LOCK 3DNR REF 0x%.8x\r\n", info.out_img[i].addr[0]);
						nvtmpp_lock_fastboot_blk(info.out_img[i].addr[0]);
					}
				}
			}

			kdrv_ipp_builtin_frc(p_hdl, &info);
			for (i = 0; i < KDRV_IPP_BUILTIN_FMD_CB_NUM; i++) {
				if (p_ctl->fmd_cb[i]) {
					p_ctl->fmd_cb[i](&info, 0);
				}
			}

			/* release blk when release_flg == 1 */
			if (info.release_flg) {
				for (i = 0; i < KDRV_IPP_BUILTIN_PATH_ID_MAX; i++) {
					if (info.out_img[i].addr[0]) {
						nvtmpp_unlock_fastboot_blk(info.out_img[i].addr[0]);
						KDRV_IPP_BUILTIN_DUMP("release path%d buf 0x%.8x\r\n", (int)i, (unsigned int)info.out_img[i].addr[0]);
					}
				}
			}
		}

		/* set jobdone flg for d2d mode */
		if (p_hdl->flow != KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
			p_ctl->p_trig_hdl = NULL;
			vos_flag_iset(p_ctl->proc_tsk_flg_id, KDRV_IPP_BUILTIN_TSK_JOBDONE);
		}
	}

	if (sts & IME_INTE_FRM_START) {
		kdrv_ipp_builtin_debug_set_ts(KDRV_IPP_BUILTIN_DBG_TS_FST, &p_hdl->dbg);
		vos_perf_mark(&p_hdl->fs_timestamp);
		p_hdl->fs_cnt++;

		if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
			kdrv_ipp_builtin_cfg_eng_all(p_hdl);
		}
	}
}

KDRV_IPP_BUILTIN_JOB* kdrv_ipp_builtin_job_alloc(void)
{
	unsigned long loc_flg;
	KDRV_IPP_BUILTIN_MEM_POOL *p_mem_pool;
	KDRV_IPP_BUILTIN_JOB *p_job;

	p_mem_pool = &g_kdrv_ipp_builtin_ctl.job_pool;

	vk_spin_lock_irqsave(&p_mem_pool->lock, loc_flg);
	p_job = vos_list_first_entry_or_null(&p_mem_pool->free_list_root, KDRV_IPP_BUILTIN_JOB, pool_list);
	if (p_job) {
		vos_list_del_init(&p_job->pool_list);
		vos_list_add_tail(&p_job->pool_list, &p_mem_pool->used_list_root);
	}
	vk_spin_unlock_irqrestore(&p_mem_pool->lock, loc_flg);

	return p_job;
}

void kdrv_ipp_builtin_job_free(KDRV_IPP_BUILTIN_JOB *p_job)
{
	unsigned long loc_flg;
	KDRV_IPP_BUILTIN_MEM_POOL *p_mem_pool;

	p_mem_pool = &g_kdrv_ipp_builtin_ctl.job_pool;

	vk_spin_lock_irqsave(&p_mem_pool->lock, loc_flg);
	vos_list_del_init(&p_job->pool_list);
	vos_list_add_tail(&p_job->pool_list, &p_mem_pool->free_list_root);
	vk_spin_unlock_irqrestore(&p_mem_pool->lock, loc_flg);
}

KDRV_IPP_BUILTIN_JOB* kdrv_ipp_builtin_get_proc_job(void)
{
	unsigned long loc_flg;
	KDRV_IPP_BUILTIN_JOB *p_job;

	vk_spin_lock_irqsave(&g_kdrv_ipp_builtin_ctl.job_list_lock, loc_flg);
	p_job = vos_list_first_entry_or_null(&g_kdrv_ipp_builtin_ctl.job_list_root, KDRV_IPP_BUILTIN_JOB, proc_list);
	vk_spin_unlock_irqrestore(&g_kdrv_ipp_builtin_ctl.job_list_lock, loc_flg);

	return p_job;
}

KDRV_IPP_BUILTIN_JOB* kdrv_ipp_builtin_pop_proc_job(void)
{
	unsigned long loc_flg;
	KDRV_IPP_BUILTIN_JOB *p_job;

	vk_spin_lock_irqsave(&g_kdrv_ipp_builtin_ctl.job_list_lock, loc_flg);
	p_job = vos_list_first_entry_or_null(&g_kdrv_ipp_builtin_ctl.job_list_root, KDRV_IPP_BUILTIN_JOB, proc_list);
	if (p_job) {
		vos_list_del_init(&p_job->proc_list);
	}
	vk_spin_unlock_irqrestore(&g_kdrv_ipp_builtin_ctl.job_list_lock, loc_flg);

	return p_job;
}

void kdrv_ipp_builtin_del_proc_job(KDRV_IPP_BUILTIN_JOB *p_job)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&g_kdrv_ipp_builtin_ctl.job_list_lock, loc_flg);
	vos_list_del_init(&p_job->proc_list);
	vk_spin_unlock_irqrestore(&g_kdrv_ipp_builtin_ctl.job_list_lock, loc_flg);
}

void kdrv_ipp_builtin_put_job(KDRV_IPP_BUILTIN_JOB *p_job)
{
	unsigned long loc_flg;

	vk_spin_lock_irqsave(&g_kdrv_ipp_builtin_ctl.job_list_lock, loc_flg);
	vos_list_add_tail(&p_job->proc_list, &g_kdrv_ipp_builtin_ctl.job_list_root);
	vk_spin_unlock_irqrestore(&g_kdrv_ipp_builtin_ctl.job_list_lock, loc_flg);
}

void kdrv_ipp_builtin_flush_job(void)
{
	KDRV_IPP_BUILTIN_JOB *p_job;

	while ((p_job = kdrv_ipp_builtin_pop_proc_job()) != NULL) {
		DBG_ERR("fastboot queue job flushed\r\n");
	}
}

void kdrv_ipp_builtin_sie_cb(UINT32 id, SIE_BUILTIN_HEADER_INFO *p_info)
{
	KDRV_IPP_BUILTIN_HDL *p_hdl;
	KDRV_IPP_BUILTIN_JOB *p_job;
	UINT32 i;

	/* todo: hdr collect two frame */
	/* find handle with corresponding src_sie_id */
	p_hdl = NULL;
	for (i = 0; i < g_kdrv_ipp_builtin_ctl.hdl_num; i++) {
		if (g_kdrv_ipp_builtin_ctl.p_hdl[i].src_sie_id_bit & (1 << id)) {
			p_hdl = &g_kdrv_ipp_builtin_ctl.p_hdl[i];
			break;
		}
	}
	if (p_hdl == NULL) {
		return ;
	}
	kdrv_ipp_builtin_debug_set_ts(KDRV_IPP_BUILTIN_DBG_TS_PUSH_IN, &p_hdl->dbg);

	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_SHDR) {
		#if KDRV_IPP_BUILTIN_NEW_BIND_FLOW
		#else
		DBG_WRN("dram mode hdr not ready\r\n");
		return ;
		#endif
	}

	if (p_info->buf_addr && p_info->addr_ch0) {
		p_job = kdrv_ipp_builtin_job_alloc();
		if (p_job == NULL) {
			DBG_ERR("job queue full\r\n");
			return ;
		}

		p_job->p_owner = p_hdl;
		p_job->blk[0] = p_info->buf_addr;
		p_job->va[0] = p_info->addr_ch0;
		p_job->va[1] = p_info->addr_ch1;
		p_job->buf_ctrl = p_info->buf_ctrl;
		p_job->timestamp = p_info->timestamp;
		p_job->count = p_info->count;
		p_job->blk[1] = 0;

		if (p_job->buf_ctrl == SIE_BUILTIN_HEADER_CTL_PUSH) {
			KDRV_IPP_BUILTIN_DUMP("fastboot hdl %s receive sie blk 0x%.8x, 0x%.8x for release blk\r\n",
				p_hdl->name, (unsigned int)p_job->blk[0], (unsigned int)p_job->blk[1]);
		}

		p_hdl->push_in_cnt++;
		kdrv_ipp_builtin_put_job(p_job);
		vos_flag_iset(g_kdrv_ipp_builtin_ctl.proc_tsk_flg_id, KDRV_IPP_BUILTIN_TSK_TRIGGER);
	} else {
		DBG_ERR("sie(%d) buf_addr 0x%.8x, ch0_addr 0x%.8x\r\n", (int)id, (unsigned int)p_info->buf_addr, (unsigned int)p_info->addr_ch0);
	}

	return ;
}

THREAD_DECLARE(kdrv_ipp_builtin_tsk, p1)
{
	FLGPTN wait_flg;
	KDRV_IPP_BUILTIN_CTL *p_ctl;
	KDRV_IPP_BUILTIN_HDL *p_hdl;
	KDRV_IPP_BUILTIN_JOB *p_job;
	INT32 rt;
	UINT8 i;
	UINT8 fastboot_done;

	p_ctl = &g_kdrv_ipp_builtin_ctl;

	THREAD_ENTRY();

	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(p_ctl->proc_tsk_flg_id, KDRV_IPP_BUILTIN_TSK_RESUME_END);
		PROFILE_TASK_IDLE();

		/* receive job from list */
		while (1) {
			p_job = kdrv_ipp_builtin_pop_proc_job();
			if (p_job == NULL) {
				vos_flag_wait(&wait_flg, p_ctl->proc_tsk_flg_id, KDRV_IPP_BUILTIN_TSK_TRIGGER, TWF_CLR);
			} else {
				break;
			}
		}

		/* process job */
		PROFILE_TASK_BUSY();
		p_hdl = p_job->p_owner;
		p_hdl->p_cur_job = (void *)p_job;
		rt = kdrv_ipp_builtin_cfg_eng_all(p_hdl);
		if (rt != 0) {
			/* config failed drop frame */
		} else {
			/* trigger engine, wait job end */
			vos_flag_clr(p_ctl->proc_tsk_flg_id, KDRV_IPP_BUILTIN_TSK_JOBDONE);
			kdrv_ipp_builtin_trig_eng_start(p_hdl);
			vos_flag_wait(&wait_flg, p_ctl->proc_tsk_flg_id, KDRV_IPP_BUILTIN_TSK_JOBDONE, TWF_CLR);
		}

		/* release buffer, check if all buffer release and all handle release */
		if (p_job->buf_ctrl == SIE_BUILTIN_HEADER_CTL_PUSH) {
			nvtmpp_unlock_fastboot_blk(p_job->blk[0]);
			if (p_job->blk[1]) {
				nvtmpp_unlock_fastboot_blk(p_job->blk[1]);
			}
			p_hdl->in_buf_release_cnt++;
			KDRV_IPP_BUILTIN_DUMP("fastboot hdl %s release sie blk 0x%.8x, 0x%.8x, release_cnt %d\r\n",
				p_hdl->name, (unsigned int)p_job->blk[0], (unsigned int)p_job->blk[1], (int)p_hdl->in_buf_release_cnt);

			fastboot_done = TRUE;
			for (i = 0; i < p_ctl->hdl_num; i++) {
				/* only check hdl with valid src_id */
				if (kdrv_ipp_builtin_check_src_valid(&p_ctl->p_hdl[i])) {
					fastboot_done &= (p_ctl->p_hdl[i].in_buf_release_cnt >= KDRV_IPP_BUILTIN_BUF_SIE);
				}
			}
			if (fastboot_done) {
				/* release job and exit task */
				kdrv_ipp_builtin_job_free(p_job);
				goto IPP_TSK_DESTROY;
			}
		}

		/* release job */
		p_hdl->p_cur_job = NULL;
		kdrv_ipp_builtin_job_free(p_job);
	}

IPP_TSK_DESTROY:
	vos_flag_set(p_ctl->proc_tsk_flg_id, (KDRV_IPP_BUILTIN_TSK_EXIT_END | KDRV_IPP_BUILTIN_TSK_FASTBOOT_DONE));
	THREAD_RETURN(0);
}

INT32 kdrv_ipp_builtin_init_out_path_info(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	CHAR regnode[64];
	UINT32 cfg_eng_bit;
	UINT32 reg_cfg_num = 0;
	UINT32 *p_reg_buf;
	UINT32 eng;
	UINT32 ofs;
	UINT32 val;
	UINT32 i;
	IME_GET_INPATH_INFO in_info;
	IME_GET_OUTPATH_INFO eng_path;
	KDRV_IPP_BUILTIN_IMG_INFO *p_path;

	cfg_eng_bit = kdrv_ipp_builtin_get_cfg_eng_bit(p_hdl);
	eng = KDRV_IPP_BUILTIN_IME;
	if ((cfg_eng_bit & KDRV_IPP_BUILTIN_ENG_BIT(eng)) == 0) {
		return 0;
	}

	snprintf(regnode, 64, "%s/register", p_hdl->node);
	p_reg_buf = kdrv_ipp_builtin_get_dtsi_buf();
	if (kdrv_ipp_builtin_plat_read_dtsi_array(regnode, "ime_reg_num", &reg_cfg_num, 1)) {
		DBG_ERR("read dtsi %s failed\r\n", "ime_reg_num");
		return -1;
	}
	if (kdrv_ipp_builtin_plat_read_dtsi_array(regnode, "ime_reg_cfg", p_reg_buf, (reg_cfg_num * 2))) {
		DBG_ERR("read dtsi %s failed\r\n", "ime_reg_cfg");
		return -1;
	}

	for (i = 0; i < reg_cfg_num; i++) {
		ofs = p_reg_buf[i * 2];
		val = p_reg_buf[(i * 2) + 1];

		/* skip engine ctrl regiser */
		if (ofs == 0) {
			continue;
		} else {
			/* write config to engine */
			kdrv_ipp_builtin_set_reg(eng, ofs, val);
		}
	}

	/* path 1~4 info */
	for (i = KDRV_IPP_BUILTIN_PATH_ID_1; i <= KDRV_IPP_BUILTIN_PATH_ID_4; i++) {
		ime_get_output_path_info((1 << i), &eng_path);

		p_path = &p_hdl->path_info[i];
		p_path->enable = eng_path.out_path_enable;
		if (p_path->enable) {
			p_path->size.w = eng_path.out_path_out_size.size_h;
			p_path->size.h = eng_path.out_path_out_size.size_v;
			p_path->fmt = kdrv_ipp_builtin_typecast_fmt(i, &eng_path);
			p_path->loff[0] = eng_path.out_path_out_lineoffset.lineoffset_y;
			p_path->loff[1] = eng_path.out_path_out_lineoffset.lineoffset_cb;
		}

		KDRV_IPP_BUILTIN_DUMP("path%d, en %d, size(%4d, %4d), lofs(%4d %4d), fmt 0x%.8x\r\n",
			(int)i, (int)p_path->enable, (int)p_path->size.w, (int)p_path->size.h,
			(int)p_path->loff[0], (int)p_path->loff[1], (unsigned int)p_path->fmt);
	}

	/* reference out info */
	ime_get_input_path_info(&in_info);
	p_path = &p_hdl->path_info[KDRV_IPP_BUILTIN_PATH_ID_5];
	p_path->enable = (kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IME, 0x04) & 0x40000000) ? ENABLE : DISABLE;
	if (p_path->enable) {
		p_path->size.w = in_info.in_path_size.size_h;
		p_path->size.h = in_info.in_path_size.size_v;
		p_path->fmt = kdrv_ipp_builtin_typecast_fmt(KDRV_IPP_BUILTIN_PATH_ID_5, NULL);
		p_path->loff[0] = p_path->size.w;
		p_path->loff[1] = p_path->size.w;
	}
	KDRV_IPP_BUILTIN_DUMP("path%d, en %d\r\n", KDRV_IPP_BUILTIN_PATH_ID_5, (int)p_path->enable);

	return 0;
}

INT32 kdrv_ipp_builtin_init_frc(KDRV_IPP_BUILTIN_HDL* p_hdl)
{
	INT32 rt;
	UINT32 i;
	UINT32 tmp[KDRV_IPP_BUILTIN_PATH_ID_MAX] = {0};
	CHAR frc_node[48];

	snprintf(frc_node, 48, "%s/frm-rate-ctrl", p_hdl->node);
	rt = kdrv_ipp_builtin_plat_read_dtsi_array(frc_node, "skip", (UINT32 *)tmp, KDRV_IPP_BUILTIN_PATH_ID_MAX);
	if (rt != 0) {
		DBG_WRN("read ipp %s/skip failed\r\n", frc_node);
	}
	for (i = 0; i < KDRV_IPP_BUILTIN_PATH_ID_MAX; i++) {
		p_hdl->frc[i].skip = (rt == 0) ? tmp[i] : 0;
	}

	rt = kdrv_ipp_builtin_plat_read_dtsi_array(frc_node, "src", (UINT32 *)tmp, KDRV_IPP_BUILTIN_PATH_ID_MAX);
	if (rt != 0) {
		DBG_WRN("read ipp %s/src failed\r\n", frc_node);
	}
	for (i = 0; i < KDRV_IPP_BUILTIN_PATH_ID_MAX; i++) {
		p_hdl->frc[i].src = (rt == 0) ? tmp[i] : 30;
	}

	rt = kdrv_ipp_builtin_plat_read_dtsi_array(frc_node, "dst", (UINT32 *)tmp, KDRV_IPP_BUILTIN_PATH_ID_MAX);
	if (rt != 0) {
		DBG_WRN("read ipp %s/dst failed\r\n", frc_node);
	}
	for (i = 0; i < KDRV_IPP_BUILTIN_PATH_ID_MAX; i++) {
		p_hdl->frc[i].dst = (rt == 0) ? tmp[i] : 30;

		if (p_hdl->frc[i].src && (p_hdl->frc[i].src != p_hdl->frc[i].dst)) {
			p_hdl->frc[i].drop_rate = (KDRV_IPP_BUILTIN_FRC_BASE * p_hdl->frc[i].src) / (p_hdl->frc[i].src - p_hdl->frc[i].dst);
		} else {
			p_hdl->frc[i].drop_rate = 0;
		}
		p_hdl->frc[i].rate_cnt = 0;

		KDRV_IPP_BUILTIN_DUMP("path %d: skip %d; frc dst/src = %d/%d\r\n", i, p_hdl->frc[i].skip, p_hdl->frc[i].dst, p_hdl->frc[i].src);
	}

	return E_OK;
}

INT32 kdrv_ipp_builtin_init_privacy_mask(KDRV_IPP_BUILTIN_HDL* p_hdl)
{
	USIZE ipp_in_size;
	UINT32 *p_buf;
	UINT32 i, j, tmp;
	UINT8 chk_limit;
	CHAR mask_node[48];

	snprintf(mask_node, 48, "%s/privacy-mask", p_hdl->node);

	/* borrow vprc ctrl buffer as read dtsi buffer */
	p_buf = kdrv_ipp_builtin_get_dtsi_buf();
	if (p_buf == NULL) {
		DBG_ERR("no buffer to read dtsi\r\n");
		goto MASK_INIT_ERR;
	}

	/* parse dtsi to struct first */
	/* enable */
	if (kdrv_ipp_builtin_plat_read_dtsi_array(mask_node, "enable", p_buf, KDRV_IPP_BUILTIN_PRI_MASK_NUM)) {
		DBG_ERR("read dtsi failed\r\n");
		goto MASK_INIT_ERR;
	}
	for (i = 0; i < KDRV_IPP_BUILTIN_PRI_MASK_NUM; i++) {
		p_hdl->mask[i].enable = p_buf[i];
	}

	/* coordinate, each mask has 4 coordinate(x, y) */
	if (kdrv_ipp_builtin_plat_read_dtsi_array(mask_node, "coordinate", p_buf, KDRV_IPP_BUILTIN_PRI_MASK_NUM * 8)) {
		DBG_ERR("read dtsi failed\r\n");
		goto MASK_INIT_ERR;
	}
	for (i = 0; i < KDRV_IPP_BUILTIN_PRI_MASK_NUM; i++) {
		p_hdl->mask[i].coord[0].x = p_buf[i * 8];
		p_hdl->mask[i].coord[0].y = p_buf[(i * 8) + 1];
		p_hdl->mask[i].coord[1].x = p_buf[(i * 8) + 2];
		p_hdl->mask[i].coord[1].y = p_buf[(i * 8) + 3];
		p_hdl->mask[i].coord[2].x = p_buf[(i * 8) + 4];
		p_hdl->mask[i].coord[2].y = p_buf[(i * 8) + 5];
		p_hdl->mask[i].coord[3].x = p_buf[(i * 8) + 6];
		p_hdl->mask[i].coord[3].y = p_buf[(i * 8) + 7];
	}

	/* alpha */
	if (kdrv_ipp_builtin_plat_read_dtsi_array(mask_node, "alpha", p_buf, KDRV_IPP_BUILTIN_PRI_MASK_NUM)) {
		DBG_ERR("read dtsi failed\r\n");
		goto MASK_INIT_ERR;
	}
	for (i = 0; i < KDRV_IPP_BUILTIN_PRI_MASK_NUM; i++) {
		p_hdl->mask[i].weight = p_buf[i];
	}

	/* yuv color, each mask has 3 component(y, u, v) */
	if (kdrv_ipp_builtin_plat_read_dtsi_array(mask_node, "color", p_buf, KDRV_IPP_BUILTIN_PRI_MASK_NUM * 3)) {
		DBG_ERR("read dtsi failed\r\n");
		goto MASK_INIT_ERR;
	}
	for (i = 0; i < KDRV_IPP_BUILTIN_PRI_MASK_NUM; i++) {
		p_hdl->mask[i].color[0] = p_buf[i * 3];
		p_hdl->mask[i].color[1] = p_buf[(i * 3) + 1];
		p_hdl->mask[i].color[2] = p_buf[(i * 3) + 2];
	}

	/* hlw_enable */
	if (kdrv_ipp_builtin_plat_read_dtsi_array(mask_node, "hlw_enable", p_buf, KDRV_IPP_BUILTIN_PRI_MASK_NUM)) {
		DBG_ERR("read dtsi failed\r\n");
		goto MASK_INIT_ERR;
	}
	for (i = 0; i < KDRV_IPP_BUILTIN_PRI_MASK_NUM; i++) {
		p_hdl->mask[i].hlw_enable = p_buf[i];
	}

	/* hlw_coordinate, each mask has 4 coordinate(x, y) */
	if (kdrv_ipp_builtin_plat_read_dtsi_array(mask_node, "hlw_coordinate", p_buf, KDRV_IPP_BUILTIN_PRI_MASK_NUM * 8)) {
		DBG_ERR("read dtsi failed\r\n");
		goto MASK_INIT_ERR;
	}
	for (i = 0; i < KDRV_IPP_BUILTIN_PRI_MASK_NUM; i++) {
		p_hdl->mask[i].coord2[0].x = p_buf[i * 8];
		p_hdl->mask[i].coord2[0].y = p_buf[(i * 8) + 1];
		p_hdl->mask[i].coord2[1].x = p_buf[(i * 8) + 2];
		p_hdl->mask[i].coord2[1].y = p_buf[(i * 8) + 3];
		p_hdl->mask[i].coord2[2].x = p_buf[(i * 8) + 4];
		p_hdl->mask[i].coord2[2].y = p_buf[(i * 8) + 5];
		p_hdl->mask[i].coord2[3].x = p_buf[(i * 8) + 6];
		p_hdl->mask[i].coord2[3].y = p_buf[(i * 8) + 7];
	}

	/* limitation check */
	chk_limit = TRUE;
	tmp = kdrv_ipp_builtin_get_reg(KDRV_IPP_BUILTIN_IME, 0x20);
	ipp_in_size.w = tmp & 0xffff;
	ipp_in_size.h = (tmp >> 16) & 0xffff;
	for (i = 0; i < KDRV_IPP_BUILTIN_PRI_MASK_NUM; i++) {
		if (p_hdl->mask[i].hlw_enable && (i & 0x1)) {
			DBG_ERR("privacy mask %d, hollow mask only support PM_SET_0/2/4/6", (int)i);
			chk_limit = FALSE;
		}

		if (p_hdl->mask[i].enable && (i & 0x1)) {
			if (p_hdl->mask[i - 1].hlw_enable) {
				DBG_ERR("privacy mask %d, when PM_SET_0/2/4/6 enable hollow mask, PM_SET_1/3/5/7 can not be used\r\n", (int)i);
				chk_limit = FALSE;
			}
		}

		for (j = 0; j < 4; j++) {
			if (p_hdl->mask[i].enable) {
				if (p_hdl->mask[i].coord[j].x > ipp_in_size.w || p_hdl->mask[i].coord[j].y > ipp_in_size.h) {
					DBG_ERR("privacy mask %d, coord(%d %d) overflow, ipp_in_size(%d, %d)\r\n", (int)i,
								(int)p_hdl->mask[i].coord[j].x, (int)p_hdl->mask[i].coord[j].y, (int)ipp_in_size.w, (int)ipp_in_size.h);
					chk_limit = FALSE;
				}

				if (p_hdl->mask[i].hlw_enable) {
					if (p_hdl->mask[i].coord[j].x > ipp_in_size.w || p_hdl->mask[i].coord[j].y > ipp_in_size.h) {
						DBG_ERR("privacy mask %d, hlw_coord(%d %d) overflow, ipp_in_size(%d, %d)\r\n", (int)i,
									(int)p_hdl->mask[i].coord[j].x, (int)p_hdl->mask[i].coord[j].y, (int)ipp_in_size.w, (int)ipp_in_size.h);
						chk_limit = FALSE;
					}
				}
			}
		}
	}

	if (!chk_limit) {
		DBG_ERR("privacy mask config not illegal, skip mask config\r\n");
		goto MASK_INIT_ERR;
	}

	/* dump privacy mask information */
	KDRV_IPP_BUILTIN_DUMP("\r\n%8s %4s %16s %4s %16s %6s %10s %10s %10s %8s\r\n",
				"mask_id", "en", "mask_points", "hlw", "hlw_points", "type", "color[0]", "color[1]", "color[2]", "weight");
	for (i = 0; i < KDRV_IPP_BUILTIN_PRI_MASK_NUM; i++) {
		KDRV_IPP_BUILTIN_DUMP("%8d %4d (%6d, %6d) %4d (%6d, %6d) %6d %10d %10d %10d %8d\r\n",
					(int)i,
					(int)p_hdl->mask[i].enable,
					(int)p_hdl->mask[i].coord[0].x,
					(int)p_hdl->mask[i].coord[0].y,
					(int)p_hdl->mask[i].hlw_enable,
					(int)p_hdl->mask[i].coord2[0].x,
					(int)p_hdl->mask[i].coord2[0].y,
					(int)0,
					(int)p_hdl->mask[i].color[0],
					(int)p_hdl->mask[i].color[1],
					(int)p_hdl->mask[i].color[2],
					(int)p_hdl->mask[i].weight);
		if (p_hdl->mask[i].enable) {
			for (j = 1; j < 4; j++) {
				KDRV_IPP_BUILTIN_DUMP("%8s %4s (%6d, %6d) %4s (%6d, %6d)\r\n", " ", " ",
							(int)p_hdl->mask[i].coord[j].x,
							(int)p_hdl->mask[i].coord[j].y,
							" ",
							(int)p_hdl->mask[i].coord2[j].x,
							(int)p_hdl->mask[i].coord2[j].y);
			}
		}
	}

	return 0;

MASK_INIT_ERR:
	for (i = 0; i < KDRV_IPP_BUILTIN_PRI_MASK_NUM; i++) {
		p_hdl->mask[i].enable = DISABLE;
	}
	return 0;
}

INT32 kdrv_ipp_builtin_init_handle(KDRV_IPP_BUILTIN_HDL *p_hdl)
{
	UINT32 cur_buf_addr;
	UINT32 tmp_buf_size = 0;
	UINT32 i, j;
	CHAR buf_node[48];
	KDRV_IPP_BUILTIN_IMG_INFO *p_path;

	snprintf(buf_node, 48, "%s/buffer-size", p_hdl->node);

	cur_buf_addr = p_hdl->vprc_ctrl.addr;
	if(kdrv_ipp_builtin_plat_read_dtsi_array(p_hdl->node, "func_en", (UINT32 *)&p_hdl->func_en, 1)){
		DBG_ERR("read dtsi func_en failed\r\n");
	}

	KDRV_IPP_BUILTIN_DUMP("name %s, flow %d, isp_id 0x%.8x, func_en 0x%.8x\r\n",
		p_hdl->name, (int)p_hdl->flow, (unsigned int)p_hdl->isp_id, (unsigned int)p_hdl->func_en);

	if(kdrv_ipp_builtin_plat_read_dtsi_array(p_hdl->node, "3dnr_ref_path", (UINT32 *)&p_hdl->_3dnr_ref_path, 1)){
		DBG_ERR("read dtsi 3dnr_ref_path failed\r\n");
	}

	if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW && p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_SHDR) {
		p_hdl->vprc_shdr_blk.phy_addr = nvtmpp_sys_va2pa(p_hdl->vprc_shdr_blk.addr);
		if (p_hdl->vprc_shdr_blk.phy_addr == 0) {
			DBG_ERR("shdr ringbuf addr 0\r\n");
			return -1;
		}
	}

	memset((void *)&p_hdl->pri_buf, 0, sizeof(KDRV_IPP_BUILTIN_PRI_BUF));
	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_YUV_SUBOUT) {
		if(kdrv_ipp_builtin_plat_read_dtsi_array(buf_node, "lca", &tmp_buf_size, 1)){
			DBG_ERR("read dtsi lca failed\r\n");
		}
		for (i = 0; i < KDRV_IPP_BUILTIN_BUF_LCA_NUM; i++) {
			p_hdl->pri_buf.lca[i].addr = cur_buf_addr;
			p_hdl->pri_buf.lca[i].size = tmp_buf_size;
			p_hdl->pri_buf.lca[i].phy_addr = nvtmpp_sys_va2pa(cur_buf_addr);
			if (p_hdl->pri_buf.lca[i].phy_addr == 0) {
				DBG_ERR("va2pa failed 0x%.8x\r\n", cur_buf_addr);
				return -1;
			}

			/* support single buffer for direct mode, add addr at last buffer idx */
			if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
				if (i == (KDRV_IPP_BUILTIN_BUF_LCA_NUM - 1)) {
					cur_buf_addr += tmp_buf_size;
				}
			} else {
				cur_buf_addr += tmp_buf_size;
			}
		}
		KDRV_IPP_BUILTIN_DUMP("LCA: 0x%.8x, 0x%.8x, size 0x%.8x\r\n",
			p_hdl->pri_buf.lca[0].addr, p_hdl->pri_buf.lca[1].addr, p_hdl->pri_buf.lca[0].size);
	}

	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_3DNR) {
		if(kdrv_ipp_builtin_plat_read_dtsi_array(buf_node, "3dnr_mv", &tmp_buf_size, 1)){
			DBG_ERR("read dtsi 3dnr_mv failed\r\n");
		}
		for (i = 0; i < KDRV_IPP_BUILTIN_BUF_3DNR_NUM; i++) {
			p_hdl->pri_buf._3dnr_mv[i].addr = cur_buf_addr;
			p_hdl->pri_buf._3dnr_mv[i].size = tmp_buf_size;
			p_hdl->pri_buf._3dnr_mv[i].phy_addr = nvtmpp_sys_va2pa(cur_buf_addr);
			if (p_hdl->pri_buf._3dnr_mv[i].phy_addr == 0) {
				DBG_ERR("va2pa failed 0x%.8x\r\n", cur_buf_addr);
				return -1;
			}

			/* support single buffer for direct mode, add addr at last buffer idx */
			if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
				if (i == (KDRV_IPP_BUILTIN_BUF_3DNR_NUM - 1)) {
					cur_buf_addr += tmp_buf_size;
				}
			} else {
				cur_buf_addr += tmp_buf_size;
			}
		}
		KDRV_IPP_BUILTIN_DUMP("3DNR MV: 0x%.8x, 0x%.8x, size 0x%.8x\r\n",
			p_hdl->pri_buf._3dnr_mv[0].addr, p_hdl->pri_buf._3dnr_mv[1].addr, p_hdl->pri_buf._3dnr_mv[0].size);

		if(kdrv_ipp_builtin_plat_read_dtsi_array(buf_node, "3dnr_ms", &tmp_buf_size, 1)){
			DBG_ERR("read dtsi 3dnr_ms failed\r\n");
		}
		for (i = 0; i < KDRV_IPP_BUILTIN_BUF_3DNR_NUM; i++) {
			p_hdl->pri_buf._3dnr_ms[i].addr = cur_buf_addr;
			p_hdl->pri_buf._3dnr_ms[i].size = tmp_buf_size;
			p_hdl->pri_buf._3dnr_ms[i].phy_addr = nvtmpp_sys_va2pa(cur_buf_addr);
			if (p_hdl->pri_buf._3dnr_ms[i].phy_addr == 0) {
				DBG_ERR("va2pa failed 0x%.8x\r\n", cur_buf_addr);
				return -1;
			}

			/* support single buffer for direct mode, add addr at last buffer idx */
			if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
				if (i == (KDRV_IPP_BUILTIN_BUF_3DNR_NUM - 1)) {
					cur_buf_addr += tmp_buf_size;
				}
			} else {
				cur_buf_addr += tmp_buf_size;
			}
		}
		KDRV_IPP_BUILTIN_DUMP("3DNR MS: 0x%.8x, 0x%.8x, size 0x%.8x\r\n",
			p_hdl->pri_buf._3dnr_ms[0].addr, p_hdl->pri_buf._3dnr_ms[1].addr, p_hdl->pri_buf._3dnr_ms[0].size);

		if(kdrv_ipp_builtin_plat_read_dtsi_array(buf_node, "3dnr_ms_roi", &tmp_buf_size, 1)){
			DBG_ERR("read dtsi 3dnr_ms_roi failed\r\n");
		}
		for (i = 0; i < KDRV_IPP_BUILTIN_BUF_3DNR_NUM; i++) {
			p_hdl->pri_buf._3dnr_ms_roi[i].addr = cur_buf_addr;
			p_hdl->pri_buf._3dnr_ms_roi[i].size = tmp_buf_size;
			p_hdl->pri_buf._3dnr_ms_roi[i].phy_addr = nvtmpp_sys_va2pa(cur_buf_addr);
			if (p_hdl->pri_buf._3dnr_ms_roi[i].phy_addr == 0) {
				DBG_ERR("va2pa failed 0x%.8x\r\n", cur_buf_addr);
				return -1;
			}
			cur_buf_addr += tmp_buf_size;
		}
		KDRV_IPP_BUILTIN_DUMP("3DNR MS ROI: 0x%.8x, 0x%.8x, size 0x%.8x\r\n",
			p_hdl->pri_buf._3dnr_ms_roi[0].addr, p_hdl->pri_buf._3dnr_ms_roi[1].addr, p_hdl->pri_buf._3dnr_ms_roi[0].size);

		if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_3DNR_STA) {
			if(kdrv_ipp_builtin_plat_read_dtsi_array(buf_node, "3dnr_sta", &tmp_buf_size, 1)){
				DBG_ERR("read dtsi 3dnr_sta failed\r\n");
			}
			for (i = 0; i < KDRV_IPP_BUILTIN_BUF_3DNR_STA_NUM; i++) {
				p_hdl->pri_buf._3dnr_sta[i].addr = cur_buf_addr;
				p_hdl->pri_buf._3dnr_sta[i].size = tmp_buf_size;
				p_hdl->pri_buf._3dnr_sta[i].phy_addr = nvtmpp_sys_va2pa(cur_buf_addr);
				if (p_hdl->pri_buf._3dnr_sta[i].phy_addr == 0) {
					DBG_ERR("va2pa failed 0x%.8x\r\n", cur_buf_addr);
					return -1;
				}
				cur_buf_addr += tmp_buf_size;
			}
			KDRV_IPP_BUILTIN_DUMP("3DNR STA: 0x%.8x, size 0x%.8x\r\n",
				p_hdl->pri_buf._3dnr_sta[0].addr, p_hdl->pri_buf._3dnr_sta[0].size);
		}
	}

	if(kdrv_ipp_builtin_plat_read_dtsi_array(buf_node, "defog", &tmp_buf_size, 1)){
		DBG_ERR("read dtsi defog failed\r\n");
	}
	for (i = 0; i < KDRV_IPP_BUILTIN_BUF_DFG_NUM; i++) {
		p_hdl->pri_buf.defog[i].addr = cur_buf_addr;
		p_hdl->pri_buf.defog[i].size = tmp_buf_size;
		p_hdl->pri_buf.defog[i].phy_addr = nvtmpp_sys_va2pa(cur_buf_addr);
		if (p_hdl->pri_buf.defog[i].phy_addr == 0) {
			DBG_ERR("va2pa failed 0x%.8x\r\n", cur_buf_addr);
			return -1;
		}
		cur_buf_addr += tmp_buf_size;
	}
	KDRV_IPP_BUILTIN_DUMP("DEFOG: 0x%.8x, 0x%.8x, size 0x%.8x\r\n",
		p_hdl->pri_buf.defog[0].addr, p_hdl->pri_buf.defog[1].addr, p_hdl->pri_buf.defog[0].size);

	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_PM_PIXELIZTION) {
		if(kdrv_ipp_builtin_plat_read_dtsi_array(buf_node, "pm", &tmp_buf_size, 1)){
			DBG_ERR("read dtsi pm failed\r\n");
		}
		for (i = 0; i < KDRV_IPP_BUILTIN_BUF_PM_NUM; i++) {
			p_hdl->pri_buf.pm[i].addr = cur_buf_addr;
			p_hdl->pri_buf.pm[i].size = tmp_buf_size;
			p_hdl->pri_buf.pm[i].phy_addr = nvtmpp_sys_va2pa(cur_buf_addr);
			if (p_hdl->pri_buf.pm[i].phy_addr == 0) {
				DBG_ERR("va2pa failed 0x%.8x\r\n", cur_buf_addr);
				return -1;
			}
			cur_buf_addr += tmp_buf_size;
		}
		KDRV_IPP_BUILTIN_DUMP("PM: 0x%.8x, 0x%.8x, 0x%.8x, size 0x%.8x\r\n",
			p_hdl->pri_buf.pm[0].addr, p_hdl->pri_buf.pm[1].addr, p_hdl->pri_buf.pm[2].addr, p_hdl->pri_buf.pm[0].size);

	}

	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_WDR) {
		if(kdrv_ipp_builtin_plat_read_dtsi_array(buf_node, "wdr", &tmp_buf_size, 1)){
			DBG_ERR("read dtsi wdr failed\r\n");
		}
		for (i = 0; i < KDRV_IPP_BUILTIN_BUF_WDR_NUM; i++) {
			p_hdl->pri_buf.wdr[i].addr = cur_buf_addr;
			p_hdl->pri_buf.wdr[i].size = tmp_buf_size;
			p_hdl->pri_buf.wdr[i].phy_addr = nvtmpp_sys_va2pa(cur_buf_addr);
			if (p_hdl->pri_buf.wdr[i].phy_addr == 0) {
				DBG_ERR("va2pa failed 0x%.8x\r\n", cur_buf_addr);
				return -1;
			}
			cur_buf_addr += tmp_buf_size;
		}
		KDRV_IPP_BUILTIN_DUMP("WDR: 0x%.8x, 0x%.8x, size 0x%.8x\r\n",
			p_hdl->pri_buf.wdr[0].addr, p_hdl->pri_buf.wdr[1].addr, p_hdl->pri_buf.wdr[0].size);

	}

	if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_GDC) {
		/* enable cnn sram for dce */
		kdrv_builtin_set_sram_ctrl(FBOOT_SRAMCTRL_DCE);
	}

	if ((cur_buf_addr - p_hdl->vprc_ctrl.addr) > p_hdl->vprc_ctrl.size) {
		DBG_ERR("buffer size overflow\r\n");
		return -1;
	}

	/* output buffer */
	kdrv_ipp_builtin_init_out_path_info(p_hdl);
	for (i = 0; i < KDRV_IPP_BUILTIN_PATH_ID_MAX; i++) {
		p_path = &p_hdl->path_info[i];
		for (j = 0; j < 2; j++) {
			if (p_path->enable) {
				p_hdl->vprc_blk[i][j].size = kdrv_ipp_builtin_yuv_size(p_path);
				p_hdl->vprc_blk[i][j].addr = nvtmpp_get_fastboot_blk(p_hdl->vprc_blk[i][j].size);

				if (p_hdl->vprc_blk[i][j].addr && p_hdl->vprc_blk[i][j].size) {
					p_hdl->vprc_blk[i][j].phy_addr = nvtmpp_sys_va2pa(p_hdl->vprc_blk[i][j].addr);
					if (p_hdl->vprc_blk[i][j].phy_addr == 0) {
						DBG_ERR("get phy addr failed, va = 0x%.8x\r\n", (unsigned int)p_hdl->vprc_blk[i][j].addr);
						return -1;
					}
				}
			}

			KDRV_IPP_BUILTIN_DUMP("pid %d, blk %d, addr 0x%.8x, size 0x%.8x, pa 0x%.8x\r\n",
				i, j, (unsigned int)p_hdl->vprc_blk[i][j].addr, (unsigned int)p_hdl->vprc_blk[i][j].size, (unsigned int)p_hdl->vprc_blk[i][j].phy_addr);
		}
	}

	/* frame rate ctrl init */
	kdrv_ipp_builtin_init_frc(p_hdl);

	/* privact mask ctrl init */
	kdrv_ipp_builtin_init_privacy_mask(p_hdl);

	/* init debug spinlock */
	vk_spin_lock_init(&p_hdl->dbg.lock);

	return 0;
}

INT32 kdrv_ipp_builtin_init(KDRV_IPP_BUILTIN_INIT_INFO *info)
{
	KDRV_IPP_BUILTIN_BLK reg_base_info[KDRV_IPP_BUILTIN_ENG_MAX] = {
		{0xF0C70000, 0x800},	/* IFE */
		{0xF0C20000, 0x650},	/* DCE */
		{0xF0C30000, 0x900},	/* IPE */
		{0xF0C40000, 0x1000},	/* IME */
		{0xF0D00000, 0x100},	/* IFE2 */
	};
	KDRV_IPP_BUILTIN_CTL* p_ctl;
	KDRV_IPP_BUILTIN_HDL* p_hdl;
	UINT32 i, k, buf_size;

	if (kdrv_builtin_is_fastboot() == FALSE) {
		return E_NOSPT;
	}

	if (info == NULL) {
		DBG_ERR("init info null\r\n");
		return -1;
	}

	nvt_bootts_add_ts("IME"); //begin

	KDRV_IPP_BUILTIN_DUMP("\r\n\r\n-------------------------- ipp builtin init ------------------------------- \r\n");
	/* init ime driver register flow */
	ime_base_reg_init();

	/* init builtin flow */
	p_ctl = &g_kdrv_ipp_builtin_ctl;
	memset((void *)p_ctl, 0, sizeof(KDRV_IPP_BUILTIN_CTL));
	p_ctl->valid_src_id_bit = info->valid_src_id_bit;
	KDRV_IPP_BUILTIN_DUMP("valid src id bit 0x%.8x\r\n", (unsigned int)p_ctl->valid_src_id_bit);

	buf_size = kdrv_ipp_builtin_get_reg_num(KDRV_IPP_BUILTIN_ENG_MAX) * 2 * 4;
	p_ctl->p_dtsi_buf = kdrv_ipp_builtin_plat_malloc(buf_size);
	if (p_ctl->p_dtsi_buf == NULL) {
		DBG_ERR("kmalloc dtsi buf(0x%.8x) failed\r\n", (unsigned int)buf_size);
		return -1;
	}
	KDRV_IPP_BUILTIN_DUMP("dtsi buffer 0x%.8x, size 0x%.8x\r\n", (unsigned int)p_ctl->p_dtsi_buf, (unsigned int)buf_size);

	/* init engine io addr */
	for (i = 0; i < KDRV_IPP_BUILTIN_ENG_MAX; i++) {
		p_ctl->reg_base[i] = kdrv_ipp_builtin_plat_ioremap_nocache(reg_base_info[i].addr, reg_base_info[i].size);
		if (p_ctl->reg_base[i] == NULL) {
			DBG_ERR("ioremap eng %d failed\r\n", i);
			return -1;
		}
	}

	/* handle init */
	if(kdrv_ipp_builtin_plat_read_dtsi_array("/fastboot/ipp/ctl", "num", (UINT32 *)&p_ctl->hdl_num, 1)){
		DBG_ERR("read dtsi num failed\r\n");
	}

	/* backward compatible flow select */
	if (p_ctl->hdl_num == 0) {
		p_ctl->hdl_num = 1;
		p_ctl->dtsi_ver = 0;
	} else {
		p_ctl->dtsi_ver = 1;
	}
	p_ctl->p_hdl = kdrv_ipp_builtin_plat_malloc(sizeof(KDRV_IPP_BUILTIN_HDL) * p_ctl->hdl_num);
	if (p_ctl->p_hdl == NULL) {
		DBG_ERR("kmalloc ipp handle buf(0x%.8x) failed\r\n", (unsigned int)(sizeof(KDRV_IPP_BUILTIN_HDL) * p_ctl->hdl_num));
		return -1;
	}
	KDRV_IPP_BUILTIN_DUMP("dtsi version %d, hdl_buffer 0x%.8x, num %d, size 0x%.8x\r\n",
		(int)p_ctl->dtsi_ver, (unsigned int)p_ctl->p_hdl, (int)p_ctl->hdl_num, (unsigned int)(sizeof(KDRV_IPP_BUILTIN_HDL) * p_ctl->hdl_num));

	/* init default handle by dtsi */
	for (k = 0; k < p_ctl->hdl_num; k++) {
		p_hdl = &p_ctl->p_hdl[k];
		if (kdrv_ipp_builtin_dtsi_ver() == 0) {
			snprintf(p_hdl->node, 63, "/fastboot/ipp-config");
			p_hdl->name ="builtin_vprc1";
			p_hdl->flow = KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW;
			p_hdl->isp_id = 0;
		} else if (kdrv_ipp_builtin_dtsi_ver() == 1) {
			snprintf(p_hdl->node, 63, "/fastboot/ipp/config-%d", (int)k);

			if(kdrv_ipp_builtin_plat_read_dtsi_string(p_hdl->node, "hdl_name", &p_hdl->name)){
				DBG_ERR("read dtsi %s failed\r\n", "hdl_name");
				return -1;
			}

			if(kdrv_ipp_builtin_plat_read_dtsi_array(p_hdl->node, "flow", (UINT32 *)&p_hdl->flow, 1)){
				DBG_ERR("read dtsi %s failed\r\n", "flow");
				return -1;
			}

			if(kdrv_ipp_builtin_plat_read_dtsi_array(p_hdl->node, "isp_id", (UINT32 *)&p_hdl->isp_id, 1)){
				DBG_ERR("read dtsi %s failed\r\n", "isp_id");
				return -1;
			}

		} else {
			DBG_ERR("Unknown dtsi version %d\r\n", kdrv_ipp_builtin_dtsi_ver());
			return -1;
		}
	}

	/* init handle ctrl blk by init_info */
	for (k = 0; k < info->hdl_num; k++) {
		/* find matched handle from pool */
		for (i = 0; i < p_ctl->hdl_num; i++) {
			if (strcmp(info->hdl_info[k].name, p_ctl->p_hdl[i].name) == 0) {
				break;
			}
		}

		if (i < p_ctl->hdl_num) {
			p_hdl = &p_ctl->p_hdl[i];
			p_hdl->src_sie_id_bit = info->hdl_info[k].src_sie_id_bit;
			p_hdl->vprc_ctrl.addr = info->hdl_info[k].ctrl_blk_addr;
			p_hdl->vprc_ctrl.size = info->hdl_info[k].ctrl_blk_size;
			p_hdl->vprc_ctrl.phy_addr = nvtmpp_sys_va2pa(p_hdl->vprc_ctrl.addr);
			if (p_hdl->vprc_ctrl.phy_addr == 0) {
				DBG_ERR("handle %s, blk_addr 0x%.8x, get phy_blk_addr 0\r\n", p_hdl->name, (unsigned int)p_hdl->vprc_ctrl.addr);
				return -1;
			}

			p_hdl->vprc_shdr_blk.addr = info->hdl_info[k].shdr_ring_buf_addr;
			p_hdl->vprc_shdr_blk.size = info->hdl_info[k].shdr_ring_buf_size;
			KDRV_IPP_BUILTIN_DUMP("ctrl: 0x%.8x, 0x%.8x;\r\n", p_hdl->vprc_ctrl.addr, p_hdl->vprc_ctrl.size);
			kdrv_ipp_builtin_init_handle(p_hdl);
		} else {
			DBG_ERR("find no handle %s\r\n", info->hdl_info[k].name);
		}
	}

	/* config engine clock */
	if (kdrv_ipp_builtin_plat_init_clk() != 0) {
		return -1;
	}

	/* sram ctrl */
	nvt_disable_sram_shutdown(IFE_SD);
	nvt_disable_sram_shutdown(DCE_SD);
	nvt_disable_sram_shutdown(IPE_SD);
	nvt_disable_sram_shutdown(IME_SD);
	nvt_disable_sram_shutdown(IFE2_SD);

	/* set ime builtin cb */
	ime_chg_builtin_isr_cb(kdrv_ipp_builtin_ime_isr);

	/* soft reset engine */
	kdrv_ipp_builtin_soft_reset_eng();

	p_hdl = &p_ctl->p_hdl[0];
	if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
		/* trigger engine start if direct mode */
		kdrv_ipp_builtin_cfg_eng_all(p_hdl);
		kdrv_ipp_builtin_trig_eng_start(p_hdl);
	} else {
		/* init resource for d2d flow */
		p_ctl->job_pool.blk_size = sizeof(KDRV_IPP_BUILTIN_JOB);
		p_ctl->job_pool.blk_num = KDRV_IPP_BUILTIN_QUE_DEPTH;
		p_ctl->job_pool.total_size = p_ctl->job_pool.blk_num * p_ctl->job_pool.blk_size;
		p_ctl->job_pool.start_addr = (UINT32)kdrv_ipp_builtin_plat_malloc(p_ctl->job_pool.total_size);
		if (p_ctl->job_pool.start_addr == 0) {
			DBG_ERR("alloc job pool failed, want_size 0x%.8x\r\n", (unsigned int)p_ctl->job_pool.total_size);
			return E_NOMEM;
		}
		KDRV_IPP_BUILTIN_DUMP("job buffer 0x%.8x, num %d, size 0x%.8x\r\n",
			p_ctl->job_pool.start_addr, KDRV_IPP_BUILTIN_QUE_DEPTH, p_ctl->job_pool.total_size);
		vk_spin_lock_init(&p_ctl->job_pool.lock);
		VOS_INIT_LIST_HEAD(&p_ctl->job_pool.free_list_root);
		VOS_INIT_LIST_HEAD(&p_ctl->job_pool.used_list_root);
		for (i = 0; i < p_ctl->job_pool.blk_num; i++) {
			KDRV_IPP_BUILTIN_JOB *p_job;

			p_job = (KDRV_IPP_BUILTIN_JOB *)(p_ctl->job_pool.start_addr + (p_ctl->job_pool.blk_size * i));
			VOS_INIT_LIST_HEAD(&p_job->pool_list);
			VOS_INIT_LIST_HEAD(&p_job->proc_list);
			vos_list_add_tail(&p_job->pool_list, &p_ctl->job_pool.free_list_root);
		}

		VOS_INIT_LIST_HEAD(&p_ctl->job_list_root);
		vk_spin_lock_init(&p_ctl->job_list_lock);
		vos_flag_create(&p_ctl->proc_tsk_flg_id, NULL, "builtin_ipp_flg");

		THREAD_CREATE(p_ctl->proc_tsk_id, kdrv_ipp_builtin_tsk, 0, "builtin_ipp_tsk");
		if (p_ctl->proc_tsk_id == 0) {
			DBG_ERR("open builtin ipp task failed\r\n");
			return E_SYS;
		}
		THREAD_SET_PRIORITY(p_ctl->proc_tsk_id, KDRV_IPP_BUILTIN_TSK_PRIORITY);
		THREAD_RESUME(p_ctl->proc_tsk_id);

		/* register sie callback */
		sie_fb_reg_buf_out_cb(kdrv_ipp_builtin_sie_cb);
	}

	p_ctl->is_init = TRUE;

	return E_OK;
}

INT32 kdrv_ipp_builtin_exit(void)
{
	FLGPTN wait_flg;
	KDRV_IPP_BUILTIN_CTL* p_ctl;
	KDRV_IPP_BUILTIN_HDL* p_hdl;
	KDRV_IPP_BUILTIN_FMD_CB_INFO info = {0};
	UINT32 i;

	if (kdrv_builtin_is_fastboot() == FALSE) {
		return E_NOSPT;
	}

	p_ctl = &g_kdrv_ipp_builtin_ctl;
	if (!p_ctl->is_init) {
		return E_OK;
	}
	p_hdl = p_ctl->p_trig_hdl;
	if (p_hdl && p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
		/* direct mode */
		/* frame end callback flow, push last output buffer */
		/* add frame start cnt, last fst is switch to normal isr cb */
		if (!p_hdl->out_buf_release_cnt) {
			DBG_ERR("direct mode exit but switch_to_normal = FALSE\r\n");
			return E_SYS;
		}

		p_hdl->fs_cnt++;
		p_hdl->fmd_cnt++;
		info.name = p_hdl->name;
		info.release_flg = (p_hdl->out_buf_release_cnt > 0) ? 1 : 0;
		for (i = 0; i < KDRV_IPP_BUILTIN_PATH_ID_MAX; i++) {
			info.out_img[i] = kdrv_ipp_builtin_get_path_info(info.name, i);
			/* lock 3dnr ref path buffer */
			if ((p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_3DNR) &&
				(p_hdl->_3dnr_ref_path == i) &&
				(info.out_img[i].enable) &&
				(info.out_img[i].addr[0])) {
				KDRV_IPP_BUILTIN_DUMP("FASTBOOT LOCK 3DNR REF 0x%.8x\r\n", info.out_img[i].addr[0]);
				nvtmpp_lock_fastboot_blk(info.out_img[i].addr[0]);
			}
		}
		for (i = 0; i < KDRV_IPP_BUILTIN_FMD_CB_NUM; i++) {
			if (p_ctl->fmd_cb[i]) {
				p_ctl->fmd_cb[i](&info, 0);
			}
		}

		/* release blk when release_flg == 1 */
		if (info.release_flg) {
			for (i = 0; i < KDRV_IPP_BUILTIN_PATH_ID_MAX; i++) {
				if (info.out_img[i].addr[0]) {
					nvtmpp_unlock_fastboot_blk(info.out_img[i].addr[0]);
					KDRV_IPP_BUILTIN_DUMP("release path%d buf 0x%.8x\r\n", (int)i, (unsigned int)info.out_img[i].addr[0]);
				}
			}
		}

		/* debug log */
		kdrv_ipp_builtin_debug_set_ts(KDRV_IPP_BUILTIN_DBG_TS_EXIT, &p_hdl->dbg);
	} else {
		/* d2d mode, wait all hdl receive last frame and finish job */
		vos_flag_wait(&wait_flg, p_ctl->proc_tsk_flg_id, (KDRV_IPP_BUILTIN_TSK_EXIT_END | KDRV_IPP_BUILTIN_TSK_FASTBOOT_DONE), TWF_ANDW);

		/* destroy flg */
		vos_flag_destroy(p_ctl->proc_tsk_flg_id);

		/* flush job, should have no job remain */
		kdrv_ipp_builtin_flush_job();

		/* release job pool */
		kdrv_ipp_builtin_plat_free((void *)p_ctl->job_pool.start_addr);

		/* debug log */
		for (i = 0; i < p_ctl->hdl_num; i++) {
			p_hdl = &p_ctl->p_hdl[i];
			kdrv_ipp_builtin_debug_set_ts(KDRV_IPP_BUILTIN_DBG_TS_EXIT, &p_hdl->dbg);
		}
	}

	/* unmap register */
	for (i = 0; i < KDRV_IPP_BUILTIN_ENG_MAX; i++) {
		if (p_ctl->reg_base[i] != NULL) {
			kdrv_ipp_builtin_plat_iounmap(p_ctl->reg_base[i]);
			p_ctl->reg_base[i] = NULL;
		}
	}
	/* release buffer */
	p_ctl->p_trig_hdl = NULL;
	kdrv_ipp_builtin_plat_free(p_ctl->p_dtsi_buf);
	p_ctl->p_dtsi_buf = NULL;
	//kdrv_ipp_builtin_plat_free(p_ctl->p_hdl);

	p_ctl->is_init = FALSE;
	KDRV_IPP_BUILTIN_DUMP("builtin ipp exit\r\n");

	return E_OK;
}

INT32 kdrv_ipp_builtin_reg_fmd_cb(KDRV_IPP_BUILTIN_FMD_CB fp)
{
	UINT32 i;

	for (i = 0; i < KDRV_IPP_BUILTIN_FMD_CB_NUM; i++) {
		if (g_kdrv_ipp_builtin_ctl.fmd_cb[i] == NULL) {
			g_kdrv_ipp_builtin_ctl.fmd_cb[i] = fp;
			KDRV_IPP_BUILTIN_DUMP("fmd_cb[%d] registered for 0x%.8x\r\n", (int)i, (unsigned int)fp);
			return E_OK;
		}
	}
	DBG_ERR("all fmd_cb have been registered\r\n");

	return E_SYS;
}

INT32 kdrv_ipp_builtin_reg_isp_cb(KDRV_IPP_BUILTIN_ISP_CB fp)
{
	g_kdrv_ipp_isp_cb = fp;
	return E_OK;
}

KDRV_IPP_BUILTIN_IMG_INFO kdrv_ipp_builtin_get_path_info(const CHAR *name, KDRV_IPP_BUILTIN_PATH_ID pid)
{
	UINT32 buf_idx;
	KDRV_IPP_BUILTIN_IMG_INFO img = {0};
	KDRV_IPP_BUILTIN_IMG_INFO *p_img = &img;
	KDRV_IPP_BUILTIN_HDL *p_hdl;
	KDRV_IPP_BUILTIN_JOB *p_job;
	UINT32 i;

	p_hdl = NULL;
	for (i = 0; i < g_kdrv_ipp_builtin_ctl.hdl_num; i++) {
		if (strcmp(name, g_kdrv_ipp_builtin_ctl.p_hdl[i].name) == 0) {
			p_hdl = &g_kdrv_ipp_builtin_ctl.p_hdl[i];
			break;
		}
	}
	if (p_hdl == NULL) {
		DBG_ERR("find no hdl %s\r\n", name);
		return img;
	}

	*p_img = p_hdl->path_info[pid];
	buf_idx = (p_hdl->fs_cnt + 1) % 2;
	if (pid < 5) {
		if (p_img->enable) {
			/* path1/2/3/4/5 address */
			if (p_hdl->vprc_blk[pid][buf_idx].addr) {
				p_img->addr[0] = p_hdl->vprc_blk[pid][buf_idx].addr;
				p_img->addr[1] = p_hdl->vprc_blk[pid][buf_idx].addr + (p_img->loff[0] * p_img->size.h);
				p_img->phyaddr[0] = nvtmpp_sys_va2pa(p_img->addr[0]);
				p_img->phyaddr[1] = nvtmpp_sys_va2pa(p_img->addr[1]);
			} else {
				p_img->enable = DISABLE;
			}
		}
	} else if (pid == 5) {
		/* todo: dce cfa out */
		p_img->enable = DISABLE;
	}

	if (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW) {
		p_img->timestamp = p_hdl->fs_timestamp;
	} else if (p_hdl->p_cur_job) {
		p_job = (KDRV_IPP_BUILTIN_JOB *)p_hdl->p_cur_job;
		p_img->timestamp = p_job->timestamp;
	}

	return img;
}

KDRV_IPP_BUILTIN_CTL_INFO* kdrv_ipp_builtin_get_ctl_info(CHAR *name)
{
	UINT32 i;

	if (g_kdrv_ipp_builtin_ctl.p_hdl) {
		/* old version only support one direct mode handle */
		if (kdrv_ipp_builtin_dtsi_ver() == 0) {
			return &g_kdrv_ipp_builtin_ctl.p_hdl[0].last_ctl_info;
		}

		/* new version, find compatible hdl_name then return
			only support get phy out once
		*/
		for (i = 0; i < g_kdrv_ipp_builtin_ctl.hdl_num; i++) {
			if (strcmp(name, g_kdrv_ipp_builtin_ctl.p_hdl[i].name) == 0) {
				if (g_kdrv_ipp_builtin_ctl.p_hdl[i].get_phy_out_cnt == 0) {
					g_kdrv_ipp_builtin_ctl.p_hdl[i].get_phy_out_cnt = 1;
					return &g_kdrv_ipp_builtin_ctl.p_hdl[i].last_ctl_info;
				} else {
					return NULL;
				}
			}
		}
		DBG_ERR("find no builtin_hdl with name %s\r\n", name);
	} else {
		DBG_ERR("builtin handle already released\r\n");
	}

	return NULL;
}

BOOL kdrv_ipp_builtin_get_eng_enable(KDRV_IPP_BUILTIN_ENG eng)
{
	KDRV_IPP_BUILTIN_HDL *p_hdl = g_kdrv_ipp_builtin_ctl.p_trig_hdl;

	if (p_hdl && (p_hdl->flow == KDRV_IPP_BUILTIN_FLOW_DIRECT_RAW)) {
		if (eng == KDRV_IPP_BUILTIN_IFE2) {
			return p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_YUV_SUBOUT;
		}
		return TRUE;
	}

	return FALSE;
}

KDRV_IPP_BUILTIN_ISP_INFO* kdrv_ipp_builtin_get_isp_info(CHAR *name)
{
	UINT32 i;

	if (g_kdrv_ipp_builtin_ctl.p_hdl) {
		/* old version only support one direct mode handle */
		if (kdrv_ipp_builtin_dtsi_ver() == 0) {
			return &g_kdrv_ipp_builtin_ctl.p_hdl[0].isp_info;
		}

		/* new version, find compatible hdl_name then return
			only support get isp info once
		*/
		for (i = 0; i < g_kdrv_ipp_builtin_ctl.hdl_num; i++) {
			if (strcmp(name, g_kdrv_ipp_builtin_ctl.p_hdl[i].name) == 0) {
				if (g_kdrv_ipp_builtin_ctl.p_hdl[i].get_isp_info_cnt== 0) {
					g_kdrv_ipp_builtin_ctl.p_hdl[i].get_isp_info_cnt = 1;
					return &g_kdrv_ipp_builtin_ctl.p_hdl[i].isp_info;
				} else {
					return NULL;
				}
			}
		}
		DBG_ERR("find no builtin_hdl with name %s\r\n", name);
	} else {
		DBG_ERR("builtin handle already released\r\n");
	}

	return NULL;
}

#if 0
#endif
static KDRV_IPP_BUILTIN_DTSI_CB g_kdrv_ipp_builtin_dtsi_cb = {NULL, NULL};

void kdrv_ipp_set_reg(UINT32 eng, UINT32 base, UINT32 ofs, UINT32 val)
{
	iowrite32(val, (void *)(base + ofs));
}

int kdrv_ipp_builtin_reg_dump(int fd, char *pre_fix)
{
	//int		fd, len;
	int		len;
	char	tmp_buf[128];
	//char	pre_fix[4] = "\t\t";

	UINT32 eng_idx = 0, eng_reg_idx = 0;
	UINT32 reg_val = 0;
	void __iomem *vaddr_base;

	CHAR *dtsi_reg_num_tag[KDRV_IPP_BUILTIN_ENG_MAX] = {
		"ife_reg_num",
		"dce_reg_num",
		"ipe_reg_num",
		"ime_reg_num",
		"ife2_reg_num",
	};


	CHAR *dtsi_reg_cfg_tag[KDRV_IPP_BUILTIN_ENG_MAX] = {
		"ife_reg_cfg",
		"dce_reg_cfg",
		"ipe_reg_cfg",
		"ime_reg_cfg",
		"ife2_reg_cfg",
	};

	UINT32 eng_reg_base[KDRV_IPP_BUILTIN_ENG_MAX] = {
		0xF0C70000,	/* IFE */
		0xF0C20000,	/* DCE */
		0xF0C30000,	/* IPE */
		0xF0C40000,	/* IME */
		0xF0D00000,	/* IFE2 */
	};

	//fd = vos_file_open(tmp_buf, O_CREAT|O_WRONLY|O_SYNC, 0);
	//if ((VOS_FILE)(-1) == fd) {
	//	printk("open %s failure\r\n", tmp_buf);
	//	return FALSE;
	//}

	if (fd == (VOS_FILE)(-1)) {
		printk("file handle failure...NULL\r\n");
		return FALSE;
	}

	// begin
	len = snprintf(tmp_buf, sizeof(tmp_buf), "%sregister {\r\n", pre_fix);
	vos_file_write(fd, (void *)tmp_buf, len);

	for(eng_idx = 0; eng_idx < KDRV_IPP_BUILTIN_ENG_MAX; eng_idx++) {
		len = snprintf(tmp_buf, sizeof(tmp_buf), "%s  %s = <%d>;\r\n", pre_fix, dtsi_reg_num_tag[eng_idx], kdrv_ipp_builtin_get_reg_num(eng_idx));
		vos_file_write(fd, (void *)tmp_buf, len);
	}
	len = snprintf(tmp_buf, sizeof(tmp_buf), "\r\n");
	vos_file_write(fd, (void *)tmp_buf, len);


	for(eng_idx = 0; eng_idx < KDRV_IPP_BUILTIN_ENG_MAX; eng_idx++)	{
			len = snprintf(tmp_buf, sizeof(tmp_buf), "%s  %s = \r\n", pre_fix, dtsi_reg_cfg_tag[eng_idx]);
			vos_file_write(fd, (void *)tmp_buf, len);

			vaddr_base = kdrv_ipp_builtin_plat_ioremap_nocache(eng_reg_base[eng_idx], ((kdrv_ipp_builtin_get_reg_num(eng_idx)+1)<<2));

			for(eng_reg_idx = 0; eng_reg_idx < kdrv_ipp_builtin_get_reg_num(eng_idx); eng_reg_idx++) {

				reg_val = *(volatile UINT32*)(vaddr_base + ((eng_reg_idx+1) << 2));

				if (eng_reg_idx == kdrv_ipp_builtin_get_reg_num(eng_idx)-1) {
					len = snprintf(tmp_buf, sizeof(tmp_buf), "%s    <0x%04x 0x%08x>;\r\n", pre_fix, ((eng_reg_idx+1) << 2), reg_val);
				} else {
					len = snprintf(tmp_buf, sizeof(tmp_buf), "%s    <0x%04x 0x%08x>,\r\n", pre_fix, ((eng_reg_idx+1) << 2), reg_val);
				}

				vos_file_write(fd, (void *)tmp_buf, len);
			}
			len = snprintf(tmp_buf, sizeof(tmp_buf), "\r\n");
			vos_file_write(fd, (void *)tmp_buf, len);
			kdrv_ipp_builtin_plat_iounmap((void *)vaddr_base);
	}

	// end
	len = snprintf(tmp_buf, sizeof(tmp_buf), "%s};\r\n", pre_fix);
	vos_file_write(fd, (void *)tmp_buf, len);

	//vos_file_close(fd);
	return TRUE;
}

void kdrv_ipp_builtin_get_reg_dtsi(UINT32 **dtsi_buffer)
{
	UINT32 *p_array;
	UINT32 eng_idx = 0, eng_reg_idx = 0;
	UINT32 cur_idx = 0;
	void __iomem *vaddr_base;

	UINT32 eng_reg_base[KDRV_IPP_BUILTIN_ENG_MAX] = {
		0xF0C70000,	/* IFE */
		0xF0C20000,	/* DCE */
		0xF0C30000,	/* IPE */
		0xF0C40000,	/* IME */
		0xF0D00000,	/* IFE2 */
	};

	if (dtsi_buffer == NULL)
		return ;

	for (eng_idx = 0; eng_idx < KDRV_IPP_BUILTIN_ENG_MAX; eng_idx++) {
		vaddr_base = kdrv_ipp_builtin_plat_ioremap_nocache(eng_reg_base[eng_idx], ((kdrv_ipp_builtin_get_reg_num(eng_idx)+1)<<2));
		p_array = dtsi_buffer[eng_idx];
		cur_idx = 0;
		for(eng_reg_idx = 0; eng_reg_idx < kdrv_ipp_builtin_get_reg_num(eng_idx); eng_reg_idx++) {
			p_array[cur_idx] = ((eng_reg_idx + 1) << 2);
			p_array[cur_idx + 1] = *(volatile UINT32*)(vaddr_base + ((eng_reg_idx+1) << 2));
			cur_idx += 2;
		}
		kdrv_ipp_builtin_plat_iounmap((void *)vaddr_base);
	}
}

UINT32 kdrv_ipp_builtin_get_reg_num(KDRV_IPP_BUILTIN_ENG eng)
{
	UINT32 eng_reg_nums[KDRV_IPP_BUILTIN_ENG_MAX] = {
		470 - 1,	/* IFE */
		394 - 1,	/* DCE */
		576 - 1,	/* IPE */
		656 - 1,	/* IME */
		44 - 1,	   /* IFE2 */
	};

	if (eng < KDRV_IPP_BUILTIN_ENG_MAX) {
		return eng_reg_nums[eng];
	}

	return eng_reg_nums[KDRV_IPP_BUILTIN_IME];
}

void kdrv_ipp_builtin_reg_dtsi_cb(KDRV_IPP_BUILTIN_DTSI_CB cb)
{
	g_kdrv_ipp_builtin_dtsi_cb = cb;
}

void kdrv_ipp_builtin_get_hdal_hdl_list(UINT32 *hdl_list, UINT32 list_size)
{
	if (g_kdrv_ipp_builtin_dtsi_cb.get_hdl_list) {
		g_kdrv_ipp_builtin_dtsi_cb.get_hdl_list(hdl_list, list_size);
	}
}

void kdrv_ipp_builtin_get_hdal_reg_dtsi(UINT32 hdl, UINT32 cnt, KDRV_IPP_BUILTIN_FDT_GET_INFO* info)
{
	if (g_kdrv_ipp_builtin_dtsi_cb.get_reg_dtsi) {
		g_kdrv_ipp_builtin_dtsi_cb.get_reg_dtsi(hdl, cnt, info);
	}
}

int kdrv_ipp_builtin_frc_dump(char *frc_node, int fd, char *pre_fix)
{
	UINT32 tmp[KDRV_IPP_BUILTIN_PATH_ID_MAX] = {0};
	char tmp_buf[128] = "";
	int len;
	int rt;

	len = snprintf(tmp_buf, sizeof(tmp_buf), "%sfrm-rate-ctrl {\r\n", pre_fix);
	vos_file_write(fd, (void *)tmp_buf, len);

	rt = kdrv_ipp_builtin_plat_read_dtsi_array(frc_node, "skip", (UINT32 *)tmp, KDRV_IPP_BUILTIN_PATH_ID_MAX);
	if (rt == 0) {
		len = snprintf(tmp_buf, sizeof(tmp_buf), "%s\tskip = <%d %d %d %d %d %d>;\r\n", pre_fix,
						tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
	} else {
		len = snprintf(tmp_buf, sizeof(tmp_buf), "%s\tskip = <%d %d %d %d %d %d>;\r\n", pre_fix, 0, 0, 0, 0, 0, 0);
	}
	vos_file_write(fd, (void *)tmp_buf, len);

	rt = kdrv_ipp_builtin_plat_read_dtsi_array(frc_node, "src", (UINT32 *)tmp, KDRV_IPP_BUILTIN_PATH_ID_MAX);
	if (rt == 0) {
		len = snprintf(tmp_buf, sizeof(tmp_buf), "%s\tsrc = <%d %d %d %d %d %d>;\r\n", pre_fix,
						tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
	} else {
		len = snprintf(tmp_buf, sizeof(tmp_buf), "%s\tsrc = <%d %d %d %d %d %d>;\r\n", pre_fix, 30, 30, 30, 30, 30, 30);
	}
	vos_file_write(fd, (void *)tmp_buf, len);

	rt = kdrv_ipp_builtin_plat_read_dtsi_array(frc_node, "dst", (UINT32 *)tmp, KDRV_IPP_BUILTIN_PATH_ID_MAX);
	if (rt == 0) {
		len = snprintf(tmp_buf, sizeof(tmp_buf), "%s\tdst = <%d %d %d %d %d %d>;\r\n", pre_fix,
						tmp[0], tmp[1], tmp[2], tmp[3], tmp[4], tmp[5]);
	} else {
		len = snprintf(tmp_buf, sizeof(tmp_buf), "%s\tdst = <%d %d %d %d %d %d>;\r\n", pre_fix, 30, 30, 30, 30, 30, 30);
	}
	vos_file_write(fd, (void *)tmp_buf, len);

	len = snprintf(tmp_buf, sizeof(tmp_buf), "%s};\r\n", pre_fix);
	vos_file_write(fd, (void *)tmp_buf, len);

	return E_OK;
}

#if 0
#endif

void kdrv_ipp_builtin_debug_set_ts(UINT32 type, KDRV_IPP_BUILTIN_DBG_INFO *p_dbg)
{
	unsigned long loc_flg;
	UINT8 idx;

	vk_spin_lock_irqsave(&p_dbg->lock, loc_flg);
	idx = p_dbg->ts_log_cnt % KDRV_IPP_BUILTIN_DBG_TS_NUM;
	p_dbg->ts_log_cnt++;
	vk_spin_unlock_irqrestore(&p_dbg->lock, loc_flg);

	p_dbg->ts[idx].type = type;
	vos_perf_mark(&p_dbg->ts[idx].timestamp);
}

void kdrv_ipp_builtin_debug_dump(void)
{
	CHAR *ts_type_str[KDRV_IPP_BUILTIN_DBG_TS_TYPE_MAX] = {"fst", "fmd", "push_in", "exit"};
	KDRV_IPP_BUILTIN_CTL* p_ctl;
	KDRV_IPP_BUILTIN_HDL* p_hdl;
	UINT32 i, j;

	p_ctl = &g_kdrv_ipp_builtin_ctl;
	for (i = 0; i < p_ctl->hdl_num; i++) {
		p_hdl = &p_ctl->p_hdl[i];

		DBG_DUMP("--------------------- ch%d ---------------------\n", (int)i);
		DBG_DUMP("flow = %d\n", (int)p_hdl->flow);
		DBG_DUMP("sensor frm cnt=%d\n", (int)p_hdl->push_in_cnt);
		DBG_DUMP("input  frm cnt=%d\n", (int)p_hdl->fs_cnt);
		DBG_DUMP("output frm cnt=%d\n", (int)p_hdl->fmd_cnt);
		//DBG_DUMP("input  w=%d, h=%d\n", info[i].input_w, info[i].input_h );
		//DBG_DUMP("apply fail cnt=%d\n", info[i].apply_fail_cnt);
		//DBG_DUMP("intr drop cnt =%d\n", info[i].intr_drop_cnt);

		DBG_DUMP("direct shdr ring buf addr=0x%x(0x%x)\r\n", (unsigned int)p_hdl->vprc_shdr_blk.addr, (unsigned int)p_hdl->vprc_shdr_blk.phy_addr);

		if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_YUV_SUBOUT) {
			for (i = 0; i < KDRV_IPP_BUILTIN_BUF_LCA_NUM; i++) {
				DBG_DUMP("lca[%d]: 0x%.8x(0x%.8x), size 0x%.8x\r\n", (int)i,
					(unsigned int)p_hdl->pri_buf.lca[i].addr, (unsigned int)p_hdl->pri_buf.lca[i].phy_addr, (unsigned int)p_hdl->pri_buf.lca[i].size);
			}
		}

		if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_3DNR) {
			for (i = 0; i < KDRV_IPP_BUILTIN_BUF_3DNR_NUM; i++) {
				DBG_DUMP("3dnr_mv[%d]: 0x%.8x(0x%.8x), size 0x%.8x\r\n", (int)i,
					(unsigned int)p_hdl->pri_buf._3dnr_mv[i].addr, (unsigned int)p_hdl->pri_buf._3dnr_mv[i].phy_addr, (unsigned int)p_hdl->pri_buf._3dnr_mv[i].size);
			}

			for (i = 0; i < KDRV_IPP_BUILTIN_BUF_3DNR_NUM; i++) {
				DBG_DUMP("3dnr_ms[%d]: 0x%.8x(0x%.8x), size 0x%.8x\r\n", (int)i,
					(unsigned int)p_hdl->pri_buf._3dnr_ms[i].addr, (unsigned int)p_hdl->pri_buf._3dnr_ms[i].phy_addr, (unsigned int)p_hdl->pri_buf._3dnr_ms[i].size);
			}

			for (i = 0; i < KDRV_IPP_BUILTIN_BUF_3DNR_NUM; i++) {
				DBG_DUMP("3dnr_ms_roi[%d]: 0x%.8x(0x%.8x), size 0x%.8x\r\n", (int)i,
					(unsigned int)p_hdl->pri_buf._3dnr_ms_roi[i].addr, (unsigned int)p_hdl->pri_buf._3dnr_ms_roi[i].phy_addr, (unsigned int)p_hdl->pri_buf._3dnr_ms_roi[i].size);
			}

			if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_3DNR_STA) {
				for (i = 0; i < KDRV_IPP_BUILTIN_BUF_3DNR_STA_NUM; i++) {
					DBG_DUMP("3dnr_sta[%d]: 0x%.8x(0x%.8x), size 0x%.8x\r\n", (int)i,
						(unsigned int)p_hdl->pri_buf._3dnr_sta[i].addr, (unsigned int)p_hdl->pri_buf._3dnr_sta[i].phy_addr, (unsigned int)p_hdl->pri_buf._3dnr_sta[i].size);
				}
			}
		}

		for (i = 0; i < KDRV_IPP_BUILTIN_BUF_DFG_NUM; i++) {
			DBG_DUMP("defog[%d]: 0x%.8x(0x%.8x), size 0x%.8x\r\n", (int)i,
				(unsigned int)p_hdl->pri_buf.defog[i].addr, (unsigned int)p_hdl->pri_buf.defog[i].phy_addr, (unsigned int)p_hdl->pri_buf.defog[i].size);
		}

		if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_PM_PIXELIZTION) {
			for (i = 0; i < KDRV_IPP_BUILTIN_BUF_PM_NUM; i++) {
				DBG_DUMP("pm[%d]: 0x%.8x(0x%.8x), size 0x%.8x\r\n", (int)i,
					(unsigned int)p_hdl->pri_buf.pm[i].addr, (unsigned int)p_hdl->pri_buf.pm[i].phy_addr, (unsigned int)p_hdl->pri_buf.pm[i].size);
			}
		}

		if (p_hdl->func_en & KDRV_IPP_BUILTIN_FUNC_WDR) {
			for (i = 0; i < KDRV_IPP_BUILTIN_BUF_WDR_NUM; i++) {
				DBG_DUMP("wdr[%d]: 0x%.8x(0x%.8x), size 0x%.8x\r\n", (int)i,
					(unsigned int)p_hdl->pri_buf.wdr[i].addr, (unsigned int)p_hdl->pri_buf.wdr[i].phy_addr, (unsigned int)p_hdl->pri_buf.wdr[i].size);
			}
		}

		for (j = 0; j < KDRV_IPP_BUILTIN_PATH_ID_MAX; j++){
			DBG_DUMP("=============== PATH %d ===============\n", (int)j);
			DBG_DUMP("output en %d, w=%d, h=%d, fmt=0x%x\r\n",
				(int)p_hdl->path_info[j].enable, (int)p_hdl->path_info[j].size.w, (int)p_hdl->path_info[j].size.h, (unsigned int)p_hdl->path_info[j].fmt);
			DBG_DUMP("output y buf addr=0x%x\r\n", p_hdl->last_ctl_info.path_addr_y[j]);
			DBG_DUMP("output u buf addr=0x%x\r\n", p_hdl->last_ctl_info.path_addr_u[j]);
			DBG_DUMP("output v buf addr=0x%x\r\n", p_hdl->last_ctl_info.path_addr_v[j]);
		}

		for (j = 0; j < KDRV_IPP_BUILTIN_DBG_TS_NUM; j++) {
			UINT8 idx = (p_hdl->dbg.ts_log_cnt + j) % KDRV_IPP_BUILTIN_DBG_TS_NUM;

			DBG_DUMP("%8s, timestamp %d\r\n", ts_type_str[p_hdl->dbg.ts[idx].type], (int)p_hdl->dbg.ts[idx].timestamp);
		}
	}
}

void kdrv_ipp_builtin_debug(UINT32 item, void *data)
{
	if (item == KDRV_IPP_BUILTIN_DEBUG_DUMP) {
		kdrv_ipp_builtin_debug_dump();
	}
}

#else	/* #if defined (__KERNEL__) */

INT32 kdrv_ipp_builtin_init(KDRV_IPP_BUILTIN_INIT_INFO *info)
{
	return E_OK;
}

void kdrv_ipp_set_reg(UINT32 eng, UINT32 base, UINT32 ofs, UINT32 val)
{

}

int kdrv_ipp_builtin_reg_dump(int fd, char *pre_fix)
{
	return 0;
}

void kdrv_ipp_builtin_get_reg_dtsi(UINT32 **dtsi_buffer)
{

}

UINT32 kdrv_ipp_builtin_get_reg_num(KDRV_IPP_BUILTIN_ENG eng)
{
	return 0;
}

void kdrv_ipp_builtin_reg_dtsi_cb(KDRV_IPP_BUILTIN_DTSI_CB cb)
{

}

int kdrv_ipp_builtin_frc_dump(char *frc_node, int fd, char *pre_fix)
{
	return 0;
}

void kdrv_ipp_builtin_debug(UINT32 item, void *data)
{

}

#endif //__KERNEL
