#include "ctl_sie_int.h"
#include "ctl_sie_event_id_int.h"
#include "ctl_sie_utility_int.h"
#include "ctl_sie_buf_int.h"
#include "ctl_sie_iosize_int.h"
#include "ctl_sie_id_int.h"
#include "ctl_sie_isp_int.h"
#include "kdrv_builtin/sie_init.h"
#include "ctl_sie_debug_int.h"
#include <plat/top.h>

#if 0
#endif

/*
    global variables
*/
static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

static CTL_SIE_HDL_CONTEXT ctl_sie_hdl_ctx = {0};
static BOOL ctl_sie_init_flg = FALSE;
static CTL_SIE_HDL *sie_ctl_info[CTL_SIE_MAX_SUPPORT_ID] = {NULL};
static CTL_SIE_INT_CTL_FLG *sie_ctl_int_flg_ctl[CTL_SIE_MAX_SUPPORT_ID] = {NULL};
static CTL_SIE_STATUS g_ctl_sie_status[CTL_SIE_MAX_SUPPORT_ID] = {0};	//ctrl layer status
CTL_SIE_LIMIT ctl_sie_limit[CTL_SIE_MAX_SUPPORT_ID] = {0};
CTL_SIE_MCLK_INFO ctl_sie_mclk_info[CTL_SIE_MAX_SUPPORT_ID] = {0};
CTL_SIE_INT_DBG_TAB *ctl_sie_dbg_tab = NULL;
CTL_SIE_DBG_LVL ctl_sie_dbg_rt_lvl = CTL_SIE_DBG_LVL_WRN;
BOOL ctl_sie_iosize[CTL_SIE_MAX_SUPPORT_ID] = {FALSE};
UINT32 ctl_sie_dbg_get_raw_id = 0;
CTL_SIE_HEADER_INFO ring_buf_head_info;
void ctl_sie_module_update_load_flg(CTL_SIE_ID id, CTL_SIE_ITEM item);
#if defined(__KERNEL__)
int fastboot = 0;
#define ctl_sie_fb_trig_stage_fastboot 			0x0
#define ctl_sie_fb_trig_stage_normal_open 		0x1
#define ctl_sie_fb_trig_stage_normal_trigger 	0x2
static UINT32 ctl_sie_fb_trig_stage = ctl_sie_fb_trig_stage_fastboot;
#endif
static UINT32 sync_enable_cnt = 0;
static UINT32 sync_enable_bit = 0;
#if 0
#endif

/*
    debug function
*/

CTL_SIE_INT_DBG_TAB *ctl_sie_get_dbg_tab(void)
{
	return ctl_sie_dbg_tab;
}

void ctl_sie_reg_dbg_tab(CTL_SIE_INT_DBG_TAB *dbg_tab)
{
	ctl_sie_dbg_tab = dbg_tab;
}

void ctl_sie_set_dbg_lvl(CTL_SIE_DBG_LVL dbg_lvl)
{
	ctl_sie_dbg_rt_lvl = dbg_lvl;
}

void ctl_sie_set_dbg_proc_t(CTL_SIE_ID id, CHAR *proc_name, CTL_SIE_PROC_TIME_ITEM item)
{
	if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->set_proc_t != NULL) {
		ctl_sie_dbg_tab->set_proc_t(id, proc_name, item);
	};
}

void ctl_sie_set_dbg_isr_ioctl(CTL_SIE_ID id, UINT32 status, UINT32 buf_ctl_type, CTL_SIE_HEAD_IDX head_idx)
{
	if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->set_isr_ioctl != NULL) {
		ctl_sie_dbg_tab->set_isr_ioctl(id, status, buf_ctl_type, head_idx);
	}
}

#if 0
#endif

CTL_SIE_HDL_CONTEXT *ctl_sie_get_ctx(void)
{
	if (ctl_sie_init_flg) {
		return &ctl_sie_hdl_ctx;
	} else {
		return NULL;
	}
}

CTL_SIE_HDL *ctl_sie_get_hdl(CTL_SIE_ID id)
{
	unsigned long flags;
	CTL_SIE_HDL *sie_hdl;

	if (!ctl_sie_module_chk_id_valid(id)) {
		return NULL;
	}

	loc_cpu(flags);
	sie_hdl = sie_ctl_info[id];
	unl_cpu(flags);
	return sie_hdl;
}

CTL_SIE_STATUS ctl_sie_get_state_machine(CTL_SIE_ID id)
{
	unsigned long flags;
	CTL_SIE_STATUS rt;

	if (!ctl_sie_module_chk_id_valid(id)) {
		return CTL_SIE_STS_MAX;
	}

	loc_cpu(flags);
	rt = g_ctl_sie_status[id];
	unl_cpu(flags);

	return rt;
}

INT32 ctl_sie_get_sen_cfg(CTL_SIE_ID id, CTL_SEN_CFGID cfg_id, void *data)
{
	if (!ctl_sie_module_chk_id_valid(id)) {
		return CTL_SIE_E_ID;
	}

	if (sie_ctl_info[id]->p_sen_obj == NULL || sie_ctl_info[id]->p_sen_obj->get_cfg == NULL) {
		ctl_sie_dbg_err("id %d get sen obj error\r\n", (int)(id));
		return CTL_SIE_E_NULL_FP;
	}

	if (sie_ctl_info[id]->p_sen_obj->get_cfg(cfg_id, data) != CTL_SIE_E_OK) {
		return CTL_SIE_E_SEN;
	}
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_set_sen_cfg(CTL_SIE_ID id, CTL_SEN_CFGID cfg_id, void *data)
{
	if (!ctl_sie_module_chk_id_valid(id)) {
		return CTL_SIE_E_ID;
	}

	if (sie_ctl_info[id]->p_sen_obj == NULL || sie_ctl_info[id]->p_sen_obj->set_cfg == NULL) {
		ctl_sie_dbg_err("id %d set sen obj error\r\n", (int)(id));
		return CTL_SIE_E_NULL_FP;
	}

	if (sie_ctl_info[id]->p_sen_obj->set_cfg(cfg_id, data) != CTL_SIE_E_OK) {
		return CTL_SIE_E_SEN;
	}
	return CTL_SIE_E_OK;
}


BOOL ctl_sie_typecast_raw(CTL_SIE_DATAFORMAT src)
{
	if (src == CTL_SIE_BAYER_8 ||
		src == CTL_SIE_BAYER_10 ||
		src == CTL_SIE_BAYER_12 ||
		src == CTL_SIE_BAYER_16) {
		return TRUE;
	}
	return FALSE;
}

BOOL ctl_sie_typecast_ccir(CTL_SIE_DATAFORMAT src)
{
	if (src == CTL_SIE_YUV_422_SPT ||
		src == CTL_SIE_YUV_422_NOSPT ||
		src == CTL_SIE_YUV_420_SPT) {
		return TRUE;
	}
	return FALSE;
}

/*
streaming
    set_imm = TRUE:  set to kdrv imm, latch when next frame(n+1)
    set_imm = FALSE: set to kdrv when next vd, latch when n+2 frame
otherwise
    set to kdrv when trigger start
*/
void ctl_sie_hdl_update_item(CTL_SIE_ID id, UINT64 item, BOOL set_imm)
{
	unsigned long flags;

	vos_flag_iclr(ctl_sie_get_flag_id(id), CTL_SIE_FLG_UP_ITEM_END);
	if (g_ctl_sie_status[id] == CTL_SIE_STS_RUN && set_imm) {
		//set to kdrv_sie and latch when next vd
		if (vos_flag_chk(ctl_sie_get_flag_id(id), CTL_SIE_FLG_CLOSE) & CTL_SIE_FLG_CLOSE) {
		} else {
			kdf_sie_set(sie_ctl_info[id]->kdf_hdl, item, (void *)sie_ctl_info[id]);
		}
		vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_UP_ITEM_END);
		return;
	}
	loc_cpu(flags);
	sie_ctl_info[id]->ctrl.update_item |= item;
	if (g_ctl_sie_status[id] == CTL_SIE_STS_RUN) {
		sie_ctl_int_flg_ctl[id]->isr_load_en_flg = TRUE;
	}
	unl_cpu(flags);
	vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_UP_ITEM_END);
}

void ctl_sie_hdl_load_all(CTL_SIE_ID id)
{
	UINT64 load_item = 0;
	unsigned long flags;

	vos_flag_iclr(ctl_sie_get_flag_id(id), CTL_SIE_FLG_KDRV_END);
	//set to kdrv_sie and latch when next vd
	if (vos_flag_chk(ctl_sie_get_flag_id(id), CTL_SIE_FLG_CLOSE) & CTL_SIE_FLG_CLOSE) {
	} else {
		loc_cpu(flags);
		if (sie_ctl_int_flg_ctl[id]->isr_load_en_flg) {
			sie_ctl_int_flg_ctl[id]->isr_load_en_flg = FALSE;
			//copy run time change parameters to ctl layer
			sie_ctl_info[id]->param = sie_ctl_info[id]->load_param;
			sie_ctl_info[id]->ctrl.rtc_obj = sie_ctl_info[id]->load_ctrl;
			load_item = sie_ctl_info[id]->ctrl.update_item;
			sie_ctl_info[id]->ctrl.update_item = 0;
		}
		unl_cpu(flags);

		kdf_sie_set(sie_ctl_info[id]->kdf_hdl, load_item, (void *)sie_ctl_info[id]);
	}
	vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_KDRV_END);
}

void ctl_sie_update_inte(CTL_SIE_ID id, BOOL enable, CTL_SIE_INTE sts)
{
	if (g_ctl_sie_status[id] != CTL_SIE_STS_CLOSE) {
		if (enable) {
			sie_ctl_info[id]->ctrl.inte |= sts;
		} else {
			//clear inte
			sie_ctl_info[id]->ctrl.inte &= ~sts;
			//get all isp_evt, isr_cb sts and update ctl sie inte
			sie_ctl_info[id]->ctrl.inte |= (sie_ctl_info[id]->ctrl.ext_isrcb_fp.sts | CTL_SIE_DFT_INT_EN | ctl_sie_isp_get_sts());
		}
		ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_INT_ITEM_INTE, TRUE);
	}
}

void ctl_sie_update_fc(CTL_SIE_ID id)
{
	unsigned long flags;
	UINT32 i;

	loc_cpu(flags);
	if (sie_ctl_info[id]->ctrl.total_frame_num > 1) {	//shdr
		if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN) {	//patgen
			//use first group id to update fc
			for (i = CTL_SIE_ID_1; i <= ctl_sie_limit[id].max_spt_id; i++) {
				if ((1 << i) & sie_ctl_info[id]->rtc_ctrl.pat_gen_param.group_idx) {
					if (i == id) {
						sie_ctl_info[id]->ctrl.frame_ctl_info.frame_cnt++;
					}
					unl_cpu(flags);
					return;
				}
			}
		} else if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SEN_IN) {
			sie_ctl_info[id]->ctrl.frame_ctl_info.frame_cnt++;	//shdr + sensor in
		}
	} else {
		sie_ctl_info[id]->ctrl.frame_ctl_info.frame_cnt++;	//linear
	}

	unl_cpu(flags);
}

UINT64 ctl_sie_get_fc(CTL_SIE_ID id)
{
	UINT32 i;

	if (sie_ctl_info[id]->ctrl.total_frame_num > 1) {	//shdr
		if ((sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN)) {	//patgen
			for (i = CTL_SIE_ID_1; i <= ctl_sie_limit[id].max_spt_id; i++) {
				if ((1 << i) & sie_ctl_info[id]->rtc_ctrl.pat_gen_param.group_idx) {
					return sie_ctl_info[i]->ctrl.frame_ctl_info.frame_cnt;
				}
			}
		} else {
			for (i = CTL_SIE_ID_1; i <= ctl_sie_limit[id].max_spt_id; i++) {
				if ((1 << i) & sie_ctl_info[id]->param.chg_senmode_info.output_dest) {
					if (sie_ctl_info[i] != NULL && sie_ctl_info[i]->flow_type == CTL_SIE_FLOW_SEN_IN) {
						return sie_ctl_info[i]->ctrl.frame_ctl_info.frame_cnt;
					}
				}
			}
		}
	}

	return sie_ctl_info[id]->ctrl.frame_ctl_info.frame_cnt;
}

#if 0
#endif

/*
    internal function
*/

static BOOL ctl_sie_module_chk_hw_single_out_en(CTL_SIE_ID id)
{
	CTL_SIE_SINGLE_OUT_CTRL single_out_en = {0};

	kdf_sie_get(sie_ctl_info[id]->kdf_hdl, CTL_SIE_INT_ITEM_SINGLE_OUT_CTL | CTL_SIE_IGN_CHK, (void *)&single_out_en);
	if (single_out_en.ch0_single_out_en || single_out_en.ch1_single_out_en || single_out_en.ch2_single_out_en) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static INT32 ctl_sie_module_hdl_init(UINT32 id)
{
	UINT32 i, hdl_addr;
	unsigned long flags;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();

	loc_cpu(flags);
	//check context
	if (ctl_sie_hdl_ctx.context_used >= ctl_sie_hdl_ctx.context_num) {
		ctl_sie_dbg_err("sie num ovf, cur %d, max %d\r\n", ctl_sie_hdl_ctx.context_used, ctl_sie_hdl_ctx.context_num);
		unl_cpu(flags);
		return CTL_SIE_E_HDL;
	} else {
		for (i = 0; i <= ctl_sie_limit[id].max_spt_id; i++) {
			if (ctl_sie_hdl_ctx.context_idx[i] == CTL_SIE_MAX_SUPPORT_ID) {
				ctl_sie_hdl_ctx.context_idx[i] = id;
				break;
			}
		}
		//start addr
		hdl_addr = ctl_sie_hdl_ctx.start_addr + (i * ctl_sie_hdl_ctx.contex_size);
		//handle buf
		sie_ctl_info[id] = (CTL_SIE_HDL *)hdl_addr;
		hdl_addr += ALIGN_CEIL(sizeof(CTL_SIE_HDL), VOS_ALIGN_BYTES);
		memset((void *)sie_ctl_info[id], 0, ALIGN_CEIL(sizeof(CTL_SIE_HDL), VOS_ALIGN_BYTES));

		//int flg buf
		sie_ctl_int_flg_ctl[id] = (CTL_SIE_INT_CTL_FLG *)hdl_addr;
		hdl_addr += ALIGN_CEIL(sizeof(CTL_SIE_INT_CTL_FLG), VOS_ALIGN_BYTES);
		memset((void *)sie_ctl_int_flg_ctl[id], 0, ALIGN_CEIL(sizeof(CTL_SIE_INT_CTL_FLG), VOS_ALIGN_BYTES));
		sie_ctl_int_flg_ctl[id]->isr_dram_end_flg = 1;
		sie_ctl_int_flg_ctl[id]->cfg_start_bp = CTL_SIE_BP_MIN_END_LINE;

		//ca buf
		sie_ctl_info[id]->ctrl.ca_private_addr = hdl_addr;
		hdl_addr += ALIGN_CEIL(ctl_sie_buf_get_ca_out_size(id), VOS_ALIGN_BYTES);

		//la buf
		sie_ctl_info[id]->ctrl.la_private_addr = hdl_addr;
		hdl_addr += ALIGN_CEIL(ctl_sie_buf_get_la_out_size(id), VOS_ALIGN_BYTES);

		//debug buf
		if (dbg_tab != NULL && dbg_tab->set_dbg_buf != NULL && ctl_sie_hdl_ctx.dbg_buf_alloc){
			dbg_tab->set_dbg_buf(id, hdl_addr);
		}
		ctl_sie_hdl_ctx.context_used++;
	}

	unl_cpu(flags);
#if defined(__KERNEL__)
	if (((ctl_sie_fb_trig_stage & ctl_sie_fb_trig_stage_normal_open) == 0) && (fastboot)) {
		sie_ctl_info[id]->fb_obj.normal_hd_create 	= FALSE;
		sie_ctl_info[id]->fb_obj.normal_shdr_rst 	= FALSE;
		sie_ctl_info[id]->fb_obj.normal_vd 			= FALSE;
		sie_ctl_info[id]->fb_obj.normal_bp 			= FALSE;
		sie_ctl_info[id]->fb_obj.normal_ce			= FALSE;
	} else {
		sie_ctl_info[id]->fb_obj.normal_hd_create 	= TRUE;
		sie_ctl_info[id]->fb_obj.normal_shdr_rst 	= TRUE;
		sie_ctl_info[id]->fb_obj.normal_vd 			= TRUE;
		sie_ctl_info[id]->fb_obj.normal_bp 			= TRUE;
		sie_ctl_info[id]->fb_obj.normal_ce			= TRUE;
	}
#endif
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_module_hdl_uninit(UINT32 id)
{
	UINT32 i;
	unsigned long flags;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();

	loc_cpu(flags);
	ctl_sie_hdl_ctx.context_used--;
	for (i = 0; i <= ctl_sie_limit[id].max_spt_id; i++) {
		if (ctl_sie_hdl_ctx.context_idx[i] == id) {
			ctl_sie_hdl_ctx.context_idx[i] = CTL_SIE_MAX_SUPPORT_ID;
			dbg_tab->set_dbg_buf(i, 0);
			break;
		}
	}
	sie_ctl_info[id] = NULL;
	sie_ctl_int_flg_ctl[id] = NULL;
	unl_cpu(flags);
	return CTL_SIE_E_OK;
}

static void ctl_sie_module_par_set_dft(UINT32 id)
{
	UINT64 update_item = ((1ULL << CTL_SIE_ITEM_ENC_RATE)|(1ULL << CTL_SIE_INT_ITEM_DMA_OUT)|(1ULL << CTL_SIE_INT_ITEM_CH_OUT_MODE));

	sie_ctl_info[id]->ctrl.ch_out_mode.out0_mode = CTL_SIE_SINGLE_OUT;//force enable single out
	sie_ctl_info[id]->ctrl.ch_out_mode.out1_mode = CTL_SIE_SINGLE_OUT;//force enable single out
	sie_ctl_info[id]->ctrl.ch_out_mode.out2_mode = CTL_SIE_SINGLE_OUT;//force enable single out
	sie_ctl_info[id]->ctrl.dma_out = ENABLE;
	sie_ctl_info[id]->rtc_param.enc_rate = CTL_SIE_ENC_58;	//raw compress ratio 58%
	sie_ctl_info[id]->ctrl.bp3_ratio = 0;
	sie_ctl_info[id]->ctrl.bp3_ratio_direct = 0;
	ctl_sie_hdl_update_item(id, update_item, TRUE);
	ctl_sie_iosize_init(id);
	ctl_sie_iosize[id] = FALSE;

	if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SIG_DUPL) {
		sie_ctl_info[id]->rtc_param.ccir_info_int.data_idx = 1;
	} else {
		sie_ctl_info[id]->rtc_param.ccir_info_int.data_idx = 0;
	}
}

static void ctl_sie_module_update_ch0_out_mode(CTL_SIE_ID id)
{
	if (((id == CTL_SIE_ID_2) &&
		 ((sie_ctl_info[id]->rtc_param.out_dest == CTL_SIE_OUT_DEST_DIRECT) || (sie_ctl_info[id]->rtc_param.out_dest == CTL_SIE_OUT_DEST_BOTH)))) {
		if (sie_ctl_info[id]->ctrl.ring_buf_ctl.en == DISABLE) {
			// ctl_sie_module_direct_to_both (not private buffer)
			sie_ctl_info[id]->ctrl.ch_out_mode.out0_mode = CTL_SIE_SINGLE_OUT;
		} else {
			/*
			    case        : HDR + direcet mode
			    condition   : SIE2 DRAM0 used private buffer (ctl_sie_module_ring_buf_malloc), will not runtime allocate buffer fail
			    issue       : avoid "isr shift -> SIE2 DRAM0 not enable SINGLE_OUTPUT_EN -> ife ring buffer error"
			    solution    : DRAM0 force to normal mode
			    note        : DRAM1/2 note private buffer, cannot config to normal mode
			*/
			if (nvt_get_chip_id() == CHIP_NA51055) {
				sie_ctl_info[id]->ctrl.ch_out_mode.out0_mode = CTL_SIE_SINGLE_OUT;
			} else {
				sie_ctl_info[id]->ctrl.ch_out_mode.out0_mode = CTL_SIE_NORMAL_OUT;
			}
		}
		ctl_sie_module_update_load_flg(id, CTL_SIE_INT_ITEM_CH_OUT_MODE);
	}
}

INT32 ctl_sie_module_state_machine(UINT32 id, CTL_SIE_OP op)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_STATUS cur_st = g_ctl_sie_status[id];
	unsigned long flags;

	loc_cpu(flags);
	switch (op) {
	case CTL_SIE_OP_OPEN:
		if (cur_st == CTL_SIE_STS_CLOSE) {
			g_ctl_sie_status[id] = CTL_SIE_STS_READY;
		} else {
			rt = CTL_SIE_E_STATE;
		}
		break;

	case CTL_SIE_OP_CLOSE:
		if (cur_st == CTL_SIE_STS_READY) {
			g_ctl_sie_status[id] = CTL_SIE_STS_CLOSE;
		} else {
			rt = CTL_SIE_E_STATE;
		}
		break;

	case CTL_SIE_OP_TRIG_START:
		if (cur_st == CTL_SIE_STS_READY) {
			g_ctl_sie_status[id] = CTL_SIE_STS_RUN;
		} else {
			rt = CTL_SIE_E_STATE;
		}
		break;

	case CTL_SIE_OP_TRIG_STOP:
		if (cur_st == CTL_SIE_STS_RUN) {
			g_ctl_sie_status[id] = CTL_SIE_STS_READY;
		} else {
			rt = CTL_SIE_E_STATE;
		}
		break;

	case CTL_SIE_OP_SUSPEND:
		if (cur_st == CTL_SIE_STS_RUN || cur_st == CTL_SIE_STS_READY) {
			g_ctl_sie_status[id] = CTL_SIE_STS_SUSPEND;
		} else {
			rt = CTL_SIE_E_STATE;
		}
		break;

	case CTL_SIE_OP_RESUME:
		if (cur_st == CTL_SIE_STS_SUSPEND) {
			g_ctl_sie_status[id] = CTL_SIE_STS_RUN;
		} else {
			rt = CTL_SIE_E_STATE;
		}
		break;

	case CTL_SIE_OP_SET:
	case CTL_SIE_OP_GET:
		if (cur_st == CTL_SIE_STS_CLOSE) {
			rt = CTL_SIE_E_STATE;
		}
		break;

	default:
		break;
	}
	unl_cpu(flags);
	if (rt != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("id %d, err op %d at status %d\r\n", (int)(id), (int)(op), (int)(cur_st));
	}
	return rt;
}

static INT32 ctl_sie_module_cfg_sie_mclk(CTL_SIE_ID id, CTL_SEN_MODE mode)
{
	CTL_SEN_GET_CLK_INFO_PARAM clk_info_param = {0};

	clk_info_param.mode = mode;
	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_CLK_INFO, (void *)&clk_info_param) == CTL_SIE_E_OK) {
		switch (clk_info_param.mclk_src_sel) {
		case CTL_SEN_CLKSRC_480:
			ctl_sie_mclk_info[id].mclk_src_sel = CTL_SIE_MCLK_SRC_480;
			break;

		case CTL_SEN_CLKSRC_PLL4:
			ctl_sie_mclk_info[id].mclk_src_sel = CTL_SIE_MCLK_SRC_PLL4;
			break;

		case CTL_SEN_CLKSRC_PLL5:
			ctl_sie_mclk_info[id].mclk_src_sel = CTL_SIE_MCLK_SRC_PLL5;
			break;

		case CTL_SEN_CLKSRC_PLL10:
			ctl_sie_mclk_info[id].mclk_src_sel = CTL_SIE_MCLK_SRC_PLL10;
			break;

		case CTL_SEN_CLKSRC_PLL12:
			ctl_sie_mclk_info[id].mclk_src_sel = CTL_SIE_MCLK_SRC_PLL12;
			break;

		case CTL_SEN_CLKSRC_PLL18:
			ctl_sie_mclk_info[id].mclk_src_sel = CTL_SIE_MCLK_SRC_PLL18;
			break;

		default:
			ctl_sie_dbg_err("N.S. mclk_src_sel %d\r\n", (int)(clk_info_param.mclk_src_sel));
			ctl_sie_mclk_info[id].mclk_src_sel = CTL_SIE_MCLK_SRC_480;
			break;
		}
		ctl_sie_mclk_info[id].clk_rate = clk_info_param.mclk_freq;
		ctl_sie_mclk_info[id].mclk_id_sel = clk_info_param.mclk_sel;
		return CTL_SIE_E_OK;
	} else {
		ctl_sie_dbg_err("fail\r\n");
		return CTL_SIE_E_SEN;
	}
}

static void ctl_sie_module_get_clk_info(CTL_SIE_ID id, CTL_SEN_MODE mode)
{
	CTL_SEN_GET_CLK_INFO_PARAM clk_info_param = {0};

	sie_ctl_info[id]->ctrl.clk_info.clk_src_sel = CTL_SIE_CLKSRC_DEFAULT;
	clk_info_param.mode = mode;
	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_CLK_INFO, (void *)&clk_info_param) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("get sen clk info fail\r\n");
		sie_ctl_info[id]->ctrl.clk_info.data_rate = ctl_sie_limit[id].max_clk_rate;
		sie_ctl_info[id]->ctrl.clk_info.clk_src_sel = CTL_SIE_CLKSRC_480;
		return;
	}
	sie_ctl_info[id]->ctrl.clk_info.data_rate = clk_info_param.data_rate;
	ctl_sie_module_cfg_sie_mclk(id, mode);
}

_INLINE CTL_SIE_RAW_PIX ctl_sie_module_cal_cfa(CTL_SIE_RAW_PIX src, UPOINT shift)
{
	if ((shift.x % 2) == 1) {
		switch (src) {
		//RGGB Sensor
		case CTL_SIE_RGGB_PIX_R:
			src = CTL_SIE_RGGB_PIX_GR;
			break;

		case CTL_SIE_RGGB_PIX_GR:
			src = CTL_SIE_RGGB_PIX_R;
			break;

		case CTL_SIE_RGGB_PIX_GB:
			src = CTL_SIE_RGGB_PIX_B;
			break;

		case CTL_SIE_RGGB_PIX_B:
			src = CTL_SIE_RGGB_PIX_GB;
			break;

		//RGBIr Sensor
		case CTL_SIE_RGBIR_PIX_RGBG_GIGI:
			src = CTL_SIE_RGBIR_PIX_GBGR_IGIG;
			break;

		case CTL_SIE_RGBIR_PIX_GBGR_IGIG:
			src = CTL_SIE_RGBIR_PIX_BGRG_GIGI;
			break;

		case CTL_SIE_RGBIR_PIX_GIGI_BGRG:
			src = CTL_SIE_RGBIR_PIX_IGIG_GRGB;
			break;

		case CTL_SIE_RGBIR_PIX_IGIG_GRGB:
			src = CTL_SIE_RGBIR_PIX_GIGI_RGBG;
			break;

		case CTL_SIE_RGBIR_PIX_BGRG_GIGI:
			src = CTL_SIE_RGBIR_PIX_GRGB_IGIG;
			break;

		case CTL_SIE_RGBIR_PIX_GRGB_IGIG:
			src = CTL_SIE_RGBIR_PIX_RGBG_GIGI;
			break;

		case CTL_SIE_RGBIR_PIX_GIGI_RGBG:
			src = CTL_SIE_RGBIR_PIX_IGIG_GBGR;
			break;

		case CTL_SIE_RGBIR_PIX_IGIG_GBGR:
			src = CTL_SIE_RGBIR_PIX_GIGI_BGRG;
			break;

		//RCCB Sensor
		case CTL_SIE_RCCB_PIX_RC:
			src = CTL_SIE_RCCB_PIX_CR;
			break;

		case CTL_SIE_RCCB_PIX_CR:
			src = CTL_SIE_RCCB_PIX_RC;
			break;

		case CTL_SIE_RCCB_PIX_CB:
			src = CTL_SIE_RCCB_PIX_BC;
			break;

		case CTL_SIE_RCCB_PIX_BC:
			src = CTL_SIE_RCCB_PIX_CB;
			break;

		default:
			break;
		}
	}

	if ((shift.y % 2) == 1) {
		switch (src) {
		//RGGB Sensor
		case CTL_SIE_RGGB_PIX_R:
			src = CTL_SIE_RGGB_PIX_GB;
			break;

		case CTL_SIE_RGGB_PIX_GR:
			src = CTL_SIE_RGGB_PIX_B;
			break;

		case CTL_SIE_RGGB_PIX_GB:
			src = CTL_SIE_RGGB_PIX_R;
			break;

		case CTL_SIE_RGGB_PIX_B:
			src = CTL_SIE_RGGB_PIX_GR;
			break;

		//RGBIr Sensor
		case CTL_SIE_RGBIR_PIX_RGBG_GIGI:
			src = CTL_SIE_RGBIR_PIX_GIGI_BGRG;
			break;

		case CTL_SIE_RGBIR_PIX_GBGR_IGIG:
			src = CTL_SIE_RGBIR_PIX_IGIG_GRGB;
			break;

		case CTL_SIE_RGBIR_PIX_GIGI_BGRG:
			src = CTL_SIE_RGBIR_PIX_BGRG_GIGI;
			break;

		case CTL_SIE_RGBIR_PIX_IGIG_GRGB:
			src = CTL_SIE_RGBIR_PIX_GRGB_IGIG;
			break;

		case CTL_SIE_RGBIR_PIX_BGRG_GIGI:
			src = CTL_SIE_RGBIR_PIX_GIGI_RGBG;
			break;

		case CTL_SIE_RGBIR_PIX_GRGB_IGIG:
			src = CTL_SIE_RGBIR_PIX_IGIG_GBGR;
			break;

		case CTL_SIE_RGBIR_PIX_GIGI_RGBG:
			src = CTL_SIE_RGBIR_PIX_RGBG_GIGI;
			break;

		case CTL_SIE_RGBIR_PIX_IGIG_GBGR:
			src = CTL_SIE_RGBIR_PIX_GBGR_IGIG;
			break;

		//RCCB Sensor
		case CTL_SIE_RCCB_PIX_RC:
			src = CTL_SIE_RCCB_PIX_CB;
			break;

		case CTL_SIE_RCCB_PIX_CR:
			src = CTL_SIE_RCCB_PIX_BC;
			break;

		case CTL_SIE_RCCB_PIX_CB:
			src = CTL_SIE_RCCB_PIX_RC;
			break;

		case CTL_SIE_RCCB_PIX_BC:
			src = CTL_SIE_RCCB_PIX_CR;
			break;

		default:
			break;
		}
	}

	if (src >= CTL_SIE_PIX_MAX_CNT) {
		src = CTL_SIE_RGGB_PIX_R;
	}
	return src;
}

_INLINE void ctl_sie_module_get_cnt_inc(CTL_SIE_ID id)
{
	FLGPTN wait_flag;

	vos_flag_wait(&wait_flag, ctl_sie_get_flag_id(id), CTL_SIE_FLG_CNT, TWF_CLR|TWF_ORW);
	sie_ctl_int_flg_ctl[id]->get_cnt_flg += 1;
	vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_CNT);
}

_INLINE BOOL ctl_sie_module_get_cnt_dec(CTL_SIE_ID id)
{
	FLGPTN wait_flag;

	vos_flag_wait(&wait_flag, ctl_sie_get_flag_id(id), CTL_SIE_FLG_CNT, TWF_CLR|TWF_ORW);
	if (sie_ctl_int_flg_ctl[id]->get_cnt_flg > 0) {
	    sie_ctl_int_flg_ctl[id]->get_cnt_flg -= 1;
	} else {
		ctl_sie_dbg_err("loc_cnt err id %d, cnt %d\r\n", (int)(id), (int)(sie_ctl_int_flg_ctl[id]->get_cnt_flg));
	}
	vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_CNT);

	if (sie_ctl_int_flg_ctl[id]->get_cnt_flg == 0) {
		return TRUE;
	}
	return FALSE;
}

_INLINE void ctl_sie_module_lock(CTL_SIE_ID id, FLGPTN flag, BOOL lock_en)
{
	FLGPTN wait_flag;

	if (lock_en) {
		vos_flag_wait(&wait_flag, ctl_sie_get_flag_id(id), flag, TWF_CLR|TWF_ANDW);
	} else {
		vos_flag_iset(ctl_sie_get_flag_id(id), flag);
	}
}

_INLINE void ctl_sie_module_get_lock(CTL_SIE_ID id, BOOL lock_en)
{
	FLGPTN wait_flag;

	if (lock_en) {
	    vos_flag_wait(&wait_flag, ctl_sie_get_flag_id(id), CTL_SIE_FLG_LOCK, TWF_ORW);
		vos_flag_clr(ctl_sie_get_flag_id(id), CTL_SIE_FLG_END);
		ctl_sie_module_get_cnt_inc(id);
	} else {
		if (ctl_sie_module_get_cnt_dec(id)) {
			vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_END);
		}
	}
}

_INLINE void ctl_sie_module_wait_get_end(CTL_SIE_ID id)
{
	FLGPTN wait_flag;

	vos_flag_wait(&wait_flag, ctl_sie_get_flag_id(id), CTL_SIE_FLG_END, TWF_ANDW);
}

//update load flag and load to kdrv_sie in next vd after set_load
void ctl_sie_module_update_load_flg(CTL_SIE_ID id, CTL_SIE_ITEM item)
{
	unsigned long flags;

	loc_cpu(flags);
	sie_ctl_int_flg_ctl[id]->set_load_flg |= (1ULL << item);
	unl_cpu(flags);
}

_INLINE void ctl_sie_module_update_crp_cfa(CTL_SIE_ID id)
{
	UPOINT shift = {0};
	unsigned long flags;

	loc_cpu(flags);
	//update crop cfa, keep in kflow for debug, kdrv will auto calc. crop cfapat
	shift.x = sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp.x;
	shift.y = sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp.y;
	sie_ctl_info[id]->rtc_ctrl.rawcfa_crp = ctl_sie_module_cal_cfa(sie_ctl_info[id]->rtc_ctrl.rawcfa_act, shift);
	unl_cpu(flags);
}

static void ctl_sie_module_update_bp3(CTL_SIE_ID id, UINT32 bp3_value, BOOL set_imm)
{
	//maximum bp value to crop end
	UINT32 bp_base = sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp.h;
	//minimum bp value to crop start
	UINT32 bp_to_crp_start = sie_ctl_info[id]->rtc_ctrl.sie_act_win.y + sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp.y;
	UINT32 line_num = 0;
	UINT32 bp3_ratio = 0;

	if (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DIRECT) {
		bp3_ratio = sie_ctl_info[id]->ctrl.bp3_ratio_direct;
	} else {
		bp3_ratio = sie_ctl_info[id]->ctrl.bp3_ratio;
	}

	if (bp3_value == 0) {//auto bp3
		if (bp3_ratio == 0) {
			//set bp3(bp3 need after sie frame start) for direct mode case
			if (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DIRECT) {
				sie_ctl_info[id]->ctrl.bp3 = bp_to_crp_start + bp_base/2;
			} else {
				if (sie_ctl_info[id]->ctrl.row_time == 0) {
					line_num = CTL_SIE_BP_MIN_END_LINE;
				} else {
				    line_num = CTL_SIE_BP_MIN_END_TIME/sie_ctl_info[id]->ctrl.row_time;
				}
				// line_num = 3ms/line_time, set bp3 to (act_y + crp_y + crp_h - line_num)
				sie_ctl_info[id]->ctrl.bp3 = bp_to_crp_start + bp_base - line_num;
			}
		} else {
			sie_ctl_info[id]->ctrl.bp3 = bp_to_crp_start + (bp3_ratio * bp_base)/100;
		}
	} else {
		//manual bp3 set
		sie_ctl_info[id]->ctrl.bp3 = bp3_value;
	}

	//bp3 valid value check
	if ((sie_ctl_info[id]->ctrl.bp3 == 0) || (sie_ctl_info[id]->ctrl.bp3 > (bp_to_crp_start + bp_base))) {
		ctl_sie_dbg_wrn("bp3 error , force set to %d\r\n", (int)(bp_to_crp_start + (bp_base >> 1)));
		// if (act_y + crp_y + crp_h) <= line_num, set bp3 to (act_y + crp_y + (crp_h/2))
		sie_ctl_info[id]->ctrl.bp3 = bp_to_crp_start + (bp_base >> 1);
	}

	if (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DIRECT) {
		if (sie_ctl_info[id]->ctrl.bp3 < (bp_to_crp_start + CTL_SIE_BP_STR_LINE_DIRECT_MIN)) {
			sie_ctl_info[id]->ctrl.bp3 = bp_to_crp_start + CTL_SIE_BP_STR_LINE_DIRECT_MIN;
			ctl_sie_dbg_wrn("direct mode bp3 minimum is crop_start + %d\r\n",CTL_SIE_BP_STR_LINE_DIRECT_MIN);
		}
	}

	if (set_imm) {
		ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_INT_ITEM_BP3, TRUE);
	} else {
		ctl_sie_module_update_load_flg(id, CTL_SIE_INT_ITEM_BP3);
	}
}

static INT32 ctl_sie_module_chk_support_sen_fmt(CTL_SIE_ID id, CTL_SEN_DATA_FMT fmt)
{
	INT32 rt = CTL_SIE_E_OK;

	switch (fmt) {
	case CTL_SEN_DATA_FMT_RGB:
		if (!(ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_RGGB_FMT_SEL)) {
			rt = CTL_SIE_E_NOSPT;
		}
		break;

	case CTL_SEN_DATA_FMT_RGBIR:
		if (!(ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_RGBIR_FMT_SEL)) {
			rt = CTL_SIE_E_NOSPT;
		}
		break;

	case CTL_SEN_DATA_FMT_RCCB:
		if (!(ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_RCCB_FMT_SEL)) {
			rt = CTL_SIE_E_NOSPT;
		}
		break;

	case CTL_SEN_DATA_FMT_YUV:
	case CTL_SEN_DATA_FMT_Y_ONLY:
		if (!(ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_DVI)) {
			rt = CTL_SIE_E_NOSPT;
		}
		break;

	case CTL_SEN_DATA_FMT_MAX_NUM:
	default:
		rt = CTL_SIE_E_NOSPT;
		break;
	}

	return rt;
}

static INT32 ctl_sie_module_cfg_sen_rawcfa(CTL_SIE_ID id, CTL_SEN_GET_MODE_BASIC_PARAM *sen_basic_info)
{
	if(!ctl_sie_module_chk_is_raw(sen_basic_info->mode_type)) {
		return CTL_SIE_E_OK;
	}

	switch (sen_basic_info->stpix) {
	case CTL_SEN_STPIX_R:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGGB_PIX_R;
		break;
	case CTL_SEN_STPIX_GR:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGGB_PIX_GR;
		break;
	case CTL_SEN_STPIX_GB:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGGB_PIX_GB;
		break;
	case CTL_SEN_STPIX_B:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGGB_PIX_B;
		break;
	case CTL_SEN_STPIX_RGBIR_RGBG_GIGI:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGBIR_PIX_RGBG_GIGI;
		break;
	case CTL_SEN_STPIX_RGBIR_GBGR_IGIG:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGBIR_PIX_GBGR_IGIG;
		break;
	case CTL_SEN_STPIX_RGBIR_GIGI_BGRG:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGBIR_PIX_GIGI_BGRG;
		break;
	case CTL_SEN_STPIX_RGBIR_IGIG_GRGB:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGBIR_PIX_IGIG_GRGB;
		break;
	case CTL_SEN_STPIX_RGBIR_BGRG_GIGI:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGBIR_PIX_BGRG_GIGI;
		break;
	case CTL_SEN_STPIX_RGBIR_GRGB_IGIG:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGBIR_PIX_GRGB_IGIG;
		break;
	case CTL_SEN_STPIX_RGBIR_GIGI_RGBG:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGBIR_PIX_GIGI_RGBG;
		break;
	case CTL_SEN_STPIX_RGBIR_IGIG_GBGR:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGBIR_PIX_IGIG_GBGR;
		break;
	case CTL_SEN_STPIX_RCCB_RC:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RCCB_PIX_RC;
		break;
	case CTL_SEN_STPIX_RCCB_CR:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RCCB_PIX_CR;
		break;
	case CTL_SEN_STPIX_RCCB_CB:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RCCB_PIX_CB;
		break;
	case CTL_SEN_STPIX_RCCB_BC:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RCCB_PIX_BC;
		break;
	case CTL_SEN_STPIX_Y_ONLY:
		sie_ctl_info[id]->rtc_ctrl.rawcfa_act = CTL_SIE_RGGB_PIX_R;
		break;

	default:
		ctl_sie_dbg_err("N.S. %d\r\n", (int)(sen_basic_info->stpix));
		return CTL_SIE_E_NOSPT;
	}

	ctl_sie_module_update_load_flg(id, CTL_SIE_ITEM_IO_SIZE);

	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_module_cfg_sen_signal(CTL_SIE_ID id)
{
	CTL_SEN_GET_ATTR_PARAM attr_param = {0};

	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_ATTR, (void *)&attr_param) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("get sen attr fail\r\n");
		return CTL_SIE_E_SEN;
	}

	if (attr_param.signal_info.vd_inv == CTL_SEN_ACTIVE_HIGH) {
		sie_ctl_info[id]->rtc_ctrl.signal.b_vd_inverse = FALSE;
	} else {
		sie_ctl_info[id]->rtc_ctrl.signal.b_vd_inverse = TRUE;
	}

	if (attr_param.signal_info.hd_inv == CTL_SEN_ACTIVE_HIGH) {
		sie_ctl_info[id]->rtc_ctrl.signal.b_hd_inverse = FALSE;
	} else {
		sie_ctl_info[id]->rtc_ctrl.signal.b_hd_inverse = TRUE;
	}

	if (attr_param.signal_info.vd_phase == CTL_SEN_PHASE_RISING) {
		sie_ctl_info[id]->rtc_ctrl.signal.vd_phase = CTL_SIE_PHASE_RISING;
	} else {
		sie_ctl_info[id]->rtc_ctrl.signal.vd_phase = CTL_SIE_PHASE_FALLING;
	}

	if (attr_param.signal_info.hd_phase == CTL_SEN_PHASE_RISING) {
		sie_ctl_info[id]->rtc_ctrl.signal.hd_phase = CTL_SIE_PHASE_RISING;
	} else {
		sie_ctl_info[id]->rtc_ctrl.signal.hd_phase = CTL_SIE_PHASE_FALLING;
	}

	if (attr_param.signal_info.data_phase == CTL_SEN_PHASE_RISING) {
		sie_ctl_info[id]->rtc_ctrl.signal.data_phase = CTL_SIE_PHASE_RISING;
	} else {
		sie_ctl_info[id]->rtc_ctrl.signal.data_phase = CTL_SIE_PHASE_FALLING;
	}

	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_module_cfg_sen_act_win(CTL_SIE_ID id, CTL_SEN_GET_MODE_BASIC_PARAM *sen_basic_info)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SEN_GET_DVI_PARAM mode_dvi_param = {0};
	UINT32 i = 0, mutil_frm_idx = 0;

	if (sen_basic_info->frame_num > 1) {
		for (i = CTL_SIE_ID_1; i <= id; i++) {
			if ((sie_ctl_info[i] != NULL) && ((1 << i) & sie_ctl_info[id]->rtc_param.chg_senmode_info.output_dest)) {
				sie_ctl_info[id]->rtc_ctrl.sie_act_win = sen_basic_info->act_size[mutil_frm_idx];
				sie_ctl_info[id]->rtc_ctrl.sie_act_win.w = ALIGN_FLOOR(sie_ctl_info[id]->rtc_ctrl.sie_act_win.w, ctl_sie_limit[id].act_win_align.w);
				sie_ctl_info[id]->rtc_ctrl.sie_act_win.h = ALIGN_FLOOR(sie_ctl_info[id]->rtc_ctrl.sie_act_win.h, ctl_sie_limit[id].act_win_align.h);
				if (sen_basic_info->act_size[mutil_frm_idx].w != sie_ctl_info[id]->rtc_ctrl.sie_act_win.w || sen_basic_info->act_size[mutil_frm_idx].h != sie_ctl_info[id]->rtc_ctrl.sie_act_win.h) {
					ctl_sie_dbg_wrn("org act win (%d, %d)-->cur (%d, %d), align limit (%d, %d)\r\n",
					(int)sen_basic_info->act_size[mutil_frm_idx].w, (int)sen_basic_info->act_size[mutil_frm_idx].h,
					(int)sie_ctl_info[id]->rtc_ctrl.sie_act_win.w, (int)sie_ctl_info[id]->rtc_ctrl.sie_act_win.h,
					(int)ctl_sie_limit[id].act_win_align.w, (int)ctl_sie_limit[id].act_win_align.h);
				}
				mutil_frm_idx++;
			}
		}
	} else {
		sie_ctl_info[id]->rtc_ctrl.sie_act_win = sen_basic_info->act_size[0];
		sie_ctl_info[id]->rtc_ctrl.sie_act_win.w = ALIGN_FLOOR(sie_ctl_info[id]->rtc_ctrl.sie_act_win.w, ctl_sie_limit[id].act_win_align.w);
		sie_ctl_info[id]->rtc_ctrl.sie_act_win.h = ALIGN_FLOOR(sie_ctl_info[id]->rtc_ctrl.sie_act_win.h, ctl_sie_limit[id].act_win_align.h);
	}

	if (!ctl_sie_module_chk_is_raw(sen_basic_info->mode_type)) {
		mode_dvi_param.mode = sen_basic_info->mode;
		if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_DVI, (void *)&mode_dvi_param) != CTL_SIE_E_OK) {
			ctl_sie_dbg_wrn("get dvi fail\r\n");
			rt = CTL_SIE_E_SEN;
		}
		if (mode_dvi_param.data_mode == CTL_SEN_DVI_DATA_MODE_SD) {
			// sie act size == sensor driver act size
		} else if (mode_dvi_param.data_mode == CTL_SEN_DVI_DATA_MODE_HD) {
			sie_ctl_info[id]->rtc_ctrl.sie_act_win.w = sie_ctl_info[id]->rtc_ctrl.sie_act_win.w * 2;
		} else {
			ctl_sie_dbg_err("get sen dvi data_mode err %d\r\n", (int)(mode_dvi_param.data_mode));
		}

	}
	ctl_sie_module_update_load_flg(id, CTL_SIE_ITEM_IO_SIZE);

	return rt;
}

static INT32 ctl_sie_module_cfg_sen_ccir(CTL_SIE_ID id, CTL_SEN_GET_MODE_BASIC_PARAM *sen_basic_info)
{
	CTL_SEN_GET_DVI_PARAM dvi_param = {0};
	CTL_SEN_GET_CMDIF_PARAM cmdif_param = {0};
	CTL_SEN_GET_IF_PARAM sen_if = {0};

	if (ctl_sie_module_chk_is_raw(sen_basic_info->mode_type)) {
		return CTL_SIE_E_OK;
	}

	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_CMDIF, (void *)&cmdif_param) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("get CMDIF fail\r\n");
		return CTL_SIE_E_SEN;
	}

	dvi_param.mode = sen_basic_info->mode;
	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_DVI, (void *)&dvi_param) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("get DVI fail\r\n");
		return CTL_SIE_E_SEN;
	}

	sen_if.mode = sen_basic_info->mode;
	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_IF, (void *)&sen_if) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("get IF fail\r\n");
		return CTL_SIE_E_SEN;
	}

	switch (dvi_param.fmt) {
	case CTL_SEN_DVI_CCIR601:
	case CTL_SEN_DVI_CCIR709:
	case CTL_SEN_DVI_CCIR601_1120:
		sie_ctl_info[id]->rtc_param.ccir_info_int.fmt = CTL_DVI_FMT_CCIR601;
		break;

	case CTL_SEN_DVI_CCIR656_EAV:
	case CTL_SEN_DVI_CCIR656_1120_EAV:
		sie_ctl_info[id]->rtc_param.ccir_info_int.fmt = CTL_DVI_FMT_CCIR656_EAV;
		break;

	case CTL_SEN_DVI_CCIR656_ACT:
	case CTL_SEN_DVI_CCIR656_1120_ACT:
		sie_ctl_info[id]->rtc_param.ccir_info_int.fmt = CTL_DVI_FMT_CCIR656_ACT;
		break;

	default:
		sie_ctl_info[id]->rtc_param.ccir_info_int.fmt = CTL_DVI_FMT_CCIR601;
		ctl_sie_dbg_err("dvi fmt err %d\r\n", (int)(dvi_param.fmt));
		break;
	}

	if (cmdif_param.type == CTL_SEN_CMDIF_TYPE_VX1) {
		// vx1 must be HD mode
		if (dvi_param.msblsb_switch) {
			sie_ctl_info[id]->rtc_param.ccir_info_int.dvi_mode = CTL_DVI_MODE_HD_INV;
		} else {
			sie_ctl_info[id]->rtc_param.ccir_info_int.dvi_mode = CTL_DVI_MODE_HD;
		}
	} else {
		if (dvi_param.data_mode == CTL_SEN_DVI_DATA_MODE_SD) {
			sie_ctl_info[id]->rtc_param.ccir_info_int.dvi_mode = CTL_DVI_MODE_SD;
			if (dvi_param.msblsb_switch) {
				ctl_sie_dbg_err("only HD spt msblsb_switch\r\n");
			}
		} else if (dvi_param.data_mode == CTL_SEN_DVI_DATA_MODE_HD) {
			if (dvi_param.msblsb_switch) {
				sie_ctl_info[id]->rtc_param.ccir_info_int.dvi_mode = CTL_DVI_MODE_HD_INV;
			} else {
				sie_ctl_info[id]->rtc_param.ccir_info_int.dvi_mode = CTL_DVI_MODE_HD;
			}
		} else {
			ctl_sie_dbg_err("dvi data_mode err %d\r\n", (int)(dvi_param.data_mode));
		}
	}

	switch (sen_basic_info->stpix) {
	case CTL_SEN_STPIX_Y_ONLY:
	case CTL_SEN_STPIX_YUV_YUYV:
		sie_ctl_info[id]->rtc_param.ccir_info_int.yuv_order = CTL_SIE_YUYV;
		break;
	case CTL_SEN_STPIX_YUV_YVYU:
		sie_ctl_info[id]->rtc_param.ccir_info_int.yuv_order = CTL_SIE_YVYU;
		break;
	case CTL_SEN_STPIX_YUV_UYVY:
		sie_ctl_info[id]->rtc_param.ccir_info_int.yuv_order = CTL_SIE_UYVY;
		break;
	case CTL_SEN_STPIX_YUV_VYUY:
		sie_ctl_info[id]->rtc_param.ccir_info_int.yuv_order = CTL_SIE_VYUY;
		break;
	default:
		sie_ctl_info[id]->rtc_param.ccir_info_int.yuv_order = CTL_SIE_YUYV;
		ctl_sie_dbg_err("sen stpix err %d\r\n", (int)(sen_basic_info->stpix));
		break;
	}


	if (sen_basic_info->mode_type == CTL_SEN_MODE_CCIR) {
		sie_ctl_info[id]->rtc_param.ccir_info_int.b_ccir656_vd_sel = 1;
	} else if (sen_basic_info->mode_type == CTL_SEN_MODE_CCIR_INTERLACE) {
		//force set sie to progress mode, and skip 1/2 frame
//		sie_ctl_info[id]->rtc_param.ccir_info_int.b_ccir656_vd_sel = 0;
		sie_ctl_info[id]->dvi_interlace_skip_en = 1;
		sie_ctl_info[id]->rtc_param.ccir_info_int.b_ccir656_vd_sel = 1;
	} else {
		ctl_sie_dbg_err("sen mode_type err %d\r\n", (int)(sen_basic_info->mode_type));
	}

	sie_ctl_info[id]->rtc_param.ccir_info_int.b_auto_align = TRUE; ///< SIE DRV suggest value
	if (sen_if.type == CTL_SEN_IF_TYPE_PARALLEL) {
		switch (sen_if.info.parallel.mux_info.mux_data_num) {
		case 1:
			sie_ctl_info[id]->rtc_param.ccir_info_int.data_period = 0;
			break;

		case 2:
			sie_ctl_info[id]->rtc_param.ccir_info_int.data_period = 1;
			break;

		default:
			ctl_sie_dbg_err("N.S. mux_data_num %d\r\n", (int)sen_if.info.parallel.mux_info.mux_data_num);
			sie_ctl_info[id]->rtc_param.ccir_info_int.data_period = 0;
			break;
		}
	} else {
		sie_ctl_info[id]->rtc_param.ccir_info_int.data_period = 0;
	}
	ctl_sie_module_update_load_flg(id, CTL_SIE_ITEM_CCIR);

	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_module_cfg_sen_pxclk(CTL_SIE_ID id, CTL_SEN_GET_MODE_BASIC_PARAM *sen_basic_info)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SEN_GET_ATTR_PARAM attr_param = {0};

	sie_ctl_info[id]->ctrl.clk_info.pclk_src_sel = CTL_SIE_PXCLKSRC_PAD;
	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_ATTR, (void *)&attr_param) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("get sen attr fail\r\n");
		rt = CTL_SIE_E_SEN;
	} else {
		if (attr_param.cmdif_type == CTL_SEN_CMDIF_TYPE_VX1) {
			/*
				KDRV_SSENIFVX1_DATAMUX_3BYTE_RAW/KDRV_SSENIFVX1_DATAMUX_4BYTE_RAW/KDRV_SSENIFVX1_DATAMUX_YUV16: CTL_SIE_PXCLKSRC_VX1_2X
				KDRV_SSENIFVX1_DATAMUX_YUV8/KDRV_SSENIFVX1_DATAMUX_NONEMUXING: CTL_SIE_PXCLKSRC_VX1_1X
			*/
			sie_ctl_info[id]->ctrl.clk_info.pclk_src_sel = CTL_SIE_PXCLKSRC_VX1_2X;

			if ((!ctl_sie_module_chk_is_raw(sen_basic_info->mode_type)) &&	(sen_basic_info->pixel_depth == CTL_SEN_PIXDEPTH_8BIT)) {
				// yuv 8 bit
				sie_ctl_info[id]->ctrl.clk_info.pclk_src_sel = CTL_SIE_PXCLKSRC_VX1_1X;
			}
		} else {
			if ((attr_param.if_type == CTL_SEN_IF_TYPE_PARALLEL) && (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SIG_DUPL)) {
				if ((id == CTL_SIE_ID_3) && (sie_ctl_info[id]->dupl_src_id == CTL_SIE_ID_2)) {
					sie_ctl_info[id]->ctrl.clk_info.pclk_src_sel = CTL_SIE_PXCLKSRC_PAD;	//for NT98520 limitation
				} else {
					sie_ctl_info[id]->ctrl.clk_info.pclk_src_sel = CTL_SIE_PXCLKSRC_PAD_AHD; // use 2-pixel mux
				}
			}
		}
	}

	if (g_ctl_sie_status[id] != CTL_SIE_STS_RUN) {
		ctl_sie_module_update_load_flg(id, CTL_SIE_INT_ITEM_PXCLK);
	}

	return rt;
}
static INT32 ctl_sie_module_cfg_sen(CTL_SIE_ID id, CTL_SEN_MODE mode)
{
	INT32 rt = CTL_SIE_E_OK, rt_val = CTL_SIE_E_OK;
	CTL_SEN_GET_MODE_BASIC_PARAM mode_basic_param = {0};

	mode_basic_param.mode = mode;
	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_MODE_BASIC, (void *)&mode_basic_param) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("id: %d get sen mode basic fail\r\n", (int)id);
		return CTL_SIE_E_SEN;
	}

	sie_ctl_info[id]->ctrl.total_frame_num = mode_basic_param.frame_num;
	if (ctl_sie_module_chk_support_sen_fmt(id, mode_basic_param.data_fmt) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("N.S. sen data fmt\r\n");
	}

	rt = ctl_sie_module_cfg_sen_rawcfa(id, &mode_basic_param);
	if (rt != CTL_SIE_E_OK) {
		rt_val = rt;
	}
	rt = ctl_sie_module_cfg_sen_signal(id);
	if (rt != CTL_SIE_E_OK) {
		rt_val = rt;
	}
	rt = ctl_sie_module_cfg_sen_act_win(id, &mode_basic_param);
	if (rt != CTL_SIE_E_OK) {
		rt_val = rt;
	}
	rt = ctl_sie_module_cfg_sen_ccir(id, &mode_basic_param);
	if (rt != CTL_SIE_E_OK) {
		rt_val = rt;
	}
	rt = ctl_sie_module_cfg_sen_pxclk(id, &mode_basic_param);
	if (rt != CTL_SIE_E_OK) {
		rt_val = rt;
	}
	ctl_sie_module_get_clk_info(id, mode);

	sie_ctl_info[id]->ctrl.row_time = mode_basic_param.row_time;
	return rt_val;
}

static INT32 ctl_sie_module_cfg_io_size(CTL_SIE_ID id)
{
	CTL_SEN_GET_MODE_BASIC_PARAM sen_mode_param = {0};

	/* get sensor information */
	sen_mode_param.mode = sie_ctl_info[id]->rtc_param.chg_senmode_info.cfg_sen_mode;
	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_MODE_BASIC, (void *)&sen_mode_param) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("get sen info err\r\n");
		return CTL_SIE_E_SEN;
	}

	sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp = ctl_sie_iosize_get_obj()->get_sie_crp_win(id, &sen_mode_param);
	sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_scl_out = ctl_sie_iosize_get_obj()->get_sie_scl_sz(id, &sen_mode_param);
	sie_ctl_info[id]->rtc_param.io_size_info.size_info.dest_crp = ctl_sie_iosize_get_obj()->get_dest_crp_win(id, &sen_mode_param);
	//force set loadbit for ccir datafmt update
	ctl_sie_module_update_load_flg(id, CTL_SIE_ITEM_IO_SIZE);
	if (ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_CA && sie_ctl_info[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AWB] && sie_ctl_info[id]->ctrl.ca_roi.roi_base != 0) {
		ctl_sie_module_update_load_flg(id, CTL_SIE_INT_ITEM_CA_ROI);
	}
	if (ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_LA && sie_ctl_info[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AE] && sie_ctl_info[id]->ctrl.la_roi.roi_base != 0) {
		ctl_sie_module_update_load_flg(id, CTL_SIE_INT_ITEM_LA_ROI);
	}
	return CTL_SIE_E_OK;
}

static CTL_SEN_MODE ctl_sie_module_get_sen_mode(CTL_SIE_ID id)
{
	CTL_SEN_GET_MODESEL_PARAM modesel_param = {0};
	CTL_SEN_GET_ATTR_PARAM attr_param = {0};
	UINT32 sen_mode = CTL_SEN_MODE_1;

	if (sie_ctl_info[id]->rtc_param.chg_senmode_info.sel == CTL_SEN_MODESEL_MANUAL) {
		if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_ATTR, (void *)&attr_param) == CTL_SIE_E_OK) {
			if (sie_ctl_info[id]->rtc_param.chg_senmode_info.manual_info.sen_mode <= attr_param.max_senmode) {
				sen_mode = sie_ctl_info[id]->rtc_param.chg_senmode_info.manual_info.sen_mode;
			} else {
				ctl_sie_dbg_err("manual sen mode %d > max %d\r\n", sie_ctl_info[id]->rtc_param.chg_senmode_info.manual_info.sen_mode, attr_param.max_senmode);
				sen_mode = CTL_SEN_MODE_1;
			}
		} else {
			ctl_sie_dbg_err("get sen attr fail\r\n");
			sen_mode = CTL_SEN_MODE_1;
		}
	} else if (sie_ctl_info[id]->rtc_param.chg_senmode_info.sel == CTL_SEN_MODESEL_AUTO) {
		modesel_param.frame_rate = sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.frame_rate;
		modesel_param.frame_num = sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.frame_num;
		modesel_param.size = sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.size;
		modesel_param.data_fmt = sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.data_fmt;
		modesel_param.pixdepth = sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.pixdepth;
		modesel_param.ccir = sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.ccir;
		modesel_param.mux_singnal_en = sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.mux_singnal_en;
		modesel_param.mux_signal_info = sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.mux_signal_info;
		modesel_param.mode_type_sel = sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.mode_type_sel;
		modesel_param.data_lane = sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.data_lane;

		if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_MODESEL, (void *)&modesel_param) == CTL_SIE_E_OK) {
			sen_mode = modesel_param.mode;
		} else {
			ctl_sie_dbg_err("sen mode auto sel fail\r\n");
			if (modesel_param.mode != CTL_SEN_MODE_UNKNOWN) {
				sen_mode = modesel_param.mode;
			}
		}
	}
	return sen_mode;
}

static CTL_SIE_ACT_MODE ctl_sie_module_get_actmode(CTL_SIE_ID id, CTL_SIE_ID dupl_id)
{
	CTL_SEN_GET_ATTR_PARAM attr_param = {0};
	CTL_SEN_GET_CMDIF_PARAM cmdif_param = {0};
	CTL_SIE_ACT_MODE act_mode = CTL_SIE_IN_PARA_MSTR_SNR;

	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_ATTR, (void *)&attr_param) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("get sen attr fail\r\n");
		return act_mode;
	}

	if (attr_param.cmdif_type == CTL_SEN_CMDIF_TYPE_VX1) {
		act_mode = CTL_SIE_IN_VX1_IF0_SNR;
		if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_CMDIF, (void *)&cmdif_param) == CTL_SIE_E_OK) {
			if (cmdif_param.vx1.if_sel == CTL_SEN_VX1_IF_SEL_0) {
				act_mode = CTL_SIE_IN_VX1_IF0_SNR;
			} else if (cmdif_param.vx1.if_sel == CTL_SEN_VX1_IF_SEL_1) {
				act_mode = CTL_SIE_IN_VX1_IF1_SNR;
			} else {
				ctl_sie_dbg_err("if_sel %d err\r\n", (int)(cmdif_param.vx1.if_sel));
			}
		}
	} else {
		switch (attr_param.if_type) {
		case CTL_SEN_IF_TYPE_PARALLEL:
			if (attr_param.signal_type == CTL_SEN_SIGNAL_MASTER) {
				act_mode = CTL_SIE_IN_PARA_MSTR_SNR;
			} else {
				act_mode = CTL_SIE_IN_PARA_SLAV_SNR;
			}
			break;

		case CTL_SEN_IF_TYPE_MIPI:
			switch (CTL_SEN_DRVDEV_MASK_CSI(attr_param.drvdev)) {	//replace by csi_id get from ctl_sen
			case CTL_SEN_DRVDEV_CSI_0:
				act_mode = CTL_SIE_IN_CSI_1;
				break;

			case CTL_SEN_DRVDEV_CSI_1:
				act_mode = CTL_SIE_IN_CSI_2;
				break;

			case CTL_SEN_DRVDEV_CSI_2:
				act_mode = CTL_SIE_IN_CSI_3;
				break;

			case CTL_SEN_DRVDEV_CSI_3:
				act_mode = CTL_SIE_IN_CSI_4;
				break;

			case CTL_SEN_DRVDEV_CSI_4:
				act_mode = CTL_SIE_IN_CSI_5;
				break;

			case CTL_SEN_DRVDEV_CSI_5:
				act_mode = CTL_SIE_IN_CSI_6;
				break;

			case CTL_SEN_DRVDEV_CSI_6:
				act_mode = CTL_SIE_IN_CSI_7;
				break;

			case CTL_SEN_DRVDEV_CSI_7:
				act_mode = CTL_SIE_IN_CSI_8;
				break;

			default:
				ctl_sie_dbg_err("drvdev id error 0x%.8x\r\n", (unsigned int)(attr_param.drvdev));
				act_mode = CTL_SIE_IN_CSI_1;
				break;
			}
			break;

		case CTL_SEN_IF_TYPE_LVDS:
			switch (CTL_SEN_DRVDEV_MASK_LVDS(attr_param.drvdev)) {	//replace by lvds_id get from ctl_sen
			case CTL_SEN_DRVDEV_LVDS_0:
				act_mode = CTL_SIE_IN_CSI_1;
				break;

			case CTL_SEN_DRVDEV_LVDS_1:
				act_mode = CTL_SIE_IN_CSI_2;
				break;

			case CTL_SEN_DRVDEV_LVDS_2:
				act_mode = CTL_SIE_IN_CSI_3;
				break;

			case CTL_SEN_DRVDEV_LVDS_3:
				act_mode = CTL_SIE_IN_CSI_4;
				break;

			case CTL_SEN_DRVDEV_LVDS_4:
				act_mode = CTL_SIE_IN_CSI_5;
				break;

			case CTL_SEN_DRVDEV_LVDS_5:
				act_mode = CTL_SIE_IN_CSI_6;
				break;

			case CTL_SEN_DRVDEV_LVDS_6:
				act_mode = CTL_SIE_IN_CSI_7;
				break;

			case CTL_SEN_DRVDEV_LVDS_7:
				act_mode = CTL_SIE_IN_CSI_8;
				break;

			default:
				ctl_sie_dbg_err("drvdev id error 0x%.8x\r\n", (unsigned int)(attr_param.drvdev));
				act_mode = CTL_SIE_IN_CSI_1;
				break;
			}
			break;

		case CTL_SEN_IF_TYPE_SLVSEC:
			if (CTL_SEN_DRVDEV_MASK_SLVSEC(attr_param.drvdev) == CTL_SEN_DRVDEV_SLVSEC_0) {
				act_mode = CTL_SIE_IN_SLVS_EC;
			} else {
				ctl_sie_dbg_err("drvdev id error 0x%.8x\r\n", (unsigned int)(attr_param.drvdev));
			}
			break;

		default:
			break;
		}
	}
	return act_mode;
}

INT32 ctl_sie_module_sen_chgmode(CTL_SIE_ID id)
{
	CTL_SEN_CHGMODE_OBJ chgmode_obj = {0};
	unsigned long flags;

	loc_cpu(flags);
	memset((void *)&chgmode_obj, 0, sizeof(CTL_SEN_CHGMODE_OBJ));

	chgmode_obj.mode_sel = sie_ctl_info[id]->param.chg_senmode_info.sel;
	chgmode_obj.output_dest = sie_ctl_info[id]->param.chg_senmode_info.output_dest;

	if (chgmode_obj.mode_sel == CTL_SEN_MODESEL_MANUAL) {
		chgmode_obj.manual_info = sie_ctl_info[id]->param.chg_senmode_info.manual_info;
	} else if (chgmode_obj.mode_sel == CTL_SEN_MODESEL_AUTO) {
		chgmode_obj.auto_info = sie_ctl_info[id]->param.chg_senmode_info.auto_info;
	} else {
		ctl_sie_dbg_err("N.S.\r\n");
		unl_cpu(flags);
		return CTL_SIE_E_NOSPT;
	}
	unl_cpu(flags);

	return sie_ctl_info[id]->p_sen_obj->chgmode(chgmode_obj);
}


static BOOL ctl_sie_module_stream_chk(CTL_SIE_ID id)
{
	BOOL b_rt = FALSE;

	if ((sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DRAM) && (sie_ctl_info[id]->ctrl.trig_info.trig_frame_num != 0xffffffff) && (sie_ctl_info[id]->ctrl.frame_ctl_info.crp_end_cnt >= sie_ctl_info[id]->ctrl.trig_info.trig_frame_num - 1)) {
		//stop sie when reach target frame count
		if (ctl_sie_module_state_machine(id, CTL_SIE_OP_TRIG_STOP) == CTL_SIE_E_OK) {
			sie_ctl_info[id]->ctrl.trig_info.trig_type = CTL_SIE_TRIG_STOP;
			sie_ctl_info[id]->ctrl.trig_info.b_wait_end = FALSE;
			ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_ITEM_TRIG_IMM, TRUE);
		}
	} else {
		//get next frame public buffer

		if (sie_ctl_info[id]->dvi_interlace_skip_en == 1) {
			sie_ctl_int_flg_ctl[id]->interlace_skip_flg = ~sie_ctl_int_flg_ctl[id]->interlace_skip_flg;
			if (sie_ctl_int_flg_ctl[id]->interlace_skip_flg == FALSE) {
				return TRUE;
			}
		}

		if (ctl_sie_header_create(sie_ctl_info[id], CTL_SIE_HEAD_IDX_NEXT) == CTL_SIE_E_OK) {
			b_rt = TRUE;
		} else {
			b_rt = FALSE;
		}
	}

	return b_rt;
}

static void ctl_sie_module_reset_frm_cnt(CTL_SIE_ID id)
{
	if ((sie_ctl_info[id] != NULL) && (sie_ctl_info[id]->ctrl.rst_fc_sts == CTL_SIE_RST_FC_BEGIN)) {
		if (sie_ctl_int_flg_ctl[id]->gyro_en == DISABLE) {
			memset((void *)&sie_ctl_info[id]->ctrl.frame_ctl_info, 0, sizeof(CTL_SIE_FRM_CTL_INFO));
		}
		sie_ctl_info[id]->ctrl.rst_fc_sts = CTL_SIE_RST_FC_DONE;
		ctl_sie_module_update_bp3(id, sie_ctl_int_flg_ctl[id]->cfg_start_bp, TRUE);// set bp value back
	}
}

static void ctl_sie_module_update_ch0_lofs(CTL_SIE_ID id, UINT32 ch0_out_width)
{
	unsigned long flags;
	UINT32 lofs_ratio = CTL_SIE_RAW_COMPRESS_BIT;	//default is 12bit out(raw compress)
	UINT32 enc_word_num = 14;

	loc_cpu(flags);
	if (ch0_out_width == 0) {	//input ch0_out_width to 0 will force use sie_scl_out width to calc. lineoffset
		ch0_out_width = sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_scl_out.w;
	}

	if (ch0_out_width != 0) {
		if (ctl_sie_typecast_ccir(sie_ctl_info[id]->rtc_param.data_fmt)) {
			sie_ctl_info[id]->rtc_ctrl.out_ch_lof[CTL_SIE_CH_0] = ALIGN_CEIL_4(ch0_out_width);
		} else {
			if (sie_ctl_info[id]->rtc_param.encode_en) {
				switch (sie_ctl_info[id]->rtc_param.enc_rate) {
				case CTL_SIE_ENC_50:
					enc_word_num = 12;	//64(pixel)*12(bit)/32 * 50% = 12words
					break;

				case CTL_SIE_ENC_58:
					enc_word_num = 14;
					break;

				case CTL_SIE_ENC_66:
				default:
					enc_word_num = 16;
					break;
				}
				sie_ctl_info[id]->rtc_ctrl.out_ch_lof[CTL_SIE_CH_0] = (ALIGN_CEIL_64(ch0_out_width)/64)*enc_word_num*4;
			} else {
				switch (sie_ctl_info[id]->rtc_param.data_fmt) {
				case CTL_SIE_BAYER_8:
					lofs_ratio = CTL_SIE_RAW_BIT_8;
					break;

				case CTL_SIE_BAYER_10:
					lofs_ratio = CTL_SIE_RAW_BIT_10;
					break;

				case CTL_SIE_BAYER_12:
					lofs_ratio = CTL_SIE_RAW_BIT_12;
					break;

				case CTL_SIE_BAYER_16:
				default:
					lofs_ratio = CTL_SIE_RAW_BIT_16;
					break;
				}
				sie_ctl_info[id]->rtc_ctrl.out_ch_lof[CTL_SIE_CH_0] = ALIGN_CEIL_4((lofs_ratio * ch0_out_width) / CTL_SIE_RAW_BIT_8);
			}
		}
	}
	unl_cpu(flags);
	ctl_sie_module_update_load_flg(id, CTL_SIE_ITEM_CH0_LOF);
}

static void ctl_sie_module_update_ch1_lofs(CTL_SIE_ID id)
{
	unsigned long flags;
	BOOL update_lofs = FALSE;

	if (ctl_sie_limit[id].out_max_ch >= CTL_SIE_CH_1) {
		loc_cpu(flags);
		if (sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_scl_out.w != 0) {
			if (ctl_sie_typecast_ccir(sie_ctl_info[id]->rtc_param.data_fmt) && sie_ctl_info[id]->rtc_param.data_fmt != CTL_SIE_YUV_422_NOSPT) {
				sie_ctl_info[id]->rtc_ctrl.out_ch_lof[CTL_SIE_CH_1] = ALIGN_CEIL_4(sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_scl_out.w);
				update_lofs = TRUE;
			}
		}
		unl_cpu(flags);
		if (update_lofs) {
			ctl_sie_module_update_load_flg(id, CTL_SIE_ITEM_CH1_LOF);
		}
	}
}

static UINT8 __ctl_sie_get_ipp_direct_ife_chk_data(CTL_SIE_ID id)
{
	CTL_SEN_GET_MODE_BASIC_PARAM sen_mode_param = {0};
	UINT32 i, output_dest = sie_ctl_info[id]->rtc_param.chg_senmode_info.output_dest;
	UINT8 frm_idx = 0;
	BOOL ife_chk_mode = 0; // 52x/56x hw only 0
	BOOL ife_chk_en = 0; // fw default 0, only dcg in hdr set 1 [IVOT_N12025_CO-159]

	if (id == CTL_SIE_ID_1) {
		return ife_chk_mode;
	}

	if ((output_dest & (1 << id)) == 0) {
		ctl_sie_dbg_err("output_dest=0x%x id=%d not match\r\n", output_dest, id);
		return ife_chk_en;
	}

	/* get sensor information */
	sen_mode_param.mode = sie_ctl_info[id]->rtc_param.chg_senmode_info.cfg_sen_mode;
	if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_MODE_BASIC, (void *)&sen_mode_param) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("get sen info err\r\n");
		return ife_chk_en;
	}

	// sensor output order : SIE 3 -> SIE2 -> SIE1
	for (i = 1; i < CTL_SIE_ID_MAX_NUM; i++) {
		if ((CTL_SIE_ID_MAX_NUM - i) == id) {
			break;
		}
		if (output_dest & (1 << (CTL_SIE_ID_MAX_NUM - i))) {
			frm_idx++;
		}
	}

	if (((CTL_SEN_ENTITY_MODETYPE_EXPTGAIN(sen_mode_param.mode_type, frm_idx) == expt_gain_sel_hcg) ||
		 (CTL_SEN_ENTITY_MODETYPE_EXPTGAIN(sen_mode_param.mode_type, frm_idx) == expt_gain_sel_lcg)) &&
		 (sen_mode_param.frame_num > 1)) {
		 ife_chk_en = 1;
	}

	return ife_chk_en;
}

static INT32 ctl_sie_module_direct_cb_trig(CTL_SIE_ID id, CTL_SIE_DIRECT_CFG cfg_op, BOOL stop)
{
	INT32 rt = CTL_SIE_E_OK, rt_ipp = 0; // CTL_IPP_E_OK

	if ((sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DIRECT || (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_BOTH)) && sie_ctl_info[id]->ctrl.direct_cb_fp.sts & cfg_op) {
		if (sie_ctl_info[id]->ctrl.direct_cb_fp.cb_fp == NULL) {
			if (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DIRECT) {
				//force release buffer when direct only + no direct cb_fp
				ctl_sie_buf_msg_snd(CTL_SIE_BUF_MSG_UNLOCK, id, (UINT32)&sie_ctl_info[id]->ctrl, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT]);
			}
			rt = CTL_SIE_E_NULL_FP;
		} else {
			switch (cfg_op) {
			case CTL_SIE_DIRECT_TRIG_START:
				sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT].vdo_frm.reserved[4] = ctl_sie_vdofrm_reserved_mask_ifechkdata(__ctl_sie_get_ipp_direct_ife_chk_data(id), sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT].vdo_frm.reserved[4]);
				rt_ipp = sie_ctl_info[id]->ctrl.direct_cb_fp.cb_fp(cfg_op, (void *)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT].vdo_frm, NULL);
				break;
			case CTL_SIE_DIRECT_FRM_CFG_START:	//update next frame info and trig ipp
				rt_ipp = sie_ctl_info[id]->ctrl.direct_cb_fp.cb_fp(cfg_op, (void *)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT].vdo_frm, NULL);
				break;

			case CTL_SIE_DIRECT_TRIG_STOP:
				//don't care ipp stop error code, force stop sie
				sie_ctl_info[id]->ctrl.direct_cb_fp.cb_fp(cfg_op, (void *)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT].vdo_frm, NULL);
				break;

			case CTL_SIE_DIRECT_PUSH_RDY_BUF:
			case CTL_SIE_DIRECT_DROP_BUF:
				if (sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].buf_addr != 0) {
					rt_ipp = sie_ctl_info[id]->ctrl.direct_cb_fp.cb_fp(cfg_op, (void *)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].vdo_frm, NULL);
					if (rt_ipp == CTL_SIE_E_OK) {
						if (stop) {
							sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].buf_addr = 0;
						}
						sie_ctl_info[id]->ctrl.frame_ctl_info.dir_rls_cnt++;
					} else {
						sie_ctl_info[id]->ctrl.frame_ctl_info.dir_rls_fail_cnt++;
						ctl_sie_buf_msg_snd(CTL_SIE_BUF_MSG_UNLOCK, id, (UINT32)&sie_ctl_info[id]->ctrl, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR]);
					}
				}
				if (stop && sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT].buf_addr != 0) {
					rt_ipp = sie_ctl_info[id]->ctrl.direct_cb_fp.cb_fp(CTL_SIE_DIRECT_DROP_BUF, (void *)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT].vdo_frm, NULL);
					if (rt_ipp == CTL_SIE_E_OK) {
						sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT].buf_addr = 0;
						sie_ctl_info[id]->ctrl.frame_ctl_info.dir_drop_cnt++;
					} else {
						sie_ctl_info[id]->ctrl.frame_ctl_info.dir_drop_fail_cnt++;
						ctl_sie_buf_msg_snd(CTL_SIE_BUF_MSG_UNLOCK, id, (UINT32)&sie_ctl_info[id]->ctrl, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT]);
					}
				}
				break;

			case CTL_SIE_DIRECT_SKIP_CFG:
				rt_ipp = sie_ctl_info[id]->ctrl.direct_cb_fp.cb_fp(cfg_op, NULL, NULL);
				break;

			default:
				break;
			}
		}
	}

	if (rt_ipp != 0) {
		ctl_sie_dbg_ind("id=%d, CTL_SIE_DIRECT_CFG 0x%.8x, rt_ipp=%d\r\n", id, (unsigned int)cfg_op, rt_ipp);
		if (rt == CTL_SIE_E_OK) {
			rt = CTL_SIE_E_IPP;
		}
	}

	return rt;
}

static void ctl_sie_module_isp_cb_trig(CTL_SIE_ID id, UINT32 status)
{
	if (sie_ctl_info[id]->flow_type != CTL_SIE_FLOW_PATGEN) {	//skip trigger isp when pattern gen mode
		if ((status == CTL_SIE_INTE_ALL) || (g_ctl_sie_status[id] == CTL_SIE_STS_RUN && sie_ctl_info[id]->ctrl.rst_fc_sts == CTL_SIE_RST_FC_DONE)) {
			ctl_sie_isp_event_cb(id, status, sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].vdo_frm.count, 0);
		}
	}

	if (status == CTL_SIE_INTE_ALL) {
		ctl_sie_set_load(id, NULL);	//load iq param
	}
}

static INT32 ctl_sie_module_ring_buf_malloc(CTL_SIE_ID id)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SEN_GET_MODE_BASIC_PARAM mode_basic_param = {0};
	UINT32 crp_h;

	if (id == CTL_SIE_ID_2) {
		if (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DIRECT) {
			mode_basic_param.mode = CTL_SEN_MODE_CUR;
			if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SEN_IN || sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SIG_DUPL) {
				if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_MODE_BASIC, (void *)&mode_basic_param) == CTL_SIE_E_OK) {
					crp_h = mode_basic_param.crp_size.h;
				} else {
					rt = CTL_SIE_E_SEN;
				}
			} else if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN) {
				crp_h = sie_ctl_info[id]->ctrl.rtc_obj.pat_gen_param.pat_gen_src_win.h;
			} else {
				rt = CTL_SIE_E_NOSPT;
			}
			if (rt == CTL_SIE_E_OK) {
				//force enable raw compress when sie2 + direct
				sie_ctl_info[id]->rtc_param.encode_en = ENABLE;
				sie_ctl_info[id]->rtc_param.enc_rate = CTL_SIE_ENC_58;
				sie_ctl_info[id]->rtc_param.data_fmt = CTL_SIE_BAYER_12;
				ctl_sie_module_update_load_flg(id, CTL_SIE_ITEM_ENCODE);
				ctl_sie_module_update_load_flg(id, CTL_SIE_ITEM_ENC_RATE);
				ctl_sie_module_update_load_flg(id, CTL_SIE_ITEM_DATAFORMAT);
				ctl_sie_module_update_ch0_lofs(id, sie_ctl_info[id]->rtc_ctrl.sie_act_win.w);
				sie_ctl_info[id]->ctrl.ring_buf_ctl.en = ENABLE;
				if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->chk_msg_type(id, CTL_SIE_DBG_MSG_RING_BUF_FULL)) {
					sie_ctl_info[id]->ctrl.ring_buf_ctl.buf_len = ctl_sie_limit[id].ring_buf_len_max;
				} else if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN) {
					sie_ctl_info[id]->ctrl.ring_buf_ctl.buf_len = ctl_sie_max(ctl_sie_limit[id].ring_buf_len_max, crp_h+1);
				} else {
					sie_ctl_info[id]->ctrl.ring_buf_ctl.buf_len = ALIGN_CEIL(crp_h >> 1, 2); // (crop_h / 2) align 2 [MTD_IAS_IR-478]
				}
				sie_ctl_info[id]->ctrl.ring_buf_ctl.buf_size = ALIGN_CEIL(sie_ctl_info[id]->rtc_ctrl.out_ch_lof[CTL_SIE_CH_0] * sie_ctl_info[id]->ctrl.ring_buf_ctl.buf_len, VOS_ALIGN_BYTES);
				ring_buf_head_info.buf_id = CTL_SIE_BUF_CB_IGNOR;	//use buf_id 0xffffffff to get public buf for ring buf out
#if defined(__KERNEL__)
				if (((ctl_sie_fb_trig_stage & ctl_sie_fb_trig_stage_normal_trigger) == 0)  && fastboot) {
					sie_fb_ring_buf_release(id);	//release fastboot ring buffer
					ctl_sie_fb_trig_stage |= ctl_sie_fb_trig_stage_normal_trigger;
				}
#endif
				ctl_sie_buf_io_cfg(id, CTL_SIE_BUF_IO_NEW, sie_ctl_info[id]->ctrl.ring_buf_ctl.buf_size, (UINT32)&ring_buf_head_info);
				if (ring_buf_head_info.buf_addr == 0) {
					rt = CTL_SIE_E_NOMEM;
					sie_ctl_info[id]->ctrl.ring_buf_ctl.en = DISABLE;
					sie_ctl_info[id]->ctrl.ring_buf_ctl.buf_addr = 0;
				} else {
					sie_ctl_info[id]->ctrl.ring_buf_ctl.buf_addr = ring_buf_head_info.buf_addr;
					sie_ctl_info[id]->ctrl.out_ch_addr[CTL_SIE_CH_0] = ring_buf_head_info.buf_addr;
				}
				ctl_sie_module_update_load_flg(id, CTL_SIE_INT_ITEM_RING_BUF);
			}
		} else {
			sie_ctl_info[id]->ctrl.ring_buf_ctl.en = DISABLE;
			sie_ctl_info[id]->ctrl.ring_buf_ctl.buf_addr = 0;
			ctl_sie_module_update_load_flg(id, CTL_SIE_INT_ITEM_RING_BUF);
		}
	}

	if (rt == CTL_SIE_E_OK) {
		ctl_sie_set_load(id, NULL);
	}
	return rt;
}

static void ctl_sie_module_ring_buf_mfree(CTL_SIE_ID id)
{
	ring_buf_head_info.buf_id = CTL_SIE_BUF_CB_IGNOR;
	ring_buf_head_info.buf_addr = sie_ctl_info[id]->ctrl.ring_buf_ctl.buf_addr;
	ctl_sie_buf_io_cfg(id, CTL_SIE_BUF_IO_UNLOCK, 0, (UINT32)&ring_buf_head_info);
}

static void ctl_sie_module_queue_flush(CTL_SIE_ID id)
{
	// flush buf and isp task
 	ctl_sie_buf_queue_flush(id);
 	ctl_sie_isp_queue_flush(id);
}

INT32 ctl_sie_module_direct_to_both(CTL_SIE_ID id)
{
	UINT32 i;

	if (sie_ctl_info[id]->ctrl.total_frame_num > 1) {
		for (i = CTL_SIE_ID_1; i <= ctl_sie_limit[id].max_spt_id; i++) {
			if ((sie_ctl_info[i] != NULL) && ((1 << i) & sie_ctl_info[i]->rtc_param.chg_senmode_info.output_dest)) {
				if (i == CTL_SIE_ID_2) {
					sie_ctl_info[i]->ctrl.ring_buf_ctl.en = DISABLE;
					ctl_sie_hdl_update_item(i, 1ULL << CTL_SIE_INT_ITEM_RING_BUF, FALSE);
					ctl_sie_module_update_ch0_out_mode(i); // CTL_SIE_NORMAL_OUT->CTL_SIE_SINGLE_OUT, for singleout disable, wait ipp finish, and new buffer
				}

				sie_ctl_info[i]->rtc_param.out_dest = CTL_SIE_OUT_DEST_BOTH;
				sie_ctl_info[i]->load_param.out_dest = CTL_SIE_OUT_DEST_BOTH;
				ctl_sie_hdl_update_item(i, 1ULL << CTL_SIE_ITEM_OUT_DEST, FALSE);

				sie_ctl_info[i]->rtc_param.encode_en = DISABLE;
				sie_ctl_info[i]->load_param.encode_en = DISABLE;
				ctl_sie_hdl_update_item(i, 1ULL << CTL_SIE_ITEM_ENCODE, FALSE);
				ctl_sie_module_update_ch0_lofs(i, 0);
				ctl_sie_set_load(i, NULL);
				sie_ctl_int_flg_ctl[i]->isr_load_en_flg = TRUE;
			}
		}
	} else {
		if (id == CTL_SIE_ID_2) {
			sie_ctl_info[id]->ctrl.ring_buf_ctl.en = DISABLE;
			ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_INT_ITEM_RING_BUF, FALSE);
			ctl_sie_module_update_ch0_out_mode(id);
		}

		sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_BOTH;
		sie_ctl_info[id]->load_param.out_dest = CTL_SIE_OUT_DEST_BOTH;
		ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_ITEM_OUT_DEST, FALSE);

		sie_ctl_info[id]->rtc_param.encode_en = DISABLE;
		sie_ctl_info[id]->load_param.encode_en = DISABLE;
		ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_ITEM_ENCODE, FALSE);
		ctl_sie_module_update_ch0_lofs(id, 0);
		ctl_sie_set_load(id, NULL);
		sie_ctl_int_flg_ctl[id]->isr_load_en_flg = TRUE;
	}

	ctl_sie_dbg_get_raw_id = id;
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_module_both_to_direct(CTL_SIE_ID id)
{
	UINT32 i;

	if (ctl_sie_dbg_get_raw_id == id) {
		if (sie_ctl_info[id]->ctrl.total_frame_num > 1) {
			for (i = CTL_SIE_ID_1; i <= ctl_sie_limit[id].max_spt_id; i++) {
				if ((sie_ctl_info[i] != NULL) && ((1 << i) & sie_ctl_info[i]->rtc_param.chg_senmode_info.output_dest)) {
					sie_ctl_info[i]->rtc_param.out_dest = CTL_SIE_OUT_DEST_DIRECT;
					sie_ctl_info[i]->load_param.out_dest = CTL_SIE_OUT_DEST_DIRECT;
					ctl_sie_hdl_update_item(i, 1ULL << CTL_SIE_ITEM_OUT_DEST, FALSE);

					sie_ctl_info[i]->rtc_param.encode_en = ENABLE;
					sie_ctl_info[i]->load_param.encode_en = ENABLE;
					ctl_sie_hdl_update_item(i, 1ULL << CTL_SIE_ITEM_ENCODE, FALSE);
					ctl_sie_module_update_ch0_lofs(i, 0);
					ctl_sie_set_load(i, NULL);
					sie_ctl_int_flg_ctl[i]->isr_load_en_flg = TRUE;

					if (sie_ctl_info[i]->id == CTL_SIE_ID_2) {
						sie_ctl_info[i]->ctrl.ring_buf_ctl.en = ENABLE;
						ctl_sie_hdl_update_item(i, 1ULL << CTL_SIE_INT_ITEM_RING_BUF, FALSE);
						ctl_sie_module_update_ch0_out_mode(i);
					}
				}
			}
		} else {
			sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_DIRECT;
			sie_ctl_info[id]->load_param.out_dest = CTL_SIE_OUT_DEST_DIRECT;
			ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_ITEM_OUT_DEST, FALSE);

			sie_ctl_info[id]->rtc_param.encode_en = ENABLE;
			sie_ctl_info[id]->load_param.encode_en = ENABLE;
			ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_ITEM_ENCODE, FALSE);
			ctl_sie_module_update_ch0_lofs(id, 0);
			ctl_sie_set_load(id, NULL);
			sie_ctl_int_flg_ctl[id]->isr_load_en_flg = TRUE;

			if (sie_ctl_info[id]->id == CTL_SIE_ID_2) {
				sie_ctl_info[id]->ctrl.ring_buf_ctl.en = ENABLE;
				ctl_sie_hdl_update_item(id, 1ULL << CTL_SIE_INT_ITEM_RING_BUF, FALSE);
				ctl_sie_module_update_ch0_out_mode(id);
			}
		}
	}

	return CTL_SIE_E_OK;
}

INT32 ctl_sie_module_config_gyro_start(CTL_SIE_ID id)
{
    GYRO_FREE_RUN_PARAM frParam = {0};
	CTL_SEN_GET_FPS_PARAM fps_param = {0};
    INT32 rt = E_OK;
    UINT32 frameRate = 3000;
    UINT32 src = GYRO_SIE_1;
    UINT32 value = 0;

	if (sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.en) {

		if (sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj == 0) {
			ctl_sie_dbg_err("get gyro obj error\r\n");
			return CTL_SIE_E_NULL_FP;
		} else {
		    rt = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj->open();
		    if(rt != E_OK) {
		        return rt;
		    }

			switch (id) {
			case CTL_SIE_ID_1:
				src = GYRO_SIE_1;
				break;

			case CTL_SIE_ID_2:
				src = GYRO_SIE_2;
				break;

			case CTL_SIE_ID_4:
				src = GYRO_SIE_4;
				break;

			case CTL_SIE_ID_5:
				src = GYRO_SIE_5;
				break;

			default:
				ctl_sie_dbg_err("gyro src mapping overflow, sie_id = %d\r\n", id);
				break;
			}
		    rt = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj->set_cfg(GYROCTL_CFG_SIE_SRC,(void *)&src);
		    if(rt != E_OK) {
		        return rt;
		    }

		    frParam.uiPeriodUs = 100000000/frameRate;
			if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_FPS, (void *)&fps_param) == CTL_SIE_E_OK) {
				if (fps_param.chg_fps != 0) {
				    frParam.uiPeriodUs = 100000000/fps_param.chg_fps;
				}
			} else {
				ctl_sie_dbg_wrn("id=%d, get sen fps cfg fail \r\n", id);
			}

		    frParam.uiDataNum = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.data_num;
		    frParam.uiTriggerIdx = 0;
		    frParam.triggerMode = GYRO_FREERUN_SIE_SYNC;
		    rt = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj->set_cfg(GYROCTL_CFG_FRUN_PARAM,&frParam);
		    if(rt != E_OK) {
		        return rt;
		    }
		    value = GYRO_OP_MODE_FREE_RUN;
		    rt = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj->set_cfg(GYROCTL_CFG_MODE,&value);
		    if(rt != E_OK) {
		        return rt;
		    }

			if (sie_ctl_int_flg_ctl[id] != NULL) {
				sie_ctl_int_flg_ctl[id]->gyro_en = TRUE;
			}
		}
	}
    return rt;
}

//for buf task get
INT32 ctl_sie_module_get_gyro_data(CTL_SIE_ID id, UINT32 gyro_out_addr, UINT32 vd_ts)
{
    INT32 rt = E_OK;
    CTL_SIE_EXT_GYRO_DATA gyro_data = {NULL};
    UINT32 data_num = 0;
    UINT32 buf_ofs = 0;

	if (sie_ctl_int_flg_ctl[id]->gyro_en) {
		if (gyro_out_addr != 0) {
			gyro_data.data_num = (UINT32 *)(gyro_out_addr);
			*gyro_data.data_num = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.data_num;

			buf_ofs += sizeof(*gyro_data.data_num);
			gyro_data.t_crop_start = (UINT64 *)(gyro_out_addr + buf_ofs);
			*gyro_data.t_crop_start = sie_ctl_info[id]->ctrl.gyro_ctl_info.crp_start_ts;

			buf_ofs += sizeof(*gyro_data.t_crop_start);
			gyro_data.t_crp_end = (UINT64 *)(gyro_out_addr + buf_ofs);
			*gyro_data.t_crp_end = sie_ctl_info[id]->ctrl.gyro_ctl_info.crp_end_ts;

			buf_ofs += sizeof(*gyro_data.t_crp_end);
			gyro_data.time_stamp = (UINT32 *)(gyro_out_addr + buf_ofs);
			*gyro_data.time_stamp = vd_ts;

			buf_ofs += (*gyro_data.data_num) *sizeof(UINT32);
			gyro_data.agyro_x =  (INT32 *)(gyro_out_addr + buf_ofs);
			buf_ofs += (*gyro_data.data_num) * sizeof(INT32);
			gyro_data.agyro_y =  (INT32 *)(gyro_out_addr + buf_ofs);
			buf_ofs += (*gyro_data.data_num) * sizeof(INT32);
			gyro_data.agyro_z =  (INT32 *)(gyro_out_addr + buf_ofs);
			buf_ofs += (*gyro_data.data_num) * sizeof(INT32);
			gyro_data.ags_x = (INT32 *)(gyro_out_addr + buf_ofs);
			buf_ofs += (*gyro_data.data_num) * sizeof(INT32);
			gyro_data.ags_y = (INT32 *)(gyro_out_addr + buf_ofs);
			buf_ofs += (*gyro_data.data_num) * sizeof(INT32);
			gyro_data.ags_z = (INT32 *)(gyro_out_addr + buf_ofs);

			if (sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj == 0) {
				ctl_sie_dbg_err("get gyro obj error\r\n");
				return CTL_SIE_E_NULL_FP;
			} else {
				rt = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj->get_data(gyro_data.data_num,gyro_data.time_stamp,gyro_data.agyro_x, gyro_data.agyro_y, gyro_data.agyro_z);
				data_num = *gyro_data.data_num;
				if (sie_ctl_info[id]->ctrl.gyro_ctl_info.chk_latency_en == ENABLE) {
					if (rt == E_OK && data_num != 0 && gyro_data.time_stamp[data_num-1] < vd_ts) {
						ctl_sie_dbg_ind("pre-frame gyro data, get gyro NUM %d, gyro_ts %d, cur_frm_ts %d\r\n", data_num,gyro_data.time_stamp[data_num-1],vd_ts);
						rt = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj->get_data(gyro_data.data_num,gyro_data.time_stamp,gyro_data.agyro_x, gyro_data.agyro_y, gyro_data.agyro_z);
					}
				}
				if (rt == E_OK)	{
				    rt = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj->get_gsdata(gyro_data.ags_x, gyro_data.ags_y, gyro_data.ags_z);
				}
			}
			data_num = *gyro_data.data_num;
		} else {
			if (sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj == 0) {
				ctl_sie_dbg_err("get gyro obj error\r\n");
				return CTL_SIE_E_NULL_FP;
			} else {
				rt = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj->get_data(&data_num,gyro_data.time_stamp,NULL, NULL, NULL);
			}
		}
	} else {
		data_num = 0;
	}

	//set latency en by first frame gyro data
	if (sie_ctl_info[id]->ctrl.gyro_ctl_info.chk_latency_en == ENABLE) {
		sie_ctl_info[id]->ctrl.gyro_ctl_info.chk_latency_en = DISABLE;
		if (data_num == 0) {
			sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.latency_en = ENABLE;
		} else {
			sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.latency_en = DISABLE;
		}
	}

    return rt;
}

INT32 ctl_sie_module_config_gyro_stop(CTL_SIE_ID id)
{
    INT32 rt = E_OK;

	if (sie_ctl_int_flg_ctl[id]->gyro_en && sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj != 0) {
    	sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.p_gyro_obj->close();
		sie_ctl_int_flg_ctl[id]->gyro_en = FALSE;
    }
    return rt;
}

INT32 ctl_sie_module_update_sub_out(CTL_SIE_ID id, UINT32 ca_src_addr, UINT32 la_src_addr) {
	CTL_SIE_ISP_SUB_OUT_PARAM sub_out_param = {0};

	sub_out_param.id = id;
	if (ca_src_addr == 0) {
		if (sie_ctl_info[id]->ctrl.ca_rst_valid) {
			sub_out_param.ca_addr = sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].vdo_frm.addr[CTL_SIE_CH_1];
		}
	} else {
		sub_out_param.ca_addr = ca_src_addr;
	}
	sub_out_param.ca_total_size = sie_ctl_info[id]->ctrl.buf_info.ch_info[CTL_SIE_CH_1].data_size;
	if (la_src_addr == 0) {
		if (sie_ctl_info[id]->ctrl.la_rst_valid) {
			sub_out_param.la_addr = sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].vdo_frm.addr[CTL_SIE_CH_2];
		}
	} else {
		sub_out_param.la_addr = la_src_addr;
	}
	sub_out_param.la_total_size = sie_ctl_info[id]->ctrl.buf_info.ch_info[CTL_SIE_CH_2].data_size;

	return ctl_sie_isp_update_sta_out(sub_out_param);
}

void ctl_sie_module_update_fb_sub_out(CTL_SIE_ID id) {
	UINT32 ca_addr = 0, la_addr = 0;

#if defined(__KERNEL__)
	sie_fb_get_rdy_addr(id, NULL, &ca_addr, &la_addr);
#endif
	ctl_sie_module_update_sub_out(id, ca_addr, la_addr);
}

void ctl_sie_module_update_sta_rst_valid(UINT32 id) {
	if (sie_ctl_info[id]->ctrl.la_param.enable) {
		sie_ctl_info[id]->ctrl.la_rst_valid = TRUE;
	} else {
		sie_ctl_info[id]->ctrl.la_rst_valid = FALSE;
	}

	if (sie_ctl_info[id]->ctrl.ca_param.enable) {
		sie_ctl_info[id]->ctrl.ca_rst_valid = TRUE;
	} else {
		sie_ctl_info[id]->ctrl.ca_rst_valid = FALSE;
	}
}

#if 0
#endif

INT32 ctl_sie_vd_sync_proc(UINT32 id, UINT32 sync_obj_addr, UINT32 step)
{
	UINT32 st_proc_time, end_proc_time, get_fps_time, diff_time, int_time;
	CTL_SEN_GET_FPS_PARAM fps_param = {0};
	UINT32 vd_time, new_fps;
	CTL_SIE_SYNC_CTRL_OBJ *sync;

	sync = (CTL_SIE_SYNC_CTRL_OBJ *)sync_obj_addr;

	st_proc_time = ctl_sie_util_get_timestamp();
	get_fps_time = st_proc_time;

	if (step == 2) {
		if (ctl_sie_get_sen_cfg(id, CTL_SEN_CFGID_GET_FPS, (void *)&fps_param) == CTL_SIE_E_OK) {
			if (fps_param.chg_fps != 0) {
				get_fps_time = ctl_sie_util_get_timestamp();

				sync->org_fps = fps_param.chg_fps;
				vd_time = (100000000 / fps_param.chg_fps) + sync->adj_time;
				new_fps = 100000000 / vd_time;
				ctl_sie_set_sen_cfg(id, CTL_SEN_CFGID_SET_FPS, (void *)&new_fps);
				//DBG_DUMP("id(%d) 2 %d %d %d %d %d %d\r\n", id, fps_param.dft_fps, fps_param.chg_fps, fps_param.cur_fps, vd_time, sync->adj_time, new_fps);
			}
		} else {
			ctl_sie_dbg_wrn("id=%d, get sen fps cfg fail \r\n", id);
		}
	} else if (step == 1) {
		new_fps = sync->org_fps;
		ctl_sie_set_sen_cfg(id, CTL_SEN_CFGID_SET_FPS, (void *)&new_fps);
		//DBG_DUMP("id(%d) 1 %d\r\n", id, new_fps);
	}

	end_proc_time = ctl_sie_util_get_timestamp();

	diff_time = (UINT32)((INT32)end_proc_time - (INT32)sync->isr_vd_t);
	int_time = 100000000 / sync->org_fps;
	if (diff_time > int_time) {
		DBG_DUMP("=>%u %u %u %u %u %u\r\n", id, step, sync->isr_vd_t, st_proc_time, get_fps_time, end_proc_time);
	}
	return 0;
}

static void ctl_sie_vd_sync_isr_proc(UINT32 id)
{
	CTL_SIE_SYNC_CTRL_OBJ *sync, *base;
	UINT32 base_vd_t[2], sync_vd_t, diff[2];
	INT32 idx0, idx1, idx;

	//sw sync flow begin
	sync = &sie_ctl_info[id]->ctrl.sync_obj;
	if (sync == NULL || sync->mode == 0) {
		return;
	}
	base = 0;
	if (sync->mode == 2 && sie_ctl_info[sync->sync_id] != NULL) {
		base = &sie_ctl_info[sync->sync_id]->ctrl.sync_obj;
	}

	if (sync != NULL && base != NULL) {
		//DBG_DUMP("id(%d) %d %d %d %d\r\n", id, sync->mode, sync->sync_id, sync->det_frm_int, sync->adj_thres);
		//detect vd time
		if (sync->mode == 2) {

			sync->isr_vd_t = ctl_sie_util_get_timestamp();
			//sync detect
			if (sync->det_pause == 0)
			{
				sync->det_frm_cnt += 1;
				if (sync->det_1st_done == 0) {
					if ((sync->rec_cnt >= 2) && (base->rec_cnt >= 2)) {
						sync->det_frm_cnt = sync->det_frm_int;
						sync->det_1st_done = 1;
					}
				}

				if (sync->det_frm_cnt >= sync->det_frm_int) {
					idx1 = (base->rec_cnt - 1) % SYNC_REC_MAX_NUM;
					idx0 = (base->rec_cnt - 2) % SYNC_REC_MAX_NUM;
					idx = (sync->rec_cnt - 1) % SYNC_REC_MAX_NUM;

					base_vd_t[0] = base->rec_time[idx0];
					base_vd_t[1] = base->rec_time[idx1];
					sync_vd_t = sync->rec_time[idx];
					if ((sync_vd_t >= base_vd_t[0]) && (sync_vd_t <= base_vd_t[1])) {

						diff[0] = (UINT32)((INT32)sync_vd_t - (INT32)base_vd_t[0]);
						diff[1] = (UINT32)((INT32)base_vd_t[1] - (INT32)sync_vd_t);

						if (sync->adj_auto == 1) {
							if (diff[0] > diff[1]) {
								sync->adj_id = id;
								sync->adj_time = diff[1];
							} else {
								sync->adj_id = sync->sync_id;
								sync->adj_time = diff[0];
							}
						} else {
							//always self
							sync->adj_id = id;
							sync->adj_time = diff[1];
						}

						if (sync->adj_time >= sync->adj_thres) {
							//DBG_DUMP("%d %d %d %d %d %d %d %d %d %d %d\r\n", idx0, idx, idx1, base_vd_t[0], sync_vd_t, base_vd_t[1], diff[0], diff[1], sync->adj_id, sync->adj_time, sync->adj_auto);
							//do adj proc
							sync->adj_proc_step = 2;

							//pause det
							sync->det_pause = 1;

							sync->adj_cnt += 1;
						}
					}

					//reset det frame count
					sync->det_frm_cnt = 0;
				}
			}

			if (sync->adj_proc_step) {
				ctl_sie_isp_msg_snd(CTL_SIE_MSG_SYNC, sync->adj_id, (UINT32)sync, sync->adj_proc_step, (UINT32)ctl_sie_vd_sync_proc);
				sync->adj_proc_step -= 1;
				if (sync->adj_proc_step == 0) {

					//start det
					sync->det_frm_cnt = 0;
					sync->det_pause = 0;
				}
			}
		}

	}
	//rec time
	sync->rec_time[(sync->rec_cnt % SYNC_REC_MAX_NUM)] = ctl_sie_util_get_timestamp();
	//DBG_DUMP("id(%d) %d %d\r\n", id, sync->rec_cnt, sync->rec_time[(sync->rec_cnt % SYNC_REC_MAX_NUM)]);
	sync->rec_cnt += 1;

}


//no bp3 | sin_out en --> skip proc
//no dram_end --> drop cur buf
//copy next to cur
static INT32 ctl_sie_vd_isr_proc(CTL_SIE_ID id)
{
	INT32 rt = CTL_SIE_E_SYS;

	if (g_ctl_sie_status[id] == CTL_SIE_STS_RUN) {
#if defined(__KERNEL__)
		if (!sie_ctl_info[id]->fb_obj.normal_vd && fastboot) {
			sie_fb_upd_timestp(id);
			sie_ctl_info[id]->fb_obj.normal_vd = TRUE;
			ctl_sie_module_update_fb_sub_out(id);
		}
#endif
		ctl_sie_module_update_sta_rst_valid(id);
		sie_ctl_info[id]->ctrl.frame_ctl_info.vd_cnt++;

		//sw sync flow
		ctl_sie_vd_sync_isr_proc(id);

		if (sie_ctl_info[id]->ctrl.rst_fc_sts == CTL_SIE_RST_FC_DONE) {
			ctl_sie_set_dbg_isr_ioctl(id, CTL_SIE_INTE_VD, CTL_SIE_BUF_IO_ALL, CTL_SIE_HEAD_IDX_CUR);
			//do when bp3_proc && single out disable
			if (!(sie_ctl_int_flg_ctl[id]->isr_valid_vd_flg) && (!ctl_sie_module_chk_hw_single_out_en(id))) {
				if (ctl_sie_module_direct_cb_trig(id, CTL_SIE_DIRECT_PUSH_RDY_BUF, FALSE) != CTL_SIE_E_OK) {	//push ready buf to ipp
					ctl_sie_dbg_wrn("id %d dir cb err\r\n", (int)id);
				}
				if (sie_ctl_int_flg_ctl[id]->isr_dram_end_flg) {
					sie_ctl_int_flg_ctl[id]->isr_dram_end_flg = FALSE;
				} else if (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DRAM || sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_BOTH) {
					//drop cur buf when no dram end
					ctl_sie_set_dbg_isr_ioctl(id, CTL_SIE_INTE_VD, CTL_SIE_BUF_IO_UNLOCK, CTL_SIE_HEAD_IDX_CUR);
					ctl_sie_buf_msg_snd(CTL_SIE_BUF_MSG_UNLOCK, id, (UINT32)&sie_ctl_info[id]->ctrl, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR]);
				}
				ctl_sie_header_set_cur_head(sie_ctl_info[id]);	//copy next header to cur
				ctl_sie_update_fc(id);
				ctl_sie_hdl_load_all(id);						//update next frame setting(next vd latch)
				ctl_sie_buf_update_out_size(sie_ctl_info[id]);	//update next frame output size
				sie_ctl_int_flg_ctl[id]->isr_valid_vd_flg = TRUE;
				rt = CTL_SIE_E_OK;
			} else {
				rt = CTL_SIE_E_SYS;
			}
		} else {
			ctl_sie_update_fc(id);
		}
		if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->set_ts != NULL) {
			ctl_sie_dbg_tab->set_ts(id, CTL_SIE_DBG_TS_VD, ctl_sie_get_fc(id));
		}
	} else {
		sie_ctl_int_flg_ctl[id]->isr_valid_vd_flg = FALSE;
	}

	vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_VD);
	sie_ctl_int_flg_ctl[id]->last_isr_sts = CTL_SIE_INTE_VD;
	return rt;
}

//no vd | sin_out en --> skip proc
static INT32 ctl_sie_bp3_isr_proc(CTL_SIE_ID id)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_DIRECT_CFG ctl_sie_dir_cfg = CTL_SIE_DIRECT_SKIP_CFG;

	if (sie_ctl_int_flg_ctl[id]->isr_valid_vd_flg && (g_ctl_sie_status[id] == CTL_SIE_STS_RUN)) {
		ctl_sie_set_dbg_isr_ioctl(id, CTL_SIE_INTE_BP3, CTL_SIE_BUF_IO_ALL, CTL_SIE_HEAD_IDX_CUR);
		//update bp ts debug log
		if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->set_ts != NULL) {
			ctl_sie_dbg_tab->set_ts(id, CTL_SIE_DBG_TS_BP3, ctl_sie_get_fc(id));
		}

#if defined(__KERNEL__)
		if (!sie_ctl_info[id]->fb_obj.normal_bp && fastboot) {
			sie_ctl_info[id]->fb_obj.normal_bp = TRUE;
		}
#endif
		//do bp3_proc (vd proc + single out disable)
		if (!ctl_sie_module_chk_hw_single_out_en(id)) {
			sie_ctl_int_flg_ctl[id]->isr_valid_vd_flg = FALSE;
			if (ctl_sie_module_stream_chk(id)) {
				ctl_sie_set_dbg_isr_ioctl(id, CTL_SIE_INTE_BP3, CTL_SIE_BUF_IO_NEW, CTL_SIE_HEAD_IDX_NEXT);
				ctl_sie_dir_cfg = CTL_SIE_DIRECT_FRM_CFG_START;
			}
		}
	}
	if (ctl_sie_module_direct_cb_trig(id, ctl_sie_dir_cfg, FALSE) != CTL_SIE_E_OK) {
//		ctl_sie_dbg_wrn("id %d dir cb %d err\r\n", (int)id, (int)ctl_sie_dir_cfg);
	}
	ctl_sie_module_reset_frm_cnt(id);
	sie_ctl_int_flg_ctl[id]->last_isr_sts = CTL_SIE_INTE_BP3;
	return rt;
}

//push cur
static INT32 ctl_sie_crp_end_isr_proc(CTL_SIE_ID id)
{
	CTL_SIE_BUF_IO_CFG buf_io_cfg = CTL_SIE_BUF_IO_ALL;
	CTL_SIE_HEADER_INFO	head_info = {0};

#if defined(__KERNEL__)
	//skip first frame de proc when fastboot
	if (!sie_ctl_info[id]->fb_obj.normal_ce && sie_ctl_info[id]->fb_obj.normal_bp && fastboot) {
		sie_fb_buf_release(id, TRUE, CTL_SIE_INTE_CROPEND, SIE_FB_BUF_IDX_CUR_OUT);
		sie_ctl_info[id]->fb_obj.normal_ce = TRUE;
		sie_ctl_int_flg_ctl[id]->last_isr_sts = CTL_SIE_INTE_CROPEND;
		return CTL_SIE_E_OK;
	}
#endif

	if (sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].buf_addr != 0) {
		if (sie_ctl_int_flg_ctl[id]->gyro_en) {
			sie_ctl_info[id]->ctrl.gyro_ctl_info.crp_end_ts	= ctl_sie_util_get_syst_timestamp();
		}
		ctl_sie_module_update_sub_out(id, 0, 0);
	}

	if (g_ctl_sie_status[id] == CTL_SIE_STS_RUN) {
		//update ts for debug log
		if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->set_ts != NULL) {
			if (sie_ctl_info[id]->ctrl.frame_ctl_info.frame_cnt > 0) {
				ctl_sie_dbg_tab->set_ts(id, CTL_SIE_DBG_TS_CRPEND, ctl_sie_get_fc(id));
			}
		}
	}

	sie_ctl_info[id]->ctrl.frame_ctl_info.crp_end_cnt++;
	if (sie_ctl_int_flg_ctl[id]->gyro_en) {
		ctl_sie_buf_msg_snd(CTL_SIE_BUF_MSG_GET_GYRO, (UINT32)id, (UINT32)sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].vdo_frm.reserved[1], (UINT32)sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].vdo_frm.timestamp);
	}
	if (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DRAM) {
		sie_ctl_int_flg_ctl[id]->isr_dram_end_flg = TRUE;

		if (sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].buf_addr != 0) {
			if (sie_ctl_info[id]->ctrl.trig_info.trig_frame_num != 0xffffffff) {
				ctl_sie_buf_msg_snd(CTL_SIE_BUF_MSG_PUSH, id, (UINT32)&sie_ctl_info[id]->ctrl, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR]);
				buf_io_cfg = CTL_SIE_BUF_IO_PUSH;
			} else {
				if (g_ctl_sie_status[id] == CTL_SIE_STS_RUN) {
					ctl_sie_buf_msg_snd(CTL_SIE_BUF_MSG_PUSH, id, (UINT32)&sie_ctl_info[id]->ctrl, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR]);
					buf_io_cfg = CTL_SIE_BUF_IO_PUSH;
				} else {
					ctl_sie_buf_io_cfg(id, CTL_SIE_BUF_IO_UNLOCK, 0, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR]);
					buf_io_cfg = CTL_SIE_BUF_IO_UNLOCK;
				}
			}
		}
	} else if (sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].vdo_frm.resv1 == 1) { //direct mode get raw
		if (ctl_sie_isp_int_pull_img_out(id, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR])) {
			ctl_sie_buf_io_cfg(id, CTL_SIE_BUF_IO_LOCK, 0, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR]);
			buf_io_cfg = CTL_SIE_BUF_IO_LOCK;
		}
	} else if (sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR].vdo_frm.reserved[1] != 0) {	//direct mode push gyro data out
		head_info = sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR];
		head_info.buf_id = CTL_SIE_BUF_CB_IGNOR;
		ctl_sie_buf_msg_snd(CTL_SIE_BUF_IO_LOCK, id, (UINT32)&sie_ctl_info[id]->ctrl, (UINT32)&head_info);
	}
	ctl_sie_set_dbg_isr_ioctl(id, CTL_SIE_INTE_DRAM_OUT0_END, buf_io_cfg, CTL_SIE_HEAD_IDX_CUR);

	if (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_BOTH) {
		if (sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT].vdo_frm.resv1 == 1 && sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT].buf_addr != 0) {
			ctl_sie_module_both_to_direct(id);
		}
		sie_ctl_int_flg_ctl[id]->isr_dram_end_flg = TRUE;
	}
	sie_ctl_int_flg_ctl[id]->last_isr_sts = CTL_SIE_INTE_CROPEND;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_crp_st_isr_proc(CTL_SIE_ID id)
{
	if (g_ctl_sie_status[id] == CTL_SIE_STS_RUN) {
		//update ts for debug log
		if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->set_ts != NULL) {
			ctl_sie_dbg_tab->set_ts(id, CTL_SIE_DBG_TS_CRPST, ctl_sie_get_fc(id));
		}
		if (sie_ctl_int_flg_ctl[id]->gyro_en) {
			sie_ctl_info[id]->ctrl.gyro_ctl_info.crp_start_ts = ctl_sie_util_get_syst_timestamp();
		}
	}
	return CTL_SIE_E_OK;
}


/*
    isr function
BP3:
	reset frame cnt flow, shdr or sync case only
	check streaming, get next frame buffer and set to kdrv
	send config start event to vproc(direct mode)

CRP_END:
	push image out by ctl_sie_buf task
	drop current buffer when stop

VD:
	load runtime update parameters to kdrv
	release(unlock) buffer when no drame end between two vd
    if streaming : cur header prepare,
     			   updte output size
*/

static INT32 ctl_sie_isr(UINT32 id, UINT32 status, void *in_data, void *out_data)
{
	INT32 rt = CTL_SIE_E_OK;

	vos_flag_iclr(ctl_sie_get_flag_id(id), CTL_SIE_FLG_ISR_PRC_END);

	switch (sie_ctl_int_flg_ctl[id]->last_isr_sts) {
	case CTL_SIE_INTE_VD:
		if (status & CTL_SIE_INTE_CRPST) {
			rt |= ctl_sie_crp_st_isr_proc(id);
		}
		if (status & CTL_SIE_INTE_BP3) {
			rt |= ctl_sie_bp3_isr_proc(id);
		}
		if (status & CTL_SIE_INTE_CROPEND) {
			rt |= ctl_sie_crp_end_isr_proc(id);
		}
		if (status & CTL_SIE_INTE_VD) {
			rt |= ctl_sie_vd_isr_proc(id);
		}
		break;

	case CTL_SIE_INTE_BP3:
		if (status & CTL_SIE_INTE_CROPEND) {
			rt |= ctl_sie_crp_end_isr_proc(id);
		}
		if (status & CTL_SIE_INTE_VD) {
			rt |= ctl_sie_vd_isr_proc(id);
		}
		if (status & CTL_SIE_INTE_CRPST) {
			rt |= ctl_sie_crp_st_isr_proc(id);
		}
		if (status & CTL_SIE_INTE_BP3) {
			rt |= ctl_sie_bp3_isr_proc(id);
		}
		break;

	default:
	case CTL_SIE_INTE_CROPEND:
		if (status & CTL_SIE_INTE_VD) {
			rt |= ctl_sie_vd_isr_proc(id);
		}
		if (status & CTL_SIE_INTE_CRPST) {
			rt |= ctl_sie_crp_st_isr_proc(id);
		}
		if (status & CTL_SIE_INTE_BP3) {
			rt |= ctl_sie_bp3_isr_proc(id);
		}
		if (status & CTL_SIE_INTE_CROPEND) {
			rt |= ctl_sie_crp_end_isr_proc(id);
		}
		break;
	}
	vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_ISR_PRC_END);

	ctl_sie_module_isp_cb_trig(id, status);	//update statistic out and trigger isp evt
	if ((sie_ctl_info[id]->evt_hdl != 0) && (sie_ctl_info[id]->ctrl.ext_isrcb_fp.sts & status)) {
		sie_event_proc_isr(sie_ctl_info[id]->evt_hdl, CTL_SIE_CBEVT_ENG_SIE_ISR, (void *)&status, NULL);	// trigger external isr cb
	}
	return rt;
}

#if 0
#endif

/** Set functions **/

static INT32 ctl_sie_set_reg_cb(CTL_SIE_ID id, void *data)
{
	CTL_SIE_REG_CB_INFO *p_data = (CTL_SIE_REG_CB_INFO *)data;
	CTL_SIE_INTE sie_inte = CTL_SIE_CLR;
	CHAR *cbevt_name_str[] = {
		"SIE1_ISR",
		"SIE2_ISR",
		"SIE3_ISR",
		"SIE4_ISR",
		"SIE5_ISR",
	};

	switch (p_data->cbevt) {
	case CTL_SIE_CBEVT_BUFIO:
		sie_ctl_info[id]->ctrl.bufiocb.cb_fp = p_data->fp;
		sie_ctl_info[id]->ctrl.bufiocb.sts = p_data->sts;
		return CTL_SIE_E_OK;

	case CTL_SIE_CBEVT_DIRECT:
		sie_ctl_info[id]->ctrl.direct_cb_fp.cb_fp = p_data->fp;
		sie_ctl_info[id]->ctrl.direct_cb_fp.sts = p_data->sts;
		return CTL_SIE_E_OK;

	case CTL_SIE_CBEVT_ENG_SIE_ISR:
		//ext isr cb
		sie_ctl_info[id]->ctrl.ext_isrcb_fp.cb_fp = p_data->fp;
		if (sie_ctl_info[id]->ctrl.ext_isrcb_fp.cb_fp  == NULL) {
			//unregister event and clear interrupte enable
			sie_inte = sie_ctl_info[id]->ctrl.ext_isrcb_fp.sts;
			sie_ctl_info[id]->ctrl.ext_isrcb_fp.sts = CTL_SIE_CLR;
			ctl_sie_update_inte(id, DISABLE, sie_inte);
			sie_event_unregister(sie_ctl_info[id]->evt_hdl, p_data->cbevt, p_data->fp);
		} else {
			sie_ctl_info[id]->ctrl.ext_isrcb_fp.sts = p_data->sts;
			ctl_sie_update_inte(id, ENABLE, p_data->sts);
			sie_event_register(sie_ctl_info[id]->evt_hdl, p_data->cbevt, p_data->fp, cbevt_name_str[id]);
		}
		return CTL_SIE_E_OK;

	default:
	case CTL_SIE_CBEVT_MAX:
		return CTL_SIE_E_NOSPT;
	}
}

static INT32 ctl_sie_set_mclk(CTL_SIE_ID id, void *data)
{
	CTL_SIE_MCLK *sie_mclk_info = (CTL_SIE_MCLK *)data;

	ctl_sie_mclk_info[id].clk_en = sie_mclk_info->mclk_en;
	ctl_sie_mclk_info[id].mclk_id_sel = sie_mclk_info->mclk_id_sel;
	if (sie_ctl_info[id] != NULL && sie_mclk_info->mclk_en == ENABLE) {
		if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN) {
#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
			ctl_sie_mclk_info[id].mclk_src_sel = CTL_SIE_MCLK_SRC_PLL5;
			ctl_sie_mclk_info[id].clk_rate = 1000000;
#else
			ctl_sie_mclk_info[id].mclk_src_sel = CTL_SIE_MCLK_SRC_480;
			ctl_sie_mclk_info[id].clk_rate = 480000000/((ALIGN_CEIL(480000000, ctl_sie_limit[id].max_mclk_rate)/ctl_sie_limit[id].max_mclk_rate));
#endif
			ctl_sie_mclk_info[id].mclk_id_sel = CTL_SIE_ID_MCLK;
		} else if ((sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SEN_IN) || (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SIG_DUPL)) {
			if (ctl_sie_module_cfg_sie_mclk(id, sie_ctl_info[id]->rtc_param.chg_senmode_info.cfg_sen_mode) != CTL_SIE_E_OK) {
				return CTL_SIE_E_SEN;
			}
		}
	}

	return kdf_sie_set_mclk(id, (void *)&ctl_sie_mclk_info[id]);
}

/*
NT96520:
    SIE1: support all output dest
    SIE2: support DRAM and
          DIRECT(only when shdr sensor)
*/
static INT32 ctl_sie_set_out_dest(CTL_SIE_ID id, void *data)
{
	CTL_SIE_OUT_DEST sie_out_dest = *(CTL_SIE_OUT_DEST *)data;
	INT32 rt = CTL_SIE_E_OK;

	switch (sie_out_dest) {
	case CTL_SIE_OUT_DEST_DIRECT:
		if (id == CTL_SIE_ID_1) {
			if (ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_DIRECT) {
				sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_DIRECT;
			} else {
				ctl_sie_dbg_wrn("id %d N.S. Dir mode\r\n", (int)(id+1));
				sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_DRAM;
				rt = CTL_SIE_E_NOSPT;
			}
		} else if (id == CTL_SIE_ID_2) {
			if ((sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SIG_DUPL)||(sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN)) {
				//SIE2 direct still need prepare ring buffer
				sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_DIRECT;
			} else {
				ctl_sie_dbg_wrn("id %d N.S. Dir mode\r\n", (int)(id+1));
				sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_DRAM;
				rt = CTL_SIE_E_NOSPT;
			}
		} else {
			ctl_sie_dbg_wrn("id %d N.S. Dir mode\r\n", (int)(id+1));
			sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_DRAM;
			rt = CTL_SIE_E_NOSPT;
		}

		break;

	case CTL_SIE_OUT_DEST_BOTH:
		if (id == CTL_SIE_ID_1) {
			if (ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_DIRECT) {
				sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_BOTH;
			} else {
				ctl_sie_dbg_wrn("id %d N.S. Dir mode\r\n", (int)(id+1));
				sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_DRAM;
				rt = CTL_SIE_E_NOSPT;
			}
		} else if (id == CTL_SIE_ID_2 || id == CTL_SIE_ID_3) {
			ctl_sie_dbg_wrn("id %d N.S. Both mode\r\n", (int)(id+1));
			sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_DRAM;
			rt = CTL_SIE_E_NOSPT;
		}
		break;

	default:
	case CTL_SIE_OUT_DEST_DRAM:
		sie_ctl_info[id]->rtc_param.out_dest = CTL_SIE_OUT_DEST_DRAM;
		break;
	}

	return rt;
}

static INT32 ctl_sie_set_io_size(CTL_SIE_ID id, void *data)
{
	INT32 rt;
	CTL_SIE_IO_SIZE_INFO *io_size_info = (CTL_SIE_IO_SIZE_INFO *)data;

	if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN) {
		ctl_sie_dbg_wrn("N.S.\r\n");
		return CTL_SIE_E_NOSPT;
	}

	// dircect mode N.S. runtime change iosize (for ringbuffer fixed crp_h / 2) [IVOT_N12025_CO-159][MTD_IAS_IR-478]
	if ((ctl_sie_get_state_machine(id) == CTL_SIE_STS_RUN) && (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DIRECT)) {
		ctl_sie_dbg_wrn("direct mode N.S. runtime change iosize\r\n");
		return CTL_SIE_E_NOSPT;
	}

	if (io_size_info->iosize_sel == CTL_SIE_IOSIZE_AUTO) {
		sie_ctl_info[id]->rtc_param.io_size_info.iosize_sel = io_size_info->iosize_sel;
		sie_ctl_info[id]->rtc_param.io_size_info.auto_info = io_size_info->auto_info;
		sie_ctl_info[id]->rtc_param.io_size_info.align = io_size_info->align;
		sie_ctl_info[id]->rtc_param.io_size_info.dest_align = io_size_info->dest_align;
	} else {
		sie_ctl_info[id]->rtc_param.io_size_info = *(CTL_SIE_IO_SIZE_INFO *)data;
	}
	rt = ctl_sie_module_cfg_io_size(id);
	ctl_sie_module_update_crp_cfa(id);
	ctl_sie_module_update_ch0_lofs(id, 0);
	ctl_sie_module_update_ch1_lofs(id);
	ctl_sie_module_update_bp3(id, 0, FALSE);

	ctl_sie_iosize[id] = TRUE;
	return rt;
}

static INT32 ctl_sie_set_pat_gen_info(CTL_SIE_ID id, void *data)
{
	CTL_SIE_PAG_GEN_INFO *p_sie_pag_gen_info = (CTL_SIE_PAG_GEN_INFO *)data;
	UPOINT shift = {0};
	UINT32 act_pad_width = 0;
	UINT32 i;
	INT32 rt = CTL_SIE_E_OK;

	if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->chk_msg_type(id, CTL_SIE_DBG_MSG_FORCE_PATGEN)) {
		if (p_sie_pag_gen_info->pat_gen_mode >= CTL_SIE_PAT_MAX) {
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.w = 0;//set src win to 0 for disable force pat gen
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h = 0;//set src win to 0 for disable force pat gen
		} else {
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.w = sie_ctl_info[id]->rtc_ctrl.sie_act_win.x + sie_ctl_info[id]->rtc_ctrl.sie_act_win.w;
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h = sie_ctl_info[id]->rtc_ctrl.sie_act_win.y + sie_ctl_info[id]->rtc_ctrl.sie_act_win.h;
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_mode = p_sie_pag_gen_info->pat_gen_mode;
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_val = p_sie_pag_gen_info->pat_gen_val;
		}
	} else {
		if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN) {
			// check align
			if (!CTL_SIE_CHK_ALIGN(p_sie_pag_gen_info->act_win.w, ctl_sie_limit[id].act_win_align.w)) {
				ctl_sie_dbg_wrn("id %d Act win w(%d) need algin to %d\r\n", (int)id, (int)(p_sie_pag_gen_info->act_win.w), (int)(ctl_sie_limit[id].act_win_align.w));
				p_sie_pag_gen_info->act_win.w = ALIGN_FLOOR(p_sie_pag_gen_info->act_win.w, ctl_sie_limit[id].act_win_align.w);
				rt = CTL_SIE_E_PAR;
			}

			if (!CTL_SIE_CHK_ALIGN(p_sie_pag_gen_info->act_win.h, ctl_sie_limit[id].act_win_align.h)) {
				ctl_sie_dbg_wrn("id %d Act win h(%d) need algin to %d\r\n", (int)id, (int)(p_sie_pag_gen_info->act_win.h), (int)(ctl_sie_limit[id].act_win_align.h));
				p_sie_pag_gen_info->act_win.h = ALIGN_FLOOR(p_sie_pag_gen_info->act_win.h, ctl_sie_limit[id].act_win_align.h);
				rt = CTL_SIE_E_PAR;
			}

			//active start x should > active_width * CTL_SIE_PAT_GEN_SRC_W_PAD_RATIO / 100 (5%) (520/528)
			act_pad_width = ALIGN_CEIL(p_sie_pag_gen_info->act_win.w * CTL_SIE_PAT_GEN_SRC_W_PAD_RATIO / 100, ctl_sie_limit[id].act_win_align.w);
			if (p_sie_pag_gen_info->act_win.x < act_pad_width) {
				p_sie_pag_gen_info->act_win.x = act_pad_width + (p_sie_pag_gen_info->act_win.x % 2);
			}

			//active start y shout >= 8 (520/528)
			if (p_sie_pag_gen_info->act_win.y < CTL_SIE_PAT_GEN_SRC_H_PAD) {
				p_sie_pag_gen_info->act_win.y = CTL_SIE_PAT_GEN_SRC_H_PAD + (p_sie_pag_gen_info->act_win.y % 2);
			}

			sie_ctl_info[id]->rtc_ctrl.sie_act_win = p_sie_pag_gen_info->act_win;
			shift.x = sie_ctl_info[id]->rtc_ctrl.sie_act_win.x;
			shift.y = sie_ctl_info[id]->rtc_ctrl.sie_act_win.y;
			sie_ctl_info[id]->rtc_ctrl.rawcfa_act = ctl_sie_module_cal_cfa(CTL_SIE_RGGB_PIX_R, shift);

			if (!CTL_SIE_CHK_ALIGN(p_sie_pag_gen_info->crp_win.w, ctl_sie_limit[id].crp_win_align.w)) {
				ctl_sie_dbg_wrn("id %d Crp win w(%d) need algin to %d\r\n", (int)id, (int)(p_sie_pag_gen_info->crp_win.w), (int)(ctl_sie_limit[id].crp_win_align.w));
				p_sie_pag_gen_info->crp_win.w = ALIGN_FLOOR(p_sie_pag_gen_info->crp_win.w, ctl_sie_limit[id].crp_win_align.w);
			}

			if (!CTL_SIE_CHK_ALIGN(p_sie_pag_gen_info->crp_win.h, ctl_sie_limit[id].crp_win_align.h)) {
				ctl_sie_dbg_wrn("id %d Crp w h(%d) need algin to %d\r\n", (int)id, (int)(p_sie_pag_gen_info->crp_win.h), (int)(ctl_sie_limit[id].crp_win_align.h));
				p_sie_pag_gen_info->crp_win.h = ALIGN_FLOOR(p_sie_pag_gen_info->crp_win.h, ctl_sie_limit[id].crp_win_align.h);
			}

			sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp = p_sie_pag_gen_info->crp_win;
			//force disable raw_scale and dest_crop when patgen mode
			sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_scl_out.w = p_sie_pag_gen_info->crp_win.w;
			sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_scl_out.h = p_sie_pag_gen_info->crp_win.h;
			sie_ctl_info[id]->rtc_param.io_size_info.size_info.dest_crp = p_sie_pag_gen_info->crp_win;
			if (sie_ctl_info[id]->rtc_param.data_fmt == CTL_SIE_YUV_422_SPT || sie_ctl_info[id]->rtc_param.data_fmt == CTL_SIE_YUV_420_SPT) {
				sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_scl_out.w = sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp.w / 2;
				sie_ctl_info[id]->rtc_param.io_size_info.size_info.dest_crp.w = sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp.w / 2;
			}
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_mode = p_sie_pag_gen_info->pat_gen_mode;
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_val = p_sie_pag_gen_info->pat_gen_val;
			if (p_sie_pag_gen_info->frame_rate == 0) {
				ctl_sie_dbg_wrn("Input fps 0\r\n");
				p_sie_pag_gen_info->frame_rate = 3000;
			}
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.frame_rate = p_sie_pag_gen_info->frame_rate;

			//source height should >= act_start_y + act_start_h + CTL_SIE_PAT_GEN_SRC_H_PAD (520/528)
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.w = ALIGN_FLOOR(p_sie_pag_gen_info->act_win.x + p_sie_pag_gen_info->act_win.w + act_pad_width, ctl_sie_limit[id].pat_gen_src_win_align.w);
			sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h = ((ctl_sie_mclk_info[id].clk_rate/sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.w)*100)/p_sie_pag_gen_info->frame_rate;
			if (sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h > CTL_SIE_PAT_GEN_SRC_WIN_MAX) {
				sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h = CTL_SIE_PAT_GEN_SRC_WIN_MAX;
				sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.w = ALIGN_FLOOR(((ctl_sie_mclk_info[id].clk_rate/sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h)*100)/p_sie_pag_gen_info->frame_rate, ctl_sie_limit[id].pat_gen_src_win_align.w);
				ctl_sie_dbg_wrn("fps(x100) adjust from %d to %d\r\n", (int)(sie_ctl_info[id]->rtc_ctrl.pat_gen_param.frame_rate), (int)(ctl_sie_mclk_info[id].clk_rate/((sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.w * sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h) / 100)));
			} else if (sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h < (p_sie_pag_gen_info->act_win.y + p_sie_pag_gen_info->act_win.h + CTL_SIE_PAT_GEN_SRC_H_PAD)) {
				sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h = ALIGN_FLOOR(p_sie_pag_gen_info->act_win.y + p_sie_pag_gen_info->act_win.h + CTL_SIE_PAT_GEN_SRC_H_PAD, ctl_sie_limit[id].pat_gen_src_win_align.h);
				ctl_sie_dbg_wrn("fps(x100) adjust from %d to %d\r\n", (int)(sie_ctl_info[id]->rtc_ctrl.pat_gen_param.frame_rate), (int)(ctl_sie_mclk_info[id].clk_rate/((sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.w * sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h) / 100)));
			} else {
				sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h = ALIGN_FLOOR(sie_ctl_info[id]->rtc_ctrl.pat_gen_param.pat_gen_src_win.h, ctl_sie_limit[id].pat_gen_src_win_align.h);
			}

			sie_ctl_info[id]->ctrl.total_frame_num = 0;
			for (i = CTL_SIE_ID_1; i <= ctl_sie_limit[id].max_spt_id; i++) {
				if (p_sie_pag_gen_info->out_dest & (1 << i)) {
					sie_ctl_info[id]->ctrl.total_frame_num ++;
				}
			}
			if (sie_ctl_info[id]->ctrl.total_frame_num == 0) {
				sie_ctl_info[id]->rtc_ctrl.pat_gen_param.group_idx = (1 << id);
			} else {
				sie_ctl_info[id]->rtc_ctrl.pat_gen_param.group_idx = p_sie_pag_gen_info->out_dest;
			}

			ctl_sie_module_update_ch0_lofs(id, 0);
			ctl_sie_module_update_crp_cfa(id);
			ctl_sie_module_update_load_flg(id, CTL_SIE_ITEM_IO_SIZE);
		} else {
				ctl_sie_dbg_wrn("N.S.\r\n");
				rt = CTL_SIE_E_NOSPT;
		}
	}

	return rt;
}

static INT32 ctl_sie_set_data_fmt(CTL_SIE_ID id, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_DATAFORMAT data_fmt = *(CTL_SIE_DATAFORMAT *)data;

	/* set CCIR SIE output info */
	if (ctl_sie_typecast_ccir(data_fmt)) {
		sie_ctl_info[id]->rtc_param.data_fmt = data_fmt;
		// for split or no-split update sie io size
		if (sie_ctl_info[id]->rtc_param.chg_senmode_info.cfg_sen_mode != CTL_SEN_MODE_UNKNOWN && ctl_sie_iosize[id] == TRUE) {
			rt = ctl_sie_module_cfg_io_size(id);
		} else if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN) {
			if (data_fmt == CTL_SIE_YUV_422_SPT || data_fmt == CTL_SIE_YUV_420_SPT) {
				sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_scl_out.w = sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp.w / 2;
				sie_ctl_info[id]->rtc_param.io_size_info.size_info.dest_crp.w = sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp.w / 2;
			}
		}
	} else {
		if (ctl_sie_get_state_machine(id) == CTL_SIE_STS_RUN && sie_ctl_info[id]->rtc_param.encode_en) {
			if (data_fmt != CTL_SIE_BAYER_12) {
				ctl_sie_dbg_err("raw enc must be BAYER_12, disable raw enc first\r\n");
				return CTL_SIE_E_PAR;
			}
		}

		sie_ctl_info[id]->rtc_param.data_fmt = data_fmt;
	}
	ctl_sie_module_update_ch0_lofs(id, 0);
	ctl_sie_module_update_ch1_lofs(id);
	return rt;
}

static INT32 ctl_sie_set_sen_mode_chg_info(CTL_SIE_ID id, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	UINT32 i;

	if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SEN_IN) {
		sie_ctl_info[id]->rtc_param.chg_senmode_info = *(CTL_SIE_CHGSENMODE_INFO *)data;
		sie_ctl_info[id]->rtc_param.chg_senmode_info.cfg_sen_mode = ctl_sie_module_get_sen_mode(id);

		rt = ctl_sie_module_cfg_sen(id, sie_ctl_info[id]->rtc_param.chg_senmode_info.cfg_sen_mode);
		if (rt == CTL_SIE_E_OK && sie_ctl_info[id]->ctrl.total_frame_num > 1) {
			if (sie_ctl_info[id]->rtc_param.chg_senmode_info.output_dest != CTL_SEN_OUTPUT_DEST_AUTO) {	//auto only support linear mode
				sie_ctl_info[id]->rtc_param.chg_senmode_info.output_dest |= 1 << id;
				for (i = CTL_SIE_ID_1; i <= ctl_sie_limit[id].max_spt_id; i++) {
					if (sie_ctl_info[i] != NULL) {
						if ((1 << i) & sie_ctl_info[id]->rtc_param.chg_senmode_info.output_dest) {
							sie_ctl_info[i]->rtc_param.chg_senmode_info = sie_ctl_info[id]->rtc_param.chg_senmode_info;
							sie_ctl_info[i]->ctrl.total_frame_num = sie_ctl_info[id]->ctrl.total_frame_num;
							rt |= ctl_sie_module_cfg_sen(i, sie_ctl_info[id]->rtc_param.chg_senmode_info.cfg_sen_mode);
						}
					}
				}
			} else {
				ctl_sie_dbg_err("OUT_DEST_AUTO must be linear mode\r\n");
			}
		}
	} else if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SIG_DUPL) {
		if (sie_ctl_info[sie_ctl_info[id]->dupl_src_id] != NULL) {
			sie_ctl_info[id]->rtc_param.chg_senmode_info = sie_ctl_info[sie_ctl_info[id]->dupl_src_id]->rtc_param.chg_senmode_info;
			sie_ctl_info[id]->ctrl.total_frame_num = sie_ctl_info[sie_ctl_info[id]->dupl_src_id]->ctrl.total_frame_num;
			rt = ctl_sie_module_cfg_sen(id, sie_ctl_info[id]->rtc_param.chg_senmode_info.cfg_sen_mode);
		}
	}

	return rt;
}

static INT32 ctl_sie_set_flip(CTL_SIE_ID id, void *data)
{
	sie_ctl_info[id]->rtc_param.flip = *(CTL_SIE_FLIP_TYPE *)data;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_set_enc_en(CTL_SIE_ID id, void *data)
{
	BOOL  encode_en = *(BOOL *)data;

	if ((id == CTL_SIE_ID_2) && (ctl_sie_get_state_machine(id) == CTL_SIE_STS_RUN) && (sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DIRECT)) {
		//ctl_sie_dbg_ind("SIE2 Direct mode force set encode enable when trigger start\r\n");
		//NT96520 SIE2 Direct mode force set encode enable when trigger start, and not support runtime change encode setting
	} else {
		if (encode_en) {
			if (ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_RAWENC && ctl_sie_typecast_raw(sie_ctl_info[id]->rtc_param.data_fmt)) {
				if (ctl_sie_get_state_machine(id) == CTL_SIE_STS_RUN && sie_ctl_info[id]->rtc_param.data_fmt != CTL_SIE_BAYER_12) {
					ctl_sie_dbg_err("raw enc only support BAYER_12\r\n");
					return CTL_SIE_E_PAR;
				}
				sie_ctl_info[id]->rtc_param.encode_en = ENABLE;
			} else {
				ctl_sie_dbg_err("id %d fmt %d N.S. encode_en=1\r\n", id, sie_ctl_info[id]->rtc_param.data_fmt);
				return CTL_SIE_E_NOSPT;
			}
		} else {
			sie_ctl_info[id]->rtc_param.encode_en = DISABLE;
		}
		ctl_sie_module_update_ch0_lofs(id, 0);
	}
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_set_enc_rate(CTL_SIE_ID id, void *data)
{
	if (id == CTL_SIE_ID_2 && sie_ctl_info[id]->param.out_dest == CTL_SIE_OUT_DEST_DIRECT) {
//		ctl_sie_dbg_wrn("SIE2 Direct mode force set encode enable when trigger start\r\n");
		//NT96520 SIE2 Direct mode force set encode enable when trigger start, and not support runtime change encode setting
	} else {
		sie_ctl_info[id]->rtc_param.enc_rate = *(CTL_SIE_ENC_RATE_SEL *)data;

		if (sie_ctl_info[id]->rtc_param.encode_en) {
			ctl_sie_module_update_ch0_lofs(id, 0);
		}
	}
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_set_ccir(CTL_SIE_ID id, void *data)
{
#if 0	//force return this item, nt9652x will run progress mode and skip frame for ccir interlace sensor
	CTL_SIE_CCIR_INFO ccir_info = *(CTL_SIE_CCIR_INFO *)data;

	/* set field */
	if (ccir_info.field_sel == CTL_SIE_FIELD_DISABLE) {
		sie_ctl_info[id]->rtc_param.ccir_info_int.b_filed_en = FALSE;
	} else if (ccir_info.field_sel == CTL_SIE_FIELD_EN_0) {
		sie_ctl_info[id]->rtc_param.ccir_info_int.b_filed_en = TRUE;
		sie_ctl_info[id]->rtc_param.ccir_info_int.b_filed_sel = 0;
	} else if (ccir_info.field_sel == CTL_SIE_FIELD_EN_1) {
		sie_ctl_info[id]->rtc_param.ccir_info_int.b_filed_en = TRUE;
		sie_ctl_info[id]->rtc_param.ccir_info_int.b_filed_sel = 1;
	} else {
		ctl_sie_dbg_err("field_sel err %d\r\n", (int)(ccir_info.field_sel));
		return CTL_SIE_E_PAR;
	}
#endif
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_set_mux_info(CTL_SIE_ID id, void *data)
{
	CTL_SIE_MUX_DATA_INFO *mux_info = (CTL_SIE_MUX_DATA_INFO *)data;

	sie_ctl_info[id]->rtc_param.ccir_info_int.data_idx = mux_info->data_idx;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_set_alg_func(CTL_SIE_ID id, void *data)
{
	CTL_SIE_ALG_FUNC *sie_alg_func = (CTL_SIE_ALG_FUNC *)data;

	if (sie_alg_func->type >= CTL_SIE_ALG_TYPE_MAX) {
		ctl_sie_dbg_err("N.S. type %d\r\n", (int)(sie_alg_func->type));
		return CTL_SIE_E_NOSPT;
	}

	if (sie_alg_func->func_en) {
		switch (sie_alg_func->type) {
		case CTL_SIE_ALG_TYPE_AWB:
			if (!(ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_CA)) {
				ctl_sie_dbg_wrn("id %d N.S. type %d\r\n", (int)id, (int)(sie_alg_func->type));
				return CTL_SIE_E_NOSPT;
			}
			break;

		case CTL_SIE_ALG_TYPE_AE:
			if (!(ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_LA)) {
				ctl_sie_dbg_wrn("id %d N.S. type %d\r\n", (int)id, (int)(sie_alg_func->type));
				return CTL_SIE_E_NOSPT;
			}
			break;

		case CTL_SIE_ALG_TYPE_AF:
		case CTL_SIE_ALG_TYPE_SHDR:
		case CTL_SIE_ALG_TYPE_WDR:
			break;

		default:
			ctl_sie_dbg_wrn("id %d N.S type %d\r\n", (int)id, (int)(sie_alg_func->type));
			return CTL_SIE_E_NOSPT;
		}
	}

	sie_ctl_info[id]->ctrl.alg_func_en[sie_alg_func->type] = sie_alg_func->func_en;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_set_reset_fc(CTL_SIE_ID id, void *data)
{
	UINT32 i;
	CTL_SIE_ID_IDX sie_rst_idx = 0;

	ctl_sie_set_dbg_proc_t(id, "reset_fc", CTL_SIE_PROC_TIME_ITEM_ENTER);
#if defined(__KERNEL__)
	//skip reset shdr frame count when fastboot case
		if (!sie_ctl_info[id]->fb_obj.normal_shdr_rst && fastboot) {
			sie_ctl_info[id]->fb_obj.normal_shdr_rst = TRUE;
		return CTL_SIE_E_OK;
	}
#endif
	if (data != NULL) {
		sie_rst_idx = *(CTL_SIE_ID_IDX *)data;
		if (sie_rst_idx > CTL_SIE_ID_IDX_MAX) {
			ctl_sie_dbg_err("reset idx 0x%x overflow, max 0x%x\r\n", sie_rst_idx, CTL_SIE_ID_IDX_MAX);
			return CTL_SIE_E_PAR;
		}
	}

	if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN) {
		sie_rst_idx = sie_ctl_info[id]->rtc_ctrl.pat_gen_param.group_idx;
	}

	//check reset flow status
	for (i = CTL_SIE_ID_1; i <= ctl_sie_limit[id].max_spt_id; i++) {
		//check group
		if (sie_ctl_info[i] != NULL) {
			if (((1 << i) & sie_ctl_info[id]->param.chg_senmode_info.output_dest) || ((1 << i) & sie_rst_idx)) {
				if (sie_ctl_info[i]->ctrl.rst_fc_sts == CTL_SIE_RST_FC_BEGIN) {
					ctl_sie_dbg_err("reset fc flow not end\r\n");
					return CTL_SIE_E_STATE;
				}
			}
		}
	}

	//set status from none to begin
	for (i = CTL_SIE_ID_1; i <= ctl_sie_limit[id].max_spt_id; i++) {
		if (sie_ctl_info[i] != NULL) {
			if (((1 << i) & sie_ctl_info[id]->param.chg_senmode_info.output_dest) || ((1 << i) & sie_rst_idx)) {
				sie_ctl_info[i]->ctrl.rst_fc_sts = CTL_SIE_RST_FC_BEGIN;
			}
		}
	}
	ctl_sie_set_dbg_proc_t(id, "reset_fc", CTL_SIE_PROC_TIME_ITEM_EXIT);
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_set_bp3_ratio(CTL_SIE_ID id, void *data)
{
	UINT32 bp3_ratio = *(UINT32 *)data;

	if (g_ctl_sie_status[id] == CTL_SIE_STS_RUN) {
		ctl_sie_dbg_wrn("not support run time changes\r\n");
		return CTL_SIE_E_SYS;
	} else {
		if (bp3_ratio <= 100) {
			sie_ctl_info[id]->ctrl.bp3_ratio = *(UINT32 *)data;
		} else {
			sie_ctl_info[id]->ctrl.bp3_ratio = 0;
			ctl_sie_dbg_wrn("bp3 ratio %d overflow (value should 0~100)\r\n", bp3_ratio);
		}
		return CTL_SIE_E_OK;
	}
}

static INT32 ctl_sie_set_bp3_ratio_direct(CTL_SIE_ID id, void *data)
{
	UINT32 bp3_ratio = *(UINT32 *)data;

	if (g_ctl_sie_status[id] == CTL_SIE_STS_RUN) {
		ctl_sie_dbg_wrn("not support run time changes\r\n");
		return CTL_SIE_E_SYS;
	} else {
		if (bp3_ratio <= 100) {
			sie_ctl_info[id]->ctrl.bp3_ratio_direct = *(UINT32 *)data;
		} else {
			sie_ctl_info[id]->ctrl.bp3_ratio_direct = 0;
			ctl_sie_dbg_wrn("bp3 ratio %d overflow (value should 0~100)\r\n", bp3_ratio);
		}
		return CTL_SIE_E_OK;
	}
}

static INT32 ctl_sie_set_gyro_cfg(CTL_SIE_ID id, void *data)
{
	sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg = *(CTL_SIE_GYRO_CFG *)data;

	//set gyro start/stop when streaming
	//or sie will start/stop gyro when trigger start/stop
	if (g_ctl_sie_status[id] == CTL_SIE_STS_RUN) {
		if (sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg.en) {
			ctl_sie_module_config_gyro_start(id);
		} else {
			ctl_sie_module_config_gyro_stop(id);
		}
	}
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_set_load(CTL_SIE_ID id, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	unsigned long lock_flags;

	//sel/chg sensor mode and update sie parameters
	if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_SEN_IN || sie_ctl_info[id]->rtc_param.chg_senmode_info.auto_info.mux_singnal_en) {
		if (sie_ctl_int_flg_ctl[id]->set_load_flg & 1ULL << CTL_SIE_ITEM_CHGSENMODE) {
			loc_cpu(lock_flags);
			sie_ctl_info[id]->load_param.chg_senmode_info = sie_ctl_info[id]->rtc_param.chg_senmode_info;
			sie_ctl_info[id]->param.chg_senmode_info = sie_ctl_info[id]->rtc_param.chg_senmode_info;
			unl_cpu(lock_flags);
			vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_CH_SEN_MODE);
			ctl_sie_set_dbg_proc_t(id, "sen_chgmode", CTL_SIE_PROC_TIME_ITEM_ENTER);
			if (ctl_sie_module_sen_chgmode(id) != CTL_SIE_E_OK) {
				rt = CTL_SIE_E_SEN;
				ctl_sie_dbg_err("sen chgmode fail\r\n");
			}
			ctl_sie_set_dbg_proc_t(id, "sen_chgmode", CTL_SIE_PROC_TIME_ITEM_EXIT);
			vos_flag_clr(ctl_sie_get_flag_id(id), CTL_SIE_FLG_CH_SEN_MODE);
		}
	}

	loc_cpu(lock_flags);
	sie_ctl_int_flg_ctl[id]->isr_load_en_flg = TRUE;
	sie_ctl_info[id]->load_param = sie_ctl_info[id]->rtc_param;
	sie_ctl_info[id]->load_ctrl = sie_ctl_info[id]->rtc_ctrl;
	sie_ctl_info[id]->ctrl.update_item |= sie_ctl_int_flg_ctl[id]->set_load_flg;
	sie_ctl_int_flg_ctl[id]->set_load_flg = 0;
	unl_cpu(lock_flags);

	//streaming mode, update output buffer size/ctl_sie parameters in next vd interrupt
	if (g_ctl_sie_status[id] == CTL_SIE_STS_READY) {
		ctl_sie_hdl_load_all(id);
	}

	return rt;
}

static INT32 ctl_sie_set_trig(CTL_SIE_ID id, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	FLGPTN wait_flag = 0;
	CTL_SIE_TRIG_INFO trig_info = *(CTL_SIE_TRIG_INFO *)data;

	if (trig_info.trig_type == CTL_SIE_TRIG_START) {
		ctl_sie_dbg_ind("[Trig][Start]id:%d Begin\r\n", (int)id);
		ctl_sie_set_dbg_proc_t(id, "trig_start", CTL_SIE_PROC_TIME_ITEM_ENTER);
		// check state machine
		if (ctl_sie_get_state_machine(id) != CTL_SIE_STS_READY) {
			return CTL_SIE_E_STATE;
		}

		kdf_sie_get(sie_ctl_info[id]->kdf_hdl, CTL_SIE_INT_ITEM_CLK | CTL_SIE_IGN_CHK, (void *)&sie_ctl_info[id]->ctrl.clk_info.drv_data_rate);
		if (sie_ctl_info[id]->ctrl.clk_info.data_rate > sie_ctl_info[id]->ctrl.clk_info.drv_data_rate) {
			ctl_sie_dbg_err("id %d SIE clk %d < sensor require min %d\r\n", (int)(id), (int)sie_ctl_info[id]->ctrl.clk_info.drv_data_rate, (int)sie_ctl_info[id]->ctrl.clk_info.data_rate);
		}

		sie_ctl_info[id]->ctrl.trig_info = trig_info;
		sie_ctl_info[id]->ctrl.gyro_ctl_info.chk_latency_en = ENABLE;
		ctl_sie_module_update_bp3(id, 0, TRUE);	//reset bp3 to initial value
		memset((void *)&sie_ctl_info[id]->ctrl.frame_ctl_info, 0, sizeof(CTL_SIE_FRM_CTL_INFO));
		memset((void *)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR], 0, sizeof(CTL_SIE_HEADER_INFO) * CTL_SIE_HEAD_IDX_MAX);
		//set reset frame_cnt flag to NONE for initial value when shdr flow
		if (sie_ctl_info[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_SHDR] && (sie_ctl_info[id]->ctrl.total_frame_num > 1)) {
			sie_ctl_info[id]->ctrl.rst_fc_sts = CTL_SIE_RST_FC_NONE;
			sie_ctl_int_flg_ctl[id]->cfg_start_bp = sie_ctl_info[id]->ctrl.bp3;//save bp value
			ctl_sie_module_update_bp3(id, (sie_ctl_info[id]->rtc_param.io_size_info.size_info.sie_crp.h >> 1), TRUE);
		} else {
			ctl_sie_update_fc(id);
		}

#if defined(__KERNEL__)
		//set frame count reset status to DONE when fastboot case
		if (!sie_ctl_info[id]->fb_obj.normal_shdr_rst && fastboot) {
			sie_ctl_info[id]->ctrl.trig_info.b_wait_end = FALSE;
			sie_ctl_info[id]->ctrl.rst_fc_sts = CTL_SIE_RST_FC_DONE;
		}
#endif

		if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->reset_ts != NULL) {
			ctl_sie_dbg_tab->reset_ts(id);
		}

		ctl_sie_module_config_gyro_start(id);
		//load first frame parameters and prepare first frame buffer/header
		ctl_sie_set_load(id, NULL);
		ctl_sie_buf_update_out_size(sie_ctl_info[id]);
		if (ctl_sie_module_ring_buf_malloc(id) != CTL_SIE_E_OK) {
			return CTL_SIE_E_NOMEM;
		}
		ctl_sie_module_update_ch0_out_mode(id);
		rt = ctl_sie_header_create(sie_ctl_info[id], CTL_SIE_HEAD_IDX_NEXT);
		if (rt == CTL_SIE_E_NULL_FP) {
			goto trig_err;
		}

		rt = ctl_sie_set_load(id, NULL);	//load first frame address
		if (rt == CTL_SIE_E_SEN) {
			goto trig_err;
		}
		rt = ctl_sie_module_direct_cb_trig(id, CTL_SIE_DIRECT_TRIG_START, FALSE);
		if (rt == CTL_SIE_E_NULL_FP) {
			goto trig_err;
		}

		ctl_sie_set_dbg_proc_t(id, "isp_reset", CTL_SIE_PROC_TIME_ITEM_ENTER);
		ctl_sie_module_isp_cb_trig(id, CTL_SIE_INTE_ALL);	//reset isp parameters
		ctl_sie_set_dbg_proc_t(id, "isp_reset", CTL_SIE_PROC_TIME_ITEM_EXIT);
		vos_flag_iclr(ctl_sie_get_flag_id(id), CTL_SIE_FLG_KDRV_END);
		ctl_sie_set_dbg_proc_t(id, "trig_start(kdrv)", CTL_SIE_PROC_TIME_ITEM_ENTER);
		rt = kdf_sie_trigger(sie_ctl_info[id]->kdf_hdl, (void *)&sie_ctl_info[id]->ctrl.trig_info);
		if (rt != CTL_SIE_E_OK) {
			vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_KDRV_END);
			goto trig_err;
		}
		ctl_sie_set_dbg_proc_t(id, "trig_start(kdrv)", CTL_SIE_PROC_TIME_ITEM_EXIT);
		vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_KDRV_END);
		// set state machine to start
		rt = ctl_sie_module_state_machine(id, CTL_SIE_OP_TRIG_START);
		if (rt != CTL_SIE_E_OK) {
			goto trig_err;
		}
		ctl_sie_set_dbg_proc_t(id, "trig_start", CTL_SIE_PROC_TIME_ITEM_EXIT);
	} else {
		ctl_sie_set_dbg_proc_t(id, "trig_stop", CTL_SIE_PROC_TIME_ITEM_ENTER);
		ctl_sie_dbg_ind("[Trig][Stop]id:%d Begin\r\n", (int)id);
#if defined(__KERNEL__)
		switch (ctl_sie_get_state_machine(id)) {
		case CTL_SIE_STS_RUN:
		case CTL_SIE_STS_READY:
			ctl_sie_fb_trig_stage |= (ctl_sie_fb_trig_stage_normal_open | ctl_sie_fb_trig_stage_normal_trigger);
			break;

		default:
			break;
		}
#endif
		switch (ctl_sie_get_state_machine(id)) {
		case CTL_SIE_STS_RUN:
			sie_ctl_info[id]->ctrl.trig_info.trig_type = trig_info.trig_type;
			sie_ctl_info[id]->ctrl.trig_info.b_wait_end = trig_info.b_wait_end;
			trig_info.b_wait_end = FALSE;	//kdrv no wait end when trig stop
			rt = ctl_sie_module_direct_cb_trig(id, CTL_SIE_DIRECT_TRIG_STOP, FALSE);	//direct trigger
			if (rt != CTL_SIE_E_OK) {
				return rt;
			}
			//check and update state machine
			rt = ctl_sie_module_state_machine(id, CTL_SIE_OP_TRIG_STOP);
			if (rt != CTL_SIE_E_OK) {
				return rt;
			}
			vos_flag_iclr(ctl_sie_get_flag_id(id), CTL_SIE_FLG_KDRV_END);
			ctl_sie_set_dbg_proc_t(id, "trig_stop(kdrv)", CTL_SIE_PROC_TIME_ITEM_ENTER);
			rt = kdf_sie_trigger(sie_ctl_info[id]->kdf_hdl, (void *)&trig_info);
			ctl_sie_set_dbg_proc_t(id, "trig_stop(kdrv)", CTL_SIE_PROC_TIME_ITEM_EXIT);
			vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_KDRV_END);

			if (vos_flag_wait_timeout(&wait_flag, ctl_sie_get_flag_id(id), CTL_SIE_FLG_ISR_PRC_END, TWF_ORW, vos_util_msec_to_tick(1000)) != 0) {
				ctl_sie_dbg_wrn("id %d, stop TO(wait isr proc end)\r\n", (int)(id));
			}
			ctl_sie_set_dbg_isr_ioctl(id, CTL_SIE_CLR, CTL_SIE_BUF_IO_UNLOCK, CTL_SIE_HEAD_IDX_CUR);
			ctl_sie_buf_io_cfg(id, CTL_SIE_BUF_IO_UNLOCK, 0, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR]);
			ctl_sie_set_dbg_isr_ioctl(id, CTL_SIE_CLR, CTL_SIE_BUF_IO_UNLOCK, CTL_SIE_HEAD_IDX_NEXT);
			ctl_sie_buf_io_cfg(id, CTL_SIE_BUF_IO_UNLOCK, 0, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT]);
			ctl_sie_module_ring_buf_mfree(id);	//free sie2 ring buffer
			if (sie_ctl_info[id]->ctrl.trig_info.b_wait_end) {//flush when wait end = TRUE
				ctl_sie_module_queue_flush(id);
			}
			ctl_sie_module_config_gyro_stop(id);
			ctl_sie_isp_rst_to_dft(id);
			memset((void *)sie_ctl_int_flg_ctl[id], 0, sizeof(CTL_SIE_INT_CTL_FLG));
			sie_ctl_int_flg_ctl[id]->isr_dram_end_flg = 1;
			sie_ctl_int_flg_ctl[id]->cfg_start_bp = CTL_SIE_BP_MIN_END_LINE;
			break;

		case CTL_SIE_STS_READY:
			rt = CTL_SIE_E_OK;
			break;

		default:
			rt = CTL_SIE_E_STATE;
			break;
		}
		ctl_sie_set_dbg_proc_t(id, "trig_stop", CTL_SIE_PROC_TIME_ITEM_EXIT);
	}
	ctl_sie_dbg_ind("[Trig]id:%d End %d\r\n", (int)id, rt);
	return rt;


trig_err:
	ctl_sie_module_ring_buf_mfree(id);	//free sie2 ring buffer
	return rt;
}

static INT32 ctl_sie_set_sync_info(CTL_SIE_ID id, void *data)
{
	CTL_SIE_SYNC_CTRL_OBJ *sync_obj;
	CTL_SIE_SYNC_INFO *sync_info = (CTL_SIE_SYNC_INFO *)data;

	if (g_ctl_sie_status[id] == CTL_SIE_STS_RUN) {
		ctl_sie_dbg_wrn("not support run time changes\r\n");
		return CTL_SIE_E_SYS;
	} else {

		//check parameter
		if (sync_info->mode > 2) {
			ctl_sie_dbg_err("sync info err mode(%d > 2)\r\n", sync_info->mode);
			return CTL_SIE_E_SYS;
		}

		if (sync_info->mode == 2) {
			if ((sync_info->det_frm_int == 0) || (sync_info->adj_thres == 0)) {
				ctl_sie_dbg_err("sync info err det_frm_int(%d), adj_thres(%d)\r\n", sync_info->det_frm_int, sync_info->adj_thres);
				return CTL_SIE_E_SYS;
			}

			if (sync_info->sync_id >= CTL_SIE_ID_MAX_NUM) {
				ctl_sie_dbg_err("sync info err sync_id(%d)\r\n", sync_info->sync_id);
				return CTL_SIE_E_SYS;
			}
		}

		if (sync_info->mode != 0) {
			if (sync_enable_cnt >= 2) {
				ctl_sie_dbg_err("sw sync err > 2 sensor 0x%.8x\r\n", sync_enable_bit);
				return CTL_SIE_E_SYS;
			} else {
				sync_enable_cnt += 1;
				sync_enable_bit |= (1 << id);

				sync_obj = &sie_ctl_info[id]->ctrl.sync_obj;
				sync_obj->mode = sync_info->mode;
				if (sync_obj->mode == 2) {
					sync_obj->sync_id = sync_info->sync_id;
					sync_obj->det_frm_int = sync_info->det_frm_int;
					sync_obj->adj_thres = sync_info->adj_thres;
					sync_obj->adj_auto = sync_info->adj_auto;
				} else {
					sync_obj->sync_id = 0;
					sync_obj->det_frm_int = 0;
					sync_obj->adj_thres = 0;
					sync_obj->adj_auto = 0;
				}
			}
		} else {
			if (sync_enable_bit & (1 << id)) {
				sync_enable_cnt -= 1;
				sync_enable_bit &= ~(1 << id);

				sync_obj = &sie_ctl_info[id]->ctrl.sync_obj;
				sync_obj->mode = sync_info->mode;
				sync_obj->sync_id = 0;
				sync_obj->det_frm_int = 0;
				sync_obj->adj_thres = 0;
				sync_obj->adj_auto = 0;
			} else {
				//already disable do nothing
				return CTL_SIE_E_OK;
			}
		}
		return CTL_SIE_E_OK;
	}
}

/** sie set functions **/
CTL_SIE_SET_FP ctl_sie_set_fp[CTL_SIE_ITEM_MAX] = {
	ctl_sie_set_mclk,
	ctl_sie_set_out_dest,
	ctl_sie_set_data_fmt,
	ctl_sie_set_sen_mode_chg_info,
	ctl_sie_set_flip,
	NULL,	//ctl_sie_set_ch0_lof,
	NULL,	//ctl_sie_set_ch1_lof,
	ctl_sie_set_io_size,
	ctl_sie_set_pat_gen_info,
	NULL,//ca rst
	NULL,//la rst
	ctl_sie_set_enc_en,
	ctl_sie_set_enc_rate,
	ctl_sie_set_ccir,
	ctl_sie_set_mux_info,
	ctl_sie_set_trig,
	ctl_sie_set_reg_cb,
	ctl_sie_set_alg_func,
	ctl_sie_set_reset_fc,
	ctl_sie_set_load,
	ctl_sie_set_bp3_ratio,
	ctl_sie_set_bp3_ratio_direct,
	ctl_sie_set_sync_info,
	ctl_sie_set_gyro_cfg,
};

#if 0
#endif

/** Get functions **/

static INT32 ctl_sie_get_mclk(CTL_SIE_ID id, void *data)
{
	*(CTL_SIE_MCLK_INFO *)data = ctl_sie_mclk_info[id];
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_out_dest(CTL_SIE_ID id, void *data)
{
	*(CTL_SIE_OUT_DEST *)data = sie_ctl_info[id]->load_param.out_dest;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_io_size(CTL_SIE_ID id, void *data)
{
	*(CTL_SIE_IO_SIZE_INFO *)data = sie_ctl_info[id]->load_param.io_size_info;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_pat_gen_info(CTL_SIE_ID id, void *data)
{
	CTL_SIE_PAG_GEN_INFO *p_pat_gen_info = (CTL_SIE_PAG_GEN_INFO *)data;

	p_pat_gen_info->act_win = sie_ctl_info[id]->ctrl.rtc_obj.sie_act_win;
	p_pat_gen_info->crp_win = sie_ctl_info[id]->load_param.io_size_info.size_info.sie_crp;
	p_pat_gen_info->pat_gen_mode = sie_ctl_info[id]->load_ctrl.pat_gen_param.pat_gen_mode;
	p_pat_gen_info->pat_gen_val = sie_ctl_info[id]->load_ctrl.pat_gen_param.pat_gen_val;
	p_pat_gen_info->frame_rate = sie_ctl_info[id]->load_ctrl.pat_gen_param.frame_rate;
	p_pat_gen_info->out_dest = sie_ctl_info[id]->load_ctrl.pat_gen_param.group_idx;

	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_data_fmt(CTL_SIE_ID id, void *data)
{
	*(CTL_SIE_DATAFORMAT *)data = sie_ctl_info[id]->load_param.data_fmt;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_sen_mode_chg_info(CTL_SIE_ID id, void *data)
{
	*(CTL_SIE_CHGSENMODE_INFO *)data = sie_ctl_info[id]->load_param.chg_senmode_info;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_flip(CTL_SIE_ID id, void *data)
{
	*(CTL_SIE_FLIP_TYPE *)data = sie_ctl_info[id]->load_param.flip;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_ch0_lof(CTL_SIE_ID id, void *data)
{
	*(UINT32 *)data = sie_ctl_info[id]->load_ctrl.out_ch_lof[CTL_SIE_CH_0];
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_ch1_lof(CTL_SIE_ID id, void *data)
{
	*(UINT32 *)data = sie_ctl_info[id]->load_ctrl.out_ch_lof[CTL_SIE_CH_1];
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_ca_rst(CTL_SIE_ID id, void *data)
{
	if (!(ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_CA)) {
		ctl_sie_dbg_err("id %d N.S CA\r\n", (int)(id));
		return CTL_SIE_E_NOSPT;
	}

	if (sie_ctl_info[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AWB]) {
		return kdf_sie_get(sie_ctl_info[id]->kdf_hdl, CTL_SIE_ITEM_CA_RSLT, (void *)data);
	}
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_la_rst(CTL_SIE_ID id, void *data)
{
	if (!(ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_LA)) {
		ctl_sie_dbg_err("id %d N.S LA\r\n", (int)(id));
		return CTL_SIE_E_NOSPT;
	}

	if (sie_ctl_info[id]->ctrl.alg_func_en[CTL_SIE_ALG_TYPE_AE]) {
		return kdf_sie_get(sie_ctl_info[id]->kdf_hdl, CTL_SIE_ITEM_LA_RSLT, (void *)data);
	}
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_enc_en(CTL_SIE_ID id, void *data)
{
	*(BOOL *)data = sie_ctl_info[id]->load_param.encode_en;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_enc_rate(CTL_SIE_ID id, void *data)
{
	*(CTL_SIE_ENC_RATE_SEL *)data = sie_ctl_info[id]->load_param.enc_rate;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_ccir(CTL_SIE_ID id, void *data)
{
	CTL_SIE_CCIR_INFO *ccir_info = (CTL_SIE_CCIR_INFO *)data;

	ccir_info->field_sel = sie_ctl_info[id]->load_param.ccir_info_int.b_filed_sel;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_mux_info(CTL_SIE_ID id, void *data)
{
	CTL_SIE_MUX_DATA_INFO *mux_info = (CTL_SIE_MUX_DATA_INFO *)data;

	mux_info->data_idx = sie_ctl_info[id]->load_param.ccir_info_int.data_idx;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_alg_func(CTL_SIE_ID id, void *data)
{
	CTL_SIE_ALG_FUNC *alg_func = (CTL_SIE_ALG_FUNC *)data;

	if (alg_func->type < CTL_SIE_ALG_TYPE_MAX) {
		alg_func->func_en = sie_ctl_info[id]->ctrl.alg_func_en[alg_func->type];
	} else {
		ctl_sie_dbg_err("N.S. type %d\r\n", (int)(alg_func->type));
		return CTL_SIE_E_PAR;
	}
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_bp3_ratio(CTL_SIE_ID id, void *data)
{
	*(UINT32 *)data = sie_ctl_info[id]->ctrl.bp3_ratio;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_bp3_ratio_direct(CTL_SIE_ID id, void *data)
{
	*(UINT32 *)data = sie_ctl_info[id]->ctrl.bp3_ratio_direct;
	return CTL_SIE_E_OK;
}

static INT32 ctl_sie_get_gyro_cfg(CTL_SIE_ID id, void *data)
{
	*(CTL_SIE_GYRO_CFG *)data = sie_ctl_info[id]->ctrl.gyro_ctl_info.gyro_cfg;
	return CTL_SIE_E_OK;
}

/** sie get functions **/
CTL_SIE_GET_FP ctl_sie_get_fp[CTL_SIE_ITEM_MAX] = {
	ctl_sie_get_mclk,
	ctl_sie_get_out_dest,
	ctl_sie_get_data_fmt,
	ctl_sie_get_sen_mode_chg_info,
	ctl_sie_get_flip,
	ctl_sie_get_ch0_lof,
	ctl_sie_get_ch1_lof,
	ctl_sie_get_io_size,
	ctl_sie_get_pat_gen_info,
	ctl_sie_get_ca_rst,
	ctl_sie_get_la_rst,
	ctl_sie_get_enc_en,
	ctl_sie_get_enc_rate,
	ctl_sie_get_ccir,
	ctl_sie_get_mux_info,
	NULL,//ctl_sie_set_trig,
	NULL,//reg cb, N.S.
	ctl_sie_get_alg_func,
	NULL,//CTL_SIE_ITEM_RESET_FC
	NULL,//CTL_SIE_ITEM_LOAD,
	ctl_sie_get_bp3_ratio,
	ctl_sie_get_bp3_ratio_direct,
	NULL,//CTL_SIE_ITEM_SW_SYNC
	ctl_sie_get_gyro_cfg,
};

#if 0
#endif

/*
    public function
*/
void kflow_ctl_sie_init(void)
{
	UINT32 i;

	for (i = 0; i < CTL_SIE_MAX_SUPPORT_ID; i++) {
		kdf_sie_get_limit(i, (void *)&ctl_sie_limit[i]);
	}
#if CTL_SIE_FORCE_REG_DBG_CB
	ctl_sie_dbg_set_msg_type(CTL_SIE_ID_1, CTL_SIE_DBG_MSG_REG_DBG_CB, 0, 0, 0);	//reg. debug api
#endif
	ctl_sie_isp_init();
}

void kflow_ctl_sie_uninit(void)
{
	memset((void *)&ctl_sie_limit[0], 0, sizeof(ctl_sie_limit));
}

CTL_SIE_LIMIT *ctl_sie_limit_query(CTL_SIE_ID id)
{
	return &ctl_sie_limit[id];
}

UINT32 ctl_sie_buf_query(UINT32 num)
{
	UINT32 i;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();

	ctl_sie_set_dbg_proc_t(CTL_SIE_ID_MAX_NUM, "buf_query", CTL_SIE_PROC_TIME_ITEM_ENTER);
	if (num == 0) {
		ctl_sie_dbg_ind("query num is 0\r\n");
		return 0;
	}

	if (ctl_sie_init_flg) {
		ctl_sie_dbg_err("uninit sie first\r\n");
		return 0;
	}

	memset((void *)&ctl_sie_hdl_ctx, 0, sizeof(CTL_SIE_HDL_CONTEXT));
	for (i = 0; i < CTL_SIE_MAX_SUPPORT_ID; i++) {
		ctl_sie_hdl_ctx.context_idx[i] = CTL_SIE_MAX_SUPPORT_ID;
	}
	ctl_sie_hdl_ctx.context_num = num;
	ctl_sie_hdl_ctx.contex_size = ALIGN_CEIL(sizeof(CTL_SIE_HDL), VOS_ALIGN_BYTES) + ALIGN_CEIL(sizeof(CTL_SIE_INT_CTL_FLG), VOS_ALIGN_BYTES) +
			ctl_sie_buf_get_ca_out_size(CTL_SIE_ID_1) + ctl_sie_buf_get_la_out_size(CTL_SIE_ID_1);
	if (dbg_tab != NULL && dbg_tab->query_dbg_buf != NULL) {
		ctl_sie_hdl_ctx.dbg_buf_alloc = TRUE;
		ctl_sie_hdl_ctx.contex_size += dbg_tab->query_dbg_buf();
	} else {
		ctl_sie_hdl_ctx.dbg_buf_alloc = FALSE;
	}
	ctl_sie_hdl_ctx.req_size = num * (ctl_sie_hdl_ctx.contex_size);
	ctl_sie_hdl_ctx.total_buf_sz = kdf_sie_buf_query(num) + ctl_sie_hdl_ctx.req_size;
	ctl_sie_set_dbg_proc_t(CTL_SIE_ID_MAX_NUM, "buf_query", CTL_SIE_PROC_TIME_ITEM_EXIT);
	ctl_sie_dbg_ind("[SIE]query buf done, num %d, req_sz %x, total_sz %x\r\n", (int)num, (unsigned int)ctl_sie_hdl_ctx.req_size, (unsigned int)ctl_sie_hdl_ctx.total_buf_sz);
	return (ctl_sie_hdl_ctx.total_buf_sz);
}

INT32 ctl_sie_init(UINT32 buf_addr, UINT32 buf_size)
{
	ctl_sie_dbg_ind("[SIE]init begin\r\n");
	ctl_sie_set_dbg_proc_t(CTL_SIE_ID_MAX_NUM, "init", CTL_SIE_PROC_TIME_ITEM_ENTER);
	if (ctl_sie_init_flg) {
		ctl_sie_dbg_wrn("sie already init\r\n");
		return CTL_SIE_E_STATE;
	}

	if (buf_addr == 0) {
		if (ctl_sie_util_os_malloc(&ctl_sie_hdl_ctx.vos_mem_info, ctl_sie_hdl_ctx.total_buf_sz) == CTL_SIE_E_OK) {
			ctl_sie_dbg_ind("auto allocate ctl_sie private mem, addr %x, sz %x\r\n", (unsigned int)ctl_sie_hdl_ctx.vos_mem_info.cma_info.vaddr, (unsigned int)ctl_sie_hdl_ctx.total_buf_sz);
			ctl_sie_hdl_ctx.auto_alloc_mem = TRUE;
			buf_addr = ctl_sie_hdl_ctx.vos_mem_info.cma_info.vaddr;
			buf_size = ctl_sie_hdl_ctx.total_buf_sz;
		} else {
			ctl_sie_hdl_ctx.auto_alloc_mem = FALSE;
			return CTL_SIE_E_NOMEM;
		}
	} else {
		if ((buf_size == 0) || (buf_size < ctl_sie_hdl_ctx.total_buf_sz)) {
			ctl_sie_dbg_err("init buf err, addr %x, buf_size %x < total size %x\r\n", (unsigned int)buf_addr, (unsigned int)buf_size, (unsigned int)ctl_sie_hdl_ctx.total_buf_sz);
			return CTL_SIE_E_NOMEM;
		}
		ctl_sie_hdl_ctx.auto_alloc_mem = FALSE;
	}

	ctl_sie_hdl_ctx.start_addr = buf_addr;
	if (kdf_sie_init(ctl_sie_hdl_ctx.start_addr + ctl_sie_hdl_ctx.req_size, buf_size - ctl_sie_hdl_ctx.req_size) == CTL_SIE_E_OK) {
		ctl_sie_install_id();
		ctl_sie_isp_install_id();
		sie_event_install_id();
		sie_event_reset();
		ctl_sie_buf_open_tsk();
		ctl_sie_isp_open_tsk();
	} else {
		ctl_sie_dbg_err("sie init fail\r\n");
		return CTL_SIE_E_NOMEM;
	}

	ctl_sie_set_dbg_proc_t(CTL_SIE_ID_MAX_NUM, "init", CTL_SIE_PROC_TIME_ITEM_EXIT);
	ctl_sie_dbg_ind("[SIE]init done\r\n");
	ctl_sie_init_flg = TRUE;
	return CTL_SIE_E_OK;
}

INT32 ctl_sie_uninit(void)
{
	INT rt = CTL_SIE_E_OK;
	CTL_SIE_INT_DBG_TAB *dbg_tab = ctl_sie_get_dbg_tab();
	UINT32 i;

	ctl_sie_dbg_ind("[SIE]uninit begin\r\n");
	ctl_sie_set_dbg_proc_t(CTL_SIE_ID_MAX_NUM, "uninit", CTL_SIE_PROC_TIME_ITEM_ENTER);
	if (ctl_sie_init_flg) {
		for (i = 0; i < CTL_SIE_MAX_SUPPORT_ID; i++) {
			if (sie_ctl_info[i] != NULL && g_ctl_sie_status[i] != CTL_SIE_STS_CLOSE) {
				ctl_sie_dbg_err("sie uninit fail, id: %d status %d err\r\n", (int)i, (int)g_ctl_sie_status[i]);
				rt = CTL_SIE_E_STATE;
			}
		}

		if (kdf_sie_uninit() == CTL_SIE_E_OK) {
			ctl_sie_buf_close_tsk();
			ctl_sie_isp_close_tsk();
			sie_event_uninstall_id();
			ctl_sie_uninstall_id();
			ctl_sie_isp_uninstall_id();
			for (i = 0; i < CTL_SIE_MAX_SUPPORT_ID; i++) {
				sie_ctl_info[i] = NULL;
				sie_ctl_int_flg_ctl[i] = NULL;
				if (dbg_tab != NULL && dbg_tab->set_dbg_buf != NULL && ctl_sie_hdl_ctx.dbg_buf_alloc){
					dbg_tab->set_dbg_buf(i, 0);
				}

			}
			if (ctl_sie_hdl_ctx.auto_alloc_mem) {
				ctl_sie_util_os_mfree(&ctl_sie_hdl_ctx.vos_mem_info);
			}
			ctl_sie_init_flg = FALSE;
			rt = CTL_SIE_E_OK;
		} else {
			ctl_sie_dbg_err("sie uninit fail\r\n");
			rt = CTL_SIE_E_STATE;
		}
	} else {
		ctl_sie_dbg_wrn("sie already uninit\r\n");
		rt = CTL_SIE_E_STATE;
	}
	ctl_sie_set_dbg_proc_t(CTL_SIE_ID_MAX_NUM, "uninit", CTL_SIE_PROC_TIME_ITEM_EXIT);
	ctl_sie_dbg_ind("[SIE]uninit done\r\n");
	return rt;
}

INT32 ctl_sie_set(UINT32 hdl, CTL_SIE_ITEM item, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_ID id = CTL_SIE_ID_1;

	if (!ctl_sie_init_flg) {
		ctl_sie_dbg_err("sie still uninit\r\n");
		return CTL_SIE_E_STATE;
	}

	if (item == CTL_SIE_ITEM_MCLK_IMM) {
		if (ctl_sie_set_fp[item] == NULL) {
			ctl_sie_dbg_wrn("fp null, item %d\r\n", (int)(item));
			rt = CTL_SIE_E_NOSPT;
		} else {
			if (hdl != 0) {
				id = CTL_SIE_GET_ID(hdl);
			}
			rt = ctl_sie_set_fp[item](id, data);
		}
		return rt;
	}

	id = CTL_SIE_GET_ID(hdl);
	if (!ctl_sie_module_chk_id_valid(id)) {
		return CTL_SIE_E_ID;
	}

	if (item >= CTL_SIE_ITEM_MAX) {
		ctl_sie_dbg_err("err item %d > %d\r\n", (int)(item), (int)(CTL_SIE_ITEM_MAX));
		return CTL_SIE_E_NOSPT;
	}

	if ((item != CTL_SIE_ITEM_LOAD && item != CTL_SIE_ITEM_RESET_FC_IMM) && data == NULL) {
		ctl_sie_dbg_err("data NULL, id %d, item %d\r\n", (int)(id), (int)(item));
		return CTL_SIE_E_PAR;
	}

	// check state machine
	if (ctl_sie_module_state_machine(id, CTL_SIE_OP_SET) != CTL_SIE_E_OK) {
		return CTL_SIE_E_STATE;
	}

	ctl_sie_module_lock(id, CTL_SIE_FLG_LOCK, TRUE);
	ctl_sie_module_wait_get_end(id);

	// check set function and keep in ctrl layer
	if (ctl_sie_set_fp[item] == NULL) {
		ctl_sie_dbg_wrn("fp null, item %d\r\n", (int)(item));
		rt = CTL_SIE_E_NOSPT;
	} else {
		rt = ctl_sie_set_fp[item](id, data);

		// update load flag for set_load
		if (rt == CTL_SIE_E_OK && item != CTL_SIE_ITEM_TRIG_IMM) {
			ctl_sie_module_update_load_flg(id, item);
		}
	}
	ctl_sie_module_lock(id, CTL_SIE_FLG_LOCK, FALSE);
	ctl_sie_dbg_trc("[S]id:%d item:%d\r\n", (int)id, (int)item);
	return rt;
}

INT32 ctl_sie_get(UINT32 hdl, CTL_SIE_ITEM item, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_ID id = CTL_SIE_GET_ID(hdl);

	if (!ctl_sie_init_flg) {
		ctl_sie_dbg_err("sie still uninit\r\n");
		return CTL_SIE_E_STATE;
	}

	if (!ctl_sie_module_chk_id_valid(id)) {
		return CTL_SIE_E_ID;
	}

	if (item >= CTL_SIE_ITEM_MAX) {
		ctl_sie_dbg_err("err item %d > %d\r\n", (int)(item), (int)(CTL_SIE_ITEM_MAX));
		return CTL_SIE_E_PAR;
	}

	if (data == NULL) {
		ctl_sie_dbg_err("data NULL, id %d, item %d\r\n", (int)(id), (int)(item));
		return CTL_SIE_E_PAR;
	}

	if (ctl_sie_module_state_machine(id, CTL_SIE_OP_GET) != CTL_SIE_E_OK) {
		return CTL_SIE_E_STATE;
	}

	if (item != CTL_SIE_ITEM_CA_RSLT &&	item != CTL_SIE_ITEM_LA_RSLT) {
		ctl_sie_module_get_lock(id, TRUE);
	}
	// check set function
	if (ctl_sie_get_fp[item] == NULL) {
		ctl_sie_dbg_wrn("fp null, item %d\r\n", (int)(item));
		rt = CTL_SIE_E_NOSPT;
	} else {
		rt = ctl_sie_get_fp[item](id, data);
	}

	if (item != CTL_SIE_ITEM_CA_RSLT &&	item != CTL_SIE_ITEM_LA_RSLT) {
		ctl_sie_module_get_lock(id, FALSE);
	}
	ctl_sie_dbg_trc("[G]id:%d item:%d\r\n", (int)id, (int)item);
	return rt;
}

UINT32 ctl_sie_open(void *open_cfg)
{
	CTL_SIE_ID id;
	CTL_SIE_OPEN_CFG ctl_sie_open_cfg;
	CTL_SIE_ALG_FUNC alg_func;
	CHAR *cbevt_name_str[] = {
		"SIE1",
		"SIE2",
		"SIE3",
		"SIE4",
		"SIE5",
	};
	CTL_SIE_MCLK ctl_sie_mclk_info = {ENABLE, CTL_SIE_ID_MCLK};

#if defined(__KERNEL__)
	fastboot = kdrv_builtin_is_fastboot();
#endif

	if (!ctl_sie_init_flg) {
		ctl_sie_dbg_err("sie still uninit\r\n");
		return 0;
	}

	if (open_cfg == NULL) {
		ctl_sie_dbg_err("open_cfg NULL\r\n");
		return 0;
	}

	ctl_sie_open_cfg = *(CTL_SIE_OPEN_CFG *)open_cfg;
	id = ctl_sie_open_cfg.id;
	ctl_sie_dbg_ind("[Open]id:%d begin, hdl %x\r\n", (int)id, (int)(UINT32)sie_ctl_info[id]);
	ctl_sie_set_dbg_proc_t(id, "open", CTL_SIE_PROC_TIME_ITEM_ENTER);
	if (!ctl_sie_module_chk_id_valid(id)) {
		return 0;
	}

	// check state machine
	if (g_ctl_sie_status[id] == CTL_SIE_STS_READY || g_ctl_sie_status[id] == CTL_SIE_STS_RUN) {
		return (UINT32)sie_ctl_info[id];
	}

	if (ctl_sie_module_state_machine(id, CTL_SIE_OP_OPEN) != CTL_SIE_E_OK) {
		ctl_sie_dbg_err("id %d, kflow state err\r\n", (int)(id));
		return 0;
	}

	if (ctl_sie_module_hdl_init(id) != CTL_SIE_E_OK) {
		ctl_sie_module_state_machine(id, CTL_SIE_OP_CLOSE);
		return 0;
	}

	sie_ctl_info[id]->evt_hdl = sie_event_open(cbevt_name_str[id]);
	if (sie_ctl_info[id]->evt_hdl == 0) {
		ctl_sie_dbg_err("evt hdl open fail\r\n");
	}

	sie_ctl_info[id]->id = ctl_sie_open_cfg.id;
	sie_ctl_info[id]->flow_type = ctl_sie_open_cfg.flow_type;
	sie_ctl_info[id]->isp_id = ctl_sie_open_cfg.isp_id;
	sie_ctl_info[id]->sen_id = ctl_sie_open_cfg.sen_id;
	if (ctl_sie_open_cfg.flow_type == CTL_SIE_FLOW_SIG_DUPL) {
		sie_ctl_info[id]->dupl_src_id = ctl_sie_open_cfg.dupl_src_id;
	} else {
		sie_ctl_info[id]->dupl_src_id = ctl_sie_open_cfg.id;
	}

	switch (sie_ctl_info[id]->flow_type) {
	case CTL_SIE_FLOW_PATGEN:
		sie_ctl_info[id]->ctrl.clk_info.act_mode = CTL_SIE_IN_PATGEN;
		sie_ctl_info[id]->ctrl.clk_info.clk_src_sel = CTL_SIE_CLKSRC_PLL10;	//pattern gen force use clksrc_480
		sie_ctl_info[id]->ctrl.clk_info.data_rate = ctl_sie_limit[id].max_clk_rate;
		sie_ctl_info[id]->ctrl.clk_info.pclk_src_sel = CTL_SIE_PXCLKSRC_MCLK;
		break;

	case CTL_SIE_FLOW_SEN_IN:
	case CTL_SIE_FLOW_SIG_DUPL:
		sie_ctl_info[id]->p_sen_obj = ctl_sen_get_object(sie_ctl_info[id]->sen_id);
		if (sie_ctl_info[id]->p_sen_obj == NULL) {
			ctl_sie_module_lock(sie_ctl_info[id]->id, CTL_SIE_FLG_LOCK, FALSE);
			ctl_sie_dbg_err("id %d, get sen obj NULL\r\n", (int)(id));
			ctl_sie_module_state_machine(id, CTL_SIE_OP_CLOSE);
			ctl_sie_module_hdl_uninit(id);
			return 0;
		}
		//get from sen drv
		sie_ctl_info[id]->ctrl.clk_info.act_mode = ctl_sie_module_get_actmode(id, sie_ctl_info[id]->dupl_src_id);
		if (ctl_sie_open_cfg.clk_src_sel != CTL_SIE_CLKSRC_DEFAULT) {
			sie_ctl_info[id]->ctrl.clk_info.clk_src_sel = ctl_sie_open_cfg.clk_src_sel;
		} else {
			sie_ctl_info[id]->ctrl.clk_info.clk_src_sel = CTL_SIE_CLKSRC_PLL10;	//force set sie clk src to PLL10@600Mhz
		}
		sie_ctl_info[id]->ctrl.clk_info.data_rate = ctl_sie_limit[id].max_clk_rate;
		sie_ctl_info[id]->ctrl.clk_info.pclk_src_sel = CTL_SIE_PXCLKSRC_PAD;	// set real select when chg sensor mode
		break;

	default:
		ctl_sie_dbg_err("id %d, N.S. flow %d\r\n", (int)(id), (int)sie_ctl_info[id]->flow_type);
		ctl_sie_module_state_machine(id, CTL_SIE_OP_CLOSE);
		ctl_sie_module_hdl_uninit(id);
		return 0;
	}

	sie_ctl_info[id]->ctrl.int_isrcb_fp = ctl_sie_isr;	//isr_cb and enable default interrupte bit
	ctl_sie_update_inte(id, ENABLE, CTL_SIE_DFT_INT_EN);
	vos_flag_iclr(ctl_sie_get_flag_id(id), CTL_SIE_FLG_KDRV_END);
	ctl_sie_set_dbg_proc_t(id, "open(kdrv)", CTL_SIE_PROC_TIME_ITEM_ENTER);
	sie_ctl_info[id]->kdf_hdl = kdf_sie_open(id, (void *)sie_ctl_info[id]);
	ctl_sie_set_dbg_proc_t(id, "open(kdrv)", CTL_SIE_PROC_TIME_ITEM_EXIT);
	vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_KDRV_END);
	if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN) {
		ctl_sie_set_mclk(id, &ctl_sie_mclk_info);
	}
	if (sie_ctl_info[id]->kdf_hdl == 0) {
		ctl_sie_dbg_err("id %d, kdf open fail\r\n", (int)(id));
		ctl_sie_module_state_machine(id, CTL_SIE_OP_CLOSE);
		ctl_sie_module_hdl_uninit(id);
		return 0;
	}

	//default setting
	ctl_sie_module_par_set_dft(id);
	ctl_sie_module_lock(id, CTL_SIE_FLG_INIT, FALSE);
	ctl_sie_module_lock(id, CTL_SIE_FLG_OPEN, FALSE);

	//get SIE limitation
	kdf_sie_get(sie_ctl_info[id]->kdf_hdl, CTL_SIE_INT_ITEM_LIMIT, (void *)&ctl_sie_limit[id]);
	if (sie_ctl_info[id]->flow_type == CTL_SIE_FLOW_PATGEN && !(ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_PATGEN)) {
		ctl_sie_dbg_err("id %d, N.S. PATGEN\r\n", (int)(id));
		kdf_sie_close(sie_ctl_info[id]->kdf_hdl);
		ctl_sie_module_state_machine(id, CTL_SIE_OP_CLOSE);
		ctl_sie_module_hdl_uninit(id);
		return 0;
	}

	if (ctl_sie_limit[id].support_func & KDRV_SIE_FUNC_SPT_YOUT) {
		alg_func.func_en = ENABLE;
		alg_func.type = CTL_SIE_ALG_TYPE_YOUT;
		ctl_sie_set_alg_func(id, (void *)&alg_func);
	}

	sie_ctl_info[id]->tag = ctl_sie_get_hdl_tag();
	ctl_sie_set_dbg_proc_t(id, "open", CTL_SIE_PROC_TIME_ITEM_EXIT);
	ctl_sie_dbg_ind("[Open]id:%d done, hdl %x\r\n", (int)id, (int)(UINT32)sie_ctl_info[id]);
	return (UINT32)sie_ctl_info[id];
}

INT32 ctl_sie_close(UINT32 hdl)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_ID id = CTL_SIE_GET_ID(hdl);
	FLGPTN wait_flag = 0;
	UINT32 i;
	unsigned long flags;

	ctl_sie_dbg_ind("[Close]id:%d begin, hdl %x\r\n", (int)id, (int)hdl);
	ctl_sie_set_dbg_proc_t(id, "close", CTL_SIE_PROC_TIME_ITEM_ENTER);
	if (!ctl_sie_init_flg) {
		ctl_sie_dbg_err("sie stil uninit\r\n");
		return CTL_SIE_E_STATE;
	}

	if (!ctl_sie_module_chk_id_valid(id)) {
		return CTL_SIE_E_ID;
	}

	// check state machine
	if (ctl_sie_module_state_machine(id, CTL_SIE_OP_CLOSE) != CTL_SIE_E_OK) {
		return CTL_SIE_E_STATE;
	}

	//lock all flg
	ctl_sie_module_lock(id, CTL_SIE_FLG_INIT, TRUE);

	// close sie ctrl
	vos_flag_iset(ctl_sie_get_flag_id(id), CTL_SIE_FLG_CLOSE);
	if (vos_flag_wait_timeout(&wait_flag, ctl_sie_get_flag_id(id), (CTL_SIE_FLG_KDRV_END|CTL_SIE_FLG_UP_ITEM_END|CTL_SIE_FLG_ISP_PRC_END), TWF_ANDW, vos_util_msec_to_tick(1000)) != 0) {
		ctl_sie_dbg_err("[Close]wait kdrv end timeout id:%d, cuf_flg = %x\r\n", (int)id, wait_flag);
	}
	wait_flag = vos_flag_chk(ctl_sie_get_flag_id(id), (CTL_SIE_FLG_KDRV_END|CTL_SIE_FLG_UP_ITEM_END|CTL_SIE_FLG_ISP_PRC_END));

	ctl_sie_set_dbg_proc_t(id, "close(kdrv)", CTL_SIE_PROC_TIME_ITEM_ENTER);
	rt = kdf_sie_close(sie_ctl_info[id]->kdf_hdl);
	ctl_sie_set_dbg_proc_t(id, "close(kdrv)", CTL_SIE_PROC_TIME_ITEM_EXIT);
	vos_flag_iclr(ctl_sie_get_flag_id(id), CTL_SIE_FLG_CLOSE);
	sie_event_close(sie_ctl_info[id]->evt_hdl);
	ctl_sie_module_queue_flush(id);
	if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->set_frm_ctrl_info != NULL) {
		ctl_sie_dbg_tab->set_frm_ctrl_info(id, &sie_ctl_info[id]->ctrl.frame_ctl_info);
	}
	//set NULL to free pointer
	sie_ctl_info[id]->ctrl.int_isrcb_fp = NULL;
	sie_ctl_info[id]->ctrl.ext_isrcb_fp.cb_fp = NULL;
	sie_ctl_info[id]->ctrl.bufiocb.cb_fp = NULL;
	sie_ctl_info[id]->ctrl.direct_cb_fp.cb_fp = NULL;
	sie_ctl_info[id]->p_sen_obj = NULL;
	loc_cpu(flags);
	ctl_sie_hdl_ctx.context_used--;
	for (i = 0; i <= ctl_sie_limit[id].max_spt_id; i++) {
		if (ctl_sie_hdl_ctx.context_idx[i] == id) {
			ctl_sie_hdl_ctx.context_idx[i] = CTL_SIE_MAX_SUPPORT_ID;
			break;
		}
	}
	unl_cpu(flags);
	sie_ctl_info[id] = NULL;
	sie_ctl_int_flg_ctl[id] = NULL;

	if (sync_enable_bit & (1 << id)) {
		sync_enable_cnt -= 1;
		sync_enable_bit &= ~(1 << id);
	}

	ctl_sie_set_dbg_proc_t(id, "close", CTL_SIE_PROC_TIME_ITEM_EXIT);
	ctl_sie_dbg_ind("[Close]id:%d done, hdl %x, flg %x\r\n", (int)id, (int)hdl, wait_flag);
	return rt;
}

INT32 ctl_sie_suspend(UINT32 hdl, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	FLGPTN wait_flag = 0;
	CTL_SIE_ID id = CTL_SIE_GET_ID(hdl);
	CTL_SIE_SUS_RES_LVL sus_lvl = *(CTL_SIE_SUS_RES_LVL *)data;

	ctl_sie_dbg_ind("[Suspend]id:%d begin\r\n", (int)id);
	ctl_sie_set_dbg_proc_t(id, "suspend", CTL_SIE_PROC_TIME_ITEM_ENTER);
	if (!ctl_sie_module_chk_id_valid(id)) {
		return CTL_SIE_E_ID;
	}

	if (ctl_sie_get_state_machine(id) == CTL_SIE_STS_RUN) {
		rt = ctl_sie_module_direct_cb_trig(id, CTL_SIE_DIRECT_TRIG_STOP, FALSE);	//direct trigger
		if (rt != CTL_SIE_E_OK) {
			return rt;
		}
		// check state machine
		rt = ctl_sie_module_state_machine(id, CTL_SIE_OP_SUSPEND);
		if (rt != CTL_SIE_E_OK) {
			return rt;
		}
		rt = kdf_sie_suspend(sie_ctl_info[id]->kdf_hdl, (void *)&sus_lvl);
		sie_ctl_int_flg_ctl[id]->resume_rst_flg = TRUE;
		if (vos_flag_wait_timeout(&wait_flag, ctl_sie_get_flag_id(id), CTL_SIE_FLG_ISR_PRC_END, TWF_ORW, vos_util_msec_to_tick(1000)) != 0) {
			ctl_sie_dbg_wrn("id %d, suspend TO(wait isr proc end), cuf_flg = %x\r\n", (int)(id), wait_flag);
		}
		ctl_sie_buf_io_cfg(id, CTL_SIE_BUF_IO_UNLOCK, 0, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR]);
		ctl_sie_buf_io_cfg(id, CTL_SIE_BUF_IO_UNLOCK, 0, (UINT32)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_NEXT]);
		ctl_sie_module_queue_flush(id);
	} else if (ctl_sie_get_state_machine(id) == CTL_SIE_STS_READY) {
		// check state machine
		rt = ctl_sie_module_state_machine(id, CTL_SIE_OP_SUSPEND);
		if (rt != CTL_SIE_E_OK) {
			return rt;
		}
		rt = kdf_sie_suspend(sie_ctl_info[id]->kdf_hdl, (void *)&sus_lvl);
		sie_ctl_int_flg_ctl[id]->resume_rst_flg = FALSE;
	} else {
		rt = CTL_SIE_E_STATE;
	}
	ctl_sie_set_dbg_proc_t(id, "suspend", CTL_SIE_PROC_TIME_ITEM_EXIT);
	ctl_sie_dbg_ind("[Suspend]id:%d done\r\n", (int)id);
	return rt;
}

INT32 ctl_sie_resume(UINT32 hdl, void *data)
{
	INT32 rt = CTL_SIE_E_OK;
	CTL_SIE_ID id = CTL_SIE_GET_ID(hdl);
	CTL_SIE_SUS_RES_LVL res_lvl = *(CTL_SIE_SUS_RES_LVL *)data;

	ctl_sie_dbg_ind("[Resume]id:%d begin\r\n", (int)id);
	ctl_sie_set_dbg_proc_t(id, "resume", CTL_SIE_PROC_TIME_ITEM_ENTER);
	if (!ctl_sie_module_chk_id_valid(id)) {
		return CTL_SIE_E_ID;
	}

	if (ctl_sie_get_state_machine(id) == CTL_SIE_STS_SUSPEND) {
		if (sie_ctl_int_flg_ctl[id]->resume_rst_flg) {
			//reset frame control and header info
			memset((void *)&sie_ctl_info[id]->ctrl.frame_ctl_info, 0, sizeof(CTL_SIE_FRM_CTL_INFO));
			memset((void *)&sie_ctl_info[id]->ctrl.head_info[CTL_SIE_HEAD_IDX_CUR], 0, sizeof(CTL_SIE_HEADER_INFO) * CTL_SIE_HEAD_IDX_MAX);
			if (ctl_sie_dbg_tab != NULL && ctl_sie_dbg_tab->reset_ts != NULL) {
				ctl_sie_dbg_tab->reset_ts(id);
			}
			//load first frame parameters and prepare first frame buffer/header
			ctl_sie_set_load(id, NULL);
			ctl_sie_buf_update_out_size(sie_ctl_info[id]);
			rt = ctl_sie_header_create(sie_ctl_info[id], CTL_SIE_HEAD_IDX_NEXT);
			if (rt == CTL_SIE_E_NULL_FP) {
				return rt;
			}

			rt = ctl_sie_set_load(id, NULL);	//load first frame address
			if (rt == CTL_SIE_E_SEN) {
				return rt;
			}

			rt = ctl_sie_module_direct_cb_trig(id, CTL_SIE_DIRECT_TRIG_START, FALSE);
			if (rt == CTL_SIE_E_NULL_FP) {
				return rt;
			}
			// set state machine to start
			if (ctl_sie_module_state_machine(id, CTL_SIE_OP_RESUME) != CTL_SIE_E_OK) {
				return CTL_SIE_E_STATE;
			}
			ctl_sie_module_isp_cb_trig(id, CTL_SIE_INTE_ALL);	//reset isp parameters
			rt = kdf_sie_resume(sie_ctl_info[id]->kdf_hdl, (void *)&res_lvl);
		} else {
			if (ctl_sie_module_state_machine(id, CTL_SIE_OP_RESUME) != CTL_SIE_E_OK) {
				return CTL_SIE_E_STATE;
			}
			rt = kdf_sie_resume(sie_ctl_info[id]->kdf_hdl, (void *)&res_lvl);
		}
	} else {
		rt = CTL_SIE_E_STATE;
	}
	ctl_sie_set_dbg_proc_t(id, "resume", CTL_SIE_PROC_TIME_ITEM_EXIT);
	ctl_sie_dbg_ind("[Resume]id:%d done\r\n", (int)id);
	return rt;
}

