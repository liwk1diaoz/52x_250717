#include "kdrv_sie_debug_int.h"
#include "kdrv_sie_int.h"
#include "kwrap/sxcmd.h"
#include <kwrap/cmdsys.h>

KDRV_SIE_DBG_LVL kdrv_sie_dbg_rt_lvl = KDRV_SIE_DBG_LVL_ERR;
KDRV_SIE_DBG_INFO kdrv_sie_dbg_info[KDRV_SIE_MAX_ENG] = {0};
static KDRV_SIE_LIMIT sie_dbg_limit[KDRV_SIE_MAX_ENG] = {0};

int kdrv_sie_int_dbg_printf(const char *fmtstr, ...)
{
	char    buf[512];
	int     len;

	va_list marker;

	va_start(marker, fmtstr);

	len = vsnprintf(buf, sizeof(buf), fmtstr, marker);
	va_end(marker);
	if (len > 0) {
		DBG_DUMP("%s", buf);
	}

	return 0;
}

void kdrv_sie_dbg_set_dbg_level(KDRV_SIE_DBG_LVL dbg_level)
{
	UINT32 id = 0;

	kdrv_sie_dbg_rt_lvl = dbg_level;
	kdrv_sie_get_sie_limit(KDRV_SIE_ID_1, (void *)&sie_dbg_limit[KDRV_SIE_ID_1]);
	for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
		if (kdrv_sie_dbg_rt_lvl == KDRV_SIE_DBG_LVL_NONE) {
			sie_set_checksum_en(kdrv_sie_conv2_sie_id(id), DISABLE);
		} else {
			sie_set_checksum_en(kdrv_sie_conv2_sie_id(id), ENABLE);
		}
	}
}

void kdrv_sie_dbg_set_msg_type(KDRV_SIE_PROC_ID id, KDRV_SIE_DBG_MSG_TYPE type)
{
	if (!kdrv_sie_chk_id_valid(id)) {
		return;
	}

	if (type >= KDRV_SIE_DBG_MSG_MAX) {
		kdrv_sie_dbg_wrn("Id: %d: debug level %d over KDRV_SIE_DBG_MSG_MAX\r\n", (int)(id), (int)(type));
		return;
	}

	switch (type) {
	case KDRV_SIE_DBG_MSG_CTL_INFO:
		kdrv_sie_dump_info(NULL);
		break;

	case KDRV_SIE_DBG_MSG_ALL:
		kdrv_sie_dump_info(NULL);
		break;

	case KDRV_SIE_DBG_MSG_OFF:
	default:
		break;
	}

	kdrv_sie_dbg_ind("Set id =_%d Debug Msg Type to %d\r\n", (int)id, (int)type);
	kdrv_sie_dbg_info[id].dbg_msg_type = type;
}

void kdrv_sie_dump_info(int (*dump)(const char *fmt, ...))
{
	UINT32 id = 0;
	KDRV_SIE_STATUS sie_status[KDRV_SIE_MAX_ENG];
	KDRV_SIE_INFO *kdrv_sie_hdl[KDRV_SIE_MAX_ENG];
	SIE_SYS_DEBUG_INFO sys_dbg_info = {0};

	if (dump == NULL) {
		dump = kdrv_sie_int_dbg_printf;
	}

	kdrv_sie_get_sie_limit(KDRV_SIE_ID_1, (void *)&sie_dbg_limit[KDRV_SIE_ID_1]);
	dump("\r\n[KDRV SIE]Dump Begin\r\n");
	if(nvt_get_chip_id() == CHIP_NA51055)  {
		dump("[Chip Id = CHIP_NA51055]\r\n");
	} else {
		dump("[Chip Id = NONE\r\n");
	}

	for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
		sie_status[id] = kdrv_sie_get_state_machine(id);
		kdrv_sie_hdl[id] = kdrv_sie_get_hdl(id);
		kdrv_sie_get_sie_limit(id, (void *)&sie_dbg_limit[id]);
	}

	/***** context info ************************************************************/
	{
		KDRV_SIE_HDL_CONTEXT *sie_ctx;

		sie_ctx = kdrv_sie_get_ctx();
		dump("\r\n-----ctx-----\r\n");
		dump("%-8s%-10s%-12s%-12s\r\n", "ctx_num", "ctx_used", "addr", "req_size");
		if (sie_ctx == NULL) {
			dump("NULL ctx\r\n");
		} else {
			dump("%-8d%-10d%#-12x%#-12x\r\n" , (int)sie_ctx->ctx_num, (int)sie_ctx->ctx_used, (unsigned int)sie_ctx->start_addr, (unsigned int)sie_ctx->req_size);

			dump("\r\n%-8s%-10s%-12s\r\n", "ctx_idx", "id", "hdl_adr");
			for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
				if (sie_ctx->ctx_idx[id] != KDRV_SIE_ID_MAX_NUM) {
					dump("%-8d%-10d%#-12x\r\n", (int)id, (int)sie_ctx->ctx_idx[id], (int)(UINT32 *)kdrv_sie_hdl[sie_ctx->ctx_idx[id]]);
				}
			}
		}
	}
	/***** limitation Info **********************************************************/
	{
		dump("\r\n-----limit-----\r\n");
		dump("%-4s%-12s%-12s%-12s\r\n", "id", "max_spt_id", "max_out_ch", "spt_func");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (kdrv_sie_hdl[id] != NULL) {
				dump("%-4d%-12d%-12d%#-10x\r\n", (int)id, (int)sie_dbg_limit[id].max_spt_id, (int)sie_dbg_limit[id].out_max_ch, (unsigned int)sie_dbg_limit[id].support_func);
			}
		}
	}
	/***** Ctrl Info **********************************************************/
	{
		CHAR* str_state[KDRV_SIE_STS_MAX] = {
			"CLOSE",
			"READY",
			"RUN",
			"SUSPEND"
		};
		dump("\r\n-----ctrl-----\r\n");
		dump("%-4s%-12s%-12s%-8s%-8s%-8s%-22s%-14s\r\n", "id", "state", "inte_en", "bp1", "bp3", "bp3", "dma_out(ch0 ch1 ch2)", "drv_func_en");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (kdrv_sie_hdl[id] != NULL) {
				dump("%-4d%-12s0x%-10x%-8d%-8d%-8d%-7d(%-4d%-4d%-4d) %#-14x\r\n", id, str_state[sie_status[id]], (unsigned int)kdrv_sie_hdl[id]->inte, (int)kdrv_sie_hdl[id]->bp_info.bp1, (int)kdrv_sie_hdl[id]->bp_info.bp2, (int)kdrv_sie_hdl[id]->bp_info.bp3,
				(int)kdrv_sie_hdl[id]->b_dma_out, (int)kdrv_sie_hdl[id]->singleout_info.ch0singleout_enable, (int)kdrv_sie_hdl[id]->singleout_info.ch1singleout_enable, (int)kdrv_sie_hdl[id]->singleout_info.ch2singleout_enable, (unsigned int)kdrv_sie_hdl[id]->func_en);
			}
		}
	}
	/***** Clock Info **********************************************************/
	{
		KDRV_SIE_CLK_HDL *clk_info;
		CHAR* str_mclk_src[7] = {
			"CUR",
			"480",
			"PLL4",
			"PLL5",
			"PLL10",
			"PLL12",
			"PLL13"
		};
		CHAR* str_mclk_id_sel[3] = {
			"MCLK",
			"MCLK2",
			"MCLK3"
		};
		CHAR* str_clk_src[8] = {
			"CUR",
			"480",
			"PLL5",
			"PLL13",
			"PLL12",
			"320",
			"192",
			"PLL10"
		};
		CHAR* str_pclk_src[6] = {
			"OFF",
			"PAD",
			"PAD_AHD",
			"MCLK",
			"VX1_1X",
			"VX1_2X"
		};

		dump("\r\n-----clock-----\r\n");
		dump("%-4s%-8s%-10s%-12s%-12s%-10s%-12s%-12s\r\n", "id", "mclk_en", "mclk_src", "mclk_id_sel", "mclk_rate", "clk_src", "clk_rate", "pclk_src");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			clk_info = kdrv_sie_get_clk_hdl(id);
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || clk_info == NULL) {
				continue;
			}

			dump("%-4d%-8d%-10s%-12s%-12d%-10s%-12d%-12s\r\n", (int)id, (int)clk_info->mclk_info.clk_en, str_mclk_src[clk_info->mclk_info.mclk_src_sel], str_mclk_id_sel[clk_info->mclk_info.mclk_id_sel], (int)clk_info->mclk_info.clk_rate,
				str_clk_src[clk_info->clk_info.clk_src_sel], (int)clk_info->clk_info.rate, str_pclk_src[clk_info->pxclk_info]);
		}
	}
	/***** Debug Info *********************************************************/
	{
		dump("\r\n-----SysDbg-----\r\n");
		dump("%-4s%-10s%-8s%-10s%-12s%-12s%-16s%-12s\r\n", "id", "pxclk_in", "clk_in", "line_cnt", "max_pxl_cnt", "csi_hd_cnt", "csi_ln_end_cnt", "csi_pxl_cnt");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE) {
				continue;
			}
			sie_get_sys_debug_info(kdrv_sie_conv2_sie_id(id), &sys_dbg_info);
			dump("%-4d%-10d%-8d%-10d%-12d%-12d%-16d%-12d\r\n", (int)id, (int)sys_dbg_info.pxclkIn, (int)sys_dbg_info.sieclkIn, (int)sys_dbg_info.uiLineCnt, (int)sys_dbg_info.uiMaxPixelCnt
				, (int)sys_dbg_info.uiCsiHdCnt, (int)sys_dbg_info.uiCsiLineEdCnt, (int)sys_dbg_info.uiCsiPixCnt);
		}

	}
	/***** Input Info *********************************************************/
	{
		CHAR* str_actmode[15] = {
			"PARA_MSTR",
			"PARA_SLAV",
			"PATGEN",
			"VX1_IF0",
			"CSI_1",
			"CSI_2",
			"CSI_3",
			"CSI_4",
			"CSI_5",
			"CSI_6",
			"CSI_7",
			"CSI_8",
			"AHD",
			"SLVS_EC",
			"VX1_IF1"
		};

		CHAR* str_sig[2] = {
			"R",
			"F"
		};

		CHAR* str_pix[KDRV_SIE_RCCB_PIX_MAX] = {
			"RGGB_R",
			"RGGB_GR",
			"RGGB_GB",
			"RGGB_B",
			"",
			"RGBIR_RG_GI",
			"RGBIR_GB_IG",
			"RGBIR_GI_BG",
			"RGBIR_IG_GR",
			"RGBIR_BG_GI",
			"RGBIR_GR_IG",
			"RGBIR_GI_RG",
			"RGBIR_IG_GB",
			"",
			"RCCB_RC",
			"RCCB_CR",
			"RCCB_CB",
			"RCCB_BC",
		};

		dump("\r\n-----input-----\r\n");
		dump("%-4s%-10s%-22s%-8s%-8s%-28s%-28s%-12s%-12s\r\n", "id", "mode", "phase(vd, hd, data)", "vd_inv", "hd_inv", "act_win(x, y, w, h)", "crp_win(x, y, w, h)", "act_cfa", "crp_cfa");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			dump("%-4d%-10s(%4s, %4s, %4s)    %-8d%-8d(%4d, %4d, %4d, %4d)    (%4d, %4d, %4d, %4d)    %-12s%-12s\r\n", id, str_actmode[kdrv_sie_hdl[id]->open_cfg.act_mode],
				str_sig[kdrv_sie_hdl[id]->signal.vd_phase], str_sig[kdrv_sie_hdl[id]->signal.hd_phase], str_sig[kdrv_sie_hdl[id]->signal.data_phase],
				kdrv_sie_hdl[id]->signal.vd_inverse, kdrv_sie_hdl[id]->signal.hd_inverse,
				kdrv_sie_hdl[id]->act_window.win.x, kdrv_sie_hdl[id]->act_window.win.y, kdrv_sie_hdl[id]->act_window.win.w, kdrv_sie_hdl[id]->act_window.win.h,
				kdrv_sie_hdl[id]->crp_window.win.x, kdrv_sie_hdl[id]->crp_window.win.y, kdrv_sie_hdl[id]->crp_window.win.w, kdrv_sie_hdl[id]->crp_window.win.h,
				str_pix[kdrv_sie_hdl[id]->act_window.cfa_pat], str_pix[kdrv_sie_hdl[id]->crp_window.cfa_pat]);
		}
	}

	/***** CCIR Info **********************************************************/
	{
		CHAR* str_yuv[4] = {
			"YUYV",
			"YVYU",
			"UYVY",
			"VYUY",
		};
		dump("\r\n-----ccir-----\r\n");
		dump("%-4s%-12s%-8s%-8s%-12s%-12s%-12s%-12s%-10s%-10s\r\n", "id", "yuv_order", "hd_swap", "split", "656vd_mode", "data_period", "data_idx", "auto_align", "field_en", "field_sel");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_DVI) {
				if (kdrv_sie_typecast_ccir(kdrv_sie_hdl[id]->data_fmt)) {
					dump("%-4d%-12s%-8d%-8d%-12d%-12d%-12d%-12d%-10d%-10d\r\n", id,str_yuv[kdrv_sie_hdl[id]->ccir_info.yuv_order],
						kdrv_sie_hdl[id]->ccir_info.fmt, kdrv_sie_hdl[id]->ccir_info.dvi_mode, kdrv_sie_hdl[id]->ccir_info.ccir656_vd_sel,
						kdrv_sie_hdl[id]->ccir_info.data_period, kdrv_sie_hdl[id]->ccir_info.data_idx, kdrv_sie_hdl[id]->ccir_info.auto_align,
						kdrv_sie_hdl[id]->ccir_info.filed_enable, kdrv_sie_hdl[id]->ccir_info.filed_sel);
				} else {
					dump("%-4dNot CCIR fmt\r\n", id);
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N.S.");
			}
		}
	}

	/***** PAT Gen Info **********************************************************/
	{
		CHAR *str_pat_gen_mode[6] = {
			"N.S.",
			"COLORBAR",
			"RANDOM",
			"FIXED",
			"HINCREASE",
			"HVINCREASE",
		};

		dump("\r\n-----pat gen-----\r\n");
		dump("%-4s%-12s%-10s%-8s%-8s\r\n", "id", "mode", "value", "src_w", "src_h");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_PATGEN) {
				if (kdrv_sie_hdl[id]->open_cfg.act_mode == KDRV_SIE_IN_PATGEN) {
					dump("%-4d%-12s%#-10x%-8d%-8d\r\n", (int)id, str_pat_gen_mode[kdrv_sie_hdl[id]->pat_gen_info.mode],
						(unsigned int)kdrv_sie_hdl[id]->pat_gen_info.val, (int)kdrv_sie_hdl[id]->pat_gen_info.src_win.w, (int)kdrv_sie_hdl[id]->pat_gen_info.src_win.h);
				} else {
					dump("%-4dNot PatGen Mode\r\n", (int)id);
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N.S.");
			}
		}
	}

	/***** Output Info ********************************************************/
	{
		CHAR *str_dest[3] = {
			"DIRECT",
			"DRAM",
			"BOTH",
		};

		CHAR *str_out_fmt[7] = {
			"BAYER_8",
			"BAYER_10",
			"BAYER_12",
			"BAYER_16",
			"YUV_422_SPT",
			"YUV_422_NOSPT",
			"YUV_420_SPT",
		};

		CHAR *out_mode_sel[2] = {
			"NORMAL",
			"SINGLE",
		};

		dump("\r\n-----output-----\r\n");
		dump("%-4s%-16s%-8s%-16s%-8s%-10s%-10s%-23s%-10s%-10s\r\n", "id", "scl_size(w, h)", "flip", "out_fmt", "enc_en", "enc_rate", "out_dest", "out_mode(ch0,ch1,ch2)", "r_buf_en", "r_buf_len");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (kdrv_sie_typecast_raw(kdrv_sie_hdl[id]->data_fmt)) {
				dump("%-4d(%4d, %4d)    %-8d%-16s%-8d%-10d%-10s(%-7s%-7s%-7s)%-10d%-10d\r\n", id, kdrv_sie_hdl[id]->scl_size.w, kdrv_sie_hdl[id]->scl_size.h,
					  kdrv_sie_hdl[id]->flip, str_out_fmt[kdrv_sie_hdl[id]->data_fmt], kdrv_sie_hdl[id]->encode_info.enable, kdrv_sie_hdl[id]->encode_info.enc_rate,
					   str_dest[kdrv_sie_hdl[id]->out_dest], out_mode_sel[kdrv_sie_hdl[id]->ch_output_mode.out0_mode], out_mode_sel[kdrv_sie_hdl[id]->ch_output_mode.out1_mode], out_mode_sel[kdrv_sie_hdl[id]->ch_output_mode.out2_mode], kdrv_sie_hdl[id]->ring_buf_info.enable, kdrv_sie_hdl[id]->ring_buf_info.ring_buf_len);
			} else if (kdrv_sie_typecast_ccir(kdrv_sie_hdl[id]->data_fmt)) {
				dump("%-4d%-16s%-8d%-16s%-8s%-10s%-10s(%-7s%-7s%-7s)%-10s%-10s\r\n", id, "N/A", kdrv_sie_hdl[id]->flip, str_out_fmt[kdrv_sie_hdl[id]->data_fmt], "N/A", "N/A",
					   "N/A", out_mode_sel[kdrv_sie_hdl[id]->ch_output_mode.out0_mode], out_mode_sel[kdrv_sie_hdl[id]->ch_output_mode.out1_mode], out_mode_sel[kdrv_sie_hdl[id]->ch_output_mode.out2_mode], "N/A", "N/A");
			} else {
				dump("%-4d, unknown datafmt %d\r\n", id, kdrv_sie_hdl[id]->data_fmt);
			}
		}
	}

	{
		UINT32 i;

		dump("\r\n-----channel-----\r\n");
		dump("%-4s%-4s%-12s%-8s\r\n", "id", "ch", "addr", "lof");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			for (i = KDRV_SIE_CH0; i <= sie_dbg_limit[id].out_max_ch; i++) {
				dump("%-4d%-4d0x%-10x%-8d\r\n", id, i, kdrv_sie_hdl[id]->out_ch_addr[i], kdrv_sie_hdl[id]->out_ch_lof[i]);
			}
		}
	}

	/***** OB Info ************************************************/
	{
		dump("\r\n-----ob-----\r\n");
		dump("%-4s%-8s%-16s\r\n", "id", "ofs", "bypass");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (kdrv_sie_typecast_raw(kdrv_sie_hdl[id]->data_fmt)) {
				if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_OB_BYPASS) {
					dump("%-4d%-8d%-4d\r\n", id, kdrv_sie_hdl[id]->ob_param.ob_ofs, kdrv_sie_hdl[id]->ob_param.bypass_enable);
				} else {
					dump("%-4d%-4s\r\n", id, "N.S.");
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	/***** Gain Info ************************************************************/
	{
		dump("\r\n-----gain-----\r\n");
		dump("%-4s%-12s%-8s%-12s%-16s%-28s\r\n", "id", "dgain_en", "dgain", "cgain_en", "cg_37_fmt_sel", "cg(r, gr, gb, g)");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (kdrv_sie_typecast_raw(kdrv_sie_hdl[id]->data_fmt)) {
				if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_CGAIN || sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_DGAIN) {
					dump("%-4d%-12d%-8d%-12d%-16d(%4d, %4d, %4d, %4d)\r\n", id,
						kdrv_sie_hdl[id]->dgain.enable, kdrv_sie_hdl[id]->dgain.gain,
						kdrv_sie_hdl[id]->cgain.enable, kdrv_sie_hdl[id]->cgain.sel_37_fmt,
						kdrv_sie_hdl[id]->cgain.r_gain, kdrv_sie_hdl[id]->cgain.gr_gain, kdrv_sie_hdl[id]->cgain.gb_gain, kdrv_sie_hdl[id]->cgain.b_gain);
				} else {
					dump("%-4d%-4s\r\n", id, "N.S.");
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	/***** CA Info ************************************************************/
	{
		dump("\r\n-----ca-----\r\n");
		dump("%-4s%-4s%-16s%-28s%-8s%-28s%-28s%-10s%-20s%-8s%-8s\r\n", "id", "en", "win_num(w, h)", "roi(x, y, w, h)", "th_en", "thr_lower(r, g, b, p)", "thr_upper(r, g, b, p)", "irsub_en", "irsub_w(r, g, b)", "ob", "ca_src");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (kdrv_sie_typecast_raw(kdrv_sie_hdl[id]->data_fmt)) {
				if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_CA) {
					dump("%-4d%-4d(%4d, %4d)    (%4d, %4d, %4d, %4d)    %-8d(%4d, %4d, %4d, %4d)    (%4d, %4d, %4d, %4d)    %-10s(%4d, %4d, %4d)  %-8d%-8d\r\n", id,
						kdrv_sie_hdl[id]->ca_param.enable, kdrv_sie_hdl[id]->ca_param.win_num.w, kdrv_sie_hdl[id]->ca_param.win_num.h,
						kdrv_sie_hdl[id]->ca_roi.roi.x, kdrv_sie_hdl[id]->ca_roi.roi.y, kdrv_sie_hdl[id]->ca_roi.roi.w, kdrv_sie_hdl[id]->ca_roi.roi.h,
						kdrv_sie_hdl[id]->ca_param.th_enable, kdrv_sie_hdl[id]->ca_param.r_th_l, kdrv_sie_hdl[id]->ca_param.g_th_l, kdrv_sie_hdl[id]->ca_param.b_th_l, kdrv_sie_hdl[id]->ca_param.p_th_l,
						kdrv_sie_hdl[id]->ca_param.r_th_u, kdrv_sie_hdl[id]->ca_param.g_th_u, kdrv_sie_hdl[id]->ca_param.b_th_u, kdrv_sie_hdl[id]->ca_param.p_th_u, "N/A",
						kdrv_sie_hdl[id]->ca_param.irsub_r_weight, kdrv_sie_hdl[id]->ca_param.irsub_g_weight, kdrv_sie_hdl[id]->ca_param.irsub_b_weight, kdrv_sie_hdl[id]->ca_param.ca_ob_ofs, kdrv_sie_hdl[id]->ca_param.ca_src);
				} else {
					dump("%-4d%-4s\r\n", id, "N.S.");
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	/***** LA Info ************************************************************/
	{
		CHAR* str_lasrc[2] = {
			"POST_CG",
			"PRE_CG"
		};
		CHAR* str_histsrc[2] = {
			"POST_GMA",
			"PRE_GMA"
		};

		dump("\r\n-----la-----\r\n");
		dump("%-4s%-4s%-16s%-28s%-12s%-8s%-22s%-8s%-12s%-10s%-8s\r\n", "id", "en", "win_num(w, h)", "roi(x, y, w, h)", "src", "cg_en", "cg(r, g, b)", "gma_en", "hist_en", "hist_src", "ob");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (kdrv_sie_typecast_raw(kdrv_sie_hdl[id]->data_fmt)) {
				if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_LA) {
					dump("%-4d%-4d(%4d, %4d)    (%4d, %4d, %4d, %4d)    %-12s%-8d(%4d, %4d, %4d)    %-8d%-12d%-10s%-8d\r\n", id,
						kdrv_sie_hdl[id]->la_param.enable, kdrv_sie_hdl[id]->la_param.win_num.w, kdrv_sie_hdl[id]->la_param.win_num.h,
						kdrv_sie_hdl[id]->la_roi.roi.x, kdrv_sie_hdl[id]->la_roi.roi.y, kdrv_sie_hdl[id]->la_roi.roi.w, kdrv_sie_hdl[id]->la_roi.roi.h,
						str_lasrc[kdrv_sie_hdl[id]->la_param.la_src], kdrv_sie_hdl[id]->la_param.cg_enable, kdrv_sie_hdl[id]->la_param.r_gain, kdrv_sie_hdl[id]->la_param.g_gain, kdrv_sie_hdl[id]->la_param.b_gain,
						kdrv_sie_hdl[id]->la_param.gamma_enable, kdrv_sie_hdl[id]->la_param.histogram_enable, str_histsrc[kdrv_sie_hdl[id]->la_param.histogram_src], kdrv_sie_hdl[id]->la_param.la_ob_ofs);
				} else {
					dump("%-4d%-4s\r\n", id, "N.S.");
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	/***** VA Info ************************************************************/
#if defined (_BSP_NA51000_)
	{
		UINT32 i;

		dump("\r\n-----va-----\r\n");
		dump("%-4s%-4s%-12s%-28s%-16s%-16s%-8s%-28s\r\n", "id", "en", "out_grp_1_2", "roi(x, y, w, h)", "win_num(w, h)", "win_size(w, h)", "cg_en", "cg(r, gr, gb, b)");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (kdrv_sie_typecast_raw(kdrv_sie_hdl[id]->data_fmt)) {
				if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_VA) {
					dump("%-4d%-4d%-12d(%4d, %4d, %4d, %4d)    (%4d, %4d)    (%4d, %4d)    %-8d(%4d, %4d, %4d, %4d)\r\n", id,
						kdrv_sie_hdl[id]->va_param.enable, kdrv_sie_hdl[id]->va_param.va_out_grp1_2,
						kdrv_sie_hdl[id]->va_eth_roi.roi.x, kdrv_sie_hdl[id]->va_eth_roi.roi.y, kdrv_sie_hdl[id]->va_eth_roi.roi.w, kdrv_sie_hdl[id]->va_eth_roi.roi.h,
						kdrv_sie_hdl[id]->va_param.win_num.w, kdrv_sie_hdl[id]->va_param.win_num.h,
						kdrv_sie_hdl[id]->va_win_sz.win_size.w, kdrv_sie_hdl[id]->va_win_sz.win_size.h,
						kdrv_sie_hdl[id]->va_param.cg_enable, kdrv_sie_hdl[id]->va_param.gain[0], kdrv_sie_hdl[id]->va_param.gain[1], kdrv_sie_hdl[id]->va_param.gain[2], kdrv_sie_hdl[id]->va_param.gain[3]);
				} else {
					dump("%-4d%-4s\r\n", id, "N.S.");
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}

		dump("\r\n-----indep va win-----\r\n");
		dump("%-4s%-8s%-4s%-12s%-12s%-28s\r\n", "id", "win_idx", "en", "linemax_g1", "linemax_g2", "roi(x, y, w, h)");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (kdrv_sie_typecast_raw(kdrv_sie_hdl[id]->data_fmt)) {
				if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_VA) {
					for (i = 0; i < KDRV_SIE_VA_INDEP_WIN_MAX; i++) {
						dump("%-4d%-8d%-4d%-12d%-12d(%4d, %4d, %4d, %4d)    \r\n", id, i, kdrv_sie_hdl[id]->va_param.indep_win[i].enable,
							kdrv_sie_hdl[id]->va_param.indep_win[i].linemax_g1, kdrv_sie_hdl[id]->va_param.indep_win[i].linemax_g2,
							kdrv_sie_hdl[id]->va_win_sz.indep_roi[i].x, kdrv_sie_hdl[id]->va_win_sz.indep_roi[i].y, kdrv_sie_hdl[id]->va_win_sz.indep_roi[i].w, kdrv_sie_hdl[id]->va_win_sz.indep_roi[i].h);
					}
				} else {
					dump("%-4d%-4s\r\n", id, "N.S.");
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}
#endif
	/***** Eth Info ************************************************************/
#if defined (_BSP_NA51000_)
	{
		UINT32 bit_number[2] = {2, 8};

		dump("\r\n-----eth-----\r\n");
		dump("***note that if va enable, eth_roi, eth_cg is decided by va_param\r\n");
		dump("%-4s%-4s%-8s%-8s%-8s%-8s%-28s\r\n", "id", "en", "bit_num", "out_sel", "h_sel", "v_sel", "roi(x, y, w, h)");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (kdrv_sie_typecast_raw(kdrv_sie_hdl[id]->data_fmt)) {
				if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_ETH) {
					dump("%-4d%-4d%-8d%-8d%-8d%-8d(%4d, %4d, %4d, %4d) \r\n", id, kdrv_sie_hdl[id]->eth_info.enable,
						bit_number[kdrv_sie_hdl[id]->eth_info.out_bit_sel], kdrv_sie_hdl[id]->eth_info.out_sel, kdrv_sie_hdl[id]->eth_info.h_out_sel, kdrv_sie_hdl[id]->eth_info.v_out_sel,
						kdrv_sie_hdl[id]->va_eth_roi.roi.x, kdrv_sie_hdl[id]->va_eth_roi.roi.y, kdrv_sie_hdl[id]->va_eth_roi.roi.w, kdrv_sie_hdl[id]->va_eth_roi.roi.h);
				} else {
					dump("%-4d%-4s\r\n", id, "N.S.");
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}
#endif
	/***** DPC Info ************************************************************/
	{
		CHAR* str_dpc_weight[4] = {
			"50%",
			"25%",
			"12.25%",
			"6.125%",
		};
		dump("\r\n-----dpc-----\r\n");
		dump("%-4s%-4s%-12s\r\n", "id", "en", "weight");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (kdrv_sie_typecast_raw(kdrv_sie_hdl[id]->data_fmt)) {
				if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_DPC) {
					dump("%-4d%-4d%-12s\r\n", id, kdrv_sie_hdl[id]->dpc_info.enable, str_dpc_weight[kdrv_sie_hdl[id]->dpc_info.weight]);
				} else {
					dump("%-4d%-4s\r\n", id, "N.S.");
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	/***** ECS Info ************************************************************/
	{
		CHAR* str_map_sel[3] = {
			"65x65",
			"49x49",
			"33x33"
		};
		dump("\r\n-----ecs-----\r\n");
		dump("%-4s%-4s%-10s%-12s%-10s%-12s%-10s%-12s\r\n", "id", "en", "map_sel", "37_fmt_sel", "dthr_en", "dthr_reset", "dthr_lv", "bayer_mode");
		for (id = KDRV_SIE_ID_1; id <= sie_dbg_limit[KDRV_SIE_ID_1].max_spt_id; id++) {
			if (sie_status[id] == KDRV_SIE_STS_CLOSE || kdrv_sie_hdl[id] == NULL) {
				continue;
			}

			if (kdrv_sie_typecast_raw(kdrv_sie_hdl[id]->data_fmt)) {
				if (sie_dbg_limit[id].support_func & KDRV_SIE_FUNC_SPT_ECS) {
					dump("%-4d%-4x%-10s%-12d%-10d%-12d%-10d%-12d\r\n", id, kdrv_sie_hdl[id]->ecs_info.enable,
						str_map_sel[kdrv_sie_hdl[id]->ecs_info.map_sel], kdrv_sie_hdl[id]->ecs_info.sel_37_fmt, kdrv_sie_hdl[id]->ecs_info.dthr_enable,
						kdrv_sie_hdl[id]->ecs_info.dthr_reset, kdrv_sie_hdl[id]->ecs_info.dthr_level, kdrv_sie_hdl[id]->ecs_info.bayer_mode);
				} else {
					dump("%-4d%-4s\r\n", id, "N.S.");
				}
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	dump("\r\n----------[KDRV SIE] Dump End----------\r\n\r\n");
}

void kdrv_sie_dbg_init(KDRV_SIE_PROC_ID id)
{
	UINT32 i;

	if (id >= KDRV_SIE_MAX_ENG) {
		return;
	}

	for (i = 0; i < KDRV_SIE_DBG_CNT_MAX; i++) {
		atomic_set(&kdrv_sie_dbg_info[id].cnt[i][KDRV_SIE_DBG_CNT_TYPE_CURR], 0);
	}

	if (kdrv_sie_dbg_info[id].init == 0) {
		kdrv_sie_dbg_info[id].init = 1;
		kdrv_sie_dbg_info[id].err_log_rate = KDRV_SIE_DFT_LOG_RATE;
		for (i = 0; i < KDRV_SIE_DBG_CNT_MAX; i++) {
			atomic_set(&kdrv_sie_dbg_info[id].cnt[i][KDRV_SIE_DBG_CNT_TYPE_ACCU], 0);
		}
	}
}

void kdrv_sie_dbg_uninit(KDRV_SIE_PROC_ID id)
{
}

KDRV_SIE_DBG_MSG kdrv_sie_dbg_msg[KDRV_SIE_DBG_CNT_MAX] = {
	{KDRV_SIE_DBG_CNT_VD				, "vd"},
	{KDRV_SIE_DBG_CNT_ERR_DRMIN1_UDFL	, "drmin1_udfl"},
	{KDRV_SIE_DBG_CNT_ERR_DRMIN2_UDFL	, "drmin2_udfl"},
	{KDRV_SIE_DBG_CNT_ERR_DRMOUT0_OVFL	, "drmout0_ovfl"},
	{KDRV_SIE_DBG_CNT_ERR_DRMOUT1_OVFL	, "drmout1_ovfl"},
	{KDRV_SIE_DBG_CNT_ERR_DRMOUT2_OVFL	, "drmout2_ovfl"},
	{KDRV_SIE_DBG_CNT_ERR_DPC			, "dpc_err"},
	{KDRV_SIE_DBG_CNT_ERR_RAWENC		, "rawenc_err"},
	{KDRV_SIE_DBG_CNT_ERR_SIECLK		, "sieclk_err"},
};

// update interrupt status
void kdrv_sie_upd_vd_intrpt_sts(KDRV_SIE_PROC_ID id, SIE_ENGINE_STATUS_INFO_CB *info)
{
	KDRV_SIE_DBG_CNT_SEL dbg_cnt_sel;

	if (id >= KDRV_SIE_MAX_ENG) {
		return;
	}

	atomic_inc(&kdrv_sie_dbg_info[id].cnt[KDRV_SIE_DBG_CNT_VD][KDRV_SIE_DBG_CNT_TYPE_CURR]);
	atomic_inc(&kdrv_sie_dbg_info[id].cnt[KDRV_SIE_DBG_CNT_VD][KDRV_SIE_DBG_CNT_TYPE_ACCU]);

	if (info->bDramIn1Udfl) {
		dbg_cnt_sel = KDRV_SIE_DBG_CNT_ERR_DRMIN1_UDFL;
		if ((atomic_read(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]) % kdrv_sie_dbg_info[id].err_log_rate) == 0) {
			kdrv_sie_dbg_err("id %d %s (log rate %d)\r\n", id, kdrv_sie_dbg_msg[dbg_cnt_sel].msg, kdrv_sie_dbg_info[id].err_log_rate);
		}
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]);
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_ACCU]);
	}

	if (info->bDramIn2Udfl) {
		dbg_cnt_sel = KDRV_SIE_DBG_CNT_ERR_DRMIN2_UDFL;
		if ((atomic_read(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]) % kdrv_sie_dbg_info[id].err_log_rate) == 0) {
			kdrv_sie_dbg_err("id %d %s (log rate %d)\r\n", id, kdrv_sie_dbg_msg[dbg_cnt_sel].msg, kdrv_sie_dbg_info[id].err_log_rate);
		}
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]);
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_ACCU]);
	}

	if (info->bDramOut0Ovfl) {
		dbg_cnt_sel = KDRV_SIE_DBG_CNT_ERR_DRMOUT0_OVFL;
		if ((atomic_read(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]) % kdrv_sie_dbg_info[id].err_log_rate) == 0) {
			kdrv_sie_dbg_err("id %d %s (log rate %d)\r\n", id, kdrv_sie_dbg_msg[dbg_cnt_sel].msg, kdrv_sie_dbg_info[id].err_log_rate);
		}
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]);
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_ACCU]);
	}

	if (info->bDramOut1Ovfl) {
		dbg_cnt_sel = KDRV_SIE_DBG_CNT_ERR_DRMOUT1_OVFL;
		if ((atomic_read(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]) % kdrv_sie_dbg_info[id].err_log_rate) == 0) {
			kdrv_sie_dbg_err("id %d %s (log rate %d)\r\n", id, kdrv_sie_dbg_msg[dbg_cnt_sel].msg, kdrv_sie_dbg_info[id].err_log_rate);
		}
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]);
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_ACCU]);
	}

	if (info->bDramOut2Ovfl) {
		dbg_cnt_sel = KDRV_SIE_DBG_CNT_ERR_DRMOUT2_OVFL;
		if ((atomic_read(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]) % kdrv_sie_dbg_info[id].err_log_rate) == 0) {
			kdrv_sie_dbg_err("id %d %s (log rate %d)\r\n", id, kdrv_sie_dbg_msg[dbg_cnt_sel].msg, kdrv_sie_dbg_info[id].err_log_rate);
		}
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]);
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_ACCU]);
	}
}

void kdrv_sie_upd_intrpt_err_sts(KDRV_SIE_PROC_ID id, UINT32 status)
{
	KDRV_SIE_DBG_CNT_SEL dbg_cnt_sel;

	if (status & KDRV_SIE_INT_ERR_SIECLK) {
		dbg_cnt_sel = KDRV_SIE_DBG_CNT_ERR_SIECLK;
		if ((atomic_read(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]) % kdrv_sie_dbg_info[id].err_log_rate) == 0) {
			kdrv_sie_dbg_err("id %d %s (log rate %d)\r\n", id, kdrv_sie_dbg_msg[dbg_cnt_sel].msg, kdrv_sie_dbg_info[id].err_log_rate);
		}
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]);
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_ACCU]);
	}

	if (status & KDRV_SIE_INT_ERR_RAWENC) {
		dbg_cnt_sel = KDRV_SIE_DBG_CNT_ERR_RAWENC;
		if ((atomic_read(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]) % kdrv_sie_dbg_info[id].err_log_rate) == 0) {
			kdrv_sie_dbg_err("id %d %s (log rate %d)\r\n", id, kdrv_sie_dbg_msg[dbg_cnt_sel].msg, kdrv_sie_dbg_info[id].err_log_rate);
		}
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]);
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_ACCU]);
	}

	if (status & KDRV_SIE_INT_ERR_DPC) {
		dbg_cnt_sel = KDRV_SIE_DBG_CNT_ERR_DPC;
		if ((atomic_read(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]) % kdrv_sie_dbg_info[id].err_log_rate) == 0) {
			kdrv_sie_dbg_err("id %d %s (log rate %d)\r\n", id, kdrv_sie_dbg_msg[dbg_cnt_sel].msg, kdrv_sie_dbg_info[id].err_log_rate);
		}
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_CURR]);
		atomic_inc(&kdrv_sie_dbg_info[id].cnt[dbg_cnt_sel][KDRV_SIE_DBG_CNT_TYPE_ACCU]);
	}
}