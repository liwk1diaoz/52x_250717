#include "kdf_sie_int.h"
#include "ctl_sie_utility_int.h"

#if 0
#endif

/*
    global variables
*/
static BOOL kdf_sie_init_flg = FALSE;
static KDF_SIE_HDL g_kdf_sie_hdl[CTL_SIE_MAX_SUPPORT_ID];
static KDF_SIE_HDL_CONTEXT kdf_sie_ctx = {0};

#if 0
#endif
/*
    static function
*/

_INLINE KDRV_DEV_ENGINE __kdf_sie_typecast_kdrv_eng(CTL_SIE_ID id)
{
	KDRV_DEV_ENGINE kdrv_eng_map[] = {
		KDRV_VDOCAP_ENGINE0,
		KDRV_VDOCAP_ENGINE1,
		KDRV_VDOCAP_ENGINE2,
		KDRV_VDOCAP_ENGINE3,
		KDRV_VDOCAP_ENGINE4
	};

	if (!ctl_sie_module_chk_id_valid(id)) {
		return 0;
	}

	return kdrv_eng_map[id];
}

static KDRV_SIE_CLKSRC_SEL __kdf_sie_typecast_clk_src(CTL_SIE_CLKSRC_SEL clk_src)
{
	switch (clk_src) {
	case CTL_SIE_CLKSRC_CURR:
		return KDRV_SIE_CLKSRC_CURR;

	case CTL_SIE_CLKSRC_192:
		return KDRV_SIE_CLKSRC_192;

	case CTL_SIE_CLKSRC_320:
		return KDRV_SIE_CLKSRC_320;

	case CTL_SIE_CLKSRC_480:
		return KDRV_SIE_CLKSRC_480;

	case CTL_SIE_CLKSRC_PLL5:
		return KDRV_SIE_CLKSRC_PLL5;

	case CTL_SIE_CLKSRC_PLL10:
		return KDRV_SIE_CLKSRC_PLL10;

	case CTL_SIE_CLKSRC_PLL12:
		return KDRV_SIE_CLKSRC_PLL12;

	case CTL_SIE_CLKSRC_PLL13:
		return KDRV_SIE_CLKSRC_PLL13;

	default:
		ctl_sie_dbg_err("N.S. this clk src %d\r\n", (int)clk_src);
		return KDRV_SIE_CLKSRC_480;
	}
}

static KDRV_SIE_PIX __kdf_sie_typecast_cfapat(CTL_SIE_RAW_PIX raw_cfa)
{
	switch (raw_cfa) {
	case CTL_SIE_RGGB_PIX_R:
		return KDRV_SIE_RGGB_PIX_R;

	case CTL_SIE_RGGB_PIX_GR:
		return KDRV_SIE_RGGB_PIX_GR;

	case CTL_SIE_RGGB_PIX_GB:
		return KDRV_SIE_RGGB_PIX_GB;

	case CTL_SIE_RGGB_PIX_B:
		return KDRV_SIE_RGGB_PIX_B;

	case CTL_SIE_RGBIR_PIX_RGBG_GIGI:
		return KDRV_SIE_RGBIR_PIX_RG_GI;

	case CTL_SIE_RGBIR_PIX_GBGR_IGIG:
		return KDRV_SIE_RGBIR_PIX_GB_IG;

	case CTL_SIE_RGBIR_PIX_GIGI_BGRG:
		return KDRV_SIE_RGBIR_PIX_GI_BG;

	case CTL_SIE_RGBIR_PIX_IGIG_GRGB:
		return KDRV_SIE_RGBIR_PIX_IG_GR;

	case CTL_SIE_RGBIR_PIX_BGRG_GIGI:
		return KDRV_SIE_RGBIR_PIX_BG_GI;

	case CTL_SIE_RGBIR_PIX_GRGB_IGIG:
		return KDRV_SIE_RGBIR_PIX_GR_IG;

	case CTL_SIE_RGBIR_PIX_GIGI_RGBG:
		return KDRV_SIE_RGBIR_PIX_GI_RG;

	case CTL_SIE_RGBIR_PIX_IGIG_GBGR:
		return KDRV_SIE_RGBIR_PIX_IG_GB;

	case CTL_SIE_RCCB_PIX_RC:
		return KDRV_SIE_RCCB_PIX_RC;

	case CTL_SIE_RCCB_PIX_CR:
		return KDRV_SIE_RCCB_PIX_CR;

	case CTL_SIE_RCCB_PIX_CB:
		return KDRV_SIE_RCCB_PIX_CB;

	case CTL_SIE_RCCB_PIX_BC:
		return KDRV_SIE_RCCB_PIX_BC;

	default:
		ctl_sie_dbg_err("N.S. this cfapat %x\r\n", (unsigned int)raw_cfa);
		return KDRV_SIE_RGGB_PIX_R;
	}
}

/*
    isr function
*/

static void __kdf_sie_isr(UINT32 id, UINT32 status, void *in_data, void *out_data)
{
	if (g_kdf_sie_hdl[id].isrcb_fp != NULL) {
		g_kdf_sie_hdl[id].isrcb_fp(id, status, NULL, NULL);
	}
}

#if 0
#endif
/*
    public function
*/
UINT32 kdf_sie_buf_query(UINT32 num)
{
	UINT32 total_buf = 0;

	if (num == 0) {
		ctl_sie_dbg_err("query number is 0\r\n");
		return 0;
	}
	memset((void *)&kdf_sie_ctx, 0, sizeof(KDF_SIE_HDL_CONTEXT));
	kdf_sie_ctx.context_num = num;
	kdf_sie_ctx.req_size = num * ALIGN_CEIL(sizeof(KDF_SIE_HDL), VOS_ALIGN_BYTES);
	//TOTO, add kdf buffer size, one kdf handle is 16byte
	total_buf = kdrv_sie_buf_query(num);
//	total_buf = kdf_sie_ctx.req_size;

	return total_buf;
}

INT32 kdf_sie_init(UINT32 buf_addr, UINT32 buf_size)
{
	if (kdf_sie_init_flg) {
		ctl_sie_dbg_wrn("ctl sie already init\r\n");
		return CTL_SIE_E_STATE;
	}

	if (buf_addr == 0 || buf_size == 0) {
		ctl_sie_dbg_err("init buffer zero, addr %x, buf_size %x\r\n", (unsigned int)buf_addr, (unsigned int)buf_size);
		return CTL_SIE_E_NOMEM;
	}

	kdf_sie_ctx.start_addr = buf_addr;
	kdf_sie_init_flg = TRUE;
	if (kdrv_sie_buf_init(buf_addr, buf_size) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("kdrv buf req size > input buf size %x\r\n", (unsigned int)buf_size);
		kdrv_sie_buf_uninit();
		kdf_sie_uninit();
		return CTL_SIE_E_NOMEM;
	}

	return CTL_SIE_E_OK;
}

INT32 kdf_sie_uninit(void)
{
	if (kdf_sie_init_flg) {
		kdf_sie_init_flg = FALSE;
		kdrv_sie_buf_uninit();
		return CTL_SIE_E_OK;
	} else {
		ctl_sie_dbg_wrn("ctl sie already uninit\r\n");
		return CTL_SIE_E_STATE;
	}
}

INT32 kdf_sie_set_mclk(UINT32 id, void *data)
{
	INT32 rt = CTL_SIE_E_OK;

	rt |= kdrv_sie_set(KDRV_DEV_ID(KDRV_CHIP0, __kdf_sie_typecast_kdrv_eng(id), 0), KDRV_SIE_ITEM_MCLK|KDRV_SIE_PARAM_REV, data);
	rt |= kdrv_sie_set(KDRV_DEV_ID(KDRV_CHIP0, __kdf_sie_typecast_kdrv_eng(id), 0), KDRV_SIE_ITEM_LOAD|KDRV_SIE_PARAM_REV, NULL);

	return rt;
}

static KDRV_SIE_PROC_ID kdf_sie_module_conv2_kdrv_id(CTL_SIE_ID id)
{
	switch (id) {
	case CTL_SIE_ID_1:
		return KDRV_SIE_ID_1;

	case CTL_SIE_ID_2:
		return KDRV_SIE_ID_2;

	case CTL_SIE_ID_3:
		return KDRV_SIE_ID_3;

	case CTL_SIE_ID_4:
		return KDRV_SIE_ID_4;

	case CTL_SIE_ID_5:
		return KDRV_SIE_ID_5;

	case CTL_SIE_ID_6:
		return KDRV_SIE_ID_6;

	case CTL_SIE_ID_7:
		return KDRV_SIE_ID_7;

	case CTL_SIE_ID_8:
		return KDRV_SIE_ID_8;

	default:
		ctl_sie_dbg_err("ctl id %d overflow (max: %d)\r\n", (int)id, (int)KDRV_SIE_ID_MAX_NUM);
		return KDRV_SIE_ID_MAX_NUM;
	}
}

INT32 kdf_sie_get_limit(UINT32 id, void *data)
{
	return kdrv_sie_get_sie_limit(kdf_sie_module_conv2_kdrv_id(id), data);
}

INT32 kdf_sie_set(UINT32 hdl, UINT64 item, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	UINT32 ign_chk = KDRV_SIE_PARAM_REV;
	CTL_SIE_HDL *sie_ctrl = (CTL_SIE_HDL *)data;
	CTL_SIE_INTE sie_inte;
	KDF_SIE_HDL *p_hdl = (KDF_SIE_HDL *)hdl;

	if (!kdf_sie_init_flg) {
		ctl_sie_dbg_err("kdf sie init flag is FALSE\r\n");
		return CTL_SIE_E_STATE;
	}

	if (sie_ctrl == NULL || p_hdl == NULL) {
		ctl_sie_dbg_err("NULL handle\r\n");
		return CTL_SIE_E_NULL_FP;
	}

	if (!ctl_sie_module_chk_id_valid(p_hdl->ctl_id)) {
		return CTL_SIE_E_ID;
	}

	if (item == 0) {
		return rt;
	}

	if (item & (1ULL << CTL_SIE_ITEM_OUT_DEST)) {
		KDRV_SIE_OUT_DEST kdrv_sie_out_dest = sie_ctrl->param.out_dest;

		//NT98520, kdrv sie2 only support dram mode, select dram mode and enable ring buffer mode
		if (p_hdl->ctl_id == CTL_SIE_ID_2 && kdrv_sie_out_dest == KDRV_SIE_OUT_DEST_DIRECT) {
			kdrv_sie_out_dest = KDRV_SIE_OUT_DEST_DRAM;
		}
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_OUT_DEST|ign_chk, (void *)&kdrv_sie_out_dest);
	}

	if (item & (1ULL << CTL_SIE_ITEM_DATAFORMAT)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_DATA_FMT|ign_chk, (void *)&sie_ctrl->param.data_fmt);
	}

	if (item & (1ULL << CTL_SIE_ITEM_CHGSENMODE)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_SIGNAL|ign_chk, (void *)&sie_ctrl->ctrl.rtc_obj.signal);
	}

	if (item & (1ULL << CTL_SIE_ITEM_FLIP)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_FLIP|ign_chk, (void *)&sie_ctrl->param.flip);
	}

	if (item & 1ULL << (CTL_SIE_ITEM_CH0_LOF)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_CH0_LOF|ign_chk, (void *)&sie_ctrl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_0]);
	}

	if (item & 1ULL << (CTL_SIE_ITEM_CH1_LOF)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_CH1_LOF|ign_chk, (void *)&sie_ctrl->ctrl.rtc_obj.out_ch_lof[CTL_SIE_CH_1]);
	}


	if (item & (1ULL << CTL_SIE_ITEM_IO_SIZE)) {
		KDRV_SIE_ACT_CRP_WIN act_crp_win;
		USIZE scl_sz = {0};

		act_crp_win.cfa_pat = __kdf_sie_typecast_cfapat(sie_ctrl->ctrl.rtc_obj.rawcfa_act);
		act_crp_win.win = sie_ctrl->ctrl.rtc_obj.sie_act_win;
		if (act_crp_win.win.x == 0 && act_crp_win.win.y == 0 && act_crp_win.win.w == 0 && act_crp_win.win.h == 0) {
		} else {
			rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_ACT_WIN|ign_chk, (void *)&act_crp_win);
		}

		act_crp_win.cfa_pat = __kdf_sie_typecast_cfapat(sie_ctrl->ctrl.rtc_obj.rawcfa_crp);
		act_crp_win.win = sie_ctrl->param.io_size_info.size_info.sie_crp;
		if (act_crp_win.win.x == 0 && act_crp_win.win.y == 0 && act_crp_win.win.w == 0 && act_crp_win.win.h == 0) {
		} else {
			rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_CROP_WIN|ign_chk, (void *)&act_crp_win);
		}
		scl_sz = sie_ctrl->param.io_size_info.size_info.sie_scl_out;
		if (scl_sz.w == 0 && scl_sz.h == 0) {
		} else {
			rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_SCALEOUT|ign_chk, (void *)&scl_sz);
		}
	}

	if (item & (1ULL << CTL_SIE_ITEM_PATGEN_INFO)) {
		KDRV_SIE_PATGEN_INFO kdrv_pat_gen_info;

		kdrv_pat_gen_info.mode = sie_ctrl->ctrl.rtc_obj.pat_gen_param.pat_gen_mode;
		kdrv_pat_gen_info.src_win = sie_ctrl->ctrl.rtc_obj.pat_gen_param.pat_gen_src_win;
		kdrv_pat_gen_info.val = sie_ctrl->ctrl.rtc_obj.pat_gen_param.pat_gen_val;
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_PATGEN_INFO|ign_chk, (void *)&kdrv_pat_gen_info);
	}

	if (item & (1ULL << CTL_SIE_ITEM_ENCODE) || item & (1ULL << CTL_SIE_ITEM_ENC_RATE)) {
		KDRV_SIE_RAW_ENCODE kdrv_enc_data;

		kdrv_enc_data.enable = sie_ctrl->param.encode_en;
		kdrv_enc_data.enc_rate = sie_ctrl->param.enc_rate;
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_ENCODE|ign_chk, (void *)&kdrv_enc_data);
	}

	if (item & (1ULL << CTL_SIE_ITEM_CCIR) || item & (1ULL << CTL_SIE_ITEM_MUX_DATA)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_CCIR|ign_chk, (void *)&sie_ctrl->param.ccir_info_int);
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_YUV_ORDER|ign_chk, (void *)&sie_ctrl->param.ccir_info_int.yuv_order);
	}

	//ctl - kdf internal item
	if (item & (1ULL << CTL_SIE_INT_ITEM_DMA_OUT)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_DMA_OUT_EN|ign_chk, (void *)&sie_ctrl->ctrl.dma_out);
	}

	if (item & (1ULL << CTL_SIE_INT_ITEM_INTE)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_INTE|ign_chk, (void *)&sie_ctrl->ctrl.inte);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_BP1)) {
		if (sie_ctrl->ctrl.bp1 == 0) {
			sie_inte = (sie_ctrl->ctrl.inte&~CTL_SIE_INTE_BP1);
		} else {
			sie_inte = sie_ctrl->ctrl.inte|CTL_SIE_INTE_BP1;
			rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_BP1|ign_chk, (void *)&sie_ctrl->ctrl.bp1);
		}
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_INTE|ign_chk, (void *)&sie_inte);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_BP2)) {
		if (sie_ctrl->ctrl.bp2 == 0) {
			sie_inte = (sie_ctrl->ctrl.inte&~CTL_SIE_INTE_BP2);
		} else {
			sie_inte = sie_ctrl->ctrl.inte|CTL_SIE_INTE_BP2;
			rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_BP2|ign_chk, (void *)&sie_ctrl->ctrl.bp2);
		}
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_INTE|ign_chk, (void *)&sie_inte);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_BP3)) {
		if (sie_ctrl->ctrl.bp3 == 0) {
			sie_inte = (sie_ctrl->ctrl.inte&~CTL_SIE_INTE_BP3);
		} else {
			sie_inte = sie_ctrl->ctrl.inte|CTL_SIE_INTE_BP3;
			rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_BP3|ign_chk, (void *)&sie_ctrl->ctrl.bp3);
		}
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_INTE|ign_chk, (void *)&sie_inte);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_CH0_ADDR)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_CH0_ADDR|ign_chk, (void *)&sie_ctrl->ctrl.out_ch_addr[CTL_SIE_CH_0]);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_CH1_ADDR)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_CH1_ADDR|ign_chk, (void *)&sie_ctrl->ctrl.out_ch_addr[CTL_SIE_CH_1]);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_CH2_ADDR)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_CH2_ADDR|ign_chk, (void *)&sie_ctrl->ctrl.out_ch_addr[CTL_SIE_CH_2]);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_CA_ROI)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_CA_ROI|ign_chk, (void *)&sie_ctrl->ctrl.ca_roi);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_LA_ROI)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_LA_ROI|ign_chk, (void *)&sie_ctrl->ctrl.la_roi);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_OB)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_OB|ign_chk, (void *)&sie_ctrl->ctrl.ob_param);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_CA)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_CA|ign_chk, (void *)&sie_ctrl->ctrl.ca_param);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_LA)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_LA|ign_chk, (void *)&sie_ctrl->ctrl.la_param);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_CGAIN)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_CGAIN|ign_chk, (void *)&sie_ctrl->ctrl.cgain_param);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_DGAIN)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_DGAIN|ign_chk, (void *)&sie_ctrl->ctrl.dgain_param);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_DPC)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_DPC|ign_chk, (void *)&sie_ctrl->ctrl.dpc_param);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_ECS)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_ECS|ign_chk, (void *)&sie_ctrl->ctrl.ecs_param);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_COMPAND)) {
		if (CTL_SIE_COMPANDING_MAX_LEN != KDRV_SIE_COMPANDING_MAX_LEN) {
			ctl_sie_dbg_err("ctl & kdrv cnt not-match\r\n");
			rt |= CTL_SIE_E_PAR;
		} else {
			rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_COMPANDING | ign_chk, (void *)&sie_ctrl->ctrl.companding_param);
		}
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_PXCLK)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_PXCLK|ign_chk, (void *)&sie_ctrl->ctrl.clk_info.pclk_src_sel);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_CH_OUT_MODE)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_OUTPUT_MODE|ign_chk, (void *)&sie_ctrl->ctrl.ch_out_mode);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_SINGLE_OUT_CTL)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_SINGLEOUT|ign_chk, (void *)&sie_ctrl->ctrl.sin_out_ctl);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_RING_BUF)) {
		KDRV_SIE_RINGBUF_INFO ring_buf_info;

		ring_buf_info.enable = sie_ctrl->ctrl.ring_buf_ctl.en;
		ring_buf_info.ring_buf_len = sie_ctrl->ctrl.ring_buf_ctl.buf_len;
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_RING_BUF|ign_chk, (void *)&ring_buf_info);
	}

	if (item & 1ULL << (CTL_SIE_INT_ITEM_MD)) {
		rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_MD|ign_chk, (void *)&sie_ctrl->ctrl.md_param);
	}

	if (item & 1ULL << (CTL_SIE_ITEM_TRIG_IMM)) {
		kdf_sie_trigger(hdl, (void *)&sie_ctrl->ctrl.trig_info);
	}

	rt |= kdrv_sie_set(p_hdl->kdrv_id, KDRV_SIE_ITEM_LOAD|ign_chk, NULL);

	if (rt != E_OK) {
		rt = CTL_SIE_E_KDRV_SET;
	}
	return rt;
}

INT32 kdf_sie_get(UINT32 hdl, UINT64 item, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	KDF_SIE_HDL *p_hdl = (KDF_SIE_HDL *)hdl;
	KDRV_SIE_PARAM_ID kdrv_item;
	CTL_SIE_ISP_KDRV_PARAM *kdrv_param;
	KDRV_SIE_CLK_INFO kdrv_sie_clk = {0};
	UINT32 ign_chk = 0;

	if (!kdf_sie_init_flg) {
		ctl_sie_dbg_err("kdf sie init flag is FALSE\r\n");
		return CTL_SIE_E_STATE;
	}

	if (p_hdl == NULL || data == NULL) {
		return CTL_SIE_E_HDL;
	}

	if (!ctl_sie_module_chk_id_valid(p_hdl->ctl_id)) {
		return CTL_SIE_E_ID;
	}


	if (CTL_SIE_IGN_CHK & item) {
		ign_chk = KDRV_SIE_IGN_CHK;
		item = item & (~CTL_SIE_IGN_CHK);
	}

	switch (item) {
	case CTL_SIE_ITEM_CA_RSLT:
		kdrv_item = KDRV_SIE_ITEM_CA_RSLT;
		break;

	case CTL_SIE_ITEM_LA_RSLT:
		kdrv_item = KDRV_SIE_ITEM_LA_RSLT;
		break;

	case CTL_SIE_INT_ITEM_LIMIT:
		kdrv_item = KDRV_SIE_ITEM_LIMIT;
		break;

	case CTL_SIE_INT_ITEM_CLK:
		if (kdrv_sie_get(p_hdl->kdrv_id, KDRV_SIE_ITEM_SIECLK|ign_chk, (void *)&kdrv_sie_clk) == CTL_SIE_E_OK) {
			*(UINT32 *)data = kdrv_sie_clk.rate;
			return CTL_SIE_E_OK;
		} else {
			return CTL_SIE_E_KDRV_GET;
		}
		break;

	case CTL_SIE_INT_ITEM_KDRV_PARAM:
		kdrv_param = (CTL_SIE_ISP_KDRV_PARAM *)data;
		rt = kdrv_sie_get(p_hdl->kdrv_id, kdrv_param->param_id|ign_chk, (void *)kdrv_param->data);
		if (rt != CTL_SIE_E_OK) {
			return CTL_SIE_E_KDRV_GET;
		}
		return CTL_SIE_E_OK;

	case CTL_SIE_INT_ITEM_SINGLE_OUT_CTL:
		kdrv_item = KDRV_SIE_ITEM_SINGLEOUT;
		break;

	case CTL_SIE_INT_ITEM_MD_RSLT:
		kdrv_item = KDRV_SIE_ITEM_MD_RSLT;
		break;

	default:
		return CTL_SIE_E_OK;
	}
	rt = kdrv_sie_get(p_hdl->kdrv_id, kdrv_item|ign_chk, (void *)data);

	if (rt != CTL_SIE_E_OK) {
		rt = CTL_SIE_E_KDRV_GET;
	}
	return rt;
}

INT32 kdf_sie_trigger(UINT32 hdl, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	KDF_SIE_HDL *p_hdl = (KDF_SIE_HDL *)hdl;
	CTL_SIE_TRIG_INFO trig_info =  *(CTL_SIE_TRIG_INFO *)data;
	KDRV_SIE_TRIG_INFO kdrv_trig_info;

	if (!ctl_sie_module_chk_id_valid(p_hdl->ctl_id)) {
		return CTL_SIE_E_ID;
	}

	//set isr cb fp
	kdrv_sie_set(g_kdf_sie_hdl[p_hdl->ctl_id].kdrv_id, KDRV_SIE_ITEM_ISRCB, (void *)__kdf_sie_isr);
	kdrv_trig_info.trig_type = trig_info.trig_type;
	kdrv_trig_info.wait_end = trig_info.b_wait_end;
	rt = kdrv_sie_trigger(p_hdl->kdrv_id, NULL, NULL, (void *)&kdrv_trig_info);

	if (rt != CTL_SIE_E_OK) {
		rt = CTL_SIE_E_KDRV_TRIG;
	}
	return rt;
}

UINT32 kdf_sie_open(CTL_SIE_ID id, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	// prepare sie open data
	KDRV_SIE_OPENCFG kdrv_open_cfg = {0};
	CTL_SIE_HDL *sie_info = (CTL_SIE_HDL *)data;

	if (!kdf_sie_init_flg) {
		ctl_sie_dbg_err("kdf sie init flag is FALSE\r\n");
		return 0;
	}

	if (data == NULL) {
		ctl_sie_dbg_err("data is NULL\r\n");
		return 0;
	}

	if (!ctl_sie_module_chk_id_valid(id)) {
		return 0;
	}

	memset((void *)&g_kdf_sie_hdl[id], 0, sizeof(KDF_SIE_HDL));

	g_kdf_sie_hdl[id].chip = KDRV_CHIP0;	//tmp add@Jarkko
	g_kdf_sie_hdl[id].ctl_id = id;
	g_kdf_sie_hdl[id].kdrv_id = KDRV_DEV_ID(g_kdf_sie_hdl[id].chip, __kdf_sie_typecast_kdrv_eng(id), 0);

	kdrv_open_cfg.act_mode = sie_info->ctrl.clk_info.act_mode;
	kdrv_open_cfg.clk_src_sel = __kdf_sie_typecast_clk_src(sie_info->ctrl.clk_info.clk_src_sel);
	kdrv_open_cfg.pclk_src_sel = sie_info->ctrl.clk_info.pclk_src_sel;
	kdrv_open_cfg.data_rate = sie_info->ctrl.clk_info.data_rate;
	//sie config
	rt |= kdrv_sie_set(g_kdf_sie_hdl[id].kdrv_id, KDRV_SIE_ITEM_OPENCFG, (void *)&kdrv_open_cfg);
	g_kdf_sie_hdl[id].isrcb_fp = sie_info->ctrl.int_isrcb_fp;
	// open sie
	rt |= kdrv_sie_open(g_kdf_sie_hdl[id].chip, __kdf_sie_typecast_kdrv_eng(id));
	//set interrupt enable
	rt |= kdrv_sie_set(g_kdf_sie_hdl[id].kdrv_id, KDRV_SIE_ITEM_INTE, (void *)&sie_info->ctrl.inte);
	if (rt == CTL_SIE_E_OK) {
		return (UINT32)&g_kdf_sie_hdl[id];
	}

	return 0;
}

INT32 kdf_sie_close(UINT32 hdl)
{
	KDF_SIE_HDL *p_hdl = (KDF_SIE_HDL *)hdl;

	if (!kdf_sie_init_flg) {
		ctl_sie_dbg_err("kdf sie init flag is FALSE\r\n");
		return CTL_SIE_E_STATE;
	}

	if (!ctl_sie_module_chk_id_valid(p_hdl->ctl_id)) {
		return CTL_SIE_E_ID;
	}

	p_hdl->isrcb_fp = NULL;
	// close sie
	if (kdrv_sie_close(p_hdl->chip, __kdf_sie_typecast_kdrv_eng(p_hdl->ctl_id)) != CTL_SIE_E_OK) {
		return CTL_SIE_E_KDRV_CLOSE;
	}
	return CTL_SIE_E_OK;
}

INT32 kdf_sie_suspend(UINT32 hdl, void *data)
{
	KDF_SIE_HDL *p_hdl = (KDF_SIE_HDL *)hdl;
	CTL_SIE_SUS_RES_LVL sus_lvl =  *(CTL_SIE_SUS_RES_LVL *)data;
	KDRV_SIE_SUSPEND_PARAM kdrv_suspend_param;

	if (sus_lvl & CTL_SIE_SUS_RES_PXCLK || sus_lvl & CTL_SIE_SUS_RES_ALL) {
		kdrv_suspend_param.pxclk_en = ENABLE;
	}
	kdrv_suspend_param.wait_end = ENABLE;
	return kdrv_sie_suspend(p_hdl->kdrv_id, (void *)&kdrv_suspend_param);
}

INT32 kdf_sie_resume(UINT32 hdl, void *data)
{
	KDF_SIE_HDL *p_hdl = (KDF_SIE_HDL *)hdl;
	CTL_SIE_SUS_RES_LVL res_lvl =  *(CTL_SIE_SUS_RES_LVL *)data;
	KDRV_SIE_RESUME_PARAM kdrv_resume_param;

	if (res_lvl & CTL_SIE_SUS_RES_PXCLK || res_lvl & CTL_SIE_SUS_RES_ALL) {
		kdrv_resume_param.pxclk_en = ENABLE;
	}
	return kdrv_sie_resume(p_hdl->kdrv_id, (void *)&kdrv_resume_param);
}

INT32 kdf_sie_dump_fb_info(INT fd, UINT32 en)
{
	return kdrv_sie_dump_fb_info(fd, en);
}
