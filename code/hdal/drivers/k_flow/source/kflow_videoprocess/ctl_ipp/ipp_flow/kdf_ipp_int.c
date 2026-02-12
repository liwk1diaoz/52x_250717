#include "kflow_common/isp_if.h"
#include "kdf_ipp_int.h"
#include "kdf_ipp_id_int.h"
#include "ctl_ipp_isp_int.h"
#include "ctl_ipp_flow_task_int.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

/* DCE GDC/CAC bind with SRAM config
	ENABLE: sram_cnn + gdc_en, sram_dce + gdc_disable
	DISABLE: sram_cnn/sram_dce + gdc_en, final gdc_en leave for iq config

*/
#if defined(CONFIG_NVT_SMALL_HDAL)
#define KDF_IPP_BIND_DCE_GDC_WITH_SRAM_CONFIG (DISABLE)
#else
#define KDF_IPP_BIND_DCE_GDC_WITH_SRAM_CONFIG (ENABLE)
#endif

#if 0
#endif
#define KDF_IPP_MSG_Q_NUM   32
static CTL_IPP_LIST_HEAD free_msg_list_head;
static CTL_IPP_LIST_HEAD proc_msg_list_head;
static UINT32 kdf_ipp_msg_queue_free;
static KDF_IPP_MSG_EVENT kdf_ipp_msg_pool[KDF_IPP_MSG_Q_NUM];

static KDF_IPP_MSG_EVENT *kdf_ipp_msg_pool_get_msg(UINT32 idx)
{
	if (idx >= KDF_IPP_MSG_Q_NUM) {
		CTL_IPP_DBG_IND("msg idx = %d overflow\r\n", (int)(idx));
		return NULL;
	}
	return &kdf_ipp_msg_pool[idx];
}

static void kdf_ipp_msg_reset(KDF_IPP_MSG_EVENT *p_event)
{
	if (p_event) {
		p_event->cmd = 0;
		p_event->param[0] = 0;
		p_event->param[1] = 0;
		p_event->param[2] = 0;
		p_event->rev[0] = KDF_IPP_MSG_STS_FREE;
	}
}

static KDF_IPP_MSG_EVENT *kdf_ipp_msg_lock(void)
{
	unsigned long loc_cpu_flg;
	KDF_IPP_MSG_EVENT *p_event = NULL;

	loc_cpu(loc_cpu_flg);

	if (!vos_list_empty(&free_msg_list_head)) {
		p_event = vos_list_entry(free_msg_list_head.next, KDF_IPP_MSG_EVENT, list);
		vos_list_del(&p_event->list);

		p_event->rev[0] |= KDF_IPP_MSG_STS_LOCK;
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[KDF_IPL]Get free queue idx = %d\r\n", (int)(p_event->rev[1]));
		kdf_ipp_msg_queue_free -= 1;

		unl_cpu(loc_cpu_flg);
		return p_event;
	}

	unl_cpu(loc_cpu_flg);
	return p_event;
}

static void kdf_ipp_msg_unlock(KDF_IPP_MSG_EVENT *p_event)
{
	unsigned long loc_cpu_flg;

	if ((p_event->rev[0] & KDF_IPP_MSG_STS_LOCK) != KDF_IPP_MSG_STS_LOCK) {
		CTL_IPP_DBG_IND("event status error (%d)\r\n", (int)(p_event->rev[0]));
	} else {

		kdf_ipp_msg_reset(p_event);

		loc_cpu(loc_cpu_flg);
		vos_list_add_tail(&p_event->list, &free_msg_list_head);
		kdf_ipp_msg_queue_free += 1;
		unl_cpu(loc_cpu_flg);
	}
}

UINT32 kdf_ipp_get_free_queue_num(void)
{
	return kdf_ipp_msg_queue_free;
}

ER kdf_ipp_msg_snd(UINT32 cmd, UINT32 p1, UINT32 p2, UINT32 p3, IPP_EVENT_FP drop_fp, UINT32 count)
{
	unsigned long loc_cpu_flg;
	KDF_IPP_MSG_EVENT *p_event;

	p_event = kdf_ipp_msg_lock();

	if (p_event == NULL) {
		return E_SYS;
	}

	p_event->cmd = cmd;
	p_event->param[0] = p1;
	p_event->param[1] = p2;
	p_event->param[2] = p3;
	p_event->drop_fp = drop_fp;
	p_event->snd_time = ctl_ipp_util_get_syst_counter();
	p_event->count = count;
	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[KDF_IPL]S Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(p1), (unsigned int)(p2), (unsigned int)(p3));

	/* add to proc list & trig proc task */
	loc_cpu(loc_cpu_flg);
	vos_list_add_tail(&p_event->list, &proc_msg_list_head);
	unl_cpu(loc_cpu_flg);
	vos_flag_set(g_kdf_ipp_flg_id, KDF_IPP_QUEUE_PROC);
	return E_OK;
}

ER kdf_ipp_msg_rcv(UINT32 *cmd, UINT32 *p1, UINT32 *p2, UINT32 *p3, IPP_EVENT_FP *drop_fp, UINT64 *snd_t)
{
	unsigned long loc_cpu_flg;
	KDF_IPP_MSG_EVENT *p_event;
	FLGPTN flag;
	UINT32 vos_list_empty_flag;

	while (1) {
		/* check empty or not */
		loc_cpu(loc_cpu_flg);
		vos_list_empty_flag = vos_list_empty(&proc_msg_list_head);
		if (!vos_list_empty_flag) {
			p_event = vos_list_entry(proc_msg_list_head.next, KDF_IPP_MSG_EVENT, list);
			vos_list_del(&p_event->list);
		}
		unl_cpu(loc_cpu_flg);

		/* is empty enter wait mode */
		if (vos_list_empty_flag) {
			vos_flag_wait(&flag, g_kdf_ipp_flg_id, KDF_IPP_QUEUE_PROC, TWF_CLR);
		} else {
			break;
		}
	}

	*cmd = p_event->cmd;
	*p1 = p_event->param[0];
	*p2 = p_event->param[1];
	*p3 = p_event->param[2];
	*drop_fp = p_event->drop_fp;
	*snd_t = p_event->snd_time;
	kdf_ipp_msg_unlock(p_event);

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[KDF_IPL]R Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(*cmd), (unsigned int)(*p1), (unsigned int)(*p2), (unsigned int)(*p3));
	return E_OK;
}

ER kdf_ipp_msg_flush(void)
{
	UINT64 time;
	IPP_EVENT_FP drop_fp;
	UINT32 cmd, param[3];
	ER err = E_OK;

	while (!vos_list_empty(&proc_msg_list_head)) {
		drop_fp = NULL;
		err = kdf_ipp_msg_rcv(&cmd, &param[0], &param[1], &param[2], &drop_fp, &time);

		if (err != E_OK) {
			CTL_IPP_DBG_IND("flush fail (%d)\r\n", (int)(err));
		}

		if (cmd != KDF_IPP_MSG_IGNORE) {
			kdf_ipp_drop(param[0], param[1], param[2], drop_fp, CTL_IPP_E_FLUSH);
		}
	}
	return err;
}

ER kdf_ipp_erase_queue(UINT32 handle)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_LIST_HEAD *p_list;
	KDF_IPP_MSG_EVENT *p_event;

	loc_cpu(loc_cpu_flg);

	if (!vos_list_empty(&proc_msg_list_head)) {
		vos_list_for_each(p_list, &proc_msg_list_head) {
			p_event = vos_list_entry(p_list, KDF_IPP_MSG_EVENT, list);
			if (p_event->cmd != KDF_IPP_MSG_IGNORE) {
				if (p_event->param[0] == handle) {
					kdf_ipp_drop(p_event->param[0], p_event->param[1], p_event->param[2], p_event->drop_fp, CTL_IPP_E_FLUSH);
					p_event->cmd = KDF_IPP_MSG_IGNORE;
				}
			}
		}
	}

	unl_cpu(loc_cpu_flg);
	return E_OK;
}

ER kdf_ipp_disable_path_dma_in_queue(UINT32 handle, UINT32 pid)
{
	unsigned long loc_cpu_flg;
	CTL_IPP_LIST_HEAD *p_list;
	KDF_IPP_MSG_EVENT *p_event;

	loc_cpu(loc_cpu_flg);

	if (!vos_list_empty(&proc_msg_list_head)) {
		vos_list_for_each(p_list, &proc_msg_list_head) {
			p_event = vos_list_entry(p_list, KDF_IPP_MSG_EVENT, list);
			if (p_event->cmd != KDF_IPP_MSG_IGNORE) {
				if (p_event->param[0] == handle) {
					CTL_IPP_BASEINFO *p_base;

					p_base = (CTL_IPP_BASEINFO *)p_event->param[1];
					if (p_base->ime_ctrl.out_img[pid].enable == ENABLE) {
						p_base->ime_ctrl.out_img[pid].enable = DISABLE;
						p_base->ime_ctrl.out_img[pid].dma_enable = DISABLE;
					}
				}
			}
		}
	}

	unl_cpu(loc_cpu_flg);
	return E_OK;
}

ER kdf_ipp_msg_reset_queue(void)
{
	UINT32 i;
	KDF_IPP_MSG_EVENT *free_event;

	/* init free & proc list head */
	VOS_INIT_LIST_HEAD(&free_msg_list_head);
	VOS_INIT_LIST_HEAD(&proc_msg_list_head);

	/* gen free list */
	for (i = 0; i < KDF_IPP_MSG_Q_NUM; i++) {

		free_event = kdf_ipp_msg_pool_get_msg(i);
		kdf_ipp_msg_reset(free_event);
		free_event->rev[1] = i;
		vos_list_add_tail(&free_event->list, &free_msg_list_head);
	}
	kdf_ipp_msg_queue_free = KDF_IPP_MSG_Q_NUM;
	return E_OK;
}

#if 0
#endif

/**
	typecast to kdriver format
*/
#define KDF_IPP_TYPECAST_FAILED	(0x5A5A5A5A)
_INLINE KDRV_IPP_PIX kdf_ipp_typecast_ipp_pix(VDO_PXLFMT fmt)
{
	UINT32 stpx;

	if (VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_RAW ||
		VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_NRX) {
		stpx = VDO_PXLFMT_PIX(fmt);
	} else {
		CTL_IPP_DBG_WRN("unsupport fmt 0x%.8x\r\n", (unsigned int)fmt);
		return KDF_IPP_TYPECAST_FAILED;
	}

	switch (stpx) {
	case VDO_PIX_RGGB_R:
		return KDRV_IPP_RGGB_PIX_R;

	case VDO_PIX_RGGB_GR:
		return KDRV_IPP_RGGB_PIX_GR;

	case VDO_PIX_RGGB_GB:
		return KDRV_IPP_RGGB_PIX_GB;

	case VDO_PIX_RGGB_B:
		return KDRV_IPP_RGGB_PIX_B;

	case VDO_PIX_RGBIR44_RGBG_GIGI:
		return KDRV_IPP_RGBIR_PIX_RG_GI;

	case VDO_PIX_RGBIR44_GBGR_IGIG:
		return KDRV_IPP_RGBIR_PIX_GB_IG;

	case VDO_PIX_RGBIR44_GIGI_BGRG:
		return KDRV_IPP_RGBIR_PIX_GI_BG;

	case VDO_PIX_RGBIR44_IGIG_GRGB:
		return KDRV_IPP_RGBIR_PIX_IG_GR;

	case VDO_PIX_RGBIR44_BGRG_GIGI:
		return KDRV_IPP_RGBIR_PIX_BG_GI;

	case VDO_PIX_RGBIR44_GRGB_IGIG:
		return KDRV_IPP_RGBIR_PIX_GR_IG;

	case VDO_PIX_RGBIR44_GIGI_RGBG:
		return KDRV_IPP_RGBIR_PIX_GI_RG;

	case VDO_PIX_RGBIR44_IGIG_GBGR:
		return KDRV_IPP_RGBIR_PIX_IG_GB;

	case VDO_PIX_RCCB_RC:
		return KDRV_IPP_RCCB_PIX_RC;

	case VDO_PIX_RCCB_CR:
		return KDRV_IPP_RCCB_PIX_CR;

	case VDO_PIX_RCCB_CB:
		return KDRV_IPP_RCCB_PIX_CB;

	case VDO_PIX_RCCB_BC:
		return KDRV_IPP_RCCB_PIX_BC;

	default:
		CTL_IPP_DBG_WRN("Unknown pix 0x%.8x\r\n", (unsigned int)stpx);
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IPP_RAW_BIT kdf_ipp_typecast_ipp_rawbit(VDO_PXLFMT fmt)
{
	UINT32 bit;

	if (VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_RAW ||
		VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_NRX) {
		bit = VDO_PXLFMT_BPP(fmt);
	} else {
		CTL_IPP_DBG_WRN("unsupport fmt 0x%.8x\r\n", (unsigned int)fmt);
		return KDF_IPP_TYPECAST_FAILED;
	}

	switch (bit) {
	case 8:
		return KDRV_IPP_RAW_BIT_8;

	case 10:
		return KDRV_IPP_RAW_BIT_10;

	case 12:
		return KDRV_IPP_RAW_BIT_12;

	case 16:
		return KDRV_IPP_RAW_BIT_16;

	default:
		CTL_IPP_DBG_WRN("unsupport rawbit %d\r\n", (int)(bit));
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IPP_FLIP kdf_ipp_typecast_ipp_flip(CTL_IPP_FLIP_TYPE flip)
{
	switch (flip) {
	case CTL_IPP_FLIP_NONE:
		return KDRV_IPP_FLIP_NONE;

	case CTL_IPP_FLIP_H:
		return KDRV_IPP_FLIP_H;

	case CTL_IPP_FLIP_V:
		return KDRV_IPP_FLIP_V;

	case CTL_IPP_FLIP_H_V:
		return KDRV_IPP_FLIP_H_V;

	default:
		CTL_IPP_DBG_WRN("unsupport flip 0x%x\r\n", (unsigned int)(flip));
		return KDF_IPP_TYPECAST_FAILED;

	}
}

_INLINE KDRV_IFE_ENCODE_RATE kdf_ipp_typecast_ife_decode_ratio(UINT32 ratio)
{
	switch (ratio) {
		case 50:
			return KDRV_IFE_ENCODE_RATE_50;

		case 58:
			return KDRV_IFE_ENCODE_RATE_58;

		case 66:
			return KDRV_IFE_ENCODE_RATE_66;

		default:
			CTL_IPP_DBG_WRN("unsupport raw compress ratio %d\r\n", (int)(ratio));
			return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_DCE_D2D_FMT_TYPE kdf_ipp_typecast_dce_fmt(VDO_PXLFMT fmt)
{
	fmt = fmt & (~VDO_PIX_YCC_MASK);
	switch (fmt) {
	case VDO_PXLFMT_YUV422:
		return KDRV_DCE_D2D_FMT_Y_PACK_UV422;

	case VDO_PXLFMT_YUV420:
		return KDRV_DCE_D2D_FMT_Y_PACK_UV420;

	/* special case for y-only, set uv-address as y address*/
	case VDO_PXLFMT_Y8:
	case VDO_PXLFMT_520_IR8:
		return KDRV_DCE_D2D_FMT_Y_PACK_UV420;

	default:
		CTL_IPP_DBG_WRN("unsupport fmt 0x%.8x\r\n", (unsigned int)fmt);
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_DCE_D2D_FMT_TYPE kdf_ipp_typecast_dce_yuvfmt(VDO_PXLFMT fmt)
{
	UINT32 yuvfmt = fmt & VDO_PIX_YCC_MASK;
	switch (yuvfmt) {
	case VDO_PIX_YCC_BT601:
		return KDRV_DCE_YUV2RGB_FMT_BT601;

	case VDO_PIX_YCC_BT709:
		return KDRV_DCE_YUV2RGB_FMT_BT709;

	default:
		return KDRV_DCE_YUV2RGB_FMT_FULL;
	}
}

_INLINE UINT32 kdf_ipp_typecast_dce_cfa_subout_ch(VDO_PXLFMT fmt)
{
	/*
		only support choose ir plane
		cfa subout ch map
		0 1
		2 3
	*/
	UINT32 stpx;

	if (VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_RAW ||
		VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_NRX) {
		stpx = VDO_PXLFMT_PIX(fmt);
	} else {
		CTL_IPP_DBG_WRN("unsupport fmt 0x%.8x\r\n", (unsigned int)fmt);
		return KDF_IPP_TYPECAST_FAILED;
	}

	switch (stpx) {
	case VDO_PIX_RGGB_R:
	case VDO_PIX_RGGB_GR:
	case VDO_PIX_RGGB_GB:
	case VDO_PIX_RGGB_B:
		return 0;

	case VDO_PIX_RGBIR44_RGBG_GIGI:
		return 3;

	case VDO_PIX_RGBIR44_GBGR_IGIG:
		return 2;

	case VDO_PIX_RGBIR44_GIGI_BGRG:
		return 1;

	case VDO_PIX_RGBIR44_IGIG_GRGB:
		return 0;

	case VDO_PIX_RGBIR44_BGRG_GIGI:
		return 3;

	case VDO_PIX_RGBIR44_GRGB_IGIG:
		return 2;

	case VDO_PIX_RGBIR44_GIGI_RGBG:
		return 1;

	case VDO_PIX_RGBIR44_IGIG_GBGR:
		return 0;

	case VDO_PIX_RCCB_RC:
	case VDO_PIX_RCCB_CR:
	case VDO_PIX_RCCB_CB:
	case VDO_PIX_RCCB_BC:
		return 0;

	default:
		CTL_IPP_DBG_WRN("Unknown pix 0x%.8x\r\n", (unsigned int)stpx);
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE UINT32 kdf_ipp_typecast_dce_stripe_rule(CTL_IPP_STRP_RULE_SELECT rule)
{
	switch (rule) {
	case CTL_IPP_STRP_RULE_1:
		return KDRV_DCE_STRP_GDC_HIGH_PRIOR;

	case CTL_IPP_STRP_RULE_2:
		return KDRV_DCE_STRP_LOW_LAT_HIGH_PRIOR;

	case CTL_IPP_STRP_RULE_3:
		return KDRV_DCE_STRP_2DLUT_HIGH_PRIOR;

	case CTL_IPP_STRP_RULE_4:
		return KDRV_DCE_STRP_GDC_BEST;

	default:
		CTL_IPP_DBG_WRN("Unknown strp rule %d\r\n", (int)rule);
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_DCE_STRIPE_TYPE kdf_ipp_typecast_dce_stripe_type(CTL_IPP_STRP_RULE_SELECT rule)
{
	switch (rule) {
	case CTL_IPP_STRP_RULE_2STRP:
		return KDRV_DCE_STRIPE_MANUAL_2STRIPE;

	case CTL_IPP_STRP_RULE_3STRP:
		return KDRV_DCE_STRIPE_MANUAL_3STRIPE;

	case CTL_IPP_STRP_RULE_4STRP:
		return KDRV_DCE_STRIPE_MANUAL_4STRIPE;

	case CTL_IPP_STRP_RULE_5STRP:
		return KDRV_DCE_STRIPE_MANUAL_5STRIPE;

	case CTL_IPP_STRP_RULE_6STRP:
		return KDRV_DCE_STRIPE_MANUAL_6STRIPE;

	case CTL_IPP_STRP_RULE_7STRP:
		return KDRV_DCE_STRIPE_MANUAL_7STRIPE;

	case CTL_IPP_STRP_RULE_8STRP:
		return KDRV_DCE_STRIPE_MANUAL_8STRIPE;

	case CTL_IPP_STRP_RULE_9STRP:
		return KDRV_DCE_STRIPE_MANUAL_9STRIPE;

	default:
		CTL_IPP_DBG_WRN("Unknown strp rule %d\r\n", (int)rule);
		return KDRV_DCE_STRIPE_AUTO;
	}
}

_INLINE KDRV_IPE_IN_FMT kdf_ipp_typecast_ipe_in_fmt(VDO_PXLFMT fmt)
{
	fmt = fmt & (~VDO_PIX_YCC_MASK);
	switch (fmt) {
	case VDO_PXLFMT_YUV444:
		return KDRV_IPE_IN_FMT_Y_PACK_UV444;

	case VDO_PXLFMT_YUV422:
		return KDRV_IPE_IN_FMT_Y_PACK_UV422;

	case VDO_PXLFMT_YUV420:
		return KDRV_IPE_IN_FMT_Y_PACK_UV420;

	case VDO_PXLFMT_Y8:
		return KDRV_IPE_IN_FMT_Y;

	default:
		CTL_IPP_DBG_WRN("unsupport fmt 0x%.8x\r\n", (unsigned int)fmt);
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IPE_IN_FMT kdf_ipp_typecast_ipe_yuvfmt(VDO_PXLFMT fmt)
{
	UINT32 yuvfmt = fmt & VDO_PIX_YCC_MASK;
	switch (yuvfmt) {
	case VDO_PIX_YCC_BT601:
		return KDRV_IPE_YUV_FMT_BT601;

	case VDO_PIX_YCC_BT709:
		return KDRV_IPE_YUV_FMT_BT709;

	default:
		return KDRV_IPE_YUV_FMT_FULL;
	}
}

_INLINE KDRV_IPE_OUT_FMT kdf_ipp_typecast_ipe_out_fmt(VDO_PXLFMT fmt)
{
	switch (fmt) {
	case VDO_PXLFMT_YUV444:
		return KDRV_IPE_OUT_FMT_Y_PACK_UV444;

	case VDO_PXLFMT_YUV422:
		return KDRV_IPE_OUT_FMT_Y_PACK_UV422;

	case VDO_PXLFMT_YUV420:
		return KDRV_IPE_OUT_FMT_Y_PACK_UV420;

	case VDO_PXLFMT_Y8:
		return KDRV_IPE_OUT_FMT_Y;

	default:
		CTL_IPP_DBG_WRN("unsupport fmt 0x%.8x\r\n", (unsigned int)fmt);
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IPE_ETH_OUTSEL kdf_ipp_typecast_ipe_eth_out_sel(CTL_IPP_ISP_ETH_OUT_SEL sel)
{
	switch (sel) {
	case CTL_IPP_ISP_ETH_OUT_ORIGINAL:
		return KDRV_ETH_OUT_ORIGINAL;

	case CTL_IPP_ISP_ETH_OUT_DOWNSAMPLE:
		return KDRV_ETH_OUT_DOWNSAMPLED;

	case CTL_IPP_ISP_ETH_OUT_BOTH:
		return KDRV_ETH_OUT_BOTH;

	default:
		CTL_IPP_DBG_WRN("unsupport eth_out_sel %d\r\n", (int)(sel));
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IME_IN_FMT kdf_ipp_typecast_ime_in_fmt(VDO_PXLFMT fmt)
{
	fmt = fmt & (~VDO_PIX_YCC_MASK);
	switch (fmt) {
	case VDO_PXLFMT_YUV420_PLANAR:
		return KDRV_IME_IN_FMT_YUV420;

	case VDO_PXLFMT_YUV420:
		return KDRV_IME_IN_FMT_Y_PACK_UV420;

	case VDO_PXLFMT_Y8:
		return KDRV_IME_IN_FMT_Y;

	default:
		CTL_IPP_DBG_WRN("unsupport fmt 0x%.8x\r\n", (unsigned int)fmt);
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IME_OUT_FMT kdf_ipp_typecast_ime_out_fmt(VDO_PXLFMT fmt)
{
	if (VDO_PXLFMT_CLASS(fmt) == VDO_PXLFMT_CLASS_NVX) {
		return KDRV_IME_OUT_FMT_Y_PACK_UV420;
	}

	switch (fmt) {
	case VDO_PXLFMT_YUV420_PLANAR:
		return KDRV_IME_OUT_FMT_YUV420;

	case VDO_PXLFMT_YUV420:
		return KDRV_IME_OUT_FMT_Y_PACK_UV420;

	case VDO_PXLFMT_Y8:
		return KDRV_IME_OUT_FMT_Y;

	default:
		CTL_IPP_DBG_WRN("unsupport fmt 0x%.8x\r\n", (unsigned int)fmt);
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IME_SCALER kdf_ipp_typecast_ime_scaler(CTL_IPP_SCL_METHOD method)
{
	switch (method) {
	case CTL_IPP_SCL_BICUBIC:
		return KDRV_IME_SCALER_BICUBIC;

	case CTL_IPP_SCL_BILINEAR:
		return KDRV_IME_SCALER_BILINEAR;

	case CTL_IPP_SCL_NEAREST:
		return KDRV_IME_SCALER_NEAREST;

	case CTL_IPP_SCL_INTEGRATION:
		return KDRV_IME_SCALER_INTEGRATION;

	case CTL_IPP_SCL_AUTO:
		return KDRV_IME_SCALER_AUTO;

	default:
		CTL_IPP_DBG_WRN("unsupport scale method %d\r\n", (int)(method));
		return KDRV_IME_SCALER_AUTO;
	}
}

_INLINE KDRV_IME_OSD_FMT_SEL kdf_ipp_typecast_ime_dsfmt(VDO_PXLFMT fmt)
{
	switch (fmt) {
	case VDO_PXLFMT_RGB565:
		return KDRV_IME_OSD_FMT_RGB565;

	case VDO_PXLFMT_ARGB1555:
		return KDRV_IME_OSD_FMT_RGB1555;

	case VDO_PXLFMT_ARGB4444:
		return KDRV_IME_OSD_FMT_RGB4444;

	case VDO_PXLFMT_ARGB8888:
		return KDRV_IME_OSD_FMT_RGB8888;

	default:
		CTL_IPP_DBG_WRN("unsupport datastamp format 0x%.8x\r\n", (unsigned int)fmt);
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IME_OSD_COLOR_KEY_MODE_SEL kdf_ipp_typecast_ime_ds_ckey_mode(CTL_IPP_DS_CKEY_MODE mode)
{
	switch (mode) {
	case CTL_IPP_DS_CKEY_MODE_RGB:
		return KDRV_IME_OSD_CKEY_RGB_MODE;

	case CTL_IPP_DS_CKEY_MODE_ARGB:
		return KDRV_IME_OSD_CKEY_ARGB_MODE;

	default:
		CTL_IPP_DBG_WRN("unsupport datastamp color key mode %d\r\n", (int)(mode));
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IME_OSD_PLT_MODE_SEL kdf_ipp_typecast_ime_ds_plt_mode(CTL_IPP_DS_PLT_MODE mode)
{
	switch (mode) {
	case CTL_IPP_DS_PLT_MODE_1BIT:
		return KDRV_IME_OSD_PLT_1BIT_MODE;

	case CTL_IPP_DS_PLT_MODE_2BIT:
		return KDRV_IME_OSD_PLT_2BIT_MODE;

	case CTL_IPP_DS_PLT_MODE_4BIT:
		return KDRV_IME_OSD_PLT_4BIT_MODE;

	default:
		CTL_IPP_DBG_WRN("unsupport datastamp plt mode %d\r\n", (int)(mode));
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IME_PM_MASK_TYPE kdf_ipp_typecast_ime_pm_type(CTL_IPP_PM_MASK_TYPE type)
{
	switch (type) {
	case CTL_IPP_PM_MASK_TYPE_YUV:
		return KDRV_IME_PM_MASK_TYPE_YUV;

	case CTL_IPP_PM_MASK_TYPE_PXL:
		return KDRV_IME_PM_MASK_TYPE_PXL;

	default:
		CTL_IPP_DBG_WRN("unsupport privact mask type %d\r\n", (int)(type));
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IME_PM_PXL_BLK_SIZE kdf_ipp_typecast_ime_pm_blk(CTL_IPP_PM_PXL_BLK blk)
{
	switch (blk) {
	case CTL_IPP_PM_PXL_BLK_08:
		return KDRV_IME_PM_PIXELATION_08;

	case CTL_IPP_PM_PXL_BLK_16:
		return KDRV_IME_PM_PIXELATION_16;

	case CTL_IPP_PM_PXL_BLK_32:
		return KDRV_IME_PM_PIXELATION_32;

	case CTL_IPP_PM_PXL_BLK_64:
		return KDRV_IME_PM_PIXELATION_64;

	default:
		CTL_IPP_DBG_WRN("unsupport pm pxl blk %d\r\n", (int)(blk));
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE KDRV_IME_LOW_DELAY_CHL_SEL kdf_ipp_typecast_ime_low_delay_ch(UINT32 path)
{
	switch (path) {
	case CTL_IPP_OUT_PATH_ID_1:
		return KDRV_IME_LOW_DELAY_CHL_PATH1;

	case CTL_IPP_OUT_PATH_ID_2:
		return KDRV_IME_LOW_DELAY_CHL_PATH2;

	case CTL_IPP_OUT_PATH_ID_3:
		return KDRV_IME_LOW_DELAY_CHL_PATH3;

	case CTL_IPP_OUT_PATH_ID_5:
		return KDRV_IME_LOW_DELAY_CHL_REFOUT;

	default:
		CTL_IPP_DBG_WRN("unsupport low delay path %d\r\n", (int)(path));
		return KDF_IPP_TYPECAST_FAILED;
	}
}

_INLINE BOOL kdf_ipp_chk_ife2_ready_to_trig(CTL_IPP_IFE2_CTRL *p_ife2_ctrl)
{
	if (p_ife2_ctrl->in_addr == 0) {
		return FALSE;
	}

	if (p_ife2_ctrl->in_size.w == 0 || p_ife2_ctrl->in_size.h == 0 || p_ife2_ctrl->in_lofs == 0) {
		return FALSE;
	}

	return TRUE;
}

#if 0
#endif
static KDF_IPP_HANDLE *p_kdf_ipp_trig_hdl[KDF_IPP_ENG_MAX] = {NULL};

KDF_IPP_HANDLE *kdf_ipp_get_trig_hdl(KDF_IPP_ENG eng)
{
	return p_kdf_ipp_trig_hdl[eng];
}

void kdf_ipp_set_trig_hdl(KDF_IPP_HANDLE *p_hdl, KDF_IPP_ENG eng)
{
	p_kdf_ipp_trig_hdl[eng] = p_hdl;
}

INT32 kdf_ipp_cb_proc(KDF_IPP_HANDLE *p_hdl, KDF_IPP_CBEVT_TYPE cbevt, UINT32 msg, void *in, void *out)
{
	if (p_hdl->cb_fp[cbevt] != NULL) {
		return p_hdl->cb_fp[cbevt](msg, in, out);
	}

	return 0;
}

static void kdf_ipp_cal_vig_cen(KDRV_IFE_IN_IMG_INFO *in_img_info, UPOINT *ratio, UINT32 ratio_base, KDRV_IFE_COORDINATE *center)
{
	center->x = ctl_ipp_util_ratio2value(in_img_info->img_info.width, ratio->x, ratio_base, 0);
	center->y = ctl_ipp_util_ratio2value(in_img_info->img_info.height, ratio->y, ratio_base, 0);

	/* check if out of image */
	if ((UINT32)center->x > in_img_info->img_info.width) {
		center->x = ctl_ipp_util_ratio2value(in_img_info->img_info.width, 500, CTL_IPP_RATIO_UNIT_DFT, 0);
	}
	if ((UINT32)center->y > in_img_info->img_info.height) {
		center->y = ctl_ipp_util_ratio2value(in_img_info->img_info.height, 500, CTL_IPP_RATIO_UNIT_DFT, 0);
	}
}

static INT32 kdf_ipp_set_load(KDF_IPP_HANDLE *p_hdl)
{
	INT32 rt;

	rt = E_OK;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_LOAD, NULL);
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_LOAD, NULL);
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_LOAD, NULL);
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_LOAD, NULL);
	if (p_hdl->engine_used_mask & KDF_IPP_ENG_BIT_IFE2) {
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_hdl, KDRV_IFE2_PARAM_IPL_LOAD, NULL);
	}
	if (rt != E_OK) {
		CTL_IPP_DBG_WRN("ipp load error rt %d\r\n", (int)(rt));
		rt = CTL_IPP_E_KDRV_SET;
	}

	return rt;
}

void kdf_ipp_direct_load(KDF_IPP_HANDLE *p_hdl)
{
	UINT32 dir_load_bit;
	unsigned long loc_cpu_flg;
	UINT32 load;
	//UINT32 temp_load_bit;

	load = FALSE;
	//temp_load_bit = 0;
	dir_load_bit = KDF_IPP_DIR_LOAD_VD | KDF_IPP_DIR_LOAD_FMS;
	loc_cpu(loc_cpu_flg);
	if ((p_hdl->dir_load_bit & dir_load_bit) == dir_load_bit) {
		//temp_load_bit = p_hdl->dir_load_bit;
		p_hdl->dir_load_bit &= ~(dir_load_bit);
		load = TRUE;
	}
	unl_cpu(loc_cpu_flg);

	if (load) {
		/* IPP LOAD */
		kdf_ipp_set_load(p_hdl);
	}

}

void kdf_ipp_direct_load_bit_set(UINT32 *load_bit, UINT32 bit)
{
	unsigned long loc_cpu_flg;

	loc_cpu(loc_cpu_flg);
	*load_bit |= bit;
	unl_cpu(loc_cpu_flg);
}

void kdf_ipp_direct_load_bit_clr(UINT32 *load_bit, UINT32 bit)
{
	unsigned long loc_cpu_flg;

	loc_cpu(loc_cpu_flg);
	*load_bit &= ~bit;
	unl_cpu(loc_cpu_flg);
}

void kdf_ipp_direct_clr_hdl(void)
{
	kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_IFE);
	kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_DCE);
	kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_IPE);
	kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_IME);
	kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_IFE2);
}

UINT32 kdf_ipp_direct_datain_drop(KDF_IPP_HANDLE *p_hdl, UINT32 evt, UINT32 data_in)
{
	UINT32 data_addr_in_process[DATA_IN_NUM] = {0};
	UINT32 proc_end_datain;
	UINT32 i, check_flag;
	CTL_IPP_BASEINFO *p_base, *p_new_base;
	unsigned long loc_cpu_flg;

	// yuv buffer not be pushed when no frame end situation, such as ring buf err happening, causing buffer not released. drop old job here to release buffer.
	if (evt == KDF_IPP_FRAME_BP_START) {
		check_flag = FALSE;
		loc_cpu(loc_cpu_flg);
		/* check data in before frame count n-1*/
		for (i = 0; i < DATA_IN_NUM; i++) {
			if(p_hdl->data_addr_in_process[i] == 0 || p_hdl->frame_sta_cnt == 0) {
				continue;
			}
			// get frame count
			p_base = (CTL_IPP_BASEINFO *) p_hdl->data_addr_in_process[i];
			if (p_base->ctl_frm_sta_count < (p_hdl->fra_sta_with_sop_cnt - 1)) {
				data_addr_in_process[i] = p_hdl->data_addr_in_process[i];
				p_hdl->data_addr_in_process[i] = 0;
				check_flag = TRUE;
			}
		}
		/*shift data in process*/
		if (check_flag) {
			for (i = 0; i < DATA_IN_NUM; i++) {
				if(p_hdl->data_addr_in_process[0] == 0) {
					p_hdl->data_addr_in_process[0] = p_hdl->data_addr_in_process[1];
					p_hdl->data_addr_in_process[1] = p_hdl->data_addr_in_process[2];
					p_hdl->data_addr_in_process[2] = 0;
		 		}
			}
		}
		unl_cpu(loc_cpu_flg);
		/* drop data in before frame count n-1*/

		for (i = 0; i < DATA_IN_NUM; i++) {
			if(data_addr_in_process[i] != 0) {
				kdf_ipp_drop((UINT32)p_hdl, data_addr_in_process[i], 0, p_hdl->cb_fp[KDF_IPP_CBEVT_DROP], CTL_IPP_E_DIR_DROP);
				data_addr_in_process[i] = 0;
			}
		}
	}

	if (evt == KDF_IPP_FRAME_BP) {
		/* drop same frame count ctrl info, drop new*/
		p_new_base = (CTL_IPP_BASEINFO *) data_in;
		for (i = 0; i < DATA_IN_NUM; i++) {
			if(p_hdl->data_addr_in_process[i] == 0) {
				continue;
			}
			p_base = (CTL_IPP_BASEINFO *) p_hdl->data_addr_in_process[i];
			if(p_new_base->ctl_frm_sta_count == p_base->ctl_frm_sta_count) {
				return CTL_IPP_E_INDATA;
			}
		}
	}

	if (evt == KDF_IPP_FRAME_BP_END) {
		/* set new data in. already lock outterly, don't lock cpu here */
		if (p_hdl->data_addr_in_process[0] == 0) {
			p_hdl->data_addr_in_process[0] = data_in;
		} else if (p_hdl->data_addr_in_process[1] == 0) {
			p_hdl->data_addr_in_process[1] = data_in;
		} else if (p_hdl->data_addr_in_process[2] == 0) {
			p_hdl->data_addr_in_process[2] = data_in;
		} else {
			CTL_IPP_DBG_WRN("data Que overflow\r\n");
			return CTL_IPP_E_QOVR;
		}
	}

	if (evt == KDF_IPP_FRAME_END) {
		loc_cpu(loc_cpu_flg);
		p_base = (CTL_IPP_BASEINFO *) p_hdl->data_addr_in_process[0];

		if (p_hdl->frame_sta_cnt == 0 || p_base == NULL) {
			unl_cpu(loc_cpu_flg);
			return 0;
		}

		if (p_base->ctl_frm_sta_count != (p_hdl->fra_sta_with_sop_cnt - 1)) {
			unl_cpu(loc_cpu_flg);
			return 0;
		}
		proc_end_datain = p_hdl->data_addr_in_process[0];
		p_hdl->data_addr_in_process[0] = p_hdl->data_addr_in_process[1];
		p_hdl->data_addr_in_process[1] = p_hdl->data_addr_in_process[2];
		p_hdl->data_addr_in_process[2] = 0;
		unl_cpu(loc_cpu_flg);

		return proc_end_datain;
	}
	return CTL_IPP_E_OK;
}

#if 0
#endif

static ER kdf_ipp_config_start(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	KDF_IPP_ISP_CB_INFO algcb_info;

	algcb_info.type = KDF_IPP_ISP_CB_TYPE_IQ_TRIG;
	algcb_info.base_info_addr = (UINT32)p_base;
	algcb_info.item = ISP_EVENT_IPP_CFGSTART;
	kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&algcb_info, NULL);

	return E_OK;
}

static ER kdf_ipp_config_end(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	KDF_IPP_ISP_CB_INFO algcb_info;

	algcb_info.type = KDF_IPP_ISP_CB_TYPE_IQ_TRIG;
	algcb_info.base_info_addr = (UINT32)p_base;
	algcb_info.item = ISP_EVENT_IPP_CFGEND;
	kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&algcb_info, NULL);

	return E_OK;
}

static ER kdf_ipp_trig_start(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	KDF_IPP_ISP_CB_INFO algcb_info;

	kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PROCSTART, (UINT32)p_hdl, (void *)&p_base, NULL);

	algcb_info.type = KDF_IPP_ISP_CB_TYPE_INT_EVT;
	algcb_info.base_info_addr = (UINT32)p_base;
	algcb_info.item = CTL_IPP_ISP_INT_EVT_TRIG_START;
	kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&algcb_info, NULL);

	// debug dma write protect for frame buffer, yuv buffer and subout. start protecting before trigger
	ctl_ipp_dbg_dma_wp_proc_start(ctl_ipp_info_get_entry_by_info_addr((UINT32)p_base));

	return E_OK;
}

static ER kdf_ipp_trig_end(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	KDF_IPP_ISP_CB_INFO algcb_info;

	p_hdl->trig_ed_time = ctl_ipp_util_get_syst_counter();

	algcb_info.type = KDF_IPP_ISP_CB_TYPE_INT_EVT;
	algcb_info.base_info_addr = (UINT32)p_base;
	algcb_info.item = CTL_IPP_ISP_INT_EVT_TRIG_END;
	kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&algcb_info, NULL);

	/* d2d flow write dtsi for fastboot */
	ctl_ipp_dbg_dump_dtsi2file((UINT32)p_hdl);

	return E_OK;
}

INT32 kdf_ipp_config_ife_base(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	CTL_IPP_IFE_CTRL *p_ife_ctrl;
	KDRV_IFE_IN_IMG_INFO in_img_info;
	KDRV_IFE_IN_OFS in_img_lofs;
	KDRV_IFE_IN_ADDR in_img_addr;
	KDRV_IFE_INTE inte_en;
	KDRV_IFE_PARAM_IPL_FUNC_EN func_en;
	KDRV_IFE_MIRROR mirror_info;
	KDRV_IFE_RDE_INFO in_img_decode;
	KDRV_IFE_FUSION_INFO fusion;
	KDRV_IFE_RING_BUF_INFO dir_hdr_buf;
	KDRV_IFE_WAIT_SIE2_DISABLE_INFO dir_hdr_wait_sie2_disable;
	UINT32 in_frm_num;

	p_ife_ctrl = &p_base->ife_ctrl;
	rt = E_OK;

	/* set interrupt enable */
	inte_en = KDF_IPP_DFT_IFE_INTE;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_INTER, (void *)&inte_en);

	/* default enable all function */
	func_en.enable = ENABLE;
	func_en.ipl_ctrl_func = KDF_IPP_DFT_IFE_FUNCEN;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);

	/* input image info */
	if (p_hdl->flow == KDF_IPP_FLOW_RAW || p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
		in_img_info.in_src = (p_hdl->flow == KDF_IPP_FLOW_RAW) ? KDRV_IFE_IN_MODE_IPP : KDRV_IFE_IN_MODE_DIRECT;
		in_img_info.img_info.width = p_ife_ctrl->in_size.w;
		in_img_info.img_info.height = p_ife_ctrl->in_size.h;
		in_img_info.img_info.crop_hpos = p_ife_ctrl->in_crp_window.x;
		in_img_info.img_info.crop_vpos = p_ife_ctrl->in_crp_window.y;
		in_img_info.img_info.crop_width = p_ife_ctrl->in_crp_window.w;
		in_img_info.img_info.crop_height = p_ife_ctrl->in_crp_window.h;
	    in_img_info.img_info.st_pix = kdf_ipp_typecast_ipp_pix(p_ife_ctrl->in_fmt);
		if (in_img_info.img_info.st_pix == KDF_IPP_TYPECAST_FAILED) {
			return CTL_IPP_E_INDATA;
		}
	    in_img_info.img_info.bit = kdf_ipp_typecast_ipp_rawbit(p_ife_ctrl->in_fmt);
		if (in_img_info.img_info.bit == KDF_IPP_TYPECAST_FAILED) {
			return CTL_IPP_E_INDATA;
		}

		in_img_lofs.line_ofs_1 = p_ife_ctrl->in_lofs[0];
		in_img_lofs.line_ofs_2 = p_ife_ctrl->in_lofs[1];

		in_img_addr.ife_in_addr_1 = p_ife_ctrl->in_addr[0];
		in_img_addr.ife_in_addr_2 = p_ife_ctrl->in_addr[1];

		if (p_ife_ctrl->decode_enable) {
			in_img_decode.enable = ENABLE;
			in_img_decode.encode_rate = kdf_ipp_typecast_ife_decode_ratio(p_ife_ctrl->decode_ratio);
			if (in_img_decode.encode_rate == KDF_IPP_TYPECAST_FAILED) {
				return CTL_IPP_E_INDATA;
			}
		} else {
			in_img_decode.enable = DISABLE;
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_IN_IMG, (void *)&in_img_info);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_RDE_INFO, (void *)&in_img_decode);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_IN_OFS_1, (void *)&in_img_lofs);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_IN_ADDR_1, (void *)&in_img_addr);

		/* SHDR, fnum start from 0 */
		fusion.enable = DISABLE;
		fusion.fnum = 0;
		if (p_ife_ctrl->shdr_enable) {
			in_frm_num = VDO_PXLFMT_PLANE(p_ife_ctrl->in_fmt);
			if (in_frm_num > 1) {
				fusion.enable = ENABLE;
				fusion.fnum = in_frm_num - 1;

				rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_IN_OFS_2, (void *)&in_img_lofs);
				rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_IN_ADDR_2, (void *)&in_img_addr);
				/* direct SHDR */
				if (p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
					if (p_ife_ctrl->dir_buf_num != 0) {
						dir_hdr_buf.dmaloop_en = ENABLE;
						dir_hdr_buf.dmaloop_line = p_ife_ctrl->dir_buf_num;
					} else {
						dir_hdr_buf.dmaloop_en = DISABLE;
						dir_hdr_buf.dmaloop_line = 0;
					}
				} else {
					dir_hdr_buf.dmaloop_en = DISABLE;
					dir_hdr_buf.dmaloop_line = 0;
				}
				rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_RINGBUF_INFO, (void *)&dir_hdr_buf);

				/* frame start will come at the same time in DCG mode, so direct hdr need check sie(2~x) already output one line to dram before ife process */
				if ((p_ife_ctrl->hdr_ref_chk_bit & CTL_IPP_IFE_HDR_REF_SIE2) == 0) {
					dir_hdr_wait_sie2_disable.dma_wait_sie2_start_disable = ENABLE;		// don't check sie2
				} else {
					dir_hdr_wait_sie2_disable.dma_wait_sie2_start_disable = DISABLE;	// check sie2
				}
				rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_WAIT_SIE2_DISABLE, (void *)&dir_hdr_wait_sie2_disable);
			} else {
				CTL_IPP_DBG_WRN("input frm num %d, can not enable hdr mode\r\n", (int)(in_frm_num));
			}
		}
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_FUSION_INFO, (void *)&fusion);

		/* MIRROR */
		mirror_info.enable = (p_ife_ctrl->flip & CTL_IPP_FLIP_H);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_MIRROR, (void *)&mirror_info);
	} else {
		CTL_IPP_DBG_WRN("Unsupport flow for ife %d\r\n", (int)(p_hdl->flow));
		return CTL_IPP_E_PAR;
	}

	return rt;
}

INT32 kdf_ipp_config_ife_vig_center(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	CTL_IPP_ISP_IFE_VIG_CENT_RATIO vig_cent_ratio;
	KDF_IPP_ISP_CB_INFO iqcb_info;
	KDRV_IFE_IN_IMG_INFO in_img_info;
	KDRV_IFE_VIG_CENTER vig;

	rt = E_OK;

	memset((void *)&vig_cent_ratio, 0, sizeof(KDRV_IFE_VIG_CENTER));
	iqcb_info.type = KDF_IPP_ISP_CB_TYPE_IQ_GET;
	iqcb_info.item = CTL_IPP_ISP_ITEM_IFE_VIG_CENT;
	iqcb_info.data_addr = (UINT32)&vig_cent_ratio;
	kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&iqcb_info, NULL);

	/* calculate center based on ratio */
	rt |= kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_IN_IMG, (void *)&in_img_info);
	if (vig_cent_ratio.ratio_base != 0) {
		/* mapping ratio channel depends on cfa pattern */
		switch (in_img_info.img_info.st_pix) {
		case KDRV_IPP_RGGB_PIX_GR:
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch1, vig_cent_ratio.ratio_base, &vig.ch0);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch2, vig_cent_ratio.ratio_base, &vig.ch1);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch3, vig_cent_ratio.ratio_base, &vig.ch2);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch0, vig_cent_ratio.ratio_base, &vig.ch3);
			break;
		case KDRV_IPP_RGGB_PIX_GB:
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch2, vig_cent_ratio.ratio_base, &vig.ch0);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch3, vig_cent_ratio.ratio_base, &vig.ch1);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch1, vig_cent_ratio.ratio_base, &vig.ch2);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch0, vig_cent_ratio.ratio_base, &vig.ch3);
			break;
		case KDRV_IPP_RGGB_PIX_B:
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch3, vig_cent_ratio.ratio_base, &vig.ch0);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch0, vig_cent_ratio.ratio_base, &vig.ch1);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch1, vig_cent_ratio.ratio_base, &vig.ch2);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch2, vig_cent_ratio.ratio_base, &vig.ch3);
			break;
		default:
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch0, vig_cent_ratio.ratio_base, &vig.ch0);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch1, vig_cent_ratio.ratio_base, &vig.ch1);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch2, vig_cent_ratio.ratio_base, &vig.ch2);
			kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch3, vig_cent_ratio.ratio_base, &vig.ch3);
			break;
		}
	} else {
		/* ratio base 0, use default 1/2 to calculate center */
		vig_cent_ratio.ratio_base = CTL_IPP_RATIO_UNIT_DFT;
		vig_cent_ratio.ch0.x = CTL_IPP_RATIO_UNIT_DFT / 2;
		vig_cent_ratio.ch0.y = CTL_IPP_RATIO_UNIT_DFT / 2;
		kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch0, vig_cent_ratio.ratio_base, &vig.ch0);
		kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch0, vig_cent_ratio.ratio_base, &vig.ch1);
		kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch0, vig_cent_ratio.ratio_base, &vig.ch2);
		kdf_ipp_cal_vig_cen(&in_img_info, &vig_cent_ratio.ch0, vig_cent_ratio.ratio_base, &vig.ch3);
	}

	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_VIG_CENTER, (void *)&vig);

	return rt;
}

static INT32 kdf_ipp_config_ife_common(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	KDRV_IFE_PARAM_IPL_FUNC_EN func_en;

	rt = E_OK;
	rt |= kdf_ipp_config_ife_base(p_hdl, p_base);
	rt |= kdf_ipp_config_ife_vig_center(p_hdl, p_base);

	/* isp debug mode, disable all iq func */
	if (ctl_ipp_dbg_get_isp_mode()) {
		func_en.enable = DISABLE;
		func_en.ipl_ctrl_func = KDF_IPP_DFT_IFE_FUNCEN;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
	}

	if (rt != E_OK) {
		return CTL_IPP_E_KDRV_SET;
	}
	return CTL_IPP_E_OK;
}

INT32 kdf_ipp_config_dce_base(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	CTL_IPP_DCE_CTRL *p_dce_ctrl;
	KDRV_DCE_IN_INFO in_info;
	KDRV_DCE_IN_IMG_INFO in_img_info;
	KDRV_DCE_OUT_IMG_INFO out_img_info;
	KDRV_DCE_PARAM_IPL_FUNC_EN func_en;
	KDRV_DCE_INTE inte_en;
	KDRV_DCE_DC_SRAM_SEL sram_sel;
	KDRV_DCE_STRP_RULE_INFO strp_rule;
	KDRV_DCE_STRIPE_PARAM strp_param;

	p_dce_ctrl = &p_base->dce_ctrl;
	rt = E_OK;

	/* set interrupt enable */
	inte_en = KDF_IPP_DFT_DCE_INTE;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_INTER, (void *)&inte_en);

	/* default enable all function */
	func_en.enable = ENABLE;
	func_en.ipl_ctrl_func = KDF_IPP_DFT_DCE_FUNCEN;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);

	/* input/output image info */
	if (p_hdl->flow == KDF_IPP_FLOW_RAW || p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
	    in_info.in_src = (p_hdl->flow == KDF_IPP_FLOW_RAW) ? KDRV_DCE_IN_SRC_DIRECT : KDRV_DCE_IN_SRC_ALL_DIRECT;
		in_info.type = 0;	/* dont care in stream mode */

		memset((void *)&in_img_info, 0, sizeof(KDRV_DCE_IN_IMG_INFO));
		in_img_info.cfa_pat = kdf_ipp_typecast_ipp_pix(p_dce_ctrl->in_fmt);
		if (in_img_info.cfa_pat == KDF_IPP_TYPECAST_FAILED) {
			return CTL_IPP_E_INDATA;
		}
	    in_img_info.img_size_h = p_dce_ctrl->in_size.w;
	    in_img_info.img_size_v = p_dce_ctrl->in_size.h;
		in_img_info.lineofst[0] = in_img_info.img_size_h; /* dont care in stream mode */
		in_img_info.lineofst[1] = in_img_info.img_size_h; /* dont care in stream mode */
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IN_INFO, (void *)&in_info);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IN_IMG, (void *)&in_img_info);

		out_img_info.crop_size_h = p_dce_ctrl->out_crp_window.w;
		out_img_info.crop_size_v = p_dce_ctrl->out_crp_window.h;
		out_img_info.crop_start_h = p_dce_ctrl->out_crp_window.x;
		out_img_info.crop_start_v = p_dce_ctrl->out_crp_window.y;
		out_img_info.lineofst[0] = 0;	/* dont care in stream mode */
		out_img_info.lineofst[1] = 0;	/* dont care in stream mode */
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
	} else if (p_hdl->flow == KDF_IPP_FLOW_DCE_D2D) {
		KDRV_DCE_IMG_IN_DMA_INFO dma_in;
		KDRV_DCE_IMG_OUT_DMA_INFO dma_out;

		in_info.in_src = KDRV_DCE_IN_SRC_DRAM;
		in_info.type = kdf_ipp_typecast_dce_fmt(p_dce_ctrl->in_fmt);
		if (in_info.type == KDF_IPP_TYPECAST_FAILED) {
			return CTL_IPP_E_INDATA;
		}

		memset((void *)&in_img_info, 0, sizeof(KDRV_DCE_IN_IMG_INFO));
		in_img_info.cfa_pat = 0;	/* dont care in d2d mode*/
	    in_img_info.img_size_h = p_dce_ctrl->in_size.w;
	    in_img_info.img_size_v = p_dce_ctrl->in_size.h;
		in_img_info.lineofst[0] = p_dce_ctrl->in_lofs[0];
		in_img_info.lineofst[1] = p_dce_ctrl->in_lofs[1];
		dma_in.addr_y = p_dce_ctrl->in_addr[0];
		dma_in.addr_cbcr = p_dce_ctrl->in_addr[1];
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IN_INFO, (void *)&in_info);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IN_IMG, (void *)&in_img_info);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IMG_DMA_IN, (void *)&dma_in);

		out_img_info.crop_size_h = p_dce_ctrl->out_crp_window.w;
		out_img_info.crop_size_v = p_dce_ctrl->out_crp_window.h;
		out_img_info.crop_start_h = p_dce_ctrl->out_crp_window.x;
		out_img_info.crop_start_v = p_dce_ctrl->out_crp_window.y;
		out_img_info.lineofst[0] = p_dce_ctrl->out_lofs[0];
		out_img_info.lineofst[1] = p_dce_ctrl->out_lofs[1];
		dma_out.addr_y = p_dce_ctrl->out_addr[0];
		dma_out.addr_cbcr = p_dce_ctrl->out_addr[1];
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IMG_DMA_OUT, (void *)&dma_out);

		/* disable cfa due to yuv in/out */
		func_en.enable = DISABLE;
		func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_CFA;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
	} else if (p_hdl->flow == KDF_IPP_FLOW_CCIR) {
		KDRV_DCE_IMG_IN_DMA_INFO dma_in;

		in_info.in_src = KDRV_DCE_IN_SRC_DCE_IME;
		in_info.yuvfmt = kdf_ipp_typecast_dce_yuvfmt(p_dce_ctrl->in_fmt);
		in_info.type = kdf_ipp_typecast_dce_fmt(p_dce_ctrl->in_fmt);
		if (in_info.type == KDF_IPP_TYPECAST_FAILED) {
			return CTL_IPP_E_INDATA;
		}

		memset((void *)&in_img_info, 0, sizeof(KDRV_DCE_IN_IMG_INFO));
		in_img_info.cfa_pat = 0;	/* dont care in ccir mode*/
	    in_img_info.img_size_h = p_dce_ctrl->in_size.w;
	    in_img_info.img_size_v = p_dce_ctrl->in_size.h;
		in_img_info.lineofst[0] = p_dce_ctrl->in_lofs[0];
		in_img_info.lineofst[1] = p_dce_ctrl->in_lofs[1];
		dma_in.addr_y = p_dce_ctrl->in_addr[0];
		dma_in.addr_cbcr = p_dce_ctrl->in_addr[1];
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IN_INFO, (void *)&in_info);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IN_IMG, (void *)&in_img_info);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IMG_DMA_IN, (void *)&dma_in);

		out_img_info.crop_size_h = p_dce_ctrl->out_crp_window.w;
		out_img_info.crop_size_v = p_dce_ctrl->out_crp_window.h;
		out_img_info.crop_start_h = p_dce_ctrl->out_crp_window.x;
		out_img_info.crop_start_v = p_dce_ctrl->out_crp_window.y;
		out_img_info.lineofst[0] = 0;	/* dont care in stream mode */
		out_img_info.lineofst[1] = 0;	/* dont care in stream mode */
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
	} else {
		CTL_IPP_DBG_WRN("Unsupport flow for dce %d\r\n", (int)(p_hdl->flow));
		return CTL_IPP_E_PAR;
	}

	/* dce sram sel, dce stripe rule sel
		when gdc with too large distortion, need to use CNN sram and set corresponding CNN register
		note: gdc_enable indicate cnn sram can be used by dce
	*/
	if (p_dce_ctrl->gdc_enable == ENABLE) {
		sram_sel.sram_mode = KDRV_DCE_SRAM_CNN;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_DC_SRAM_MODE, (void *)&sram_sel);
	} else {
		sram_sel.sram_mode = KDRV_DCE_SRAM_DCE;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_DC_SRAM_MODE, (void *)&sram_sel);

		/* if bind config, force disable gdc/cac
			else leave for iq config
		*/
		#if KDF_IPP_BIND_DCE_GDC_WITH_SRAM_CONFIG
		func_en.enable = DISABLE;
		func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_DC | KDRV_DCE_IQ_FUNC_CAC;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
		#endif
	}

	/* stripe rule select */
	if (p_dce_ctrl->strp_rule > CTL_IPP_STRP_RULE_NSTRP_OFS) {
		// set kdrv stripe type by hdal stripe rule (dont care kdrv stripe rule)
		strp_param.stripe_type = kdf_ipp_typecast_dce_stripe_type(p_dce_ctrl->strp_rule);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IQ_STRIPE_PARAM, (void *)&strp_param);
		strp_rule.dce_strp_rule = kdf_ipp_typecast_dce_stripe_rule(CTL_IPP_STRP_RULE_1); // dont care
	} else {
		// set kdrv stripe rule by hdal stripe rule
		strp_rule.dce_strp_rule = kdf_ipp_typecast_dce_stripe_rule(p_dce_ctrl->strp_rule);
	}
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_STRIPE_RULE, (void *)&strp_rule);

	return rt;
}

INT32 kdf_ipp_config_dce_dc_center(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	CTL_IPP_DCE_CTRL *p_dce_ctrl;
	UPOINT ratio;
	UINT32 base;
	KDRV_DCE_GDC_CENTER_INFO dc_center;
	KDF_IPP_ISP_CB_INFO iqcb_info;
	CTL_IPP_ISP_DCE_DC_CENT_RATIO dce_cent_ratio;

	p_dce_ctrl = &p_base->dce_ctrl;
	rt = E_OK;

	memset((void *)&dce_cent_ratio, 0, sizeof(CTL_IPP_ISP_DCE_DC_CENT_RATIO));
	iqcb_info.type = KDF_IPP_ISP_CB_TYPE_IQ_GET;
	iqcb_info.item = CTL_IPP_ISP_ITEM_DCE_DC_CENT;
	iqcb_info.data_addr = (UINT32)&dce_cent_ratio;
	kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&iqcb_info, NULL);

	/* calculate center based on ratio */
	base = CTL_IPP_RATIO_UNIT_DFT;
	ratio.x = base / 2;
	ratio.y = base / 2;

	if (dce_cent_ratio.ratio_base != 0) {
		base = dce_cent_ratio.ratio_base;
		ratio = dce_cent_ratio.center;

		if (ratio.x > base) {
			ratio.x = base / 2;
		}
		if (ratio.y > base) {
			ratio.y = base / 2;
		}
	}

	dc_center.geo_center_x = ctl_ipp_util_ratio2value(p_dce_ctrl->in_size.w, ratio.x, base, 0);
	dc_center.geo_center_y = ctl_ipp_util_ratio2value(p_dce_ctrl->in_size.h, ratio.y, base, 0);
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_GDC_CENTER, (void *)&dc_center);

	return rt;
}

INT32 kdf_ipp_config_dce_cfa_subout(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base, KDRV_DCE_DRAM_SINGLE_OUT_EN *p_single_out)
{
	INT32 rt;
	CTL_IPP_DCE_CTRL *p_dce_ctrl;
	KDRV_DCE_CFA_SUBOUT_INFO cfa_subout;
	CTL_IPP_IME_OUT_IMG *p_path6;

	p_dce_ctrl = &p_base->dce_ctrl;
	rt = E_OK;

	p_path6 = &p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_6];
	cfa_subout.cfa_subout_enable = (p_path6->dma_enable == ENABLE) ? ENABLE : DISABLE;
	if (cfa_subout.cfa_subout_enable) {
		cfa_subout.cfa_addr = p_path6->addr[0];
		cfa_subout.cfa_lofs = p_path6->lofs[0];
		cfa_subout.cfa_subout_flip_enable = p_path6->flip_enable;
		cfa_subout.subout_byte = (p_path6->fmt == VDO_PXLFMT_520_IR16) ? KDRV_DCE_CFA_SUBOUT_2BYTE : KDRV_DCE_CFA_SUBOUT_1BYTE;
		cfa_subout.subout_shiftbit = (p_path6->fmt == VDO_PXLFMT_520_IR8) ? 2 : 0;
		cfa_subout.subout_ch_sel = kdf_ipp_typecast_dce_cfa_subout_ch(p_dce_ctrl->in_fmt);
		if (cfa_subout.subout_ch_sel == KDF_IPP_TYPECAST_FAILED) {
			return CTL_IPP_E_INDATA;
		}
	}
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_CFAOUT, (void *)&cfa_subout);

	if (cfa_subout.cfa_subout_enable) {
		p_single_out->single_out_cfa_en = ENABLE;
	}

	return rt;
}

INT32 kdf_ipp_config_dce_wdr(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base, KDRV_DCE_DRAM_SINGLE_OUT_EN *p_single_out)
{
	INT32 rt;
	CTL_IPP_DCE_CTRL *p_dce_ctrl;
	KDRV_DCE_SUBIMG_IN_ADDR wdr_in_info;
	KDRV_DCE_SUBIMG_OUT_ADDR wdr_out_info;
	KDRV_DCE_PARAM_IPL_FUNC_EN func_en;

	p_dce_ctrl = &p_base->dce_ctrl;
	rt = E_OK;

	wdr_out_info.enable = p_dce_ctrl->wdr_enable;
	wdr_out_info.addr = p_dce_ctrl->wdr_out_addr;
	if (wdr_out_info.addr == 0) {
		wdr_out_info.enable = DISABLE;
	}
	if (wdr_out_info.enable) {
		p_single_out->single_out_wdr_en = ENABLE;
	}
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_DMA_SUBOUT, (void *)&wdr_out_info);

	wdr_in_info.addr = p_dce_ctrl->wdr_in_addr;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_DMA_SUBIN, (void *)&wdr_in_info);

	/* wdr error handle */
	if (wdr_in_info.addr == 0) {
		func_en.enable = DISABLE;
		func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_WDR;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
	}

	return rt;
}

static INT32 kdf_ipp_config_dce_common(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	KDRV_DCE_PARAM_IPL_FUNC_EN func_en;
	KDRV_DCE_DRAM_OUTPUT_MODE out_mode;
	KDRV_DCE_DRAM_SINGLE_OUT_EN single_out_en;

	rt = E_OK;

	/* single out init */
	memset((void *)&single_out_en, 0, sizeof(KDRV_DCE_DRAM_SINGLE_OUT_EN));
	out_mode = KDRV_DCE_DRAM_OUT_SINGLE;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_OUTPUT_MODE, (void *)&out_mode);

	/* basic config */
	rt |= kdf_ipp_config_dce_base(p_hdl, p_base);

	/* dc_cac dc center calculation, need to update at each frame */
	kdf_ipp_config_dce_dc_center(p_hdl, p_base);

	/* cfa subout ctrl */
	kdf_ipp_config_dce_cfa_subout(p_hdl, p_base, &single_out_en);

	/* wdr ctrl */
	kdf_ipp_config_dce_wdr(p_hdl, p_base, &single_out_en);

	/* set single out at last */
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_SINGLE_OUT, (void *)&single_out_en);

	/* isp debug mode, disable all iq func */
	if (ctl_ipp_dbg_get_isp_mode()) {
		func_en.enable = DISABLE;
		func_en.ipl_ctrl_func = KDF_IPP_DFT_DCE_FUNCEN;
		if (p_hdl->flow == KDF_IPP_FLOW_RAW || p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
			/* raw flow must enable cfa */
			func_en.ipl_ctrl_func &= ~(KDRV_DCE_IQ_FUNC_CFA);
		}
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
	}

	if (rt != E_OK) {
		return CTL_IPP_E_KDRV_SET;
	}
	return CTL_IPP_E_OK;
}

INT32 kdf_ipp_config_ipe_base(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base, KDRV_IPE_DRAM_SINGLE_OUT_EN *p_single_out)
{
	INT32 rt;
	CTL_IPP_IPE_CTRL *p_ipe_ctrl;
	KDRV_IPE_IN_IMG_INFO in_img_info;
	KDRV_IPE_DMA_ADDR_INFO out_addr_info;
	KDRV_IPE_PARAM_IPL_FUNC_EN func_en;
	KDRV_IPE_INTE inte_en;

	p_ipe_ctrl = &p_base->ipe_ctrl;
	rt = E_OK;

	/* set interrupt enable */
	inte_en = KDF_IPP_DFT_IPE_INTE;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_INTER, (void *)&inte_en);

	/* default enable all function */
	func_en.enable = ENABLE;
	func_en.ipl_ctrl_func = KDF_IPP_DFT_IPE_FUNCEN;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);

	/* set input img parameters */
	if (p_hdl->flow == KDF_IPP_FLOW_RAW || p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW || p_hdl->flow == KDF_IPP_FLOW_CCIR) {
		KDRV_IPE_ETH_INFO eth_info;

		in_img_info.in_src = (p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) ? KDRV_IPE_IN_SRC_ALL_DIRECT : KDRV_IPE_IN_SRC_DIRECT;
		in_img_info.type = 0;	/* dont care in stream mode */
		in_img_info.ch.y_width = p_ipe_ctrl->in_size.w;
		in_img_info.ch.y_height = p_ipe_ctrl->in_size.h;
		in_img_info.ch.y_line_ofs = p_ipe_ctrl->in_lofs[0];
		in_img_info.ch.uv_line_ofs = p_ipe_ctrl->in_lofs[1];
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_IN_IMG, (void *)&in_img_info);

		/* eth */
		eth_info.enable = p_ipe_ctrl->eth.enable;
		eth_info.eth_outsel = kdf_ipp_typecast_ipe_eth_out_sel(p_ipe_ctrl->eth.out_sel);
		if (eth_info.eth_outsel == KDF_IPP_TYPECAST_FAILED) {
			eth_info.enable = DISABLE;
		}

		eth_info.eth_outfmt = p_ipe_ctrl->eth.out_bit_sel == 0 ? KDRV_ETH_OUT_2BITS : KDRV_ETH_OUT_8BITS;
		eth_info.outsel_h = p_ipe_ctrl->eth.h_out_sel == 0 ? KDRV_IPE_ETH_POS_EVEN : KDRV_IPE_ETH_POS_ODD;
		eth_info.outsel_v = p_ipe_ctrl->eth.v_out_sel == 0 ? KDRV_IPE_ETH_POS_EVEN : KDRV_IPE_ETH_POS_ODD;
		eth_info.th_low = p_ipe_ctrl->eth.th_low;
		eth_info.th_mid = p_ipe_ctrl->eth.th_mid;
		eth_info.th_high = p_ipe_ctrl->eth.th_high;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_ETH, (void *)&eth_info);

		/* isp_eth addr[0] for original; addr[1] for downsample
			ipe addr_y for downsample; addr_uv for original
		*/
		out_addr_info.addr_y = p_ipe_ctrl->eth.buf_addr[1];
		out_addr_info.addr_uv = p_ipe_ctrl->eth.buf_addr[0];
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_OUT_ADDR, (void *)&out_addr_info);

		if (eth_info.enable == ENABLE) {
			if (eth_info.eth_outsel == KDRV_ETH_OUT_DOWNSAMPLED || eth_info.eth_outsel == KDRV_ETH_OUT_BOTH) {
				p_single_out->single_out_y_en = ENABLE;
			}

			if (eth_info.eth_outsel == KDRV_ETH_OUT_ORIGINAL || eth_info.eth_outsel == KDRV_ETH_OUT_BOTH) {
				p_single_out->single_out_c_en = ENABLE;
			}

		}
	} else if (p_hdl->flow == KDF_IPP_FLOW_IPE_D2D) {
		KDRV_IPE_OUT_IMG_INFO out_img_info;
		KDRV_IPE_DMA_ADDR_INFO in_addr_info;

		in_img_info.in_src = KDRV_IPE_IN_SRC_DRAM;
		in_img_info.yuv_range_fmt = kdf_ipp_typecast_ipe_yuvfmt(p_ipe_ctrl->in_fmt);
		in_img_info.type = kdf_ipp_typecast_ipe_in_fmt(p_ipe_ctrl->in_fmt);
		if (in_img_info.type == KDF_IPP_TYPECAST_FAILED) {
			return CTL_IPP_E_INDATA;
		}
		in_img_info.ch.y_width = p_ipe_ctrl->in_size.w;
		in_img_info.ch.y_height = p_ipe_ctrl->in_size.h;
		in_img_info.ch.y_line_ofs = p_ipe_ctrl->in_lofs[0];
		in_img_info.ch.uv_line_ofs = p_ipe_ctrl->in_lofs[1];
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_IN_IMG, (void *)&in_img_info);

		in_addr_info.addr_y = p_ipe_ctrl->in_addr[0];
		in_addr_info.addr_uv = p_ipe_ctrl->in_addr[1];
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_IN_ADDR, (void *)&in_addr_info);

		out_img_info.dram_out = ENABLE;
		out_img_info.type = kdf_ipp_typecast_ipe_out_fmt(p_ipe_ctrl->out_fmt);
		if (out_img_info.type == KDF_IPP_TYPECAST_FAILED) {
			return CTL_IPP_E_PAR;
		}
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_OUT_IMG, (void *)&out_img_info);

		out_addr_info.addr_y = p_ipe_ctrl->out_addr[0];
		out_addr_info.addr_uv = p_ipe_ctrl->out_addr[1];
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_OUT_ADDR, (void *)&out_addr_info);

		p_single_out->single_out_y_en = ENABLE;
		p_single_out->single_out_c_en = ENABLE;
	} else {
		return CTL_IPP_E_PAR;
	}

	return rt;
}

INT32 kdf_ipp_config_ipe_defog_lce(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base, KDRV_IPE_DRAM_SINGLE_OUT_EN *p_single_out)
{
	INT32 rt;
	CTL_IPP_IPE_CTRL *p_ipe_ctrl;
	KDRV_IPE_DMA_SUBIN_ADDR sub_in_info;
	KDRV_IPE_DMA_SUBOUT_ADDR sub_out_info;
	KDRV_IPE_PARAM_IPL_FUNC_EN func_en;

	p_ipe_ctrl = &p_base->ipe_ctrl;
	rt = E_OK;

	/* sub image ctrl, for defog and lce, always try to enable */
	sub_out_info.enable = ENABLE;
	sub_out_info.addr = p_ipe_ctrl->defog_out_addr;
	if (sub_out_info.addr == 0) {
		sub_out_info.enable = DISABLE;
	}
	if (sub_out_info.enable) {
		p_single_out->single_out_defog_en = ENABLE;
	}
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_SUBOUT_ADDR, (void *)&sub_out_info);

	sub_in_info.addr = p_ipe_ctrl->defog_in_addr;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_SUBIN_ADDR, (void *)&sub_in_info);

	/* defog func ctrl */
	func_en.enable = p_ipe_ctrl->defog_enable;
	func_en.ipl_ctrl_func = KDRV_IPE_IQ_FUNC_DEFOG;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);

	/* defog, lce error handle */
	if (sub_in_info.addr == 0) {
		func_en.enable = DISABLE;
		func_en.ipl_ctrl_func = KDRV_IPE_IQ_FUNC_DEFOG | KDRV_IPE_IQ_FUNC_LCE;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
	}

	return rt;
}

INT32 kdf_ipp_config_ipe_va(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base, KDRV_IPE_DRAM_SINGLE_OUT_EN *p_single_out)
{
	INT32 rt;
	CTL_IPP_IPE_CTRL *p_ipe_ctrl;
	KDRV_IPE_DMA_VA_ADDR va_out_info;
	KDRV_IPE_VA_WIN_INFO va_out_window;
	KDRV_IPE_PARAM_IPL_FUNC_EN func_en;
	KDF_IPP_ISP_CB_INFO iqcb_info;

	p_ipe_ctrl = &p_base->ipe_ctrl;
	rt = E_OK;

	memset((void *)&va_out_window, 0, sizeof(KDRV_IPE_VA_WIN_INFO));
	iqcb_info.type = KDF_IPP_ISP_CB_TYPE_IQ_GET;
	iqcb_info.item = CTL_IPP_ISP_ITEM_VA_WIN_SIZE;
	iqcb_info.data_addr = (UINT32)&va_out_window;
	kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&iqcb_info, NULL);

	if (p_ipe_ctrl->va_enable && p_ipe_ctrl->va_out_addr != 0) {
		va_out_info.addr_va = p_ipe_ctrl->va_out_addr;
		p_single_out->single_out_va_en = ENABLE;
	}
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_VA_ADDR, (void *)&va_out_info);
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_VA_WIN_INFO, (void *)&va_out_window);

	/* error handle
		1. va output enable = 0
		2. va window ratio base = 0
	*/
	if (p_single_out->single_out_va_en == DISABLE || va_out_window.ratio_base == 0) {
		func_en.enable = DISABLE;
		func_en.ipl_ctrl_func = KDRV_IPE_IQ_FUNC_VA | KDRV_IPE_IQ_FUNC_VA_INDEP;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
	}

	return rt;
}

static INT32 kdf_ipp_config_ipe_common(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	KDRV_IPE_PARAM_IPL_FUNC_EN func_en;
	KDRV_IPE_DRAM_OUTPUT_MODE out_mode;
	KDRV_IPE_DRAM_SINGLE_OUT_EN single_out_en;

	rt = E_OK;

	/* single out init */
	memset((void *)&single_out_en, 0, sizeof(KDRV_IPE_DRAM_SINGLE_OUT_EN));
	out_mode = KDRV_IPE_DRAM_OUT_SINGLE;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_OUTPUT_MODE, (void *)&out_mode);

	/* basic conifg */
	rt |= kdf_ipp_config_ipe_base(p_hdl, p_base, &single_out_en);

	/* dce lce ctrl */
	rt |= kdf_ipp_config_ipe_defog_lce(p_hdl, p_base, &single_out_en);

	/* va ctrl */
	rt |= kdf_ipp_config_ipe_va(p_hdl, p_base, &single_out_en);

	/* set single out at last */
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_SINGLE_OUT, (void *)&single_out_en);

	/* isp debug mode, disable all iq func */
	if (ctl_ipp_dbg_get_isp_mode()) {
		func_en.enable = DISABLE;
		func_en.ipl_ctrl_func = KDF_IPP_DFT_IPE_FUNCEN;
		func_en.ipl_ctrl_func &= ~(KDRV_IPE_IQ_FUNC_CST);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
	}

	if (rt != E_OK) {
		return CTL_IPP_E_KDRV_SET;
	}
	return CTL_IPP_E_OK;
}

INT32 kdf_ipp_config_ime_path(KDF_IPP_HANDLE *hdl, CTL_IPP_IME_CTRL *p_ime_ctrl, CTL_IPP_OUT_PATH_ID pid)
{
	INT32 rt;
	CTL_IPP_IME_OUT_IMG *p_ime_path;
	KDRV_IME_OUT_PATH_IMG_INFO out_img_info;
	KDRV_IME_OUT_PATH_ADDR_INFO out_addr;
	KDRV_IME_ENCODE_INFO enc_info;

	memset((void *)&out_img_info, 0, sizeof(KDRV_IME_OUT_PATH_IMG_INFO));
	memset((void *)&out_addr, 0, sizeof(KDRV_IME_OUT_PATH_ADDR_INFO));
	p_ime_path = &p_ime_ctrl->out_img[pid];
	out_img_info.path_id = pid;
	rt = E_OK;

	if (pid == CTL_IPP_OUT_PATH_ID_5) {
		/* PATH 5 use refout
			output size = input size
		*/
		KDRV_IME_TMNR_REF_OUT_IMG_INFO ref_out;

		memset((void *)&ref_out, 0, sizeof(KDRV_IME_TMNR_REF_OUT_IMG_INFO));
		if (p_ime_path->enable == ENABLE) {
			if (p_ime_path->fmt != VDO_PXLFMT_YUV420 &&
				p_ime_path->fmt != VDO_PXLFMT_YUV420_NVX2) {
				p_ime_path->dma_enable = DISABLE;
				CTL_IPP_DBG_WRN("path5 not support fmt 0x%.8x", (unsigned int)p_ime_path->fmt);
			}
			if (p_ime_path->size.w != p_ime_ctrl->in_size.w ||
				p_ime_path->size.h != p_ime_ctrl->in_size.h) {
				p_ime_path->dma_enable = DISABLE;
				CTL_IPP_DBG_WRN("path5 not support scale, in(%d %d), out(%d %d)\r\n",
					(int)p_ime_ctrl->in_size.w, (int)p_ime_ctrl->in_size.h, (int)p_ime_path->size.w, (int)p_ime_path->size.h);
			}
			if (p_ime_path->crp_window.x != 0 ||
				p_ime_path->crp_window.y != 0 ||
				p_ime_path->crp_window.w != p_ime_path->size.w ||
				p_ime_path->crp_window.h != p_ime_path->size.h) {
				p_ime_path->dma_enable = DISABLE;
				CTL_IPP_DBG_WRN("path5 not support crop, scl(%d %d), crp(%d %d %d %d)\r\n",
					(int)p_ime_path->size.w, (int)p_ime_path->size.h,
					(int)p_ime_path->crp_window.x, (int)p_ime_path->crp_window.y, (int)p_ime_path->crp_window.w, (int)p_ime_path->crp_window.h);
			}
			ref_out.img_line_ofs_y = p_ime_path->lofs[0];
			ref_out.img_line_ofs_cb = p_ime_path->lofs[1];

			if (p_ime_path->dma_enable) {
				ref_out.img_addr_y = p_ime_path->addr[0];
				ref_out.img_addr_cb = p_ime_path->addr[1];
			}

			if (p_ime_path->fmt == VDO_PXLFMT_YUV420_NVX2) {
				/* encode check 16x align */
				if (p_ime_path->size.w == ALIGN_CEIL_16(p_ime_path->size.w)) {
					ref_out.encode.enable = ENABLE;
				} else {
					ref_out.encode.enable = DISABLE;
					p_ime_path->dma_enable = DISABLE;
					CTL_IPP_DBG_WRN("YUV Compress width must be 16x, %d\r\n", (int)(p_ime_path->size.w));
				}
			} else {
				ref_out.encode.enable = DISABLE;
			}
		}
		ref_out.flip_en = p_ime_path->flip_enable;
		ref_out.enable = p_ime_path->dma_enable;
		ref_out.buf_flush = KDRV_IME_NOTDO_BUF_FLUSH;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, hdl, KDRV_IME_PARAM_IPL_TMNR_REF_OUT_IMG, (void *)&ref_out);
	} else {
		if (p_ime_path->enable == ENABLE) {
			/* path1 encode
				check width 16x align
				check no scale up
				another size check need stripe information
				if MST, output size must = input size(no crop & scale)
			*/
			if (pid == CTL_IPP_OUT_PATH_ID_1) {
				if (p_ime_path->fmt == VDO_PXLFMT_YUV420_NVX2) {
					if (p_ime_path->size.w != ALIGN_CEIL_16(p_ime_path->size.w)) {
						enc_info.enable = DISABLE;
						p_ime_path->dma_enable = DISABLE;
						CTL_IPP_DBG_WRN("YUV Compress width must be 16x, %d\r\n", (int)(p_ime_path->size.w));
					} else {
						enc_info.enable = ENABLE;
					}
				} else {
					enc_info.enable = DISABLE;
				}
				rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, hdl, KDRV_IME_PARAM_IPL_ENCODE, (void *)&enc_info);
			}

			/* output path info */
			out_img_info.type = kdf_ipp_typecast_ime_out_fmt(p_ime_path->fmt);
			if (out_img_info.type == KDF_IPP_TYPECAST_FAILED) {
				p_ime_path->dma_enable = DISABLE;
			}
			out_img_info.ch[0].width = p_ime_path->size.w;
			out_img_info.ch[0].height = p_ime_path->size.h;
			out_img_info.ch[0].line_ofs = p_ime_path->lofs[0];
			out_img_info.ch[1].line_ofs = p_ime_path->lofs[1];
			out_img_info.crp_window.x = p_ime_path->crp_window.x;
			out_img_info.crp_window.y = p_ime_path->crp_window.y;
			out_img_info.crp_window.w = p_ime_path->crp_window.w;
			out_img_info.crp_window.h = p_ime_path->crp_window.h;

			if (p_ime_path->dma_enable) {
				out_addr.path_id = pid;
				out_addr.addr_info.addr_y = p_ime_path->addr[0];
				out_addr.addr_info.addr_cb = p_ime_path->addr[1];
				out_addr.addr_info.addr_cr = p_ime_path->addr[2];
				rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, hdl, KDRV_IME_PARAM_IPL_OUT_ADDR, (void *)&out_addr);
			}
		}
		out_img_info.path_flip_en = p_ime_path->flip_enable;
		out_img_info.path_en = p_ime_path->dma_enable;
		out_img_info.dma_flush = KDRV_IME_NOTDO_BUF_FLUSH;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, hdl, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
	}

	return rt;
}

INT32 kdf_ipp_config_ime_datastamp(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	static CTL_IPP_DS_CB_OUTPUT_INFO dscb_out_info;
	static CTL_IPP_DS_CB_INPUT_INFO dscb_in_info;
	INT32 rt;
	UINT32 i;
	UINT32 color_key;
	KDRV_IME_OSD_CST_INFO *p_ds_cst;
	KDRV_IME_OSD_PLT_INFO *p_ds_plt;
	KDRV_IME_OSD_INFO *p_stamp;
	CTL_IPP_IME_CTRL *p_ime_ctrl;

	rt = E_OK;
	p_ime_ctrl = &p_base->ime_ctrl;

	memset((void *)&dscb_out_info, 0, sizeof(CTL_IPP_DS_CB_OUTPUT_INFO));
	memset((void *)&dscb_in_info, 0, sizeof(CTL_IPP_DS_CB_INPUT_INFO));
	dscb_in_info.ctl_ipp_handle = 0;
	dscb_in_info.img_size = p_ime_ctrl->in_size;
	if (p_ime_ctrl->pat_paste_enable) {
		// use pattern paste info instead of callback
		UINT32 x, y;

		x = CTL_IPP_PATTERN_PASTE_OFS(p_base->ime_ctrl.pat_paste_win.x, p_base->ime_ctrl.pat_paste_info.img_info.size.w, p_base->ime_ctrl.pat_paste_win.w);
		y = CTL_IPP_PATTERN_PASTE_OFS(p_base->ime_ctrl.pat_paste_win.y, p_base->ime_ctrl.pat_paste_info.img_info.size.h, p_base->ime_ctrl.pat_paste_win.h);

		dscb_out_info.cst_info = p_base->ime_ctrl.pat_paste_cst_info;
		dscb_out_info.plt_info = p_base->ime_ctrl.pat_paste_plt_info;
		dscb_out_info.stamp[0] = p_base->ime_ctrl.pat_paste_info;
		dscb_out_info.stamp[0].img_info.pos.x = x;
		dscb_out_info.stamp[0].img_info.pos.y = y;

		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[pattern paste] datastamp x=%u y=%u w=%u h=%u lofs=%u\r\n", dscb_out_info.stamp[0].img_info.pos.x, dscb_out_info.stamp[0].img_info.pos.y,
			dscb_out_info.stamp[0].img_info.size.w, dscb_out_info.stamp[0].img_info.size.h, dscb_out_info.stamp[0].img_info.lofs);
	} else if (p_ime_ctrl->ds_enable) {
		kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_DATASTAMP, (UINT32)p_base, (void *)&dscb_in_info, (void *)&dscb_out_info);
	}

	/*
		directly typecast, check struct size
		CTL_IPP_DS_CST	-> KDRV_IME_OSD_CST_INFO
		CTL_IPP_DS_PLT	-> KDRV_IME_OSD_PLT_INFO
		CTL_IPP_DS_INFO	-> KDRV_IME_OSD_INFO
	*/
	if (sizeof(KDRV_IME_OSD_CST_INFO) != sizeof(CTL_IPP_DS_CST)) {
		CTL_IPP_DBG_ERR("datastamp struct not match, KDRV_IME_OSD_CST_INFO != CTL_IPP_DS_CST\r\n");
		return rt;
	}

	if (sizeof(KDRV_IME_OSD_PLT_INFO) != sizeof(CTL_IPP_DS_PLT)) {
		CTL_IPP_DBG_ERR("datastamp struct not match, KDRV_IME_OSD_PLT_INFO != CTL_IPP_DS_PLT\r\n");
		return rt;
	}

	if (sizeof(KDRV_IME_OSD_INFO) != sizeof(CTL_IPP_DS_INFO)) {
		CTL_IPP_DBG_ERR("datastamp struct not match, KDRV_IME_OSD_INFO != CTL_IPP_DS_INFO\r\n");
		return rt;
	}

	for (i = 0; i < CTL_IPP_DS_SET_ID_MAX; i++) {
		p_stamp = (KDRV_IME_OSD_INFO *)&dscb_out_info.stamp[i];

		p_stamp->ds_set_idx = i;
		if (p_stamp->enable) {
			if (!p_stamp->image.img_addr) {
				CTL_IPP_DBG_ERR("datastamp addr is 0\r\n");
				return CTL_IPP_E_PAR;
			}
			p_stamp->image.img_fmt = kdf_ipp_typecast_ime_dsfmt(dscb_out_info.stamp[i].img_info.fmt);
			if (p_stamp->image.img_fmt == KDF_IPP_TYPECAST_FAILED) {
				p_stamp->enable = DISABLE;
			}
			if (p_stamp->ds_iq.color_key_en) {
				p_stamp->ds_iq.color_key_mode = kdf_ipp_typecast_ime_ds_ckey_mode(dscb_out_info.stamp[i].iq_info.color_key_mode);
				if (p_stamp->ds_iq.color_key_mode == KDF_IPP_TYPECAST_FAILED) {
					p_stamp->enable = DISABLE;
				}
				color_key = dscb_out_info.stamp[i].iq_info.color_key_val;
				p_stamp->ds_iq.color_key_val[0] = (color_key >> 24) & 0xff;
				p_stamp->ds_iq.color_key_val[1] = (color_key >> 16) & 0xff;
				p_stamp->ds_iq.color_key_val[2] = (color_key >> 8) & 0xff;
				p_stamp->ds_iq.color_key_val[3] = color_key & 0xff;
			}
		}
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OSD, (void *)p_stamp);
	}

	p_ds_cst = (KDRV_IME_OSD_CST_INFO *)&dscb_out_info.cst_info;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OSD_CST, (void *)p_ds_cst);

	p_ds_plt = (KDRV_IME_OSD_PLT_INFO *)&dscb_out_info.plt_info.mode;
	p_ds_plt->plt_mode = kdf_ipp_typecast_ime_ds_plt_mode(dscb_out_info.plt_info.mode);
	if (p_ds_plt->plt_mode == KDF_IPP_TYPECAST_FAILED) {
		// do not set to kdriver
	} else {
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OSD_PLT, (void *)p_ds_plt);
	}

	return rt;
}

INT32 kdf_ipp_config_ime_privacy_mask(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	static CTL_IPP_PM_CB_OUTPUT_INFO pmcb_out_info;
	static CTL_IPP_PM_CB_INPUT_INFO pmcb_in_info;
	INT32 rt;
	UINT32 i;
	KDRV_IME_PM_PXL_IMG_INFO pm_pxlimg;
	CTL_IPP_IME_CTRL *p_ime_ctrl;

	rt = E_OK;
	p_ime_ctrl = &p_base->ime_ctrl;

	memset((void *)&pmcb_out_info, 0, sizeof(CTL_IPP_PM_CB_OUTPUT_INFO));
	memset((void *)&pmcb_in_info, 0, sizeof(CTL_IPP_PM_CB_INPUT_INFO));
	pmcb_in_info.ctl_ipp_handle = 0;
	pmcb_in_info.img_size = p_ime_ctrl->in_size;
	if (p_ime_ctrl->pat_paste_enable) {
		// use pattern paste info instead of callback
		UINT32 w, h;

		w = CTL_IPP_PATTERN_PASTE_SIZE(p_base->ime_ctrl.pat_paste_info.img_info.size.w, p_base->ime_ctrl.pat_paste_win.w);
		h = CTL_IPP_PATTERN_PASTE_SIZE(p_base->ime_ctrl.pat_paste_info.img_info.size.h, p_base->ime_ctrl.pat_paste_win.h);

		pmcb_out_info.pxl_blk_size = CTL_IPP_PM_PXL_BLK_08;
		pmcb_out_info.mask[0].func_en = 1;
		pmcb_out_info.mask[0].hollow_mask_en = 0;
		pmcb_out_info.mask[0].pm_coord[0].x = 0;
		pmcb_out_info.mask[0].pm_coord[0].y = 0;
		pmcb_out_info.mask[0].pm_coord[1].x = (INT32)w;
		pmcb_out_info.mask[0].pm_coord[1].y = 0;
		pmcb_out_info.mask[0].pm_coord[2].x = (INT32)w;
		pmcb_out_info.mask[0].pm_coord[2].y = (INT32)h;
		pmcb_out_info.mask[0].pm_coord[3].x = 0;
		pmcb_out_info.mask[0].pm_coord[3].y = (INT32)h;
		pmcb_out_info.mask[0].msk_type = CTL_IPP_PM_MASK_TYPE_YUV;
		for (i = 0; i < 3; i++) {
			pmcb_out_info.mask[0].color[i] = p_base->ime_ctrl.pat_paste_bgn_color[i];
		}
		pmcb_out_info.mask[0].alpha_weight = 255;

		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[pattern paste] pm_coord (%d, %d) (%d, %d) (%d, %d) (%d, %d)\r\n",
			pmcb_out_info.mask[0].pm_coord[0].x,
			pmcb_out_info.mask[0].pm_coord[0].y,
			pmcb_out_info.mask[0].pm_coord[1].x,
			pmcb_out_info.mask[0].pm_coord[1].y,
			pmcb_out_info.mask[0].pm_coord[2].x,
			pmcb_out_info.mask[0].pm_coord[2].y,
			pmcb_out_info.mask[0].pm_coord[3].x,
			pmcb_out_info.mask[0].pm_coord[3].y);
	} else if (p_ime_ctrl->pm_enable) {
		kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PRIMASK, (UINT32)p_base, (void *)&pmcb_in_info, (void *)&pmcb_out_info);
	}

	// when ctl_ipp_update_reference() set pm_in_lofs to 0 means current frame count is 0 (first frame) and these info is dummy
	// dummy info may get garbage mosaic result (first ipp open) or last ipp open mosaic result (without hdal uninit)
	if (p_ime_ctrl->pm_in_lofs == 0) {
		// when pm info. is dummy, use current frame's blk_size to configure the following parameter. otherwise, these info. are from previous frame (see ctl_ipp_update_reference())
		p_ime_ctrl->pm_pxl_blk = pmcb_out_info.pxl_blk_size;
		p_ime_ctrl->pm_in_size = p_ime_ctrl->in_size;
		ctl_ipp_ise_cal_pm_size(p_ime_ctrl->pm_pxl_blk, &p_ime_ctrl->pm_in_size.w, &p_ime_ctrl->pm_in_size.h);
		p_ime_ctrl->pm_in_lofs = ALIGN_CEIL(p_ime_ctrl->pm_in_size.w * 3, 4);
	}

	pm_pxlimg.blk_size = kdf_ipp_typecast_ime_pm_blk(p_ime_ctrl->pm_pxl_blk);
	pm_pxlimg.dma_addr = p_ime_ctrl->pm_in_addr;
	pm_pxlimg.lofs = p_ime_ctrl->pm_in_lofs;
	pm_pxlimg.img_size = p_ime_ctrl->pm_in_size;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_PM_PXL_IMG, (void *)&pm_pxlimg);

	/* directly tpyecast CTL_IPP_PM to KDRV_IME_PM_INFO
		check struct size first
	*/
	if (sizeof(KDRV_IME_PM_INFO) != sizeof(CTL_IPP_PM)) {
		CTL_IPP_DBG_ERR("privacy mask struct not match, KDRV_IME_PM_INFO != CTL_IPP_PM\r\n");
		return rt;
	}
	for (i = 0; i < CTL_IPP_PM_SET_ID_MAX; i++) {
		KDRV_IME_PM_INFO *p_mask;

		p_mask = (KDRV_IME_PM_INFO *)&pmcb_out_info.mask[i];
		p_mask->set_idx = i;

		/* only support pm1/3/5/7 for hollow mask */
		if (i % 2 == 1) {
			if (p_mask->hlw_enable) {
				p_mask->enable = DISABLE;
				p_mask->hlw_enable = DISABLE;
				CTL_IPP_DBG_WRN("hollow mask only support PM_ID_1/3/5/7, %d\r\n", (int)((i + 1)));
			}
		}
		if (p_mask->enable) {
			/* check hollow mask limitation */
			if (i % 2 == 1) {
				if (pmcb_out_info.mask[i - 1].func_en && pmcb_out_info.mask[i - 1].hollow_mask_en) {
					CTL_IPP_DBG_WRN("When PM_ID_1/3/5/7/ enable hollow mask, PM_ID_2/4/6/8 can not be used, %d\r\n", (int)((i + 1)));
					p_mask->enable = DISABLE;
				}
			}
			p_mask->msk_type = kdf_ipp_typecast_ime_pm_type(p_mask->msk_type);
			if (p_mask->msk_type == KDF_IPP_TYPECAST_FAILED) {
				p_mask->enable = DISABLE;
			}

			if (p_mask->msk_type == KDRV_IME_PM_MASK_TYPE_PXL) {
				/* check pm input image exist */
				if (pm_pxlimg.dma_addr == 0 || pm_pxlimg.blk_size == KDF_IPP_TYPECAST_FAILED) {
					CTL_IPP_DBG_IND("pixelation mask(%d) must enable yuv subout, 0x%x %d. force pm type to yuv\r\n",
						i, (unsigned int)pm_pxlimg.dma_addr, (int)pm_pxlimg.blk_size);
					p_mask->msk_type = KDRV_IME_PM_MASK_TYPE_YUV; // force use yuv type, dont disable mask function to prevent security issue
				}

				/* not support hollow mask */
				if (p_mask->hlw_enable) {
					CTL_IPP_DBG_WRN("hollow mask only support yuv type, %d\r\n", (int)((i + 1)));
					p_mask->enable = DISABLE;
					p_mask->hlw_enable = DISABLE;
				}
			}
		}
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_PM, (void *)p_mask);
	}
	/* set pm pixel block to baseinfo for ise task */
	p_ime_ctrl->pm_pxl_blk = pmcb_out_info.pxl_blk_size;

	return rt;
}

INT32 kdf_ipp_config_ime_lca(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base, KDRV_IME_SINGLE_OUT_INFO *p_single_out)
{
	INT32 rt;
	CTL_IPP_IME_CTRL *p_ime_ctrl;
	KDRV_IME_LCA_SUBOUT_INFO subout_info;
	KDRV_IME_LCA_CA_CENT_INFO lca_cent;
	KDRV_IME_LCA_CTRL_PARAM  lca_func_en;
	KDRV_IME_LCA_IMG_INFO subin_info;

	rt = E_OK;
	p_ime_ctrl = &p_base->ime_ctrl;

	/* lca subout */
	subout_info.enable = p_ime_ctrl->lca_out_enable;
	subout_info.sub_out_size.width = p_ime_ctrl->lca_out_size.w;
	subout_info.sub_out_size.height = p_ime_ctrl->lca_out_size.h;
	subout_info.sub_out_size.line_ofs = p_ime_ctrl->lca_out_lofs;
	subout_info.sub_out_addr = p_ime_ctrl->lca_out_addr;
	if (subout_info.sub_out_addr == 0 || subout_info.sub_out_size.width == 0 || subout_info.sub_out_size.height == 0) {
		subout_info.enable = DISABLE;
	}
	if (subout_info.enable) {
		p_single_out->sout_chl |= KDRV_IME_SOUT_LCA_SUBOUT;
	}
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_LCA_SUB, (void *)&subout_info);

	lca_cent.cent_u = p_base->ife2_ctrl.gray_avg_u;
	lca_cent.cent_v = p_base->ife2_ctrl.gray_avg_v;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_LCA_CA_CENT, (void *)&lca_cent);

	/* lca subin setting and error handle
		check input_addr
		check lca in/out size
		check flow
	*/
	if ((kdf_ipp_chk_ife2_ready_to_trig(&p_base->ife2_ctrl) == FALSE) ||
		((p_hdl->flow != KDF_IPP_FLOW_RAW) && (p_hdl->flow != KDF_IPP_FLOW_DIRECT_RAW) && (p_hdl->flow != KDF_IPP_FLOW_CCIR))) {
		lca_func_en.enable = DISABLE;
		lca_func_en.bypass = DISABLE;
	} else {
		if ((p_ime_ctrl->lca_out_size.w != p_ime_ctrl->lca_in_size.w) || (p_ime_ctrl->lca_out_size.h != p_ime_ctrl->lca_in_size.h)) {
			lca_func_en.enable = ENABLE;
			lca_func_en.bypass = ENABLE;

			p_ime_ctrl->lca_in_size = p_ime_ctrl->lca_out_size;
			p_ime_ctrl->lca_in_lofs = p_ime_ctrl->lca_out_lofs;
			p_base->ife2_ctrl.in_size = p_ime_ctrl->lca_out_size;
			p_base->ife2_ctrl.in_lofs =  p_ime_ctrl->lca_in_lofs;
		} else {
			lca_func_en.enable = ENABLE;
			lca_func_en.bypass = DISABLE;
		}
		/* lca subinput */
		subin_info.in_src = KDRV_IME_IN_SRC_DIRECT;
		subin_info.img_size.width = p_ime_ctrl->lca_in_size.w;
		subin_info.img_size.height = p_ime_ctrl->lca_in_size.h;
		subin_info.img_size.line_ofs = p_ime_ctrl->lca_in_lofs;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_LCA_REF_IMG, (void *)&subin_info);
	}
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_LCA_FUNC_EN, (void *)&lca_func_en);
	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "lca en = %d, bypass = %d\r\n", (int)(lca_func_en.enable), (int)(lca_func_en.bypass));

	return rt;
}

INT32 kdf_ipp_config_ime_low_latency(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	CTL_IPP_IME_CTRL *p_ime_ctrl;
	KDRV_IME_LOW_DELAY_INFO low_dly_info;
	KDRV_IME_BREAK_POINT_INFO bp_info;

	rt = E_OK;
	p_ime_ctrl = &p_base->ime_ctrl;

	/* low delay */
	low_dly_info.dly_enable = p_ime_ctrl->low_delay_enable;
	if (low_dly_info.dly_enable == ENABLE) {
		low_dly_info.dly_ch = kdf_ipp_typecast_ime_low_delay_ch(p_ime_ctrl->low_delay_path);
		if (low_dly_info.dly_ch == KDF_IPP_TYPECAST_FAILED) {
			low_dly_info.dly_enable = DISABLE;
			low_dly_info.dly_ch = 0;
		}
	} else {
		low_dly_info.dly_ch = 0;
	}
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_LOW_DELAY, (void *)&low_dly_info);

	/* clamp bp from 1 ~ (h - 50) */
	bp_info.bp3 = p_ime_ctrl->low_delay_bp;
	if (bp_info.bp3 == 0) {
		bp_info.bp3 = 1;
	} else if (bp_info.bp3 > (p_ime_ctrl->in_size.h - 50)) {
		bp_info.bp3 = p_ime_ctrl->in_size.h - 50;
	}
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_BREAK_POINT, (void *)&bp_info);

	return rt;
}

INT32 kdf_ipp_config_ime_3dnr(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base, KDRV_IME_SINGLE_OUT_INFO *p_single_out)
{
	INT32 rt;
	CTL_IPP_IME_CTRL *p_ime_ctrl;
	KDRV_IME_PARAM_IPL_FUNC_EN func_en;
	KDRV_IME_TMNR_REF_IN_IMG_INFO ref_img;
	KDRV_IME_TMNR_MOTION_VECTOR_INFO mv_info;
	KDRV_IME_TMNR_MOTION_STATUS_INFO ms_info;
	KDRV_IME_TMNR_STATISTIC_DATA_INFO sta_info;
	KDRV_IME_TMNR_MOTION_STATUS_ROI_INFO ms_roi_info;
	KDRV_IME_TMNR_BUF_SIZE_INFO buf_info;
	BOOL ref_in_va2pa;

	rt = E_OK;
	p_ime_ctrl = &p_base->ime_ctrl;

	/* 3dnr
		1. check func_en
		2. check ref image ready
		3. check subout buffer not 0
	*/
	if (p_ime_ctrl->tplnr_enable) {
		if (p_ime_ctrl->tplnr_in_ref_addr[0] != 0) {
			memset((void *)&buf_info, 0, sizeof(KDRV_IME_TMNR_BUF_SIZE_INFO));
			buf_info.in_size_h = p_ime_ctrl->in_size.w;
			buf_info.in_size_v = p_ime_ctrl->in_size.h;
			buf_info.in_sta_max_num = p_ime_ctrl->tplnr_sta_info.max_sample_num;
			if (buf_info.in_sta_max_num == 0) {
				/* max sample num = 0, disable sta output, reset max_num for calculation */
				buf_info.in_sta_max_num = CTL_IPP_ISP_IQ_3DNR_STA_MAX_NUM;
				sta_info.sta_out_en = DISABLE;
			}
			kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_TMNR_BUF_SIZE, (void *)&buf_info);

			/* reference in use physical address */
			ref_in_va2pa = FALSE;
			rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_TMNR_REFIN_USE_VA, (void *)&ref_in_va2pa);
			ref_img.img_addr_y = p_ime_ctrl->tplnr_in_ref_pa[0];
			ref_img.img_addr_cb = p_ime_ctrl->tplnr_in_ref_pa[1];
			ref_img.img_line_ofs_y = p_ime_ctrl->tplnr_in_ref_lofs[0];
			ref_img.img_line_ofs_cb = p_ime_ctrl->tplnr_in_ref_lofs[1];
			ref_img.flip_en = p_ime_ctrl->tplnr_in_ref_flip_enable;
			ref_img.decode.enable = p_ime_ctrl->tplnr_in_dec_enable;
			rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_TMNR_REF_IN_IMG, (void *)&ref_img);

			mv_info.mot_vec_out_addr = p_ime_ctrl->tplnr_out_mv_addr;
			mv_info.mot_vec_ofs = buf_info.get_mv_lofs;
			if (mv_info.mot_vec_out_addr != 0) {
				mv_info.mot_vec_in_addr = p_ime_ctrl->tplnr_in_mv_addr;
				if (mv_info.mot_vec_in_addr == 0) {
					mv_info.mot_vec_in_addr = mv_info.mot_vec_out_addr;
				}
				rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_TMNR_MV, (void *)&mv_info);
			}

			ms_info.mot_sta_out_addr = p_ime_ctrl->tplnr_out_ms_addr;
			ms_info.mot_sta_ofs = buf_info.get_ms_lofs;
			if (ms_info.mot_sta_out_addr != 0) {
				ms_info.mot_sta_in_addr = p_ime_ctrl->tplnr_in_ms_addr;
				if (ms_info.mot_sta_in_addr == 0) {
					ms_info.mot_sta_in_addr = ms_info.mot_sta_out_addr;
				}
				rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_TMNR_MS, (void *)&ms_info);
			}

			/* update info to sta_info for isp */
			p_ime_ctrl->tplnr_sta_info.lofs = buf_info.get_sta_lofs;
			p_ime_ctrl->tplnr_sta_info.sample_num.w = buf_info.get_sta_param.sample_num_x;
			p_ime_ctrl->tplnr_sta_info.sample_num.h = buf_info.get_sta_param.sample_num_y;
			p_ime_ctrl->tplnr_sta_info.sample_st.x = buf_info.get_sta_param.sample_st_x;
			p_ime_ctrl->tplnr_sta_info.sample_st.y = buf_info.get_sta_param.sample_st_y;
			p_ime_ctrl->tplnr_sta_info.sample_step.w = buf_info.get_sta_param.sample_step_hori;
			p_ime_ctrl->tplnr_sta_info.sample_step.h = buf_info.get_sta_param.sample_step_vert;
			sta_info.sta_out_en = p_ime_ctrl->tplnr_sta_info.enable;
			if (sta_info.sta_out_en) {
				sta_info.sta_out_addr = p_ime_ctrl->tplnr_sta_info.buf_addr;
				sta_info.sta_out_ofs = buf_info.get_sta_lofs;
				sta_info.sta_param = buf_info.get_sta_param;

				if (sta_info.sta_out_addr == 0 || sta_info.sta_out_ofs == 0) {
					sta_info.sta_out_en = DISABLE;
					p_ime_ctrl->tplnr_sta_info.enable = DISABLE;
				}
			}
			rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_TMNR_STA, (void *)&sta_info);

			ms_roi_info.mot_sta_roi_out_en = p_ime_ctrl->tplnr_out_ms_roi_enable;
			if (ms_roi_info.mot_sta_roi_out_en) {
				ms_roi_info.mot_sta_roi_out_addr = p_ime_ctrl->tplnr_out_ms_roi_addr;
				ms_roi_info.mot_sta_roi_out_ofs = buf_info.get_ms_roi_lofs;
				ms_roi_info.mot_sta_roi_out_flip_en = DISABLE;

				if (ms_roi_info.mot_sta_roi_out_addr == 0 || ms_roi_info.mot_sta_roi_out_ofs == 0) {
					ms_roi_info.mot_sta_roi_out_en = DISABLE;
				}
			}
			rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_TMNR_MS_ROI, (void *)&ms_roi_info);

			p_single_out->sout_chl |= (KDRV_IME_SOUT_TMNR_MV | KDRV_IME_SOUT_TMNR_MS);
			if (sta_info.sta_out_en) {
				p_single_out->sout_chl |= KDRV_IME_SOUT_TMNR_STA;
			}
			if (ms_roi_info.mot_sta_roi_out_en) {
				p_single_out->sout_chl |= KDRV_IME_SOUT_TMNR_MS_ROI;
			}
		}
	}

	/* tplnr error handle
		check ctl enable
		check reference addr
		check subout addr
	*/
	if ((p_ime_ctrl->tplnr_enable == DISABLE) ||
		(p_ime_ctrl->tplnr_in_ref_addr[0] == 0) ||
		(p_ime_ctrl->tplnr_out_mv_addr == 0) ||
		(p_ime_ctrl->tplnr_out_ms_addr == 0)) {
		func_en.ipl_ctrl_func = KDRV_IME_IQ_FUNC_TMNR;
		func_en.enable = DISABLE;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
		p_single_out->sout_chl &= ~(KDRV_IME_SOUT_TMNR_MV | KDRV_IME_SOUT_TMNR_MS);

		/* set sta_info to disable */
		p_ime_ctrl->tplnr_sta_info.enable = DISABLE;
	}

	return rt;
}

INT32 kdf_ipp_config_ime_base(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	CTL_IPP_IME_CTRL *p_ime_ctrl;
	KDRV_IME_IN_INFO in_img_info;
	KDRV_IME_DMA_ADDR_INFO in_addr;
	KDRV_IME_SCL_METHOD_SEL_INFO out_scl_method_sel;
	KDRV_IME_PARAM_IPL_FUNC_EN func_en;
	KDRV_IME_INTE inte_en;

	rt = E_OK;
	p_ime_ctrl = &p_base->ime_ctrl;

	/* set interrupt enable */
	inte_en = KDF_IPP_DFT_IME_INTE;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_INTER, (void *)&inte_en);

	/* default enable all function */
	func_en.ipl_ctrl_func = KDF_IPP_DFT_IME_FUNCEN;
	func_en.enable = ENABLE;
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);

	/* set input img parameters */
	if (p_hdl->flow == KDF_IPP_FLOW_IME_D2D || p_ime_ctrl->pat_paste_enable) {
		in_img_info.in_src = KDRV_IME_IN_SRC_DRAM;
		in_img_info.type = kdf_ipp_typecast_ime_in_fmt(p_ime_ctrl->in_fmt);
		if (in_img_info.type == KDF_IPP_TYPECAST_FAILED) {
			return CTL_IPP_E_INDATA;
		}
		in_img_info.ch[0].width = p_ime_ctrl->in_size.w;
		in_img_info.ch[0].height = p_ime_ctrl->in_size.h;
		in_img_info.ch[0].line_ofs = p_ime_ctrl->in_lofs[0];
		in_img_info.ch[1].line_ofs = p_ime_ctrl->in_lofs[1];
		in_img_info.dma_flush = KDRV_IME_NOTDO_BUF_FLUSH;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_IN_IMG, (void *)&in_img_info);

		in_addr.addr_y = p_ime_ctrl->in_addr[0];
		in_addr.addr_cb = p_ime_ctrl->in_addr[1];
		in_addr.addr_cr = p_ime_ctrl->in_addr[2];
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_IN_ADDR, (void *)&in_addr);
	} else if (p_hdl->flow == KDF_IPP_FLOW_RAW || p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW || p_hdl->flow == KDF_IPP_FLOW_CCIR) {
		in_img_info.in_src = (p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) ? KDRV_IME_IN_SRC_ALL_DIRECT : KDRV_IME_IN_SRC_DIRECT;
		in_img_info.type = kdf_ipp_typecast_ime_in_fmt(VDO_PXLFMT_YUV420);
		in_img_info.ch[0].width = p_ime_ctrl->in_size.w;
		in_img_info.ch[0].height = p_ime_ctrl->in_size.h;
		in_img_info.ch[0].line_ofs = p_ime_ctrl->in_lofs[0];	/* dont care in rhe2ime mode */
		in_img_info.ch[1].line_ofs = p_ime_ctrl->in_lofs[1];	/* dont care in rhe2ime mode */
		in_img_info.dma_flush = KDRV_IME_NOTDO_BUF_FLUSH;		/* dont care in rhe2ime mode */
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_IN_IMG, (void *)&in_img_info);
	} else {
		CTL_IPP_DBG_WRN("Unsupport flow for ime %d\r\n", (int)(p_hdl->flow));
		return CTL_IPP_E_PAR;
	}

	/* scale method select */
	out_scl_method_sel.scl_th = p_ime_ctrl->out_scl_method_sel.scl_th;
	out_scl_method_sel.method_l = kdf_ipp_typecast_ime_scaler(p_ime_ctrl->out_scl_method_sel.method_l);
	out_scl_method_sel.method_h = kdf_ipp_typecast_ime_scaler(p_ime_ctrl->out_scl_method_sel.method_h);
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_SCL_METHOD_SEL, (void *)&out_scl_method_sel);

	return rt;
}

static INT32 kdf_ipp_config_ime_common(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	CTL_IPP_IME_CTRL *p_ime_ctrl;
	KDRV_IME_PARAM_IPL_FUNC_EN func_en;
	KDRV_IME_SINGLE_OUT_INFO single_out_info;
	UINT32 i;

	rt = E_OK;
	p_ime_ctrl = &p_base->ime_ctrl;

	/* single out init to 0 */
	single_out_info.sout_enable = ENABLE;
	single_out_info.sout_chl = 0;

	rt |= kdf_ipp_config_ime_base(p_hdl, p_base);

	/* set output path parameters */
	for (i = CTL_IPP_OUT_PATH_ID_1; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		/* special case: path6 use dce to output */
		if (i == CTL_IPP_OUT_PATH_ID_6) {
			continue;
		}

		rt |= kdf_ipp_config_ime_path(p_hdl, p_ime_ctrl, i);

		if (p_ime_ctrl->out_img[i].enable == ENABLE  && p_ime_ctrl->out_img[i].dma_enable == ENABLE) {
			if (i == CTL_IPP_OUT_PATH_ID_5) {
				single_out_info.sout_chl |= KDRV_IME_SOUT_TMNR_REFOUT;
			} else {
				single_out_info.sout_chl |= (KDRV_IME_SOUT_PATH1 << i);
			}
		}
	}

	/* set lca */
	rt |= kdf_ipp_config_ime_lca(p_hdl, p_base, &single_out_info);

	/* low delay */
	rt |= kdf_ipp_config_ime_low_latency(p_hdl, p_base);

	/* 3dnr	*/
	rt |= kdf_ipp_config_ime_3dnr(p_hdl, p_base, &single_out_info);

	/* data stamp */
	rt |= kdf_ipp_config_ime_datastamp(p_hdl, p_base);

	/* privacy mask */
	rt |= kdf_ipp_config_ime_privacy_mask(p_hdl, p_base);

	/* set single out at last */
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_SINGLE_OUTPUT, (void *)&single_out_info);

	/* isp debug mode, disable all iq func */
	if (ctl_ipp_dbg_get_isp_mode()) {
		func_en.ipl_ctrl_func = KDF_IPP_DFT_IME_FUNCEN;
		func_en.enable = DISABLE;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
	}

	if (rt != E_OK) {
		return CTL_IPP_E_KDRV_SET;
	}
	return CTL_IPP_E_OK;
}

static INT32 kdf_ipp_config_ife2_common(KDF_IPP_HANDLE *p_hdl, CTL_IPP_BASEINFO *p_base)
{
	INT32 rt;
	CTL_IPP_IFE2_CTRL *p_ife2_ctrl;

	rt = E_OK;
	p_ife2_ctrl = &p_base->ife2_ctrl;

	if (kdf_ipp_chk_ife2_ready_to_trig(p_ife2_ctrl) == TRUE) {
		KDRV_IFE2_IN_INFO in_img_info;
		KDRV_IFE2_OUT_IMG out_img_info;
		KDRV_IFE2_INTE inte_en;
		KDRV_IFE2_PARAM_IPL_FUNC_EN func_en;
		UINT32 in_addr;

		/* set default func en */
		func_en.ipl_ctrl_func = KDF_IPP_DFT_IFE2_FUNCEN;
		func_en.enable = ENABLE;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_hdl, KDRV_IFE2_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);

		/* set inte */
		inte_en = KDF_IPP_DFT_IFE2_INTE;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_hdl, KDRV_IFE2_PARAM_IPL_INTER, (void *)&inte_en);

		/* set input img parameters */
		in_img_info.type = KDRV_IFE2_IN_FMT_PACK_YUV444;
		in_img_info.ch.width = p_ife2_ctrl->in_size.w;
		in_img_info.ch.height = p_ife2_ctrl->in_size.h;
		in_img_info.ch.line_ofs = p_ife2_ctrl->in_lofs;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_hdl, KDRV_IFE2_PARAM_IPL_IN_IMG_INFO, (void *)&in_img_info);

		/* set input address */
		in_addr = p_ife2_ctrl->in_addr;
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_hdl, KDRV_IFE2_PARAM_IPL_IN_ADDR, (void *)&in_addr);

		/* set output */
		out_img_info.out_dst = (p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) ? KDRV_IFE2_OUT_DST_ALL_DIRECT : KDRV_IFE2_OUT_DST_DIRECT;

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_hdl, KDRV_IFE2_PARAM_IPL_OUT_IMG_INFO, (void *)&out_img_info);
	}

	if (rt != E_OK) {
		return CTL_IPP_E_KDRV_SET;
	}
	return CTL_IPP_E_OK;
}

INT32 kdf_ipp_process_raw(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev)
{
	INT32 rt;
	CTL_IPP_BASEINFO *p_base;
	KDF_IPP_ISP_CB_INFO iqcb_info;
	CTL_IPP_ISP_IME_LCA_SIZE_RATIO lca_size_ratio;

	p_base = (CTL_IPP_BASEINFO *)header_addr;

	/* GET IME LCA SUBOUT SIZE */
	{
		memset((void *)&lca_size_ratio, 0, sizeof(CTL_IPP_ISP_IME_LCA_SIZE_RATIO));
		iqcb_info.type = KDF_IPP_ISP_CB_TYPE_IQ_GET;
		iqcb_info.item = CTL_IPP_ISP_ITEM_IME_LCA_SIZE;
		iqcb_info.data_addr = (UINT32)&lca_size_ratio;
		kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&iqcb_info, NULL);

		p_base->ime_ctrl.lca_out_size = ctl_ipp_util_lca_subout_size(p_base->ime_ctrl.in_size.w, p_base->ime_ctrl.in_size.h, lca_size_ratio, 0xFFFFFFFF);
		p_base->ime_ctrl.lca_out_lofs = p_base->ime_ctrl.lca_out_size.w * 3;

		if (p_base->ime_ctrl.lca_out_enable) {
			if (p_base->ime_ctrl.lca_out_size.w < CTL_IPP_LCA_H_MIN || p_base->ime_ctrl.lca_out_size.h < CTL_IPP_LCA_V_MIN) {
				CTL_IPP_DBG_WRN("lca out must > (%d,%d) (%d, %d)\r\n", CTL_IPP_LCA_H_MIN, CTL_IPP_LCA_V_MIN, p_base->ime_ctrl.lca_out_size.w, p_base->ime_ctrl.lca_out_size.h);
				rt = CTL_IPP_E_PAR;
				return rt;
			}
		}
	}

	/* GET OUTPUT BUFFER */
	{
		UINT32 i;
		CTL_IPP_IME_OUT_IMG *p_ime_path;
		CTL_IPP_IME_CTRL *p_ime_ctrl = &p_base->ime_ctrl;

		if (kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PRESET, 0, (void *)p_base, NULL) != CTL_IPP_E_OK) {
			/* DISABLE Output when get buffer failed */
			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				p_ime_path = &p_ime_ctrl->out_img[i];
				p_ime_path->dma_enable = FALSE;
			}

			/* skip frame if no reference output buffer*/
			if (p_ime_ctrl->tplnr_enable) {
				if(p_ime_ctrl->out_img[p_ime_ctrl->tplnr_in_ref_path].addr[0] == 0)
					return CTL_IPP_E_NOMEM;
			}
		}
	}

	kdf_ipp_config_start(p_hdl, p_base);

	/* IFE SETTING */
	{
		rt = kdf_ipp_config_ife_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ife set error rt %d\r\n", (int)(rt));
		}
	}

	/* DCE SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_dce_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("dce set error rt %d\r\n", (int)(rt));
			return rt;
		}
	}

	/* IPE SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_ipe_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ipe set error rt %d\r\n", (int)(rt));
		}
	}

	/* IME SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_ime_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ime set error rt %d\r\n", (int)(rt));
		}
	}

	/* IFE2 SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_ife2_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ife2 set error rt %d\r\n", (int)(rt));
		}
	}

	/* STRIPE CALCULATION */
	if (rt == CTL_IPP_E_OK) {
		KDRV_DCE_TRIG_TYPE_INFO dce_trig_cfg = {0};
		KDRV_DCE_STRIPE_INFO dce_strp = {0};
		KDRV_DCE_EXTEND_INFO dce_ext_info = {0};
		KDRV_IME_EXTEND_INFO ime_ext_info = {0};
		KDRV_IPE_EXTEND_INFO ipe_ext_info = {0};
		UINT32 i;
		INT32 kdrv_rt;

		kdrv_rt = E_OK;
		if (VDO_PXLFMT_CLASS(p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1].fmt) == VDO_PXLFMT_CLASS_NVX ||
			VDO_PXLFMT_CLASS(p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_5].fmt) == VDO_PXLFMT_CLASS_NVX) {
			dce_ext_info.ime_enc_enable = ENABLE;
		} else {
			dce_ext_info.ime_enc_enable = DISABLE;
		}
		dce_ext_info.ime_3dnr_enable = p_base->ime_ctrl.tplnr_enable;
		dce_ext_info.ime_dec_enable = DISABLE;
		dce_ext_info.low_latency_enable = p_base->ime_ctrl.low_delay_enable;
		kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_EXTEND, (void *)&dce_ext_info);

		/*	trigger dce for stripe calculation
			cal_stripe always wait end and not support timeout due to driver mechanism
			only when stripe_type = KDRV_DCE_STRIPE_MANUAL_MULTI will cause DCE D2D Calculation
		*/
		dce_trig_cfg.opmode = KDRV_DCE_OP_MODE_CAL_STRIP;
		kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_DCE, p_hdl, NULL, (void *)&dce_trig_cfg);

		/* get/set stripe info for ipe/ime */
		kdrv_rt |= kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_STRIPE_INFO, (void *)&dce_strp);

		/* set stripe info for debug */
		ctl_ipp_dbg_hdl_stripe_set((UINT32)p_hdl, &dce_strp);

		ipe_ext_info.hstrp_num = dce_strp.hstp_num;
		ipe_ext_info.ime_3dnr_en = dce_ext_info.ime_3dnr_enable;
		ipe_ext_info.ime_enc_en = dce_ext_info.ime_enc_enable;
		kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_EXTEND_INFO, (void *)&ipe_ext_info);

		/* ime stripe info is used for scale method select, use ime_in_hstp
			stripe info keep in baseinfo is used for codec, use ime_out_hstp
		*/
		ime_ext_info.stripe_h_max = dce_strp.ime_in_hstp[0];
		ime_ext_info.stripe_num = dce_strp.hstp_num;
		ime_ext_info.tmnr_en = dce_ext_info.ime_3dnr_enable;
		ime_ext_info.p1_enc_en = dce_ext_info.ime_enc_enable;

		memset((void *)p_base->dce_ctrl.strp_h_arr, 0, (sizeof(UINT32) * CTL_IPP_DCE_STRP_NUM_MAX));
		for (i = 0; i < dce_strp.hstp_num; i++) {
			if (dce_strp.ime_in_hstp[i] > ime_ext_info.stripe_h_max) {
				ime_ext_info.stripe_h_max = dce_strp.ime_in_hstp[i];
			}

			/* keep stripe size in baseinfo */
			if (i < CTL_IPP_DCE_STRP_NUM_MAX) {
				p_base->dce_ctrl.strp_h_arr[i] = dce_strp.ime_out_hstp[i];
			}
		}
		kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_EXTEND, (void *)&ime_ext_info);

		#if 0
		/* todo: 680 yuv compress patch for multi-stripe
			temp clean all y buffer when multi-stripe + yuv-compress
		*/
		if (dce_strp.hstp_num > 1 && dce_ext_info.ime_enc_enable) {
			CTL_IPP_IME_OUT_IMG *ime_path1;

			ime_path1 = &p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1];
			if (ime_path1->dma_enable && (ime_path1->addr[0] != 0)) {
				UINT32 y_buf_size;

				y_buf_size = ime_path1->addr[1] - ime_path1->addr[0];
				memset((void *)ime_path1->addr[0], 0, y_buf_size);
			}
		}
		#endif

		/* check ime yuv compress limitation when multi stripe */
		if (dce_strp.hstp_num > 1 && dce_ext_info.ime_enc_enable) {
			CTL_IPP_IME_OUT_IMG *p_ime_path;

			p_ime_path = &p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1];
			if (p_ime_path->fmt == VDO_PXLFMT_YUV420_NVX2) {
				if (p_base->ime_ctrl.in_size.w != p_ime_path->size.w ||
					p_base->ime_ctrl.in_size.h != p_ime_path->size.h ||
					p_ime_path->crp_window.x != 0 ||
					p_ime_path->crp_window.y != 0 ||
					p_ime_path->size.w != p_ime_path->crp_window.w ||
					p_ime_path->size.h != p_ime_path->crp_window.h) {
					KDRV_IME_OUT_PATH_IMG_INFO out_img_info;

					out_img_info.path_id = CTL_IPP_OUT_PATH_ID_1;
					kdrv_rt |= kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&out_img_info);

					if (out_img_info.path_en) {
						out_img_info.path_en = DISABLE;
						kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
						CTL_IPP_DBG_ERR_FREQ(120,"yuv compress not support scale/crop when mstp\r\n");
					}
				}
			}
		}

		/* check 3dnr one buffer mode
			mstrp can not support 3dnr reference r/w same buffer
		*/
		if (dce_strp.hstp_num > 1 && dce_ext_info.ime_3dnr_enable) {
			CTL_IPP_IME_OUT_IMG *p_ime_path;

			if (p_base->ime_ctrl.tplnr_in_ref_path < CTL_IPP_OUT_PATH_ID_MAX) {
				p_ime_path = &p_base->ime_ctrl.out_img[p_base->ime_ctrl.tplnr_in_ref_path];
				if (p_ime_path->addr[0] == p_base->ime_ctrl.tplnr_in_ref_addr[0]) {
					KDRV_IME_OUT_PATH_IMG_INFO out_img_info;

					out_img_info.path_id = p_base->ime_ctrl.tplnr_in_ref_path;
					kdrv_rt |= kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
					if (out_img_info.path_en) {
						out_img_info.path_en = DISABLE;
						kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
						CTL_IPP_DBG_ERR_FREQ(120,"3dnr reference image(%d) in/out same address when mstp\r\n", p_base->ime_ctrl.tplnr_in_ref_path);
					}
				}
			}
		}

		/* disable 2dlut, mstrp not support 2dlut */
		if (dce_strp.hstp_num > 1) {
			KDRV_DCE_PARAM_IPL_FUNC_EN func_en;

			func_en.enable = DISABLE;
			func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_2DLUT;
			kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
		}

		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("dce stripe calculation error rt %d\r\n", (int)(kdrv_rt));
			rt = CTL_IPP_E_KDRV_STRP;
		}
	}
	kdf_ipp_config_end(p_hdl, p_base);
	kdf_ipp_trig_start(p_hdl, p_base);

	/* TRIGGER */
	if (rt == CTL_IPP_E_OK) {
		KDRV_DCE_TRIG_TYPE_INFO dce_trig_cfg;
		INT32 kdrv_rt;

		dce_trig_cfg.opmode = KDRV_DCE_OP_MODE_NORMAL;

		p_hdl->data_addr_in_process[0] = header_addr;
		p_hdl->frame_sta_mask = KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
		p_hdl->proc_end_mask = KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
		p_hdl->engine_used_mask = KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;

		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IFE);
		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_DCE);
		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IPE);
		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IME);
		if (kdf_ipp_chk_ife2_ready_to_trig(&p_base->ife2_ctrl) == TRUE) {
			kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IFE2);
		}

		kdrv_rt = E_OK;
		if (kdf_ipp_chk_ife2_ready_to_trig(&p_base->ife2_ctrl) == TRUE) {
			p_hdl->proc_end_mask |= KDF_IPP_ENG_BIT_IFE2;
			p_hdl->engine_used_mask |= KDF_IPP_ENG_BIT_IFE2;
			kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IFE2, p_hdl, NULL, NULL);
		}
		kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IFE, p_hdl, NULL, NULL);
		kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IPE, p_hdl, NULL, NULL);
		kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IME, p_hdl, NULL, NULL);
		kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_DCE, p_hdl, NULL, (void *)&dce_trig_cfg);

		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("ipp trigger error rt %d\r\n", (int)(kdrv_rt));
			rt = CTL_IPP_E_KDRV_TRIG;
		}
	}

	kdf_ipp_trig_end(p_hdl, p_base);

	return rt;
}

INT32 kdf_ipp_process_ccir(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev)
{
	INT32 rt;
	CTL_IPP_BASEINFO *p_base;
	KDF_IPP_ISP_CB_INFO iqcb_info;

	p_base = (CTL_IPP_BASEINFO *)header_addr;

	/* GET IME LCA SUBOUT SIZE */
	{
		CTL_IPP_ISP_IME_LCA_SIZE_RATIO lca_size_ratio;

		memset((void *)&lca_size_ratio, 0, sizeof(CTL_IPP_ISP_IME_LCA_SIZE_RATIO));
		iqcb_info.type = KDF_IPP_ISP_CB_TYPE_IQ_GET;
		iqcb_info.item = CTL_IPP_ISP_ITEM_IME_LCA_SIZE;
		iqcb_info.data_addr = (UINT32)&lca_size_ratio;
		kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&iqcb_info, NULL);

		p_base->ime_ctrl.lca_out_size = ctl_ipp_util_lca_subout_size(p_base->ime_ctrl.in_size.w, p_base->ime_ctrl.in_size.h, lca_size_ratio, 0xFFFFFFFF);
		p_base->ime_ctrl.lca_out_lofs = p_base->ime_ctrl.lca_out_size.w * 3;

		if (p_base->ime_ctrl.lca_out_enable) {
			if (p_base->ime_ctrl.lca_out_size.w < CTL_IPP_LCA_H_MIN || p_base->ime_ctrl.lca_out_size.h < CTL_IPP_LCA_V_MIN) {
				CTL_IPP_DBG_WRN("lca out must > (%d,%d) (%d, %d)\r\n", CTL_IPP_LCA_H_MIN, CTL_IPP_LCA_V_MIN, p_base->ime_ctrl.lca_out_size.w, p_base->ime_ctrl.lca_out_size.h);
				rt = CTL_IPP_E_PAR;
				return rt;
			}
		}
	}

	/* GET OUTPUT BUFFER */
	{
		UINT32 i;
		CTL_IPP_IME_OUT_IMG *p_ime_path;
		CTL_IPP_IME_CTRL *p_ime_ctrl = &p_base->ime_ctrl;

		if (kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PRESET, 0, (void *)p_base, NULL) != CTL_IPP_E_OK) {
			/* DISABLE Output when get buffer failed */
			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				p_ime_path = &p_ime_ctrl->out_img[i];
				p_ime_path->dma_enable = FALSE;
			}
		}
	}

	kdf_ipp_config_start(p_hdl, p_base);

	/* DCE SETTING */
	{
		rt = kdf_ipp_config_dce_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("dce set error rt %d\r\n", (int)(rt));
			return rt;
		}
	}

	/* IPE SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_ipe_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ipe set error rt %d\r\n", (int)(rt));
		}
	}

	/* IME SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_ime_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ime set error rt %d\r\n", (int)(rt));
		}
	}

	/* IFE2 SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_ife2_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ife2 set error rt %d\r\n", (int)(rt));
		}
	}

	/* STRIPE CALCULATION */
	if (rt == CTL_IPP_E_OK) {
		KDRV_DCE_TRIG_TYPE_INFO dce_trig_cfg = {0};
		KDRV_DCE_STRIPE_INFO dce_strp = {0};
		KDRV_DCE_EXTEND_INFO dce_ext_info = {0};
		KDRV_IME_EXTEND_INFO ime_ext_info = {0};
		KDRV_IPE_EXTEND_INFO ipe_ext_info = {0};
		UINT32 i;
		INT32 kdrv_rt;

		kdrv_rt = E_OK;
		if (VDO_PXLFMT_CLASS(p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1].fmt) == VDO_PXLFMT_CLASS_NVX ||
			VDO_PXLFMT_CLASS(p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_5].fmt) == VDO_PXLFMT_CLASS_NVX) {
			dce_ext_info.ime_enc_enable = ENABLE;
		} else {
			dce_ext_info.ime_enc_enable = DISABLE;
		}
		dce_ext_info.ime_3dnr_enable = p_base->ime_ctrl.tplnr_enable;
		dce_ext_info.ime_dec_enable = DISABLE;
		dce_ext_info.low_latency_enable = p_base->ime_ctrl.low_delay_enable;
		kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_EXTEND, (void *)&dce_ext_info);

		/*	trigger dce for stripe calculation
			cal_stripe always wait end and not support timeout due to driver mechanism
			only when stripe_type = KDRV_DCE_STRIPE_MANUAL_MULTI will cause DCE D2D Calculation
		*/
		dce_trig_cfg.opmode = KDRV_DCE_OP_MODE_CAL_STRIP;
		kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_DCE, p_hdl, NULL, (void *)&dce_trig_cfg);

		/* get/set stripe info for ipe/ime */
		kdrv_rt |= kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_STRIPE_INFO, (void *)&dce_strp);

		/* set stripe info for debug */
		ctl_ipp_dbg_hdl_stripe_set((UINT32)p_hdl, &dce_strp);

		ipe_ext_info.hstrp_num = dce_strp.hstp_num;
		ipe_ext_info.ime_3dnr_en = dce_ext_info.ime_3dnr_enable;
		ipe_ext_info.ime_enc_en = dce_ext_info.ime_enc_enable;
		kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_EXTEND_INFO, (void *)&ipe_ext_info);

		/* ime stripe info is used for scale method select, use ime_in_hstp
			stripe info keep in baseinfo is used for codec, use ime_out_hstp
		*/
		ime_ext_info.stripe_h_max = dce_strp.ime_in_hstp[0];
		ime_ext_info.stripe_num = dce_strp.hstp_num;
		ime_ext_info.tmnr_en = dce_ext_info.ime_3dnr_enable;
		ime_ext_info.p1_enc_en = dce_ext_info.ime_enc_enable;

		memset((void *)p_base->dce_ctrl.strp_h_arr, 0, (sizeof(UINT32) * CTL_IPP_DCE_STRP_NUM_MAX));
		for (i = 0; i < dce_strp.hstp_num; i++) {
			if (dce_strp.ime_in_hstp[i] > ime_ext_info.stripe_h_max) {
				ime_ext_info.stripe_h_max = dce_strp.ime_in_hstp[i];
			}

			/* keep stripe size in baseinfo */
			if (i < CTL_IPP_DCE_STRP_NUM_MAX) {
				p_base->dce_ctrl.strp_h_arr[i] = dce_strp.ime_out_hstp[i];
			}
		}
		kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_EXTEND, (void *)&ime_ext_info);

		/* check ime yuv compress limitation when multi stripe */
		if (dce_strp.hstp_num > 1 && dce_ext_info.ime_enc_enable) {
			CTL_IPP_IME_OUT_IMG *p_ime_path;

			p_ime_path = &p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1];
			if (p_ime_path->fmt == VDO_PXLFMT_YUV420_NVX2) {
				if (p_base->ime_ctrl.in_size.w != p_ime_path->size.w ||
					p_base->ime_ctrl.in_size.h != p_ime_path->size.h ||
					p_ime_path->crp_window.x != 0 ||
					p_ime_path->crp_window.y != 0 ||
					p_ime_path->size.w != p_ime_path->crp_window.w ||
					p_ime_path->size.h != p_ime_path->crp_window.h) {
					KDRV_IME_OUT_PATH_IMG_INFO out_img_info;

					out_img_info.path_id = CTL_IPP_OUT_PATH_ID_1;
					kdrv_rt |= kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&out_img_info);

					if (out_img_info.path_en) {
						out_img_info.path_en = DISABLE;
						kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
						CTL_IPP_DBG_ERR_FREQ(120,"yuv compress not support scale/crop when mstp\r\n");
					}
				}
			}
		}

		/* check 3dnr one buffer mode
			mstrp can not support 3dnr reference r/w same buffer
		*/
		if (dce_strp.hstp_num > 1 && dce_ext_info.ime_3dnr_enable) {
			CTL_IPP_IME_OUT_IMG *p_ime_path;

			if (p_base->ime_ctrl.tplnr_in_ref_path < CTL_IPP_OUT_PATH_ID_MAX) {
				p_ime_path = &p_base->ime_ctrl.out_img[p_base->ime_ctrl.tplnr_in_ref_path];
				if (p_ime_path->addr[0] == p_base->ime_ctrl.tplnr_in_ref_addr[0]) {
					KDRV_IME_OUT_PATH_IMG_INFO out_img_info;

					out_img_info.path_id = p_base->ime_ctrl.tplnr_in_ref_path;
					kdrv_rt |= kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
					if (out_img_info.path_en) {
						out_img_info.path_en = DISABLE;
						kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OUT_IMG, (void *)&out_img_info);
						CTL_IPP_DBG_ERR_FREQ(120,"3dnr reference image(%d) in/out same address when mstp\r\n", p_base->ime_ctrl.tplnr_in_ref_path);
					}
				}
			}
		}

		/* disable 2dlut, mstrp not support 2dlut */
		if (dce_strp.hstp_num > 1) {
			KDRV_DCE_PARAM_IPL_FUNC_EN func_en;

			func_en.enable = DISABLE;
			func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_2DLUT;
			kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
		}

		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("dce stripe calculation error rt %d\r\n", (int)(kdrv_rt));
			rt = CTL_IPP_E_KDRV_STRP;
		}
	}
	kdf_ipp_config_end(p_hdl, p_base);
	kdf_ipp_trig_start(p_hdl, p_base);

	/* TRIGGER */
	if (rt == CTL_IPP_E_OK) {
		KDRV_DCE_TRIG_TYPE_INFO dce_trig_cfg;
		INT32 kdrv_rt;

		dce_trig_cfg.opmode = KDRV_DCE_OP_MODE_NORMAL;

		p_hdl->data_addr_in_process[0] = header_addr;
		p_hdl->frame_sta_mask = KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
		p_hdl->proc_end_mask = KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
		p_hdl->engine_used_mask = KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;

		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_DCE);
		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IPE);
		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IME);
		if (kdf_ipp_chk_ife2_ready_to_trig(&p_base->ife2_ctrl) == TRUE) {
			kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IFE2);
		}

		kdrv_rt = E_OK;
		if (kdf_ipp_chk_ife2_ready_to_trig(&p_base->ife2_ctrl) == TRUE) {
			p_hdl->proc_end_mask |= KDF_IPP_ENG_BIT_IFE2;
			p_hdl->engine_used_mask |=KDF_IPP_ENG_BIT_IFE2;
			kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IFE2, p_hdl, NULL, NULL);
		}
		kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IPE, p_hdl, NULL, NULL);
		kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IME, p_hdl, NULL, NULL);
		kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_DCE, p_hdl, NULL, (void *)&dce_trig_cfg);

		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("ipp trigger error rt %d\r\n", (int)(kdrv_rt));
			rt = CTL_IPP_E_KDRV_TRIG;
		}
	}

	kdf_ipp_trig_end(p_hdl, p_base);

	return rt;
}

INT32 kdf_ipp_process_ime_d2d(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev)
{
	INT32 rt;
	CTL_IPP_BASEINFO *p_base;
	KDF_IPP_ISP_CB_INFO iqcb_info;

	p_base = (CTL_IPP_BASEINFO *)header_addr;

	/* GET IME LCA SUBOUT SIZE */
	{
		CTL_IPP_ISP_IME_LCA_SIZE_RATIO lca_size_ratio;

		memset((void *)&lca_size_ratio, 0, sizeof(CTL_IPP_ISP_IME_LCA_SIZE_RATIO));
		iqcb_info.type = KDF_IPP_ISP_CB_TYPE_IQ_GET;
		iqcb_info.item = CTL_IPP_ISP_ITEM_IME_LCA_SIZE;
		iqcb_info.data_addr = (UINT32)&lca_size_ratio;
		kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&iqcb_info, NULL);

		p_base->ime_ctrl.lca_out_size = ctl_ipp_util_lca_subout_size(p_base->ime_ctrl.in_size.w, p_base->ime_ctrl.in_size.h, lca_size_ratio, 0xFFFFFFFF);
		p_base->ime_ctrl.lca_out_lofs = p_base->ime_ctrl.lca_out_size.w * 3;

		if (p_base->ime_ctrl.lca_out_enable) {
			if (p_base->ime_ctrl.lca_out_size.w < CTL_IPP_LCA_H_MIN || p_base->ime_ctrl.lca_out_size.h < CTL_IPP_LCA_V_MIN) {
				CTL_IPP_DBG_WRN("lca out must > (%d,%d) (%d, %d)\r\n", CTL_IPP_LCA_H_MIN, CTL_IPP_LCA_V_MIN, p_base->ime_ctrl.lca_out_size.w, p_base->ime_ctrl.lca_out_size.h);
				rt = CTL_IPP_E_PAR;
				return rt;
			}
		}
	}

	/* GET OUTPUT BUFFER */
	{
		UINT32 i;
		CTL_IPP_IME_OUT_IMG *p_ime_path;
		CTL_IPP_IME_CTRL *p_ime_ctrl = &p_base->ime_ctrl;

		if (kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PRESET, 0, (void *)p_base, NULL) != CTL_IPP_E_OK) {
			/* DISABLE Output when get buffer failed */
			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				p_ime_path = &p_ime_ctrl->out_img[i];
				p_ime_path->dma_enable = FALSE;
			}

			/* skip frame if no reference output buffer*/
			if (p_ime_ctrl->tplnr_enable) {
				if(p_ime_ctrl->out_img[p_ime_ctrl->tplnr_in_ref_path].addr[0] == 0)
					return CTL_IPP_E_NOMEM;
			}
		}
	}

	kdf_ipp_config_start(p_hdl, p_base);

	/* IME SETTING */
	{
		rt = kdf_ipp_config_ime_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ime set error rt %d\r\n", (int)(rt));
		}
	}

	/* TRIGGER */
	kdf_ipp_config_end(p_hdl, p_base);
	kdf_ipp_trig_start(p_hdl, p_base);
	if (rt == CTL_IPP_E_OK) {
		INT32 kdrv_rt;

		/* todo: stripe size for low latency */
		p_hdl->data_addr_in_process[0] = header_addr;
		p_hdl->frame_sta_mask = KDF_IPP_ENG_BIT_IME;
		p_hdl->proc_end_mask = KDF_IPP_ENG_BIT_IME;
		p_hdl->engine_used_mask = KDF_IPP_ENG_BIT_IME;
		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IME);

		kdrv_rt = kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IME, p_hdl, NULL, NULL);
		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("ime d2d trigger error rt %d\r\n", (int)(kdrv_rt));
			rt = CTL_IPP_E_KDRV_TRIG;
		}
	}

	kdf_ipp_trig_end(p_hdl, p_base);

	return rt;
}

INT32 kdf_ipp_process_ipe_d2d(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev)
{
	ER rt;
	CTL_IPP_BASEINFO *p_base;

	p_base = (CTL_IPP_BASEINFO *)header_addr;

	/* GET OUTPUT BUFFER */
	if (kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PRESET, 0, (void *)p_base, NULL) != CTL_IPP_E_OK) {
		return CTL_IPP_E_NOMEM;
	}
	if (p_base->ime_ctrl.out_img[0].dma_enable == DISABLE) {
		return CTL_IPP_E_NOMEM;
	}

	kdf_ipp_config_start(p_hdl, p_base);

	/* IPE SETTING */
	{
		p_base->ipe_ctrl.out_addr[0] = p_base->ime_ctrl.out_img[0].addr[0];
		p_base->ipe_ctrl.out_addr[1] = p_base->ime_ctrl.out_img[0].addr[1];

		rt = kdf_ipp_config_ipe_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ipe set error rt %d\r\n", (int)(rt));
		}
	}

	/* TRIGGER */
	kdf_ipp_config_end(p_hdl, p_base);
	kdf_ipp_trig_start(p_hdl, p_base);
	if (rt == CTL_IPP_E_OK) {
		INT32 kdrv_rt;

		p_hdl->data_addr_in_process[0] = header_addr;
		p_hdl->frame_sta_mask = KDF_IPP_ENG_BIT_IPE;
		p_hdl->proc_end_mask = KDF_IPP_ENG_BIT_IPE;
		p_hdl->engine_used_mask = KDF_IPP_ENG_BIT_IPE;
		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IPE);

		kdrv_rt = kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IPE, p_hdl, NULL, NULL);
		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("ipe d2d trigger error rt %d\r\n", (int)(kdrv_rt));
			rt = CTL_IPP_E_KDRV_TRIG;
		}
	}

	kdf_ipp_trig_end(p_hdl, p_base);

	return rt;
}

INT32 kdf_ipp_process_dce_d2d(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev)
{
	ER rt;
	CTL_IPP_BASEINFO *p_base;

	p_base = (CTL_IPP_BASEINFO *)header_addr;

	/* GET OUTPUT BUFFER */
	if (kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PRESET, 0, (void *)p_base, NULL) != CTL_IPP_E_OK) {
		return CTL_IPP_E_NOMEM;
	}
	if (p_base->ime_ctrl.out_img[0].dma_enable == DISABLE) {
		return CTL_IPP_E_NOMEM;
	}

	kdf_ipp_config_start(p_hdl, p_base);

	/* DCE SETTING */
	{
		p_base->dce_ctrl.out_addr[0] = p_base->ime_ctrl.out_img[0].addr[0];
		p_base->dce_ctrl.out_addr[1] = p_base->ime_ctrl.out_img[0].addr[1];

		rt = kdf_ipp_config_dce_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("dce set error rt %d\r\n", (int)(rt));
		}
	}

	/* STRIPE CALCULATION */
	if (rt == CTL_IPP_E_OK) {
		KDRV_DCE_TRIG_TYPE_INFO dce_trig_cfg = {0};
		KDRV_DCE_EXTEND_INFO dce_ext_info = {0};
		KDRV_DCE_STRIPE_INFO dce_strp = {0};
		INT32 kdrv_rt;

		kdrv_rt = E_OK;
		if (VDO_PXLFMT_CLASS(p_base->ime_ctrl.out_img[0].fmt) == VDO_PXLFMT_CLASS_NVX) {
			dce_ext_info.ime_enc_enable = ENABLE;
		} else {
			dce_ext_info.ime_enc_enable = DISABLE;
		}
		dce_ext_info.ime_3dnr_enable = p_base->ime_ctrl.tplnr_enable;
		dce_ext_info.ime_dec_enable = DISABLE;
		dce_ext_info.low_latency_enable = p_base->ime_ctrl.low_delay_enable;
		kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_EXTEND, (void *)&dce_ext_info);

		/*	trigger dce for stripe calculation
			cal_stripe always wait end and not support timeout due to driver mechanism
			only when stripe_type = KDRV_DCE_STRIPE_MANUAL_MULTI will cause DCE D2D Calculation
		*/
		dce_trig_cfg.opmode = KDRV_DCE_OP_MODE_CAL_STRIP;
		kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_DCE, p_hdl, NULL, (void *)&dce_trig_cfg);
		kdrv_rt |= kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_STRIPE_INFO, (void *)&dce_strp);

		/* set stripe info for debug */
		ctl_ipp_dbg_hdl_stripe_set((UINT32)p_hdl, &dce_strp);

		/* disable 2dlut, mstrp not support 2dlut */
		if (dce_strp.hstp_num > 1) {
			KDRV_DCE_PARAM_IPL_FUNC_EN func_en;

			func_en.enable = DISABLE;
			func_en.ipl_ctrl_func = KDRV_DCE_IQ_FUNC_2DLUT;
			kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ALL_FUNC_EN, (void *)&func_en);
		}

		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("dce stripe calculation error rt %d\r\n", (int)(kdrv_rt));
			rt = CTL_IPP_E_KDRV_STRP;
		}
	}
	kdf_ipp_config_end(p_hdl, p_base);
	kdf_ipp_trig_start(p_hdl, p_base);

	/* TRIGGER */
	if (rt == CTL_IPP_E_OK) {
		KDRV_DCE_TRIG_TYPE_INFO dce_trig_cfg = {0};
		INT32 kdrv_rt;

		p_hdl->data_addr_in_process[0] = header_addr;
		p_hdl->frame_sta_mask = KDF_IPP_ENG_BIT_DCE;
		p_hdl->proc_end_mask = KDF_IPP_ENG_BIT_DCE;
		p_hdl->engine_used_mask = KDF_IPP_ENG_BIT_DCE;
		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_DCE);

		dce_trig_cfg.opmode = KDRV_DCE_OP_MODE_NORMAL;
		kdrv_rt = kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_DCE, p_hdl, NULL, (void *)&dce_trig_cfg);
		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("ipe d2d trigger error rt %d\r\n", (int)(kdrv_rt));
			rt = CTL_IPP_E_KDRV_TRIG;
		}
	}

	kdf_ipp_trig_end(p_hdl, p_base);

	return rt;
}

INT32 kdf_ipp_process_direct(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 msg)
{
	INT32 rt;
	CTL_IPP_BASEINFO *p_base;
	KDRV_IME_SINGLE_OUT_INFO ime_single_out_info = {0};
	unsigned long loc_cpu_flg;

	rt = CTL_IPP_E_OK;
	p_base = (CTL_IPP_BASEINFO *)header_addr;


	/* skip frame if no reference output buffer*/
	if (msg == KDF_IPP_DIRECT_MSG_START) {
		/* do nothing */
	} else {
		if (p_base->ime_ctrl.tplnr_enable) {
			if (p_base->ime_ctrl.out_img[p_base->ime_ctrl.tplnr_in_ref_path].addr[0] == 0) {
				return CTL_IPP_E_NOMEM;
			}
		}
	}

	/* Start don't check single out*/
	if (msg != KDF_IPP_DIRECT_MSG_START) {
		/* check ime single out, if ime_sigle_out_info.sout_chl > 0, means ISR is shift, do not set parameters */
		if (kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_SINGLE_OUTPUT_HW, (void *)&ime_single_out_info) != E_OK) {
			CTL_IPP_DBG_WRN("get single out fail\r\n");
			return CTL_IPP_E_KDRV_GET;
		}
		if (ime_single_out_info.sout_chl != 0) {
			CTL_IPP_DBG_WRN("single out not load %x\r\n",ime_single_out_info.sout_chl);
			return CTL_IPP_E_STATE;
		}
	}
	/* for debug */
	ctl_ipp_dbg_set_direct_isr_sequence((UINT32) p_hdl, "BP", header_addr);


	p_hdl->rev_evt_time = ctl_ipp_util_get_syst_counter();

	kdf_ipp_config_start(p_hdl, p_base);

	/* IFE SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_ife_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ife set error rt %d\r\n", (int)(rt));
		}
	}

	/* DCE SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_dce_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("dce set error rt %d\r\n", (int)(rt));
			return rt;
		}
	}

	/* IPE SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_ipe_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ipe set error rt %d\r\n", (int)(rt));
		}
	}

	/* IME SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_ime_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ime set error rt %d\r\n", (int)(rt));
		}
	}

	/* IFE2 SETTING */
	if (rt == CTL_IPP_E_OK) {
		rt = kdf_ipp_config_ife2_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ife2 set error rt %d\r\n", (int)(rt));
		}
	}

	if (rt == CTL_IPP_E_OK) {
		if (kdf_ipp_direct_datain_drop(p_hdl, KDF_IPP_FRAME_BP, header_addr) != CTL_IPP_E_OK){
			kdf_ipp_config_end(p_hdl, p_base);
			return CTL_IPP_E_INDATA;
		}
	}

	if (rt == CTL_IPP_E_OK) {
		KDRV_IME_EXTEND_INFO ime_ext_info;
		INT32 kdrv_rt;

		kdrv_rt = E_OK;

		/* keep stripe size in baseinfo */
		memset((void *)p_base->dce_ctrl.strp_h_arr, 0, (sizeof(UINT32) * CTL_IPP_DCE_STRP_NUM_MAX));
		p_base->dce_ctrl.strp_h_arr[0] = p_base->dce_ctrl.in_size.w;

		/* direct mode only surpport single stripe, set ime input size to ext_info*/
		ime_ext_info.stripe_h_max = p_base->ime_ctrl.in_size.w;
		ime_ext_info.stripe_num = 1;
		ime_ext_info.tmnr_en = 0;
		ime_ext_info.p1_enc_en = 0;
		kdrv_rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_EXTEND, (void *)&ime_ext_info);
		if (kdrv_rt != E_OK) {
			return CTL_IPP_E_KDRV_SET;
		}

		// debug dma write protect for frame buffer, yuv buffer and subout. start protecting before trigger
		ctl_ipp_dbg_dma_wp_proc_start(ctl_ipp_info_get_entry_by_info_addr((UINT32)p_base));

		/* TRIGGER */
		if (msg == KDF_IPP_DIRECT_MSG_START) {
			KDRV_DCE_TRIG_TYPE_INFO dce_trig_cfg;

			dce_trig_cfg.opmode = KDRV_DCE_OP_MODE_NORMAL;

			#if defined (__KERNEL__)
			/* check if siwtch flow from fastboot to normal */
			if (kdrv_builtin_is_fastboot() && p_hdl->is_first_handle) {
				/* first frame use only ime interrupt, switch to normal mask when first framend */
				p_hdl->proc_end_mask = KDF_IPP_ENG_BIT_IME;
				p_hdl->frame_sta_mask = KDF_IPP_ENG_BIT_IME;
			} else {
				p_hdl->proc_end_mask = KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
				p_hdl->frame_sta_mask = KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
			}
			#else
				p_hdl->proc_end_mask = KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
				p_hdl->frame_sta_mask = KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
			#endif
			p_hdl->engine_used_mask = KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
			if (kdf_ipp_chk_ife2_ready_to_trig(&p_base->ife2_ctrl) == TRUE) {
				p_hdl->engine_used_mask |= KDF_IPP_ENG_BIT_IFE2;
				/* do not wait ife2 frame end, ife2 do not provide frame start signal*/
				/* Problem will be caused when frame start and frame end ISR accumulate */
			}

			/* trigger engine */
			kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IFE);
			kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_DCE);
			kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IPE);
			kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IME);
			kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IFE2);
			if (kdf_ipp_chk_ife2_ready_to_trig(&p_base->ife2_ctrl) == TRUE) {
				kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IFE2, p_hdl, NULL, NULL);
			}
			kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IFE, p_hdl, NULL, NULL);
			kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IPE, p_hdl, NULL, NULL);
			kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IME, p_hdl, NULL, NULL);
			kdrv_rt |= kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_DCE, p_hdl, NULL, (void *)&dce_trig_cfg);

			if (kdrv_rt != E_OK) {
				CTL_IPP_DBG_WRN("ipp trigger error rt %d\r\n", (int)(kdrv_rt));
				rt = CTL_IPP_E_KDRV_TRIG;
			}
			kdf_ipp_direct_load_bit_clr(&p_hdl->dir_load_bit, KDF_IPP_DIR_LOAD_PAUSE_E);
			//kdf_ipp_direct_load_bit_set(&p_hdl->dir_load_bit, KDF_IPP_DIR_LOAD_FMS);
		} else if (msg == KDF_IPP_DIRECT_MSG_CFG) {
			kdf_ipp_direct_load_bit_set(&p_hdl->dir_load_bit, KDF_IPP_DIR_LOAD_VD);
			kdf_ipp_direct_load(p_hdl);
		} else if (msg == KDF_IPP_DIRECT_MSG_STOP) {
			if (p_hdl->dir_load_bit & KDF_IPP_DIR_LOAD_PAUSE_E) {
				kdf_ipp_direct_load_bit_set(&p_hdl->dir_load_bit, KDF_IPP_DIR_LOAD_VD);
			} else {
				kdf_ipp_direct_load_bit_set(&p_hdl->dir_load_bit, KDF_IPP_DIR_LOAD_VD | KDF_IPP_DIR_LOAD_PAUSE);
			}
			kdf_ipp_direct_load(p_hdl);
		} else {
			CTL_IPP_DBG_WRN("unknown msg %d\r\n", (int)(msg));
		}
	}

	/* below is the code that need to be confirmed set in the end of process */
	/* lock cpu to prevent isr coming, which can cause using incomplete data */
	loc_cpu(loc_cpu_flg);
	p_base->ctl_frm_sta_count = p_hdl->fra_sta_with_sop_cnt;
	rt |= kdf_ipp_direct_datain_drop(p_hdl, KDF_IPP_FRAME_BP_END, header_addr) ;
	unl_cpu(loc_cpu_flg);

	kdf_ipp_config_end(p_hdl, p_base);

	return rt;
}

INT32 kdf_ipp_process_flow_pattern_paste(KDF_IPP_HANDLE *p_hdl, UINT32 header_addr, UINT32 rev)
{
	INT32 rt;
	CTL_IPP_BASEINFO *p_base;

	p_base = (CTL_IPP_BASEINFO *)header_addr;

	/* GET OUTPUT BUFFER */
	{
		UINT32 i;
		CTL_IPP_IME_OUT_IMG *p_ime_path;
		CTL_IPP_IME_CTRL *p_ime_ctrl = &p_base->ime_ctrl;

		if (kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PRESET, 0, (void *)p_base, NULL) != CTL_IPP_E_OK) {
			/* DISABLE Output when get buffer failed */
			for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
				p_ime_path = &p_ime_ctrl->out_img[i];
				p_ime_path->dma_enable = FALSE;
			}
		}
	}

	/* IME SETTING */
	{
		rt = kdf_ipp_config_ime_common(p_hdl, p_base);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("ime set error rt %d\r\n", (int)(rt));
		}
	}

	/* TRIGGER */
	if (rt == CTL_IPP_E_OK) {
		INT32 kdrv_rt;

		/* todo: stripe size for low latency */
		p_hdl->data_addr_in_process[0] = header_addr;
		p_hdl->frame_sta_mask = KDF_IPP_ENG_BIT_IME;
		p_hdl->proc_end_mask = KDF_IPP_ENG_BIT_IME;
		p_hdl->engine_used_mask = KDF_IPP_ENG_BIT_IME;
		kdf_ipp_set_trig_hdl(p_hdl, KDF_IPP_ENG_IME);

		kdrv_rt = kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG_IME, p_hdl, NULL, NULL);
		if (kdrv_rt != E_OK) {
			CTL_IPP_DBG_WRN("ime d2d trigger error rt %d\r\n", (int)(kdrv_rt));
			rt = CTL_IPP_E_KDRV_TRIG;
		}
	}

	return rt;
}

#if 0
#endif

static KDF_IPP_KDRV_OPEN_FP kdf_ipp_kdrv_open_fp[KDF_IPP_ENG_MAX];
static KDF_IPP_KDRV_CLOSE_FP kdf_ipp_kdrv_close_fp[KDF_IPP_ENG_MAX];
static KDF_IPP_KDRV_TRIG_FP kdf_ipp_kdrv_trig_fp[KDF_IPP_ENG_MAX];
static KDF_IPP_KDRV_GET_FP kdf_ipp_kdrv_get_fp[KDF_IPP_ENG_MAX];
static KDF_IPP_KDRV_SET_FP kdf_ipp_kdrv_set_fp[KDF_IPP_ENG_MAX];
static KDF_IPP_KDRV_PAUSE_FP kdf_ipp_kdrv_pause_fp[KDF_IPP_ENG_MAX];
static KDF_IPP_KDRV_DBG_INFO kdf_ipp_kdrv_debug_info[KDF_IPP_ENG_MAX];
static BOOL *kdf_ipp_kdrv_channel_is_used[KDF_IPP_ENG_MAX];
static UINT32 kdf_ipp_kdrv_channel_max_num;

_INLINE KDRV_DEV_ENGINE kdf_ipp_typecast_kdrv_eng(KDF_IPP_ENG eng)
{
	switch (eng) {
	case KDF_IPP_ENG_RHE:
		return 0;

	case KDF_IPP_ENG_IFE:
		return KDRV_VIDEOPROCS_IFE_ENGINE0;

	case KDF_IPP_ENG_DCE:
		return KDRV_VIDEOPROCS_DCE_ENGINE0;

	case KDF_IPP_ENG_IPE:
		return KDRV_VIDEOPROCS_IPE_ENGINE0;

	case KDF_IPP_ENG_IME:
		return KDRV_VIDEOPROCS_IME_ENGINE0;

	case KDF_IPP_ENG_IFE2:
		return KDRV_VIDEOPROCS_IFE2_ENGINE0;

	default:
		return 0;
	}
}

_INLINE UINT32 kdf_ipp_get_eng_isr_param_bit(KDF_IPP_ENG eng)
{
	switch (eng) {
	case KDF_IPP_ENG_RHE:
		return 0;

	case KDF_IPP_ENG_IFE:
		return KDRV_IFE_PARAM_REV;

	case KDF_IPP_ENG_DCE:
		return KDRV_DCE_PARAM_REV;

	case KDF_IPP_ENG_IPE:
		return KDRV_IPE_PARAM_REV;

	case KDF_IPP_ENG_IME:
		return KDRV_IME_PARAM_REV;

	case KDF_IPP_ENG_IFE2:
		return KDRV_IFE2_PARAM_REV;

	default:
		return 0;
	}
}

UINT32 kdf_ipp_kdrv_wrapper_init(UINT32 num, UINT32 buf_addr, UINT32 is_query)
{
	UINT32 i;
	UINT32 buf_size;

	buf_size = num * sizeof(BOOL) * KDF_IPP_ENG_MAX;
	if (is_query) {
		ctl_ipp_dbg_ctxbuf_log_set("KDRV_WRAP", CTL_IPP_DBG_CTX_BUF_QUERY, buf_size, buf_addr, num);
		return buf_size;
	}

	memset((void *)buf_addr, 0, buf_size);
	kdf_ipp_kdrv_channel_max_num = num;
	for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
		kdf_ipp_kdrv_channel_is_used[i] = (BOOL *)buf_addr;
		buf_addr += num * sizeof(BOOL);
	}
	ctl_ipp_dbg_ctxbuf_log_set("KDRV_WRAP", CTL_IPP_DBG_CTX_BUF_ALLOC, buf_size, buf_addr, num);

	kdf_ipp_kdrv_wrapper_mount_api(FALSE);

	return buf_size;
}

void kdf_ipp_kdrv_wrapper_uninit(void)
{
	UINT32 i;

	for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
		kdf_ipp_kdrv_channel_is_used[i] = NULL;
	}
	kdf_ipp_kdrv_channel_max_num = 0;
}

void kdf_ipp_kdrv_wrapper_mount_api(BOOL dbg_mode)
{
	kdf_ipp_kdrv_open_fp[KDF_IPP_ENG_RHE] = NULL;
	kdf_ipp_kdrv_close_fp[KDF_IPP_ENG_RHE] = NULL;
	kdf_ipp_kdrv_trig_fp[KDF_IPP_ENG_RHE] = NULL;
	kdf_ipp_kdrv_set_fp[KDF_IPP_ENG_RHE] = NULL;
	kdf_ipp_kdrv_get_fp[KDF_IPP_ENG_RHE] = NULL;
	kdf_ipp_kdrv_pause_fp[KDF_IPP_ENG_RHE] = NULL;

	kdf_ipp_kdrv_open_fp[KDF_IPP_ENG_IFE] = kdrv_ife_open;
	kdf_ipp_kdrv_close_fp[KDF_IPP_ENG_IFE] = kdrv_ife_close;
	kdf_ipp_kdrv_trig_fp[KDF_IPP_ENG_IFE] = (KDF_IPP_KDRV_TRIG_FP)kdrv_ife_trigger;
	kdf_ipp_kdrv_set_fp[KDF_IPP_ENG_IFE] = (KDF_IPP_KDRV_SET_FP)kdrv_ife_set;
	kdf_ipp_kdrv_get_fp[KDF_IPP_ENG_IFE] = (KDF_IPP_KDRV_GET_FP)kdrv_ife_get;
	kdf_ipp_kdrv_pause_fp[KDF_IPP_ENG_IFE] = kdrv_ife_pause;

	kdf_ipp_kdrv_open_fp[KDF_IPP_ENG_DCE] = kdrv_dce_open;
	kdf_ipp_kdrv_close_fp[KDF_IPP_ENG_DCE] = kdrv_dce_close;
	kdf_ipp_kdrv_trig_fp[KDF_IPP_ENG_DCE] = (KDF_IPP_KDRV_TRIG_FP)kdrv_dce_trigger;
	kdf_ipp_kdrv_set_fp[KDF_IPP_ENG_DCE] = (KDF_IPP_KDRV_SET_FP)kdrv_dce_set;
	kdf_ipp_kdrv_get_fp[KDF_IPP_ENG_DCE] = (KDF_IPP_KDRV_GET_FP)kdrv_dce_get;
	kdf_ipp_kdrv_pause_fp[KDF_IPP_ENG_DCE] = kdrv_dce_pause;

	kdf_ipp_kdrv_open_fp[KDF_IPP_ENG_IPE] = kdrv_ipe_open;
	kdf_ipp_kdrv_close_fp[KDF_IPP_ENG_IPE] = kdrv_ipe_close;
	kdf_ipp_kdrv_trig_fp[KDF_IPP_ENG_IPE] = (KDF_IPP_KDRV_TRIG_FP)kdrv_ipe_trigger;
	kdf_ipp_kdrv_set_fp[KDF_IPP_ENG_IPE] = (KDF_IPP_KDRV_SET_FP)kdrv_ipe_set;
	kdf_ipp_kdrv_get_fp[KDF_IPP_ENG_IPE] = (KDF_IPP_KDRV_GET_FP)kdrv_ipe_get;
	kdf_ipp_kdrv_pause_fp[KDF_IPP_ENG_IPE] = kdrv_ipe_pause;

	kdf_ipp_kdrv_open_fp[KDF_IPP_ENG_IME] = kdrv_ime_open;
	kdf_ipp_kdrv_close_fp[KDF_IPP_ENG_IME] = kdrv_ime_close;
	kdf_ipp_kdrv_trig_fp[KDF_IPP_ENG_IME] = (KDF_IPP_KDRV_TRIG_FP)kdrv_ime_trigger;
	kdf_ipp_kdrv_set_fp[KDF_IPP_ENG_IME] = (KDF_IPP_KDRV_SET_FP)kdrv_ime_set;
	kdf_ipp_kdrv_get_fp[KDF_IPP_ENG_IME] = (KDF_IPP_KDRV_GET_FP)kdrv_ime_get;
	kdf_ipp_kdrv_pause_fp[KDF_IPP_ENG_IME] = kdrv_ime_pause;

	kdf_ipp_kdrv_open_fp[KDF_IPP_ENG_IFE2] = kdrv_ife2_open;
	kdf_ipp_kdrv_close_fp[KDF_IPP_ENG_IFE2] = kdrv_ife2_close;
	kdf_ipp_kdrv_trig_fp[KDF_IPP_ENG_IFE2] = (KDF_IPP_KDRV_TRIG_FP)kdrv_ife2_trigger;
	kdf_ipp_kdrv_set_fp[KDF_IPP_ENG_IFE2] = (KDF_IPP_KDRV_SET_FP)kdrv_ife2_set;
	kdf_ipp_kdrv_get_fp[KDF_IPP_ENG_IFE2] = (KDF_IPP_KDRV_GET_FP)kdrv_ife2_get;
	kdf_ipp_kdrv_pause_fp[KDF_IPP_ENG_IFE2] = kdrv_ife2_pause;
}

INT32 kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl)
{
	INT32 rt;
	UINT32 channel;

	if (eng == KDF_IPP_ENG_RHE) {
		return E_OK;
	}

	if (eng >= KDF_IPP_ENG_MAX) {
		CTL_IPP_DBG_ERR("unknown engine %d\r\n", (int)(eng));
		return E_ID;
	}

	/* get free channel	*/
	for (channel = 0; channel < kdf_ipp_kdrv_channel_max_num; channel++) {
		if (kdf_ipp_kdrv_channel_is_used[eng][channel] == FALSE) {
			break;
		}
	}

	if (channel == kdf_ipp_kdrv_channel_max_num) {
		CTL_IPP_DBG_ERR("no free channel for eng %d\r\n", (int)(eng));
		return E_ID;
	}

	rt = E_SYS;
	if (kdf_ipp_kdrv_open_fp[eng] != NULL) {
		UINT64 proc_time;
		KDF_IPP_KDRV_DBG_INFO *p_dbg_info;

		proc_time = ctl_ipp_util_get_syst_counter();
		rt = kdf_ipp_kdrv_open_fp[eng](hdl->chip, kdf_ipp_typecast_kdrv_eng(eng));
		proc_time = ctl_ipp_util_get_syst_counter() - proc_time;

		if (rt == E_OK) {
			kdf_ipp_kdrv_channel_is_used[eng][channel] = TRUE;
			hdl->channel[eng] = channel;
		}

		/* update debug info */
		p_dbg_info = &kdf_ipp_kdrv_debug_info[eng];
		if (p_dbg_info->open_cnt >= KDF_IPP_KDRV_DBG_AVG_NUM) {
			p_dbg_info->open_cnt = 1;
			p_dbg_info->open_time_acc = proc_time;
		} else {
			p_dbg_info->open_cnt++;
			p_dbg_info->open_time_acc += proc_time;
		}

		if (proc_time > p_dbg_info->open_time_max) {
			p_dbg_info->open_time_max = proc_time;
		}
	}

	return rt;
}

INT32 kdf_ipp_kdrv_close_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl)
{
	INT32 rt;

	if (eng == KDF_IPP_ENG_RHE) {
		return E_OK;
	}

	if (eng >= KDF_IPP_ENG_MAX) {
		CTL_IPP_DBG_ERR("unknown engine %d\r\n", (int)(eng));
		return E_ID;
	}

	/* free used channel */
	if (hdl->channel[eng] >= kdf_ipp_kdrv_channel_max_num) {
		CTL_IPP_DBG_ERR("illegal channel %d of eng %d\r\n", (int)(hdl->channel[eng]), (int)(eng));
		return E_ID;
	}

	if (kdf_ipp_kdrv_channel_is_used[eng][hdl->channel[eng]] == FALSE) {
		CTL_IPP_DBG_ERR("eng %d, channel %d already closed\r\n", (int)(eng), (int)(hdl->channel[eng]));
		return E_OBJ;
	}
	kdf_ipp_kdrv_channel_is_used[eng][hdl->channel[eng]] = FALSE;
	hdl->channel[eng] = KDF_IPP_HANDLE_KDRV_CH_NONE;

	rt = E_SYS;
	if (kdf_ipp_kdrv_close_fp[eng] != NULL) {
		UINT64 proc_time;
		KDF_IPP_KDRV_DBG_INFO *p_dbg_info;

		proc_time = ctl_ipp_util_get_syst_counter();
		rt = kdf_ipp_kdrv_close_fp[eng](hdl->chip, kdf_ipp_typecast_kdrv_eng(eng));
		proc_time = ctl_ipp_util_get_syst_counter() - proc_time;

		/* update debug info */
		p_dbg_info = &kdf_ipp_kdrv_debug_info[eng];
		if (p_dbg_info->close_cnt >= KDF_IPP_KDRV_DBG_AVG_NUM) {
			p_dbg_info->close_cnt = 1;
			p_dbg_info->close_time_acc = proc_time;
		} else {
			p_dbg_info->close_cnt++;
			p_dbg_info->close_time_acc += proc_time;
		}

		if (proc_time > p_dbg_info->close_time_max) {
			p_dbg_info->close_time_max = proc_time;
		}
	}

	return rt;
}

INT32 kdf_ipp_kdrv_trig_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl, void *param, void *p_user_data)
{
	INT32 rt;
	UINT32 id;

	if (eng == KDF_IPP_ENG_RHE) {
		return E_OK;
	}

	if (eng >= KDF_IPP_ENG_MAX) {
		CTL_IPP_DBG_WRN("unknown engine %d\r\n", (int)(eng));
		return E_ID;
	}

	if (hdl->channel[eng] == KDF_IPP_HANDLE_KDRV_CH_NONE) {
		return E_OK;
	}

	rt = E_SYS;
	id = KDRV_DEV_ID(hdl->chip, kdf_ipp_typecast_kdrv_eng(eng), hdl->channel[eng]);
	if (kdf_ipp_kdrv_trig_fp[eng] != NULL) {
		UINT64 proc_time;
		KDF_IPP_KDRV_DBG_INFO *p_dbg_info;

		proc_time = ctl_ipp_util_get_syst_counter();
		rt = kdf_ipp_kdrv_trig_fp[eng](id, param, NULL, p_user_data);
		proc_time = ctl_ipp_util_get_syst_counter() - proc_time;

		/* update debug info */
		p_dbg_info = &kdf_ipp_kdrv_debug_info[eng];
		if (p_dbg_info->trig_cnt >= KDF_IPP_KDRV_DBG_AVG_NUM) {
			p_dbg_info->trig_cnt = 1;
			p_dbg_info->trig_time_acc = proc_time;
		} else {
			p_dbg_info->trig_cnt++;
			p_dbg_info->trig_time_acc += proc_time;
		}

		if (proc_time > p_dbg_info->trig_time_max) {
			p_dbg_info->trig_time_max = proc_time;
		}
	}

	return rt;
}

INT32 kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl, UINT32 param_id, void *data)
{
	INT32 rt;
	UINT32 id;

	if (eng == KDF_IPP_ENG_RHE) {
		return E_OK;
	}

	if (eng >= KDF_IPP_ENG_MAX) {
		CTL_IPP_DBG_WRN("unknown engine %d\r\n", (int)(eng));
		return E_PAR;
	}

	if (hdl->channel[eng] == KDF_IPP_HANDLE_KDRV_CH_NONE) {
		return E_OK;
	}

	rt = E_SYS;
	id = KDRV_DEV_ID(hdl->chip, kdf_ipp_typecast_kdrv_eng(eng), hdl->channel[eng]);
	if (kdf_ipp_kdrv_get_fp[eng] != NULL) {
		UINT64 proc_time;
		KDF_IPP_KDRV_DBG_INFO *p_dbg_info;

		param_id = param_id | kdf_ipp_get_eng_isr_param_bit(eng);
		proc_time = ctl_ipp_util_get_syst_counter();
		rt = kdf_ipp_kdrv_get_fp[eng](id, param_id, data);
		proc_time = ctl_ipp_util_get_syst_counter() - proc_time;

		/* update debug info */
		p_dbg_info = &kdf_ipp_kdrv_debug_info[eng];
		if (p_dbg_info->get_cnt >= KDF_IPP_KDRV_DBG_AVG_NUM) {
			p_dbg_info->get_cnt = 1;
			p_dbg_info->get_time_acc = proc_time;
		} else {
			p_dbg_info->get_cnt++;
			p_dbg_info->get_time_acc += proc_time;
		}

		if (proc_time > p_dbg_info->get_time_max) {
			p_dbg_info->get_time_max = proc_time;
			p_dbg_info->get_time_max_item = param_id;
		}
	}

	return rt;
}

INT32 kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl, UINT32 param_id, void *data)
{
	INT32 rt;
	UINT32 id;

	if (eng == KDF_IPP_ENG_RHE) {
		return E_OK;
	}

	if (eng >= KDF_IPP_ENG_MAX) {
		CTL_IPP_DBG_WRN("unknown engine %d\r\n", (int)(eng));
		return E_PAR;
	}

	if (hdl->channel[eng] == KDF_IPP_HANDLE_KDRV_CH_NONE) {
		return E_OK;
	}


	rt = E_SYS;
	id = KDRV_DEV_ID(hdl->chip, kdf_ipp_typecast_kdrv_eng(eng), hdl->channel[eng]);
	if (kdf_ipp_kdrv_set_fp[eng] != NULL) {
		UINT64 proc_time;
		KDF_IPP_KDRV_DBG_INFO *p_dbg_info;

		param_id = param_id | kdf_ipp_get_eng_isr_param_bit(eng);
		proc_time = ctl_ipp_util_get_syst_counter();
		rt = kdf_ipp_kdrv_set_fp[eng](id, param_id, data);
		proc_time = ctl_ipp_util_get_syst_counter() - proc_time;

		/* update debug info */
		p_dbg_info = &kdf_ipp_kdrv_debug_info[eng];
		if (p_dbg_info->set_cnt >= KDF_IPP_KDRV_DBG_AVG_NUM) {
			p_dbg_info->set_cnt = 1;
			p_dbg_info->set_time_acc = proc_time;
		} else {
			p_dbg_info->set_cnt++;
			p_dbg_info->set_time_acc += proc_time;
		}

		if (proc_time > p_dbg_info->set_time_max) {
			p_dbg_info->set_time_max = proc_time;
			p_dbg_info->set_time_max_item = param_id;
		}
	}

	return rt;
}

INT32 kdf_ipp_kdrv_pause_wrapper(KDF_IPP_ENG eng, KDF_IPP_HANDLE *hdl)
{
	INT32 rt;

	if (eng == KDF_IPP_ENG_RHE) {
		return E_OK;
	}

	if (eng >= KDF_IPP_ENG_MAX) {
		CTL_IPP_DBG_WRN("unknown engine %d\r\n", (int)(eng));
		return E_ID;
	}

	if (hdl->channel[eng] == KDF_IPP_HANDLE_KDRV_CH_NONE) {
		return E_OK;
	}

	rt = E_SYS;
	if (kdf_ipp_kdrv_pause_fp[eng] != NULL) {
		UINT64 proc_time;
		KDF_IPP_KDRV_DBG_INFO *p_dbg_info;

		proc_time = ctl_ipp_util_get_syst_counter();
		rt = kdf_ipp_kdrv_pause_fp[eng](hdl->chip, kdf_ipp_typecast_kdrv_eng(eng));
		proc_time = ctl_ipp_util_get_syst_counter() - proc_time;

		/* update debug info */
		p_dbg_info = &kdf_ipp_kdrv_debug_info[eng];
		if (p_dbg_info->pause_cnt >= KDF_IPP_KDRV_DBG_AVG_NUM) {
			p_dbg_info->pause_cnt = 1;
			p_dbg_info->pause_time_acc = proc_time;
		} else {
			p_dbg_info->pause_cnt++;
			p_dbg_info->pause_time_acc += proc_time;
		}

		if (proc_time > p_dbg_info->pause_time_max) {
			p_dbg_info->pause_time_max = proc_time;
		}
	}

	return rt;
}

void kdf_ipp_kdrv_wrapper_debug_dump(int (*dump)(const char *fmt, ...))
{
	CHAR *str_eng[] = {
		"RHE",
		"IFE",
		"DCE",
		"IPE",
		"IME",
		"IFE2",
		""
	};
	UINT32 i;
	KDF_IPP_KDRV_DBG_INFO *p_dbg_info;

	dump("---- KDRV API PROC TIME ----\r\n");
	dump("%8s%8s%16s%16s%16s\r\n", "op", "eng", "avg_time", "max_time", "max_time_item");
	for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
		UINT32 proc_time;

		p_dbg_info = &kdf_ipp_kdrv_debug_info[i];
		if (p_dbg_info->open_cnt > 0) {
			proc_time = p_dbg_info->open_time_acc / p_dbg_info->open_cnt;
		} else {
			proc_time = 0;
		}

		dump("%8s%8s%16d%16d%16s\r\n", "open", str_eng[i],
			(int)proc_time, (int)p_dbg_info->open_time_max, "N/A");
	}

	for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
		UINT32 proc_time;

		p_dbg_info = &kdf_ipp_kdrv_debug_info[i];
		if (p_dbg_info->close_cnt > 0) {
			proc_time = p_dbg_info->close_time_acc / p_dbg_info->close_cnt;
		} else {
			proc_time = 0;
		}

		dump("%8s%8s%16d%16d%16s\r\n", "close", str_eng[i],
			(int)proc_time, (int)p_dbg_info->close_time_max, "N/A");
	}

	for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
		UINT32 proc_time;

		p_dbg_info = &kdf_ipp_kdrv_debug_info[i];
		if (p_dbg_info->trig_cnt > 0) {
			proc_time = p_dbg_info->trig_time_acc / p_dbg_info->trig_cnt;
		} else {
			proc_time = 0;
		}

		dump("%8s%8s%16d%16d%16s\r\n", "trig", str_eng[i],
			(int)proc_time, (int)p_dbg_info->trig_time_max, "N/A");
	}

	for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
		UINT32 proc_time;

		p_dbg_info = &kdf_ipp_kdrv_debug_info[i];
		if (p_dbg_info->pause_cnt > 0) {
			proc_time = p_dbg_info->pause_time_acc / p_dbg_info->pause_cnt;
		} else {
			proc_time = 0;
		}

		dump("%8s%8s%16d%16d%16s\r\n", "pause", str_eng[i],
			(int)proc_time, (int)p_dbg_info->pause_time_max, "N/A");
	}

	for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
		UINT32 proc_time;

		p_dbg_info = &kdf_ipp_kdrv_debug_info[i];
		if (p_dbg_info->get_cnt > 0) {
			proc_time = p_dbg_info->get_time_acc / p_dbg_info->get_cnt;
		} else {
			proc_time = 0;
		}

		dump("%8s%8s%16d%16d      0x%.8x\r\n", "get", str_eng[i],
			(int)proc_time, (int)p_dbg_info->get_time_max, (unsigned int)p_dbg_info->get_time_max_item);
	}

	for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
		UINT32 proc_time;

		p_dbg_info = &kdf_ipp_kdrv_debug_info[i];
		if (p_dbg_info->set_cnt > 0) {
			proc_time = p_dbg_info->set_time_acc / p_dbg_info->set_cnt;
		} else {
			proc_time = 0;
		}

		dump("%8s%8s%16d%16d      0x%.8x\r\n", "set", str_eng[i],
			(int)proc_time, (int)p_dbg_info->set_time_max, (unsigned int)p_dbg_info->set_time_max_item);
	}
}

