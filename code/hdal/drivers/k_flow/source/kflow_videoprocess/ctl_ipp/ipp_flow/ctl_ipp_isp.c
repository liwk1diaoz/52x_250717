#include "kflow_videoprocess/ctl_ipp_isp.h"
#include "ctl_ipp_isp_int.h"
#include "ctl_ipp_buf_int.h"
#include "ctl_ipp_id_int.h"
#include "kdf_ipp_int.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

#define CTL_IPP_ISP_INT_STATE_FREE		(0)
#define CTL_IPP_ISP_INT_STATE_LOCKED	(1)

#define CTL_IPP_ISP_ID_UNUSED	(0xFFFFFFFF)
#define CTL_IPP_ISP_ID_UNUSED_2	(0x0000FFFF)

static CTL_IPP_ISP_HANDLE ctl_ipp_isp_handle[CTL_IPP_ISP_HANDLE_MAX_NUMBER];
static UINT32 ctl_ipp_isp_max_id_num;
static CTL_IPP_ISP_INT_INFO *ctl_ipp_isp_int_info;
static CTL_IPP_ISP_YUV_OUT_INFO ctl_ipp_isp_yuv_out_info;
static UINT32 ctl_ipp_isp_init_sts = FALSE;

#if CTL_IPP_ISP_YUVOUT_DEBUG_EN
static CTL_IPP_ISP_YUVOUT_LOG ctl_ipp_isp_yuvout_log[CTL_IPP_ISP_YUVOUT_DEBUG_NUM];
#endif

static UINT32 ctl_ipp_isp_dbg_cbtime_catch_number;
static UINT32 ctl_ipp_isp_dbg_cbtime_threshold;
#if CTL_IPP_ISP_CBTIME_DEBUG_EN
static CTL_IPP_ISP_CBTIME_LOG ctl_ipp_isp_dbg_cbtime_log[CTL_IPP_ISP_CBTIME_DEBUG_NUM];
static UINT32 ctl_ipp_isp_dbg_cbtime_cnt;
#endif


static BOOL ctl_ipp_isp_is_init(void)
{
	unsigned long loc_cpu_flg;
	BOOL rt;

	loc_cpu(loc_cpu_flg);
	rt = ctl_ipp_isp_init_sts;
	unl_cpu(loc_cpu_flg);

	return rt;
}

static void ctl_ipp_isp_handle_reset(CTL_IPP_ISP_HANDLE *p_hdl)
{
	memset((void *)&p_hdl->name[0], '\0', sizeof(CHAR) * CTL_IPP_ISP_HANDLE_NAME_MAX_LENGTH);
	p_hdl->fp = NULL;
	p_hdl->evt = 0;
	p_hdl->cb_msg = CTL_IPP_ISP_CB_MSG_NONE;
	p_hdl->status = CTL_IPP_ISP_HANDLE_STATUS_FREE;
}

static CTL_IPP_ISP_HANDLE *ctl_ipp_isp_handle_alloc(void)
{
	UINT32 i;

	for (i = 0; i < CTL_IPP_ISP_HANDLE_MAX_NUMBER; i++) {
		if (ctl_ipp_isp_handle[i].status == CTL_IPP_ISP_HANDLE_STATUS_FREE) {
			ctl_ipp_isp_handle[i].status = CTL_IPP_ISP_HANDLE_STATUS_USED;
			return &ctl_ipp_isp_handle[i];
		}
	}

	return NULL;
}

static void ctl_ipp_isp_handle_release(CTL_IPP_ISP_HANDLE *p_hdl)
{
	ctl_ipp_isp_handle_reset(p_hdl);
}

ER ctl_ipp_isp_evt_fp_reg(CHAR *name, ISP_EVENT_FP fp, ISP_EVENT evt, CTL_IPP_ISP_CB_MSG cb_msg)
{
	CTL_IPP_ISP_HANDLE *p_hdl;

	p_hdl = ctl_ipp_isp_handle_alloc();
	if (p_hdl == NULL) {
		CTL_IPP_DBG_WRN("no free isp handle\r\n");
		return E_NOMEM;
	}

	/* strncpy may not have null terminator,
		dont copy full length incase overwrite the last null terminator */
	strncpy(p_hdl->name, name, (CTL_IPP_ISP_HANDLE_NAME_MAX_LENGTH - 1));
	p_hdl->fp = fp;
	p_hdl->evt = evt;
	p_hdl->cb_msg = cb_msg;

	return E_OK;
}

ER ctl_ipp_isp_evt_fp_unreg(CHAR *name)
{
	UINT32 i;

	for (i = 0; i < CTL_IPP_ISP_HANDLE_MAX_NUMBER; i++) {
		if (strcmp(ctl_ipp_isp_handle[i].name, name) == 0) {
			ctl_ipp_isp_handle_release(&ctl_ipp_isp_handle[i]);
		}
	}

	return E_OK;
}

ER ctl_ipp_isp_evt_fp_dbg_mode(UINT32 dbg_mode)
{
	static void* tmp_hdl_buf = NULL;
	CTL_IPP_ISP_HANDLE *p_hdl;
	UINT32 i;

	if (dbg_mode == 0) {
		/* restore orignal isp */
		if (tmp_hdl_buf != NULL) {
			p_hdl = (CTL_IPP_ISP_HANDLE *)tmp_hdl_buf;
			for (i = 0; i < CTL_IPP_ISP_HANDLE_MAX_NUMBER; i++) {
				ctl_ipp_isp_handle[i] = p_hdl[i];
			}
			ctl_ipp_util_os_mfree_wrap(tmp_hdl_buf);
			tmp_hdl_buf = NULL;

			DBG_DUMP("ISP DBGMODE = OFF, Restore all isp\r\n");
		}
	} else {
		/* unreg all isp */
		if (tmp_hdl_buf == NULL) {
			tmp_hdl_buf = ctl_ipp_util_os_malloc_wrap(sizeof(CTL_IPP_ISP_HANDLE) * CTL_IPP_ISP_HANDLE_MAX_NUMBER);
			if (tmp_hdl_buf == NULL) {
				DBG_DUMP("ISP DBGMODE Changed failed\r\n");
				return E_SYS;
			}

			p_hdl = (CTL_IPP_ISP_HANDLE *)tmp_hdl_buf;
			for (i = 0; i < CTL_IPP_ISP_HANDLE_MAX_NUMBER; i++) {
				p_hdl[i] = ctl_ipp_isp_handle[i];
				ctl_ipp_isp_handle_release(&ctl_ipp_isp_handle[i]);
			}

			DBG_DUMP("ISP DBGMODE = ON, Remove all isp\r\n");
		} else {
			DBG_DUMP("ISP DBGMODE already ON, do nothing\r\n");
			return E_SYS;
		}
	}

	return E_OK;
}

#if 0
#endif

_INLINE CTL_IPP_ISP_INT_INFO *ctl_ipp_isp_get_int_info(UINT32 id)
{
	unsigned long loc_flg;
	UINT32 i;

	loc_cpu(loc_flg);
	for (i = 0; i < ctl_ipp_isp_max_id_num; i++) {
		if (ctl_ipp_isp_int_info[i].isp_id == id) {
			unl_cpu(loc_flg);
			return &ctl_ipp_isp_int_info[i];
		}
	}
	unl_cpu(loc_flg);

	return NULL;
}

_INLINE void ctl_ipp_isp_cfg_lock(void)
{
	/* this lock is used to prevent iq set parameters to kdriver during ipp flow cfg_start ~ cfg_end */
	FLGPTN flg;

	vos_flag_wait(&flg, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_CFG_LOCK, TWF_CLR);
}

_INLINE void ctl_ipp_isp_cfg_unlock(void)
{
	vos_flag_set(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_CFG_LOCK);
}

_INLINE void ctl_ipp_isp_get_yuv_start(void)
{
	FLGPTN flg;

	vos_flag_wait(&flg, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_GET_YUV_END, TWF_CLR);
	vos_flag_clr(g_ctl_ipp_isp_flg_id, (CTL_IPP_ISP_GET_YUV_READY | CTL_IPP_ISP_GET_YUV_TIMEOUT));
}

_INLINE void ctl_ipp_isp_get_yuv_end(void)
{
	vos_flag_clr(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_GET_YUV_READY);
	vos_flag_set(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_GET_YUV_END);
}

_INLINE ER ctl_ipp_isp_get_yuv_wait_ready(void)
{
	#define CTL_IPP_ISP_WAIT_YUV_TIMEOUT_MS	(500)
	FLGPTN flg = 0;
	ER rt;

	rt = vos_flag_wait_timeout(&flg, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_GET_YUV_READY, TWF_ORW, vos_util_msec_to_tick(CTL_IPP_ISP_WAIT_YUV_TIMEOUT_MS));
	if (rt != E_OK) {
		return E_TMOUT;
	}

	return E_OK;
}

_INLINE ISP_EVENT ctl_ipp_isp_get_imm_event(ISP_EVENT evt)
{
	switch (evt) {
	case ISP_EVENT_IPP_CFGSTART:
		return ISP_EVENT_IPP_CFGSTART_IMM;

	case ISP_EVENT_IPP_CFGEND:
		return ISP_EVENT_IPP_CFGEND_IMM;

	case ISP_EVENT_IPP_PROCEND:
		return ISP_EVENT_IPP_PROCEND_IMM;

	default:
		return ISP_EVENT_NONE;
	}
}

static void ctl_ipp_isp_dbg_set_yuvout_log(UINT32 isp_id, CTL_IPP_ISP_YUV_OUT *p_yuv, UINT32 unlock)
{
#if CTL_IPP_ISP_YUVOUT_DEBUG_EN
	UINT32 i;

	if (unlock) {
		for (i = 0; i < CTL_IPP_ISP_YUVOUT_DEBUG_NUM; i++) {
			if (ctl_ipp_isp_yuvout_log[i].ispid == isp_id && ctl_ipp_isp_yuvout_log[i].buf_addr == p_yuv->buf_addr) {
				ctl_ipp_isp_yuvout_log[i].unlock_ts = ctl_ipp_util_get_syst_counter();
			}
		}
	} else {
		for (i = 0; i < CTL_IPP_ISP_YUVOUT_DEBUG_NUM; i++) {
			if (ctl_ipp_isp_yuvout_log[i].buf_addr == 0) {
				break;
			}
		}
		if (i >= CTL_IPP_ISP_YUVOUT_DEBUG_NUM) {
			for (i = 0; i < CTL_IPP_ISP_YUVOUT_DEBUG_NUM; i++) {
				if (ctl_ipp_isp_yuvout_log[i].unlock_ts != 0) {
					break;
				}
			}
		}

		if (i < CTL_IPP_ISP_YUVOUT_DEBUG_NUM) {
			memset((void *)&ctl_ipp_isp_yuvout_log[i], 0, sizeof(CTL_IPP_ISP_YUVOUT_LOG));
			ctl_ipp_isp_yuvout_log[i].ispid = isp_id;
			ctl_ipp_isp_yuvout_log[i].pid = p_yuv->pid;
			ctl_ipp_isp_yuvout_log[i].buf_addr = p_yuv->buf_addr;
			ctl_ipp_isp_yuvout_log[i].buf_id = p_yuv->buf_id;
			ctl_ipp_isp_yuvout_log[i].buf_size = p_yuv->buf_size;
			ctl_ipp_isp_yuvout_log[i].buf_addr = p_yuv->buf_addr;
			ctl_ipp_isp_yuvout_log[i].lock_ts = ctl_ipp_util_get_syst_counter();
		}
	}
#endif
}

static void ctl_ipp_isp_dbg_set_cbtime_log(UINT32 id, ISP_EVENT evt, UINT32 fc, UINT64 ts_start, UINT64 ts_end)
{
	UINT64 cb_proc_time;

	cb_proc_time = ts_end - ts_start;
	if (ctl_ipp_isp_dbg_cbtime_catch_number > 0 &&
		cb_proc_time > ctl_ipp_isp_dbg_cbtime_threshold) {
		CTL_IPP_DBG_ERR("ispid %d, event 0x%.8x, cb_proc_time %llu(us)\r\n", (int)id, (unsigned int)evt, cb_proc_time);
		ctl_ipp_isp_dbg_cbtime_catch_number -= 1;
	}
#if CTL_IPP_ISP_CBTIME_DEBUG_EN
	{
		unsigned long loc_cpu_flg;
		UINT32 idx;

		loc_cpu(loc_cpu_flg);
		idx = ctl_ipp_isp_dbg_cbtime_cnt % CTL_IPP_ISP_CBTIME_DEBUG_NUM;
		ctl_ipp_isp_dbg_cbtime_log[idx].ispid = id;
		ctl_ipp_isp_dbg_cbtime_log[idx].evt = evt;
		ctl_ipp_isp_dbg_cbtime_log[idx].raw_fc = fc;
		ctl_ipp_isp_dbg_cbtime_log[idx].ts_start = ts_start;
		ctl_ipp_isp_dbg_cbtime_log[idx].ts_end = ts_end;
		ctl_ipp_isp_dbg_cbtime_cnt++;
		unl_cpu(loc_cpu_flg);
	}
#endif
}

#if 0
#endif

static INT32 ctl_ipp_isp_set_rhe_iq(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
#if 0
	ER rt;
	CTL_IPP_HANDLE *p_hdl;
	KDF_IPP_HANDLE *p_kdf_hdl;

	p_hdl = ctl_ipp_get_hdl_by_ispid(id);
	if (p_hdl == NULL || p_hdl->kdf_hdl == 0) {
		return E_SYS;
	}

	rt = E_OK;
	p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;

	if (data != NULL) {
		ctl_ipp_isp_cfg_lock();
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_RHE, p_kdf_hdl, (KDRV_RHE_PARAM_IQ_ALL | KDRV_RHE_PARAM_REV), data);
		ctl_ipp_isp_cfg_unlock();
	}

	return rt;
#else
	return 0;
#endif
}

static INT32 ctl_ipp_isp_set_ife_iq(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	ER rt;
	CTL_IPP_HANDLE *p_hdl;
	KDF_IPP_HANDLE *p_kdf_hdl;
	UINT32 imm_bit;

	p_hdl = ctl_ipp_get_hdl_by_ispid(id);
	if (p_hdl == NULL || p_hdl->kdf_hdl == 0) {
		return E_SYS;
	}

	rt = E_OK;
	p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;
	imm_bit = item & CTL_IPP_ISP_ITEM_IMM_BIT;

	if (data != NULL) {
		if (imm_bit == 0) {
			ctl_ipp_isp_cfg_lock();
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_kdf_hdl, (KDRV_IFE_PARAM_IQ_ALL | KDRV_IFE_PARAM_REV), data);

		if (imm_bit == 0) {
			ctl_ipp_isp_cfg_unlock();
		}
	}

	return rt;
}

static INT32 ctl_ipp_isp_set_dce_iq(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	ER rt;
	CTL_IPP_HANDLE *p_hdl;
	KDF_IPP_HANDLE *p_kdf_hdl;
	UINT32 imm_bit;

	p_hdl = ctl_ipp_get_hdl_by_ispid(id);
	if (p_hdl == NULL || p_hdl->kdf_hdl == 0) {
		return E_SYS;
	}

	rt = E_OK;
	p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;
	imm_bit = item & CTL_IPP_ISP_ITEM_IMM_BIT;

	if (data != NULL) {
		if (imm_bit == 0) {
			ctl_ipp_isp_cfg_lock();
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_kdf_hdl, (KDRV_DCE_PARAM_IQ_ALL | KDRV_DCE_PARAM_REV), data);

		if (imm_bit == 0) {
			ctl_ipp_isp_cfg_unlock();
		}
	}

	return rt;
}

static INT32 ctl_ipp_isp_set_ipe_iq(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	ER rt;
	CTL_IPP_HANDLE *p_hdl;
	KDF_IPP_HANDLE *p_kdf_hdl;
	UINT32 imm_bit;

	p_hdl = ctl_ipp_get_hdl_by_ispid(id);
	if (p_hdl == NULL || p_hdl->kdf_hdl == 0) {
		return E_SYS;
	}

	rt = E_OK;
	p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;
	imm_bit = item & CTL_IPP_ISP_ITEM_IMM_BIT;

	if (data != NULL) {
		if (imm_bit == 0) {
			ctl_ipp_isp_cfg_lock();
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_kdf_hdl, (KDRV_IPE_PARAM_IQ_ALL | KDRV_IPE_PARAM_REV), data);

		if (imm_bit == 0) {
			ctl_ipp_isp_cfg_unlock();
		}
	}

	return rt;
}

static INT32 ctl_ipp_isp_set_ime_iq(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	ER rt;
	CTL_IPP_HANDLE *p_hdl;
	KDF_IPP_HANDLE *p_kdf_hdl;
	UINT32 imm_bit;

	p_hdl = ctl_ipp_get_hdl_by_ispid(id);
	if (p_hdl == NULL || p_hdl->kdf_hdl == 0) {
		return E_SYS;
	}

	rt = E_OK;
	p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;
	imm_bit = item & CTL_IPP_ISP_ITEM_IMM_BIT;

	if (data != NULL) {
		if (imm_bit == 0) {
			ctl_ipp_isp_cfg_lock();
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_kdf_hdl, (KDRV_IME_PARAM_IQ_ALL | KDRV_IME_PARAM_REV), data);

		if (imm_bit == 0) {
			ctl_ipp_isp_cfg_unlock();
		}
	}

	return rt;
}

static INT32 ctl_ipp_isp_set_ife2_iq(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	ER rt;
	CTL_IPP_HANDLE *p_hdl;
	KDF_IPP_HANDLE *p_kdf_hdl;
	UINT32 imm_bit;

	p_hdl = ctl_ipp_get_hdl_by_ispid(id);
	if (p_hdl == NULL || p_hdl->kdf_hdl == 0) {
		return E_SYS;
	}

	rt = E_OK;
	p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;
	imm_bit = item & CTL_IPP_ISP_ITEM_IMM_BIT;

	if (data != NULL) {
		if (imm_bit == 0) {
			ctl_ipp_isp_cfg_lock();
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_kdf_hdl, (KDRV_IFE2_PARAM_IQ_ALL | KDRV_IFE2_PARAM_REV), data);

		if (imm_bit == 0) {
			ctl_ipp_isp_cfg_unlock();
		}
	}

	return rt;
}

static INT32 ctl_ipp_isp_set_ife_vig_cent(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	p_isp_info->ife_vig_cent_ratio = *(CTL_IPP_ISP_IFE_VIG_CENT_RATIO *)data;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_set_dce_dc_cent(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	p_isp_info->dce_dc_cent_ratio = *(CTL_IPP_ISP_DCE_DC_CENT_RATIO *)data;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_set_ime_lca_size(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	p_isp_info->ime_lca_size_ratio = *(CTL_IPP_ISP_IME_LCA_SIZE_RATIO *)data;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_set_ife2_filt_time(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	p_isp_info->ife2_filt_time = *(CTL_IPP_ISP_IFE2_FILTER_TIME *)data;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_set_yuv_out(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	CTL_IPP_HANDLE *p_hdl;
	CTL_IPP_ISP_YUV_OUT *p_yuv;

	p_hdl = ctl_ipp_get_hdl_by_ispid(id);
	p_yuv = (CTL_IPP_ISP_YUV_OUT *)data;

	if (p_hdl == NULL || p_yuv == NULL) {
		return E_SYS;
	}

	if (p_hdl->evt_outbuf_fp != NULL) {
		CTL_IPP_OUT_BUF_INFO buf = {0};

		buf.pid = p_yuv->pid;
		buf.buf_id = p_yuv->buf_id;
		buf.buf_size = p_yuv->buf_size;
		buf.buf_addr = p_yuv->buf_addr;
		buf.vdo_frm = p_yuv->vdo_frm;
		if (buf.buf_addr != 0) {
			p_hdl->evt_outbuf_fp(CTL_IPP_BUF_IO_UNLOCK, (void *)&buf, NULL);

			ctl_ipp_isp_dbg_set_yuvout_log(id, p_yuv, TRUE);
		}
	}
	return E_OK;
}

static INT32 ctl_ipp_isp_set_eth(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	p_isp_info->cur_eth = *(CTL_IPP_ISP_ETH *)data;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_set_va_win(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	p_isp_info->va_win_ratio = *(CTL_IPP_ISP_VA_WIN_SIZE_RATIO *)data;
	unl_cpu(loc_flg);

	return E_OK;
}

static CTL_IPP_ISP_SET_FP ctl_ipp_isp_set_fp[CTL_IPP_ISP_ITEM_MAX] = {
	NULL,
	NULL,
	ctl_ipp_isp_set_rhe_iq,
	ctl_ipp_isp_set_ife_iq,
	ctl_ipp_isp_set_dce_iq,
	ctl_ipp_isp_set_ipe_iq,
	ctl_ipp_isp_set_ime_iq,
	ctl_ipp_isp_set_ife2_iq,
	NULL,
	ctl_ipp_isp_set_ife_vig_cent,
	ctl_ipp_isp_set_dce_dc_cent,
	ctl_ipp_isp_set_ime_lca_size,
	ctl_ipp_isp_set_ife2_filt_time,
	NULL,
	ctl_ipp_isp_set_yuv_out,
	NULL,
	ctl_ipp_isp_set_eth,
	NULL,
	NULL,
	ctl_ipp_isp_set_va_win,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

ER ctl_ipp_isp_set(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	ER rt;
	CTL_IPP_ISP_ITEM idx;

	if (ctl_ipp_isp_is_init() == FALSE) {
		CTL_IPP_DBG_WRN("ctl_ipp_isp not init yet\r\n");
		return E_SYS;
	}

	if (ctl_ipp_isp_int_id_validate(id) == FALSE) {
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISP, "invalid isp_id 0x%.8x\r\n", id);
		return E_PAR;
	}

	idx = item & (~CTL_IPP_ISP_ITEM_IMM_BIT);
	if (idx >= CTL_IPP_ISP_ITEM_MAX) {
		CTL_IPP_DBG_WRN("unknown item %d\r\n", (int)(idx));
		return E_PAR;
	}

	rt = E_OK;
	if (ctl_ipp_isp_set_fp[idx] != NULL) {
		rt = ctl_ipp_isp_set_fp[idx](id, item, data);
	}

	return rt;
}

#if 0
#endif

static INT32 ctl_ipp_isp_get_func_en(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;
	CTL_IPP_INFO_LIST_ITEM *p_info;
	CTL_IPP_HANDLE *p_hdl;
	ISP_FUNC_EN func_en;

	p_isp_info = ctl_ipp_isp_get_int_info(id);

	loc_cpu(loc_flg);
	if (p_isp_info == NULL || p_isp_info->p_ctrl_info == NULL) {
		unl_cpu(loc_flg);

		/* if ctrl_info not exist, use handle to get func_en */
		p_hdl = ctl_ipp_get_hdl_by_ispid(id);
		if (p_hdl) {
			func_en = 0;

			if (p_hdl->func_en & CTL_IPP_FUNC_SHDR) {
				func_en |= ISP_FUNC_EN_SHDR;
			}
			if (p_hdl->func_en & CTL_IPP_FUNC_WDR) {
				func_en |= ISP_FUNC_EN_WDR;
			}
			if (p_hdl->func_en & CTL_IPP_FUNC_DEFOG) {
				func_en |= ISP_FUNC_EN_DEFOG;
			}
			if (p_hdl->func_en & CTL_IPP_FUNC_VA_SUBOUT) {
				func_en |= ISP_FUNC_EN_AF;
			}
			if (p_hdl->func_en & CTL_IPP_FUNC_GDC) {
				func_en |= ISP_FUNC_EN_GDC;
			}
		} else {
			return E_SYS;
		}
	} else {
		p_info = p_isp_info->p_ctrl_info;

		func_en = 0;

		if (p_info->info.ife_ctrl.shdr_enable) {
			func_en |= ISP_FUNC_EN_SHDR;
		}
		if (p_info->info.dce_ctrl.wdr_enable) {
			func_en |= ISP_FUNC_EN_WDR;
		}
		if (p_info->info.ipe_ctrl.defog_enable) {
			func_en |= ISP_FUNC_EN_DEFOG;
		}
		if (p_info->info.ipe_ctrl.va_enable) {
			func_en |= ISP_FUNC_EN_AF;
		}
		if (p_info->info.dce_ctrl.gdc_enable) {
			func_en |= ISP_FUNC_EN_GDC;
		}

		unl_cpu(loc_flg);
	}

	*(ISP_FUNC_EN *)data = func_en;

	return E_OK;
}

static INT32 ctl_ipp_isp_get_iosize(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	UINT32 i;
	CTL_IPP_ISP_INT_INFO *p_isp_info;
	CTL_IPP_INFO_LIST_ITEM *p_info;
	CTL_IPP_ISP_IOSIZE *p_iosize;
	CTL_IPP_HANDLE *p_hdl;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	p_hdl = ctl_ipp_get_hdl_by_ispid(id);

	loc_cpu(loc_flg);
	if (p_isp_info == NULL || p_isp_info->p_ctrl_info == NULL || p_hdl == NULL) {
		unl_cpu(loc_flg);
		return E_SYS;
	}
	p_info = p_isp_info->p_ctrl_info;

	p_iosize = (CTL_IPP_ISP_IOSIZE *)data;
	p_iosize->max_in_sz.w = p_hdl->private_buf.buf_info.max_size.w;
	p_iosize->max_in_sz.h = p_hdl->private_buf.buf_info.max_size.h;
	p_iosize->in_sz.w = p_info->info.ife_ctrl.in_crp_window.w;
	p_iosize->in_sz.h = p_info->info.ife_ctrl.in_crp_window.h;
	for (i = 0; i < CTL_IPP_ISP_OUT_CH_MAX_NUM; i++) {
		if (p_info->info.ime_ctrl.out_img[i].enable == ENABLE) {
			p_iosize->out_ch[i].w = p_info->info.ime_ctrl.out_img[i].crp_window.w;
			p_iosize->out_ch[i].h = p_info->info.ime_ctrl.out_img[i].crp_window.h;
		} else {
			p_iosize->out_ch[i].w = 0;
			p_iosize->out_ch[i].h = 0;
		}
	}
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_yout(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
#if 0
	unsigned long loc_flg;
	UINT32 i, buf_addr;
	CTL_IPP_ISP_INT_INFO *p_isp_info;
	CTL_IPP_INFO_LIST_ITEM *p_info;
	CTL_IPP_ISP_YOUT *p_yout;

	p_isp_info = ctl_ipp_isp_get_int_info(id);

	loc_cpu(loc_flg);
	if (p_isp_info == NULL || p_isp_info->p_ctrl_info == NULL) {
		unl_cpu(loc_flg);
		return E_SYS;
	}
	p_info = p_isp_info->p_ctrl_info;

	p_yout = (CTL_IPP_ISP_YOUT *)data;
	memset((void *)p_yout, 0, sizeof(CTL_IPP_ISP_YOUT));

	if (p_info->info.rhe_ctrl.func_mode != CTL_IPP_RHE_FUNC_MODE_PHDR &&
		p_info->info.rhe_ctrl.func_mode != CTL_IPP_RHE_FUNC_MODE_SHDR &&
		p_info->info.rhe_ctrl.func_mode != CTL_IPP_RHE_FUNC_MODE_SHDR_WITH_PHDR) {
		unl_cpu(loc_flg);
		return E_OK;
	}

	p_yout->frame_num = VDO_PXLFMT_PLANE(p_info->info.rhe_ctrl.in_fmt);
	if (p_yout->frame_num > 4) {
		p_yout->frame_num = 4;
	}
	p_yout->win_size = p_info->info.rhe_ctrl.subin_img_size;
	p_yout->blk_size = p_info->info.rhe_ctrl.subin_blk_size;
	p_yout->lofs = p_info->info.rhe_ctrl.subin_lofs;

	/* yout buffer will be overwrite when ipp update information */
	buf_addr = 0;
	for (i = 0; i < p_yout->frame_num; i++) {
		buf_addr = ctl_ipp_buf_task_alloc(CTRL_IPP_BUF_ITEM_YOUT, (UINT32)p_info, buf_addr, p_yout->win_size);
		if (buf_addr == 0) {
			break;
		}

		p_yout->addr[i] = buf_addr;
	}
	unl_cpu(loc_flg);
#endif

	return E_OK;
}

static INT32 ctl_ipp_isp_get_ife_vig_cent(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	*(CTL_IPP_ISP_IFE_VIG_CENT_RATIO *)data = p_isp_info->ife_vig_cent_ratio;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_dce_dc_cent(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	*(CTL_IPP_ISP_DCE_DC_CENT_RATIO *)data = p_isp_info->dce_dc_cent_ratio;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_ime_lca_size(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	*(CTL_IPP_ISP_IME_LCA_SIZE_RATIO *)data = p_isp_info->ime_lca_size_ratio;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_ife2_filt_time(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	*(CTL_IPP_ISP_IFE2_FILTER_TIME *)data = p_isp_info->ife2_filt_time;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_defog_subout(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;
	CTL_IPP_INFO_LIST_ITEM *p_info;
	CTL_IPP_ISP_DEFOG_SUBOUT *p_defog_subout;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	loc_cpu(loc_flg);
	if (p_isp_info == NULL || p_isp_info->p_ctrl_info == NULL) {
		unl_cpu(loc_flg);
		return E_SYS;
	}
	p_info = p_isp_info->p_ctrl_info;
	p_defog_subout = (CTL_IPP_ISP_DEFOG_SUBOUT *)data;
	p_defog_subout->addr = p_info->info.ipe_ctrl.defog_in_addr;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_yuv_out(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	ER rt;
	CTL_IPP_HANDLE *p_hdl;
	CTL_IPP_ISP_YUV_OUT *p_yuv;

	p_hdl = ctl_ipp_get_hdl_by_ispid(id);
	p_yuv = (CTL_IPP_ISP_YUV_OUT *)data;

	if (p_hdl == NULL || p_yuv == NULL) {
		return E_SYS;
	}

	if (item & CTL_IPP_ISP_ITEM_IMM_BIT) {
		CTL_IPP_DBG_WRN("get yuv not support imm_bit\r\n");
		return E_NOSPT;
	}

	if (p_yuv->pid >= CTL_IPP_YUV_OUT_PATH_ID_MAX) {
		CTL_IPP_DBG_WRN("pid overflow %d\r\n", (int)(p_yuv->pid));
		return E_PAR;
	}

	/* prevent stuck other set/get api */
	memset((void *)&ctl_ipp_isp_yuv_out_info, 0, sizeof(CTL_IPP_ISP_YUV_OUT_INFO));
	ctl_ipp_isp_yuv_out_info.handle = (UINT32)p_hdl;
	ctl_ipp_isp_yuv_out_info.yuv_out.pid = p_yuv->pid;
	ctl_ipp_isp_get_yuv_start();
	rt = ctl_ipp_isp_get_yuv_wait_ready();

	if (rt == E_OK) {
		*p_yuv = ctl_ipp_isp_yuv_out_info.yuv_out;

		ctl_ipp_isp_dbg_set_yuvout_log(id, p_yuv, FALSE);
	} else {
		p_yuv->buf_addr = 0;
		p_yuv->buf_id = 0;
		p_yuv->buf_size = 0;
		memset((void *)&p_yuv->vdo_frm, 0, sizeof(VDO_FRAME));
	}

	ctl_ipp_isp_get_yuv_end();
	return rt;
}

static INT32 ctl_ipp_isp_get_status_info(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	CTL_IPP_ISP_STATUS_INFO *p_status;

	p_status = (CTL_IPP_ISP_STATUS_INFO *)data;

	switch (ctl_ipp_get_hdl_sts_by_ispid(id)) {
	case CTL_IPP_HANDLE_STS_READY:
	case CTL_IPP_HANDLE_STS_DIR_START:
	case CTL_IPP_HANDLE_STS_DIR_READY:
		p_status->sts = CTL_IPP_ISP_STS_READY;
		break;

	case CTL_IPP_HANDLE_STS_FREE:
		p_status->sts = CTL_IPP_ISP_STS_CLOSE;
		break;

	case 0:
		p_status->sts = CTL_IPP_ISP_STS_ID_NOT_FOUND;
		break;

	default:
		p_status->sts = CTL_IPP_ISP_STS_CLOSE;
		break;
	}

	p_status->flow = CTL_IPP_ISP_FLOW_UNKNOWN;
	if (p_status->sts == CTL_IPP_ISP_STS_READY ||
		p_status->sts == CTL_IPP_ISP_STS_RUN) {
		CTL_IPP_HANDLE *p_hdl;

		p_hdl = ctl_ipp_get_hdl_by_ispid(id);

		if (p_hdl != NULL) {
			switch (p_hdl->flow) {
			case CTL_IPP_FLOW_RAW:
				p_status->flow = CTL_IPP_ISP_FLOW_RAW;
				break;

			case CTL_IPP_FLOW_DIRECT_RAW:
				p_status->flow = CTL_IPP_ISP_FLOW_DIRECT_RAW;
				break;

			case CTL_IPP_FLOW_CCIR:
				p_status->flow = CTL_IPP_ISP_FLOW_CCIR;
				break;

			case CTL_IPP_FLOW_IME_D2D:
				p_status->flow = CTL_IPP_ISP_FLOW_IME_D2D;
				break;

			case CTL_IPP_FLOW_IPE_D2D:
				p_status->flow = CTL_IPP_ISP_FLOW_IPE_D2D;
				break;

			case CTL_IPP_FLOW_DCE_D2D:
				p_status->flow = CTL_IPP_ISP_FLOW_DCE_D2D;
				break;

			case CTL_IPP_FLOW_VR360:
				p_status->flow = CTL_IPP_ISP_FLOW_VR360;
				break;

			case CTL_IPP_FLOW_CAPTURE_RAW:
				p_status->flow = CTL_IPP_ISP_FLOW_CAPTURE_RAW;
				break;

			case CTL_IPP_FLOW_CAPTURE_CCIR:
				p_status->flow = CTL_IPP_ISP_FLOW_CAPTURE_CCIR;
				break;

			default:
				p_status->flow = CTL_IPP_ISP_FLOW_UNKNOWN;
				break;
			}
		}
	}

	return E_OK;
}

static INT32 ctl_ipp_isp_get_eth(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	*(CTL_IPP_ISP_ETH *)data = p_isp_info->rdy_eth;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_kdrv_param(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	KDF_IPP_ENG eng_map_tbl[CTL_IPP_ISP_ENG_MAX] = {
		KDF_IPP_ENG_RHE,
		KDF_IPP_ENG_IFE,
		KDF_IPP_ENG_DCE,
		KDF_IPP_ENG_IPE,
		KDF_IPP_ENG_IME,
		KDF_IPP_ENG_IFE2,
	};
	UINT32 eng_isr_param_bit[CTL_IPP_ISP_ENG_MAX] = {
		0, /*KDRV_RHE_PARAM_REV,*/
		KDRV_IFE_PARAM_REV,
		KDRV_DCE_PARAM_REV,
		KDRV_IPE_PARAM_REV,
		KDRV_IME_PARAM_REV,
		KDRV_IFE2_PARAM_REV,
	};
	CTL_IPP_HANDLE *p_hdl;
	KDF_IPP_HANDLE *p_kdf_hdl;
	CTL_IPP_ISP_KDRV_PARAM *p_kdrv_param;
	UINT32 param_id;
	INT32 rt;

	p_hdl = ctl_ipp_get_hdl_by_ispid(id);
	if (p_hdl == NULL || p_hdl->kdf_hdl == 0) {
		return E_SYS;
	}

	p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;
	p_kdrv_param = (CTL_IPP_ISP_KDRV_PARAM *)data;
	if (p_kdrv_param->eng >= CTL_IPP_ISP_ENG_MAX) {
		return E_PAR;
	}
	param_id = p_kdrv_param->param_id | eng_isr_param_bit[p_kdrv_param->eng];
	rt = kdf_ipp_kdrv_get_wrapper(eng_map_tbl[p_kdrv_param->eng], p_kdf_hdl, param_id, p_kdrv_param->data);

	return rt;
}

static INT32 ctl_ipp_isp_get_3dnr_sta(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	#define CTL_IPP_ISP_WAIT_STA_TIMEOUT_MS	(500)
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;
	FLGPTN flg = 0;
	UINT32 buf_addr;
	ER rt;

	if (item & CTL_IPP_ISP_ITEM_IMM_BIT) {
		CTL_IPP_DBG_WRN("get 3dnr sta not support imm_bit\r\n");
		return E_NOSPT;
	}

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_SYS;
	}

	/* wait last get done */
	rt = vos_flag_wait_timeout(&flg, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_GET_3DNR_STA_READY, TWF_CLR, vos_util_msec_to_tick(CTL_IPP_ISP_WAIT_STA_TIMEOUT_MS));
	if (rt != E_OK) {
		CTL_IPP_DBG_WRN("isp_id %d wait sta buffer unlock timeout or failed\r\n", (int)id);
		return E_TMOUT;
	}

	/* get sta buffer */
	buf_addr = 0;
	if (p_isp_info->p_ctrl_info != NULL) {
		buf_addr = ctl_ipp_buf_task_alloc(CTRL_IPP_BUF_ITEM_3DNR_STA, (UINT32)p_isp_info->p_ctrl_info, 0, p_isp_info->p_ctrl_info->info.ime_ctrl.in_size);
	}

	if (buf_addr == 0) {
		vos_flag_iset(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_GET_3DNR_STA_READY);
		CTL_IPP_DBG_WRN("isp_id %d sta buffer not exist\r\n", (int)id);
		return E_SYS;
	}

	// debug dma write protect for 3dnr sta. start protecting after allocate
	ctl_ipp_dbg_dma_wp_task_start(CTRL_IPP_BUF_ITEM_3DNR_STA, CTL_IPP_BUF_ADDR_MAKE(buf_addr, 0), ((CTL_IPP_HANDLE *)p_isp_info->p_ctrl_info->owner)->private_buf.buf_item._3dnr_sta[0].size);

	loc_cpu(loc_flg);
	p_isp_info->cur_3dnr_sta.enable = ENABLE;
	p_isp_info->cur_3dnr_sta.buf_addr = buf_addr;
	p_isp_info->cur_3dnr_sta.max_sample_num = CTL_IPP_ISP_IQ_3DNR_STA_MAX_NUM;
	unl_cpu(loc_flg);

	/* wait sta output finish */
	rt = vos_flag_wait_timeout(&flg, g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_GET_3DNR_STA_READY, TWF_ORW, vos_util_msec_to_tick(CTL_IPP_ISP_WAIT_STA_TIMEOUT_MS));
	if (rt != E_OK) {
		vos_flag_iset(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_GET_3DNR_STA_READY);
		CTL_IPP_DBG_WRN("isp_id %d wait sta buffer output timeout\r\n", (int)id);
		return E_TMOUT;
	}

	// debug dma write protect for 3dnr sta. finish protecting before assign to user
	ctl_ipp_dbg_dma_wp_task_end(CTRL_IPP_BUF_ITEM_3DNR_STA, CTL_IPP_BUF_ADDR_MAKE(buf_addr, 0));

	loc_cpu(loc_flg);
	*(CTL_IPP_ISP_3DNR_STA *)data = p_isp_info->rdy_3dnr_sta;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_va_win(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	*(CTL_IPP_ISP_VA_WIN_SIZE_RATIO *)data = p_isp_info->va_win_ratio;
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_va_rst(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	memcpy(data, (void *)&p_isp_info->va_rst, sizeof(CTL_IPP_ISP_VA_RST));
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_va_indep_rst(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	memcpy(data, (void *)&p_isp_info->va_indep_rst, sizeof(CTL_IPP_ISP_VA_INDEP_RST));
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_dce_hist_rst(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	memcpy(data, (void *)&p_isp_info->dce_hist_rst, sizeof(CTL_IPP_ISP_DCE_HIST_RST));
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_edge_stcs(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	memcpy(data, (void *)&p_isp_info->ipe_edge_stcs, sizeof(CTL_IPP_ISP_EDGE_STCS));
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_defog_stcs(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	memcpy(data, (void *)&p_isp_info->defog_stcs, sizeof(CTL_IPP_ISP_DEFOG_STCS));
	unl_cpu(loc_flg);

	return E_OK;
}

static INT32 ctl_ipp_isp_get_stripe_info(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;
	CTL_IPP_ISP_STRP_INFO *p_strp;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	memcpy(data, (void *)&p_isp_info->stripe_info, sizeof(CTL_IPP_ISP_STRP_INFO));
	unl_cpu(loc_flg);

	p_strp = (CTL_IPP_ISP_STRP_INFO *)data;
	if (p_strp->num == 0) {
		return E_NOEXS;
	}

	return E_OK;
}

static INT32 ctl_ipp_isp_get_md_subout(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;
	CTL_IPP_ISP_MD_SUBOUT *p_md;

	p_md = (CTL_IPP_ISP_MD_SUBOUT *)data;
	memset((void *)p_md, 0, sizeof(CTL_IPP_ISP_MD_SUBOUT));

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL || p_isp_info->p_ctrl_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	if (p_isp_info->p_ctrl_info->info.ime_ctrl.tplnr_enable &&
		p_isp_info->p_ctrl_info->info.ime_ctrl.tplnr_out_ms_roi_enable &&
		p_isp_info->p_ctrl_info->info.ime_ctrl.tplnr_in_ref_addr[0]) {
		p_md->addr = p_isp_info->p_ctrl_info->info.ime_ctrl.tplnr_out_ms_roi_addr;
	}
	unl_cpu(loc_flg);

	if (p_md->addr == 0) {
		return E_NOEXS;
	}
	p_md->src_img_size = p_isp_info->p_ctrl_info->info.ime_ctrl.in_size;
	p_md->md_size.w = ctl_ipp_util_3dnr_ms_roi_width(p_md->src_img_size.w);
	p_md->md_size.h = ctl_ipp_util_3dnr_ms_roi_height(p_md->src_img_size.h);
	p_md->md_lofs = ctl_ipp_util_3dnr_ms_roi_lofs(p_md->src_img_size.w);

	return E_OK;
}

static CTL_IPP_ISP_GET_FP ctl_ipp_isp_get_fp[CTL_IPP_ISP_ITEM_MAX] = {
	ctl_ipp_isp_get_func_en,
	ctl_ipp_isp_get_iosize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ctl_ipp_isp_get_yout,
	ctl_ipp_isp_get_ife_vig_cent,
	ctl_ipp_isp_get_dce_dc_cent,
	ctl_ipp_isp_get_ime_lca_size,
	ctl_ipp_isp_get_ife2_filt_time,
	ctl_ipp_isp_get_defog_subout,
	ctl_ipp_isp_get_yuv_out,
	ctl_ipp_isp_get_status_info,
	ctl_ipp_isp_get_eth,
	ctl_ipp_isp_get_kdrv_param,
	ctl_ipp_isp_get_3dnr_sta,
	ctl_ipp_isp_get_va_win,
	ctl_ipp_isp_get_va_rst,
	ctl_ipp_isp_get_va_indep_rst,
	ctl_ipp_isp_get_dce_hist_rst,
	ctl_ipp_isp_get_edge_stcs,
	ctl_ipp_isp_get_defog_stcs,
	ctl_ipp_isp_get_stripe_info,
	ctl_ipp_isp_get_md_subout,
};

ER ctl_ipp_isp_get(ISP_ID id, CTL_IPP_ISP_ITEM item, void *data)
{
	ER rt;
	CTL_IPP_ISP_ITEM idx;

	if (ctl_ipp_isp_is_init() == FALSE) {
		CTL_IPP_DBG_WRN("ctl_ipp_isp not init yet\r\n");
		return E_SYS;
	}

	if (ctl_ipp_isp_int_id_validate(id) == FALSE) {
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISP, "invalid isp_id 0x%.8x\r\n", id);
		return E_OK;
	}

	idx = item & (~CTL_IPP_ISP_ITEM_IMM_BIT);
	if (idx >= CTL_IPP_ISP_ITEM_MAX) {
		CTL_IPP_DBG_WRN("unknown item %d\r\n", (int)(idx));
		return E_PAR;
	}

	rt = E_OK;
	if (ctl_ipp_isp_get_fp[idx] != NULL) {
		rt = ctl_ipp_isp_get_fp[idx](id, item, data);
	}

	return rt;
}

#if 0
#endif

UINT32 ctl_ipp_isp_init(UINT32 num, UINT32 buf_addr, UINT32 is_query)
{
	unsigned long loc_cpu_flg;
	UINT32 i, buf_size;
	void *p_buf;

	/* CTL_IPP_ISP_INT_INFO buffer alloc */
	buf_size = sizeof(CTL_IPP_ISP_INT_INFO) * num;
	if (is_query) {
		ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_ISP", CTL_IPP_DBG_CTX_BUF_QUERY, buf_size, buf_addr, num);
		return buf_size;
	}

	p_buf = (void *)buf_addr;
	if (p_buf == NULL) {
		ctl_ipp_isp_max_id_num = 0;
		CTL_IPP_DBG_ERR("alloc isp_info pool memory failed, num %d, want_size 0x%.8x\r\n", (int)num, (unsigned int)buf_size);
		return CTL_IPP_E_NOMEM;
	}
	ctl_ipp_isp_max_id_num = num;
	ctl_ipp_isp_int_info = p_buf;
	CTL_IPP_DBG_TRC("alloc isp_info pool memory, num %d, size 0x%.8x, addr 0x%.8x\r\n", (int)num, (unsigned int)buf_size, (unsigned int)p_buf);

	for (i = 0; i < ctl_ipp_isp_max_id_num; i++) {
		memset((void *)&ctl_ipp_isp_int_info[i], 0, sizeof(CTL_IPP_ISP_INT_INFO));
		ctl_ipp_isp_int_info[i].isp_id = CTL_IPP_ISP_ID_UNUSED;
		ctl_ipp_isp_int_info[i].p_ctrl_info = NULL;
	}

	#if CTL_IPP_ISP_YUVOUT_DEBUG_EN
	memset((void *)&ctl_ipp_isp_yuvout_log[0], 0, sizeof(CTL_IPP_ISP_YUVOUT_LOG) * CTL_IPP_ISP_YUVOUT_DEBUG_NUM);
	#endif

	loc_cpu(loc_cpu_flg);
	ctl_ipp_isp_init_sts = TRUE;
	unl_cpu(loc_cpu_flg);

	ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_ISP", CTL_IPP_DBG_CTX_BUF_ALLOC, buf_size, buf_addr, num);

	return buf_size;
}

INT32 ctl_ipp_isp_uninit(void)
{
	unsigned long loc_cpu_flg;
	UINT32 i;

	if (ctl_ipp_isp_is_init() == TRUE) {
		if (ctl_ipp_isp_int_info != NULL) {
			CTL_IPP_DBG_TRC("free isp_info memory, addr 0x%.8x\r\n", (unsigned int)ctl_ipp_isp_int_info);
			ctl_ipp_dbg_ctxbuf_log_set("CTL_IPP_ISP", CTL_IPP_DBG_CTX_BUF_FREE, 0, (UINT32)ctl_ipp_isp_int_info, 0);

			for (i = 0; i < ctl_ipp_isp_max_id_num; i++) {
				if (ctl_ipp_isp_int_info[i].p_ctrl_info != NULL) {
					ctl_ipp_info_release(ctl_ipp_isp_int_info[i].p_ctrl_info);
				}
			}
			ctl_ipp_isp_max_id_num = 0;
			ctl_ipp_isp_int_info = NULL;
		}

		loc_cpu(loc_cpu_flg);
		ctl_ipp_isp_init_sts = FALSE;
		unl_cpu(loc_cpu_flg);
	}

	return E_OK;
}

INT32 ctl_ipp_isp_alloc_int_info(UINT32 id)
{
	unsigned long loc_flg;
	UINT32 i;

	loc_cpu(loc_flg);
	for (i = 0; i < ctl_ipp_isp_max_id_num; i++) {
		if (ctl_ipp_isp_int_info[i].isp_id == CTL_IPP_ISP_ID_UNUSED) {
			memset((void *)&ctl_ipp_isp_int_info[i], 0, sizeof(CTL_IPP_ISP_INT_INFO));
			ctl_ipp_isp_int_info[i].isp_id = id;
			ctl_ipp_isp_int_info[i].ime_lca_size_ratio.ratio = CTL_IPP_ISP_IQ_LCA_RATIO;
			ctl_ipp_isp_int_info[i].ime_lca_size_ratio.ratio_base = CTL_IPP_RATIO_UNIT_DFT;
			break;
		}
	}
	unl_cpu(loc_flg);

	return E_OK;
}

INT32 ctl_ipp_isp_release_int_info(UINT32 id)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	p_isp_info->isp_id = CTL_IPP_ISP_ID_UNUSED;
	if (p_isp_info->p_ctrl_info != NULL) {
		ctl_ipp_info_release(p_isp_info->p_ctrl_info);
		p_isp_info->p_ctrl_info = NULL;
	}
	unl_cpu(loc_flg);

	return E_OK;
}

INT32 ctl_ipp_isp_update_int_info(UINT32 old_id, UINT32 new_id)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	p_isp_info = ctl_ipp_isp_get_int_info(old_id);
	if (p_isp_info == NULL) {
		return E_PAR;
	}

	loc_cpu(loc_flg);
	p_isp_info->isp_id = new_id;
	unl_cpu(loc_flg);

	return E_OK;
}

INT32 ctl_ipp_isp_event_cb_proc(ISP_ID id, ISP_EVENT evt, UINT32 raw_frame_cnt)
{
	UINT32 i;
	CTL_IPP_ISP_HANDLE *p_hdl;

	if (evt == ISP_EVENT_NONE) {
		return E_OK;
	}

	for (i = 0; i < CTL_IPP_ISP_HANDLE_MAX_NUMBER; i++) {
		p_hdl = &ctl_ipp_isp_handle[i];
		if (p_hdl->status == CTL_IPP_ISP_HANDLE_STATUS_FREE) {
			continue;
		}

		if ((p_hdl->fp != NULL) && (p_hdl->evt & evt)) {
			UINT64 cb_proc_ts1, cb_proc_ts2;

			cb_proc_ts1 = ctl_ipp_util_get_syst_counter();
			p_hdl->fp(id, evt, raw_frame_cnt, NULL);
			cb_proc_ts2 = ctl_ipp_util_get_syst_counter();
			ctl_ipp_isp_dbg_set_cbtime_log(id, evt, raw_frame_cnt, cb_proc_ts1, cb_proc_ts2);
		}
	}

	return E_OK;
}

INT32 ctl_ipp_isp_event_cb_upd_stripe_info(CTL_IPP_HANDLE *p_hdl, CTL_IPP_ISP_INT_INFO *p_isp_info)
{
	static KDRV_DCE_STRIPE_INFO dce_strp;
	unsigned long loc_flg;
	KDF_IPP_HANDLE *p_kdf_hdl;
	INT32 rt_strp;

	p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;

	if (p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW) {
		/* direct mode always single stripe */
		if (p_isp_info->stripe_info.num != 1) {
			loc_cpu(loc_flg);
			p_isp_info->stripe_info.num = 1;
			if (p_isp_info->p_ctrl_info != NULL) {
				p_isp_info->stripe_info.size[0] = p_isp_info->p_ctrl_info->info.dce_ctrl.in_size.w;
			}
			unl_cpu(loc_flg);
		}
	} else if (p_hdl->flow == CTL_IPP_FLOW_RAW ||
		p_hdl->flow == CTL_IPP_FLOW_CCIR ||
		p_hdl->flow == CTL_IPP_FLOW_CAPTURE_RAW ||
		p_hdl->flow == CTL_IPP_FLOW_CAPTURE_CCIR) {

		if (p_kdf_hdl != NULL) {
			rt_strp = kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_DCE, p_kdf_hdl, KDRV_DCE_PARAM_IPL_STRIPE_INFO, (void *)&dce_strp);
		} else {
			rt_strp = E_PAR;
		}
		if (rt_strp == E_OK) {
			loc_cpu(loc_flg);
			p_isp_info->stripe_info.num = dce_strp.hstp_num;
			memcpy((void *)&p_isp_info->stripe_info.size[0], (void *)&dce_strp.hstp[0], sizeof(UINT16) * CTL_IPP_ISP_STRIPE_MAX_NUM);
			unl_cpu(loc_flg);
		}
	}

	return E_OK;
}

INT32 ctl_ipp_isp_event_cb_upd_eth(CTL_IPP_INFO_LIST_ITEM *p_info, CTL_IPP_ISP_INT_INFO *p_isp_info)
{
	unsigned long loc_flg;

	if (p_info->info.ipe_ctrl.eth.enable) {
		if (p_isp_info != NULL) {
			loc_cpu(loc_flg);
			p_isp_info->rdy_eth = p_info->info.ipe_ctrl.eth;
			unl_cpu(loc_flg);
		}
	}

	return E_OK;
}

INT32 ctl_ipp_isp_event_cb_upd_3dnr_sta(CTL_IPP_INFO_LIST_ITEM *p_info, CTL_IPP_ISP_INT_INFO *p_isp_info)
{
	unsigned long loc_flg;

	if (p_info->info.ime_ctrl.tplnr_sta_info.enable && p_info->info.ime_ctrl.tplnr_sta_info.buf_addr != 0) {
		if (p_isp_info != NULL) {
			loc_cpu(loc_flg);
			p_isp_info->rdy_3dnr_sta = p_info->info.ime_ctrl.tplnr_sta_info;
			vos_flag_iset(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_GET_3DNR_STA_READY);
			unl_cpu(loc_flg);
		}
	}

	return E_OK;
}

INT32 ctl_ipp_isp_event_cb_upd_va_result(CTL_IPP_INFO_LIST_ITEM *p_info, CTL_IPP_ISP_INT_INFO *p_isp_info)
{
	static CTL_IPP_ISP_VA_RST va_rst_buf;
	static CTL_IPP_ISP_VA_INDEP_RST va_indep_rst_buf;
	unsigned long loc_flg;
	INT32 rt_va, rt_va_indep;
	CTL_IPP_HANDLE *p_hdl = NULL;
	KDF_IPP_HANDLE *p_kdf_hdl = NULL;

	if (p_info->info.ipe_ctrl.va_enable && p_info->info.ipe_ctrl.va_out_addr != 0) {
		if (p_info->owner) {
			p_hdl = (CTL_IPP_HANDLE *) p_info->owner;
			if (p_hdl) {
				p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;
			}
		}

		if (p_kdf_hdl != NULL && p_isp_info != NULL) {
			rt_va = kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IPE, p_kdf_hdl, (KDRV_IPE_PARAM_IPL_VA_RSLT | KDRV_IPE_PARAM_REV), (void *)&va_rst_buf);
			rt_va_indep = kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IPE, p_kdf_hdl, (KDRV_IPE_PARAM_IPL_VA_INDEP_RSLT | KDRV_IPE_PARAM_REV), (void *)&va_indep_rst_buf);
			loc_cpu(loc_flg);
			if (rt_va == E_OK) {
				memcpy((void *)&p_isp_info->va_rst, (void *)&va_rst_buf, sizeof(CTL_IPP_ISP_VA_RST));
			}
			if (rt_va_indep == E_OK) {
				memcpy((void *)&p_isp_info->va_indep_rst, (void *)&va_indep_rst_buf, sizeof(CTL_IPP_ISP_VA_INDEP_RST));
			}
			unl_cpu(loc_flg);
		}
	}

	return E_OK;
}

INT32 ctl_ipp_isp_event_cb_upd_dce_hist(CTL_IPP_INFO_LIST_ITEM *p_info, CTL_IPP_ISP_INT_INFO *p_isp_info)
{
	static KDRV_DCE_HIST_RSLT hist_rst;
	unsigned long loc_flg;
	CTL_IPP_HANDLE *p_hdl = NULL;
	KDF_IPP_HANDLE *p_kdf_hdl = NULL;
	UINT32 i;

	if (p_info->owner) {
		p_hdl = (CTL_IPP_HANDLE *) p_info->owner;
		if (p_hdl) {
			p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;
		}
	}

	if (p_kdf_hdl != NULL && p_isp_info != NULL) {
		kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_DCE, p_kdf_hdl, (KDRV_DCE_PARAM_IPL_HIST_RSLT | KDRV_DCE_PARAM_REV), (void *)&hist_rst);
		if (hist_rst.enable == ENABLE) {
			loc_cpu(loc_flg);
			/* fix cim issue, kdrv_dce change type to UINT16, ipp_isp keep UINT32 */
			if (hist_rst.hist_sel == KDRV_BEFORE_WDR) {
				for (i = 0; i < CTL_IPP_ISP_DCE_HIST_NUM; i++) {
					p_isp_info->dce_hist_rst.hist_stcs_pre_wdr[i] = hist_rst.hist_stcs[i];
				}
			} else {
				for (i = 0; i < CTL_IPP_ISP_DCE_HIST_NUM; i++) {
					p_isp_info->dce_hist_rst.hist_stcs_post_wdr[i] = hist_rst.hist_stcs[i];
				}
			}
			unl_cpu(loc_flg);
		}
	}

	return E_OK;
}

INT32 ctl_ipp_isp_event_cb_upd_edge_stcs(CTL_IPP_INFO_LIST_ITEM *p_info, CTL_IPP_ISP_INT_INFO *p_isp_info)
{
	unsigned long loc_flg;
	KDRV_IPE_EDGE_STCS edge_stcs;
	CTL_IPP_HANDLE *p_hdl = NULL;
	KDF_IPP_HANDLE *p_kdf_hdl = NULL;

	if (p_info->owner) {
		p_hdl = (CTL_IPP_HANDLE *) p_info->owner;
		if (p_hdl) {
			p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;
		}
	}

	if (p_kdf_hdl != NULL && p_isp_info != NULL) {
		memset((void *)&edge_stcs, 0, sizeof(KDRV_IPE_EDGE_STCS));
		kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IPE, p_kdf_hdl, (KDRV_IPE_PARAM_IPL_EDGE_STCS_RSLT | KDRV_IPE_PARAM_REV), (void *)&edge_stcs);
		loc_cpu(loc_flg);
		p_isp_info->ipe_edge_stcs.localmax_max = edge_stcs.localmax_max;
		p_isp_info->ipe_edge_stcs.coneng_max = edge_stcs.coneng_max;
		p_isp_info->ipe_edge_stcs.coneng_avg = edge_stcs.coneng_avg;
		unl_cpu(loc_flg);
	}

	return E_OK;
}

INT32 ctl_ipp_isp_event_cb_upd_defog_stcs(CTL_IPP_INFO_LIST_ITEM *p_info, CTL_IPP_ISP_INT_INFO *p_isp_info)
{
	unsigned long loc_flg;
	KDRV_IPE_DEFOG_STCS defog_stcs;
	CTL_IPP_HANDLE *p_hdl = NULL;
	KDF_IPP_HANDLE *p_kdf_hdl = NULL;

	if (p_info->owner) {
		p_hdl = (CTL_IPP_HANDLE *) p_info->owner;
		if (p_hdl) {
			p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;
		}
	}

	if (p_kdf_hdl != NULL && p_isp_info != NULL) {
		memset((void *)&defog_stcs, 0, sizeof(KDRV_IPE_DEFOG_STCS));
		kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IPE, p_kdf_hdl, (KDRV_IPE_PARAM_IPL_DEFOG_STCS_RSLT | KDRV_IPE_PARAM_REV), (void *)&defog_stcs);
		loc_cpu(loc_flg);
		p_isp_info->defog_stcs.airlight[0] = defog_stcs.dfg_airlight[0];
		p_isp_info->defog_stcs.airlight[1] = defog_stcs.dfg_airlight[1];
		p_isp_info->defog_stcs.airlight[2] = defog_stcs.dfg_airlight[2];
		unl_cpu(loc_flg);
	}

	return E_OK;
}

INT32 ctl_ipp_isp_event_cb(ISP_ID id, ISP_EVENT evt, CTL_IPP_INFO_LIST_ITEM *p_info, void *param)
{
	unsigned long loc_flg;
	UINT32 raw_frame_count;
	VDO_FRAME *p_vdoin_info;
	CTL_IPP_HANDLE *p_hdl;
	CTL_IPP_ISP_INT_INFO *p_isp_info;

	if (ctl_ipp_isp_int_id_validate(id) == FALSE) {
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISP, "invalid isp_id 0x%.8x\r\n", id);
		return CTL_IPP_E_OK;
	}

	raw_frame_count = 0;
	p_hdl = NULL;
	if (evt != ISP_EVENT_PARAM_RST && p_info != NULL) {
		if (p_info->info.ime_ctrl.pat_paste_enable) { // pattern paste don't do isp event
			return CTL_IPP_E_OK;
		}

		p_hdl = (CTL_IPP_HANDLE *) p_info->owner;
		if(p_hdl->flow == CTL_IPP_FLOW_DIRECT_RAW){
			raw_frame_count = p_info->input_buf_id;
			if (raw_frame_count >= 1) {
				raw_frame_count = raw_frame_count - 1;
			}
		} else {
			p_vdoin_info = (VDO_FRAME *)p_info->input_header_addr;
			raw_frame_count = p_vdoin_info->count;
		}
	}
	if (evt == ISP_EVENT_IPP_CFGSTART) {
		p_isp_info = ctl_ipp_isp_get_int_info(id);
		if (p_info != NULL && p_isp_info != NULL) {
			loc_cpu(loc_flg);
			if (p_isp_info->p_ctrl_info != NULL) {
				ctl_ipp_info_release(p_isp_info->p_ctrl_info);
			}
			p_isp_info->p_ctrl_info = p_info;
			ctl_ipp_info_lock(p_isp_info->p_ctrl_info);
			unl_cpu(loc_flg);
		} else {
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISP, "isp_id 0x%08x cfg_start ctl_info %lx isp_info %lx\r\n", id, (ULONG)p_info, (ULONG)p_isp_info);
		}
	}
	if (evt == ISP_EVENT_IPP_CFGEND) {
		/* Update dce stripe info */
		p_isp_info = ctl_ipp_isp_get_int_info(id);
		if (p_hdl != NULL && p_isp_info != NULL) {
			ctl_ipp_isp_event_cb_upd_stripe_info(p_hdl, p_isp_info);
		} else {
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISP, "isp_id 0x%08x cfg_end ctl_info %lx isp_info %lx\r\n", id, (ULONG)p_info, (ULONG)p_isp_info);
		}
	}
	if (evt == ISP_EVENT_IPP_PROCEND) {
		if (p_info != NULL){
			p_isp_info = ctl_ipp_isp_get_int_info(id);

			/* Update ready eth */
			ctl_ipp_isp_event_cb_upd_eth(p_info, p_isp_info);

			/* Update ready 3dnr sta */
			ctl_ipp_isp_event_cb_upd_3dnr_sta(p_info, p_isp_info);

			/* Update va result */
			ctl_ipp_isp_event_cb_upd_va_result(p_info, p_isp_info);

			/* get dce hist result */
			ctl_ipp_isp_event_cb_upd_dce_hist(p_info, p_isp_info);

			/* get edge statistic */
			ctl_ipp_isp_event_cb_upd_edge_stcs(p_info, p_isp_info);

			/* get defog statistic*/
			ctl_ipp_isp_event_cb_upd_defog_stcs(p_info, p_isp_info);
		} else {
			CTL_IPP_DBG_WRN("info NULL\r\n");
		}
	}

	if (evt == ISP_EVENT_PARAM_RST) {
		/* get statstic info for fastboot */
		#if defined (__KERNEL__)
		if (kdrv_builtin_is_fastboot()) {
			KDRV_IPP_BUILTIN_ISP_INFO *p_builtin_isp;
			UINT32 i;

			p_builtin_isp = kdrv_ipp_builtin_get_isp_info(param);
			p_isp_info = ctl_ipp_isp_get_int_info(id);
			if (p_builtin_isp && p_isp_info) {
				/* histogram */
				if (p_builtin_isp->hist_rst.enable == ENABLE) {
					if (p_builtin_isp->hist_rst.sel == KDRV_BEFORE_WDR) {
						for (i = 0; i < CTL_IPP_ISP_DCE_HIST_NUM; i++) {
							p_isp_info->dce_hist_rst.hist_stcs_pre_wdr[i] = p_builtin_isp->hist_rst.stcs[i];
						}
					} else {
						for (i = 0; i < CTL_IPP_ISP_DCE_HIST_NUM; i++) {
							p_isp_info->dce_hist_rst.hist_stcs_post_wdr[i] = p_builtin_isp->hist_rst.stcs[i];
						}
					}
				}

				/* edge */
				p_isp_info->ipe_edge_stcs.localmax_max = p_builtin_isp->edge_stcs.localmax_max;
				p_isp_info->ipe_edge_stcs.coneng_max = p_builtin_isp->edge_stcs.coneng_max;
				p_isp_info->ipe_edge_stcs.coneng_avg = p_builtin_isp->edge_stcs.coneng_avg;

				/* defog */
				p_isp_info->defog_stcs.airlight[0] = p_builtin_isp->defog_stcs.airlight[0];
				p_isp_info->defog_stcs.airlight[1] = p_builtin_isp->defog_stcs.airlight[1];
				p_isp_info->defog_stcs.airlight[2] = p_builtin_isp->defog_stcs.airlight[2];
			}

			if (p_builtin_isp && p_info && p_isp_info) {
				/* for isp get ipe subout */
				if (p_builtin_isp->defog_subout_addr) {
					p_info->info.ipe_ctrl.defog_in_addr = nvtmpp_sys_pa2va(p_builtin_isp->defog_subout_addr);
				}
				p_isp_info->p_ctrl_info = p_info;
				ctl_ipp_info_lock(p_isp_info->p_ctrl_info);
			}
		}
		#endif
		ctl_ipp_isp_event_cb_proc(id, ISP_EVENT_PARAM_RST, raw_frame_count);
	} else {
		ctl_ipp_isp_event_cb_proc(id, ctl_ipp_isp_get_imm_event(evt), raw_frame_count);
		if (ctl_ipp_isp_get_free_queue_num() > 0) {
			ctl_ipp_isp_msg_snd(CTL_IPP_ISP_MSG_PROCESS, id, evt, raw_frame_count);
		}
	}

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_isp_int_event_cb(ISP_ID id, CTL_IPP_ISP_INT_EVENT evt)
{
	if (evt == CTL_IPP_ISP_INT_EVT_TRIG_START) {
		ctl_ipp_isp_cfg_lock();
	}

	if (evt == CTL_IPP_ISP_INT_EVT_TRIG_END) {
		ctl_ipp_isp_cfg_unlock();
	}

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_isp_int_get(CTL_IPP_HANDLE *p_hdl, CTL_IPP_ISP_ITEM item, void *data)
{
	unsigned long loc_flg;
	CTL_IPP_ISP_INT_INFO *p_isp_info;
	UINT32 id;

	if (item >= CTL_IPP_ISP_ITEM_MAX) {
		CTL_IPP_DBG_WRN("unknown item %d\r\n", (int)(item));
		return CTL_IPP_E_PAR;
	}

	id = p_hdl->isp_id[CTL_IPP_ALGID_IQ];
	p_isp_info = ctl_ipp_isp_get_int_info(id);
	if (p_isp_info == NULL) {
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_ISP, "isp_id 0x%.8x info NULL (item %u)\r\n", id, item);
		return CTL_IPP_E_OK;
	}

	loc_cpu(loc_flg);

	switch (item) {
	case CTL_IPP_ISP_ITEM_IFE_VIG_CENT:
		*(CTL_IPP_ISP_IFE_VIG_CENT_RATIO *)data = p_isp_info->ife_vig_cent_ratio;
		break;

	case CTL_IPP_ISP_ITEM_DCE_DC_CENT:
		*(CTL_IPP_ISP_DCE_DC_CENT_RATIO *)data = p_isp_info->dce_dc_cent_ratio;
		break;

	case CTL_IPP_ISP_ITEM_IME_LCA_SIZE:
		*(CTL_IPP_ISP_IME_LCA_SIZE_RATIO *)data = p_isp_info->ime_lca_size_ratio;
		break;

	case CTL_IPP_ISP_ITEM_IFE2_FILT_TIME:
		*(CTL_IPP_ISP_IFE2_FILTER_TIME *)data = p_isp_info->ife2_filt_time;
		break;

	case CTL_IPP_ISP_ITEM_ETH_PARAM:
		*(CTL_IPP_ISP_ETH *)data = p_isp_info->cur_eth;
		p_isp_info->cur_eth.enable = DISABLE;
		break;

	case CTL_IPP_ISP_ITEM_VA_WIN_SIZE:
		*(CTL_IPP_ISP_VA_WIN_SIZE_RATIO *)data = p_isp_info->va_win_ratio;
		break;

	case CTL_IPP_ISP_ITEM_3DNR_STA:
		*(CTL_IPP_ISP_3DNR_STA *)data = p_isp_info->cur_3dnr_sta;
		p_isp_info->cur_3dnr_sta.enable = DISABLE;
		break;

	default:
		break;
	}

	unl_cpu(loc_flg);

	return CTL_IPP_E_OK;
}

INT32 ctl_ipp_isp_int_set_yuv_out(CTL_IPP_HANDLE *p_hdl, CTL_IPP_OUT_BUF_INFO *p_buf)
{
	FLGPTN flgptn;

	flgptn = vos_flag_chk(g_ctl_ipp_isp_flg_id, (CTL_IPP_ISP_GET_YUV_END | CTL_IPP_ISP_GET_YUV_TIMEOUT));
	if (flgptn & (CTL_IPP_ISP_GET_YUV_END | CTL_IPP_ISP_GET_YUV_TIMEOUT)) {
		return E_SYS;
	}

	if (ctl_ipp_isp_yuv_out_info.handle == (UINT32)p_hdl &&
		ctl_ipp_isp_yuv_out_info.yuv_out.pid == (UINT32)p_buf->pid) {
		ctl_ipp_isp_yuv_out_info.yuv_out.buf_id = p_buf->buf_id;
		ctl_ipp_isp_yuv_out_info.yuv_out.buf_addr = p_buf->buf_addr;
		ctl_ipp_isp_yuv_out_info.yuv_out.buf_size = p_buf->buf_size;
		ctl_ipp_isp_yuv_out_info.yuv_out.vdo_frm = p_buf->vdo_frm;

		if (p_hdl->evt_outbuf_fp != NULL) {
			p_hdl->evt_outbuf_fp(CTL_IPP_BUF_IO_LOCK, (void *)p_buf, NULL);
		}
		vos_flag_set(g_ctl_ipp_isp_flg_id, CTL_IPP_ISP_GET_YUV_READY);
		return E_OK;
	}

	return E_PAR;
}

UINT32 ctl_ipp_isp_int_id_validate(UINT32 id)
{
	/* CTL_IPP_ISP_ID_UNUSED, CTL_IPP_ISP_ID_UNUSED_2 means no isp callback */
	if (id == CTL_IPP_ISP_ID_UNUSED || id == CTL_IPP_ISP_ID_UNUSED_2) {
		return FALSE;
	}
	return TRUE;
}

void ctl_ipp_isp_int_chk_callback_registerd(void)
{
	UINT32 i;

	/* check if no iq callback exist */
	for (i = 0; i < CTL_IPP_ISP_HANDLE_MAX_NUMBER; i++) {
		if (ctl_ipp_isp_handle[i].status == CTL_IPP_ISP_HANDLE_STATUS_USED) {
			return ;
		}
	}

	CTL_IPP_DBG_ERR("no isp callback registered for ctl_ipp\r\n");
}

void ctl_ipp_isp_dump(int (*dump)(const char *fmt, ...))
{
	CHAR *str_status[] = {
		"FREE",
		"USED"
	};
	UINT32 i;
	CTL_IPP_ISP_HANDLE *p_hdl;

	dump("---- CTL IPP ISP INFO ----\r\n");
	dump("%32s%8s%12s%12s\r\n", "name", "status", "evt", "cb_msg");
	for (i = 0; i < CTL_IPP_ISP_HANDLE_MAX_NUMBER; i++) {
		p_hdl = &ctl_ipp_isp_handle[i];
		dump("%32s%8s  0x%.8x  0x%.8x\r\n", p_hdl->name, str_status[p_hdl->status], (unsigned int)p_hdl->evt, (unsigned int)p_hdl->cb_msg);
	}

	#if CTL_IPP_ISP_YUVOUT_DEBUG_EN
	dump("\r\n---- CTL IPP ISP YUV LOCK/UNLOCK LOG ----\r\n");
	dump("%8s%8s  %10s  %10s%12s%12s%12s\r\n", "isp_id", "pid", "buf_addr", "buf_id", "lock_ts", "unlock_ts", "diff(us)");
	for (i = 0; i < CTL_IPP_ISP_YUVOUT_DEBUG_NUM; i++) {
		if (ctl_ipp_isp_yuvout_log[i].buf_addr == 0) {
			continue;
		}
		dump("%8d%8d  0x%.8x  0x%.8x%12llu%12llu%12llu",
			(int)ctl_ipp_isp_yuvout_log[i].ispid,
			(int)ctl_ipp_isp_yuvout_log[i].pid,
			(unsigned int)ctl_ipp_isp_yuvout_log[i].buf_addr,
			(unsigned int)ctl_ipp_isp_yuvout_log[i].buf_id,
			ctl_ipp_isp_yuvout_log[i].lock_ts,
			ctl_ipp_isp_yuvout_log[i].unlock_ts,
			ctl_ipp_isp_yuvout_log[i].unlock_ts - ctl_ipp_isp_yuvout_log[i].lock_ts);
	}
	#endif

	dump("\r\n---- CTL IPP ISP INT INFO MAP ----\r\n");
	dump("%10s  %10s\r\n", "int_info", "isp_id");
	for (i = 0; i < ctl_ipp_isp_max_id_num; i++) {
		dump("0x%.8x  0x%.8x\r\n", (unsigned int)&ctl_ipp_isp_int_info[i], (unsigned int)ctl_ipp_isp_int_info[i].isp_id);
	}

	ctl_ipp_isp_task_dumpinfo(dump);
}

void ctl_ipp_isp_dbg_cbtime_set_threshold(UINT32 threshold_us, UINT32 max_catch_number)
{
	ctl_ipp_isp_dbg_cbtime_threshold = threshold_us;
	ctl_ipp_isp_dbg_cbtime_catch_number = max_catch_number;
}

void ctl_ipp_isp_dbg_cbtime_dump(int (*dump)(const char *fmt, ...))
{
#if CTL_IPP_ISP_CBTIME_DEBUG_EN
	UINT32 i, idx;
	CTL_IPP_ISP_CBTIME_LOG *p_log;

	dump("---- ISP CALLBACK PROC TIME ----\r\n");
	dump("%8s  %10s%8s%12s%12s%16s\r\n", "isp_id", "event", "raw_fc", "start", "end", "proc_time(us)");
	for (i = 0; i < CTL_IPP_ISP_CBTIME_DEBUG_NUM; i++) {
		if (i > ctl_ipp_isp_dbg_cbtime_cnt) {
			break;
		}

		idx = (ctl_ipp_isp_dbg_cbtime_cnt - i - 1) % CTL_IPP_ISP_CBTIME_DEBUG_NUM;
		p_log = &ctl_ipp_isp_dbg_cbtime_log[idx];
		dump("%8d  0x%.8x%8d%12lld%12lld%12lld\r\n",
			(int)p_log->ispid, (unsigned int)p_log->evt, (int)p_log->raw_fc,
			p_log->ts_start,
			p_log->ts_end,
			(p_log->ts_end - p_log->ts_start));
	}
#endif
}

#if 0
#endif

void ctl_ipp_isp_drv_init(void)
{
	UINT32 i;

	for (i = 0; i < CTL_IPP_ISP_HANDLE_MAX_NUMBER; i++) {
		ctl_ipp_isp_handle_reset(&ctl_ipp_isp_handle[i]);
	}
}

void ctl_ipp_isp_drv_uninit(void)
{
	UINT32 i;

	if (ctl_ipp_isp_is_init() == TRUE) {
		ctl_ipp_isp_uninit();
	}

	for (i = 0; i < CTL_IPP_ISP_HANDLE_MAX_NUMBER; i++) {
		ctl_ipp_isp_handle_release(&ctl_ipp_isp_handle[i]);
	}
}

