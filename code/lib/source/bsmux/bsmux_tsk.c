/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

// INCLUDE PROTECTED
#include "bsmux_init.h"

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static BSMUX_TSK_CTRL_OBJ g_bsmux_tsk_ctrl_obj[BSMUX_MAX_CTRL_NUM] = {0};
static UINT32             g_bsmux_tsk_to_idle[BSMUX_MAX_CTRL_NUM] = {0};
static UINT32             g_bsmux_tsk_to_cutfile[BSMUX_MAX_CTRL_NUM] = {0};
static UINT32             g_bsmux_tsk_keep_still[BSMUX_MAX_CTRL_NUM] = {0};
static UINT32             g_bsmux_tsk_pick_count[BSMUX_MAX_CTRL_NUM] = {0};
static UINT32             g_bsmux_tsk_still_us = 50;
static UINT32             g_debug_tskinfo = BSMUX_DEBUG_TSKINFO;
static UINT32             g_debug_action = BSMUX_DEBUG_ACTION;
static UINT32             g_bsmux_tsk_last_qidx = 0;
#define BSMUX_TSK_FUNC(fmtstr, args...) do { if(g_debug_tskinfo) DBG_DUMP(fmtstr, ##args); } while(0)
#define BSMUX_ACT_FUNC(fmtstr, args...) do { if(g_debug_action) DBG_DUMP(fmtstr, ##args); } while(0)
extern BOOL               g_bsmux_debug_msg;
#define BSMUX_MSG(fmtstr, args...) do { if(g_bsmux_debug_msg) DBG_DUMP(fmtstr, ##args); } while(0)

/*-----------------------------------------------------------------------------*/
/* Internal Functions                                                          */
/*-----------------------------------------------------------------------------*/
//mapping => NMR_BsMuxer_GetObj, NMediaRecTS_GetObj
static BSMUX_TSK_CTRL_OBJ *bsmux_tsk_get_ctrl_obj(UINT32 id)
{
	return &g_bsmux_tsk_ctrl_obj[id];
}

static UINT32 bsmux_tsk_set_last_idx(UINT32 last_idx)
{
	g_bsmux_tsk_last_qidx = last_idx;
	return 0;
}

static UINT32 bsmux_tsk_get_last_idx(UINT32 *last_idx)
{
	*last_idx = g_bsmux_tsk_last_qidx;
	return 0;
}

static ER bsmux_tsk_chk_pick_idx(UINT32 pick_idx)
{
	UINT32 value = 0;
	bsmux_util_get_param(pick_idx, BSMUXER_PARAM_VID_GOP, &value); //based on gop
	if (g_bsmux_tsk_pick_count[pick_idx] >= value) {
		DBG_IND("pick_count=%d\r\n", g_bsmux_tsk_pick_count[pick_idx]);
		g_bsmux_tsk_pick_count[pick_idx] = 0;
		return E_SYS;
	}
	g_bsmux_tsk_pick_count[pick_idx]++;
	return E_OK;
}

static ER bsmux_tsk_pick_idx(UINT32 last_idx, UINT32 *pick_idx)
{
	UINT32 cur_index;
	UINT32 cur_idx;
	UINT32 ret_idx = 0;
	for (cur_index = 1; cur_index <= BSMUX_MAX_CTRL_NUM; cur_index++) {
		cur_idx = last_idx + cur_index;
		if (cur_idx >= BSMUX_MAX_CTRL_NUM) {
			cur_idx -= BSMUX_MAX_CTRL_NUM;
		}
		if (bsmux_ctrl_bs_getnum(cur_idx, 1) > 0) {
			if (bsmux_ctrl_bs_active(cur_idx, 1) == 0) {
				DBG_ERR("[%d] not active but num=%d\r\n", cur_idx, bsmux_ctrl_bs_getnum(cur_idx, 1));
			} else {
				ret_idx = cur_idx;
				DBG_IND("pick_idx %d\r\n", ret_idx);
				*pick_idx = ret_idx;
				return bsmux_tsk_chk_pick_idx(ret_idx);
			}
		}
	}
	return E_SYS;
}

static ER bsmux_tsk_check_pause(UINT32 id)
{
	UINT32 pause_id, pause_on, pause_cnt;
	UINT32 vfr, gop, keepsec;

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_PAUSE_ID, &pause_id);
	bsmux_util_get_param(pause_id, BSMUXER_PARAM_FILE_PAUSE_ON, &pause_on);

	if ((pause_id != id) && (pause_on)) {
		bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_ID, id);
		bsmux_util_set_param(pause_id, BSMUXER_PARAM_FILE_PAUSE_ON, 0);

		bsmux_util_get_param(pause_id, BSMUXER_PARAM_FILE_PAUSE_CNT, &pause_cnt);
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_GOP, &gop);
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_KEEPSEC, &keepsec);
		pause_cnt = ((keepsec * (vfr / gop)) << 16) | pause_cnt;
		BSMUX_MSG("[%d]pause_cnt(0x%lx,%d/%d/%d)\r\n", (int)id, (unsigned long)pause_cnt, (int)keepsec, (int)vfr, (int)gop);
		bsmux_util_set_param(pause_id, BSMUXER_PARAM_FILE_PAUSE_CNT, pause_cnt);

		bsmux_tsk_status_resume(pause_id);
	}

	return E_OK;
}

static ER bsmux_tsk_handle_loop(UINT32 id)
{
	UINT32 emron = 0, emrloop = 0, strgbuf = 0;

	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emron);
	bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
	bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &strgbuf);

	if (strgbuf) {
		bsmux_util_set_minfo(MEDIAWRITE_SETINFO_INITENTRYPOS, 0, 0, id);
		DBG_IND("[%d] INIT ENTRY POS\r\n", id);
	}

	if (emron && emrloop) { // emrloop is set
		if (emrloop == BSMUX_STEP_EMRREC) {
			emrloop = BSMUX_STEP_EMRLOOP;
			DBG_MSG("[%d] set emrloop as %d to waiting run\r\n", id, emrloop);
			bsmux_util_set_param(id, BSMUXER_PARAM_FILE_EMRLOOP, emrloop);
		} else {
			DBG_MSG("[%d] emrloop as %d to waiting run\r\n", id, emrloop);
		}
	} else {
		if (emron) {
			DBG_MSG("[%d] emr on to waiting hit\r\n", id);
		} else {
			DBG_MSG("[%d] emr off to waiting run\r\n", id);
		}
	}

	return E_OK;
}

static UINT32 bsmux_tsk_status_is_idle(UINT32 id)
{
	BSMUX_TSK_CTRL_OBJ *p_obj = NULL;
	UINT32 status = 0;
	UINT32 ret;

	p_obj = bsmux_tsk_get_ctrl_obj(id);

	bsmux_status_lock();
	status = p_obj->Status;
	bsmux_status_unlock();

	if (status == BSMUX_STATUS_IDLE) {
		ret = 1;
	} else {
		ret = 0;
	}

	return ret;
}

static UINT32 bsmux_tsk_chk_action(UINT32 id, UINT32 type)
{
	BSMUX_TSK_CTRL_OBJ *p_obj = NULL;
	UINT32 emr_on = 0, overlap = 0, pause_on = 0, emrloop = 0;
	UINT32 vfn = 0, afn = 0;
	UINT32 sub_vfn = 0, sub_vfr = 0, sub_stopVF = 0;
	UINT32 drop_vfn = 0, drop_sub_vfn = 0;
	UINT32 vfr = 0, stopVF = 0;
	UINT32 recovery = 0;
	UINT32 now_addr = 0;
	UINT32 flush_freq = 0, wrblk_count = 0, frontPut = 0, blkPut = 0;
	UINT32 flush_tune = 0;
	#if 1	// support meta-sync
	UINT32 meta_on = 0, meta_num = 0, meta_cnt = 0, meta_sync_cnt = 0;
	#endif	// support meta-sync
	UINT32 action_flag = 0;
	UINT32 ret;

	p_obj = bsmux_tsk_get_ctrl_obj(id);

	if (!(p_obj->Action & type)) {
		return 0;
	}

	switch (type) {
	case BSMUX_ACTION_OVERLAP_1SEC:
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_OVERLAP_ON, &overlap);
		if (overlap) {
			BSMUX_ACT_FUNC("BSM:[%d] OVERLAP_1SEC\r\n", id);
			ret = 1;
		} else {
			ret = 0;
		}
		break;
	case BSMUX_ACTION_REORDER_1SEC:
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &drop_vfn);
		if (drop_vfn) {
			vfn += drop_vfn;
		}
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &drop_sub_vfn);
		if (drop_sub_vfn) {
			sub_vfn += drop_sub_vfn;
		}
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
		if ((vfr != 0) && (sub_vfr != 0)) {
			if (vfn != sub_vfn) {
				BSMUX_ACT_FUNC("BSM:[%d] REORDER_1SEC (main=%d)(sub=%d)\r\n", id, vfn, sub_vfn);
				ret = 1;
			} else {
				ret = 0;
			}
		} else {
			ret = 0;
		}
		break;
	case BSMUX_ACTION_WAITING_HIT:
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emr_on);
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_PAUSE_ON, &pause_on);
		if (emr_on) {
			if (emrloop) {
				ret = (emrloop != BSMUX_STEP_EMRLOOP);
			} else {
				ret = 1;
			}
		} else if (pause_on) {
			ret = 1;
		} else {
			ret = 0;
		}
		break;
	case BSMUX_ACTION_WAITING_RUN:
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRON, &emr_on);
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_EMRLOOP, &emrloop);
		bsmux_util_get_param(id, BSMUXER_PARAM_FILE_PAUSE_ON, &pause_on);
		if (emr_on) {
			if (emrloop) {
				ret = (emrloop == BSMUX_STEP_EMRLOOP);
			} else {
				ret = 0;
			}
		} else if (pause_on) {
			ret = 0;
		} else {
			ret = 1;
		}
		break;
	case BSMUX_ACTION_WAITING_IDLE:
		if (g_bsmux_tsk_to_idle[id]) {
			ret = 1;
		} else {
			ret = 0;
		}
		break;
	case BSMUX_ACTION_MAKE_HEADER:
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALAF, &afn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
		if ((vfn == 0) && (afn == 0)) {
			if (sub_vfn) {
				return 0;
			}
			BSMUX_MSG("BSM:[%d] MAKE_HEADER\r\n", id);
			return 1;
		} else {
			return 0;
		}
		break;
	case BSMUX_ACTION_UPDATE_HEADER:
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &drop_vfn);
		if (drop_vfn) {
			vfn += drop_vfn;
		}
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &drop_sub_vfn);
		if (drop_sub_vfn) {
			sub_vfn += drop_sub_vfn;
		}
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
		stopVF = bsmux_util_calc_frm_num(id, BSMUX_TYPE_VIDEO);
		sub_stopVF = bsmux_util_calc_frm_num(id, BSMUX_TYPE_SUBVD);
		if (sub_vfr) {
			if ((vfn == stopVF) && (sub_vfn >= sub_stopVF)) {
				BSMUX_ACT_FUNC("BSM:[%d] UPDATE_HEADER(main)\r\n", id);
				action_flag = 1;
			} else if ((sub_vfn == sub_stopVF) && (vfn >= stopVF)) {
				BSMUX_ACT_FUNC("BSM:[%d] UPDATE_HEADER(sub)\r\n", id);
				action_flag = 1;
			} else {
				action_flag = 0;
			}
		} else {
			if (vfn == stopVF) {
				action_flag = 1;
			}
		}
		if (!frontPut) {
			ret = 0;
		} else if ((action_flag) || (g_bsmux_tsk_to_idle[id]) || (g_bsmux_tsk_to_cutfile[id])) {
			BSMUX_MSG("BSM:[%d] UPDATE_HEADER (main=%d/%d)(sub=%d/%d)\r\n", id, drop_vfn, vfn, drop_sub_vfn, sub_vfn);
			ret = 1;
		} else {
			ret = 0;
		}
		break;
	case BSMUX_ACTION_MAKE_MOOV:
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_BLKPUT, &blkPut);
		bsmux_util_get_param(id, BSMUXER_PARAM_MOOV_FREQ, &flush_freq); //use moov setting as freq
		bsmux_util_get_param(id, BSMUXER_PARAM_MOOV_TUNE, &flush_tune);
		bsmux_util_get_param(id, BSMUXER_PARAM_WRINFO_WRBLK_CNT, &wrblk_count);
		if (!blkPut) {
			ret = 0;
		} else {
			if (flush_freq) {
				if ((wrblk_count % flush_freq) == 1) {
					ret = 1;
				} else {
					ret = 0;
				}
			} else {
				if ((wrblk_count % 5) == 1) {
					ret = 1;
				} else {
					ret = 0;
				}
			}
		}
		if (flush_tune) {
			UINT32 tbr = 0, blksize = 0, remain_cnt = 0;
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &drop_vfn);
			if (drop_vfn) {
				vfn += drop_vfn;
			}
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
			stopVF = bsmux_util_calc_frm_num(id, BSMUX_TYPE_VIDEO);
			bsmux_util_get_param(id, BSMUXER_PARAM_VID_TBR, &tbr);
			bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_WRITEBLOCK, &blksize);
			remain_cnt = (((stopVF/vfr) - (vfn/vfr)) * tbr) / blksize;
			if (ret == 1) {
				if (remain_cnt <= flush_freq) {
					ret = 0;
					DBG_IND("[%d] makefmoov(%d)(wrblk%d)remain%d) do nothing\r\n", id, (vfn/vfr), wrblk_count, remain_cnt);
				} else {
					DBG_IND("[%d] makefmoov(%d)(wrblk%d)(remain%d)\r\n", id, (vfn/vfr), wrblk_count, remain_cnt);
				}
			} else {
				DBG_IND("[%d] makefmoov(%d)(wrblk%d)(remain%d) do nothing\r\n", id, (vfn/vfr), wrblk_count, remain_cnt);
			}
		}
		#if BSMUX_TEST_MOOV_RC
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &drop_vfn);
		if (drop_vfn) {
			vfn += drop_vfn;
		}
		vfn = vfn - 1; // since CalcAlign round off
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
		{
			UINT32 flush_sync = 0;
			#if 0	//fast put flow ver3
			UINT32 fastput_en = 0;
			bsmux_util_get_param(id, BSMUXER_PARAM_EN_FASTPUT, &fastput_en);
			#endif	//fast put flow ver3
			if (flush_freq) {
				flush_sync = (frontPut) ? 1 : (((vfn / vfr) % flush_freq) + 1);
				if (vfn % vfr == 0) {
					if ((vfn / vfr) % flush_freq == flush_sync) {
						ret = 1;
					} else {
						ret = 0;
					}
					if ((vfn / vfr) == 1) {
						DBG_IND("[%d] not make moov since new frontput flow\r\n", id);
						ret = 0;
					}
				} else {
					ret = 0;
				}
				#if 0	//fast put flow ver3
				if (blkPut) {
					if (fastput_en && wrblk_count == 2) {
						DBG_IND("[%d] make moov since fastput flow\r\n", id);
						ret = 1;
					}
				}
				#endif	//fast put flow ver3
			} else {
				flush_sync = (frontPut) ? 1 : (((vfn / vfr) % 5) + 1);
				if (vfn % vfr == 0) {
					if ((vfn / vfr) % 5 == flush_sync) {
						ret = 1;
					} else {
						ret = 0;
					}
					if ((vfn / vfr) == 1) {
						DBG_IND("[%d] not make moov since new frontput flow\r\n", id);
						ret = 0;
					}
				} else {
					ret = 0;
				}
				#if 0	//fast put flow ver3
				if (blkPut) {
					if (fastput_en && wrblk_count == 2) {
						DBG_IND("[%d] make moov since fastput flow\r\n", id);
						ret = 1;
					}
				}
				#endif	//fast put flow ver3
			}
		}
		if (flush_tune) {
			UINT32 remain_cnt = 0;
			stopVF = bsmux_util_calc_frm_num(id, BSMUX_TYPE_VIDEO);
			remain_cnt = (stopVF/vfr) - (vfn/vfr);
			if (ret == 1) {
				if (remain_cnt <= flush_freq || (vfn/vfr) <= flush_freq) {
					ret = 0;
					DBG_DUMP("[%d] makefmoov(%d)remain%d) do nothing\r\n", id, (vfn/vfr), remain_cnt);
				} else {
					DBG_DUMP("[%d] makefmoov(%d)(remain%d)\r\n", id, (vfn/vfr), remain_cnt);
				}
			} else {
				DBG_IND("[%d] makefmoov(%d)(remain%d) do nothing\r\n", id, (vfn/vfr), remain_cnt);
			}

			bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
			vfn = vfn - 1; // since CalcAlign round off
			if ((vfn == 1)) {
				BSMUX_MSG("[%d] makefmoov(%d)(with header)\r\n", id, (vfn/vfr));
				ret = 1;
			}
		}
		#endif
		break;
	case BSMUX_ACTION_CUTFILE:
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF_DROP, &drop_vfn);
		if (drop_vfn) {
			vfn += drop_vfn;
		}
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF, &sub_vfn);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_SUBVF_DROP, &drop_sub_vfn);
		if (drop_sub_vfn) {
			sub_vfn += drop_sub_vfn;
		}
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_SUB_VFR, &sub_vfr);
		stopVF = bsmux_util_calc_frm_num(id, BSMUX_TYPE_VIDEO);
		sub_stopVF = bsmux_util_calc_frm_num(id, BSMUX_TYPE_SUBVD);
		if (sub_vfr) {
			if ((vfn % stopVF == 0) && (vfn != 0) && (sub_vfn >= sub_stopVF)) {
				BSMUX_ACT_FUNC("BSM:[%d] CUTFILE(main)\r\n", id);
				action_flag = 1;
			} else if ((sub_vfn % sub_stopVF == 0) && (sub_vfn != 0) && (vfn >= stopVF)) {
				BSMUX_ACT_FUNC("BSM:[%d] CUTFILE(sub)\r\n", id);
				action_flag = 1;
			} else {
				action_flag = 0;
			}
		} else {
			if ((vfn % stopVF == 0) && (vfn != 0)) {
				action_flag = 1;
			}
		}
		#if 1	// support meta-sync
		bsmux_util_get_param(id, BSMUXER_PARAM_META_ON, &meta_on);
		bsmux_util_get_param(id, BSMUXER_PARAM_META_NUM, &meta_num);
		if (meta_on && action_flag) { // video frame should be followed by meta data
			UINT32 uibufid;
			for (uibufid = 0; uibufid < meta_num; uibufid++) {
				UINT32 fileinfo_type = BSMUX_FILEINFO_TOTALMETA + uibufid;
				bsmux_ctrl_bs_get_fileinfo(id, fileinfo_type, &meta_cnt);
				meta_sync_cnt = bsmux_util_calc_frm_num(id, BSMUX_TYPE_VIDEO); //temp as vfr
				if ((meta_cnt % meta_sync_cnt == 0) && (meta_cnt != 0)) {
					action_flag = 1;
					BSMUX_MSG("[%d][%d][count=%d][sync=%d][MATCH!]\r\n", id, uibufid, meta_cnt, meta_sync_cnt);
				} else {
					action_flag = 0;
					BSMUX_MSG("[%d][%d][count=%d][sync=%d][BREAK!]\r\n", id, uibufid, meta_cnt, meta_sync_cnt);
					break;
				}
			}
		}
		#endif	// support meta-sync
		if (action_flag) {
			BSMUX_MSG("BSM:[%d] CUTFILE (main=%d/%d)(sub=%d/%d)\r\n", id, drop_vfn, vfn, drop_sub_vfn, sub_vfn);
			ret = 1;
		} else if ((vfn % vfr == 0) && (g_bsmux_tsk_to_cutfile[id])) {
			BSMUX_ACT_FUNC("BSM:[%d] CUTNOW %d\r\n", id, vfn);
			ret = 1;
		} else {
			ret = 0;
		}
		break;
	case BSMUX_ACTION_SAVE_ENTRY:
		ret = 1;
		break;
	case BSMUX_ACTION_PAD_NIDX:
		bsmux_util_get_param(id, BSMUXER_PARAM_NIDX_EN, &recovery);
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_TOTALVF, &vfn);
		bsmux_util_get_param(id, BSMUXER_PARAM_VID_VFR, &vfr);
		if ((vfn % vfr == 0) && (recovery != 0)) {
			ret = 1;
		} else {
			ret = 0;
		}
		break;
	case BSMUX_ACTION_MAKE_PES:
		ret = 1;
		break;
	case BSMUX_ACTION_MAKE_PAT:
		bsmux_ctrl_mem_get_bufinfo(id, BSMUX_CTRL_NOWADDR, &now_addr);
		if ((now_addr % 8) != 0) {
			ret = 1;
		} else {
			ret = 0;
		}
		break;
	case BSMUX_ACTION_MAKE_PMT:
		ret = 0;
		break;
	case BSMUX_ACTION_MAKE_PCR:
		ret = 0;
		break;
	case BSMUX_ACTION_ADDLAST:
		ret = 1;
		break;
	case BSMUX_ACTION_PUT_LAST:
		bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_FRONTPUT, &frontPut);
		if (!frontPut) {
			ret = 0;
		} else {
			ret = 1;
		}
		break;
	default:
		DBG_DUMP("type invalid %d\r\n", type);
		ret = 0;
	}
	return ret;
}

static ER bsmux_tsk_chk_status(UINT32 id)
{
	BSMUX_SAVEQ_BS_INFO cur_buf = {0};
	BSMUX_TSK_CTRL_OBJ *p_ctrl_obj = NULL;
	UINT32 status = 0;
	UINT32 bsqnum = 0, val = 0;

	if (bsmux_util_is_invalid_id(id)) {
		DBG_ERR("%d invalid\r\n", id);
		return E_ID;
	}

	p_ctrl_obj = bsmux_tsk_get_ctrl_obj(id);
	if (bsmux_util_is_null_obj((void *)p_ctrl_obj)) {
		DBG_ERR("get tsk ctrk obj null\r\n");
		return E_NOEXS;
	}

	BSMUX_TSK_FUNC("bsmux[%d] status %d action %d\r\n", id, p_ctrl_obj->Status, p_ctrl_obj->Action);

	bsmux_status_lock();
	status = p_ctrl_obj->Status;
	bsmux_status_unlock();


	//mapping
	//1. NMRBS_STATUS_WAITING_HIT
	//   = BSMUX_STATUS_IDLE and BSMUX_ACTION_WAITING_HIT
	//2. NMRBS_STATUS_IDLE
	//   = BSMUX_STATUS_IDLE
	//3. NMRBS_STATUS_ROLLBACK
	//   = BSMUX_STATUS_IDLE and BSMUX_ACTION_OVERLAP_1SEC
	//4. NMRBS_STATUS_NORMAL
	//   = BSMUX_STATUS_RUN
	//5. NMRBS_ACTION_SUSPEND
	//   = BSMUX_STATUS_SUSPEND
	//6. NMRBS_ACTION_CUTNOW ??
	//   = BSMUX_STATUS_SUSPEND ??
	//7. NMRBS_ACTION_RESUME
	//   = NMRBS_ACTION_RESUME ?? or no ??
	//8. NMRBS_ACTION_GETALL2END
	//   = BSMUX_STATUS_SUSPEND and ??

	switch (status) {

	case BSMUX_STATUS_IDLE:
		{
			BSMUX_TSK_FUNC("BSM:[%d] idle\r\n", id);

			bsqnum = bsmux_ctrl_bs_getnum(id, 1);
			if (bsqnum != 0) {
				BSMUX_TSK_FUNC("BSM:[%d] bsqnum %d. drop since tsk status idle.\r\n", bsqnum, id);
				bsmux_ctrl_bs_dequeue(id, (void *)&cur_buf);
				return E_OK;
			}

			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_WAITING_IDLE)) {
				BSMUX_TSK_FUNC("BSM:[%d] stop... already in [idle]\r\n", id);
				return E_OK;
			}
		}
		break;

	case BSMUX_STATUS_RESUME:
		{
			BSMUX_TSK_FUNC("BSM:[%d] resume\r\n", id);

			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_WAITING_IDLE)) {
				BSMUX_TSK_FUNC("BSM:[%d] stop... resume -> idle\r\n", id);
				// idle
				bsmux_tsk_status_idle(id);
				return E_OK;
			}

			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_WAITING_HIT)) {

				if (g_bsmux_tsk_keep_still[id]) {
					/* sleep to release cpu resource */
					vos_util_delay_us(g_bsmux_tsk_still_us);
					return E_OK;
				}

				// init something...
				bsmux_ctrl_bs_init_fileinfo(id);
				bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_TOTAL2CARD, 0);
				g_bsmux_tsk_to_cutfile[id] = 0;
				// get new buf
				memset(&cur_buf, 0, sizeof(cur_buf));
				bsmux_ctrl_bs_dequeue(id, (void *)&cur_buf);
				// chk util storage buffer
				bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &val);
				if (val) {
					bsmux_util_strgbuf_handle_buffer(id, (void *)&cur_buf);
				}
				// chk emr pause
				bsmux_util_get_param(id, BSMUXER_PARAM_FILE_PAUSE_ON, &val);
				if (val) { // pause
					if (cur_buf.uiIsKey) {
						bsmux_util_get_param(id, BSMUXER_PARAM_FILE_PAUSE_CNT, &val);
						val++;
						bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_CNT, val);
						DBG_IND("[%d] pause gop cnt = %d\r\n", id, val);
					}
				}
				// retuen
				return E_OK;
			}

			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_WAITING_RUN)) {
				// init something...
				#if 0
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_EXTNSEC, &sec);
				bsmux_ctrl_bs_init_fileinfo(id);
				bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_TOTAL2CARD, 0);
				g_bsmux_tsk_to_cutfile[id] = 0;
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_EXTNUM, ((sec) ? 1 : 0));
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_EXTSEC, ((sec) ? sec : 0));
				#else
				bsmux_ctrl_mem_set_bufinfo(id, BSMUX_CTRL_TOTAL2CARD, 0);
				g_bsmux_tsk_to_cutfile[id] = 0;
				#endif
				// chk emr pause
				bsmux_util_get_param(id, BSMUXER_PARAM_FILE_PAUSE_CNT, &val);
				if (val) { // resume after pause
					bsqnum = ((val) >> 16);
					val = ((val) & 0xffff);
					BSMUX_MSG("[%d]resume:val(%d/%d)\r\n", (int)id, (int)bsqnum, (int)val);
					if (val != bsqnum) {
						DBG_DUMP("[%d] cnt=%d wanted=%d rollback1\r\n", id, val, bsqnum);
						bsmux_ctrl_bs_rollback(id, 1, 0);
					}
					{
						BSMUX_SAVEQ_BS_INFO tmp_buf = {0};
						if (E_OK == bsmux_ctrl_bs_predequeue(id, &tmp_buf)) {
							BSMUX_MSG("[%d]resume:tmp(%d/%d).\r\n", (int)id, (int)tmp_buf.uiType, (int)tmp_buf.uiIsKey);
							if (!tmp_buf.uiIsKey) {
								BSMUX_MSG("[%d]resume:uiIsKey(%d)\r\n", (int)id, (int)tmp_buf.uiIsKey);
								DBG_DUMP("[%d] cnt=%d wanted=%d rollback2\r\n", id, val, bsqnum);
								bsmux_ctrl_bs_rollback(id, 1, 0);
							}
						}
					}
					bsmux_util_set_param(id, BSMUXER_PARAM_FILE_PAUSE_CNT, 0);
				}
				// run
				bsmux_tsk_status_run(id);
			}
		}
		break;

	case BSMUX_STATUS_RUN:
		{
			BSMUX_TSK_FUNC("BSM:[%d] run\r\n", id);

			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_WAITING_IDLE)) {
				BSMUX_TSK_FUNC("BSM:[%d] stop... run -> suspend -> idle\r\n", id);
				// suspend
				bsmux_tsk_status_suspend(id);
				return E_OK;
			}

			if (g_bsmux_tsk_keep_still[id]) {
				/* sleep to release cpu resource */
				vos_util_delay_us(g_bsmux_tsk_still_us);
				return E_OK;
			}

			// check buffer lock status
			if (bsmux_util_check_buflocksts(id, (void *)&cur_buf) == FALSE) {
				if (bsmux_util_check_buflockdur(id, (void *)&cur_buf) == FALSE) {
					bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
				}
				goto label_check_cut_file;
			}

			// get
			bsmux_ctrl_bs_dequeue(id, (void *)&cur_buf);
			if (cur_buf.uiBSSize == 0) {
				DBG_DUMP(">> [%d] bs dequeue error\r\n", id);
				return E_SYS;
			}

			// chk util storage buffer
			bsmux_util_get_param(id, BSMUXER_PARAM_EN_STRGBUF, &val);
			if (val) {
				bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_ACT, &val);
				if (val) {
					bsmux_util_strgbuf_handle_buffer(id, (void *)&cur_buf);
					return E_OK;
				}
				bsmux_util_get_param(id, BSMUXER_PARAM_STRGBUF_HDR, &val);
				if (val) {
					bsmux_util_strgbuf_handle_header(id);
				}
			}

			// check frmidx
			if (bsmux_util_check_frmidx(id, (void *)&cur_buf) == FALSE) {
				return E_OK;
			}

			#if BSMUX_TEST_CHK_FIRSTI
			// check firstI
			if (bsmux_util_check_firstI(id, (void *)&cur_buf) == FALSE) {
				if (cur_buf.uiType == BSMUX_TYPE_VIDEO)
					DBG_DUMP("FirstIWrong[%d][0x%lx][0x%lx] drop\r\n", id, cur_buf.uiBSMemAddr, cur_buf.uiBSSize);
				return E_OK;
			}
			#endif

			// check tag
			if (bsmux_util_check_tag(id, (void *)&cur_buf) == FALSE) {
				bsmux_util_get_param(id, BSMUXER_PARAM_EN_DROP, &val);
				if (val) {
					goto label_check_cut_file;
				} else {
					DBG_ERR("CopyVWrong[%d][0x%lx][0x%lx]\r\n", id, cur_buf.uiBSMemAddr, cur_buf.uiBSSize);
					bsmux_cb_result(BSMUXER_RESULT_SLOWMEDIA, id);
					return E_SYS;
				}
			}

			// check buffer usage
			if (bsmux_util_check_bufuse(id, (void *)&cur_buf) == FALSE) {
				goto label_check_cut_file;
			}

			// action lock
			bsmux_action_lock();

			// common: make container header
			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_MAKE_HEADER)) {
				bsmux_util_make_header(id);
			}

			// specific: add nidx (mov)
			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_PAD_NIDX)) {
				if (cur_buf.uiType == BSMUX_TYPE_VIDEO) {
					bsmux_util_nidx_pad(id);
				}
			}

			// specific: add pat (ts)
			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_MAKE_PAT)) {
				bsmux_util_make_pat(id);
			}

			// specific: add pes (ts)
			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_MAKE_PES)) {
				if ((cur_buf.uiType == BSMUX_TYPE_VIDEO)
					|| (cur_buf.uiType == BSMUX_TYPE_AUDIO)) {
					bsmux_util_make_pes(id, (void *)&cur_buf);
				}
			}

			// specific: save nidx (mov)
			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_SAVE_ENTRY)) {
				if (cur_buf.uiType == BSMUX_TYPE_NIDXT) {
					bsmux_util_save_entry(id, (void *)&cur_buf);
				}
			}

			// package
			bsmux_ctrl_bs_copy2buffer(id, (void *)&cur_buf);

			// common: save entry
			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_SAVE_ENTRY)) {
				if (cur_buf.uiType != BSMUX_TYPE_NIDXT) {
					bsmux_util_save_entry(id, (void *)&cur_buf);
				}
			}

			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_MAKE_MOOV)) {
				#if BSMUX_TEST_MOOV_RC
				if (cur_buf.uiType == BSMUX_TYPE_VIDEO) {
					bsmux_util_make_moov(id, 1);
				}
				#else
				bsmux_util_make_moov(id, 1);
				#endif
			}

			// action unlock
			bsmux_action_unlock();

		label_check_cut_file:
			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_CUTFILE)) {
				bsmux_util_dump_info(id);
				bsmux_tsk_status_suspend(id);
			}
		}
		break;

	case BSMUX_STATUS_SUSPEND:
		{
			BSMUX_TSK_FUNC("BSM:[%d] suspend\r\n", id);

			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_ADDLAST)) {
				bsmux_util_add_lasting(id);
			}

			// put all to card
			bsmux_ctrl_bs_putall2card(id);

			// callback event: BEGIN
			// meaning status suspend and all bs has put
			// add here to avoid putall2card put front blk
			// event only, cur_buf is empty
			val = bsmux_cb_event(BSMUXER_CBEVENT_CUT_BEGIN, &cur_buf, id);
			if (val != E_OK) {
				DBG_ERR("[%d] cb event fail\r\n", id);
			}

			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_UPDATE_HEADER)) {
				bsmux_util_update_header(id);
			}

			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_PUT_LAST)) {
				bsmux_util_put_lasting(id);
			}

			// callback event: COMPLETE
			// event only, cur_buf is empty
			val = bsmux_cb_event(BSMUXER_CBEVENT_CUT_COMPLETE, &cur_buf, id);
			if (val != E_OK) {
				DBG_ERR("[%d] cb event fail\r\n", id);
			}

			// resume or idle
			if (bsmux_tsk_chk_action(id, BSMUX_ACTION_WAITING_IDLE)) {
				BSMUX_TSK_FUNC("BSM:[%d] stop... suspend -> idle\r\n", id);
				// init something...
				bsmux_ctrl_bs_init_fileinfo(id);
				// idle
				bsmux_tsk_status_idle(id);
				return E_OK;
			} else {
				// action: reorder 1sec
				if (bsmux_tsk_chk_action(id, BSMUX_ACTION_REORDER_1SEC)) {
					bsmux_ctrl_bs_reorder(id, 1);
				}
				// init something...
				bsmux_ctrl_bs_get_fileinfo(id, BSMUX_FILEINFO_EXTNSEC, &val);
				bsmux_ctrl_bs_init_fileinfo(id);
				#if 0
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_EXTNSEC, sec);
				#else
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_EXTNUM, ((val) ? 1 : 0));
				bsmux_ctrl_bs_set_fileinfo(id, BSMUX_FILEINFO_EXTSEC, ((val) ? val : 0));
				#endif
				// action: overlap 1sec
				if (bsmux_tsk_chk_action(id, BSMUX_ACTION_OVERLAP_1SEC)) {
					bsmux_ctrl_bs_rollback(id, 1, 0);
				}
				// resume
				bsmux_tsk_status_resume(id);
				// chk pause id
				bsmux_tsk_check_pause(id);
				// chk loop
				bsmux_tsk_handle_loop(id);
			}
		}
		break;

	default:
		DBG_DUMP("status invalid\r\n");
		break;
	}

	return E_OK;
}

static THREAD_RETTYPE ISF_BsMux_Tsk_Func(void)
{
	BSMUX_CTRL_OBJ *p_ctrlobj;
	FLGPTN ret_flgptn = 0;
	UINT32 ret_idx = 0;
	#if 1
	UINT32 last_idx = 0;
	#endif

	THREAD_ENTRY();

	p_ctrlobj = bsmux_get_ctrl_obj();
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed\r\n");
		goto label_bsmux_tsk_func_end;
	}

	BSMUX_TSK_FUNC("BSM:[TSK] -> wait ready\r\n");

	vos_flag_clr(p_ctrlobj->FlagID, BSMUX_FLG_TSK_IDLE | BSMUX_FLG_TSK_STOPPED);
	vos_flag_set(p_ctrlobj->FlagID, BSMUX_FLG_TSK_READY);

	while (1) {
		vos_flag_set(p_ctrlobj->FlagID, BSMUX_FLG_TSK_IDLE);
		PROFILE_TASK_IDLE();
		BSMUX_TSK_FUNC("BSM:[TSK] -> idle wait trigger\r\n");
		// vos_flag_wait is identical to vos_flag_wait_interruptible
		vos_flag_wait(&ret_flgptn, p_ctrlobj->FlagID, BSMUX_FLG_TSK_BS_IN | BSMUX_FLG_TSK_GETALL| BSMUX_FLG_TSK_STOP, TWF_ORW | TWF_CLR);
		PROFILE_TASK_BUSY();
		vos_flag_clr(p_ctrlobj->FlagID, BSMUX_FLG_TSK_IDLE);

		if (ret_flgptn & BSMUX_FLG_TSK_BS_IN) {
			DBG_IND(">>> BSMUX_FLG_TSK_BS_IN\r\n");

			while (bsmux_tsk_pick_idx(last_idx, &ret_idx) != E_SYS) {
				bsmux_tsk_set_last_idx(ret_idx);
				bsmux_tsk_chk_status(ret_idx);
				bsmux_tsk_get_last_idx(&last_idx);
			}
		}

		if (ret_flgptn & BSMUX_FLG_TSK_GETALL) {

			for (ret_idx = 0; ret_idx < BSMUX_MAX_CTRL_NUM; ret_idx++) {
				if (g_bsmux_tsk_to_idle[ret_idx]) {
					while (!bsmux_tsk_status_is_idle(ret_idx)) {
						bsmux_tsk_chk_status(ret_idx);
					}
					g_bsmux_tsk_to_idle[ret_idx] = 0;
				}
				DBG_IND("[%d] tsk_to_idle %d\r\n", ret_idx, g_bsmux_tsk_to_idle[ret_idx]);
			}

			vos_flag_set(p_ctrlobj->FlagID, BSMUX_FLG_TSK_GETALLDONE);
		}

		if (ret_flgptn & BSMUX_FLG_TSK_STOP) {
			DBG_IND(">>> BSMUX_FLG_TSK_STOP\r\n");
			break;
		}
	}

	vos_flag_clr(p_ctrlobj->FlagID, BSMUX_FLG_TSK_READY);
	vos_flag_set(p_ctrlobj->FlagID, BSMUX_FLG_TSK_STOPPED);
	vos_flag_set(p_ctrlobj->FlagID, BSMUX_FLG_TSK_CLOSE);

label_bsmux_tsk_func_end:
	THREAD_RETURN(0);
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
//                                                     [WAITING HIT] -- hit --> [HIT TO RUN]
//                                                          ^                      |
//                                                          |                      v
//  [UNINIT] -- init   --> [INIT] -- open  --> [IDLE]  -- start              --> [RUN] -- suspend --> [SUS]
//          <-- uninit --          <-- close --         <-- [PUTALL] <-- stop --       <-- resume --
//

void ISF_BsMux_Tsk(void)
{
	ISF_BsMux_Tsk_Func();
    return;
}

ER bsmux_tsk_status_dbg(UINT32 value)
{
	g_debug_tskinfo = value;
	return E_OK;
}

ER bsmux_tsk_action_dbg(UINT32 value)
{
	g_debug_action = value;
	return E_OK;
}

//>>> tsk: init
//>>> module status should be [INIT]
void bsmux_tsk_init(void)
{
	BSMUX_TSK_CTRL_OBJ *p_ctrl_obj = NULL;
	BSMUX_CTRL_OBJ *p_ctrlobj = NULL;
	UINT32 id, num = BSMUX_MAX_CTRL_NUM;

	p_ctrlobj = bsmux_get_ctrl_obj();
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed\r\n");
		return;
	}

	for (id = 0; id < num; id++) {
		p_ctrl_obj = bsmux_tsk_get_ctrl_obj(id);
		p_ctrl_obj->Status = BSMUX_STATUS_IDLE;
		p_ctrl_obj->Action = BSMUX_ACTION_NONE;
		g_bsmux_tsk_to_idle[id] = 0;
	}

	return ;
}

//>>> tsk motion: start
//>>> module status should be [INIT] -> [OPEN]
void bsmux_tsk_start(void)
{
	BSMUX_CTRL_OBJ *p_ctrlobj;

	p_ctrlobj = bsmux_get_ctrl_obj();
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed\r\n");
		return;
	}

	if (0 == vos_flag_chk(p_ctrlobj->FlagID, BSMUX_FLG_TSK_READY)) {

		p_ctrlobj->TaskID = vos_task_create(ISF_BsMux_Tsk, 0, "BsMux_Tsk", PRI_ISF_BSMUX, STKSIZE_ISF_BSMUX);

		THREAD_RESUME(p_ctrlobj->TaskID);
	}
}

//>>> tsk motion: bitstream in
//>>> module status should be [START] & [IN]
void bsmux_tsk_bs_in(void)
{
	BSMUX_CTRL_OBJ *p_ctrlobj;

	p_ctrlobj = bsmux_get_ctrl_obj();
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed\r\n");
		return;
	}

	if (0 == vos_flag_chk(p_ctrlobj->FlagID, BSMUX_FLG_TSK_READY)) {
		DBG_WRN("task is not ready\r\n");
	}

	vos_flag_set(p_ctrlobj->FlagID, BSMUX_FLG_TSK_BS_IN);
}

//>>> tsk motion: get all bs to next module
void bsmux_tsk_getall(void)
{
	BSMUX_CTRL_OBJ *p_ctrlobj;

	p_ctrlobj = bsmux_get_ctrl_obj();
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed\r\n");
		return;
	}

	if (0 == vos_flag_chk(p_ctrlobj->FlagID, BSMUX_FLG_TSK_READY)) {
		DBG_WRN("task is not ready\r\n");
	}

	vos_flag_set(p_ctrlobj->FlagID, BSMUX_FLG_TSK_GETALL);
}

//>>> tsk motion: stop
//>>> module status should be [STOP] -> [CLOSE]
void bsmux_tsk_stop(void)
{
	BSMUX_CTRL_OBJ *p_ctrlobj;
	FLGPTN ret_flgptn = 0;

	p_ctrlobj = bsmux_get_ctrl_obj();
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed\r\n");
		return;
	}

	vos_flag_set(p_ctrlobj->FlagID, BSMUX_FLG_TSK_STOP);
	// vos_flag_wait is identical to vos_flag_wait_interruptible
	vos_flag_wait(&ret_flgptn, p_ctrlobj->FlagID, BSMUX_FLG_TSK_STOPPED, TWF_ORW | TWF_CLR);
}

//>>> tsk motion: wait until idle
void bsmux_tsk_wait_idle(void)
{
	BSMUX_CTRL_OBJ *p_ctrlobj;
	FLGPTN ret_flgptn = 0;

	p_ctrlobj = bsmux_get_ctrl_obj();
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed\r\n");
		return;
	}
	// vos_flag_wait is identical to vos_flag_wait_interruptible
	vos_flag_wait(&ret_flgptn, p_ctrlobj->FlagID, BSMUX_FLG_TSK_GETALLDONE, TWF_ORW | TWF_CLR);
}

//>>> tsk motion: destroy
void bsmux_tsk_destroy(void)
{
	BSMUX_CTRL_OBJ *p_ctrlobj;
	UINT32 bsmux_status = TRUE;
	UINT32 delay_cnt = 50;

	p_ctrlobj = bsmux_get_ctrl_obj();
	if (NULL == p_ctrlobj) {
		DBG_ERR("Get ctrl obj failed\r\n");
		return;
	}

	while (bsmux_status) {
		if (0 == vos_flag_chk(p_ctrlobj->FlagID, BSMUX_FLG_TSK_READY)) {
			bsmux_status = FALSE;
		} else {
			DBG_WRN("task is not stopped\r\n");
		}
		if (bsmux_status == FALSE) break;
		// force tsk stop
		vos_flag_set(p_ctrlobj->FlagID, BSMUX_FLG_TSK_STOP);
		vos_util_delay_ms(10);
		delay_cnt--;
		if (delay_cnt == 0) {
			bsmux_status = FALSE;
		}
	}

	if (0 == vos_flag_chk(p_ctrlobj->FlagID, BSMUX_FLG_TSK_CLOSE)) {
		DBG_DUMP("BsMux vos_task_destroy (%x)\r\n", p_ctrlobj->TaskID);
		vos_task_destroy(p_ctrlobj->TaskID);
	}
}

//>>> init tsk status & action
ER bsmux_tsk_status_init(UINT32 id)
{
	BSMUX_TSK_CTRL_OBJ *p_obj = NULL;
	UINT32 action = 0;
	ER ret;

	p_obj = bsmux_tsk_get_ctrl_obj(id);
	if (bsmux_util_is_null_obj((void *)p_obj)) {
		DBG_ERR("p_obj null\r\n");
		return E_NOEXS;
	}

	ret = bsmux_util_tskobj_init(id, &action);
	if (ret != E_OK) {
		DBG_ERR("init fail\r\n");
		return ret;
	}

	bsmux_action_lock();
	p_obj->Action = action;
	bsmux_action_unlock();

	//debug
	BSMUX_ACT_FUNC("BSM:[%d] action 0x%X\r\n", id, action);

	g_bsmux_tsk_to_idle[id] = 0;
	return E_OK;
}

//>>> tsk status: idle
ER bsmux_tsk_status_idle(UINT32 id)
{
	BSMUX_TSK_CTRL_OBJ *p_ctrl_obj = NULL;

	p_ctrl_obj = bsmux_tsk_get_ctrl_obj(id);
	if (p_ctrl_obj == NULL) {
		DBG_ERR("get tsk ctrl obj null\r\n");
		return E_SYS;
	}

	bsmux_status_lock();
	p_ctrl_obj->Status = BSMUX_STATUS_IDLE;
	bsmux_status_unlock();

	return E_OK;
}

//>>> tsk status: resume
ER bsmux_tsk_status_resume(UINT32 id)
{
	BSMUX_TSK_CTRL_OBJ *p_ctrl_obj = NULL;

	p_ctrl_obj = bsmux_tsk_get_ctrl_obj(id);
	if (p_ctrl_obj == NULL) {
		DBG_ERR("get tsk ctrl obj null\r\n");
		return E_SYS;
	}

	bsmux_status_lock();
	p_ctrl_obj->Status = BSMUX_STATUS_RESUME;
	bsmux_status_unlock();

	return E_OK;
}

//>>> tsk status: run
ER bsmux_tsk_status_run(UINT32 id)
{
	BSMUX_TSK_CTRL_OBJ *p_ctrl_obj = NULL;

	p_ctrl_obj = bsmux_tsk_get_ctrl_obj(id);
	if (p_ctrl_obj == NULL) {
		DBG_ERR("get tsk ctrl obj null\r\n");
		return E_SYS;
	}

	bsmux_status_lock();
	p_ctrl_obj->Status = BSMUX_STATUS_RUN;
	bsmux_status_unlock();

	return E_OK;
}

//>>> tsk status: suspend
ER bsmux_tsk_status_suspend(UINT32 id)
{
	BSMUX_TSK_CTRL_OBJ *p_ctrl_obj = NULL;

	p_ctrl_obj = bsmux_tsk_get_ctrl_obj(id);
	if (p_ctrl_obj == NULL) {
		DBG_ERR("get tsk ctrl obj null\r\n");
		return E_SYS;
	}

	bsmux_status_lock();
	p_ctrl_obj->Status = BSMUX_STATUS_SUSPEND;
	bsmux_status_unlock();

	return E_OK;
}

//>>> tsk status: getall
ER bsmux_tsk_trig_getall(UINT32 id)
{
	g_bsmux_tsk_to_idle[id] = 1;
	bsmux_tsk_getall();
	bsmux_tsk_wait_idle();

	DBG_IND("g_bsmux_tsk_to_idle %d\r\n", g_bsmux_tsk_to_idle[id]);
	return E_OK;
}

//>>> tsk status: cutfile
ER bsmux_tsk_trig_cutfile(UINT32 id)
{
	g_bsmux_tsk_to_cutfile[id] = 1;
	return E_OK;
}

ER bsmux_tsk_still(UINT32 id)
{
	g_bsmux_tsk_keep_still[id] = 1;
	return E_OK;
}

ER bsmux_tsk_awake(UINT32 id)
{
	g_bsmux_tsk_keep_still[id] = 0;
	return E_OK;
}

