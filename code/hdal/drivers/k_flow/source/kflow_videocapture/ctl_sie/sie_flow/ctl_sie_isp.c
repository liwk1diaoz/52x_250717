#include "ctl_sie_int.h"
#include "ctl_sie_isp_int.h"
#include "ctl_sie_id_int.h"
#include "ctl_sie_buf_int.h"
#include "ctl_sie_debug_int.h"
#include "ctl_sie_utility_int.h"
#include "ctl_sie_iosize_int.h"
#include "kflow_videocapture/ctl_sie_isp.h"

// statistics data
static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)
static CTL_SIE_INT_ISP_CA_RSLT		isp_ca_rslt[CTL_SIE_ISP_STA_NUM] = {0};
static CTL_SIE_INT_ISP_LA_RSLT		isp_la_rslt[CTL_SIE_ISP_STA_NUM] = {0};
static CTL_SIE_ISP_HANDLE			ctl_sie_isp_handle[CTL_SIE_ISP_STA_NUM];
static CTL_SIE_ISP_GET_IMG_INFO 	ctl_sie_isp_get_img_info;
static BOOL							ctl_sie_isp_sim_trig_start[CTL_SIE_ISP_STA_NUM] = {0};
static CTL_SIE_ISP_IQ_DBG_INFO		ctl_sie_isp_dbg_info[CTL_SIE_ISP_STA_NUM] = {0};

#if 0
#endif

/*
    static function
*/

_INLINE CTL_SIE_ID __ctl_sie_isp_conv2_sie_id(ISP_ID isp_id)
{
	UINT32 i;
	CTL_SIE_HDL *sie_hdl;
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(CTL_SIE_ID_1);

	for (i = 0; i <= sie_limit->max_spt_id; i++) {
		sie_hdl = ctl_sie_get_hdl(i);
		if (sie_hdl != NULL && sie_hdl->isp_id == isp_id) {
			return sie_hdl->id;
		}
	}
	ctl_sie_dbg_err("ISP_ID 0x%x overflow\r\n", (unsigned int)isp_id);
	return CTL_SIE_ID_1;
}

_INLINE INT32 __ctl_sie_isp_lock(UINT32 id, FLGPTN flag, BOOL lock_en)
{
	FLGPTN wait_flag;

	if (lock_en) {
		if (vos_flag_wait_timeout(&wait_flag, g_ctl_sie_isp_flg_id, flag, TWF_CLR|TWF_ANDW, vos_util_msec_to_tick(CTL_SIE_TO_MS)) == 0) {
		} else {
			return CTL_SIE_E_TMOUT;
		}
	} else {
		vos_flag_set(g_ctl_sie_isp_flg_id, flag);
	}
	return CTL_SIE_E_OK;
}

_INLINE INT32 __ctl_sie_isp_get_img_start(void)
{
	FLGPTN wait_flag;

	if (vos_flag_wait_timeout(&wait_flag, g_ctl_sie_isp_flg_id, CTL_SIE_ISP_GET_IMG_END, TWF_CLR, vos_util_msec_to_tick(CTL_SIE_TO_MS)) == 0) {
		vos_flag_clr(g_ctl_sie_isp_flg_id, (CTL_SIE_ISP_GET_IMG_READY));
		return CTL_SIE_E_OK;
	}
	ctl_sie_dbg_err("timeout(id=%d)\r\n", g_ctl_sie_isp_flg_id);
	return CTL_SIE_E_SYS;
}

_INLINE void __ctl_sie_isp_get_img_end(void)
{
	vos_flag_clr(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_GET_IMG_READY);
	vos_flag_set(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_GET_IMG_END);
}

static CTL_SIE_ISP_HANDLE *__ctl_sie_isp_handle_alloc(void)
{
	UINT32 i;
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(CTL_SIE_ID_1);

	for (i = 0; i <= sie_limit->max_spt_id; i++) {
		if (ctl_sie_isp_handle[i].status == CTL_SIE_ISP_HANDLE_STATUS_FREE) {
			ctl_sie_isp_handle[i].status = CTL_SIE_ISP_HANDLE_STATUS_USED;
			return &ctl_sie_isp_handle[i];
		}
	}

	return NULL;
}

static void __ctl_sie_isp_handle_release(CTL_SIE_ISP_HANDLE *p_hdl)
{
	memset((void *)&p_hdl->name[0], '\0', sizeof(p_hdl->name));
	p_hdl->fp = NULL;
	p_hdl->evt = ISP_EVENT_NONE;
	p_hdl->status = CTL_SIE_ISP_HANDLE_STATUS_FREE;
}

_INLINE UINT32 __ctl_sie_isp_evt_conv2_sts(ISP_EVENT evt)
{
	UINT32 sie_inte = CTL_SIE_CLR;

	if (evt & ISP_EVENT_SIE_VD || evt & ISP_EVENT_SIE_VD_IMM) {
		sie_inte |= CTL_SIE_INTE_VD;
	}
	if (evt & ISP_EVENT_SIE_BP1 || evt & ISP_EVENT_SIE_BP1_IMM) {
		sie_inte |= CTL_SIE_INTE_BP1;
	}
	if (evt & ISP_EVENT_SIE_MD_HIT || evt & ISP_EVENT_SIE_MD_HIT_IMM) {
		sie_inte |= CTL_SIE_INTE_MD_HIT;
	}
	if (evt & ISP_EVENT_SIE_ACTST || evt & ISP_EVENT_SIE_ACTST_IMM) {
		sie_inte |= CTL_SIE_INTE_ACTST;
	}
	if (evt & ISP_EVENT_SIE_CRPST || evt & ISP_EVENT_SIE_CRPST_IMM) {
		sie_inte |= CTL_SIE_INTE_CRPST;
	}
	if (evt & ISP_EVENT_SIE_DRAM_OUT0_END || evt & ISP_EVENT_SIE_DRAM_OUT0_END_IMM) {
		sie_inte |= CTL_SIE_INTE_DRAM_OUT0_END;
	}
	if (evt & ISP_EVENT_SIE_ACTEND || evt & ISP_EVENT_SIE_ACTEND_IMM) {
		sie_inte |= CTL_SIE_INTE_ACTEND;
	}
	if (evt & ISP_EVENT_SIE_CRPEND || evt & ISP_EVENT_SIE_CRPEND_IMM) {
		sie_inte |= CTL_SIE_INTE_CROPEND;
	}

	return sie_inte;
}

_INLINE ISP_EVENT __ctl_sie_isp_sts_conv2_evt(UINT32 status)
{
	ISP_EVENT isp_evt = ISP_EVENT_NONE;

	if (status == CTL_SIE_INTE_ALL) {
		return isp_evt;
	}

	if (status & CTL_SIE_INTE_VD) {
		isp_evt |= ISP_EVENT_SIE_VD;
	}
	if (status & CTL_SIE_INTE_BP1) {
		isp_evt |= ISP_EVENT_SIE_BP1;
	}
	if (status & CTL_SIE_INTE_MD_HIT) {
		isp_evt |= ISP_EVENT_SIE_MD_HIT;
	}
	if (status & CTL_SIE_INTE_ACTST) {
		isp_evt |= ISP_EVENT_SIE_ACTST;
	}
	if (status & CTL_SIE_INTE_CRPST) {
		isp_evt |= ISP_EVENT_SIE_CRPST;
	}
	if (status & CTL_SIE_INTE_DRAM_OUT0_END) {
		isp_evt |= ISP_EVENT_SIE_DRAM_OUT0_END;
	}
	if (status & CTL_SIE_INTE_ACTEND) {
		isp_evt |= ISP_EVENT_SIE_ACTEND;
	}
	if (status & CTL_SIE_INTE_CROPEND) {
		isp_evt |= ISP_EVENT_SIE_CRPEND;
	}

	return isp_evt;
}

_INLINE ISP_EVENT __ctl_sie_isp_sts_conv2_evt_imm(UINT32 status)
{
	ISP_EVENT isp_evt = ISP_EVENT_NONE;

	if (status == CTL_SIE_INTE_ALL) {
		isp_evt = ISP_EVENT_PARAM_RST;
		return isp_evt;
	}

	if (status & CTL_SIE_INTE_VD) {
		isp_evt |= ISP_EVENT_SIE_VD_IMM;
	}
	if (status & CTL_SIE_INTE_BP1) {
		isp_evt |= ISP_EVENT_SIE_BP1_IMM;
	}
	if (status & CTL_SIE_INTE_MD_HIT) {
		isp_evt |= ISP_EVENT_SIE_MD_HIT_IMM;
	}
	if (status & CTL_SIE_INTE_ACTST) {
		isp_evt |= ISP_EVENT_SIE_ACTST_IMM;
	}
	if (status & CTL_SIE_INTE_CRPST) {
		isp_evt |= ISP_EVENT_SIE_CRPST_IMM;
	}
	if (status & CTL_SIE_INTE_DRAM_OUT0_END) {
		isp_evt |= ISP_EVENT_SIE_DRAM_OUT0_END_IMM;
	}
	if (status & CTL_SIE_INTE_ACTEND) {
		isp_evt |= ISP_EVENT_SIE_ACTEND_IMM;
	}
	if (status & CTL_SIE_INTE_CROPEND) {
		isp_evt |= ISP_EVENT_SIE_CRPEND_IMM;
	}

	return isp_evt;
}

#if 0
#endif

INT32 ctl_sie_isp_update_sta_out(CTL_SIE_ISP_SUB_OUT_PARAM sub_out)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(sub_out.id);
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(sub_out.id);
	INT32 rt = CTL_SIE_E_OK;

	if (hdl == NULL) {
		ctl_sie_dbg_wrn("NULL SIE HDL\r\n");
		return CTL_SIE_E_HDL;
	}

	//get ca rslt
	if ((sie_limit->support_func & KDRV_SIE_FUNC_SPT_CA) && (hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AWB]) &&
		sub_out.ca_addr != 0) {
		vos_cpu_dcache_sync(sub_out.ca_addr, sub_out.ca_total_size, VOS_DMA_FROM_DEVICE);
		memcpy((void *)hdl->ctrl.ca_private_addr, (void *)sub_out.ca_addr, sub_out.ca_total_size);
		isp_ca_rslt[sub_out.id].src_addr = hdl->ctrl.ca_private_addr;
	}

	//get la rslt
	if ((sie_limit->support_func & KDRV_SIE_FUNC_SPT_LA) && (hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AE]) &&
		sub_out.la_addr != 0) {
		vos_cpu_dcache_sync(sub_out.la_addr, sub_out.la_total_size, VOS_DMA_FROM_DEVICE);
		memcpy((void *)hdl->ctrl.la_private_addr, (void *)sub_out.la_addr, sub_out.la_total_size);
		isp_la_rslt[sub_out.id].src_addr = hdl->ctrl.la_private_addr;
	}

	return rt;
}

/*
	ISP Get Function
*/

static INT32 ctl_sie_isp_get_func_en(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	ISP_FUNC_EN isp_func_en = 0;
	unsigned long loc_cpu_flg;

	if (hdl == NULL) {
		ctl_sie_dbg_wrn("NULL SIE HDL\r\n");
		return CTL_SIE_E_HDL;
	}

	loc_cpu(loc_cpu_flg);
	if (hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AE]) {
		isp_func_en |= ISP_FUNC_EN_AE;
	}

	if (hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AWB]) {
		isp_func_en |= ISP_FUNC_EN_AWB;
	}

	if (hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_SHDR]) {
		isp_func_en |= ISP_FUNC_EN_SHDR;
	}

	if (hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_WDR]) {
		isp_func_en |= ISP_FUNC_EN_WDR;
	}

	*(ISP_FUNC_EN *)data = isp_func_en;
	unl_cpu(loc_cpu_flg);

	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_isp_get_status(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);

	if (hdl == NULL) {
		ctl_sie_dbg_wrn("NULL SIE HDL\r\n");
		return CTL_SIE_E_HDL;
	}

	*(CTL_SIE_ISP_STATUS *)data = ctl_sie_get_state_machine(id);
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_isp_get_io_size(CTL_SIE_ID id, void *data)
{
	CTL_SIE_ISP_IO_SIZE *io_size = (CTL_SIE_ISP_IO_SIZE *)data;
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	unsigned long loc_cpu_flg;

	if (hdl == NULL) {
		ctl_sie_dbg_wrn("NULL SIE HDL\r\n");
		return CTL_SIE_E_HDL;
	}

	loc_cpu(loc_cpu_flg);
	io_size->in_act_win = hdl->ctrl.rtc_obj.sie_act_win;
	io_size->in_crp_win = hdl->param.io_size_info.size_info.sie_crp;
	io_size->out_sz = hdl->param.io_size_info.size_info.sie_scl_out;
	unl_cpu(loc_cpu_flg);
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_isp_get_dupl_src_id(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	unsigned long loc_cpu_flg;

	if (hdl == NULL) {
		ctl_sie_dbg_wrn("NULL SIE HDL\r\n");
		return CTL_SIE_E_HDL;
	}

	loc_cpu(loc_cpu_flg);
	*(CTL_SIE_ID *)data = hdl->dupl_src_id;
	unl_cpu(loc_cpu_flg);
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_isp_get_mutil_frm_grp(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	unsigned long loc_cpu_flg;

	if (hdl == NULL) {
		ctl_sie_dbg_wrn("NULL SIE HDL\r\n");
		return CTL_SIE_E_HDL;
	}

	loc_cpu(loc_cpu_flg);
	if (hdl->flow_type == CTL_SIE_FLOW_PATGEN) {
		*(CTL_SEN_OUTPUT_DEST *)data = hdl->ctrl.rtc_obj.pat_gen_param.group_idx;
	} else {
		*(CTL_SEN_OUTPUT_DEST *)data = hdl->param.chg_senmode_info.output_dest;
	}

	unl_cpu(loc_cpu_flg);
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_isp_get_img_out(CTL_SIE_ID id, void *data)
{
	INT32 rt;
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	CTL_SIE_ISP_HEADER_INFO *isp_header_info = (CTL_SIE_ISP_HEADER_INFO *)data;
	unsigned long loc_cpu_flg;
	FLGPTN wait_flag = 0;

	if (hdl == NULL || isp_header_info == NULL) {
		ctl_sie_dbg_err("null 0x%.8x 0x%.8x\r\n", (unsigned int)hdl, (unsigned int)isp_header_info);
		return CTL_SIE_E_SYS;
	}

	if (__ctl_sie_isp_get_img_start() != CTL_SIE_E_OK) {
		return CTL_SIE_E_SYS;
	}

	//enable dma out when direct mode get raw
	if (hdl->param.out_dest == CTL_SIE_OUT_DEST_DIRECT) {
		ctl_sie_module_direct_to_both(id);
	}

	loc_cpu(loc_cpu_flg);
	memset((void *)&ctl_sie_isp_get_img_info, 0, sizeof(CTL_SIE_ISP_GET_IMG_INFO));
	ctl_sie_isp_get_img_info.id = id;
	unl_cpu(loc_cpu_flg);

	if (vos_flag_wait_timeout(&wait_flag, g_ctl_sie_isp_flg_id, CTL_SIE_ISP_GET_IMG_READY, TWF_ORW, vos_util_msec_to_tick(CTL_SIE_TO_MS)) == 0) {
		rt = CTL_SIE_E_OK;
		loc_cpu(loc_cpu_flg);
		isp_header_info->buf_id = ctl_sie_isp_get_img_info.sie_head_info.buf_id;
		isp_header_info->buf_addr = ctl_sie_isp_get_img_info.sie_head_info.buf_addr;
		isp_header_info->vdo_frm = ctl_sie_isp_get_img_info.sie_head_info.vdo_frm;
		unl_cpu(loc_cpu_flg);
	} else {
		ctl_sie_dbg_err("timeout(id=%d)\r\n", id);
		rt = CTL_SIE_E_SYS;
		isp_header_info->buf_id = 0;
		isp_header_info->buf_addr = 0;
		if (hdl->param.out_dest == CTL_SIE_OUT_DEST_BOTH) {
			ctl_sie_module_both_to_direct(id);
		}
	}

	__ctl_sie_isp_get_img_end();
	return rt;
}

static INT32 ctl_sie_isp_get_kdrv_param(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);

	if (hdl == NULL || data == NULL) {
		return CTL_SIE_E_HDL;
	}

	return kdf_sie_get(hdl->kdf_hdl, CTL_SIE_INT_ITEM_KDRV_PARAM|CTL_SIE_INT_ITEM_REV, (void *)data);
}

static INT32 ctl_sie_isp_get_md_rslt(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);

	if (hdl == NULL || data == NULL) {
		return CTL_SIE_E_HDL;
	}

	return kdf_sie_get(hdl->kdf_hdl, CTL_SIE_INT_ITEM_MD_RSLT|CTL_SIE_INT_ITEM_REV, (void *)data);
}

static INT32 ctl_sie_isp_get_ca_rslt(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);
	INT32 rt = CTL_SIE_E_OK;

	if (hdl == NULL || data == NULL) {
		return CTL_SIE_E_HDL;
	}

	isp_ca_rslt[id].ca_rst = (CTL_SIE_ISP_CA_RSLT *)data;
	//get ca rslt
	if ((sie_limit->support_func & KDRV_SIE_FUNC_SPT_CA) &&	(hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AWB]) &&
		(isp_ca_rslt[id].src_addr != 0 && isp_ca_rslt[id].ca_rst != NULL)) {
		rt |= kdf_sie_get(hdl->kdf_hdl, CTL_SIE_ITEM_CA_RSLT|CTL_SIE_INT_ITEM_REV, (void *)&isp_ca_rslt[id]);
	}

	return rt;
}

static INT32 ctl_sie_isp_get_la_rslt(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);
	INT32 rt = CTL_SIE_E_OK;

	if (hdl == NULL || data == NULL) {
		return CTL_SIE_E_HDL;
	}

	isp_la_rslt[id].la_rst = (CTL_SIE_ISP_LA_RSLT *)data;
	//get ca rslt
	if ((sie_limit->support_func & KDRV_SIE_FUNC_SPT_LA) && (hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AE]) &&
		(isp_la_rslt[id].src_addr != 0 && isp_la_rslt[id].la_rst != NULL)) {
		rt |= kdf_sie_get(hdl->kdf_hdl, CTL_SIE_ITEM_LA_RSLT|CTL_SIE_INT_ITEM_REV, (void *)&isp_la_rslt[id]);
	}

	return rt;
}

/** sie get functions **/
CTL_SIE_ISP_GET_FP ctl_sie_isp_get_fp[CTL_SIE_ISP_ITEM_MAX] = {
	ctl_sie_isp_get_func_en,//func en
	ctl_sie_isp_get_status,//status
	ctl_sie_isp_get_io_size,//io size
	ctl_sie_isp_get_dupl_src_id,//dupl src id
	ctl_sie_isp_get_mutil_frm_grp,//multi frame group
	ctl_sie_isp_get_ca_rslt,
	ctl_sie_isp_get_la_rslt,
	NULL,	//sta roi ratio
	NULL,	//iq param
	NULL,	//bp1
	NULL,	//bp2
	NULL,
	ctl_sie_isp_get_img_out,
	ctl_sie_isp_get_kdrv_param,
	NULL,
	NULL,
	ctl_sie_isp_get_md_rslt,
};

#if 0
#endif

/*
	ISP Set Function
*/

static INT32 ctl_sie_isp_set_sta_roi_ratio(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	CTL_SIE_ISP_ROI_RATIO *isp_roi_ratio = (CTL_SIE_ISP_ROI_RATIO *)data;
	UINT64 set_item_imm = 0;
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);

	if (hdl == NULL || data == NULL) {
		return CTL_SIE_E_HDL;
	}

	if (isp_roi_ratio->ratio_base == 0) {
		ctl_sie_dbg_err("%d roi base = 0\r\n", (int)id);
		return CTL_SIE_E_PAR;
	} else {
		if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_CA && hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AWB]) {
			if (isp_roi_ratio->ca_crop_win_roi.w != 0 && isp_roi_ratio->ca_crop_win_roi.h != 0) {
				hdl->ctrl.ca_roi.roi_base = isp_roi_ratio->ratio_base;
				hdl->ctrl.ca_roi.roi = isp_roi_ratio->ca_crop_win_roi;
				set_item_imm |= 1ULL << CTL_SIE_INT_ITEM_CA_ROI;
			}
		}

		if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_LA && hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AE]) {
			if (isp_roi_ratio->la_crop_win_roi.w != 0 && isp_roi_ratio->la_crop_win_roi.h != 0) {
				hdl->ctrl.la_roi.roi_base = isp_roi_ratio->ratio_base;
				hdl->ctrl.la_roi.roi = isp_roi_ratio->la_crop_win_roi;
				set_item_imm |= 1ULL << CTL_SIE_INT_ITEM_LA_ROI;
			}
		}
		ctl_sie_hdl_update_item(id, set_item_imm, TRUE);
		return CTL_SIE_E_OK;
	}
}

static INT32 ctl_sie_isp_set_iq_param(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	CTL_SIE_IQ_PARAM *sie_iq_param = (CTL_SIE_IQ_PARAM *)data;
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);
	UINT64 update_item = 0;

	if (hdl == NULL) {
		ctl_sie_dbg_err("id = %d hdl NULL\r\n", (int)id);
		return CTL_SIE_E_HDL;
	} else if (!ctl_sie_isp_dbg_info[id].skip_cb_en) {
		if (data == NULL) {
			ctl_sie_dbg_err("id = %d iq par NULL\r\n", (int)id);
			return CTL_SIE_E_PAR;
		}
	}

	if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_OB_BYPASS) {
		if (ctl_sie_isp_dbg_info[id].skip_cb_en) {
			if (ctl_sie_isp_dbg_info[id].iq_func_off & CTL_SIE_IQ_FUNC_OB_BYPASS) {
				hdl->ctrl.ob_param.bypass_enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_OB;
			}
		} else if (sie_iq_param->ob_param != NULL) {
			hdl->ctrl.ob_param = *(CTL_SIE_OB_PARAM *)sie_iq_param->ob_param;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_OB;
		}
	}

	if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_CA) {
		if (ctl_sie_isp_dbg_info[id].skip_cb_en) {
			if (ctl_sie_isp_dbg_info[id].iq_func_off & CTL_SIE_IQ_FUNC_CA) {
				hdl->ctrl.ca_param.enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_CA;
			}
		} else 	if (sie_iq_param->ca_param != NULL) {
			if (sie_iq_param->ca_param->enable) {
				if (hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AWB]) {
					hdl->ctrl.ca_param = *(CTL_SIE_CA_PARAM *)sie_iq_param->ca_param;
					update_item |= 1ULL << CTL_SIE_INT_ITEM_CA;
				} else {
					ctl_sie_dbg_err("id %d force disable AWB\r\n", (int)id);
				}
			} else {
				hdl->ctrl.ca_param.enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_CA;
			}
		}
	}

	if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_LA) {
		if (ctl_sie_isp_dbg_info[id].skip_cb_en) {
			if (ctl_sie_isp_dbg_info[id].iq_func_off & CTL_SIE_IQ_FUNC_LA) {
				hdl->ctrl.la_param.enable = DISABLE;
				hdl->ctrl.la_param.histogram_enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_LA;
			}
		} else if (sie_iq_param->la_param != NULL) {
			if (sie_iq_param->la_param->enable) {
				if (hdl->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AE]) {
					hdl->ctrl.la_param.enable = sie_iq_param->la_param->enable;
					hdl->ctrl.la_param.win_num = sie_iq_param->la_param->win_num;
					hdl->ctrl.la_param.la_src = sie_iq_param->la_param->la_src;
					hdl->ctrl.la_param.la_rgb2y1mod = sie_iq_param->la_param->la_rgb2y1mod;
					hdl->ctrl.la_param.la_rgb2y2mod = sie_iq_param->la_param->la_rgb2y2mod;
					hdl->ctrl.la_param.cg_enable = sie_iq_param->la_param->cg_enable;
					hdl->ctrl.la_param.r_gain = sie_iq_param->la_param->r_gain;
					hdl->ctrl.la_param.g_gain = sie_iq_param->la_param->g_gain;
					hdl->ctrl.la_param.b_gain = sie_iq_param->la_param->b_gain;
					hdl->ctrl.la_param.gamma_enable = sie_iq_param->la_param->gamma_enable;
					hdl->ctrl.la_param.gamma_tbl_addr = (UINT32)&sie_iq_param->la_param->gamma_tbl[0];
					hdl->ctrl.la_param.histogram_enable = sie_iq_param->la_param->hist_enable;
					hdl->ctrl.la_param.histogram_src = sie_iq_param->la_param->histo_src;
					hdl->ctrl.la_param.irsub_r_weight = sie_iq_param->la_param->irsub_r_weight;
					hdl->ctrl.la_param.irsub_g_weight = sie_iq_param->la_param->irsub_g_weight;
					hdl->ctrl.la_param.irsub_b_weight = sie_iq_param->la_param->irsub_b_weight;
					hdl->ctrl.la_param.la_ob_ofs = sie_iq_param->la_param->la_ob_ofs;
					hdl->ctrl.la_param.lath_enable = sie_iq_param->la_param->lath_enable;
					hdl->ctrl.la_param.lathy1lower = sie_iq_param->la_param->lathy1lower;
					hdl->ctrl.la_param.lathy1upper = sie_iq_param->la_param->lathy1upper;
					hdl->ctrl.la_param.lathy2lower = sie_iq_param->la_param->lathy2lower;
					hdl->ctrl.la_param.lathy2upper = sie_iq_param->la_param->lathy2upper;
					update_item |= 1ULL << CTL_SIE_INT_ITEM_LA;
				} else {
					ctl_sie_dbg_err("id %d force disable AE\r\n", (int)id);
				}
			} else {
				hdl->ctrl.la_param.enable = DISABLE;
				hdl->ctrl.la_param.histogram_enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_LA;
			}
		}
	}

	if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_CGAIN) {
		if (ctl_sie_isp_dbg_info[id].skip_cb_en) {
			if (ctl_sie_isp_dbg_info[id].iq_func_off & CTL_SIE_IQ_FUNC_CG) {
				hdl->ctrl.cgain_param.enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_CGAIN;
			}
		} else if (sie_iq_param->cgain_param != NULL) {
			hdl->ctrl.cgain_param = *(CTL_SIE_CGAIN *)sie_iq_param->cgain_param;
			update_item |= 1ULL << CTL_SIE_INT_ITEM_CGAIN;
		}
	}

	if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_DGAIN) {
		if (ctl_sie_isp_dbg_info[id].skip_cb_en) {
			if (ctl_sie_isp_dbg_info[id].iq_func_off & CTL_SIE_IQ_FUNC_DG) {
				hdl->ctrl.dgain_param.enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_DGAIN;
			}
		} else if (sie_iq_param->dgain_param != NULL) {
			hdl->ctrl.dgain_param = *(CTL_SIE_DGAIN *)sie_iq_param->dgain_param;
			update_item |= 1ULL << CTL_SIE_INT_ITEM_DGAIN;
		}
	}

	if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_DPC) {
		if (ctl_sie_isp_dbg_info[id].skip_cb_en) {
			if (ctl_sie_isp_dbg_info[id].iq_func_off & CTL_SIE_IQ_FUNC_DPC) {
				hdl->ctrl.dpc_param.enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_DPC;
			}
		} else if (sie_iq_param->dpc_param != NULL) {
			hdl->ctrl.dpc_param.enable = sie_iq_param->dpc_param->enable;
			hdl->ctrl.dpc_param.tbl_addr = sie_iq_param->dpc_param->tbl_addr_va;
			hdl->ctrl.dpc_param.weight = sie_iq_param->dpc_param->weight;
			hdl->ctrl.dpc_param.dp_total_size = sie_iq_param->dpc_param->dp_total_size;
			update_item |= 1ULL << CTL_SIE_INT_ITEM_DPC;
		}
	}

	if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_ECS) {
		if (ctl_sie_isp_dbg_info[id].skip_cb_en) {
			if (ctl_sie_isp_dbg_info[id].iq_func_off & CTL_SIE_IQ_FUNC_ECS) {
				hdl->ctrl.ecs_param.enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_ECS;
			}
		} else if (sie_iq_param->ecs_param != NULL) {
			hdl->ctrl.ecs_param.enable = sie_iq_param->ecs_param->enable;
			hdl->ctrl.ecs_param.sel_37_fmt = sie_iq_param->ecs_param->sel_37_fmt;
			hdl->ctrl.ecs_param.map_tbl_addr = sie_iq_param->ecs_param->tbl_addr_va;
			hdl->ctrl.ecs_param.map_sel = sie_iq_param->ecs_param->map_sel;
			hdl->ctrl.ecs_param.dthr_enable = sie_iq_param->ecs_param->dthr_enable;
			hdl->ctrl.ecs_param.dthr_reset = sie_iq_param->ecs_param->dthr_reset;
			hdl->ctrl.ecs_param.dthr_level = sie_iq_param->ecs_param->dthr_level;
			hdl->ctrl.ecs_param.bayer_mode = sie_iq_param->ecs_param->bayer_mode;
			update_item |= 1ULL << CTL_SIE_INT_ITEM_ECS;
		}
	}

	if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_COMPANDING) {
		if (ctl_sie_isp_dbg_info[id].skip_cb_en) {
			if (ctl_sie_isp_dbg_info[id].iq_func_off & CTL_SIE_IQ_FUNC_COMPAND) {
				hdl->ctrl.companding_param.enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_COMPAND;
			}
		} else if (sie_iq_param->companding_param != NULL) {
			hdl->ctrl.companding_param = *(CTL_SIE_COMPANDING *)sie_iq_param->companding_param;
			update_item |= 1ULL << CTL_SIE_INT_ITEM_COMPAND;
		}
	}

	if (sie_limit->support_func & KDRV_SIE_FUNC_SPT_MD) {
		if (ctl_sie_isp_dbg_info[id].skip_cb_en) {
			if (ctl_sie_isp_dbg_info[id].iq_func_off & CTL_SIE_IQ_FUNC_MD) {
				hdl->ctrl.md_param.enable = DISABLE;
				update_item |= 1ULL << CTL_SIE_INT_ITEM_MD;
			}
		} else if (sie_iq_param->md_param != NULL) {
			hdl->ctrl.md_param = *(CTL_SIE_MD_PARAM *)sie_iq_param->md_param;
			update_item |= 1ULL << CTL_SIE_INT_ITEM_MD;
		}
	}

	ctl_sie_hdl_update_item(id, update_item, TRUE);
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_isp_set_bp1(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);

	if (hdl == NULL || data == NULL) {
		return CTL_SIE_E_HDL;
	}

	hdl->ctrl.bp1 = *(UINT32 *)data;
	ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_INT_ITEM_BP1, TRUE);
	if (hdl->ctrl.bp1) {
		ctl_sie_update_inte(id, ENABLE, CTL_SIE_INTE_BP1);
	} else {
		ctl_sie_update_inte(id, DISABLE, CTL_SIE_INTE_BP1);
	}
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_isp_set_bp2(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);

	if (hdl == NULL || data == NULL) {
		return CTL_SIE_E_HDL;
	}

	hdl->ctrl.bp2 = *(UINT32 *)data;
	ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_INT_ITEM_BP2, TRUE);
	if (hdl->ctrl.bp2) {
		ctl_sie_update_inte(id, ENABLE, CTL_SIE_INTE_BP2);
	} else {
		ctl_sie_update_inte(id, DISABLE, CTL_SIE_INTE_BP2);
	}
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_isp_set_img_out(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);

	if (hdl == NULL || data == NULL) {
		return CTL_SIE_E_HDL;
	}

	ctl_sie_isp_get_img_info.sie_head_info = *(CTL_SIE_HEADER_INFO *)data;
	ctl_sie_buf_io_cfg(id, CTL_SIE_BUF_IO_UNLOCK, 0, (UINT32)&ctl_sie_isp_get_img_info.sie_head_info);
	memset((void *)&ctl_sie_isp_get_img_info.sie_head_info, 0, sizeof(CTL_SIE_HEADER_INFO));

	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_isp_set_sim_buf_new(CTL_SIE_ID id, void *data)
{
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	CTL_SIE_ISP_SIM_BUF_NEW *isp_sim_buf_new;
	CTL_SIE_HEADER_INFO sie_head_info = {0};
	CTL_SIE_STATUS status = ctl_sie_get_state_machine(id);

	if (hdl == NULL || data == NULL) {
		ctl_sie_dbg_err("id: %d NULL hdl or data\r\n", (int)id);
		return CTL_SIE_E_HDL;
	}

	if (hdl->param.out_dest != CTL_SIE_OUT_DEST_DRAM) {
		ctl_sie_dbg_err("id: %d only dram mode spt sim flow\r\n", (int)id);
		return CTL_SIE_E_NOSPT;
	}

	if (hdl->ctrl.bufiocb.cb_fp == NULL) {
		ctl_sie_dbg_err("BUFIO_CB NULL\r\n");
		return CTL_SIE_E_NULL_FP;
	}

	switch (status) {
	case CTL_SIE_STS_RUN: {
			CTL_SIE_TRIG_INFO trig_info;

			trig_info.trig_type = CTL_SIE_TRIG_STOP;
			trig_info.b_wait_end = TRUE;
			ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);
			ctl_sie_isp_sim_trig_start[id] = TRUE;
		}
		break;

	case CTL_SIE_STS_READY:
		break;

	default:
		ctl_sie_dbg_wrn("%d state err\r\n", (int)id);
		return CTL_SIE_E_STATE;
	}

	isp_sim_buf_new = (CTL_SIE_ISP_SIM_BUF_NEW *)data;
	sie_head_info.buf_id = isp_sim_buf_new->frm_cnt;
	sie_head_info.vdo_frm.count = isp_sim_buf_new->frm_cnt;
	ctl_sie_buf_io_cfg(id, CTL_SIE_BUF_IO_NEW, isp_sim_buf_new->buf_size, (UINT32)&sie_head_info);
	if (sie_head_info.buf_addr == 0) {
		isp_sim_buf_new->buf_id = 0;
		isp_sim_buf_new->buf_addr = 0;
		return CTL_SIE_E_NOMEM;
	} else {
		isp_sim_buf_new->buf_id = sie_head_info.buf_id;
		isp_sim_buf_new->buf_addr = sie_head_info.buf_addr;
		return CTL_SIE_E_OK;
	}
}

static INT32 ctl_sie_isp_set_sim_buf_push(CTL_SIE_ID id, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_HDL *hdl = ctl_sie_get_hdl(id);
	CTL_SIE_ISP_SIM_BUF_PUSH *isp_sim_buf_push;

	if (hdl == NULL || data == NULL) {
		ctl_sie_dbg_err("id: %d NULL hdl or data\r\n", (int)id);
		return CTL_SIE_E_HDL;
	}

	if (hdl->param.out_dest != CTL_SIE_OUT_DEST_DRAM) {
		ctl_sie_dbg_err("id: %d only dram mode spt sim flow\r\n", (int)id);
		return CTL_SIE_E_NOSPT;
	}

	isp_sim_buf_push = (CTL_SIE_ISP_SIM_BUF_PUSH *)data;
	isp_sim_buf_push->isp_head_info.vdo_frm.timestamp = ctl_sie_util_get_syst_timestamp();	//requested from IQ-Tool
	rt = ctl_sie_buf_msg_snd(CTL_SIE_BUF_MSG_PUSH, id, (UINT32)&hdl->ctrl, (UINT32)&isp_sim_buf_push->isp_head_info);
	if (isp_sim_buf_push->sim_end && ctl_sie_isp_sim_trig_start[id]) {
		CTL_SIE_TRIG_INFO trig_info;

		ctl_sie_isp_sim_trig_start[id] = FALSE;
		trig_info.trig_type = CTL_SIE_TRIG_START;
		trig_info.trig_frame_num = 0xffffffff;
		trig_info.b_wait_end = TRUE;
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_TRIG_IMM, (void *)&trig_info);
		ctl_sie_set((UINT32)hdl, CTL_SIE_ITEM_RESET_FC_IMM, NULL);
	}
	return rt;
}

/** sie set functions **/
CTL_SIE_ISP_SET_FP ctl_sie_isp_set_fp[CTL_SIE_ISP_ITEM_MAX] = {
	NULL,//func en
	NULL,//status
	NULL,//io size
	NULL,//dupl src id
	NULL,//multi frame group
	NULL,
	NULL,
	ctl_sie_isp_set_sta_roi_ratio,
	ctl_sie_isp_set_iq_param,
	ctl_sie_isp_set_bp1,
	ctl_sie_isp_set_bp2,
	NULL,
	ctl_sie_isp_set_img_out,
	NULL,
	ctl_sie_isp_set_sim_buf_new,
	ctl_sie_isp_set_sim_buf_push,
	NULL,
};

#if 0
#endif

/*
	public Function
*/

UINT32 ctl_sie_isp_get_sts(void)
{
	UINT32 i;
	ISP_EVENT evt = ISP_EVENT_NONE;
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(CTL_SIE_ID_1);

	for (i = 0; i <= sie_limit->max_spt_id; i++) {
		if (ctl_sie_isp_handle[i].status == CTL_SIE_ISP_HANDLE_STATUS_USED) {
			evt |= ctl_sie_isp_handle[i].evt;
		}
	}

	return __ctl_sie_isp_evt_conv2_sts(evt);
}

INT32 ctl_sie_isp_event_cb(CTL_SIE_ID id, UINT32 sts, UINT64 fc, void *param)
{
	UINT32 i;
	CTL_SIE_ISP_HANDLE *p_hdl;
	CTL_SIE_HDL *sie_hdl = ctl_sie_get_hdl(id);
	ISP_EVENT evt = __ctl_sie_isp_sts_conv2_evt(sts);
	ISP_EVENT evt_imm = __ctl_sie_isp_sts_conv2_evt_imm(sts);
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);
	UINT32 ts_start, ts_end;
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();

	if (ctl_sie_isp_dbg_info[id].skip_cb_en) {
		if (ctl_sie_isp_dbg_info[id].iq_func_off != 0) {
			ctl_sie_isp_set_iq_param(id, NULL);
		}
		return rt;
	}

	if (sie_hdl == NULL) {
		ctl_sie_dbg_err("id = %d hdl NULL\r\n", (int)id);
		return CTL_SIE_E_HDL;
	}

	if (evt != ISP_EVENT_NONE || evt_imm != ISP_EVENT_NONE) {
		for (i = 0; i <= sie_limit->max_spt_id; i++) {
			p_hdl = &ctl_sie_isp_handle[i];
			if (p_hdl->fp != NULL) {
				//imm trigger isp by isr
				if (p_hdl->evt & evt_imm) {
					ts_start = ctl_sie_util_get_timestamp();
					vos_flag_iset(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_CB_OUT);
					p_hdl->fp(sie_hdl->isp_id, p_hdl->evt & evt_imm, (UINT32)fc, 0);
					vos_flag_iclr(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_TASK_CB_OUT);
					ts_end = ctl_sie_util_get_timestamp();
					if (dbg_tab != NULL && dbg_tab->set_isp_cb_t_log != NULL) {
						dbg_tab->set_isp_cb_t_log(sie_hdl->isp_id, p_hdl->evt & evt_imm, (UINT32)fc, ts_start, ts_end);
					}
				}
				//trigger isp by task
				if (p_hdl->evt & evt) {
					ctl_sie_isp_msg_snd(CTL_SIE_ISP_MSG_CBEVT_ISP, sie_hdl->isp_id, p_hdl->evt & evt, (UINT32)p_hdl->fp, (UINT32)fc);
				}
			}
		}
	}

	return rt;
}

void ctl_sie_isp_init(void)
{
	UINT32 i;
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(CTL_SIE_ID_1);

	for (i = 0; i <= sie_limit->max_spt_id; i++) {
		__ctl_sie_isp_handle_release(&ctl_sie_isp_handle[i]);
	}
}

INT32 ctl_sie_isp_evt_fp_reg(CHAR *name, ISP_EVENT_FP fp, ISP_EVENT evt, CTL_SIE_ISP_CB_MSG cb_msg)
{
	CTL_SIE_ISP_HANDLE *p_hdl;
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(CTL_SIE_ID_1);
	UINT32 i;

	p_hdl = __ctl_sie_isp_handle_alloc();
	if (p_hdl == NULL) {
		ctl_sie_dbg_wrn("no free isp hdl\r\n");
		return CTL_SIE_E_HDL;
	}

	/* strncpy may not have null terminator,
		dont copy full length incase overwrite the last null terminator */
	strncpy(p_hdl->name, name, (sizeof(p_hdl->name) - 1));
	p_hdl->fp = fp;
	p_hdl->evt = evt;
	p_hdl->cb_msg = cb_msg;

	//update sie intee
	for (i = 0; i <= sie_limit->max_spt_id; i++) {
		ctl_sie_update_inte(i, ENABLE, __ctl_sie_isp_evt_conv2_sts(evt));
	}

	return CTL_SIE_E_OK;
}

INT32 ctl_sie_isp_evt_fp_unreg(CHAR *name)
{
	UINT32 i, j;
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(CTL_SIE_ID_1);
	BOOL unreg_done = FALSE;

	for (i = 0; i <= sie_limit->max_spt_id; i++) {
		if (strcmp(ctl_sie_isp_handle[i].name, name) == 0) {
			__ctl_sie_isp_handle_release(&ctl_sie_isp_handle[i]);
			for (j = 0; j <= sie_limit->max_spt_id; j++) {
				ctl_sie_update_inte(j, DISABLE, __ctl_sie_isp_evt_conv2_sts(ctl_sie_isp_handle[i].evt));
			}
			unreg_done = TRUE;
		}
	}

	if (unreg_done) {
		return CTL_SIE_E_OK;
	}
	return CTL_SIE_E_PAR;
}

INT32 ctl_sie_isp_set(ISP_ID id, CTL_SIE_ISP_ITEM item, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_ID sie_id = __ctl_sie_isp_conv2_sie_id(id);
	UINT32 isr_ctx_chk = (CTL_SIE_ISP_ISR_CTX & item);
	CTL_SIE_HDL *hdl;
	CTL_SIE_LIMIT *sie_limit;

	hdl = ctl_sie_get_hdl(sie_id);
	sie_limit = ctl_sie_limit_query(sie_id);

	if (hdl == NULL) {
		ctl_sie_dbg_wrn("NULL SIE HDL\r\n");
		return CTL_SIE_E_HDL;
	}

	item &= (~CTL_SIE_ISP_ISR_CTX);
	if (sie_id > sie_limit->max_spt_id) {
		ctl_sie_dbg_err("illegal sie_id %d > %d\r\n", (int)(sie_id), (int)sie_limit->max_spt_id);
		return CTL_SIE_E_ID;
	}

	if (item >= CTL_SIE_ISP_ITEM_MAX) {
		ctl_sie_dbg_err("isp_id %d, illegal item %d > %d\r\n", (int)(id), (int)(item), (int)(CTL_SIE_ISP_ITEM_MAX));
		return CTL_SIE_E_NOSPT;
	}

	if (data == NULL) {
		ctl_sie_dbg_err("data NULL, isp_id %d, item %d\r\n", (int)(id), (int)(item));
		return CTL_SIE_E_HDL;
	}

	if (item != CTL_SIE_ISP_ITEM_SIM_BUF_NEW && item != CTL_SIE_ISP_ITEM_SIM_BUF_PUSH) {
		if (!isr_ctx_chk) {
			if (__ctl_sie_isp_lock(sie_id, CTL_SIE_ISP_CFG_LOCK, TRUE) != CTL_SIE_E_OK) {
				return CTL_SIE_E_TMOUT;
			}
		}
	}

	vos_flag_iclr(ctl_sie_get_flag_id(id), CTL_SIE_FLG_ISP_PRC_END);
	if (ctl_sie_get_state_machine(id) != CTL_SIE_STS_CLOSE) {
		// check set function and keep in ctrl layer
		if (ctl_sie_isp_set_fp[item] != NULL) {
			rt = ctl_sie_isp_set_fp[item](sie_id, data);
		} else {
			rt = CTL_SIE_E_NOSPT;
			ctl_sie_dbg_wrn("id %d, fp null, item %d\r\n", (int)id, (int)(item));
		}
	}
	vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_ISP_PRC_END);

	if (item != CTL_SIE_ISP_ITEM_SIM_BUF_NEW && item != CTL_SIE_ISP_ITEM_SIM_BUF_PUSH) {
		if (!isr_ctx_chk) {
			if (__ctl_sie_isp_lock(sie_id, CTL_SIE_ISP_CFG_LOCK, FALSE) != CTL_SIE_E_OK) {
				return CTL_SIE_E_TMOUT;
			}
		}
	}
	ctl_sie_dbg_trc("[ISP][S]isp_id:%d item:%d\r\n", (int)id, (int)item);
	return rt;
}

INT32 ctl_sie_isp_get(ISP_ID id, CTL_SIE_ISP_ITEM item, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_ID sie_id = __ctl_sie_isp_conv2_sie_id(id);
	UINT32 isr_ctx_chk = (CTL_SIE_ISP_ISR_CTX & item);
	CTL_SIE_HDL *hdl;
	CTL_SIE_LIMIT *sie_limit;

	hdl = ctl_sie_get_hdl(sie_id);
	sie_limit = ctl_sie_limit_query(sie_id);
	if (hdl == NULL) {
		ctl_sie_dbg_wrn("NULL SIE HDL\r\n");
		return CTL_SIE_E_HDL;
	}

	item &= (~CTL_SIE_ISP_ISR_CTX);
	if (sie_id > sie_limit->max_spt_id) {
		ctl_sie_dbg_err("illegal sie_id %d > %d\r\n", (int)(sie_id), (int)sie_limit->max_spt_id);
		return CTL_SIE_E_ID;
	}

	if (item >= CTL_SIE_ISP_ITEM_MAX) {
		ctl_sie_dbg_err("isp_id %d, illegal item %d > %d\r\n", (int)(id), (int)(item), (int)(CTL_SIE_ISP_ITEM_MAX));
		return CTL_SIE_E_NOSPT;
	}

	if (data == NULL) {
		ctl_sie_dbg_err("data NULL, isp_id %d, item %d\r\n", (int)(id), (int)(item));
		return CTL_SIE_E_HDL;
	}

	if (!isr_ctx_chk) {
		if (__ctl_sie_isp_lock(sie_id, CTL_SIE_ISP_CFG_LOCK, TRUE) != CTL_SIE_E_OK) {
			return CTL_SIE_E_TMOUT;
		}
	}
	// check set function
	if (ctl_sie_isp_get_fp[item] != NULL) {
		rt = ctl_sie_isp_get_fp[item](sie_id, data);
	} else {
		rt = CTL_SIE_E_NOSPT;
		ctl_sie_dbg_wrn("fp  null, item %d\r\n", (int)(item));
	}
	if (!isr_ctx_chk) {
		if (__ctl_sie_isp_lock(sie_id, CTL_SIE_ISP_CFG_LOCK, FALSE) != CTL_SIE_E_OK) {
			return CTL_SIE_E_TMOUT;
		}
	}
	ctl_sie_dbg_trc("[ISP][G]isp_id:%d item:%d\r\n", (int)id, (int)item);
	return rt;
}

BOOL ctl_sie_isp_int_pull_img_out(UINT32 id, UINT32 sie_header_addr)
{
	if (vos_flag_chk(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_GET_IMG_END) & CTL_SIE_ISP_GET_IMG_END) {
		return FALSE;
	}

	if (ctl_sie_isp_get_img_info.id == id && sie_header_addr != 0) {
		ctl_sie_isp_get_img_info.sie_head_info = *(CTL_SIE_HEADER_INFO *)sie_header_addr;
		if (ctl_sie_isp_get_img_info.sie_head_info.vdo_frm.addr[0] != 0) {
			vos_flag_iset(g_ctl_sie_isp_flg_id, CTL_SIE_ISP_GET_IMG_READY);
			return TRUE;
		}
	}
	return FALSE;
}

void ctl_sie_isp_set_skip(UINT32 id, BOOL skip_en, UINT32 iq_func_off)
{
	ctl_sie_isp_dbg_info[id].skip_cb_en = skip_en;
	ctl_sie_isp_dbg_info[id].iq_func_off = iq_func_off;
}

CTL_SIE_ISP_HANDLE *ctl_sie_isp_get_hdl(void)
{
	return &ctl_sie_isp_handle[0];
}

void ctl_sie_isp_rst_to_dft(UINT32 id)
{
	isp_ca_rslt[id].src_addr = 0;
	isp_la_rslt[id].src_addr = 0;
}

