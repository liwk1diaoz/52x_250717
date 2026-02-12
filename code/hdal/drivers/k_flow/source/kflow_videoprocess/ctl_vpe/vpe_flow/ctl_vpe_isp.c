#include "kflow_videoprocess/ctl_vpe_isp.h"
#include "ctl_vpe_isp_int.h"
#include "ctl_vpe_int.h"

#if CTL_VPE_MODULE_ENABLE
static CTL_VPE_ISP_HANDLE g_ctl_vpe_isp_handle[CTL_VPE_ISP_HANDLE_MAX_NUMBER];

static void ctl_vpe_isp_handle_reset(CTL_VPE_ISP_HANDLE *p_hdl)
{
	memset((void *)&p_hdl->name[0], '\0', sizeof(CHAR) * CTL_VPE_ISP_HANDLE_NAME_MAX_LENGTH);
	p_hdl->fp = NULL;
	p_hdl->evt = 0;
	p_hdl->cb_msg = CTL_VPE_ISP_CB_MSG_NONE;
	p_hdl->status = CTL_VPE_ISP_HANDLE_STATUS_FREE;
}

static CTL_VPE_ISP_HANDLE *ctl_vpe_isp_handle_alloc(void)
{
	UINT32 i;

	for (i = 0; i < CTL_VPE_ISP_HANDLE_MAX_NUMBER; i++) {
		if (g_ctl_vpe_isp_handle[i].status == CTL_VPE_ISP_HANDLE_STATUS_FREE) {
			g_ctl_vpe_isp_handle[i].status = CTL_VPE_ISP_HANDLE_STATUS_USED;
			return &g_ctl_vpe_isp_handle[i];
		}
	}

	return NULL;
}

static void ctl_vpe_isp_handle_release(CTL_VPE_ISP_HANDLE *p_hdl)
{
	ctl_vpe_isp_handle_reset(p_hdl);
}

ER ctl_vpe_isp_evt_fp_reg(CHAR *name, ISP_EVENT_FP fp, ISP_EVENT evt, CTL_VPE_ISP_CB_MSG cb_msg)
{
	CTL_VPE_ISP_HANDLE *p_hdl;

	p_hdl = ctl_vpe_isp_handle_alloc();
	if (p_hdl == NULL) {
		DBG_WRN("no free isp handle\r\n");
		return E_NOMEM;
	}

	/* strncpy may not have null terminator,
		dont copy full length incase overwrite the last null terminator */
	strncpy(p_hdl->name, name, (CTL_VPE_ISP_HANDLE_NAME_MAX_LENGTH - 1));
	p_hdl->fp = fp;
	p_hdl->evt = evt;
	p_hdl->cb_msg = cb_msg;

	return E_OK;
}

ER ctl_vpe_isp_evt_fp_unreg(CHAR *name)
{
	UINT32 i;

	for (i = 0; i < CTL_VPE_ISP_HANDLE_MAX_NUMBER; i++) {
		if (strcmp(g_ctl_vpe_isp_handle[i].name, name) == 0) {
			ctl_vpe_isp_handle_release(&g_ctl_vpe_isp_handle[i]);
		}
	}

	return E_OK;
}

ER ctl_vpe_isp_evt_fp_dbg_mode(UINT32 enable)
{
	static CTL_VPE_ISP_HANDLE tmp_hdl_buf[CTL_VPE_ISP_HANDLE_MAX_NUMBER] = {0};
	static UINT32 status = 0;
	UINT32 i;

	if (enable) {
		if (status == 0) {
			/* unregister all isp */
			status = 1;
			for (i = 0; i < CTL_VPE_ISP_HANDLE_MAX_NUMBER; i++) {
				tmp_hdl_buf[i] = g_ctl_vpe_isp_handle[i];
				ctl_vpe_isp_handle_release(&g_ctl_vpe_isp_handle[i]);
			}

			DBG_DUMP("ISP DBGMODE = ON, Remove all isp\r\n");
		} else {
			DBG_DUMP("ISP DBGMODE already ON, do nothing\r\n");
		}
	} else {
		/* restore isp */
		if (status == 1) {
			for (i = 0; i < CTL_VPE_ISP_HANDLE_MAX_NUMBER; i++) {
				g_ctl_vpe_isp_handle[i] = tmp_hdl_buf[i];
				ctl_vpe_isp_handle_reset(&tmp_hdl_buf[i]);
			}
		} else {
			DBG_DUMP("ISP DBGMODE already OFF, do nothing\r\n");
		}
	}

	return E_OK;
}

void ctl_vpe_isp_dump(int (*dump)(const char *fmt, ...))
{
	CHAR *str_status[] = {
		"FREE",
		"USED"
	};
	UINT32 i;
	CTL_VPE_ISP_HANDLE *p_hdl;

	dump("---- ctl_vpe isp interface info ----\r\n");
	dump("%32s%8s%12s%12s\r\n", "name", "status", "evt", "cb_msg");
	for (i = 0; i < CTL_VPE_ISP_HANDLE_MAX_NUMBER; i++) {
		p_hdl = &g_ctl_vpe_isp_handle[i];
		dump("%32s%8s  0x%.8x  0x%.8x\r\n", p_hdl->name, str_status[p_hdl->status], (unsigned int)p_hdl->evt, (unsigned int)p_hdl->cb_msg);
	}
}

#if 0
#endif

static INT32 ctl_vpe_isp_set_iq_param(ISP_ID id, CTL_VPE_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_VPE_HANDLE *p_hdl;
	CTL_VPE_ISP_IQ_ALL *p_iq;
	INT32 rt;

	if (data == NULL) {
		DBG_WRN("[ISP] null data\r\n");
		return E_PAR;
	}

	p_hdl = ctl_vpe_int_get_handle_by_isp_id(id);
	if (p_hdl == NULL) {
		DBG_WRN("[ISP] find no handle for isp_id %d\r\n", id);
		return E_ID;
	}

	p_iq = (CTL_VPE_ISP_IQ_ALL *)data;
	rt = CTL_VPE_E_OK;
	vk_spin_lock_irqsave(&p_hdl->isp_lock, loc_flg);

	if (p_iq->p_sharpen_param)
		p_hdl->isp_param.sharpen = *p_iq->p_sharpen_param;

	if (p_iq->p_yuv_cvt_param)
		p_hdl->isp_param.yuv_cvt_param = *p_iq->p_yuv_cvt_param;

	if (p_iq->p_dce_ctl) {
		if (p_iq->p_flip_rot_ctl) {
			if (p_iq->p_dce_ctl->enable && p_iq->p_flip_rot_ctl->flip_rot_mode != CTL_VPE_ISP_ROTATE_0) {
				DBG_ERR("plz disable dce_ctrl when flip_rot enable\r\n");
				vk_spin_unlock_irqrestore(&p_hdl->isp_lock, loc_flg);
				return E_PAR;
			}
		} else {
			if (p_iq->p_dce_ctl->enable && p_hdl->isp_param.flip_rot_ctl.flip_rot_mode != CTL_VPE_ISP_ROTATE_0) {
				DBG_ERR("plz disable dce_ctrl when flip_rot enable\r\n");
				vk_spin_unlock_irqrestore(&p_hdl->isp_lock, loc_flg);
				return E_PAR;
			}
		}
		p_hdl->isp_param.dce_ctl = *p_iq->p_dce_ctl;
	}

	if (p_iq->p_dce_gdc_param)
		p_hdl->isp_param.dce_gdc_param = *p_iq->p_dce_gdc_param;

	if (p_iq->p_dctg_param) {
		p_hdl->isp_param.dctg_param = *p_iq->p_dctg_param;
	}

	if (p_iq->p_dce_2dlut_param) {
		UINT32 lut_size;

		p_hdl->isp_param.dce_2dlut_param.out_size = p_iq->p_dce_2dlut_param->out_size;
		p_hdl->isp_param.dce_2dlut_param.xofs_i = p_iq->p_dce_2dlut_param->xofs_i;
		p_hdl->isp_param.dce_2dlut_param.xofs_f = p_iq->p_dce_2dlut_param->xofs_f;
		p_hdl->isp_param.dce_2dlut_param.yofs_i = p_iq->p_dce_2dlut_param->yofs_i;
		p_hdl->isp_param.dce_2dlut_param.yofs_f = p_iq->p_dce_2dlut_param->yofs_f;
		p_hdl->isp_param.dce_2dlut_param.lut_sz = p_iq->p_dce_2dlut_param->lut_sz;
		if (p_hdl->isp_param.dce_2dlut_param.p_lut == NULL) {
			rt = E_NOMEM;
		} else {
			switch (p_iq->p_dce_2dlut_param->lut_sz) {
			case CTL_VPE_ISP_2DLUT_SZ_9X9:
				lut_size = 12*9*4;
				break;

			case CTL_VPE_ISP_2DLUT_SZ_65X65:
				lut_size = 68*65*4;
				break;

			case CTL_VPE_ISP_2DLUT_SZ_129X129:
				lut_size = 132*129*4;
				break;

			case CTL_VPE_ISP_2DLUT_SZ_257X257:
				lut_size = 260*257*4;
				break;

			default:
				DBG_ERR("[ISP] unknown 2dlut size select %d\r\n", p_iq->p_dce_2dlut_param->lut_sz);
				rt = E_PAR;
			}

			if (rt == E_OK) {
				if (p_hdl->isp_param.dce_2dlut_param.lut_buf_sz < lut_size) {
					DBG_ERR("[ISP] lut buffer overflow, cur lut_buf = 0x%x, isp lut size = 0x%x\r\n", p_hdl->isp_param.dce_2dlut_param.lut_buf_sz, lut_size);
					rt = E_NOMEM;
				} else {
					memcpy((void *)p_hdl->isp_param.dce_2dlut_param.p_lut,
							(void *)&p_iq->p_dce_2dlut_param->lut[0],
							lut_size);
					lut_size = ALIGN_CEIL(lut_size, VOS_ALIGN_BYTES);
					vos_cpu_dcache_sync((UINT32)p_hdl->isp_param.dce_2dlut_param.p_lut, lut_size, VOS_DMA_TO_DEVICE);
				}
			}
		}
	}

	if (p_iq->p_flip_rot_ctl) {
		if (p_iq->p_dce_ctl) {
			if (p_iq->p_dce_ctl->enable && p_iq->p_flip_rot_ctl->flip_rot_mode != CTL_VPE_ISP_ROTATE_0) {
				DBG_ERR("plz disable dce_ctrl when flip_rot enable\r\n");
				vk_spin_unlock_irqrestore(&p_hdl->isp_lock, loc_flg);
				return E_PAR;
			}
		} else {
			if (p_hdl->isp_param.dce_ctl.enable && p_iq->p_flip_rot_ctl->flip_rot_mode != CTL_VPE_ISP_ROTATE_0) {
				DBG_ERR("plz disable dce_ctrl when flip_rot enable\r\n");
				vk_spin_unlock_irqrestore(&p_hdl->isp_lock, loc_flg);
				return E_PAR;
			}
		}
		p_hdl->isp_param.flip_rot_ctl = *p_iq->p_flip_rot_ctl;
	}

	vk_spin_unlock_irqrestore(&p_hdl->isp_lock, loc_flg);

	return rt;
}

static CTL_VPE_ISP_SET_FP g_ctl_vpe_isp_set_fp[CTL_VPE_ISP_ITEM_MAX] =
{
	ctl_vpe_isp_set_iq_param,
};

ER ctl_vpe_isp_set(ISP_ID id, CTL_VPE_ISP_ITEM item, void *data)
{
	INT32 rt;

	if (item >= CTL_VPE_ISP_ITEM_MAX) {
		DBG_ERR("isp_item overflow %d\r\n", item);
		return E_PAR;
	}

	rt = E_OK;
	if (g_ctl_vpe_isp_set_fp[item]) {
		rt = g_ctl_vpe_isp_set_fp[item](id, item, data);
	}

	return rt;
}

#if 0
#endif

static CTL_VPE_ISP_GET_FP g_ctl_vpe_isp_get_fp[CTL_VPE_ISP_ITEM_MAX] =
{
	NULL,
};

ER ctl_vpe_isp_get(ISP_ID id, CTL_VPE_ISP_ITEM item, void *data)
{
	INT32 rt;

	if (item >= CTL_VPE_ISP_ITEM_MAX) {
		DBG_ERR("isp_item overflow %d\r\n", item);
		return E_PAR;
	}

	rt = E_OK;
	if (g_ctl_vpe_isp_get_fp[item]) {
		rt = g_ctl_vpe_isp_get_fp[item](id, item, data);
	}

	return rt;
}

#if 0
#endif

static UINT32 ctl_vpe_isp_id_validate(UINT32 id)
{
	/* CTL_VPE_ISP_ID_UNUSED, CTL_VPE_ISP_ID_UNUSED_2 means no isp callback */
	if (id == CTL_VPE_ISP_ID_UNUSED || id == CTL_VPE_ISP_ID_UNUSED_2) {
		return FALSE;
	}
	return TRUE;
}


INT32 ctl_vpe_isp_event_cb(ISP_ID id, ISP_EVENT evt, CTL_VPE_JOB *p_job, void *param)
{
	CTL_VPE_ISP_HANDLE *p_isp_hdl;
	UINT32 frm_cnt;
	UINT32 i;

	if (ctl_vpe_isp_id_validate(id) == FALSE) {
		return CTL_VPE_E_OK;
	}

	if (evt != ISP_EVENT_VPE_CFG_IMM && evt != ISP_EVENT_PARAM_RST) {
		DBG_ERR("isp_cb_event error 0x%x\r\n", evt);
		return CTL_VPE_E_PAR;
	}

	/* get frm_cnt from in_evt */
	frm_cnt = 0;
	if ((evt == ISP_EVENT_VPE_CFG_IMM) && (p_job->in_evt.data_addr)) {
		VDO_FRAME *p_frm;

		p_frm = (VDO_FRAME *)p_job->in_evt.data_addr;
		frm_cnt = p_frm->count;
	}

	for (i = 0; i < CTL_VPE_ISP_HANDLE_MAX_NUMBER; i++) {
		p_isp_hdl = &g_ctl_vpe_isp_handle[i];
		if (p_isp_hdl->status == CTL_VPE_ISP_HANDLE_STATUS_FREE) {
			continue;
		}

		if ((p_isp_hdl->fp) && (p_isp_hdl->evt & evt)) {
			p_isp_hdl->fp(id, evt, frm_cnt, param);
		}
	}

	return CTL_VPE_E_OK;
}

#if 0
#endif

void ctl_vpe_isp_drv_init(void)
{
	UINT32 i;

	for (i = 0; i < CTL_VPE_ISP_HANDLE_MAX_NUMBER; i++) {
		ctl_vpe_isp_handle_reset(&g_ctl_vpe_isp_handle[i]);
	}
}

void ctl_vpe_isp_drv_uninit(void)
{
	UINT32 i;

	for (i = 0; i < CTL_VPE_ISP_HANDLE_MAX_NUMBER; i++) {
		ctl_vpe_isp_handle_reset(&g_ctl_vpe_isp_handle[i]);
	}
}
#else

void ctl_vpe_isp_drv_init(void) {}
void ctl_vpe_isp_drv_uninit(void) {}

#endif
