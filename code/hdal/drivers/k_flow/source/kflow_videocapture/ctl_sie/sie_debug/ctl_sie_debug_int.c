#include "ctl_sie_debug_int.h"
#include "ctl_sie_event_int.h"
#include "ctl_sie_isp_int.h"
#include "ctl_sie_id_int.h"
#include "kwrap/file.h"
#if defined (__FREERTOS)
#include <stdio.h>
#endif

static BOOL ctl_sie_dbg_buf_ioctl_dump = FALSE;	//ste flag = TRUE when dump isr ioctl
static UINT32 ctl_sie_dbg_ts_dumpflg = 0;
static UINT32 ctl_sie_dbg_proc_t_cnt;
CTL_SIE_DBG_INFO *ctl_sie_dbg_info[CTL_SIE_MAX_SUPPORT_ID] = {NULL};
CTL_SIE_DBG_CTRL_INFO ctl_sie_dbg_ctl_info[CTL_SIE_MAX_SUPPORT_ID] = {0};
CTL_SIE_FRM_CTL_INFO ctl_sie_dbg_frm_ctrl[CTL_SIE_MAX_SUPPORT_ID] = {0};
CTL_SIE_PROC_TIME_INFO ctl_sie_dbg_proc_t_info[CTL_SIE_DBG_TS_MAXNUM] = {0};

//ISP
static CTL_SIE_DBG_ISP_CB_T_LOG		ctl_sie_isp_dbg_cbtime_log[CTL_SIE_DBG_ISP_CB_T_DBG_NUM];
static UINT32 						ctl_sie_isp_dbg_cbtime_cnt;
static BOOL 						ctl_sie_isp_dbg_cb_dump = FALSE;

int ctl_sie_int_dbg_printf(const char *fmtstr, ...)
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

#if 0
#endif
CTL_SIE_INT_DBG_TAB sie_dbg_tab = {
	ctl_sie_dbg_chk_msg_type,
	ctl_sie_dbg_query_buf,
	ctl_sie_dbg_set_buf,
	ctl_sie_dbg_set_ts,
	ctl_sie_dbg_reset_ts,
	ctl_sie_dbg_dump_buf_io,
#if CTL_SIE_DBG_DUMP_IO_BUF
	ctl_sie_dbg_set_isr_ioctl,
#else
	NULL,
#endif
	ctl_sie_dbg_set_frm_ctrl_info,
	ctl_sie_dbg_set_isp_cb_t_log,
	ctl_sie_dbg_isp_cb_t_dump,
	ctl_sie_dbg_set_proc_t,
	ctl_sie_dbg_get_buf_wp_info,
};

BOOL ctl_sie_dbg_chk_msg_type(CTL_SIE_ID id, UINT32 chk_type)
{
	return (ctl_sie_dbg_ctl_info[id].dbg_msg_type & ((UINT32)(1 << chk_type)));
}

void ctl_sie_dbg_get_buf_wp_info(CTL_SIE_ID id, UINT32 *ddr_id, UINT32 *dis_wp)
{
	if (ddr_id != NULL && dis_wp != NULL) {
		*dis_wp = CTL_SIE_DBG_GET_WP_RLS_EN(ctl_sie_dbg_ctl_info[id].dbg_msg_type);
		*ddr_id = CTL_SIE_DBG_GET_DDR_ID(ctl_sie_dbg_ctl_info[id].dbg_msg_type);
	} else {
		ctl_sie_dbg_err("id %d: dbg info NULL\r\n", (int)(id));
	}
}

void ctl_sie_dbg_set_proc_t(CTL_SIE_ID id, CHAR *proc_name, CTL_SIE_PROC_TIME_ITEM item)
{
	UINT32 proc_t_cnt = ctl_sie_dbg_proc_t_cnt;

	ctl_sie_dbg_proc_t_cnt = (ctl_sie_dbg_proc_t_cnt + 1) % CTL_SIE_DBG_TS_MAXNUM;
	memcpy((void *)&ctl_sie_dbg_proc_t_info[proc_t_cnt].proc_name[0], (void *)proc_name, sizeof(ctl_sie_dbg_proc_t_info[proc_t_cnt].proc_name));
	ctl_sie_dbg_proc_t_info[proc_t_cnt].item = item;
	ctl_sie_dbg_proc_t_info[proc_t_cnt].id = id;
	ctl_sie_dbg_proc_t_info[proc_t_cnt].time_us = ctl_sie_util_get_timestamp();
}

void ctl_sie_dbg_set_msg_type(CTL_SIE_ID id, CTL_SIE_DBG_MSG_TYPE type, UINT32 par1, UINT32 par2, UINT32 par3)
{
	BOOL type_en = FALSE;

	if (!ctl_sie_module_chk_id_valid(id)) {
		return;
	}

	if (type >= CTL_SIE_DBG_MSG_MAX) {
		DBG_DUMP("CTL_SIE_ID_%d dbg type %d overflow\r\n", (int)(id+1), (int)(type));
		return;
	}

	switch (type) {
	case CTL_SIE_DBG_MSG_OFF:
		DBG_DUMP("CTL_SIE_ID_%d CTL_SIE_DBG_MSG_OFF\r\n", (int)id + 1);
		ctl_sie_dbg_ctl_info[id].dbg_msg_type = 0;
		break;

	case CTL_SIE_DBG_MSG_CTL_INFO:
		if (ctl_sie_dbg_info[id] == NULL) {
			break;
		}
		ctl_sie_dbg_dump_info(NULL);
		break;

	case CTL_SIE_DBG_MSG_PROC_TIME:
		ctl_sie_dbg_dump_ts(id, NULL);
		break;

	case CTL_SIE_DBG_MSG_ALL:
		if (ctl_sie_dbg_info[id] == NULL) {
			break;
		}
		ctl_sie_dbg_dump_ts(id, NULL);
		ctl_sie_dbg_dump_info(NULL);
		break;

	case CTL_SIE_DBG_MSG_BUF_IO_LITE:
	case CTL_SIE_DBG_MSG_BUF_IO_FULL:
		ctl_sie_dbg_ctl_info[id].dbg_bufio_cnt = par1;
		type_en = TRUE;
		break;

	case CTL_SIE_DBG_MSG_ISR_IOCTL:
		if (par1 == 0) {    //disable rec.
			ctl_sie_dbg_ind("Disable CTL_SIE_ID_%d isr log record\r\n", (int)id + 1);
		} else if (par1 == 1) { //start rec.
			ctl_sie_dbg_ind("Enable CTL_SIE_ID_%d isr log record\r\n", (int)id + 1);
			type_en = TRUE;
		} else {    //print result
			ctl_sie_dbg_dump_isr_ioctl(NULL);
		}
		break;

	case CTL_SIE_DBG_MSG_RING_BUF_FULL:
		if (par1 == 0) {
			DBG_DUMP("Disable CTL_SIE_ID_%d max ring buffer mode\r\n", (int)id + 1);
		} else {
			DBG_DUMP("Enable CTL_SIE_ID_%d max ring buffer mode\r\n", (int)id + 1);
			type_en = TRUE;
		}
		break;

	case CTL_SIE_DBG_MSG_REG_DBG_CB:
		ctl_sie_reg_dbg_tab(&sie_dbg_tab);
		break;

	case CTL_SIE_DBG_MSG_BUF_DMA_WP:
		if (par1 == 0) {
			DBG_DUMP("DISABLE CTL_SIE_ID_%d out_ch0_buf dma wp\r\n", (int)id + 1);
			CTL_SIE_DBG_SET_DBG_TYPE(&ctl_sie_dbg_ctl_info[id].dbg_msg_type, CTL_SIE_DBG_DDR_ID_MASK, CTL_SIE_DBG_DDR_ID_OFS, FALSE);
			CTL_SIE_DBG_SET_DBG_TYPE(&ctl_sie_dbg_ctl_info[id].dbg_msg_type, CTL_SIE_DBG_WP_RLS_MASK, CTL_SIE_DBG_WP_RLS_OFS, FALSE);
		} else {
			DBG_DUMP("ENABLE CTL_SIE_ID_%d out_ch0_buf dma wp for DDR_ARB_%d, DIS_WP before Push: %d\r\n", (int)id + 1, (int)par2 + 1, (int)par3);
			CTL_SIE_DBG_SET_DBG_TYPE(&ctl_sie_dbg_ctl_info[id].dbg_msg_type, CTL_SIE_DBG_DDR_ID_MASK, CTL_SIE_DBG_DDR_ID_OFS, FALSE);
			CTL_SIE_DBG_SET_DBG_TYPE(&ctl_sie_dbg_ctl_info[id].dbg_msg_type, CTL_SIE_DBG_WP_RLS_MASK, CTL_SIE_DBG_WP_RLS_OFS, FALSE);
			CTL_SIE_DBG_SET_DBG_TYPE(&ctl_sie_dbg_ctl_info[id].dbg_msg_type, par2, CTL_SIE_DBG_DDR_ID_OFS, TRUE);
			CTL_SIE_DBG_SET_DBG_TYPE(&ctl_sie_dbg_ctl_info[id].dbg_msg_type, par3, CTL_SIE_DBG_WP_RLS_OFS, TRUE);
			type_en = TRUE;
		}
		break;

	case CTL_SIE_DBG_MSG_BUF_CHK:
		if (par1 == 0) {
			DBG_DUMP("Disable CTL_SIE_ID_%d out_ch0_buf flag check\r\n", (int)id + 1);
		} else {
			DBG_DUMP("Enable CTL_SIE_ID_%d out_ch0_buf flag check\r\n", (int)id + 1);
			type_en = TRUE;
		}
		break;

	case CTL_SIE_DBG_MSG_FORCE_PATGEN:
		DBG_DUMP("Set CTL_SIE_ID_%d force pattern gen\r\n", (int)id + 1);
		break;

	case CTL_SIE_DBG_MSG_PSR:
		if (par1 == 0) {
			DBG_DUMP("Disable CTL_SIE_ID_%d PSR log\r\n", (int)id + 1);
		} else {
			DBG_DUMP("Enable CTL_SIE_ID_%d PSR log\r\n", (int)id + 1);
			type_en = TRUE;
		}
		break;

	default:
		return;
	}

	CTL_SIE_DBG_SET_DBG_TYPE(&ctl_sie_dbg_ctl_info[id].dbg_msg_type, 1, type, type_en);
	ctl_sie_dbg_ind("CTL_SIE_ID_%d, dbg_type 0x%x, par1: %d, par2: %d\r\n", (int)(id + 1), (unsigned int)(ctl_sie_dbg_ctl_info[id].dbg_msg_type), (int)par1, (int)par2);
}

void ctl_sie_dbg_set_frm_ctrl_info(CTL_SIE_ID id, CTL_SIE_FRM_CTL_INFO *frm_ctrl)
{
	memcpy((void *)&ctl_sie_dbg_frm_ctrl[id], (void *)frm_ctrl, sizeof(CTL_SIE_FRM_CTL_INFO));
}

UINT32 ctl_sie_dbg_query_buf(void)
{
	return ALIGN_CEIL(sizeof(CTL_SIE_DBG_INFO), VOS_ALIGN_BYTES);
}

void ctl_sie_dbg_set_buf(UINT32 id, UINT32 buf_addr)
{
	if (buf_addr == 0) {
		ctl_sie_dbg_info[id] = NULL;
	} else {
		ctl_sie_dbg_info[id] = (CTL_SIE_DBG_INFO *)buf_addr;
		memset((void *)ctl_sie_dbg_info[id], 0, ALIGN_CEIL(sizeof(CTL_SIE_DBG_INFO), VOS_ALIGN_BYTES));
	}
}

void ctl_sie_dbg_set_ts(CTL_SIE_ID id, CTL_SIE_DBG_TS_EVT evt, UINT64 sie_fc)
{
	UINT32 idx;
	CTL_SIE_DBG_TS_INFO *p_ts;

	if (ctl_sie_dbg_info[id] == NULL) {
		return;
	}

	idx = ctl_sie_dbg_ctl_info[id].dbg_ts_cnt % CTL_SIE_DBG_TS_MAXNUM;
	p_ts = &ctl_sie_dbg_info[id]->dbg_ts_info[idx];

	if ((ctl_sie_dbg_ts_dumpflg & (1 << id)) || (evt >= CTL_SIE_DBG_TS_MAX)) {
		return;
	}

	p_ts->t = ctl_sie_util_get_timestamp();
	p_ts->fc = sie_fc;
	p_ts->evt = evt;
	p_ts->evtcnt = ctl_sie_dbg_ctl_info[id].dbg_ts_cnt;
	ctl_sie_dbg_ctl_info[id].dbg_ts_cnt++;
}

void ctl_sie_dbg_set_isr_ioctl(CTL_SIE_ID id, UINT32 status, UINT32 buf_ctl_type, CTL_SIE_HEAD_IDX head_idx)
{
#if CTL_SIE_DBG_DUMP_IO_BUF
	UINT32 idx;
	CTL_SIE_HDL *hdl[CTL_SIE_MAX_SUPPORT_ID];

	if (ctl_sie_dbg_info[id] == NULL) {
		return;
	}

	if (ctl_sie_dbg_buf_ioctl_dump) {
		return;
	}

	if (ctl_sie_dbg_chk_msg_type(id, CTL_SIE_DBG_MSG_ISR_IOCTL)) {
		hdl[id] = ctl_sie_get_hdl(id);
		idx = ctl_sie_dbg_ctl_info[id].dbg_isr_ioctl_cnt % CTL_SIE_DBG_ISR_IOCTL_NUM;
		ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].sie_id = id;
		ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].isr_evt = status;
		ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].buf_ctl_type = buf_ctl_type;
		ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].buf_addr = hdl[id]->ctrl.head_info[head_idx].buf_addr;
		ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].buf_id = hdl[id]->ctrl.head_info[head_idx].buf_id;
		ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].ch0_addr = hdl[id]->ctrl.head_info[head_idx].vdo_frm.addr[0];
		if (status == CTL_SIE_INTE_BP3) {
			ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].sie_out_size.w = hdl[id]->param.io_size_info.size_info.sie_scl_out.w;
			ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].sie_out_size.h = hdl[id]->param.io_size_info.size_info.sie_scl_out.h;
			ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].ch0_lofs = hdl[id]->ctrl.rtc_obj.out_ch_lof[0];
		} else {
			ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].sie_out_size.w = hdl[id]->ctrl.head_info[head_idx].vdo_frm.size.w;
			ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].sie_out_size.h = hdl[id]->ctrl.head_info[head_idx].vdo_frm.size.h;
			ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].ch0_lofs = hdl[id]->ctrl.head_info[head_idx].vdo_frm.loff[0];
		}

		ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].frm_cnt = hdl[id]->ctrl.head_info[head_idx].vdo_frm.count;
		ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].ts = ctl_sie_util_get_timestamp();
		ctl_sie_dbg_ctl_info[id].dbg_isr_ioctl_cnt++;
	}
#endif
}

void ctl_sie_dbg_reset_ts(CTL_SIE_ID id)
{
	UINT32 i;

	if (ctl_sie_dbg_info[id] == NULL) {
		return;
	}
	ctl_sie_dbg_ts_dumpflg |= (1 << id);
	for (i = 0; i < CTL_SIE_DBG_TS_MAXNUM; i++) {
		ctl_sie_dbg_info[id]->dbg_ts_info[i].t = 0;
		ctl_sie_dbg_info[id]->dbg_ts_info[i].fc = 0;
		ctl_sie_dbg_info[id]->dbg_ts_info[i].evt = 0;
		ctl_sie_dbg_info[id]->dbg_ts_info[i].evtcnt = 0;
		ctl_sie_dbg_ctl_info[id].dbg_ts_cnt = 0;
	}
	ctl_sie_dbg_ts_dumpflg &= ~(1 << id);
}

#if 0
#endif
/**
	dump/record dbg info
*/

void ctl_sie_dbg_dump_info(int (*dump)(const char *fmt, ...))
{
	UINT32 id;
	CTL_SIE_STATUS sie_status[CTL_SIE_MAX_SUPPORT_ID];
	CTL_SIE_HDL *sie_hdl[CTL_SIE_MAX_SUPPORT_ID];
	CTL_SIE_LIMIT *sie_limit[CTL_SIE_MAX_SUPPORT_ID];
	UINT32 ctl_sie_isp_freee_queue_num = ctl_sie_isp_get_free_queue_num();
	BOOL dump_evt = FALSE;

	if (dump == NULL) {
		dump = ctl_sie_int_dbg_printf;
	}

	dump("\r\n----------[CTL SIE] Dump Begin----------\r\n");
	/***** Input Info *********************************************************/
	{
		CHAR *str_flow_type[4] = {
			"UNKNOWN",
			"SENSOR_IN",
			"PATGEN",
			"SIG_DUPL"
		};
		CHAR *str_actmode[15] = {
			"PARA_MSTR",
			"PARA_SLAV",
			"PAT_GEN",
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
		CHAR *str_status[5] = {
			"CLOSE",
			"READY",
			"RUN",
			"SUSPEND",
			"MAX"
		};
		CHAR *str_sig[2] = {
			"R",
			"F"
		};
		CHAR *str_mclk_src[6] = {
			"CURR",
			"480",
			"PLL4",
			"PLL5",
			"PLL10",
			"PLL12",
		};

		CHAR *str_mclk_id_sel[2] = {
			"MCLK",
			"MCLK2",
		};

		CHAR *str_clk_src[12] = {
			"DFT",
			"CURR",
			"192",
			"320",
			"480",
			"PLL2",
			"PLL5",
			"PLL10",
			"PLL12",
			"PLL13",
			"PLL14",
			"PLL15",
		};
		CHAR *str_pclk_src[6] = {
			"OFF",
			"PAD",
			"AHD",
			"MCLK",
			"VX1_1X",
			"VX1_2X",
		};
		CHAR *str_reset_fc[3] = {
			"DONE",
			"BEGIN",
			"NONE",
		};
		CTL_SIE_MCLK_INFO sie_dbg_mclk_info = {0};

		dump("-----accu info-----\r\n");
		dump("%-4s%-8s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s%-12s\r\n", "id", "status", "vd_cnt", "crp_end_cnt", "frm_cnt", "new_b_ok", "new_b_fail", "buf_q_full", "drop_b_cnt", "flush_b_cnt", "push_b_cnt", "d_rls_ok", "d_rls_fail", "d_drop_ok", "d_drop_fail");
		for (id = CTL_SIE_ID_1; id < CTL_SIE_MAX_SUPPORT_ID; id++) {
			sie_status[id] = ctl_sie_get_state_machine(id);
			sie_hdl[id] = ctl_sie_get_hdl(id);
			sie_limit[id] = ctl_sie_limit_query(id);
			if (sie_hdl[id] == NULL || sie_status[id] == CTL_SIE_STS_CLOSE) {
				if (ctl_sie_dbg_frm_ctrl[id].vd_cnt != 0) {
					dump("%-4d%-8s%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu\r\n", (int)id, str_status[sie_status[id]], ctl_sie_dbg_frm_ctrl[id].vd_cnt, ctl_sie_dbg_frm_ctrl[id].crp_end_cnt, ctl_sie_dbg_frm_ctrl[id].frame_cnt,
					ctl_sie_dbg_frm_ctrl[id].new_ok_cnt, ctl_sie_dbg_frm_ctrl[id].new_fail_cnt, ctl_sie_dbg_frm_ctrl[id].buf_queue_full_cnt, (ctl_sie_dbg_frm_ctrl[id].drop_cnt - ctl_sie_dbg_frm_ctrl[id].flush_cnt), ctl_sie_dbg_frm_ctrl[id].flush_cnt, ctl_sie_dbg_frm_ctrl[id].push_cnt,
					ctl_sie_dbg_frm_ctrl[id].dir_rls_cnt, ctl_sie_dbg_frm_ctrl[id].dir_rls_fail_cnt, ctl_sie_dbg_frm_ctrl[id].dir_drop_cnt, ctl_sie_dbg_frm_ctrl[id].dir_drop_fail_cnt);
				}
			} else {
				dump_evt = TRUE;
				dump("%-4d%-8s%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu%-12llu\r\n", (int)id, str_status[sie_status[id]],sie_hdl[id]->ctrl.frame_ctl_info.vd_cnt, sie_hdl[id]->ctrl.frame_ctl_info.crp_end_cnt, sie_hdl[id]->ctrl.frame_ctl_info.frame_cnt,
				sie_hdl[id]->ctrl.frame_ctl_info.new_ok_cnt, sie_hdl[id]->ctrl.frame_ctl_info.new_fail_cnt, sie_hdl[id]->ctrl.frame_ctl_info.buf_queue_full_cnt, (sie_hdl[id]->ctrl.frame_ctl_info.drop_cnt - sie_hdl[id]->ctrl.frame_ctl_info.flush_cnt), sie_hdl[id]->ctrl.frame_ctl_info.flush_cnt, sie_hdl[id]->ctrl.frame_ctl_info.push_cnt,
				sie_hdl[id]->ctrl.frame_ctl_info.dir_rls_cnt, sie_hdl[id]->ctrl.frame_ctl_info.dir_rls_fail_cnt, sie_hdl[id]->ctrl.frame_ctl_info.dir_drop_cnt, sie_hdl[id]->ctrl.frame_ctl_info.dir_drop_fail_cnt);
			}
		}

		if (dump_evt == FALSE) {
			ctl_sie_dbg_dump_proc_time(NULL);
			dump("\r\n----------[CTL SIE] Dump End----------\r\n");
			return;
		}

		dump("\r\n-----status-----\r\n");
		dump("%-4s%-8s%-12s%-12s%-10s%-8s%-8s%-8s%-8s%-22s%-12s%-10s%-14s%-10s%-14s\r\n", "id", "status", "inte", "isr_sts", "trig_type", "bp1", "bp2", "bp3", "rst_fc", "dma_out(ch0 ch1 ch2)", "trig_frm", "wait_end", "support_func", "bp_ratio", "bp_ratio_dir");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			dump("%-4d%-8s%#-12x%#-12x%-10d%-8d%-8d%-8d%-8s%-7d(%-4d%-4d%-4d) %#-12x%-10d%#-14x%-10d%-14d\r\n", (int)id, str_status[sie_status[id]], (unsigned int)sie_hdl[id]->ctrl.inte, (unsigned int)sie_hdl[id]->ctrl.ext_isrcb_fp.sts, (int)sie_hdl[id]->ctrl.trig_info.trig_type,
				 (int)sie_hdl[id]->ctrl.bp1, (int)sie_hdl[id]->ctrl.bp2, (int)sie_hdl[id]->ctrl.bp3, str_reset_fc[sie_hdl[id]->ctrl.rst_fc_sts], (int)sie_hdl[id]->ctrl.dma_out, (int)sie_hdl[id]->ctrl.sin_out_ctl.ch0_single_out_en, (int)sie_hdl[id]->ctrl.sin_out_ctl.ch1_single_out_en, (int)sie_hdl[id]->ctrl.sin_out_ctl.ch2_single_out_en, (int)sie_hdl[id]->ctrl.trig_info.trig_frame_num, (int)sie_hdl[id]->ctrl.trig_info.b_wait_end, (unsigned int)sie_limit[id]->support_func,(int)sie_hdl[id]->ctrl.bp3_ratio,(int)sie_hdl[id]->ctrl.bp3_ratio_direct);
		}

		dump("\r\n-----clock-----\r\n");
		dump("%-4s%-10s%-10s%-12s%-12s%-10s%-12s%-14s%-10s\r\n", "id", "mclk_en", "mclk_src", "mclk_id_sel", "mclk_rate", "clk_src", "clk_rate", "drv_clk_rate", "pclk_src");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}
			ctl_sie_get((UINT32)sie_hdl[id], CTL_SIE_ITEM_MCLK_IMM, (void *)&sie_dbg_mclk_info);
			dump("%-4d%-10d%-10s%-12s%-12d%-10s%-12d%-14d%-10s\r\n", (int)id, (int)sie_dbg_mclk_info.clk_en,
				 str_mclk_src[sie_dbg_mclk_info.mclk_src_sel], str_mclk_id_sel[sie_dbg_mclk_info.mclk_id_sel], (int)sie_dbg_mclk_info.clk_rate,
				 str_clk_src[sie_hdl[id]->ctrl.clk_info.clk_src_sel], (int)sie_hdl[id]->ctrl.clk_info.data_rate, (int)sie_hdl[id]->ctrl.clk_info.drv_data_rate, str_pclk_src[sie_hdl[id]->ctrl.clk_info.pclk_src_sel]);
		}

		dump("\r\n-----input-----\r\n");
		dump("%-4s%-10s%-10s%-10s%-18s%-8s%-8s%-8s%-23s%-23s%-8s%-8s\r\n", "id", "dupl_src", "flow_type", "act_mode", "phase(vd,hd,data)", "vd_inv", "hd_inv", "grp_id", "act_win(x, y, w, h)", "crp_win(x, y, w, h)", "act_cfa", "crp_cfa");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			dump("%-4d%-10d%-10s%-10s(%3s, %3s, %3s)   %-8d%-8d%#-8x(%4d,%4d,%4d,%4d)  (%4d,%4d,%4d,%4d)  0x%-6x0x%-6x\r\n", (int)id, (int)sie_hdl[id]->dupl_src_id, str_flow_type[sie_hdl[id]->flow_type], str_actmode[sie_hdl[id]->ctrl.clk_info.act_mode],
				 str_sig[sie_hdl[id]->ctrl.rtc_obj.signal.vd_phase], str_sig[sie_hdl[id]->ctrl.rtc_obj.signal.hd_phase], str_sig[sie_hdl[id]->ctrl.rtc_obj.signal.data_phase],
				 (int)sie_hdl[id]->ctrl.rtc_obj.signal.b_vd_inverse, (int)sie_hdl[id]->ctrl.rtc_obj.signal.b_hd_inverse, (unsigned int)sie_hdl[id]->param.chg_senmode_info.output_dest,
				 (int)sie_hdl[id]->ctrl.rtc_obj.sie_act_win.x, (int)sie_hdl[id]->ctrl.rtc_obj.sie_act_win.y, (int)sie_hdl[id]->ctrl.rtc_obj.sie_act_win.w, (int)sie_hdl[id]->ctrl.rtc_obj.sie_act_win.h,
				 (int)sie_hdl[id]->param.io_size_info.size_info.sie_crp.x, (int)sie_hdl[id]->param.io_size_info.size_info.sie_crp.y, (int)sie_hdl[id]->param.io_size_info.size_info.sie_crp.w, (int)sie_hdl[id]->param.io_size_info.size_info.sie_crp.h,
				 (int)sie_hdl[id]->ctrl.rtc_obj.rawcfa_act, (int)sie_hdl[id]->ctrl.rtc_obj.rawcfa_crp);
		}
	}

	/***** CCIR Info **********************************************************/
	{
		CHAR *str_dvi_fmt[3] = {
			"601",
			"656_EAV",
			"656_ACT",
		};
		CHAR *str_dvi_in_mode[3] = {
			"SD",
			"HD",
			"HD_INV",
		};
		CHAR *str_yuv[4] = {
			"YUYV",
			"YVYU",
			"UYVY",
			"VYUY",
		};
		CHAR *str_vd_sel[2] = {
			"INTERLACED",
			"PROGRESSIVE",
		};

		dump("\r\n-----ccir-----\r\n");
		dump("%-4s%-8s%-8s%-12s%-16s%-12s%-12s%-12s%-10s%-10s%-12s\r\n", "id", "fmt", "mode", "yuv_order", "ccir656vd_mode", "data_period", "data_idx", "auto_align", "field_en", "field_sel", "inter_skip");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (ctl_sie_typecast_ccir(sie_hdl[id]->param.data_fmt)) {
				if (sie_limit[id]->support_func & KDRV_SIE_FUNC_SPT_DVI) {
					dump("%-4d%-8s%-8s%-12s%-16s%-12d%-12d%-12d%-10d%-10d%-12d\r\n", (int)id,
						 str_dvi_fmt[sie_hdl[id]->param.ccir_info_int.fmt],
						 str_dvi_in_mode[sie_hdl[id]->param.ccir_info_int.dvi_mode],
						 str_yuv[sie_hdl[id]->param.ccir_info_int.yuv_order],
						 str_vd_sel[sie_hdl[id]->param.ccir_info_int.b_ccir656_vd_sel], (int)sie_hdl[id]->param.ccir_info_int.data_period, (int)sie_hdl[id]->param.ccir_info_int.data_idx, (int)sie_hdl[id]->param.ccir_info_int.b_auto_align,
						 (int)sie_hdl[id]->param.ccir_info_int.b_filed_en, (int)sie_hdl[id]->param.ccir_info_int.b_filed_sel, (int)sie_hdl[id]->dvi_interlace_skip_en);
				} else {
					dump("%-4d N.S.\r\n", (int)id);
				}
			} else {
				dump("%-4dNot CCIR Format\r\n", (int)id);
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
		UINT32 real_fps = 0;
		CTL_SIE_MCLK_INFO sie_dbg_mclk_info = {0};

		dump("\r\n-----pat gen-----\r\n");
		dump("%-4s%-12s%-10s%-8s%-8s%-16s%-16s%-8s%-8s\r\n", "id", "mode", "value", "src_w", "src_h", "Set_fps(x100)", "Real_fps(x100)", "frm_num", "grp_idx");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (sie_hdl[id]->flow_type == CTL_SIE_FLOW_PATGEN) {
				if (sie_limit[id]->support_func & KDRV_SIE_FUNC_SPT_PATGEN) {
					ctl_sie_get((UINT32)sie_hdl[id], CTL_SIE_ITEM_MCLK_IMM, (void *)&sie_dbg_mclk_info);
					if ((sie_hdl[id]->ctrl.rtc_obj.pat_gen_param.pat_gen_src_win.w * sie_hdl[id]->ctrl.rtc_obj.pat_gen_param.pat_gen_src_win.h) > 100) { // avoid div 0
						real_fps = sie_dbg_mclk_info.clk_rate/((sie_hdl[id]->ctrl.rtc_obj.pat_gen_param.pat_gen_src_win.w * sie_hdl[id]->ctrl.rtc_obj.pat_gen_param.pat_gen_src_win.h) / 100);
					}
					dump("%-4d%-12s%#-10x%-8d%-8d%-16d%-16d%-8d%-8x\r\n", (int)id, str_pat_gen_mode[sie_hdl[id]->ctrl.rtc_obj.pat_gen_param.pat_gen_mode]
						,(unsigned int)sie_hdl[id]->ctrl.rtc_obj.pat_gen_param.pat_gen_val
						, (int)sie_hdl[id]->ctrl.rtc_obj.pat_gen_param.pat_gen_src_win.w, (int)sie_hdl[id]->ctrl.rtc_obj.pat_gen_param.pat_gen_src_win.h, (int)sie_hdl[id]->ctrl.rtc_obj.pat_gen_param.frame_rate
						, (int)real_fps, (int)sie_hdl[id]->ctrl.total_frame_num, (int)sie_hdl[id]->ctrl.rtc_obj.pat_gen_param.group_idx);
				} else {
					dump("%-4d N.S.\r\n", (int)id);
				}
			} else {
				dump("%-4dNot PatGen Mode\r\n", (int)id);
			}
		}
	}

	/***** sw sync Info **********************************************************/
	{
		dump("\r\n-----sync info-----\r\n");
		dump("%-4s%-6s%-8s%-12s%-10s%-10s%-10s%-12s%-14s%-14s%-10s%-8s%-10s%-10s%-10s\r\n", "id", "mode", "sync_id", "det_frm_int", "adj_thres",
			"adj_auto", "det_pause", "det_frm_cnt", "det_1st_done", "adj_prc_step", "adj_time", "adj_id", "org_fps", "rec_cnt", "adj_cnt");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			dump("%-4d%-6d%-8d%-12d%-10d%-10d%-10d%-12d%-14d%-14d%-10d%-8d%-10d%-10d%-10d\r\n", (int)id, sie_hdl[id]->ctrl.sync_obj.mode,
				sie_hdl[id]->ctrl.sync_obj.sync_id, sie_hdl[id]->ctrl.sync_obj.det_frm_int,
				sie_hdl[id]->ctrl.sync_obj.adj_thres, sie_hdl[id]->ctrl.sync_obj.adj_auto,
				sie_hdl[id]->ctrl.sync_obj.det_pause, sie_hdl[id]->ctrl.sync_obj.det_frm_cnt, sie_hdl[id]->ctrl.sync_obj.det_1st_done,
				sie_hdl[id]->ctrl.sync_obj.adj_proc_step, sie_hdl[id]->ctrl.sync_obj.adj_time, sie_hdl[id]->ctrl.sync_obj.adj_id,
				sie_hdl[id]->ctrl.sync_obj.org_fps, sie_hdl[id]->ctrl.sync_obj.rec_cnt, sie_hdl[id]->ctrl.sync_obj.adj_cnt);
		}
	}

	/***** gyro Info **********************************************************/
	{
		dump("\r\n-----gyro info-----\r\n");
		dump("%-4s%-10s%-4s%-10s%-12s\r\n", "id", "gyro_obj", "en", "data_num", "latency_en");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (sie_hdl[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj == NULL) {
				dump("%-4d%-10s%-4d%-10d%-12d\r\n", (int)id, "NULL", 0, 0, 0);
			} else {
				dump("%-4d%-10s%-4d%-10d%-12d\r\n", (int)id, "Y"
					, (int)sie_hdl[id]->ctrl.gyro_ctl_info.gyro_cfg.en
					, (int)sie_hdl[id]->ctrl.gyro_ctl_info.gyro_cfg.data_num
					, (int)sie_hdl[id]->ctrl.gyro_ctl_info.gyro_cfg.latency_en);
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
		dump("%-4s%-14s%-6s%-14s%-8s%-10s%-10s%-23s%-9s%-10s%-11s\r\n", "id", "scale_out_win", "flip", "out_fmt", "encode", "enc_rate", "out_dest", "out_mode(ch0,ch1,ch2)", "r_buf_en", "r_buf_len", "r_buf_sz");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}
			if (ctl_sie_typecast_raw(sie_hdl[id]->param.data_fmt)) {
				dump("%-4d(%4d, %4d)  %-6d%-14s%-8d%-10d%-10s(%-6s,%-6s,%-6s) %-9d%-10d0x%-9x\r\n", (int)id,
						(int)sie_hdl[id]->param.io_size_info.size_info.sie_scl_out.w, (int)sie_hdl[id]->param.io_size_info.size_info.sie_scl_out.h,
						(int)sie_hdl[id]->param.flip, str_out_fmt[sie_hdl[id]->param.data_fmt],
						(int)sie_hdl[id]->param.encode_en, (int)sie_hdl[id]->param.enc_rate, str_dest[sie_hdl[id]->param.out_dest], out_mode_sel[sie_hdl[id]->ctrl.ch_out_mode.out0_mode], out_mode_sel[sie_hdl[id]->ctrl.ch_out_mode.out1_mode], out_mode_sel[sie_hdl[id]->ctrl.ch_out_mode.out2_mode], (int)sie_hdl[id]->ctrl.ring_buf_ctl.en, (int)sie_hdl[id]->ctrl.ring_buf_ctl.buf_len, (int)sie_hdl[id]->ctrl.ring_buf_ctl.buf_size);
			} else if (ctl_sie_typecast_ccir(sie_hdl[id]->param.data_fmt)) {
				dump("%-4d(%4d, %4d)  %-6d%-14s%-8s%-10s%-10s(%-6s,%-6s,%-6s) %-9d%-10d0x%-9x\r\n", (int)id,
						(int)sie_hdl[id]->param.io_size_info.size_info.sie_scl_out.w, (int)sie_hdl[id]->param.io_size_info.size_info.sie_scl_out.h,
						(int)sie_hdl[id]->param.flip, str_out_fmt[sie_hdl[id]->param.data_fmt], "N/A", "N/A", "N/A", out_mode_sel[sie_hdl[id]->ctrl.ch_out_mode.out0_mode], out_mode_sel[sie_hdl[id]->ctrl.ch_out_mode.out1_mode], out_mode_sel[sie_hdl[id]->ctrl.ch_out_mode.out2_mode], (int)sie_hdl[id]->ctrl.ring_buf_ctl.en, (int)sie_hdl[id]->ctrl.ring_buf_ctl.buf_len, (int)sie_hdl[id]->ctrl.ring_buf_ctl.buf_size);
			} else {
				dump("%-4d, unknown datafmt %d\r\n", (int)id, (int)sie_hdl[id]->param.data_fmt);
			}
		}
	}

	/***** Out Channel ************************************************/
	{
		UINT32 i;

		dump("\r\n-----channel-----\r\n");
		dump("%-4s%-4s%-12s%-8s\r\n", "id", "ch", "addr", "lof");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}
			for (i = CTL_SIE_CH_0; i <= sie_limit[id]->out_max_ch; i++) {
				if (i < CTL_SIE_CH_MAX) {
					if ((ctl_sie_typecast_raw(sie_hdl[id]->param.data_fmt)) && (i == CTL_SIE_CH_1 || i == CTL_SIE_CH_2)) {
						dump("%-4d%-4d0x%-10x%-8s\r\n", (int)id, (int)i, (unsigned int)sie_hdl[id]->ctrl.out_ch_addr[i], "N/A");
					} else {
						dump("%-4d%-4d0x%-10x%-8d\r\n", (int)id, (int)i, (unsigned int)sie_hdl[id]->ctrl.out_ch_addr[i], (int)sie_hdl[id]->ctrl.rtc_obj.out_ch_lof[i]);
					}
				}
			}
		}
	}

	/***** ISP func Info ************************************************************/
	{
		dump("\r\n-----ISP Function en-----\r\n");
		dump("free_queue_num: %d\r\n", (int)(ctl_sie_isp_freee_queue_num));
		dump("isp sts: 0x%x\r\n", (int)(ctl_sie_isp_get_sts()));
		dump("%-4s%-10s  %-6s%-6s%-6s%-6s%-6s%-6s\r\n", "id", "isp_id", "AWB", "AE", "AF", "HDR", "WDR", "YOUT");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}
			dump("%-4d%#-10x  %-6d%-6d%-6d%-6d%-6d%-6d\r\n", (int)id, (unsigned int)sie_hdl[id]->isp_id,
					 (int)sie_hdl[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AWB],
					 (int)sie_hdl[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AE],
					 (int)sie_hdl[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AF],
					 (int)sie_hdl[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_SHDR],
					 (int)sie_hdl[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_WDR],
					 (int)sie_hdl[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_YOUT]);
		}
	}

	/***** OB Info ************************************************/
	{
		dump("\r\n-----ob-----\r\n");
		dump("%-4s%-8s%-16s\r\n", "id", "ofs", "bypass");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (ctl_sie_typecast_raw(sie_hdl[id]->param.data_fmt)) {
				dump("%-4d%-8d%-4d\r\n", id, sie_hdl[id]->ctrl.ob_param.ob_ofs,  sie_hdl[id]->ctrl.ob_param.bypass_enable);
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	/***** Gain Info ************************************************************/
	{
		dump("\r\n-----gain-----\r\n");
		dump("%-4s%-12s%-16s%-28s\r\n", "id", "cgain_en", "cg_37_fmt_sel", "cg(r, gr, gb, g)");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (ctl_sie_typecast_raw(sie_hdl[id]->param.data_fmt)) {
				dump("%-4d%-12d%-16d(%4d, %4d, %4d, %4d)\r\n", id,
					sie_hdl[id]->ctrl.cgain_param.enable, sie_hdl[id]->ctrl.cgain_param.sel_37_fmt,
					sie_hdl[id]->ctrl.cgain_param.r_gain, sie_hdl[id]->ctrl.cgain_param.gr_gain, sie_hdl[id]->ctrl.cgain_param.gb_gain, sie_hdl[id]->ctrl.cgain_param.b_gain);
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	/***** CA Info ************************************************************/
	{
		dump("\r\n-----ca-----\r\n");
		dump("%-4s%-4s%-16s%-28s%-8s%-28s%-28s%-10s%-20s%-6s%-6s\r\n", "id", "en", "win_num(w, h)", "roi(x, y, w, h)", "thr_en", "thr_lower(r, g, b, p)", "thr_upper(r, g, b, p)", "irsub_en", "irsub_w(r, g, b)", "ob", "src");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (ctl_sie_typecast_raw(sie_hdl[id]->param.data_fmt)) {
				dump("%-4d%-4d(%4d, %4d)    (%4d, %4d, %4d, %4d)    %-8d(%4d, %4d, %4d, %4d)    (%4d, %4d, %4d, %4d)    %-10s(%4d, %4d, %4d)  %-6d%-6d\r\n", id,
					sie_hdl[id]->ctrl.ca_param.enable, sie_hdl[id]->ctrl.ca_param.win_num.w, sie_hdl[id]->ctrl.ca_param.win_num.h,
					sie_hdl[id]->ctrl.ca_roi.roi.x, sie_hdl[id]->ctrl.ca_roi.roi.y, sie_hdl[id]->ctrl.ca_roi.roi.w, sie_hdl[id]->ctrl.ca_roi.roi.h,
					sie_hdl[id]->ctrl.ca_param.th_enable, sie_hdl[id]->ctrl.ca_param.r_th_l, sie_hdl[id]->ctrl.ca_param.g_th_l, sie_hdl[id]->ctrl.ca_param.b_th_l, sie_hdl[id]->ctrl.ca_param.p_th_l,
					sie_hdl[id]->ctrl.ca_param.r_th_u, sie_hdl[id]->ctrl.ca_param.g_th_u, sie_hdl[id]->ctrl.ca_param.b_th_u, sie_hdl[id]->ctrl.ca_param.p_th_u, "N/A",
					sie_hdl[id]->ctrl.ca_param.irsub_r_weight, sie_hdl[id]->ctrl.ca_param.irsub_g_weight, sie_hdl[id]->ctrl.ca_param.irsub_b_weight, sie_hdl[id]->ctrl.ca_param.ca_ob_ofs, sie_hdl[id]->ctrl.ca_param.ca_src);
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
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (ctl_sie_typecast_raw(sie_hdl[id]->param.data_fmt)) {
				dump("%-4d%-4d(%4d, %4d)    (%4d, %4d, %4d, %4d)    %-12s%-8d(%4d, %4d, %4d)    %-8d%-12d%-10s%-8d\r\n", id,
					sie_hdl[id]->ctrl.la_param.enable, sie_hdl[id]->ctrl.la_param.win_num.w, sie_hdl[id]->ctrl.la_param.win_num.h,
					sie_hdl[id]->ctrl.la_roi.roi.x, sie_hdl[id]->ctrl.la_roi.roi.y, sie_hdl[id]->ctrl.la_roi.roi.w, sie_hdl[id]->ctrl.la_roi.roi.h,
					str_lasrc[sie_hdl[id]->ctrl.la_param.la_src], sie_hdl[id]->ctrl.la_param.cg_enable, sie_hdl[id]->ctrl.la_param.r_gain, sie_hdl[id]->ctrl.la_param.g_gain, sie_hdl[id]->ctrl.la_param.b_gain,
					sie_hdl[id]->ctrl.la_param.gamma_enable, sie_hdl[id]->ctrl.la_param.histogram_enable, str_histsrc[sie_hdl[id]->ctrl.la_param.histogram_src], sie_hdl[id]->ctrl.la_param.la_ob_ofs);
			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	/***** DPC Info ************************************************************/
	{
		CHAR* str_dpc_weight[4] = {
			"50%",
			"25%",
			"12.25%",
			"6.125%",
		};
		dump("\r\n-----dpc-----\r\n");
		dump("%-4s%-4s%-10s%-12s\r\n", "id", "en", "addr", "weight");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (ctl_sie_typecast_raw(sie_hdl[id]->param.data_fmt)) {
				dump("%-4d%-4d0x%-8x%-12s\r\n", id, sie_hdl[id]->ctrl.dpc_param.enable,
					sie_hdl[id]->ctrl.dpc_param.tbl_addr,
					str_dpc_weight[sie_hdl[id]->ctrl.dpc_param.weight]);
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
		dump("%-4s%-4s%-10s%-10s%-12s%-10s%-12s%-10s%-12s\r\n", "id", "en", "addr", "map_sel", "37_fmt_sel", "dthr_en", "dthr_reset", "dthr_lv", "bayer_mode");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (ctl_sie_typecast_raw(sie_hdl[id]->param.data_fmt)) {
				dump("%-4d%-4x0x%-8x%-10s%-12d%-10d%-12d%-10d%-12d\r\n", id, sie_hdl[id]->ctrl.ecs_param.enable, sie_hdl[id]->ctrl.ecs_param.map_tbl_addr,
					str_map_sel[sie_hdl[id]->ctrl.ecs_param.map_sel], sie_hdl[id]->ctrl.ecs_param.sel_37_fmt,
					sie_hdl[id]->ctrl.ecs_param.dthr_enable, sie_hdl[id]->ctrl.ecs_param.dthr_reset, sie_hdl[id]->ctrl.ecs_param.dthr_level, sie_hdl[id]->ctrl.ecs_param.bayer_mode);

			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	/***** MD Info ************************************************************/
	{
		CHAR* str_md_src[2] = {
			"PRE_GAMMA",
			"POST_GAMMA"
		};
		CHAR* str_md_mask_mode[2] = {
			"COL_ROW",
			"8x8"
		};

		dump("\r\n-----md-----\r\n");
		dump("%-4s%-4s%-12s%-10s%-10s%-8s%-8s%-12s%-16s%-16s\r\n", "id", "en", "src", "sum_frms", "mask_mode", "mask0", "mask1", "blk_dif_thr", "ttl_blk_dif_thr", "blk_dif_cnt_thr");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (ctl_sie_typecast_raw(sie_hdl[id]->param.data_fmt)) {
				dump("%-4d%-4d%-12s%-10d%-10s%-8d%-8d%-12d%-16d%-16d\r\n", id, sie_hdl[id]->ctrl.md_param.enable,
				str_md_src[sie_hdl[id]->ctrl.md_param.md_src], sie_hdl[id]->ctrl.md_param.sum_frms, str_md_mask_mode[sie_hdl[id]->ctrl.md_param.mask_mode],
				sie_hdl[id]->ctrl.md_param.mask0, sie_hdl[id]->ctrl.md_param.mask1, sie_hdl[id]->ctrl.md_param.blkdiff_cnt_thr,
				sie_hdl[id]->ctrl.md_param.total_blkdiff_thr, sie_hdl[id]->ctrl.md_param.blkdiff_thr);

			} else {
				dump("%-4d%-4s\r\n", id, "N/A");
			}
		}
	}

	/***** CHG SENMODE Info ************************************************************/
	{
		CHAR *str_mode_sel[2] = {
			"AUTO",
			"MANUAL",
		};
		dump("\r\n-----chg_senmode info-----\r\n");
		dump("%-4s%-8s%-8sAUTO(%8s %8s %8s %6s %4s %4s %6s %4s %8s %8s %6s) MANUAL(%8s%8s%6s) %-8s %-10s\r\n"
			, "id", "sen_id", "sel", "fps", "sz.w", "sz.h", "num", "fmt", "bit", "type", "dl", "ccir_int", "ccir_fmt", "mux_en", "mux_num"
			, "fps", "mode", "cfg_mode", "out_dest");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}

			if (ctl_sie_typecast_raw(sie_hdl[id]->param.data_fmt)) {
				dump("%-4d%-8d%-8sAUTO(%8d %8d %8d %6d %4d %4d %6d %4d %8s %8s %6d) MANUAL(%8d%8d%6d) %-8d 0x%.8x\r\n"
					 , (int)id , (int) sie_hdl[id]->sen_id
					 , str_mode_sel[sie_hdl[id]->param.chg_senmode_info.sel]
					 , (unsigned int)sie_hdl[id]->param.chg_senmode_info.auto_info.frame_rate
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.size.w
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.size.h
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.frame_num
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.data_fmt
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.pixdepth
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.mode_type_sel
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.data_lane
					 , "-"
					 , "-"
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.mux_singnal_en
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.mux_signal_info.mux_data_num
					 , (unsigned int)sie_hdl[id]->param.chg_senmode_info.manual_info.frame_rate
					 , (int)sie_hdl[id]->param.chg_senmode_info.manual_info.sen_mode+1
					 , (int)sie_hdl[id]->param.chg_senmode_info.cfg_sen_mode+1
					 , (unsigned int)sie_hdl[id]->param.chg_senmode_info.output_dest
					);
			} else {
				dump("%-4d%-8sAUTO(%8d %8d %8d %6d %4d %4d %6s %4s %8d %8d %6d) MANUAL(%8d%8d%6d) %-8d 0x%.8x\r\n"
					 , (int)id
					 , str_mode_sel[sie_hdl[id]->param.chg_senmode_info.sel]
					 , (unsigned int)sie_hdl[id]->param.chg_senmode_info.auto_info.frame_rate
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.size.w
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.size.h
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.frame_num
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.data_fmt
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.pixdepth
					 , "-"
					 , "-"
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.ccir.interlace
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.ccir.fmt
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.mux_singnal_en
					 , (int)sie_hdl[id]->param.chg_senmode_info.auto_info.mux_signal_info.mux_data_num
					 , (unsigned int)sie_hdl[id]->param.chg_senmode_info.manual_info.frame_rate
					 , (int)sie_hdl[id]->param.chg_senmode_info.manual_info.sen_mode+1
					 , (int)sie_hdl[id]->param.chg_senmode_info.cfg_sen_mode+1
					 , (unsigned int)sie_hdl[id]->param.chg_senmode_info.output_dest
					);
			}
		}
	}

	/***** io size Info ************************************************************/
	{
		CHAR *str_crp_sel[2] = {
			"NONE",
			"ON",
		};
		CHAR *str_dz_sel[2] = {
			"AUTO",
			"MANUAL",
		};

		dump("\r\n-----io size info-----\r\n");
		dump("%-4s%-8s%-10s%-8s%-18s%-12s%-12s%-12s%-24s%-16s%-16s%-12s\r\n"
			 , "id", "crp_sel", "iosz_sel", "factor", "dst_ext_crp_prop", "shift_start"
			 , "crp_min_win", "align", "scl_max", "dest_crp_start", "desp_crp_win", "dest_align");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}
			dump("%-4d%-8s%-10s%-8d(%4d,%4d)       (%4d,%4d) (%4d,%4d) (%4d,%4d) (0x%8x,0x%8x) (%4d,%4d)     (%4d,%4d)     (%4d,%4d)\r\n"
				 , (int)id
				 , str_crp_sel[sie_hdl[id]->param.io_size_info.auto_info.crp_sel]
				 , str_dz_sel[sie_hdl[id]->param.io_size_info.iosize_sel]
				 , (int)sie_hdl[id]->param.io_size_info.auto_info.factor
				 , (int)sie_hdl[id]->param.io_size_info.auto_info.dest_ext_crp_prop.w
				 , (int)sie_hdl[id]->param.io_size_info.auto_info.dest_ext_crp_prop.h
				 , (int)sie_hdl[id]->param.io_size_info.auto_info.sft.x
				 , (int)sie_hdl[id]->param.io_size_info.auto_info.sft.y
				 , (int)sie_hdl[id]->param.io_size_info.auto_info.sie_crp_min.w
				 , (int)sie_hdl[id]->param.io_size_info.auto_info.sie_crp_min.h
				 , (int)sie_hdl[id]->param.io_size_info.align.w
				 , (int)sie_hdl[id]->param.io_size_info.align.h
				 , (int)sie_hdl[id]->param.io_size_info.auto_info.sie_scl_max_sz.w
				 , (int)sie_hdl[id]->param.io_size_info.auto_info.sie_scl_max_sz.h
				 , (int)sie_hdl[id]->param.io_size_info.size_info.dest_crp.x
				 , (int)sie_hdl[id]->param.io_size_info.size_info.dest_crp.y
				 , (int)sie_hdl[id]->param.io_size_info.size_info.dest_crp.w
				 , (int)sie_hdl[id]->param.io_size_info.size_info.dest_crp.h
				 , (int)sie_hdl[id]->param.io_size_info.dest_align.w
				 , (int)sie_hdl[id]->param.io_size_info.dest_align.h
				);
		}
	}

	/***** ctrl buf ************************************************************/
	{
		dump("\r\n-----ctrl buf-----\r\n");
		dump("%-4s%-12s ch0(%-4s%-10s) ch1(%-4s%-10s) ch2(%-4s%-10s)\r\n"
			 , "id", "total_size", "en", "buf_size", "en", "buf_size", "en", "buf_size");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}
			dump("%-4d0x%-10x ch0(%-4d0x%-8x) ch1(%-4d0x%-8x) ch2(%-4d0x%-8x)\r\n"
				 , (int)id
				 , (unsigned int)sie_hdl[id]->ctrl.buf_info.total_size
				 , (int)sie_hdl[id]->ctrl.buf_info.ch_info[0].out_enable
				 , (unsigned int)sie_hdl[id]->ctrl.buf_info.ch_info[0].data_size
				 , (int)sie_hdl[id]->ctrl.buf_info.ch_info[1].out_enable
				 , (unsigned int)sie_hdl[id]->ctrl.buf_info.ch_info[1].data_size
				 , (int)sie_hdl[id]->ctrl.buf_info.ch_info[2].out_enable
				 , (unsigned int)sie_hdl[id]->ctrl.buf_info.ch_info[2].data_size
				);
		}
	}
	/***** flag status ************************************************************/
	{
		dump("\r\n-----flag info-----\r\n");
		dump("%-10s%-8s%-16s\r\n", "flag", "flag_id", "ptn");
		for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
			if (sie_status[id] == CTL_SIE_STS_CLOSE) {
				continue;
			}
			dump("SIE%d      %-8d%#-16x\r\n" , (int)(id+1), (int)ctl_sie_get_flag_id(id), (unsigned int)vos_flag_chk(ctl_sie_get_flag_id(id), FLGPTN_BIT_ALL));
		}
		dump("%-10s%-8d%#-16x\r\n", "isp_flag", (int)ctl_sie_get_isp_flag_id(), (unsigned int)vos_flag_chk(ctl_sie_get_isp_flag_id(), FLGPTN_BIT_ALL));
		dump("%-10s%-8d%#-16x\r\n", "buf_flag", (int)ctl_sie_get_buf_flag_id(), (unsigned int)vos_flag_chk(ctl_sie_get_buf_flag_id(), FLGPTN_BIT_ALL));
	}

	/***** context info ************************************************************/
	{
		CTL_SIE_HDL_CONTEXT *sie_ctx;

		sie_ctx = ctl_sie_get_ctx();
		dump("\r\n-----ctx info-----\r\n");
		dump("%-8s%-10s%-12s%-12s\r\n", "ctx_num", "ctx_used", "addr", "req_size");
		if (sie_ctx == NULL) {
			dump("NULL SIE Context\r\n");
		} else {
			dump("%-8d%-10d%#-12x%#-12x\r\n" , (int)sie_ctx->context_num, (int)sie_ctx->context_used, (unsigned int)sie_ctx->start_addr, (unsigned int)sie_ctx->req_size);

			dump("\r\n%-8s%-10s%-12s\r\n", "ctx_idx", "id", "hdl_adr");
			for (id = CTL_SIE_ID_1; id <= sie_limit[CTL_SIE_ID_1]->max_spt_id; id++) {
				if (sie_ctx->context_idx[id] != CTL_SIE_MAX_SUPPORT_ID) {
					dump("%-8d%-10d%#-12x\r\n", (int)id, (int)sie_ctx->context_idx[id], (int)(UINT32 *)sie_hdl[sie_ctx->context_idx[id]]);
				}
			}
		}
	}
	/***** sie event info **********************************************************/
	sie_event_dump(dump);
	/***** sie isp info **********************************************************/
	ctl_sie_dbg_isp_dump(dump);
	/***** sie proc time info **********************************************************/
	ctl_sie_dbg_dump_proc_time(dump);
	dump("\r\n----------[CTL SIE] Dump End----------\r\n");
	return;
}

void ctl_sie_dbg_dump_ts(CTL_SIE_ID id, int (*dump)(const char *fmt, ...))
{
	CHAR *evt_str_map[CTL_SIE_DBG_TS_MAX] = {
		"VD",
		"BP3",
		"ADR_RDY",
		"DRAMEND",
		"RCVMSG",
		"UNLOCK",
		"CRPST",
		"CRPEND",
	};
	UINT32 i, idx, prv_vd_t, dump_id_bit = 0, dump_id_bit_remain;
	CTL_SIE_DBG_TS_INFO *p_ts;
	UINT64 min_fc, max_fc, fc;
	CTL_SIE_STATUS sie_status;

	if (ctl_sie_dbg_info[id] == NULL) {
		return;
	}

	if (dump == NULL) {
		dump = ctl_sie_int_dbg_printf;
	}

	/* dump all run sie_id */
	dump_id_bit |= (1 << id);
	for (i = 0; i < CTL_SIE_MAX_SUPPORT_ID; i++) {
		sie_status = ctl_sie_get_state_machine(i);
		if ((sie_status == CTL_SIE_STS_RUN) && (ctl_sie_dbg_info[i] != NULL)) {
			dump_id_bit |= (1 << i);
		}
	}
	dump_id_bit_remain = dump_id_bit;

	ctl_sie_dbg_ts_dumpflg |= dump_id_bit;

next_id:
	prv_vd_t = 0;
	max_fc = 0;
	min_fc = 0xFFFFFFFFFFFFFFFF;

	// find sie_fc min_max
	for (i = 0; i < CTL_SIE_DBG_TS_MAXNUM; i++) {
		p_ts = &ctl_sie_dbg_info[id]->dbg_ts_info[i];
		if (p_ts->fc > max_fc) {
			max_fc = p_ts->fc;
		}
		if (p_ts->fc < min_fc) {
			min_fc = p_ts->fc;
		}
	}

	// dump VD interval
	dump("\r\n----------CTL_SIE_ID %d PROC Begin----------\r\n", (int)(id));
	dump("%-4s%-16s%-16s%-12s\r\n", "", "FC", "Interval(us)", "fps(x100)");
	for (i = 0; i < CTL_SIE_DBG_TS_MAXNUM; i++) {
		idx = ((ctl_sie_dbg_ctl_info[id].dbg_ts_cnt - 1 - i) % CTL_SIE_DBG_TS_MAXNUM);
		p_ts = &ctl_sie_dbg_info[id]->dbg_ts_info[idx];

		if (p_ts->evt == CTL_SIE_DBG_TS_VD) {
			if (prv_vd_t == 0) {
				prv_vd_t = p_ts->t;
			} else {
				dump("%-4s%-16lld%-16lld%-12lld\r\n", "VD", (long long)p_ts->fc, (long long)(prv_vd_t - p_ts->t), (long long)CTL_SIE_DIV_U64(100000000, (prv_vd_t - p_ts->t)));
				prv_vd_t = p_ts->t;
			}
		}
	}
	dump("\r\n");

	// dump profile result
	for (fc = max_fc; fc >= min_fc; fc--) {
		UINT32 prv_ts = 0;
		UINT32 d_t = 0;

		dump("%32s %llu ----\r\n", "---- ctl_sie_profile, frame cnt = ", (unsigned long long)(fc));
		dump("%-12s%-16s%-16s%-16s\r\n", "EVT_TYPE", "STAMP(us)", "Duration(us)", "EVT_CNT");
		for (i = 0; i < CTL_SIE_DBG_TS_MAXNUM; i++) {
			idx = ((ctl_sie_dbg_ctl_info[id].dbg_ts_cnt + i) % CTL_SIE_DBG_TS_MAXNUM);
			p_ts = &ctl_sie_dbg_info[id]->dbg_ts_info[idx];
			if (p_ts->fc == fc) {
				if (prv_ts != 0) {
					d_t = (p_ts->t - prv_ts);
				}
				dump("%-12s%-16lld%-16lld%-16d\r\n", evt_str_map[p_ts->evt], (long long)p_ts->t, (long long)d_t, (int)p_ts->evtcnt);
				prv_ts = p_ts->t;
			}
		}
		dump("\r\n");
		if (fc == 0) {
			break;
		}
	}

	dump_id_bit_remain &= ~(1 << id);
	if (dump_id_bit_remain) {
		for (i = 0; i < CTL_SIE_MAX_SUPPORT_ID; i++) {
			if ((UINT32)(dump_id_bit_remain & (1 << i)) == (UINT32)(1 << i)) {
				id = i;
			}
		}
		goto next_id;
	}

	dump("----------CTL_SIE_ID_%d PROC End----------\r\n", (int)(id + 1));
	ctl_sie_dbg_ts_dumpflg &= ~(dump_id_bit);
}

void ctl_sie_dbg_dump_buf_io(CTL_SIE_ID id, CTL_SIE_BUF_IO_CFG buf_io, UINT32 total_size, UINT32 header_addr)
{
	CTL_SIE_HEADER_INFO *p_int_head = (CTL_SIE_HEADER_INFO *)header_addr;

	if (ctl_sie_dbg_info[id] == NULL) {
		return;
	}

	if (ctl_sie_dbg_chk_msg_type(id, CTL_SIE_DBG_MSG_BUF_IO_FULL) && (ctl_sie_dbg_ctl_info[id].dbg_bufio_cnt > 0)) {
		if (buf_io == CTL_SIE_BUF_IO_NEW) {
			DBG_DUMP("[N]id:%d, buf_adr:%#x, buf_id:%#x, req_sz:%#x, sz(%d, %d), fc:%llu\r\n",
				(int)id, (unsigned int)p_int_head->buf_addr, (unsigned int)p_int_head->buf_id, (unsigned int)total_size, (int)p_int_head->vdo_frm.size.w, (int)p_int_head->vdo_frm.size.h, p_int_head->vdo_frm.count);
		} else if (buf_io == CTL_SIE_BUF_IO_PUSH) {
			DBG_DUMP("[P]id:%d, buf_adr:%#x, buf_id:%#x, img_adr(%#x, %#x, %#x), sz(%d, %d), lofs(%d, %d, %d), pxfmt:%#x, fc:%llu, ts:%llu\r\n",
				(int)id, (unsigned int)p_int_head->buf_addr, (unsigned int)p_int_head->buf_id,
				(unsigned int)p_int_head->vdo_frm.addr[0], (unsigned int)p_int_head->vdo_frm.addr[1], (unsigned int)p_int_head->vdo_frm.addr[2],
				(int)p_int_head->vdo_frm.size.w, (int)p_int_head->vdo_frm.size.h,
				(int)p_int_head->vdo_frm.loff[0], (int)p_int_head->vdo_frm.loff[1], (int)p_int_head->vdo_frm.loff[2],
				(unsigned int)p_int_head->vdo_frm.pxlfmt, p_int_head->vdo_frm.count, p_int_head->vdo_frm.timestamp);
		} else if (buf_io == CTL_SIE_BUF_IO_UNLOCK) {
			DBG_DUMP("[U]id:%d, buf_adr:%#x, buf_id:%#x, fc:%llu\r\n",
				(int)id, (unsigned int)p_int_head->buf_addr, (unsigned int)p_int_head->buf_id, (long long)p_int_head->vdo_frm.count);
		} else if (buf_io == CTL_SIE_BUF_IO_LOCK) {
			DBG_DUMP("[L]id:%d, buf_adr:%#x, buf_id:%#x, fc:%llu\r\n",
				(int)id, (unsigned int)p_int_head->buf_addr, (unsigned int)p_int_head->buf_id, (long long)p_int_head->vdo_frm.count);
		}
		ctl_sie_dbg_ctl_info[id].dbg_bufio_cnt--;
	} else if (ctl_sie_dbg_chk_msg_type(id, CTL_SIE_DBG_MSG_BUF_IO_LITE) && (ctl_sie_dbg_ctl_info[id].dbg_bufio_cnt > 0)) {
		if (buf_io == CTL_SIE_BUF_IO_NEW) {
			DBG_DUMP("[N]id:%d adr:%#x r_sz:%#x f:%llu\r\n",
				(int)id, (unsigned int)p_int_head->buf_addr, (unsigned int)total_size, (long long)p_int_head->vdo_frm.count);
		} else if (buf_io == CTL_SIE_BUF_IO_PUSH) {
			DBG_DUMP("[P]id:%d adr:%#x f:%llu\r\n",
				(int)id, (unsigned int)p_int_head->buf_addr, (long long)p_int_head->vdo_frm.count);
		} else if (buf_io == CTL_SIE_BUF_IO_UNLOCK) {
			DBG_DUMP("[U]id:%d adr:%#x\r\n",
				(int)id, (unsigned int)p_int_head->buf_addr);
		} else if (buf_io == CTL_SIE_BUF_IO_LOCK) {
			DBG_DUMP("[L]id:%d adr:%#x\r\n",
				(int)id, (unsigned int)p_int_head->buf_addr);
		}
		ctl_sie_dbg_ctl_info[id].dbg_bufio_cnt--;
	}
}

void ctl_sie_dbg_savefile(CHAR *f_name, UINT32 addr, UINT32 size)
{
	VOS_FILE fp;

	if (addr != 0) {
		fp = vos_file_open(f_name, O_CREAT|O_WRONLY|O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
		    ctl_sie_dbg_err("failed in file open:%s\n", f_name);
		} else {
			vos_file_write(fp, (void *)addr, size);
			vos_file_close(fp);
			DBG_DUMP("save file %s\r\n", f_name);
		}
	}
}

void ctl_sie_dbg_dump_isr_ioctl(int (*dump)(const char *fmt, ...))
{
#if CTL_SIE_DBG_DUMP_IO_BUF
	UINT32 i, idx, id;
	UINT32 prev_ts[CTL_SIE_MAX_SUPPORT_ID] = {0};

	if (dump == NULL) {
		dump = ctl_sie_int_dbg_printf;
	}

	ctl_sie_dbg_buf_ioctl_dump = TRUE;
	for (id = 0; id < CTL_SIE_MAX_SUPPORT_ID; id++) {

		if (ctl_sie_dbg_info[id] != NULL) {
			if (ctl_sie_dbg_chk_msg_type(id, CTL_SIE_DBG_MSG_ISR_IOCTL)) {
				prev_ts[id] = 0;
				for (i = CTL_SIE_DBG_ISR_IOCTL_NUM; i > 0 ; i--) {
					idx = (ctl_sie_dbg_ctl_info[id].dbg_isr_ioctl_cnt - i) % CTL_SIE_DBG_ISR_IOCTL_NUM;
					if (ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].ts != 0) {
						dump("id:%d, isr:%-2x, buf_io:%d, fc:%lld, buf_adr %x(%x), ch0_adr:%x, sz:(%d,%d), lofs:%d, ts:%lld, diff:%lld\r\n",
						(int)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].sie_id, (unsigned int)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].isr_evt, (int)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].buf_ctl_type,
						ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].frm_cnt, (unsigned int)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].buf_addr, (unsigned int)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].buf_id,
						(unsigned int)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].ch0_addr,
						(int)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].sie_out_size.w, (int)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].sie_out_size.h,
						(int)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].ch0_lofs, (long long)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].ts, (long long)ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].ts - prev_ts[id]);
						prev_ts[id] = ctl_sie_dbg_info[id]->dbg_isr_ioctl[idx].ts;
					}
				};
				memset((void *)(&ctl_sie_dbg_info[id]->dbg_isr_ioctl[0]), 0, sizeof(CTL_SIE_DBG_ISR_IOCTL)*CTL_SIE_DBG_ISR_IOCTL_NUM);
				ctl_sie_dbg_ctl_info[id].dbg_isr_ioctl_cnt = 0;
			}
		}
	}
#endif
	ctl_sie_dbg_buf_ioctl_dump = FALSE;
}

void ctl_sie_dbg_dump_fb_buf_info(INT fd, UINT32 en)
{
	CTL_SIE_STATUS sie_status[CTL_SIE_MAX_SUPPORT_ID];
	CTL_SIE_HDL *sie_hdl[CTL_SIE_MAX_SUPPORT_ID];
	CTL_SIE_LIMIT *sie_limit[CTL_SIE_MAX_SUPPORT_ID];
	UINT32 ctl_id = CTL_SIE_ID_1, strlen;
	CHAR strbuf[256];

	if (en) {
	} else {
		strlen = snprintf(strbuf, sizeof(strbuf), "&fastboot {\r\n\tsie {\r\n");
		vos_file_write(fd, (void *)strbuf, strlen);

		strlen = snprintf(strbuf, sizeof(strbuf), "\t\tdbg {\r\n\t\t\tmsg_en = <0>;\r\n\t\t};\r\n\r\n");
		vos_file_write(fd, (void *)strbuf, strlen);

		sie_limit[CTL_SIE_ID_1] = ctl_sie_limit_query(ctl_id);
		for (ctl_id = CTL_SIE_ID_1; ctl_id < sie_limit[CTL_SIE_ID_1]->max_spt_id; ctl_id++) {
			sie_status[ctl_id] = ctl_sie_get_state_machine(ctl_id);
			sie_hdl[ctl_id] = ctl_sie_get_hdl(ctl_id);
			if (sie_status[ctl_id] == CTL_SIE_STS_CLOSE) {
				continue;
			}
			if (sie_hdl[ctl_id] != NULL) {
				if (sie_hdl[ctl_id]->param.out_dest == CTL_SIE_OUT_DEST_DIRECT) {
					strlen = snprintf(strbuf, sizeof(strbuf), "\t\tsie%d_ctrl {\r\n\t\t\tout0_sz = <0>;\r\n", (int)(ctl_id+1));
				} else {
					strlen = snprintf(strbuf, sizeof(strbuf), "\t\tsie%d_ctrl {\r\n\t\t\tout0_sz = <0x%x>;\r\n", (int)(ctl_id+1), (unsigned int)sie_hdl[ctl_id]->ctrl.buf_info.ch_info[0].data_size);
				}
				vos_file_write(fd, (void *)strbuf, strlen);

				strlen = snprintf(strbuf, sizeof(strbuf), "\t\t\tdupl_src_id = <%d>;\r\n", (int)sie_hdl[ctl_id]->dupl_src_id);
				vos_file_write(fd, (void *)strbuf, strlen);

				strlen = snprintf(strbuf, sizeof(strbuf), "\t\t\tout1_sz = <0x%x>;\r\n\t\t\tout2_sz = <0x%x>;\r\n\t\t\tout_dest = <%d>;\r\n\t\t\tring_buf_en = <%d>;\r\n\t\t\tring_buf_sz = <0x%x>;\r\n\t\t};\r\n\r\n"
					 , (unsigned int)sie_hdl[ctl_id]->ctrl.buf_info.ch_info[1].data_size
					 , (unsigned int)sie_hdl[ctl_id]->ctrl.buf_info.ch_info[2].data_size
					 , (int)sie_hdl[ctl_id]->param.out_dest
					 , (int)sie_hdl[ctl_id]->ctrl.ring_buf_ctl.en
					 , (unsigned int)sie_hdl[ctl_id]->ctrl.ring_buf_ctl.buf_size);
				vos_file_write(fd, (void *)strbuf, strlen);
			}
		}
		kdf_sie_dump_fb_info(fd, en);
		strlen = snprintf(strbuf, sizeof(strbuf), "\t};\r\n};");
		vos_file_write(fd, (void *)strbuf, strlen);
	}
}

void ctl_sie_dbg_dump_proc_time(int (*dump)(const char *fmt, ...))
{
	UINT32 idx = 0, i = 0, pre_idx_ts = 0;
	CHAR* item_str_map[CTL_SIE_PROC_TIME_ITEM_MAX] = {
		"ENTER",
		"EXIT",
	};

	if (dump == NULL) {
		dump = ctl_sie_int_dbg_printf;
	}

	dump("\r\n-----%14s-----\r\n", "sie proc begin");
	dump("%-4s%-20s%-8s%-14s%-14s\r\n", "id", "proc", "item", "clock(us)", "diff(us)");

	for (i = 0; i < CTL_SIE_DBG_TS_MAXNUM; i++) {
		idx = (ctl_sie_dbg_proc_t_cnt + i) % CTL_SIE_DBG_TS_MAXNUM;
		if (ctl_sie_dbg_proc_t_info[idx].time_us != 0) {
			dump("%-4d%-20s%-8s%-14lld%-14lld\r\n", (int)ctl_sie_dbg_proc_t_info[idx].id, ctl_sie_dbg_proc_t_info[idx].proc_name,
				item_str_map[ctl_sie_dbg_proc_t_info[idx].item], (long long)ctl_sie_dbg_proc_t_info[idx].time_us, (long long)(ctl_sie_dbg_proc_t_info[idx].time_us - pre_idx_ts));
			pre_idx_ts = ctl_sie_dbg_proc_t_info[idx].time_us;
		}
	}
}

#if 0
#endif

void ctl_sie_dbg_set_isp_cb_t_log(ISP_ID id, ISP_EVENT evt, UINT64 fc, UINT32 ts_start, UINT32 ts_end)
{
	UINT32 idx;

	if (ctl_sie_isp_dbg_cb_dump) {
		return;
	}

	idx = ctl_sie_isp_dbg_cbtime_cnt % CTL_SIE_DBG_ISP_CB_T_DBG_NUM;
	ctl_sie_isp_dbg_cbtime_log[idx].ispid = id;
	ctl_sie_isp_dbg_cbtime_log[idx].evt = evt;
	ctl_sie_isp_dbg_cbtime_log[idx].raw_fc = fc;
	ctl_sie_isp_dbg_cbtime_log[idx].ts_start = ts_start;
	ctl_sie_isp_dbg_cbtime_log[idx].ts_end = ts_end;
	ctl_sie_isp_dbg_cbtime_cnt++;
}

void ctl_sie_dbg_isp_cb_t_dump(int (*dump)(const char *fmt, ...))
{
	UINT32 i, idx;
	CTL_SIE_DBG_ISP_CB_T_LOG *p_log;

	if (dump == NULL) {
		dump = ctl_sie_int_dbg_printf;
	}
	ctl_sie_isp_dbg_cb_dump = TRUE;
	dump("\r\n----------isp cb proc t----\r\n");
	dump("%-10s%-10s%-16s%-16s%-16s%-16s\r\n", "isp_id", "event", "frm_cnt", "start_time", "end_time", "cb_proc_time(us)");
	for (i = 0; i < CTL_SIE_DBG_ISP_CB_T_DBG_NUM; i++) {
		if (i > ctl_sie_isp_dbg_cbtime_cnt) {
			break;
		}

		idx = (ctl_sie_isp_dbg_cbtime_cnt - i - 1) % CTL_SIE_DBG_ISP_CB_T_DBG_NUM;
		p_log = &ctl_sie_isp_dbg_cbtime_log[idx];
		dump("%#-10x%#-10x%-16lld%-16lld%-16lld%-16lld\r\n",
			(unsigned int)p_log->ispid, (unsigned int)p_log->evt, (long long)p_log->raw_fc,
			(long long)p_log->ts_start,
			(long long)p_log->ts_end,
			(long long)(p_log->ts_end - p_log->ts_start));
	}
	ctl_sie_isp_dbg_cb_dump = FALSE;
}

/*
	ISP debug Function
*/

void ctl_sie_dbg_isp_dump(int (*dump)(const char *fmt, ...))
{
	CHAR *str_status[] = {
		"FREE",
		"USED"
	};
	UINT32 i;
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(CTL_SIE_ID_1);
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();
	CTL_SIE_ISP_HANDLE *isp_hdl = ctl_sie_isp_get_hdl();

	if (dump == NULL) {
		dump = ctl_sie_int_dbg_printf;
	}

	dump("\r\n-----isp info [max_isp_hdl_num = %d]-----\r\n", (int)(sie_limit->max_spt_id));
	dump("%-8s%-8s%-12s%-12s%-32s\r\n", "hdl_idx", "status", "event", "cb_msg", "name");
	for (i = 0; i <= sie_limit->max_spt_id; i++) {
		if (isp_hdl[i].status == CTL_SIE_ISP_HANDLE_STATUS_USED) {
			dump("%-8d%-8s%#-12x%#-12x%-32s\r\n", (int)i, str_status[isp_hdl[i].status], (unsigned int)isp_hdl[i].evt, (unsigned int)isp_hdl[i].cb_msg, isp_hdl[i].name);
		}
	}

	if (dbg_tab != NULL && dbg_tab->isp_cb_t_dump != NULL) {
		dbg_tab->isp_cb_t_dump(dump);
	}
}

void ctl_sie_dbg_isp_set_msg_type(ISP_ID id, CTL_SIE_DBG_ISP_TYPE type, UINT32 par1, UINT32 par2)
{
	CTL_SIE_LIMIT *sie_limit = ctl_sie_limit_query(id);

	if (id > (ISP_ID)sie_limit->max_spt_id) {
		ctl_sie_dbg_err("illegal id %d > %d\r\n", (int)(id), (int)sie_limit->max_spt_id);
		return;
	}

	if (type >= CTL_SIE_DBG_ISP_MAX) {
		ctl_sie_dbg_wrn("id %d: dbg level %d ovf\r\n", (int)(id), (int)(type));
		return;
	}

	switch (type) {
	case CTL_SIE_DBG_ISP_CB_SKIP:
		ctl_sie_isp_set_skip(id, par1, par2);
		break;

	case CTL_SIE_DBG_ISP_OFF:
	default:
		break;
	}

//	ctl_sie_dbg_ind("Set id %d dbg Msg Type to %d\r\n", (int)(id+1), (int)(type));
}
