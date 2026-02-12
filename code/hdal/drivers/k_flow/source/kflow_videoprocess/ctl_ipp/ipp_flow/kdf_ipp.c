#include "kflow_common/isp_if.h"
#include "kdf_ipp_int.h"
#include "kdf_ipp_id_int.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

#define KDF_IPP_INIT_KDRV_FLOW_ENABLE (ENABLE)

#if 0
#endif

/*
	global variables
*/
static BOOL kdf_ipp_task_opened = FALSE;
static UINT32 kdf_ipp_task_pause_count;
static UINT32 kdf_ipp_kdrv_open_count;
static KDF_IPP_HANDLE_POOL kdf_ipp_handle_pool;

static CHAR kdf_ipp_eng_hang_dump_file_path[32] = "//mnt//sd//";
static BOOL kdf_ipp_eng_hang_is_dump = FALSE;
static KDF_IPP_ENG_HANG_HANDLE_MODE kdf_ipp_eng_hand_mode = KDF_IPP_ENG_HANG_HWRST;


#if 0
#endif

/*
	callback functions
*/
static INT32 kdf_ipp_eng_lock(UINT32 eng_bit)
{
	FLGPTN flg, eng_flg;
	INT32 rt;
	ER wai_rt;

	if (eng_bit == 0) {
		return CTL_IPP_E_OK;
	}

	rt = CTL_IPP_E_OK;

	eng_flg = eng_bit << KDF_IPP_ENG_IDLE_OFFSET;

	wai_rt = vos_flag_wait_timeout(&flg, g_kdf_ipp_flg_id, eng_flg, (TWF_CLR), vos_util_msec_to_tick(KDF_IPP_HANDLE_LOCK_TIMEOUT_MS));
	if (wai_rt != E_OK) {
		if (wai_rt == E_TMOUT) {
			rt = CTL_IPP_E_TIMEOUT;
			CTL_IPP_DBG_WRN("lock engine timeout(%d ms), check if engine hang\r\n", (int)(KDF_IPP_HANDLE_LOCK_TIMEOUT_MS));
		}
	}

	return rt;
}

static void kdf_ipp_eng_unlock(UINT32 eng_bit)
{
	FLGPTN eng_flg;

	if (eng_bit == 0) {
		return ;
	}

	eng_flg = eng_bit << KDF_IPP_ENG_IDLE_OFFSET;
	vos_flag_iset(g_kdf_ipp_flg_id, eng_flg);
}

static UINT32 kdf_ipp_get_eng_lock_mask(KDF_IPP_FLOW_TYPE flow)
{
	UINT32 lock_eng_mask[KDF_IPP_FLOW_MAX] = {
		0,
		KDF_IPP_RAW_ENG_MASK,		/* FLOW_RAW */
		KDF_IPP_RAW_ENG_MASK,		/* FLOW_DIRECT_RAW */
		KDF_IPP_CCIR_ENG_MASK,		/* FLOW_CCIR */
		KDF_IPP_ENG_BIT_IME,		/* FLOW_IME_D2D */
		KDF_IPP_ENG_BIT_IPE,		/* FLOW_IPE_D2D */
		KDF_IPP_ENG_BIT_DCE,		/* FLOW_DCE_D2D */
	};

	if (flow >= KDF_IPP_FLOW_MAX) {
		return 0;
	}

	return lock_eng_mask[flow];
}

static void kdf_ipp_eng_hang(void)
{
	/* debug information */
	if (kdf_ipp_eng_hand_mode & (KDF_IPP_ENG_HANG_DUMP | KDF_IPP_ENG_HANG_SAVE_INPUT)) {
		KDF_IPP_HANDLE *kdf_hdl;
		CTL_IPP_BASEINFO *p_base;
		UINT32 i, file_size;
		CHAR f_name[64];

		if (kdf_ipp_eng_hang_is_dump) {
			return ;
		}
		kdf_ipp_eng_hang_is_dump = TRUE;

		if (kdf_ipp_eng_hand_mode & KDF_IPP_ENG_HANG_SAVE_INPUT) {
			/* save input file */
			/* raw2yuv */
			kdf_ipp_eng_hand_mode &= ~(KDF_IPP_ENG_HANG_SAVE_INPUT);
			for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
				kdf_hdl = kdf_ipp_get_trig_hdl(i);
				if (kdf_hdl != NULL) {
					break;
				}
			}
			if (kdf_hdl != NULL && kdf_hdl->data_addr_in_process[0] != 0) {
				p_base = (CTL_IPP_BASEINFO *)kdf_hdl->data_addr_in_process[0];

				if (kdf_hdl->flow == KDF_IPP_FLOW_RAW) {
					for (i = 0; i < 2; i++) {
						if (p_base->ife_ctrl.in_addr[i] != 0) {
							file_size = p_base->ife_ctrl.in_lofs[i] * p_base->ife_ctrl.in_size.h;
							snprintf(f_name, 64, "%sipphang_main_in[%d]_%dx%d.RAW",
								kdf_ipp_eng_hang_dump_file_path, (int)i, (int)p_base->ife_ctrl.in_size.w, (int)p_base->ife_ctrl.in_size.h);
							ctl_ipp_dbg_savefile(f_name, p_base->ife_ctrl.in_addr[i], file_size);
						}
					}
				}
			}
		}

		if (kdf_ipp_eng_hand_mode & KDF_IPP_ENG_HANG_DUMP) {
			kdf_ipp_eng_hand_mode &= ~(KDF_IPP_ENG_HANG_DUMP);

			ctl_ipp_dbg_hdl_dump_all(ctl_ipp_int_printf);
			DBG_DUMP("[IFE]\r\n");
			debug_dumpmem(0xf0c70000, 0x200);
			DBG_DUMP("[DCE]\r\n");
			debug_dumpmem(0xf0c20000, 0x300);
			DBG_DUMP("[IPE]\r\n");
			debug_dumpmem(0xf0c30000, 0x800);
			DBG_DUMP("[IME]\r\n");
			debug_dumpmem(0xf0c40000, 0x1000);
			DBG_DUMP("[IFE2]\r\n");
			debug_dumpmem(0xf0d00000, 0x100);
		}
	}

	/* reset engine */
	if (kdf_ipp_eng_hand_mode & KDF_IPP_ENG_HANG_HWRST) {
		UINT32 kdrv_reset_param_id[KDF_IPP_ENG_MAX] = {
			0,
			KDRV_IFE_PARAM_IPL_RESET,
			KDRV_DCE_PARAM_IPL_RESET,
			KDRV_IPE_PARAM_IPL_RESET,
			KDRV_IME_PARAM_IPL_RESET,
			KDRV_IFE2_PARAM_IPL_RESET
		};
		KDF_IPP_ENG kdrv_reset_order[KDF_IPP_ENG_MAX] = {
			KDF_IPP_ENG_DCE, KDF_IPP_ENG_IME, KDF_IPP_ENG_IPE, KDF_IPP_ENG_IFE, KDF_IPP_ENG_IFE2, KDF_IPP_ENG_MAX
		};
		KDF_IPP_HANDLE *p_hdl;
		KDF_IPP_ENG eng;

		/* scan all engine to check which handle is in charge */
		for (eng = KDF_IPP_ENG_IFE; eng < KDF_IPP_ENG_MAX; eng++) {
			p_hdl = kdf_ipp_get_trig_hdl(eng);
			if (p_hdl != NULL) {
				/* reset all engine used by this handle */
				UINT32 eng_in_used;
				UINT32 proc_data_addr;
				UINT32 i;

				for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
					eng_in_used = kdrv_reset_order[i];
					if (p_hdl->engine_used_mask & (1 << eng_in_used)) {
						CTL_IPP_DBG_WRN("reset eng %d, param id 0x%.8x\r\n", eng_in_used, kdrv_reset_param_id[eng_in_used]);
						kdf_ipp_set_trig_hdl(NULL, eng_in_used);
						kdf_ipp_kdrv_set_wrapper(eng_in_used, p_hdl, kdrv_reset_param_id[eng_in_used], NULL);
					}
				}
				CTL_IPP_DBG_WRN("hdl %s, reset kdf engine, fc %d, eng_mask 0x%.8x\r\n",
					p_hdl->name, p_hdl->proc_end_cnt, p_hdl->engine_used_mask);

				p_hdl->reset_cnt += 1;
				p_hdl->proc_end_bit = 0;
				proc_data_addr = p_hdl->data_addr_in_process[0];
				p_hdl->data_addr_in_process[0] = 0;

				kdf_ipp_drop((UINT32)p_hdl, proc_data_addr, 0, p_hdl->cb_fp[KDF_IPP_CBEVT_DROP], CTL_IPP_E_TIMEOUT);
				kdf_ipp_eng_unlock(kdf_ipp_get_eng_lock_mask(p_hdl->flow));
				kdf_ipp_handle_unlock(p_hdl->lock);
			}
		}
	}
}

void kdf_ipp_eng_hang_config(KDF_IPP_ENG_HANG_HANDLE_MODE mode, CHAR *f_path)
{
	memset((void *)kdf_ipp_eng_hang_dump_file_path, '\0', sizeof(CHAR) * 32);
	strncpy(kdf_ipp_eng_hang_dump_file_path, f_path, (32 - 1));

	kdf_ipp_eng_hand_mode = mode;
	kdf_ipp_eng_hang_is_dump = FALSE;
}

void kdf_ipp_eng_hang_manual(KDF_IPP_ENG_HANG_HANDLE_MODE mode, CHAR *f_path)
{
	static KDF_IPP_HWRST_CFG cfg;
	INT32 rt;

	cfg.mode = mode;
	memset((void *)cfg.file_path, '\0', sizeof(CHAR) * 32);
	strncpy(cfg.file_path, f_path, (32 - 1));
	rt = kdf_ipp_msg_snd(KDF_IPP_MSG_HWRST, 0, (UINT32)&cfg, 0, NULL, 0);
	if (rt != E_OK) {
		CTL_IPP_DBG_ERR("snd hwrst event failed %d\r\n", rt);
	}
}

INT32 kdf_ipp_dir_wait_pause_end(void)
{
	FLGPTN flg;
	INT32 rt;
	ER wai_rt;

	if (!kdf_ipp_task_opened) {
		CTL_IPP_DBG_WRN("kdf task not open\r\n");
		return CTL_IPP_E_STATE;
	}

	rt = CTL_IPP_E_OK;
	vos_flag_clr(g_kdf_ipp_flg_id, KDF_IPP_DIR_PAUSE_END);

	wai_rt = vos_flag_wait_timeout(&flg, g_kdf_ipp_flg_id, KDF_IPP_DIR_PAUSE_END, TWF_CLR, vos_util_msec_to_tick(KDF_IPP_HANDLE_LOCK_TIMEOUT_MS*2)); // direct stop need up to 2 frame to stop. wait 2x time
	if (wai_rt != E_OK) {
		if (wai_rt == E_TMOUT) {
			/* hw reset to force pause when direct mode */
			KDF_IPP_HANDLE *kdf_hdl = kdf_ipp_get_trig_hdl(KDF_IPP_ENG_IFE);

			if (kdf_hdl != NULL) {
				if (kdf_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
					UINT32 kdrv_reset_param_id[KDF_IPP_ENG_MAX] = {
						0,
						KDRV_IFE_PARAM_IPL_RESET,
						KDRV_DCE_PARAM_IPL_RESET,
						KDRV_IPE_PARAM_IPL_RESET,
						KDRV_IME_PARAM_IPL_RESET,
						KDRV_IFE2_PARAM_IPL_RESET
					};

					kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, kdf_hdl, kdrv_reset_param_id[KDF_IPP_ENG_DCE], NULL);
					kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, kdf_hdl, kdrv_reset_param_id[KDF_IPP_ENG_IME], NULL);
					kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, kdf_hdl, kdrv_reset_param_id[KDF_IPP_ENG_IPE], NULL);
					kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, kdf_hdl, kdrv_reset_param_id[KDF_IPP_ENG_IFE], NULL);
					kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, kdf_hdl, kdrv_reset_param_id[KDF_IPP_ENG_IFE2], NULL);

					kdf_ipp_direct_load_bit_clr(&kdf_hdl->dir_load_bit, KDF_IPP_DIR_LOAD_PAUSE);
					kdf_ipp_direct_load_bit_set(&kdf_hdl->dir_load_bit, KDF_IPP_DIR_LOAD_PAUSE_E);
					kdf_ipp_eng_unlock(kdf_ipp_get_eng_lock_mask(kdf_hdl->flow));
					kdf_ipp_direct_clr_hdl();

					CTL_IPP_DBG_ERR("dir pause timeout(%d ms), reset to force pause\r\n", (int)(KDF_IPP_HANDLE_LOCK_TIMEOUT_MS));
				}
			}
		}
	}

	return rt;
}

static void kdf_ipp_dir_set_pause(void)
{
	if (!kdf_ipp_task_opened) {
		CTL_IPP_DBG_WRN("kdf task not open\r\n");
		return;
	}
	vos_flag_iset(g_kdf_ipp_flg_id, KDF_IPP_DIR_PAUSE_END);
}

INT32 kdf_ipp_dir_eng_lock(KDF_IPP_HANDLE *p_hdl)
{
	INT32 rt;

	/* lock engine */
	rt = kdf_ipp_eng_lock(kdf_ipp_get_eng_lock_mask(p_hdl->flow));
	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_ERR_FREQ(CTL_IPP_DBG_FREQ_NORMAL, "engine lock fail (%d)\r\n", rt);
		return rt;
	}

	return CTL_IPP_E_OK;
}

static void kdf_ipp_update_ktab_to_baseinfo(KDF_IPP_HANDLE *p_hdl)
{
	#if 0
	CTL_IPP_BASEINFO *p_base;

	p_base = (CTL_IPP_BASEINFO *)p_hdl->data_addr_in_process[0];

	/* update ktab if yuv compress */
	if (VDO_PXLFMT_CLASS(p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1].fmt) == VDO_PXLFMT_CLASS_NVX) {
		KDRV_IME_ENCODE_INFO enc_info = {0};

		if (kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, p_hdl, (KDRV_IME_PARAM_IPL_ENCODE_INFO | KDRV_IME_IGN_CHK), (void *)&enc_info) == E_OK) {
			p_base->ime_ctrl.enc_out_k_tbl[0] = enc_info.k_tbl0;
			p_base->ime_ctrl.enc_out_k_tbl[1] = enc_info.k_tbl1;
			p_base->ime_ctrl.enc_out_k_tbl[2] = enc_info.k_tbl2;
		}
	}
	#endif
}

static void kdf_ipp_update_lca_grayavg_to_baseinfo(KDF_IPP_HANDLE *p_hdl)
{
	KDRV_IFE2_GRAY_AVG gray_avg = {0};
	CTL_IPP_BASEINFO *p_base;

	p_base = (CTL_IPP_BASEINFO *)p_hdl->data_addr_in_process[0];

	if (p_base == NULL) {
		return;
	}

	if (kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IFE2, p_hdl, (KDRV_IFE2_PARAM_IPL_GRAY_AVG | KDRV_IFE2_IGN_CHK), (void *)&gray_avg) == E_OK) {
		p_base->ife2_ctrl.gray_avg_u = gray_avg.u_avg;
		p_base->ife2_ctrl.gray_avg_v = gray_avg.v_avg;
	}
}

static void kdf_ipp_kdrv_pause_eng(KDF_IPP_HANDLE *p_hdl, UINT32 eng_bit)
{
	UINT32 i;

	for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
		if (eng_bit & (1 << i)) {
			kdf_ipp_kdrv_pause_wrapper(i, p_hdl);
		}
	}
}

static void kdf_ipp_set_proc_sts(UINT32 *sts, UINT32 proc_item, UINT32 flag)
{
	unsigned long loc_cpu_flg;

	loc_cpu(loc_cpu_flg);
	if (flag == TRUE) {
		*sts |= proc_item;
	} else {
		*sts &= ~proc_item;
	}
	unl_cpu(loc_cpu_flg);
}

static void kdf_ipp_set_time_info_cb(KDF_IPP_HANDLE *p_hdl)
{
	CTL_IPP_EVT_TIME_INFO time_info;

	time_info.snd = p_hdl->snd_evt_time;
	time_info.rev = p_hdl->rev_evt_time;
	time_info.start = p_hdl->proc_st_time;
	time_info.trig = p_hdl->trig_ed_time;
	time_info.ime_bp = (p_hdl->proc_end_bit & KDF_IPP_ENG_BIT_IME)? p_hdl->ime_bp_time : p_hdl->trig_ed_time;
	time_info.end = ctl_ipp_util_get_syst_counter();
	time_info.proc_data_addr = p_hdl->data_addr_in_process[0];
	time_info.err_msg = CTL_IPP_E_OK;
	kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PROC_TIME, (UINT32)p_hdl, (void *)&time_info, NULL);
}

static void kdf_ipp_chk_proc_end(KDF_IPP_HANDLE *p_hdl, KDF_IPP_ENG eng)
{
	KDF_IPP_ISP_CB_INFO iqcb_info;
	UINT32 proc_end_data_addr;

	if ((1 << eng) & p_hdl->proc_end_mask) {
		if (p_hdl->proc_end_bit & (1 << eng)) {
			CTL_IPP_DBG_WRN("ISR ERROR 0x%.8x, Handle 0x%.8x, ENG %d\r\n", (unsigned int)p_hdl->proc_end_bit, (unsigned int)p_hdl, (int)eng);
		} else {
			p_hdl->proc_end_bit |= (1 << eng);
		}
	}

	if ((p_hdl->proc_end_bit & p_hdl->proc_end_mask) == p_hdl->proc_end_mask) {
		/* Direct mode will reference sts when reset happened*/
		kdf_ipp_set_proc_sts(&p_hdl->proc_sts, KDF_IPP_FRAME_END, TRUE);
		/* TBD*/
		/* Disable ime path*/

		p_hdl->proc_end_cnt += 1;
		p_hdl->proc_end_bit = 0;

		kdf_ipp_set_time_info_cb(p_hdl);
		if (p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
			proc_end_data_addr = kdf_ipp_direct_datain_drop(p_hdl, KDF_IPP_FRAME_END, 0);
			ctl_ipp_dbg_set_direct_isr_sequence((UINT32) p_hdl, "FD", proc_end_data_addr);
			if(proc_end_data_addr == 0) {
				kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PROCEND, (UINT32)p_hdl, NULL, NULL);
				return;
			}

			#if defined (__KERNEL__)
			/* set correct proc_end/start mask */
			if (kdrv_builtin_is_fastboot() && (p_hdl->proc_end_mask == KDF_IPP_ENG_BIT_IME)) {
				p_hdl->proc_end_mask = KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
				p_hdl->frame_sta_mask = KDF_IPP_ENG_BIT_IFE | KDF_IPP_ENG_BIT_DCE | KDF_IPP_ENG_BIT_IPE | KDF_IPP_ENG_BIT_IME;
				/* do not wait ife2 frame end, ife2 do not provide frame start signal*/
				/* Problem will be caused when frame start and frame end ISR accumulate */
			}

			/* trigger builtin ipp push last buffer for direct mode */
			if (kdrv_builtin_is_fastboot() && p_hdl->is_first_handle && p_hdl->proc_end_cnt == 1 && p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
				kdrv_ipp_builtin_exit();
			}
			#endif
		} else {
			proc_end_data_addr = p_hdl->data_addr_in_process[0];
			p_hdl->data_addr_in_process[0] = 0;
		}
		kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PROCEND, (UINT32)p_hdl, (void *)&proc_end_data_addr, NULL);

		iqcb_info.type = KDF_IPP_ISP_CB_TYPE_IQ_TRIG;
		iqcb_info.base_info_addr = proc_end_data_addr;
		iqcb_info.item = ISP_EVENT_IPP_PROCEND;
		kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_ISP, (UINT32)p_hdl, (void *)&iqcb_info, NULL);

		/* direct flow can not lock/unlock except pause*/
		if (p_hdl->flow != KDF_IPP_FLOW_DIRECT_RAW) {
			kdf_ipp_kdrv_pause_eng(p_hdl, p_hdl->engine_used_mask);
			kdf_ipp_eng_unlock(kdf_ipp_get_eng_lock_mask(p_hdl->flow));
			kdf_ipp_handle_unlock(p_hdl->lock);
		}

		/* direct mode pasue stage2 */
		if (p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW && (p_hdl->dir_load_bit & KDF_IPP_DIR_LOAD_PAUSE_E)) {
			kdf_ipp_eng_unlock(kdf_ipp_get_eng_lock_mask(p_hdl->flow));
			kdf_ipp_direct_clr_hdl();
			kdf_ipp_dir_set_pause();
		}

		/* direct mode pasue stage1 */
		if (p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW && (p_hdl->dir_load_bit & KDF_IPP_DIR_LOAD_PAUSE)) {
			kdf_ipp_kdrv_pause_eng(p_hdl, p_hdl->engine_used_mask);
			kdf_ipp_direct_load_bit_clr(&p_hdl->dir_load_bit, KDF_IPP_DIR_LOAD_PAUSE);
			kdf_ipp_direct_load_bit_set(&p_hdl->dir_load_bit, KDF_IPP_DIR_LOAD_PAUSE_E);
		}
	}
}

static void kdf_ipp_chk_frame_start(KDF_IPP_HANDLE *p_hdl, KDF_IPP_ENG eng)
{
	UINT32 frm_str_data_addr;
	UINT32 mask;
	unsigned long loc_cpu_flg;
	KDRV_IME_SINGLE_OUT_INFO ime_single_out_info = {0};


	if ((1 << eng) & p_hdl->frame_sta_mask) {
		if (p_hdl->frame_sta_bit & (1 << eng)) {
			CTL_IPP_DBG_WRN("ISR ERROR 0x%.8x, Handle 0x%.8x, ENG %d\r\n", (unsigned int)p_hdl->frame_sta_bit, (unsigned int)p_hdl, (int)eng);
		} else {
			p_hdl->frame_sta_bit |= (1 << eng);

		}
	}

	if ((p_hdl->frame_sta_bit & p_hdl->frame_sta_mask) == p_hdl->frame_sta_mask) {
		p_hdl->frame_sta_bit = 0;
		mask = KDF_IPP_FRAME_START|KDF_IPP_FRAME_BP|KDF_IPP_FRAME_END;
		if (p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
			p_hdl->proc_st_time = ctl_ipp_util_get_syst_counter();
			p_hdl->trig_ed_time = p_hdl->proc_st_time;
			kdf_ipp_direct_load_bit_set(&p_hdl->dir_load_bit, KDF_IPP_DIR_LOAD_FMS);
			kdf_ipp_direct_load(p_hdl);
			/* reset flow , no previous frame end */
			if (p_hdl->proc_sts == (KDF_IPP_FRAME_START | KDF_IPP_FRAME_BP) && p_hdl->frame_sta_cnt != 0) {
				p_hdl->reset_cnt += 1;
			} else if (p_hdl->proc_sts == KDF_IPP_FRAME_START) {
				p_hdl->reset_cnt += 1;
			}
			vk_spin_lock_irqsave(&p_hdl->spinlock, loc_cpu_flg);
			frm_str_data_addr = p_hdl->data_addr_in_process[0];
			p_hdl->frame_sta_cnt += 1;
			vk_spin_unlock_irqrestore(&p_hdl->spinlock, loc_cpu_flg);
			kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_FRAMESTART, (UINT32)p_hdl, (void *) &frm_str_data_addr, NULL);

			/* aovid ISR shift and accumulate, fra_sta_with_sop_cnt will be plus after singleout load*/
			if (kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_SINGLE_OUTPUT_HW, (void *)&ime_single_out_info) != E_OK) {
				CTL_IPP_DBG_WRN("start get single out fail\r\n");
			} else {
				if (ime_single_out_info.sout_chl == 0) {
					vk_spin_lock_irqsave(&p_hdl->spinlock, loc_cpu_flg);
					p_hdl->fra_sta_with_sop_cnt += 1;
					vk_spin_unlock_irqrestore(&p_hdl->spinlock, loc_cpu_flg);
				}
			}
			kdf_ipp_set_proc_sts(&p_hdl->proc_sts, mask, FALSE);
			kdf_ipp_set_proc_sts(&p_hdl->proc_sts, KDF_IPP_FRAME_START, TRUE);
			ctl_ipp_dbg_set_direct_isr_sequence((UINT32) p_hdl, "FS", 0);
		} else {
			frm_str_data_addr = p_hdl->data_addr_in_process[0];
			kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_FRAMESTART, (UINT32)p_hdl, (void *) &frm_str_data_addr, NULL);
		}
	}
}

/*
	ISR Event handler, should process one event at a time
*/
_INLINE void kdf_ipp_ife_isr_handler(KDF_IPP_HANDLE *kdf_hdl, UINT32 msg)
{
	if (msg & CTL_IPP_IFE_INTE_FMD) {

	}

	/* callback */
	kdf_ipp_cb_proc(kdf_hdl, KDF_IPP_CBEVT_ENG_IFE_ISR, msg, (UINT32 *)&kdf_hdl, NULL);
	if (msg & CTL_IPP_IFE_INTE_FMD) {
		if (kdf_hdl->flow != KDF_IPP_FLOW_DIRECT_RAW) {
			kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_IFE);
		}
		kdf_ipp_chk_proc_end(kdf_hdl, KDF_IPP_ENG_IFE);
	}

	if (msg & CTL_IPP_IFE_INTE_SIE_FRAME_START) {
		kdf_ipp_chk_frame_start(kdf_hdl, KDF_IPP_ENG_IFE);
	}
}

_INLINE void kdf_ipp_dce_isr_handler(KDF_IPP_HANDLE *kdf_hdl, UINT32 msg)
{
	if (msg & CTL_IPP_DCE_INTE_FMD) {

	}

	/* callback */
	kdf_ipp_cb_proc(kdf_hdl, KDF_IPP_CBEVT_ENG_DCE_ISR, msg, (UINT32 *)&kdf_hdl, NULL);
	if (msg & CTL_IPP_DCE_INTE_FMD) {
		if (kdf_hdl->flow != KDF_IPP_FLOW_DIRECT_RAW) {
			kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_DCE);
		}
		kdf_ipp_chk_proc_end(kdf_hdl, KDF_IPP_ENG_DCE);
	}

	if (msg & CTL_IPP_DCE_INTE_FST) {
		kdf_ipp_chk_frame_start(kdf_hdl, KDF_IPP_ENG_DCE);
	}
}

_INLINE void kdf_ipp_ipe_isr_handler(KDF_IPP_HANDLE *kdf_hdl, UINT32 msg)
{
	if (msg & CTL_IPP_IPE_INTE_FMD) {

	}

	/* callback */
	kdf_ipp_cb_proc(kdf_hdl, KDF_IPP_CBEVT_ENG_IPE_ISR, msg, (UINT32 *)&kdf_hdl, NULL);
	if (msg & CTL_IPP_IPE_INTE_FMD) {
		if (kdf_hdl->flow != KDF_IPP_FLOW_DIRECT_RAW) {
			kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_IPE);
		}
		kdf_ipp_chk_proc_end(kdf_hdl, KDF_IPP_ENG_IPE);
	}

	if (msg & CTL_IPP_IPE_INTE_FMS) {
		kdf_ipp_chk_frame_start(kdf_hdl, KDF_IPP_ENG_IPE);
	}
}

_INLINE void kdf_ipp_ime_isr_handler(KDF_IPP_HANDLE *kdf_hdl, UINT32 msg)
{
	CTL_IPP_BASEINFO *p_base;
	UINT8 need_flush = TRUE;

	if (msg & CTL_IPP_IME_INTE_FMD) {
		kdf_ipp_update_ktab_to_baseinfo(kdf_hdl);

		/* copy md info */
		if (kdf_hdl->data_addr_in_process[0] != 0) {
			p_base = (CTL_IPP_BASEINFO *)kdf_hdl->data_addr_in_process[0];

			if (p_base->ime_ctrl.tplnr_enable &&
				p_base->ime_ctrl.tplnr_out_ms_roi_enable &&
				p_base->ime_ctrl.tplnr_in_ref_addr[0]) {
				UINT32 i;
				UINT32 md_size;

				md_size = ctl_ipp_util_3dnr_ms_roi_size(p_base->ime_ctrl.in_size.w, p_base->ime_ctrl.in_size.h);
				for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
					CTL_IPP_IME_OUT_IMG *p_path = &p_base->ime_ctrl.out_img[i];

					if (p_path->md_enable && (p_path->md_addr != 0) && (p_base->ime_ctrl.tplnr_out_ms_roi_addr)) {
						if (need_flush) {
							vos_cpu_dcache_sync(p_base->ime_ctrl.tplnr_out_ms_roi_addr , md_size, VOS_DMA_FROM_DEVICE); // sync device data from dma first
							need_flush = FALSE; // md is from 3dnr ms roi buffer, only need flush one time for all path
						}
						memcpy((void *)p_path->md_addr, (void *)p_base->ime_ctrl.tplnr_out_ms_roi_addr, md_size);
						vos_cpu_dcache_sync(p_path->md_addr , md_size, VOS_DMA_TO_DEVICE); // sync cpu data to dma to prevent cache delay sync to yuv buffer, which cause image problem
					}
				}
			}
		}
	}

	/* callback */
	kdf_ipp_cb_proc(kdf_hdl, KDF_IPP_CBEVT_ENG_IME_ISR, msg, (UINT32 *)&kdf_hdl, NULL);
	if (msg & CTL_IPP_IME_INTE_FMD) {
		if (kdf_hdl->flow != KDF_IPP_FLOW_DIRECT_RAW) {
			kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_IME);
		}
		kdf_ipp_chk_proc_end(kdf_hdl, KDF_IPP_ENG_IME);
	}

	/* low latency trigger timing base on bp setting
		1. bp == 0 --> trigger at frame start
		2. else    --> trigger at bp3
	*/
	if (msg & CTL_IPP_IME_INTE_BP3) {
		kdf_hdl->ime_bp_time = ctl_ipp_util_get_syst_counter();

		if (kdf_hdl->data_addr_in_process[0] != 0) {
			p_base = (CTL_IPP_BASEINFO *)kdf_hdl->data_addr_in_process[0];
			if (p_base->ime_ctrl.low_delay_bp > 0) {
				kdf_ipp_cb_proc(kdf_hdl, KDF_IPP_CBEVT_LOWDELAY, (UINT32)kdf_hdl, (void *)&kdf_hdl->data_addr_in_process[0], NULL);
			}
		}
	}

	if (msg & CTL_IPP_IME_INTE_FRM_START) {
		#if defined (__KERNEL__)
		/* update builtin timestamp */
		if (kdrv_builtin_is_fastboot() && kdf_hdl->is_first_handle && kdf_hdl->proc_end_cnt == 0) {
			kdrv_ipp_builtin_update_timestamp();
		}
		#endif
		kdf_ipp_chk_frame_start(kdf_hdl, KDF_IPP_ENG_IME);

		if (kdf_hdl->data_addr_in_process[0] != 0) {
			p_base = (CTL_IPP_BASEINFO *)kdf_hdl->data_addr_in_process[0];
			if (p_base->ime_ctrl.low_delay_bp == 0) {
				kdf_ipp_cb_proc(kdf_hdl, KDF_IPP_CBEVT_LOWDELAY, (UINT32)kdf_hdl, (void *)&kdf_hdl->data_addr_in_process[0], NULL);
			}
		}
	}
}

_INLINE void kdf_ipp_ife2_isr_handler(KDF_IPP_HANDLE *kdf_hdl, UINT32 msg)
{
	if (msg & CTL_IPP_IFE2_INTE_FMD) {
		kdf_ipp_update_lca_grayavg_to_baseinfo(kdf_hdl);
	}

	/* callback */
	kdf_ipp_cb_proc(kdf_hdl, KDF_IPP_CBEVT_ENG_IFE2_ISR, msg, (UINT32 *)&kdf_hdl, NULL);
	if (msg & CTL_IPP_IFE2_INTE_FMD) {
		if (kdf_hdl->flow != KDF_IPP_FLOW_DIRECT_RAW) {
			kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_IFE2);
		}
		kdf_ipp_chk_proc_end(kdf_hdl, KDF_IPP_ENG_IFE2);
	}
}

/*
	ISR Event callback, should follow state machine and use isr_handler to processe event
*/
INT32 kdf_ipp_rhe_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out)
{
	KDF_IPP_HANDLE *kdf_hdl = kdf_ipp_get_trig_hdl(KDF_IPP_ENG_RHE);

	if (kdf_hdl == NULL) {
		CTL_IPP_DBG_WRN("KDF HANDLE NULL\r\n");
		return 0;
	}

	if (msg & CTL_IPP_RHE_INTE_FMD) {

	}

	/* callback */
	kdf_ipp_cb_proc(kdf_hdl, KDF_IPP_CBEVT_ENG_RHE_ISR, msg, (UINT32 *)&kdf_hdl, NULL);
	if (msg & CTL_IPP_RHE_INTE_FMD) {
		kdf_ipp_set_trig_hdl(NULL, KDF_IPP_ENG_RHE);
		kdf_ipp_chk_proc_end(kdf_hdl, KDF_IPP_ENG_RHE);
	}

	return 0;
}

INT32 kdf_ipp_ife_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out)
{
	static const UINT32 pri_table[KDF_IPP_KDRV_ISR_STATE_MAX][KDF_IPP_KDRV_ISR_PRI_TABLE_MAX] = {
		/* UNKNOWN */ {CTL_IPP_IFE_INTE_SIE_FRAME_START, CTL_IPP_IFE_INTE_FMD, 0, 0},
		/* START */   {CTL_IPP_IFE_INTE_FMD|CTL_IPP_IFE_INTE_FRAME_ERR|CTL_IPP_IFE_INTE_BUFOVFL, CTL_IPP_IFE_INTE_SIE_FRAME_START, 0, 0},
		/* BP */      {0, 0, 0, 0},
		/* END */     {CTL_IPP_IFE_INTE_SIE_FRAME_START|CTL_IPP_IFE_INTE_FRAME_ERR|CTL_IPP_IFE_INTE_BUFOVFL, CTL_IPP_IFE_INTE_FMD, 0, 0},
	};
	static const KDF_IPP_KDRV_ISR_STATE state_machine[KDF_IPP_KDRV_ISR_STATE_MAX][KDF_IPP_KDRV_ISR_PRI_TABLE_MAX] = {
		/* UNKNOWN */ {KDF_IPP_KDRV_ISR_STATE_START, KDF_IPP_KDRV_ISR_STATE_END, 0, 0},
		/* START */   {KDF_IPP_KDRV_ISR_STATE_END, KDF_IPP_KDRV_ISR_STATE_START, 0, 0},
		/* BP */      {0, 0, 0, 0},
		/* END */     {KDF_IPP_KDRV_ISR_STATE_START, KDF_IPP_KDRV_ISR_STATE_END, 0, 0},
	};
	static KDF_IPP_KDRV_ISR_STATE cur_state = KDF_IPP_KDRV_ISR_STATE_UNKNOWN;
	KDF_IPP_HANDLE *kdf_hdl = kdf_ipp_get_trig_hdl(KDF_IPP_ENG_IFE);

	if (kdf_hdl == NULL) {
		CTL_IPP_DBG_WRN("KDF HANDLE NULL\r\n");
		return 0;
	}

	if (msg & CTL_IPP_IFE_INTE_STS_ERR_MASK) {
		kdf_hdl->eng_err_sts[KDF_IPP_ENG_IFE] = msg;
		kdf_hdl->eng_err_frm[KDF_IPP_ENG_IFE] = kdf_hdl->proc_end_cnt;
	}

	while (msg != 0) {
		UINT32 i;

		for (i = 0; i < KDF_IPP_KDRV_ISR_PRI_TABLE_MAX; i++) {
			if (pri_table[cur_state][i] == 0) {
				/* nothing can be process, ignore rest of the msg */
				if (msg & KDF_IPP_PROC_IFE_INTE) {
					CTL_IPP_DBG_IND("kflow_ipp ife_isr cur_state %d, ignore msg 0x%.8x\r\n", (int)cur_state, (unsigned int)msg);
				}
				msg = 0;
				break;
			}

			if (msg & pri_table[cur_state][i]) {
				kdf_ipp_ife_isr_handler(kdf_hdl, msg & pri_table[cur_state][i]);
				msg &= ~(pri_table[cur_state][i]);
				cur_state = state_machine[cur_state][i];
				break;
			}
		}
	}

	return 0;
}

INT32 kdf_ipp_dce_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out)
{
	static const UINT32 pri_table[KDF_IPP_KDRV_ISR_STATE_MAX][KDF_IPP_KDRV_ISR_PRI_TABLE_MAX] = {
		/* UNKNOWN */ {CTL_IPP_DCE_INTE_FST, 0, 0, 0},
		/* START */   {CTL_IPP_DCE_INTE_FMD, CTL_IPP_DCE_INTE_FST, 0, 0},
		/* BP */      {0, 0, 0, 0},
		/* END */     {CTL_IPP_DCE_INTE_FST, 0, 0, 0},
	};
	static const KDF_IPP_KDRV_ISR_STATE state_machine[KDF_IPP_KDRV_ISR_STATE_MAX][KDF_IPP_KDRV_ISR_PRI_TABLE_MAX] = {
		/* UNKNOWN */ {KDF_IPP_KDRV_ISR_STATE_START, 0, 0, 0},
		/* START */   {KDF_IPP_KDRV_ISR_STATE_END, KDF_IPP_KDRV_ISR_STATE_START, 0, 0},
		/* BP */      {0, 0, 0, 0},
		/* END */     {KDF_IPP_KDRV_ISR_STATE_START, 0, 0, 0},
	};
	static KDF_IPP_KDRV_ISR_STATE cur_state = KDF_IPP_KDRV_ISR_STATE_UNKNOWN;
	KDF_IPP_HANDLE *kdf_hdl = kdf_ipp_get_trig_hdl(KDF_IPP_ENG_DCE);

	if (kdf_hdl == NULL) {
		CTL_IPP_DBG_WRN("KDF HANDLE NULL\r\n");
		return 0;
	}

	if (msg & CTL_IPP_DCE_INTE_STS_ERR_MASK) {
		kdf_hdl->eng_err_sts[KDF_IPP_ENG_DCE] = msg;
		kdf_hdl->eng_err_frm[KDF_IPP_ENG_DCE] = kdf_hdl->proc_end_cnt;
	}

	while (msg != 0) {
		UINT32 i;

		for (i = 0; i < KDF_IPP_KDRV_ISR_PRI_TABLE_MAX; i++) {
			if (pri_table[cur_state][i] == 0) {
				/* nothing can be process, ignore rest of the msg */
				if (msg & KDF_IPP_PROC_DCE_INTE) {
					CTL_IPP_DBG_IND("kflow_ipp dce_isr cur_state %d, ignore msg 0x%.8x\r\n", (int)cur_state, (unsigned int)msg);
				}
				msg = 0;
				break;
			}

			if (msg & pri_table[cur_state][i]) {
				kdf_ipp_dce_isr_handler(kdf_hdl, pri_table[cur_state][i]);
				msg &= ~(pri_table[cur_state][i]);
				cur_state = state_machine[cur_state][i];
				break;
			}
		}
	}

	return 0;
}

INT32 kdf_ipp_ipe_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out)
{
	static const UINT32 pri_table[KDF_IPP_KDRV_ISR_STATE_MAX][KDF_IPP_KDRV_ISR_PRI_TABLE_MAX] = {
		/* UNKNOWN */ {CTL_IPP_IPE_INTE_FMS, 0, 0, 0},
		/* START */   {CTL_IPP_IPE_INTE_FMD, CTL_IPP_IPE_INTE_FMS, 0, 0},
		/* BP */      {0, 0, 0, 0},
		/* END */     {CTL_IPP_IPE_INTE_FMS, 0, 0, 0},
	};
	static const KDF_IPP_KDRV_ISR_STATE state_machine[KDF_IPP_KDRV_ISR_STATE_MAX][KDF_IPP_KDRV_ISR_PRI_TABLE_MAX] = {
		/* UNKNOWN */ {KDF_IPP_KDRV_ISR_STATE_START, 0, 0, 0},
		/* START */   {KDF_IPP_KDRV_ISR_STATE_END, KDF_IPP_KDRV_ISR_STATE_START, 0, 0},
		/* BP */      {0, 0, 0, 0},
		/* END */     {KDF_IPP_KDRV_ISR_STATE_START, 0, 0, 0},
	};
	static KDF_IPP_KDRV_ISR_STATE cur_state = KDF_IPP_KDRV_ISR_STATE_UNKNOWN;
	KDF_IPP_HANDLE *kdf_hdl = kdf_ipp_get_trig_hdl(KDF_IPP_ENG_IPE);

	if (kdf_hdl == NULL) {
		CTL_IPP_DBG_WRN("KDF HANDLE NULL\r\n");
		return 0;
	}

	if (msg & CTL_IPP_IPE_INTE_STS_ERR_MASK) {
		kdf_hdl->eng_err_sts[KDF_IPP_ENG_IPE] = msg;
		kdf_hdl->eng_err_frm[KDF_IPP_ENG_IPE] = kdf_hdl->proc_end_cnt;
	}

	while (msg != 0) {
		UINT32 i;

		for (i = 0; i < KDF_IPP_KDRV_ISR_PRI_TABLE_MAX; i++) {
			if (pri_table[cur_state][i] == 0) {
				/* nothing can be process, ignore rest of the msg */
				if (msg & KDF_IPP_PROC_IPE_INTE) {
					CTL_IPP_DBG_IND("kflow_ipp ipe_isr cur_state %d, ignore msg 0x%.8x\r\n", (int)cur_state, (unsigned int)msg);
				}
				msg = 0;
				break;
			}

			if (msg & pri_table[cur_state][i]) {
				kdf_ipp_ipe_isr_handler(kdf_hdl, pri_table[cur_state][i]);
				msg &= ~(pri_table[cur_state][i]);
				cur_state = state_machine[cur_state][i];
				break;
			}
		}
	}

	return 0;
}


INT32 kdf_ipp_ime_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out)
{
	static const UINT32 pri_table[KDF_IPP_KDRV_ISR_STATE_MAX][KDF_IPP_KDRV_ISR_PRI_TABLE_MAX] = {
		/* UNKNOWN */ {CTL_IPP_IME_INTE_FRM_START, 0, 0, 0},
		/* START */   {CTL_IPP_IME_INTE_BP3, CTL_IPP_IME_INTE_FMD, CTL_IPP_IME_INTE_FRM_START, 0},
		/* BP */      {CTL_IPP_IME_INTE_FMD, CTL_IPP_IME_INTE_FRM_START, 0, 0},
		/* END */     {CTL_IPP_IME_INTE_FRM_START, 0, 0, 0},
	};
	static const KDF_IPP_KDRV_ISR_STATE state_machine[KDF_IPP_KDRV_ISR_STATE_MAX][KDF_IPP_KDRV_ISR_PRI_TABLE_MAX] = {
		/* UNKNOWN */ {KDF_IPP_KDRV_ISR_STATE_START, 0, 0, 0},
		/* START */   {KDF_IPP_KDRV_ISR_STATE_BP, KDF_IPP_KDRV_ISR_STATE_END, KDF_IPP_KDRV_ISR_STATE_START, 0},
		/* BP */      {KDF_IPP_KDRV_ISR_STATE_END, KDF_IPP_KDRV_ISR_STATE_START, 0, 0},
		/* END */     {KDF_IPP_KDRV_ISR_STATE_START, 0, 0, 0},
	};
	static KDF_IPP_KDRV_ISR_STATE cur_state = KDF_IPP_KDRV_ISR_STATE_UNKNOWN;
	KDF_IPP_HANDLE *kdf_hdl = kdf_ipp_get_trig_hdl(KDF_IPP_ENG_IME);


	if (kdf_hdl == NULL) {
		CTL_IPP_DBG_WRN("KDF HANDLE NULL\r\n");
		return 0;
	}

	if (msg & CTL_IPP_IME_INTE_STS_ERR_MASK) {
		kdf_hdl->eng_err_sts[KDF_IPP_ENG_IME] = msg;
		kdf_hdl->eng_err_frm[KDF_IPP_ENG_IME] = kdf_hdl->proc_end_cnt;
	}

	while (msg != 0) {
		UINT32 i;

		for (i = 0; i < KDF_IPP_KDRV_ISR_PRI_TABLE_MAX; i++) {
			if (pri_table[cur_state][i] == 0) {
				/* nothing can be process, ignore rest of the msg */
				if (msg & KDF_IPP_PROC_IME_INTE) {
					CTL_IPP_DBG_IND("kflow_ipp ime_isr cur_state %d, ignore msg 0x%.8x\r\n", (int)cur_state, (unsigned int)msg);
				}
				msg = 0;
				break;
			}

			if (msg & pri_table[cur_state][i]) {
				kdf_ipp_ime_isr_handler(kdf_hdl, pri_table[cur_state][i]);
				msg &= ~(pri_table[cur_state][i]);
				cur_state = state_machine[cur_state][i];
				break;
			}
		}
	}

	return 0;
}

INT32 kdf_ipp_ife2_isr_cb(UINT32 handle, UINT32 msg, void *in, void *out)
{
	static const UINT32 pri_table[KDF_IPP_KDRV_ISR_STATE_MAX][KDF_IPP_KDRV_ISR_PRI_TABLE_MAX] = {
		/* UNKNOWN */ {CTL_IPP_IFE2_INTE_FMD, 0, 0, 0},
		/* START */   {0, 0, 0, 0},
		/* BP */      {0, 0, 0, 0},
		/* END */     {CTL_IPP_IFE2_INTE_FMD, 0, 0, 0},
	};
	static const KDF_IPP_KDRV_ISR_STATE state_machine[KDF_IPP_KDRV_ISR_STATE_MAX][KDF_IPP_KDRV_ISR_PRI_TABLE_MAX] = {
		/* UNKNOWN */ {KDF_IPP_KDRV_ISR_STATE_END, 0, 0, 0},
		/* START */   {0, 0, 0, 0},
		/* BP */      {0, 0, 0, 0},
		/* END */     {KDF_IPP_KDRV_ISR_STATE_END, 0, 0, 0},
	};
	static KDF_IPP_KDRV_ISR_STATE cur_state = KDF_IPP_KDRV_ISR_STATE_UNKNOWN;
	KDF_IPP_HANDLE *kdf_hdl = kdf_ipp_get_trig_hdl(KDF_IPP_ENG_IFE2);

	if (kdf_hdl == NULL) {
		CTL_IPP_DBG_IND("KDF HANDLE NULL\r\n");
		return 0;
	}

	/* work around in case of ife2 do not provide frame start */
	if ((kdf_hdl->dir_load_bit & KDF_IPP_DIR_LOAD_PAUSE) || (kdf_hdl->dir_load_bit & KDF_IPP_DIR_LOAD_PAUSE_E)) {
		return 0;
	}

	if (msg & CTL_IPP_IFE2_INTE_STS_ERR_MASK) {
		kdf_hdl->eng_err_sts[KDF_IPP_ENG_IFE2] = msg;
		kdf_hdl->eng_err_frm[KDF_IPP_ENG_IFE2] = kdf_hdl->proc_end_cnt;
	}

	while (msg != 0) {
		UINT32 i;

		for (i = 0; i < KDF_IPP_KDRV_ISR_PRI_TABLE_MAX; i++) {
			if (pri_table[cur_state][i] == 0) {
				/* nothing can be process, ignore rest of the msg */
				if (msg & KDF_IPP_PROC_IFE2_INTE) {
					CTL_IPP_DBG_IND("kflow_ipp ife2_isr cur_state %d, ignore msg 0x%.8x\r\n", (int)cur_state, (unsigned int)msg);
				}
				msg = 0;
				break;
			}

			if (msg & pri_table[cur_state][i]) {
				kdf_ipp_ife2_isr_handler(kdf_hdl, pri_table[cur_state][i]);
				msg &= ~(pri_table[cur_state][i]);
				cur_state = state_machine[cur_state][i];
				break;
			}
		}
	}

	return 0;
}

#if 0
#endif
/*
	internal used function
*/
static void kdf_ipp_handle_pool_lock(void)
{
	FLGPTN flg;

	vos_flag_wait(&flg, kdf_ipp_handle_pool.flg_id[0], KDF_IPP_HANDLE_POOL_LOCK, TWF_CLR);
}

static void kdf_ipp_handle_pool_unlock(void)
{
	vos_flag_set(kdf_ipp_handle_pool.flg_id[0], KDF_IPP_HANDLE_POOL_LOCK);
}

static KDF_IPP_HANDLE_LOCKINFO kdf_ipp_handle_get_lockinfo(UINT32 lock)
{
	KDF_IPP_HANDLE_LOCKINFO rt;

	rt.flag = (UINT32)(lock / KDF_IPP_HANDLE_AVAILABLE_FLAG_BIT);
	rt.ptn = 1 << (lock % KDF_IPP_HANDLE_AVAILABLE_FLAG_BIT);

	return rt;
}

INT32 kdf_ipp_handle_lock(UINT32 lock, BOOL b_timeout_en)
{
	FLGPTN flg = 0;
	KDF_IPP_HANDLE_LOCKINFO lock_info;
	ER wai_rt;

	lock_info = kdf_ipp_handle_get_lockinfo(lock);
	if (b_timeout_en) {
		wai_rt = vos_flag_wait_timeout(&flg, kdf_ipp_handle_pool.flg_id[lock_info.flag], lock_info.ptn, TWF_CLR, vos_util_msec_to_tick(KDF_IPP_HANDLE_LOCK_TIMEOUT_MS));
		if (wai_rt != E_OK) {
			if (wai_rt == E_TMOUT) {
				CTL_IPP_DBG_WRN("lock kdf_handle timeout(%d ms), check if engine hang\r\n", (int)(KDF_IPP_HANDLE_LOCK_TIMEOUT_MS));
				return CTL_IPP_E_TIMEOUT;
			}
		}
	} else {
		vos_flag_wait(&flg, kdf_ipp_handle_pool.flg_id[lock_info.flag], lock_info.ptn, TWF_CLR);
	}

	return CTL_IPP_E_OK;
}

void kdf_ipp_handle_unlock(UINT32 lock)
{
	KDF_IPP_HANDLE_LOCKINFO lock_info;

	lock_info = kdf_ipp_handle_get_lockinfo(lock);

	vos_flag_iset(kdf_ipp_handle_pool.flg_id[lock_info.flag], lock_info.ptn);
}

INT32 kdf_ipp_handle_validate(KDF_IPP_HANDLE *p_hdl)
{
	UINT32 i;

	for (i = 0; i < kdf_ipp_handle_pool.handle_num; i++) {
		if ((UINT32)p_hdl == (UINT32)&kdf_ipp_handle_pool.handle[i]) {
			return CTL_IPP_E_OK;
		}
	}

	CTL_IPP_DBG_IND("kdf invalid handle(0x%.8x)\r\n", (unsigned int)p_hdl);
	return CTL_IPP_E_ID;
}

static KDF_IPP_HANDLE *kdf_ipp_handle_alloc(void)
{
	KDF_IPP_HANDLE *p_handle;

	if (vos_list_empty(&kdf_ipp_handle_pool.free_head)) {
		return NULL;
	}

	p_handle = vos_list_entry(kdf_ipp_handle_pool.free_head.next, KDF_IPP_HANDLE, list);
	kdf_ipp_handle_reset(p_handle);
	vos_list_del(&p_handle->list);
	vos_list_add_tail(&p_handle->list, &kdf_ipp_handle_pool.used_head);

	return p_handle;
}

static void kdf_ipp_handle_release(KDF_IPP_HANDLE *p_hdl)
{
	p_hdl->sts = KDF_IPP_HANDLE_STS_FREE;
	vos_list_del(&p_hdl->list);
	vos_list_add_tail(&p_hdl->list, &kdf_ipp_handle_pool.free_head);
}

static INT32 kdf_ipp_open_kdrv(KDF_IPP_HANDLE *p_hdl)
{
	/* open kdriver based on flow type */
	INT32 rt = E_OK;
	UINT8 is_fastboot = FALSE;

	#if defined (__KERNEL__)
	is_fastboot = kdrv_builtin_is_fastboot();
	#endif

	if (kdf_ipp_kdrv_open_count == 0) {
		KDRV_IFE_OPENCFG ife_open_obj;
		KDRV_DCE_OPENCFG dce_open_obj;
		KDRV_IPE_OPENCFG ipe_open_obj;
		KDRV_IME_OPENCFG ime_open_obj;
		KDRV_IFE2_OPENCFG ife2_open_obj;
		UINT32 clk;
		UINT32 i;

		/* set open config, preset channel to prevent kdriver set error
			todo: get clock rate from ?
		*/
		clk = ctl_ipp_util_get_dtsi_clock();
		ife_open_obj.ife_clock_sel = clk;
		dce_open_obj.dce_clock_sel = clk;
		ipe_open_obj.ipe_clock_sel = clk;
		ime_open_obj.ime_clock_sel = clk;
		ife2_open_obj.ife2_clock_sel = clk;

		for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
			p_hdl->channel[i] = 0;
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_OPENCFG, (void *)&ife_open_obj);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_OPENCFG, (void *)&dce_open_obj);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_OPENCFG, (void *)&ipe_open_obj);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_OPENCFG, (void *)&ime_open_obj);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_hdl, KDRV_IFE2_PARAM_IPL_OPENCFG, (void *)&ife2_open_obj);
		for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
			p_hdl->channel[i] = KDF_IPP_HANDLE_KDRV_CH_NONE;
		}

		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("flow %d, set engine clock failed\r\n", (int)(p_hdl->flow));
			return CTL_IPP_E_KDRV_SET;
		}
	}

	switch (p_hdl->flow) {
	case KDF_IPP_FLOW_RAW:
	case KDF_IPP_FLOW_DIRECT_RAW:
		/* open engine */
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_IFE, p_hdl);
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_DCE, p_hdl);
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_IPE, p_hdl);
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_IME, p_hdl);
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_IFE2, p_hdl);
		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("flow %d, engine open failed\r\n", (int)(p_hdl->flow));
			return CTL_IPP_E_KDRV_OPEN;
		}

		/* register isr cb */
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_ISR_CB, (void *)kdf_ipp_ife_isr_cb);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ISR_CB, (void *)kdf_ipp_dce_isr_cb);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_ISR_CB, (void *)kdf_ipp_ipe_isr_cb);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_ISR_CB, (void *)kdf_ipp_ime_isr_cb);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_hdl, KDRV_IFE2_PARAM_IPL_ISR_CB, (void *)&kdf_ipp_ife2_isr_cb);
		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("KDRV ISR Callback setting failed\r\n");
			return CTL_IPP_E_KDRV_SET;
		}
		break;

	case KDF_IPP_FLOW_IME_D2D:
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_IME, p_hdl);
		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("flow %d, engine open failed\r\n", (int)(p_hdl->flow));
			return CTL_IPP_E_KDRV_OPEN;
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_ISR_CB, (void *)kdf_ipp_ime_isr_cb);
		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("KDRV ISR Callback setting failed\r\n");
			return CTL_IPP_E_KDRV_SET;
		}
		break;

	case KDF_IPP_FLOW_CCIR:
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_DCE, p_hdl);
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_IPE, p_hdl);
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_IME, p_hdl);
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_IFE2, p_hdl);
		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("flow %d, engine open failed\r\n", (int)(p_hdl->flow));
			return CTL_IPP_E_KDRV_OPEN;
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ISR_CB, (void *)kdf_ipp_dce_isr_cb);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_ISR_CB, (void *)kdf_ipp_ipe_isr_cb);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_ISR_CB, (void *)kdf_ipp_ime_isr_cb);
		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_hdl, KDRV_IFE2_PARAM_IPL_ISR_CB, (void *)&kdf_ipp_ife2_isr_cb);
		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("KDRV ISR Callback setting failed\r\n");
			return CTL_IPP_E_KDRV_SET;
		}
		break;

	case KDF_IPP_FLOW_IPE_D2D:
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_IPE, p_hdl);
		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("flow %d, engine open failed\r\n", (int)(p_hdl->flow));
			return CTL_IPP_E_KDRV_OPEN;
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_ISR_CB, (void *)kdf_ipp_ipe_isr_cb);
		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("KDRV ISR Callback setting failed\r\n");
			return CTL_IPP_E_KDRV_SET;
		}
		break;

	case KDF_IPP_FLOW_DCE_D2D:
		rt |= kdf_ipp_kdrv_open_wrapper(KDF_IPP_ENG_DCE, p_hdl);
		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("flow %d, engine open failed\r\n", (int)(p_hdl->flow));
			return CTL_IPP_E_KDRV_OPEN;
		}

		rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_ISR_CB, (void *)kdf_ipp_dce_isr_cb);
		if (rt != E_OK) {
			CTL_IPP_DBG_ERR("KDRV ISR Callback setting failed\r\n");
			return CTL_IPP_E_KDRV_SET;
		}
		break;

	default:
		CTL_IPP_DBG_ERR("Unknown flow %d\r\n", (int)(p_hdl->flow));
		return CTL_IPP_E_PAR;
	}

	/* check fastboot engine status
		1. direct mode, if ife2 not enable, disable kdrv_ife2 fastboot flow
		2. d2d mode, disable kdrv fastboot flow
	*/
	if (is_fastboot) {
		UINT32 dummy = 0;

		if (kdrv_ipp_builtin_get_eng_enable(KDRV_IPP_BUILTIN_IFE) == FALSE) {
			kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE, p_hdl, KDRV_IFE_PARAM_IPL_BUILTIN_DISABLE, (void *)&dummy);
		}

		if (kdrv_ipp_builtin_get_eng_enable(KDRV_IPP_BUILTIN_DCE) == FALSE) {
			kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_BUILTIN_DISABLE, (void *)&dummy);
		}

		if (kdrv_ipp_builtin_get_eng_enable(KDRV_IPP_BUILTIN_IPE) == FALSE) {
			kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IPE, p_hdl, KDRV_IPE_PARAM_IPL_BUILTIN_DISABLE, (void *)&dummy);
		}

		if (kdrv_ipp_builtin_get_eng_enable(KDRV_IPP_BUILTIN_IME) == FALSE) {
			kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IME, p_hdl, KDRV_IME_PARAM_IPL_BUILTIN_DISABLE, (void *)&dummy);
		}

		if (kdrv_ipp_builtin_get_eng_enable(KDRV_IPP_BUILTIN_IFE2) == FALSE) {
			kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_IFE2, p_hdl, KDRV_IFE2_PARAM_IPL_BUILTIN_DISABLE, (void *)&dummy);
		}
	}

	kdf_ipp_kdrv_open_count += 1;
	return CTL_IPP_E_OK;
}

static void kdf_ipp_close_kdrv(KDF_IPP_HANDLE *p_hdl)
{
	/* close kdrv, release channel */
	INT32 rt;
	KDF_IPP_ENG eng;

	for (eng = KDF_IPP_ENG_RHE; eng < KDF_IPP_ENG_MAX; eng++) {
		if (p_hdl->channel[eng] != KDF_IPP_HANDLE_KDRV_CH_NONE) {
			rt = kdf_ipp_kdrv_close_wrapper(eng, p_hdl);

			if (rt != E_OK) {
				CTL_IPP_DBG_ERR("eng %d, close failed\r\n", (int)(eng));
			}
		}
	}

	kdf_ipp_kdrv_open_count -= 1;
}

INT32 kdf_ipp_process(UINT32 hdl, UINT32 data_addr, UINT32 rev, UINT64 snd_time, UINT32 proc_cmd)
{
	KDF_IPP_HANDLE *p_hdl;
	UINT64 rev_time;
	INT32 rt;

	rev_time = ctl_ipp_util_get_syst_counter();
	p_hdl = (KDF_IPP_HANDLE *)hdl;

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[KDF_IPL] process hdl 0x%x, header 0x%.8x, rev2 %d\r\n", (unsigned int)hdl, (unsigned int)data_addr, (int)rev);

	if (kdf_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		return CTL_IPP_E_ID;
	}

	/* kdf process based on flow */
	rt = kdf_ipp_handle_lock(p_hdl->lock, TRUE);
	if (rt != CTL_IPP_E_OK) {
		kdf_ipp_eng_hang();

		rt = kdf_ipp_handle_lock(p_hdl->lock, TRUE);
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("lock handle(%s) timeout(%d ms), check if engine hang\r\n", p_hdl->name, KDF_IPP_HANDLE_LOCK_TIMEOUT_MS);
			return rt;
		}
	}

	p_hdl->snd_evt_time = snd_time;
	p_hdl->rev_evt_time = rev_time;

	if (p_hdl->sts == KDF_IPP_HANDLE_STS_FREE) {
		/* handle is in free status */
		kdf_ipp_handle_unlock(p_hdl->lock);
		CTL_IPP_DBG_WRN("illegal handle(0x%.8x), status(0x%.8x)\r\n", (unsigned int)hdl, (unsigned int)p_hdl->sts);
		return CTL_IPP_E_STATE;
	}

	rt = kdf_ipp_eng_lock(kdf_ipp_get_eng_lock_mask(p_hdl->flow));
	if (rt != CTL_IPP_E_OK) {
		kdf_ipp_eng_hang();

		rt = kdf_ipp_eng_lock(kdf_ipp_get_eng_lock_mask(p_hdl->flow));
		if (rt != CTL_IPP_E_OK) {
			CTL_IPP_DBG_WRN("handle(%s) lock engine timeout(%d ms), check if engine hang\r\n", p_hdl->name, KDF_IPP_HANDLE_LOCK_TIMEOUT_MS);
			kdf_ipp_handle_unlock(p_hdl->lock);
			return rt;
		}
	}

	p_hdl->proc_st_time = ctl_ipp_util_get_syst_counter();
	p_hdl->trig_ed_time = 0;
	p_hdl->ime_bp_time = 0;
	if (proc_cmd == KDF_IPP_MSG_PROCESS_PATTERN_PASTE) {
		rt = kdf_ipp_process_flow_pattern_paste(p_hdl, data_addr, 0);
	} else {
		switch (p_hdl->flow) {
		case KDF_IPP_FLOW_RAW:
			rt = kdf_ipp_process_raw(p_hdl, data_addr, 0);
			break;

		case KDF_IPP_FLOW_IME_D2D:
			rt = kdf_ipp_process_ime_d2d(p_hdl, data_addr, 0);
			break;

		case KDF_IPP_FLOW_CCIR:
			rt = kdf_ipp_process_ccir(p_hdl, data_addr, 0);
			break;

		case KDF_IPP_FLOW_IPE_D2D:
			rt = kdf_ipp_process_ipe_d2d(p_hdl, data_addr, 0);
			break;

		case KDF_IPP_FLOW_DCE_D2D:
			rt = kdf_ipp_process_dce_d2d(p_hdl, data_addr, 0);
			break;

		default:
			CTL_IPP_DBG_WRN("Unknown Flow %d\r\n", (int)(p_hdl->flow));
			rt = CTL_IPP_E_PAR;
			break;
		}
	}

	if (rt != CTL_IPP_E_OK) {
		/* unlock handle/eng when process fail */
		kdf_ipp_eng_unlock(kdf_ipp_get_eng_lock_mask(p_hdl->flow));
		kdf_ipp_handle_unlock(p_hdl->lock);
	}

	return rt;
}

INT32 kdf_ipp_drop(UINT32 hdl, UINT32 data_addr, UINT32 rev, IPP_EVENT_FP drop_fp, INT32 err_msg)
{
	CTL_IPP_EVT_TIME_INFO time_info;
	KDF_IPP_EVT drop_evt;
	KDF_IPP_HANDLE *p_hdl;

	p_hdl = (KDF_IPP_HANDLE *)hdl;

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[KDF_IPL] drop hdl 0x%x, header 0x%.8x, rev2 %d\r\n", (unsigned int)hdl, (unsigned int)data_addr, (int)rev);

	if (kdf_ipp_handle_validate(p_hdl) == CTL_IPP_E_OK) {
		unsigned long loc_cpu_flg;

		loc_cpu(loc_cpu_flg);
		p_hdl->drop_cnt += 1;
		unl_cpu(loc_cpu_flg);
	}

	memset((void *)&time_info, 0, sizeof(CTL_IPP_EVT_TIME_INFO));
	time_info.snd = p_hdl->snd_evt_time;
	time_info.rev = p_hdl->rev_evt_time;
	time_info.start = p_hdl->proc_st_time;
	time_info.proc_data_addr = data_addr;
	time_info.err_msg = err_msg;
	kdf_ipp_cb_proc(p_hdl, KDF_IPP_CBEVT_PROC_TIME, (UINT32)p_hdl, (void *)&time_info, NULL);

	if (drop_fp == NULL) {
		return E_PAR;
	}

	drop_evt.data_addr = data_addr;
	drop_evt.rev = rev;
	drop_evt.err_msg = err_msg;
	drop_fp((UINT32)p_hdl, (void *)&drop_evt, NULL);

	return E_OK;
}

INT32 kdf_ipp_manual_hwrst(UINT32 hdl, UINT32 data_addr, UINT32 rev)
{
	KDF_IPP_HWRST_CFG *p_cfg;
	KDF_IPP_HWRST_CFG backup_cfg;
	BOOL backup_is_dump;

	CTL_IPP_DBG_TRC("[KDF_IPL] hwrst event\r\n");

	p_cfg = (KDF_IPP_HWRST_CFG *)data_addr;
	if (p_cfg == NULL) {
		return E_PAR;
	}
	memset((void *)backup_cfg.file_path, '\0', sizeof(CHAR) * 32);
	strncpy(backup_cfg.file_path, kdf_ipp_eng_hang_dump_file_path, (32 - 1));
	backup_cfg.mode = kdf_ipp_eng_hand_mode;
	backup_is_dump = kdf_ipp_eng_hang_is_dump;

	/* do eng hang flow */
	memset((void *)kdf_ipp_eng_hang_dump_file_path, '\0', sizeof(CHAR) * 32);
	strncpy(kdf_ipp_eng_hang_dump_file_path, p_cfg->file_path, (32 - 1));
	kdf_ipp_eng_hand_mode = p_cfg->mode;
	kdf_ipp_eng_hang_is_dump = FALSE;

	kdf_ipp_eng_hang();

	/* restore config */
	memset((void *)kdf_ipp_eng_hang_dump_file_path, '\0', sizeof(CHAR) * 32);
	strncpy(kdf_ipp_eng_hang_dump_file_path, backup_cfg.file_path, (32 - 1));
	kdf_ipp_eng_hand_mode = backup_cfg.mode;
	kdf_ipp_eng_hang_is_dump = backup_is_dump;

	return E_OK;
}

#if 0
#endif

ER kdf_ipp_open_tsk(void)
{
	if (kdf_ipp_task_opened) {
		CTL_IPP_DBG_ERR("re-open\r\n");
		return E_SYS;
	}

	if (g_kdf_ipp_flg_id == 0) {
		CTL_IPP_DBG_ERR("g_kdf_ipp_flg_id = %d\r\n", (int)(g_kdf_ipp_flg_id));
		return E_SYS;
	}

	/* kdf init */
	kdf_ipp_task_pause_count = 0;
	kdf_ipp_kdrv_open_count = 0;

	/* reset flag */
	vos_flag_clr(g_kdf_ipp_flg_id, FLGPTN_BIT_ALL);
	vos_flag_set(g_kdf_ipp_flg_id, KDF_IPP_PROC_INIT);

	/* reset queue */
	kdf_ipp_msg_reset_queue();

	/* start kdf task */
	THREAD_CREATE(g_kdf_ipp_tsk_id, kdf_ipp_tsk, NULL, "kdf_ipp_tsk");
	if (g_kdf_ipp_tsk_id == 0) {
		return E_SYS;
	}
	THREAD_SET_PRIORITY(g_kdf_ipp_tsk_id, KDF_IPP_TSK_PRI);
	THREAD_RESUME(g_kdf_ipp_tsk_id);

	kdf_ipp_task_opened = TRUE;

	/* set kdf task to processing */
	kdf_ipp_set_resume(TRUE);

	return E_OK;
}

ER kdf_ipp_close_tsk(void)
{
	FLGPTN wait_flg = 0;

	if (!kdf_ipp_task_opened) {
		CTL_IPP_DBG_ERR("re-close\r\n");
		return E_SYS;
	}

	while (kdf_ipp_task_pause_count) {
		kdf_ipp_set_pause(ENABLE, ENABLE);
	}

	vos_flag_set(g_kdf_ipp_flg_id, KDF_IPP_TASK_RES);
	if (vos_flag_wait(&wait_flg, g_kdf_ipp_flg_id, KDF_IPP_TASK_EXIT_END, (TWF_ORW | TWF_CLR))) {
		THREAD_DESTROY(g_kdf_ipp_tsk_id);
	}

	kdf_ipp_task_opened = FALSE;
	return E_OK;
}


THREAD_DECLARE(kdf_ipp_tsk, p1)
{
	ER err;
	UINT32 cmd, param[3];
	FLGPTN wait_flg = 0;
	IPP_EVENT_FP drop_fp;
	UINT64 snd_time;

	THREAD_ENTRY();

	goto KDF_IPP_PAUSE;

KDF_IPP_PROC:
	while (!THREAD_SHOULD_STOP) {
		vos_flag_set(g_kdf_ipp_flg_id, (KDF_IPP_TASK_IDLE | KDF_IPP_TASK_RESUME_END));
		PROFILE_TASK_IDLE();
		drop_fp = NULL;
		err = kdf_ipp_msg_rcv(&cmd, &param[0], &param[1], &param[2], &drop_fp, &snd_time);
		PROFILE_TASK_BUSY();
		vos_flag_clr(g_kdf_ipp_flg_id, KDF_IPP_TASK_IDLE);

		if (err != E_OK) {
			CTL_IPP_DBG_IND("[KDF_IPL]Rcv Msg error (%d)\r\n", (int)(err));
		} else {
			/* process event */
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_FLOW, "[KDF_IPL]P Cmd:%d P1:0x%x P2:0x%x P3:0x%x Drop_fp 0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]), (unsigned int)((UINT32)drop_fp));

			if (cmd == KDF_IPP_MSG_IGNORE) {
				/* do nothing */
			} else if (cmd == KDF_IPP_MSG_PROCESS || cmd == KDF_IPP_MSG_PROCESS_PATTERN_PASTE) {
				INT32 rt;

				rt = kdf_ipp_process(param[0], param[1], param[2], snd_time, cmd);
				if (rt != CTL_IPP_E_OK) {
					/* process failed, inform drop */
					kdf_ipp_drop(param[0], param[1], param[2], drop_fp, rt);
				}
			} else if (cmd == KDF_IPP_MSG_DROP) {
				kdf_ipp_drop(param[0], param[1], param[2], drop_fp, CTL_IPP_E_OK);
			} else if (cmd == KDF_IPP_MSG_HWRST) {
				/* for manual trigger hwrst */
				kdf_ipp_manual_hwrst(param[0], param[1], param[2]);
			} else if (cmd == KDF_IPP_MSG_BUILTIN_EXIT) {
				/* fastboot exit before start processing */
				#if defined (__KERNEL__)
				kdrv_ipp_builtin_exit();
				#endif
			} else {
				CTL_IPP_DBG_WRN("un-support Cmd:%d P1:0x%x P2:0x%x P3:0x%x\r\n", (int)(cmd), (unsigned int)(param[0]), (unsigned int)(param[1]), (unsigned int)(param[2]));
			}

			/* check pause after cmd is processed */
			vos_flag_set(g_kdf_ipp_flg_id, KDF_IPP_TASK_CHK);
			vos_flag_wait(&wait_flg, g_kdf_ipp_flg_id, (KDF_IPP_TASK_PAUSE | KDF_IPP_TASK_CHK), (TWF_ORW | TWF_CLR));
			if (wait_flg & KDF_IPP_TASK_PAUSE) {
				INT32 rt;

				/* lock engine to ensure engine process end */
				rt = kdf_ipp_eng_lock(KDF_IPP_ENG_BIT_ALL);
				if (rt != CTL_IPP_E_OK) {
					kdf_ipp_eng_hang();
					rt = kdf_ipp_eng_lock(KDF_IPP_ENG_BIT_ALL);
				}

				if (rt == CTL_IPP_E_OK) {
					kdf_ipp_eng_unlock(KDF_IPP_ENG_BIT_ALL);
				} else {
					CTL_IPP_DBG_WRN("kdf task pause wait eng timeout(%d ms)\r\n", (int)(KDF_IPP_TASK_TIMEOUT_MS));
				}
				break;
			}
		}
	}

KDF_IPP_PAUSE:

	vos_flag_clr(g_kdf_ipp_flg_id, (KDF_IPP_TASK_RESUME_END));
	vos_flag_set(g_kdf_ipp_flg_id, KDF_IPP_TASK_PAUSE_END);
	vos_flag_wait(&wait_flg, g_kdf_ipp_flg_id, (KDF_IPP_TASK_RES | KDF_IPP_TASK_RESUME | KDF_IPP_TASK_FLUSH), (TWF_ORW | TWF_CLR));

	if (wait_flg & KDF_IPP_TASK_RES) {
		goto KDF_IPP_DESTROY;
	}

	if (wait_flg & KDF_IPP_TASK_RESUME) {
		vos_flag_clr(g_kdf_ipp_flg_id, KDF_IPP_TASK_PAUSE_END);
		if (wait_flg & KDF_IPP_TASK_FLUSH) {
			kdf_ipp_msg_flush();
		}
		goto KDF_IPP_PROC;
	}

KDF_IPP_DESTROY:
	vos_flag_set(g_kdf_ipp_flg_id, KDF_IPP_TASK_EXIT_END);
	THREAD_RETURN(0);
}

ER kdf_ipp_set_resume(BOOL b_flush_evt)
{
	FLGPTN wait_flg;
	unsigned long loc_cpu_flg;

	if (!kdf_ipp_task_opened) {
		return E_SYS;
	}

	loc_cpu(loc_cpu_flg);
	if (kdf_ipp_task_pause_count == 0) {
		kdf_ipp_task_pause_count += 1;
		unl_cpu(loc_cpu_flg);
		if (b_flush_evt == ENABLE) {
			vos_flag_set(g_kdf_ipp_flg_id, (KDF_IPP_TASK_RESUME | KDF_IPP_TASK_FLUSH));
		} else {
			vos_flag_set(g_kdf_ipp_flg_id, KDF_IPP_TASK_RESUME);
		}

		vos_flag_wait(&wait_flg, g_kdf_ipp_flg_id, KDF_IPP_TASK_RESUME_END, TWF_ANDW);
	} else {
		kdf_ipp_task_pause_count += 1;
		unl_cpu(loc_cpu_flg);
	}

	return E_OK;
}

ER kdf_ipp_set_pause(BOOL b_wait_end, BOOL b_flush_evt)
{
	FLGPTN wait_flg;
	unsigned long loc_cpu_flg;

	if (!kdf_ipp_task_opened) {
		return E_SYS;
	}

	loc_cpu(loc_cpu_flg);
	if (kdf_ipp_task_pause_count != 0) {
		kdf_ipp_task_pause_count -= 1;

		if (kdf_ipp_task_pause_count == 0) {
			unl_cpu(loc_cpu_flg);
			wait_flg = KDF_IPP_TASK_PAUSE;

			vos_flag_set(g_kdf_ipp_flg_id, wait_flg);
			/* send dummy msg, kdf_tsk must rcv msg to pause */
			kdf_ipp_msg_snd(KDF_IPP_MSG_IGNORE, 0, 0, 0, NULL, 0);

			if (b_wait_end == ENABLE) {
				if (vos_flag_wait_timeout(&wait_flg, g_kdf_ipp_flg_id, KDF_IPP_TASK_PAUSE_END, TWF_ORW, vos_util_msec_to_tick(KDF_IPP_TASK_TIMEOUT_MS)) == E_OK) {
					if (b_flush_evt == TRUE) {
						kdf_ipp_msg_flush();
					}
				} else {
					CTL_IPP_DBG_ERR("wait kdf task pause timeout(%d ms)\r\n", (int)(KDF_IPP_TASK_TIMEOUT_MS));
					return E_TMOUT;
				}
			}
		} else {
			unl_cpu(loc_cpu_flg);
		}
	} else {
		CTL_IPP_DBG_WRN("task pause cnt already 0\r\n");
		unl_cpu(loc_cpu_flg);
	}

	return E_OK;
}

ER kdf_ipp_wait_pause_end(void)
{
	FLGPTN wait_flg;

	if (!kdf_ipp_task_opened) {
		return E_SYS;
	}

	vos_flag_wait(&wait_flg, g_kdf_ipp_flg_id, KDF_IPP_TASK_PAUSE_END, TWF_ORW);
	return E_OK;
}

#if 0
#endif

void kdf_ipp_handle_reset(KDF_IPP_HANDLE *p_hdl)
{
	UINT32 i;

	memset((void *)&p_hdl->name[0], '\0', sizeof(CHAR) * KDF_IPP_HANDLE_NAME_MAX);
	p_hdl->name[0] = '\0';
	p_hdl->sts = KDF_IPP_HANDLE_STS_FREE;
	p_hdl->flow = KDF_IPP_FLOW_UNKNOWN;
	p_hdl->chip = KDRV_CHIP0;
	p_hdl->proc_end_bit = 0;
	p_hdl->proc_end_mask = 0;
	p_hdl->frame_sta_bit= 0;
	p_hdl->frame_sta_mask= 0;
	p_hdl->dir_load_bit = 0;
	p_hdl->data_addr_in_process[0] = 0;
	p_hdl->data_addr_in_process[1] = 0;
	p_hdl->data_addr_in_process[2] = 0;
	p_hdl->snd_evt_cnt = 0;
	p_hdl->proc_end_cnt = 0;
	p_hdl->drop_cnt = 0;
	p_hdl->frame_sta_cnt = 0;
	p_hdl->fra_sta_with_sop_cnt = 0;
	p_hdl->reset_cnt = 0;
	p_hdl->snd_evt_time = 0;
	p_hdl->rev_evt_time = 0;
	p_hdl->proc_st_time = 0;
	p_hdl->proc_sts = 0;
	p_hdl->is_first_handle = 0;
	p_hdl->engine_used_mask = 0;

	vk_spin_lock_init(&p_hdl->spinlock);
	for (i = 0; i < KDF_IPP_CBEVT_MAX; i++) {
		p_hdl->cb_fp[i] = NULL;
	}

	for (i = 0; i < KDF_IPP_ENG_MAX; i++) {
		p_hdl->channel[i] = KDF_IPP_HANDLE_KDRV_CH_NONE;

		p_hdl->eng_err_sts[i] = 0;
		p_hdl->eng_err_frm[i] = 0;
	}
}

static UINT32 kdf_ipp_handle_pool_init(UINT32 num, UINT32 buf_addr, UINT32 is_query)
{
	UINT32 i, lock_val, buf_size;
	void *p_buf;

	/* buffer alloc
		1. handle buffer
		2. flag id buffer
	*/
	kdf_ipp_handle_pool.handle_num = num;
	kdf_ipp_handle_pool.flg_num = ALIGN_CEIL((num + KDF_IPP_HANDLE_FREE_FLAG_START), KDF_IPP_HANDLE_AVAILABLE_FLAG_BIT) / KDF_IPP_HANDLE_AVAILABLE_FLAG_BIT;

	buf_size = (sizeof(KDF_IPP_HANDLE) * num) + kdf_ipp_handle_pool.flg_num * sizeof(FLGPTN);
	if (is_query) {
		ctl_ipp_dbg_ctxbuf_log_set("KDF_IPP_HANDLE", CTL_IPP_DBG_CTX_BUF_QUERY, buf_size, buf_addr, num);
		return buf_size;
	}

	p_buf = (void *)buf_addr;
	if (p_buf == NULL) {
		kdf_ipp_handle_pool.handle_num = 0;
		kdf_ipp_handle_pool.flg_num = 0;
		CTL_IPP_DBG_ERR("alloc kdf_handle pool memory failed, num %d, want_size 0x%.8x\r\n", (int)num, (unsigned int)buf_size);
		return CTL_IPP_E_NOMEM;
	}
	kdf_ipp_handle_pool.flg_id = p_buf;
	kdf_ipp_handle_pool.handle = (KDF_IPP_HANDLE *)((UINT32)p_buf + kdf_ipp_handle_pool.flg_num * sizeof(FLGPTN));
	CTL_IPP_DBG_TRC("alloc kdf_handle pool memory, num %d, flg_num %d, size 0x%.8x, addr 0x%.8x\r\n",
		(int)kdf_ipp_handle_pool.handle_num, (int)kdf_ipp_handle_pool.flg_num, (unsigned int)buf_size, (unsigned int)p_buf);

	/* reset & create free handle pool list */
	VOS_INIT_LIST_HEAD(&kdf_ipp_handle_pool.free_head);
	VOS_INIT_LIST_HEAD(&kdf_ipp_handle_pool.used_head);

	lock_val = KDF_IPP_HANDLE_FREE_FLAG_START;
	for (i = 0; i < kdf_ipp_handle_pool.handle_num; i++) {
		kdf_ipp_handle_reset(&kdf_ipp_handle_pool.handle[i]);
		kdf_ipp_handle_pool.handle[i].lock = lock_val;
		vos_list_add_tail(&kdf_ipp_handle_pool.handle[i].list, &kdf_ipp_handle_pool.free_head);
		lock_val += 1;
	}

	/* reset handle flag */
	for (i = 0; i < kdf_ipp_handle_pool.flg_num; i++) {
		OS_CONFIG_FLAG(kdf_ipp_handle_pool.flg_id[i]);
		if (i == 0) {
			vos_flag_set(kdf_ipp_handle_pool.flg_id[i], (FLGPTN_BIT_ALL & ~(KDF_IPP_HANDLE_POOL_LOCK)));
		} else {
			vos_flag_set(kdf_ipp_handle_pool.flg_id[i], FLGPTN_BIT_ALL);
		}
	}

	kdf_ipp_handle_pool_unlock();

	ctl_ipp_dbg_ctxbuf_log_set("KDF_IPP_HANDLE", CTL_IPP_DBG_CTX_BUF_ALLOC, buf_size, buf_addr, num);

	return buf_size;
}

static void kdf_ipp_handle_pool_free(void)
{
	UINT32 i;

	if (kdf_ipp_handle_pool.flg_id != NULL) {
		CTL_IPP_DBG_TRC("free ctl_handle memory, addr 0x%.8x\r\n", (unsigned int)kdf_ipp_handle_pool.flg_id);
		ctl_ipp_dbg_ctxbuf_log_set("KDF_IPP_HANDLE", CTL_IPP_DBG_CTX_BUF_FREE, 0, (UINT32)kdf_ipp_handle_pool.flg_id, 0);

		for (i = 0; i < kdf_ipp_handle_pool.flg_num; i++) {
			rel_flg(kdf_ipp_handle_pool.flg_id[i]);
		}
		kdf_ipp_handle_pool.flg_num = 0;
		kdf_ipp_handle_pool.handle_num = 0;
		kdf_ipp_handle_pool.flg_id = NULL;
		kdf_ipp_handle_pool.handle = NULL;
	}
}

UINT32 kdf_ipp_init(UINT32 num, UINT32 buf_addr, UINT32 is_query)
{
	UINT32 st_addr;

	if (is_query == TRUE) {
		buf_addr = 0;
	} else {
		/* install id and open task */
		kdf_ipp_install_id();
		if (kdf_ipp_open_tsk() != E_OK) {
			CTL_IPP_DBG_ERR("Open kdf ipp task failed\r\n");
			return (0xFFFFFFFF - buf_addr);
		}
	}

	/* assign buffer */
	st_addr = buf_addr;
	buf_addr += kdf_ipp_handle_pool_init(num, buf_addr, is_query);
	buf_addr += kdf_ipp_kdrv_wrapper_init(num, buf_addr, is_query);

	/* kdriver init */
	#if KDF_IPP_INIT_KDRV_FLOW_ENABLE
	{
		UINT32 kdrv_buf_size;
		CTL_IPP_DBG_CTX_BUF_OP ctx_log_op;

		ctx_log_op = (is_query == TRUE) ? CTL_IPP_DBG_CTX_BUF_QUERY : CTL_IPP_DBG_CTX_BUF_ALLOC;

		kdrv_buf_size = (is_query == TRUE) ? kdrv_ife_buf_query(num) : kdrv_ife_buf_init(buf_addr, num);
		ctl_ipp_dbg_ctxbuf_log_set("KDRV_IFE", ctx_log_op, kdrv_buf_size, buf_addr, num);
		buf_addr += kdrv_buf_size;

		kdrv_buf_size = (is_query == TRUE) ? kdrv_dce_buf_query(num) : kdrv_dce_buf_init(buf_addr, num);
		ctl_ipp_dbg_ctxbuf_log_set("KDRV_DCE", ctx_log_op, kdrv_buf_size, buf_addr, num);
		buf_addr += kdrv_buf_size;

		kdrv_buf_size = (is_query == TRUE) ? kdrv_ipe_buf_query(num) : kdrv_ipe_buf_init(buf_addr, num);
		ctl_ipp_dbg_ctxbuf_log_set("KDRV_IPE", ctx_log_op, kdrv_buf_size, buf_addr, num);
		buf_addr += kdrv_buf_size;

		/* ime need to alloc num+1 for graphic used */
		kdrv_buf_size = (is_query == TRUE) ? kdrv_ime_buf_query(num + 1) : kdrv_ime_buf_init(buf_addr, (num + 1));
		ctl_ipp_dbg_ctxbuf_log_set("KDRV_IME", ctx_log_op, kdrv_buf_size, buf_addr, (num + 1));
		buf_addr += kdrv_buf_size;

		kdrv_buf_size = (is_query == TRUE) ? kdrv_ife2_buf_query(num) : kdrv_ife2_buf_init(buf_addr, num);
		ctl_ipp_dbg_ctxbuf_log_set("KDRV_IFE2", ctx_log_op, kdrv_buf_size, buf_addr, num);
		buf_addr += kdrv_buf_size;
	}
	#endif

	return (buf_addr - st_addr);
}

void kdf_ipp_uninit(void)
{
	kdf_ipp_handle_pool_free();
	kdf_ipp_kdrv_wrapper_uninit();

	#if KDF_IPP_INIT_KDRV_FLOW_ENABLE
	kdrv_ife_buf_uninit();
	kdrv_dce_buf_uninit();
	kdrv_ipe_buf_uninit();
	kdrv_ime_buf_uninit();
	kdrv_ife2_buf_uninit();
	#endif

	/* close task and uninstall id */
	kdf_ipp_close_tsk();
	kdf_ipp_uninstall_id();
}

UINT32 kdf_ipp_open(CHAR *name, KDF_IPP_FLOW_TYPE flow)
{
	UINT32 rt;
	UINT32 handle = 0;
	KDF_IPP_HANDLE *p_hdl;

	kdf_ipp_handle_pool_lock();

	p_hdl = kdf_ipp_handle_alloc();
	if (p_hdl != NULL) {
		if (kdf_ipp_handle_lock(p_hdl->lock, TRUE) != CTL_IPP_E_OK) {
			kdf_ipp_handle_release(p_hdl);
			kdf_ipp_handle_pool_unlock();
			return 0;
		}

		/* reset handle then set new status */
		kdf_ipp_handle_reset(p_hdl);
		if (name != NULL) {
			strncpy(p_hdl->name, name, (KDF_IPP_HANDLE_NAME_MAX - 1));
		}
		p_hdl->sts = KDF_IPP_HANDLE_STS_READY;
		p_hdl->flow = flow;

		rt = kdf_ipp_open_kdrv(p_hdl);

		kdf_ipp_handle_unlock(p_hdl->lock);

		if (rt != CTL_IPP_E_OK) {
			kdf_ipp_close_kdrv(p_hdl);
			kdf_ipp_handle_release(p_hdl);
			handle = 0;
		} else {
			handle = (UINT32)p_hdl;
		}
	}

	kdf_ipp_handle_pool_unlock();
	return handle;
}

INT32 kdf_ipp_close(UINT32 hdl)
{
	ER rt;
	KDF_IPP_HANDLE *p_hdl;

	p_hdl = (KDF_IPP_HANDLE *)hdl;
	if (kdf_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		CTL_IPP_DBG_ERR("close fail(0x%.8x)\r\n", (unsigned int)hdl);
		return CTL_IPP_E_ID;
	}

	rt = kdf_ipp_set_pause(TRUE, FALSE);
	if (rt != E_OK) {
		/* reset engine */
		CTL_IPP_DBG_ERR("pause failed\r\n");
		return CTL_IPP_E_SYS;
	}

	if (kdf_ipp_handle_lock(p_hdl->lock, TRUE) != CTL_IPP_E_OK) {
		kdf_ipp_set_resume(FALSE);
		return CTL_IPP_E_TIMEOUT;
	}

	if (p_hdl->sts == KDF_IPP_HANDLE_STS_FREE) {
		CTL_IPP_DBG_ERR("illegal handle(0x%.8x), status(0x%.8x)\r\n", (unsigned int)hdl, (unsigned int)p_hdl->sts);
		kdf_ipp_handle_unlock(p_hdl->lock);
		kdf_ipp_set_resume(FALSE);
		return CTL_IPP_E_STATE;
	}

	kdf_ipp_erase_queue(hdl);

	kdf_ipp_handle_pool_lock();

	kdf_ipp_close_kdrv(p_hdl);
	kdf_ipp_handle_unlock(p_hdl->lock);
	kdf_ipp_handle_release(p_hdl);

	kdf_ipp_handle_pool_unlock();

	kdf_ipp_set_resume(FALSE);
	return CTL_IPP_E_OK;
}

#if 0
#endif

static INT32 kdf_ipp_ioctl_sndevt(KDF_IPP_HANDLE *p_hdl, void *data)
{
	ER rt;
	KDF_IPP_EVT *p_evt;

	p_evt = (KDF_IPP_EVT *)data;
	rt = kdf_ipp_msg_snd(KDF_IPP_MSG_PROCESS, (UINT32)p_hdl, p_evt->data_addr, p_evt->rev, p_hdl->cb_fp[KDF_IPP_CBEVT_DROP], p_evt->count);

	if (rt != E_OK) {
		CTL_IPP_DBG_WRN("send event failed\r\n");
		return CTL_IPP_E_QOVR;
	}

	p_hdl->snd_evt_cnt += 1;

	return CTL_IPP_E_OK;
}

static INT32 kdf_ipp_ioctl_direct_start(KDF_IPP_HANDLE *p_hdl, void *data)
{
	INT32 rt;
	KDF_IPP_EVT *p_evt;

	rt = CTL_IPP_E_OK;
	p_evt = (KDF_IPP_EVT *)data;

	rt = kdf_ipp_process_direct(p_hdl, p_evt->data_addr, KDF_IPP_DIRECT_MSG_START);
	if (rt == CTL_IPP_E_QOVR) {
		CTL_IPP_DBG_WRN("Drop\r\n");
	}

	return rt;
}

static INT32 kdf_ipp_ioctl_direct_stop(KDF_IPP_HANDLE *p_hdl, void *data)
{
	INT32 rt;
	KDF_IPP_EVT *p_evt;

	rt = CTL_IPP_E_OK;
	p_evt = (KDF_IPP_EVT *)data;

	kdf_ipp_set_proc_sts(&p_hdl->proc_sts, KDF_IPP_FRAME_BP, TRUE);
	rt = kdf_ipp_process_direct(p_hdl, p_evt->data_addr, KDF_IPP_DIRECT_MSG_STOP);
	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_WRN("stop fail\r\n");
	}

	return rt;
}

static INT32 kdf_ipp_ioctl_direct_cfg(KDF_IPP_HANDLE *p_hdl, void *data)
{
	INT32 rt;
	KDF_IPP_EVT *p_evt;

	rt = CTL_IPP_E_OK;
	p_evt = (KDF_IPP_EVT *)data;

	kdf_ipp_set_proc_sts(&p_hdl->proc_sts, KDF_IPP_FRAME_BP, TRUE);
	rt = kdf_ipp_process_direct(p_hdl, p_evt->data_addr, KDF_IPP_DIRECT_MSG_CFG);
	if (rt == CTL_IPP_E_QOVR) {
		CTL_IPP_DBG_WRN("Drop\r\n");
	}

	return rt;
}

static INT32 kdf_ipp_ioctl_direct_oneshot(KDF_IPP_HANDLE *p_hdl, void *data)
{
	/* TBD */
	return CTL_IPP_E_OK;
}

static INT32 kdf_ipp_ioctl_direct_wakeup(KDF_IPP_HANDLE *p_hdl, void *data)
{
	INT32 rt;

	rt = kdf_ipp_open_kdrv(p_hdl);
	if (rt != CTL_IPP_E_OK) {
		CTL_IPP_DBG_WRN("[kdf] direct wake up fail \r\n");
	}

	return rt;
}

static INT32 kdf_ipp_ioctl_direct_sleep(KDF_IPP_HANDLE *p_hdl, void *data)
{
	INT32 rt;

	rt = CTL_IPP_E_OK;
	kdf_ipp_close_kdrv(p_hdl);

	return rt;
}

static INT32 kdf_ipp_ioctl_free_que_num(KDF_IPP_HANDLE *p_hdl, void *data)
{
	UINT32 que_num;

	que_num = kdf_ipp_get_free_queue_num();
	*(UINT32 *)data = que_num;

	return CTL_IPP_E_OK;
}

static INT32 kdf_ipp_ioctl_sndevt_pattern_paste(KDF_IPP_HANDLE *p_hdl, void *data)
{
	ER rt;
	KDF_IPP_EVT *p_evt;

	p_evt = (KDF_IPP_EVT *)data;
	rt = kdf_ipp_msg_snd(KDF_IPP_MSG_PROCESS_PATTERN_PASTE, (UINT32)p_hdl, p_evt->data_addr, p_evt->rev, p_hdl->cb_fp[KDF_IPP_CBEVT_DROP], p_evt->count);

	if (rt != E_OK) {
		CTL_IPP_DBG_WRN("send event failed\r\n");
		return CTL_IPP_E_QOVR;
	}

	p_hdl->snd_evt_cnt += 1;

	return CTL_IPP_E_OK;
}

static KDF_IPP_IOCTL_FP kdf_ipp_ioctl_tab[KDF_IPP_IOCTL_MAX] = {
	kdf_ipp_ioctl_sndevt,
	kdf_ipp_ioctl_direct_start,
	kdf_ipp_ioctl_direct_stop,
	kdf_ipp_ioctl_direct_cfg,
	kdf_ipp_ioctl_direct_oneshot,
	kdf_ipp_ioctl_free_que_num,
	kdf_ipp_ioctl_direct_wakeup,
	kdf_ipp_ioctl_direct_sleep,
	kdf_ipp_ioctl_sndevt_pattern_paste,
};

INT32 kdf_ipp_ioctl(UINT32 hdl, UINT32 item, void *data)
{
	KDF_IPP_HANDLE *p_hdl;
	INT32 rt = CTL_IPP_E_OK;

	/* process ioctl funciton */
	p_hdl = (KDF_IPP_HANDLE *)hdl;

	if (kdf_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		goto err;
	}

	if (item >= KDF_IPP_IOCTL_MAX) {
		goto err;
	}

	if (item == KDF_IPP_IOCTL_DIRECT_START || item == KDF_IPP_IOCTL_DIRECT_STOP || item == KDF_IPP_IOCTL_DIRECT_CFG) {
		p_hdl->snd_evt_time = ctl_ipp_util_get_syst_counter();
	}

	if (kdf_ipp_ioctl_tab[item] != NULL) {
		rt = kdf_ipp_ioctl_tab[item](p_hdl, data);
	}

err:
	if (p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
		p_hdl->snd_evt_cnt += 1;
		if (item == KDF_IPP_IOCTL_DIRECT_START || item == KDF_IPP_IOCTL_DIRECT_STOP || item == KDF_IPP_IOCTL_DIRECT_CFG) {
			if (rt != CTL_IPP_E_OK &&  rt != CTL_IPP_E_DIR_DROP) {
				KDF_IPP_EVT *p_evt;
				p_evt = (KDF_IPP_EVT *)data;
				kdf_ipp_drop(hdl, p_evt->data_addr, p_evt->rev, p_hdl->cb_fp[KDF_IPP_CBEVT_DROP], rt);
			}
		}
	}

	return rt;
}

#if 0
#endif

static INT32 kdf_ipp_set_reg_cb(KDF_IPP_HANDLE *p_hdl, void *data)
{
	KDF_IPP_REG_CB_INFO *p_data = (KDF_IPP_REG_CB_INFO *)data;

	if (p_data->cbevt >= KDF_IPP_CBEVT_MAX) {
		CTL_IPP_DBG_WRN("Illegal cbevt %d\r\n", (int)(p_data->cbevt));
		return CTL_IPP_E_PAR;
	}

	if (p_data->fp == NULL) {
		CTL_IPP_DBG_WRN("Null function pointer\r\n");
		return CTL_IPP_E_PAR;
	}

	p_hdl->cb_fp[p_data->cbevt] = p_data->fp;

	return CTL_IPP_E_OK;
}

static INT32 kdf_ipp_set_flush(KDF_IPP_HANDLE *p_hdl, void *data)
{
	KDF_IPP_FLUSH_CONFIG *p_cfg;
	UINT32 i;

	p_cfg = (KDF_IPP_FLUSH_CONFIG *)data;
	if (p_cfg == NULL) {
		kdf_ipp_erase_queue((UINT32)p_hdl);
	} else {
		kdf_ipp_disable_path_dma_in_queue((UINT32)p_hdl, p_cfg->pid);
	}

	/* direct mode should flush data in process when pause */
	if(p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
		for (i = 0; i < DATA_IN_NUM; i++) {
			if (p_hdl->data_addr_in_process[i] != 0 ) {
				if(p_hdl->dir_load_bit & KDF_IPP_DIR_LOAD_PAUSE_E) {
						kdf_ipp_drop((UINT32) p_hdl, p_hdl->data_addr_in_process[i], 0, p_hdl->cb_fp[KDF_IPP_CBEVT_DROP], CTL_IPP_E_FLUSH);
					    p_hdl->data_addr_in_process[i] = 0;
				}
			}
		}
	}

	return CTL_IPP_E_OK;
}

static INT32 kdf_ipp_set_first_handle(KDF_IPP_HANDLE *p_hdl, void *data)
{
	p_hdl->is_first_handle = TRUE;

	/* send event for fastboot exist for d2d mode */
	#if defined (__KERNEL__)
	if (kdrv_builtin_is_fastboot() && p_hdl->flow != KDF_IPP_FLOW_DIRECT_RAW) {
		if (kdf_ipp_msg_snd(KDF_IPP_MSG_BUILTIN_EXIT, 0, 0, 0, NULL, 0) != E_OK) {
			CTL_IPP_DBG_ERR("snd hwrst event failed\r\n");
		}
	}
	#endif

	return CTL_IPP_E_OK;
}

static KDF_IPP_SET_FP kdf_ipp_set_tab[KDF_IPP_ITEM_MAX] = {
	NULL,
	kdf_ipp_set_reg_cb,
	kdf_ipp_set_flush,
	NULL,
	kdf_ipp_set_first_handle,
};

INT32 kdf_ipp_set(UINT32 hdl, UINT32 item, void *data)
{
	KDF_IPP_HANDLE *p_hdl;
	/* process set funciton */
	INT32 rt = CTL_IPP_E_OK;

	p_hdl = (KDF_IPP_HANDLE *)hdl;

	if (kdf_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		return CTL_IPP_E_ID;
	}

	if (item >= KDF_IPP_ITEM_MAX) {
		return CTL_IPP_E_PAR;
	}

	if (kdf_ipp_set_tab[item] != NULL) {
		if (kdf_ipp_handle_lock(p_hdl->lock, TRUE) != CTL_IPP_E_OK) {
			return CTL_IPP_E_TIMEOUT;
		}

		if (p_hdl->sts == KDF_IPP_HANDLE_STS_FREE) {
			CTL_IPP_DBG_WRN("illegal handle(0x%.8x), status(0x%.8x)\r\n", (unsigned int)hdl, (unsigned int)p_hdl->sts);
			kdf_ipp_handle_unlock(p_hdl->lock);
			return CTL_IPP_E_STATE;
		}
		rt = kdf_ipp_set_tab[item](p_hdl, data);

		/* incase flush lock failed */
		if (rt != CTL_IPP_E_TIMEOUT) {
			kdf_ipp_handle_unlock(p_hdl->lock);
		}
	}

	return rt;
}

#if 0
#endif

static INT32 kdf_ipp_get_flow(KDF_IPP_HANDLE *p_hdl, void *data)
{
	*(KDF_IPP_FLOW_TYPE *)data = p_hdl->flow;

	return CTL_IPP_E_OK;
}

static INT32 kdf_ipp_get_stripe_num(KDF_IPP_HANDLE *p_hdl, void *data)
{
	KDRV_DCE_IN_INFO dce_in_info = {0};
	KDRV_DCE_IN_IMG_INFO dce_in_img = {0};
	KDRV_DCE_STRP_RULE_INFO dce_strp_rule = {0};
	KDRV_DCE_STRIPE_NUM dce_strp_num = {0};
	KDF_IPP_STRIPE_NUM_INFO *p_info;
	INT32 rt = CTL_IPP_E_OK;

	p_info = (KDF_IPP_STRIPE_NUM_INFO *)data;

	/* only set minimum information for dce to cal stripe */
	if (p_hdl->flow == KDF_IPP_FLOW_DIRECT_RAW) {
		dce_in_info.in_src = KDRV_DCE_IN_SRC_ALL_DIRECT;
	} else if (p_hdl->flow == KDF_IPP_FLOW_RAW) {
		dce_in_info.in_src = KDRV_DCE_IN_SRC_DIRECT;
	} else if (p_hdl->flow == KDF_IPP_FLOW_CCIR) {
		dce_in_info.in_src = KDRV_DCE_IN_SRC_DCE_IME;
	} else if (p_hdl->flow == KDF_IPP_FLOW_DCE_D2D){
		dce_in_info.in_src = KDRV_DCE_IN_SRC_DRAM;
	} else {
		CTL_IPP_DBG_WRN("Unsupport flow(%d) to cal stripe number\r\n", p_hdl->flow);
		return CTL_IPP_E_PAR;
	}

	dce_in_img.img_size_h = p_info->in_size.w;
	dce_in_img.img_size_v = p_info->in_size.h;
	dce_in_img.lineofst[KDRV_DCE_YUV_Y] = p_info->in_size.w;
	dce_in_img.lineofst[KDRV_DCE_YUV_UV] = p_info->in_size.w;

	dce_strp_rule.dce_strp_rule = (p_info->gdc_enable) ? KDRV_DCE_STRP_GDC_HIGH_PRIOR : KDRV_DCE_STRP_LOW_LAT_HIGH_PRIOR;

	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IN_INFO, (void *)&dce_in_info);
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_IN_IMG, (void *)&dce_in_img);
	rt |= kdf_ipp_kdrv_set_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_STRIPE_RULE, (void *)&dce_strp_rule);

	if (rt != E_OK) {
		CTL_IPP_DBG_ERR("set dce for cal stripe number failed\r\n");
		return CTL_IPP_E_KDRV_SET;
	}

	rt |= kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_DCE, p_hdl, KDRV_DCE_PARAM_IPL_STRIPE_NUM, (void *)&dce_strp_num);
	if (rt != E_OK) {
		CTL_IPP_DBG_ERR("dce cal stripe number failed\r\n");
		return CTL_IPP_E_KDRV_GET;
	}

	p_info->stripe_num = dce_strp_num.num;

	return CTL_IPP_E_OK;
}

static KDF_IPP_GET_FP kdf_ipp_get_tab[KDF_IPP_ITEM_MAX] = {
	kdf_ipp_get_flow,
	NULL,
	NULL,
	kdf_ipp_get_stripe_num,
	NULL,
};

INT32 kdf_ipp_get(UINT32 hdl, UINT32 item, void *data)
{
	KDF_IPP_HANDLE *p_hdl;
	/* process set funciton */
	UINT32 rt = CTL_IPP_E_OK;

	p_hdl = (KDF_IPP_HANDLE *)hdl;

	if (kdf_ipp_handle_validate(p_hdl) != CTL_IPP_E_OK) {
		return CTL_IPP_E_ID;
	}

	if (item >= KDF_IPP_ITEM_MAX) {
		return CTL_IPP_E_PAR;
	}

	if (kdf_ipp_get_tab[item] != NULL) {
		if (kdf_ipp_handle_lock(p_hdl->lock, TRUE) != CTL_IPP_E_OK) {
			return CTL_IPP_E_TIMEOUT;
		}

		if (p_hdl->sts == KDF_IPP_HANDLE_STS_FREE) {
			CTL_IPP_DBG_WRN("illegal handle(0x%.8x), status(0x%.8x)\r\n", (unsigned int)hdl, (unsigned int)p_hdl->sts);
			kdf_ipp_handle_unlock(p_hdl->lock);
			return CTL_IPP_E_STATE;
		}
		rt = kdf_ipp_get_tab[item](p_hdl, data);
		kdf_ipp_handle_unlock(p_hdl->lock);
	}

	return rt;
}

