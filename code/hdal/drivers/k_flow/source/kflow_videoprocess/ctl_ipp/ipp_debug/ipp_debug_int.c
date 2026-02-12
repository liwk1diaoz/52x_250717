#include "kwrap/file.h"
#include "kdrv_builtin/kdrv_ipp_builtin.h"
#include "ipp_debug_int.h"
#include "kdf_ipp_int.h"
#include "ctl_ipp_isp_int.h"
#include "ctl_ipp_flow_task_int.h"
#include "ipp_event_int.h"

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

/* for debug.h */
unsigned int ctl_ipp_debug_level = NVT_DBG_ERR;

// for specify CTL_IPP_DBG_TRC content
CTL_IPP_DBG_TRC_TYPE g_ctl_ipp_dbg_trc_type = CTL_IPP_DBG_TRC_NONE;

/* ipp debug log rate control. assume base frame rate = 30 */
UINT32 g_ctl_ipp_dbg_base_frame_rate = 30;

#if 0
#endif

int ctl_ipp_int_printf(const char *fmtstr, ...)
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

/**
	time stamp log
*/
#if CTL_IPP_DBG_TS_ENABLE
static UINT32 ctl_ipp_dbg_ts_lock;
static UINT32 ctl_ipp_dbg_ts_cnt;
static CTL_IPP_DBG_TS_LOG ctl_ipp_dbg_ts_log[CTL_IPP_DBG_TS_LOG_NUM];

/* fastboot only */
#if defined (__KERNEL__)
static UINT8 ctl_ipp_dbg_ts_fastboot_is_set = FALSE;
static CTL_IPP_DBG_TS_LOG ctl_ipp_dbg_ts_log_fastboot[CTL_IPP_DBG_TS_LOG_FASTBOOT_NUM];
#endif

_INLINE UINT64 ctl_ipp_dbg_ts_diff(UINT64 ts_0, UINT64 ts_1)
{
	if (ts_1 < ts_0) {
		return 0;
	}

	return ((ts_1) - (ts_0));
}

void ctl_ipp_dbg_ts_set(CTL_IPP_DBG_TS_LOG ts)
{
	UINT32 idx;

	if (ctl_ipp_dbg_ts_lock == FALSE) {
		idx = ctl_ipp_dbg_ts_cnt % CTL_IPP_DBG_TS_LOG_NUM;
		ctl_ipp_dbg_ts_log[idx] = ts;
		ctl_ipp_dbg_ts_cnt += 1;

		#if defined (__KERNEL__)
			/* check if switch flow from fastboot to normal */
			if (kdrv_builtin_is_fastboot() && !ctl_ipp_dbg_ts_fastboot_is_set &&
				(ctl_ipp_dbg_ts_cnt == CTL_IPP_DBG_TS_LOG_FASTBOOT_NUM)) {
				for (idx = 0; idx < CTL_IPP_DBG_TS_LOG_FASTBOOT_NUM; idx++) {
					ctl_ipp_dbg_ts_log_fastboot[idx] = ctl_ipp_dbg_ts_log[idx];
				}
				ctl_ipp_dbg_ts_fastboot_is_set = true;
			}
		#endif
	}
}

void ctl_ipp_dbg_ts_dump(int (*dump)(const char *fmt, ...))
{
	UINT32 i, idx;
	CTL_IPP_DBG_TS_LOG *p_ts;

	ctl_ipp_dbg_ts_lock = TRUE;

	dump("\r\n---- IPP PROC TIME ----\r\n");
	dump("%10s  %4s%12s %12s%20s %20s %20s %20s %20s %20s %20s\r\n", "HANDLE", "ERR", "FRM_CNT", "CTL_SND", "CTL_RCV", "KDF_SND", "KDF_RCV", "PROC_START", "TRIG_END", "BP", "PROC_END(us)");
	for (i = 0; i < CTL_IPP_DBG_TS_LOG_NUM; i++) {
		if (i >= ctl_ipp_dbg_ts_cnt) {
			break;
		}
		idx = (ctl_ipp_dbg_ts_cnt - 1 - i) % CTL_IPP_DBG_TS_LOG_NUM;
		p_ts = &ctl_ipp_dbg_ts_log[idx];
		dump("0x%.8x  %4d%12d %12lld %12lld(%-+6lld) %12lld(%-+6lld) %12lld(%-+6lld) %12lld(%-+6lld) %12lld(%-+6lld) %12lld(%-+6lld) %12lld(%-+6lld)\r\n",
			(unsigned int)p_ts->handle, (int)p_ts->err_msg, (int)p_ts->cnt,
			(p_ts->ts[0]),
			(p_ts->ts[1]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[0], p_ts->ts[1]),
			(p_ts->ts[2]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[1], p_ts->ts[2]),
			(p_ts->ts[3]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[2], p_ts->ts[3]),
			(p_ts->ts[4]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[3], p_ts->ts[4]),
			(p_ts->ts[5]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[4], p_ts->ts[5]),
			(p_ts->ts[6]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[5], p_ts->ts[6]),
			(p_ts->ts[7]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[6], p_ts->ts[7]));
	}

	#if 0
	{
		UINT64 ts_start = 0;
		UINT64 ts_end = 0;
		UINT64 eng_busy_time = 0;
		UINT64 eng_usage = 0;

		for (i = 0; i < CTL_IPP_DBG_TS_LOG_NUM; i++) {
			if (i > ctl_ipp_dbg_ts_cnt) {
				break;
			}

			idx = (ctl_ipp_dbg_ts_cnt - 1 - i) % CTL_IPP_DBG_TS_LOG_NUM;
			p_ts = &ctl_ipp_dbg_ts_log[idx];

			if (i == 0) {
				ts_end = p_ts->ts[CTL_IPP_DBG_TS_PROCEND];
			}

			if (p_ts->ts[CTL_IPP_DBG_TS_PROCEND] != 0) {
				ts_start = p_ts->ts[CTL_IPP_DBG_TS_PROCSTART];
				eng_busy_time += ctl_ipp_dbg_ts_diff(p_ts->ts[CTL_IPP_DBG_TS_PROCSTART], p_ts->ts[CTL_IPP_DBG_TS_PROCEND]);
			}
		}
		eng_usage = ctl_ipp_dbg_ts_diff(ts_start, ts_end);

		if (eng_usage != 0) {
			eng_usage = ((UINT32)(eng_busy_time * 1000) / (UINT32)eng_usage);
		}

		DBG_DUMP("Total duration %lld, eng_busy %lld, usage %lld", ctl_ipp_dbg_ts_diff(ts_start, ts_end), eng_busy_time, eng_usage);
	}
	#endif

	ctl_ipp_dbg_ts_lock = FALSE;
}

void ctl_ipp_dbg_ts_dump_fastboot(void)
{
#if defined (__KERNEL__)
	UINT32 i;
	CTL_IPP_DBG_TS_LOG *p_ts;

	DBG_DUMP("---- IPP PROC TIME(FASTBOOT TO HDAL) ----\r\n");
	DBG_DUMP("%10s  %4s%12s %12s%20s %20s %20s %20s %20s %20s %20s\r\n", "HANDLE", "ERR", "FRM_CNT", "CTL_SND", "CTL_RCV", "KDF_SND", "KDF_RCV", "PROC_START", "TRIG_END", "BP", "PROC_END(us)");
	for (i = 0; i < CTL_IPP_DBG_TS_LOG_FASTBOOT_NUM; i++) {
		p_ts = &ctl_ipp_dbg_ts_log_fastboot[i];
		DBG_DUMP("0x%.8x  %4d%12d %12lld %12lld(%-+6lld) %12lld(%-+6lld) %12lld(%-+6lld) %12lld(%-+6lld) %12lld(%-+6lld) %12lld(%-+6lld) %12lld(%-+6lld)\r\n",
			(unsigned int)p_ts->handle, (int)p_ts->err_msg, (int)p_ts->cnt,
			(p_ts->ts[0]),
			(p_ts->ts[1]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[0], p_ts->ts[1]),
			(p_ts->ts[2]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[1], p_ts->ts[2]),
			(p_ts->ts[3]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[2], p_ts->ts[3]),
			(p_ts->ts[4]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[3], p_ts->ts[4]),
			(p_ts->ts[5]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[4], p_ts->ts[5]),
			(p_ts->ts[6]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[5], p_ts->ts[6]),
			(p_ts->ts[7]),
			ctl_ipp_dbg_ts_diff(p_ts->ts[6], p_ts->ts[7]));
	}
#endif
}

#else
/* CTL_IPP_DBG_TS_ENABLE == 0 */
void ctl_ipp_dbg_ts_set(CTL_IPP_DBG_TS_LOG ts)
{

}

void ctl_ipp_dbg_ts_dump(int (*dump)(const char *fmt, ...))
{
	DBG_DUMP("not support when CTL_IPP_DBG_TS_ENABLE = 0\r\n");
}

void ctl_ipp_dbg_ts_dump_fastboot(void)
{
	DBG_DUMP("not support when CTL_IPP_DBG_TS_ENABLE = 0\r\n");
}
#endif

#if 0
#endif

#define CTL_IPP_DBG_DCE_STRP_OFFSET (1)
#define CTL_IPP_DBG_IMEIN_STRP_OFFSET (6)
#define CTL_IPP_DBG_IMEOUT_STRP_OFFSET (11)
#define CTL_IPP_DBG_BASEINFO_NUM (8)

static CTL_IPP_DBG_BASE_INFO ctl_ipp_dbg_base_info[CTL_IPP_DBG_BASEINFO_NUM];
static CTL_IPP_LIST_HEAD *ctl_ipp_dbg_hdl_used_head ;

void ctl_ipp_dbg_hdl_head_set(CTL_IPP_LIST_HEAD *hdl_used_head)
{
	ctl_ipp_dbg_hdl_used_head = hdl_used_head;
}

void ctl_ipp_dbg_hdl_stripe_set(UINT32 kdf_hdl, KDRV_DCE_STRIPE_INFO *p_strp_info)
{
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_HANDLE *p_hdl;
	CTL_IPP_DCE_CTRL *p_dce;
	UINT32 i;

	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		if (p_hdl->kdf_hdl == kdf_hdl) {
			break;
		}

		p_hdl = NULL;
	}

	if (p_hdl) {
		/* keep dce stripe information in strp_h_arr
			strp_h_arr[0]:		strp_num
			strp_h_arr[1~5]:	dce strp size
			strp_h_arr[6~10]:	ime_in strp size
			strp_h_arr[11~15]:	ime_out strp size
			note that max stripe number is 64, strp size may not fully stored
		*/
		p_dce = &p_hdl->ctrl_info.dce_ctrl;
		p_dce->strp_h_arr[0] = p_strp_info->hstp_num;
		for (i = 0; i < 5; i++) {
			p_dce->strp_h_arr[CTL_IPP_DBG_DCE_STRP_OFFSET + i] = p_strp_info->hstp[i];
			p_dce->strp_h_arr[CTL_IPP_DBG_IMEIN_STRP_OFFSET + i] = p_strp_info->ime_in_hstp[i];
			p_dce->strp_h_arr[CTL_IPP_DBG_IMEOUT_STRP_OFFSET + i] = p_strp_info->ime_out_hstp[i];
		}
	}
}

void ctl_ipp_dbg_hdl_dump_all(int (*dump)(const char *fmt, ...))
{
	CHAR *ctl_ipp_dbg_flow_tbl[CTL_IPP_FLOW_MAX] = {
		"unknown",
		"raw",
		"dir_raw",
		"ccir",
		"dir_ccir",
		"ime_d2d",
		"ipe_d2d",
		"vr360",
		"dce_d2d",
		"cap_raw",
		"cap_ccir",
	};
	CHAR *ctl_ipp_dbg_eng_tbl[KDF_IPP_ENG_MAX] = {
		"RHE",
		"IFE",
		"DCE",
		"IPE",
		"IME",
		"IFE2",
	};
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_HANDLE *p_hdl;
	KDF_IPP_HANDLE *p_kdf_hdl;
	UINT32 i;

	/* dump basic information of every handle */
	dump("\r\n---- ctrl handle basic information ----\r\n");
	dump("%24s  %11s%11s%10s  %10s%12s%10s%10s%10s%10s%10s%10s%10s%10s%10s\r\n",
		"name", "sts", "addr", "flow", "func_en", "alg_iq_id", "ctl_snd", "ctl_sta", "ctl_end", "ctl_drop", "kdf_snd", "kdf_sta", "kdf_end", "kdf_drop", "hwrst_cnt" );
	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;

		if (p_kdf_hdl != NULL) {
			dump("%24s   0x%.8x 0x%.8x%10s  0x%.8x  0x%.8x%10d%10d%10d%10d%10d%10d%10d%10d%10d\r\n",
				p_hdl->name, (int)p_hdl->sts, (unsigned int)p_hdl, ctl_ipp_dbg_flow_tbl[p_hdl->flow],
				(unsigned int)p_hdl->func_en, (unsigned int)p_hdl->isp_id[CTL_IPP_ALGID_IQ],
				(int)p_hdl->ctl_snd_evt_cnt, (int)p_hdl->ctl_frm_str_cnt, (int)p_hdl->proc_end_cnt, (int)p_hdl->drop_cnt,
				(int)p_kdf_hdl->snd_evt_cnt, (int)p_kdf_hdl->frame_sta_cnt, (int)p_kdf_hdl->proc_end_cnt, (int)p_kdf_hdl->drop_cnt, (int)p_kdf_hdl->reset_cnt);
		} else {
			dump("%24s   0x%.8x 0x%.8x%10s  0x%.8x  0x%.8x%10d%10d%10d%10d%10d%10d%10d%10d%10d%10d\r\n",
				p_hdl->name, (int)p_hdl->sts,  (unsigned int)p_hdl, ctl_ipp_dbg_flow_tbl[p_hdl->flow],
				(unsigned int)p_hdl->func_en, (unsigned int)p_hdl->isp_id[CTL_IPP_ALGID_IQ],
				(int)p_hdl->ctl_snd_evt_cnt, (int)p_hdl->ctl_frm_str_cnt, (int)p_hdl->proc_end_cnt, (int)p_hdl->drop_cnt,
				0, 0, 0, 0, 0, 0);
		}
	}

	/* dump direct buf event count of every handle */
	dump("\r\n---- buf callback event count ----\r\n");
	dump("%12s%12s%12s%12s\r\n", "in_buf_rel", "in_buf_drop", "in_buf_re_cb", "pro_skip");
	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;

		dump("%12d%12d%12d%12d\r\n",(int)p_hdl->in_buf_re_cnt, (int)p_hdl->in_buf_drop_cnt, (int)p_hdl->in_buf_re_cb_cnt ,(int)p_hdl->in_pro_skip_cnt);

	}

	dump("\r\n---- ctrl handle func_en ----\r\n");
	dump("%24s  %8s%8s%8s%8s%12s%12s%12s%16s%12s%12s%8s%14s\r\n",
		"name", "WDR", "SHDR", "DEFOG", "3DNR", "3DNR_STA", "DATASTAMP", "PRIMASK", "PM_PIXELATION", "YUV_SUBOUT", "VA_SUBOUT", "GDC", "DIRECT_SCL_UP");
	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		dump("%24s  %8d%8d%8d%8d%12d%12d%12d%16d%12d%12d%8d%14d\r\n\r\n",
			p_hdl->name,
			((p_hdl->func_en & CTL_IPP_FUNC_WDR) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_SHDR) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_DEFOG) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_3DNR) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_3DNR_STA) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_DATASTAMP) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_PRIMASK) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_PM_PIXELIZTION) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_YUV_SUBOUT) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_VA_SUBOUT) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_GDC) ? 1 : 0),
			((p_hdl->func_en & CTL_IPP_FUNC_DIRECT_SCL_UP) ? 1 : 0));
	}
	dump("%24s  %32s  %18s  dir_tsk[%2s %10s]\r\n",
		"name", "3DNR_LAST_DISABLE/SHIFT_FRM", "PAT_PASTE_LAST_FRM", "en", "status");
	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		dump("%24s  %22d/%9d  %18d  dir_tsk[%2u 0x%08x]\r\n",
			p_hdl->name,
			(int)p_hdl->last_3dnr_disable_frm, (int)p_hdl->last_3dnr_ref_shift_frm,
			(int)p_hdl->last_pattern_paste_frm,
			ctl_ipp_direct_get_tsk_en(),
			p_hdl->dir_tsk_sts);
	}

	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);

		dump("\r\n---- ctrl handle 0x%.8x, %s ----\r\n", (unsigned int)p_hdl, p_hdl->name);
		ctl_ipp_dbg_baseinfo_dump((UINT32)&p_hdl->ctrl_info, FALSE, dump);
		ctl_ipp_dbg_hdl_dump_buf((UINT32)p_hdl, dump);
	}

	/* engine err interrupt status */
	dump("\r\n---- engine inte error log ----\r\n");
	dump("%24s  %8s: %12s%12s\r\n", "name", "eng", "sts", "at frm_cnt");
	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		p_kdf_hdl = (KDF_IPP_HANDLE *)p_hdl->kdf_hdl;

		if (p_kdf_hdl != NULL) {
			UINT32 eng;

			for (eng = KDF_IPP_ENG_IFE; eng < KDF_IPP_ENG_MAX; eng++) {
				dump("%24s  %8s:   0x%.8x%12d\r\n",
					p_hdl->name,
					ctl_ipp_dbg_eng_tbl[eng],
					(unsigned int)p_kdf_hdl->eng_err_sts[eng],
					(int)p_kdf_hdl->eng_err_frm[eng]);
			}
		}
	}


	for(i = 0; i < CTL_IPP_DBG_BASEINFO_NUM; i++) {
		if (strcmp(ctl_ipp_dbg_base_info[i].name, "\0") != 0) {
			if (i == 0) {
				dump("\r\n---- global base info ----\r\n");
			} else {
				dump("\r\n");
			}
			dump("%12s  %10s %10s%10s%10s%10s\r\n",
				"name",  "flow", "ctl_snd", "ctl_sta", "ctl_end", "ctl_drop");

			dump("%12s  %10s %10d%10d%10d%10d\r\n",
				ctl_ipp_dbg_base_info[i].name, ctl_ipp_dbg_flow_tbl[ctl_ipp_dbg_base_info[i].flow],
				(int)ctl_ipp_dbg_base_info[i].ctl_snd_evt_cnt, (int)ctl_ipp_dbg_base_info[i].ctl_frm_str_cnt, (int)ctl_ipp_dbg_base_info[i].ctl_proc_end_cnt, (int)ctl_ipp_dbg_base_info[i].ctl_drop_cnt);

			dump("%12s%12s%14s%12s\r\n", "in_buf_rel", "in_buf_drop", "in_buf_re_cb", "pro_skip");
			dump("%12d%12d%14d%12d\r\n",(int)ctl_ipp_dbg_base_info[i].in_buf_re_cnt, (int)ctl_ipp_dbg_base_info[i].in_buf_drop_cnt, (int)ctl_ipp_dbg_base_info[i].in_buf_re_cb_cnt ,(int)ctl_ipp_dbg_base_info[i].in_pro_skip_cnt);
		}
	}
}

void ctl_ipp_dbg_baseinfo_dump(UINT32 baseinfo, BOOL is_from_pool, int (*dump)(const char *fmt, ...))
{
	#define IPP_DBG_STRBUF_LENGTH 512
	CHAR *ctl_ipp_dbg_func_mode_tbl[] = {
		"linear",
		"DFS",
		"PHDR",
		"SHDR",
		"SHDR",
		""
	};

	CHAR *ctl_ipp_dbg_flip_tbl[] = {
		"FLIP_NONE",
		"FLIP_H",
		"FLIP_V",
		"FLIP_H_V",
		""
	};

	CHAR *ctl_ipp_dbg_imepath_tbl[] = {
		"path1",
		"path2",
		"path3",
		"path4",
		"path5",
		"path6",
		""
	};

	CHAR *ctl_ipp_dbg_scl_tbl[] = {
		"CTL_IPP_SCL_BICUBIC",
		"CTL_IPP_SCL_BILINEAR",
		"CTL_IPP_SCL_NEAREST",
		"CTL_IPP_SCL_INTEGRATION",
		"CTL_IPP_SCL_AUTO",
		""
	};
	UINT32 i;
	CTL_IPP_BASEINFO *p_base_info;
	BOOL tplnr_en;

	p_base_info = (CTL_IPP_BASEINFO *)baseinfo;

#if 0
	str_len = snprintf(str_buf, IPP_DBG_STRBUF_LENGTH, "--------------- RHE ------------\r\n");
	str_len += snprintf(str_buf + str_len, IPP_DBG_STRBUF_LENGTH - str_len, "%9s %12s %12s %8s %8s %6s %12s %12s %12s %12s %12s  %10s\r\n", "Func mode", "Raw Decode", "Flip", "width", "height", "lofs", "Crop mode", "Crop Start_x", "Crop Start_y", "Crop size_x", "Crop size_y", "fmt");
	str_len += snprintf(str_buf + str_len, IPP_DBG_STRBUF_LENGTH - str_len, "%9s %12d %12s %8d %8d %6d %12d %12d %12d %12d %12d  0x%.8x\r\n",
						ctl_ipp_dbg_func_mode_tbl[p_base_info->rhe_ctrl.func_mode],
						p_base_info->rhe_ctrl.decode_enable,
						ctl_ipp_dbg_flip_tbl[p_base_info->rhe_ctrl.flip],
						p_base_info->rhe_ctrl.in_size.w,
						p_base_info->rhe_ctrl.in_size.h,
						p_base_info->rhe_ctrl.in_lofs,
						p_base_info->rhe_ctrl.in_crp_mode,
						p_base_info->rhe_ctrl.in_crp_window.x,
						p_base_info->rhe_ctrl.in_crp_window.y,
						p_base_info->rhe_ctrl.in_crp_window.w,
						p_base_info->rhe_ctrl.in_crp_window.h,
						p_base_info->rhe_ctrl.in_fmt);
	dump("%s", str_buf);
#endif

	dump("--------------- IFE ------------\r\n");
	dump("%9s %12s %12s %12s %8s %8s %10s %10s %6s %6s %8s\r\n", "Func mode", "Raw Decode", "Decode Rate", "Flip Type", "width", "height", "addr0", "addr1", "lofs0", "lofs1", "ring_buf");
	dump("%9s %12d %12d %12s %8d %8d 0x%.8x 0x%.8x %6d %6d %8d\r\n",
						ctl_ipp_dbg_func_mode_tbl[0],
						p_base_info->ife_ctrl.decode_enable,
						(int)p_base_info->ife_ctrl.decode_ratio,
						ctl_ipp_dbg_flip_tbl[p_base_info->ife_ctrl.flip],
						(int)p_base_info->ife_ctrl.in_size.w,
						(int)p_base_info->ife_ctrl.in_size.h,
						(unsigned int)p_base_info->ife_ctrl.in_addr[0],
						(unsigned int)p_base_info->ife_ctrl.in_addr[1],
						(int)p_base_info->ife_ctrl.in_lofs[0],
						(int)p_base_info->ife_ctrl.in_lofs[1],
						(int)p_base_info->ife_ctrl.dir_buf_num);

	dump("\r\n%9s %12s %12s %12s %12s  %10s %11s\r\n", "Crop mode", "Crop Start_x", "Crop Start_y", "Crop size_x", "Crop size_y", "fmt", "dir_hdr_sie");
	dump("%9d %12d %12d %12d %12d  0x%.8x  0x%.8x\r\n",
						(int)p_base_info->ife_ctrl.in_crp_mode,
						(int)p_base_info->ife_ctrl.in_crp_window.x,
						(int)p_base_info->ife_ctrl.in_crp_window.y,
						(int)p_base_info->ife_ctrl.in_crp_window.w,
						(int)p_base_info->ife_ctrl.in_crp_window.h,
						(unsigned int)p_base_info->ife_ctrl.in_fmt,
						(unsigned int)p_base_info->ife_ctrl.hdr_ref_chk_bit);



	dump("\r\n--------------- DCE ------------\r\n");
	dump("%10s %8s %8s %8s %8s %10s\r\n", "in_fmt", "width", "height", "lofs_y", "lofs_uv", "strp_rule");
	dump("0x%.8x %8d %8d %8d %8d %10d\r\n",
						(unsigned int)p_base_info->dce_ctrl.in_fmt,
						(int)p_base_info->dce_ctrl.in_size.w,
						(int)p_base_info->dce_ctrl.in_size.h,
						(int)p_base_info->dce_ctrl.in_lofs[0],
						(int)p_base_info->dce_ctrl.in_lofs[1],
						(int)p_base_info->dce_ctrl.strp_rule);

	if (p_base_info->dce_ctrl.strp_h_arr[0] > 0) {
		dump("\r\n%16s %10s %s\r\n", "strp_type", "strp_num", "strp_size");
		dump("%16s %10d %5d, %5d, %5d, %5d, %5d...\r\n", "dce_strp",
						(int)p_base_info->dce_ctrl.strp_h_arr[0],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_DCE_STRP_OFFSET + 0],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_DCE_STRP_OFFSET + 1],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_DCE_STRP_OFFSET + 2],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_DCE_STRP_OFFSET + 3],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_DCE_STRP_OFFSET + 4]);
		dump("%16s %10d %5d, %5d, %5d, %5d, %5d...\r\n", "ime_in_strp",
						(int)p_base_info->dce_ctrl.strp_h_arr[0],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_IMEIN_STRP_OFFSET + 0],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_IMEIN_STRP_OFFSET + 1],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_IMEIN_STRP_OFFSET + 2],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_IMEIN_STRP_OFFSET + 3],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_IMEIN_STRP_OFFSET + 4]);
		dump("%16s %10d %5d, %5d, %5d, %5d, %5d...\r\n", "ime_out_strp",
						(int)p_base_info->dce_ctrl.strp_h_arr[0],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_IMEOUT_STRP_OFFSET + 0],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_IMEOUT_STRP_OFFSET + 1],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_IMEOUT_STRP_OFFSET + 2],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_IMEOUT_STRP_OFFSET + 3],
						(int)p_base_info->dce_ctrl.strp_h_arr[CTL_IPP_DBG_IMEOUT_STRP_OFFSET + 4]);
	}

	dump("\r\n--------------- IPE ------------\r\n");
	dump("%10s %8s %8s %8s %10s %8s %12s %12s\r\n", "in_fmt", "width", "height", "out_en", "out_fmt", "eth_en", "eth_bit_sel", "eth_out_sel");
	dump("0x%.8x %8d %8d %8d 0x%.8x %8d %12d %12d\r\n",
						(unsigned int)p_base_info->ipe_ctrl.in_fmt,
						(int)p_base_info->ipe_ctrl.in_size.w,
						(int)p_base_info->ipe_ctrl.in_size.h,
						(int)p_base_info->ipe_ctrl.out_enable,
						(unsigned int)p_base_info->ipe_ctrl.out_fmt,
						(int)p_base_info->ipe_ctrl.eth.enable,
						(int)p_base_info->ipe_ctrl.eth.out_bit_sel,
						(int)p_base_info->ipe_ctrl.eth.out_sel);

	dump("\r\n--------------- IME ------------\r\n");
	dump("%10s %8s %8s %8s %8s %8s\r\n", "in_fmt", "width", "height", "lofs_y", "lofs_u", "lofs_v");
	dump("0x%.8x %8d %8d %8d %8d %8d\r\n",
						(unsigned int)p_base_info->ime_ctrl.in_fmt,
						(int)p_base_info->ime_ctrl.in_size.w,
						(int)p_base_info->ime_ctrl.in_size.h,
						(int)p_base_info->ime_ctrl.in_lofs[0],
						(int)p_base_info->ime_ctrl.in_lofs[1],
						(int)p_base_info->ime_ctrl.in_lofs[2]);

	dump("%20s %20s %20s\r\n", "scl_method_threshold", "scl_method_l", "scl_method_h");
	dump("0x%.18x %20s %20s\r\n",
						(unsigned int)p_base_info->ime_ctrl.out_scl_method_sel.scl_th,
						ctl_ipp_dbg_scl_tbl[p_base_info->ime_ctrl.out_scl_method_sel.method_l],
						ctl_ipp_dbg_scl_tbl[p_base_info->ime_ctrl.out_scl_method_sel.method_h]);

	if (is_from_pool) {
		if ((p_base_info->ime_ctrl.tplnr_enable == DISABLE) ||
			(p_base_info->ime_ctrl.tplnr_in_ref_addr[0] == 0) ||
			(p_base_info->ime_ctrl.tplnr_out_ms_addr == 0) ||
			(p_base_info->ime_ctrl.tplnr_out_mv_addr == 0)) {
			tplnr_en = DISABLE;
		} else {
			tplnr_en = ENABLE;
		}
	} else {
		tplnr_en = p_base_info->ime_ctrl.tplnr_enable;
	}
	dump("\r\n%16s %18s %18s %10s %16s %16s\r\n", "yuv_subout_en", "yuv_subout_size H", "yuv_subout_size V", "3dnr_en", "3dnr_ref_path", "3dnr_ms_roi_en");
	dump("%16d %18d %18d %10d %16s %16d\r\n",
						(int)p_base_info->ime_ctrl.lca_out_enable,
						(int)p_base_info->ime_ctrl.lca_out_size.w,
						(int)p_base_info->ime_ctrl.lca_out_size.h,
						(int)tplnr_en,
						ctl_ipp_dbg_imepath_tbl[p_base_info->ime_ctrl.tplnr_in_ref_path],
						(int)p_base_info->ime_ctrl.tplnr_out_ms_roi_enable);

	dump("\r\npattern_paste [BGN] size(%4spx, %4spx) color(%3s, %3s, %3s)  [PAT]   %8s %8s win(%3s%%(%4spx), %3s%%(%4spx), %3s%%(%4spx), %3s%%(%4spx)) %4s %4s\r\n", "w", "h", "r", "g", "b", "addr", "fmt", "x", "x", "y", "y", "w", "w", "h", "h", "lofs", "cst");
	dump("                        (%4dpx, %4dpx)      (%3u, %3u, %3u)        0x%8x %8x    (%3u%%(%4dpx), %3u%%(%4dpx), %3u%%(%4dpx), %3u%%(%4dpx)) %4u %4u\r\n",
		(p_base_info->ime_ctrl.pat_paste_win.w > 0) ? (int)CTL_IPP_PATTERN_PASTE_SIZE(p_base_info->ime_ctrl.pat_paste_info.img_info.size.w, p_base_info->ime_ctrl.pat_paste_win.w) : -1,
		(p_base_info->ime_ctrl.pat_paste_win.h > 0) ? (int)CTL_IPP_PATTERN_PASTE_SIZE(p_base_info->ime_ctrl.pat_paste_info.img_info.size.h, p_base_info->ime_ctrl.pat_paste_win.h) : -1,
		(unsigned int)p_base_info->ime_ctrl.pat_paste_bgn_color[0],
		(unsigned int)p_base_info->ime_ctrl.pat_paste_bgn_color[1],
		(unsigned int)p_base_info->ime_ctrl.pat_paste_bgn_color[2],
		(unsigned int)p_base_info->ime_ctrl.pat_paste_info.img_info.addr,
		(unsigned int)p_base_info->ime_ctrl.pat_paste_info.img_info.fmt,
		(unsigned int)p_base_info->ime_ctrl.pat_paste_win.x,
		(p_base_info->ime_ctrl.pat_paste_win.w > 0) ? (int)CTL_IPP_PATTERN_PASTE_OFS(p_base_info->ime_ctrl.pat_paste_win.x, p_base_info->ime_ctrl.pat_paste_info.img_info.size.w, p_base_info->ime_ctrl.pat_paste_win.w) : -1,
		(unsigned int)p_base_info->ime_ctrl.pat_paste_win.y,
		(p_base_info->ime_ctrl.pat_paste_win.h > 0) ? (int)CTL_IPP_PATTERN_PASTE_OFS(p_base_info->ime_ctrl.pat_paste_win.y, p_base_info->ime_ctrl.pat_paste_info.img_info.size.h, p_base_info->ime_ctrl.pat_paste_win.h) : -1,
		(unsigned int)p_base_info->ime_ctrl.pat_paste_win.w,
		(unsigned int)p_base_info->ime_ctrl.pat_paste_info.img_info.size.w,
		(unsigned int)p_base_info->ime_ctrl.pat_paste_win.h,
		(unsigned int)p_base_info->ime_ctrl.pat_paste_info.img_info.size.h,
		(unsigned int)p_base_info->ime_ctrl.pat_paste_info.img_info.lofs,
		(unsigned int)p_base_info->ime_ctrl.pat_paste_cst_info.func_en);

	dump("\r\n%8s %8s %18s %12s %8s %10s\r\n", "path_num", "Enable", "Low_Delay", "one_buf_en", "md_en", "region_en");
	for (i = 0; i < CTL_IPP_IME_PATH_ID_MAX; i++) {
		UINT32 path_enable;
		UINT32 low_dly;

		path_enable = p_base_info->ime_ctrl.out_img[i].enable == ENABLE ? ENABLE : DISABLE;
		if (p_base_info->ime_ctrl.low_delay_enable == ENABLE && p_base_info->ime_ctrl.low_delay_path == i) {
			low_dly = ENABLE;
			dump("%8s %8d %12d(%4d) %12d %8d %10d\r\n",
							ctl_ipp_dbg_imepath_tbl[i],
							(int)path_enable,
							(int)low_dly,
							(int)p_base_info->ime_ctrl.low_delay_bp,
							(int)p_base_info->ime_ctrl.out_img[i].one_buf_mode_enable,
							(int)p_base_info->ime_ctrl.out_img[i].md_enable,
							(int)p_base_info->ime_ctrl.out_img[i].region.enable);
		} else {
			low_dly = DISABLE;
			dump("%8s %8d %18d %12d %8d %10d\r\n",
							ctl_ipp_dbg_imepath_tbl[i],
							(int)path_enable,
							(int)low_dly,
							(int)p_base_info->ime_ctrl.out_img[i].one_buf_mode_enable,
							(int)p_base_info->ime_ctrl.out_img[i].md_enable,
							(int)p_base_info->ime_ctrl.out_img[i].region.enable);
		}
	}

	dump("\r\n%8s %15s %15s %8s %15s %15s %15s %15s %10s %8s %8s\r\n",
			"path_num", "Image size H", "Image size V", "lofs", "Crop start X", "Crop start Y",  "Crop size H", "Crop size V", "format", "flip_en", "h_align");
	for (i = 0; i < CTL_IPP_IME_PATH_ID_MAX; i++) {
		if (p_base_info->ime_ctrl.out_img[i].enable == ENABLE) {
			dump("%8s %15d %15d %8d %15d %15d %15d %15d 0x%.8x %8d %8d\r\n",
								ctl_ipp_dbg_imepath_tbl[i],
								(int)p_base_info->ime_ctrl.out_img[i].size.w,
								(int)p_base_info->ime_ctrl.out_img[i].size.h,
								(int)p_base_info->ime_ctrl.out_img[i].lofs[0],
								(int)p_base_info->ime_ctrl.out_img[i].crp_window.x,
								(int)p_base_info->ime_ctrl.out_img[i].crp_window.y,
								(int)p_base_info->ime_ctrl.out_img[i].crp_window.w,
								(int)p_base_info->ime_ctrl.out_img[i].crp_window.h,
								(unsigned int)p_base_info->ime_ctrl.out_img[i].fmt,
								(int)p_base_info->ime_ctrl.out_img[i].flip_enable,
								(int)p_base_info->ime_ctrl.out_img[i].h_align);
		}
	}

	dump("\r\n%8s %6s %6s %9s %6s %6s\r\n",
			"path_num", "bgn_w", "bgn_h", "bgn_lofs", "ofs_x", "ofs_y");
	for (i = 0; i < CTL_IPP_IME_PATH_ID_MAX; i++) {
		if (p_base_info->ime_ctrl.out_img[i].enable == ENABLE) {
			dump("%8s %6d %6d %9d %6d %6d\r\n",
								ctl_ipp_dbg_imepath_tbl[i],
								(int)p_base_info->ime_ctrl.out_img[i].region.bgn_size.w,
								(int)p_base_info->ime_ctrl.out_img[i].region.bgn_size.h,
								(int)p_base_info->ime_ctrl.out_img[i].region.bgn_lofs,
								(int)p_base_info->ime_ctrl.out_img[i].region.region_ofs.x,
								(int)p_base_info->ime_ctrl.out_img[i].region.region_ofs.y);
		}
	}
}

void ctl_ipp_dbg_hdl_dump_buf(UINT32 hdl, int (*dump)(const char *fmt, ...))
{
	CHAR *ctl_ipp_dbg_imepath_tbl[] = {
		"path1",
		"path2",
		"path3",
		"path4",
		"path5",
		"path6",
		""
	};
	CTL_IPP_HANDLE *p_hdl = (CTL_IPP_HANDLE *) hdl;
	UINT32 i, str_addr;

	dump("\r\n---------crl ipp path order ---------\r\n");
	dump("%8s %8s\r\n", "path", "order");
	for (i = 0; i < CTL_IPP_IME_PATH_ID_MAX; i++) {
		dump("%8s %8d\r\n", ctl_ipp_dbg_imepath_tbl[i], (int)p_hdl->buf_push_order[i]);
	}

	dump("\r\n---------crl ipp pri buf ---------\r\n");
	dump("%10s %12s %10s %10s %10s %10s\r\n", "max_size_w", "max_size_h", "pxlfmt", "fun_enable", "addr_start", "  buf_size");
	dump("%10d %12d 0x%.8x 0x%.8x 0x%.8x 0x%.8x\r\n",
		 (int)p_hdl->private_buf.buf_info.max_size.w,
		 (int)p_hdl->private_buf.buf_info.max_size.h,
		 (unsigned int)p_hdl->private_buf.buf_info.pxlfmt,
		 (unsigned int)p_hdl->private_buf.buf_info.func_en,
		 (unsigned int)p_hdl->bufcfg.start_addr,
		 (unsigned int)p_hdl->bufcfg.size);
	dump("\r\n");
	dump("addr start   addr end   size       name\r\n");


	/* LCA always enable */
	if (p_hdl->private_buf.buf_info.func_en & CTL_IPP_FUNC_YUV_SUBOUT) {
		for (i = 0; i < CTL_IPP_BUF_LCA_NUM; i++) {
			if (p_hdl->private_buf.buf_item.lca[i].size != 0) {
				str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.lca[i].lofs;
				dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
					(unsigned int)str_addr,  (unsigned int)(str_addr + p_hdl->private_buf.buf_item.lca[i].size),
					(unsigned int)p_hdl->private_buf.buf_item.lca[i].size, p_hdl->private_buf.buf_item.lca[i].name);
			}
		}
	}

	if (p_hdl->private_buf.buf_info.func_en & CTL_IPP_FUNC_3DNR) {
		/* MV */
		for (i = 0; i < CTL_IPP_BUF_3DNR_NUM; i++) {
			str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.mv[i].lofs;
			dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
				(unsigned int)str_addr,  (unsigned int)(str_addr + p_hdl->private_buf.buf_item.mv[i].size),
				(unsigned int)p_hdl->private_buf.buf_item.mv[i].size, p_hdl->private_buf.buf_item.mv[i].name);
		}
		/* MS */
		for (i = 0; i < CTL_IPP_BUF_3DNR_NUM; i++) {
			str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.ms[i].lofs;
			dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
				(unsigned int)str_addr,  (unsigned int)(str_addr + p_hdl->private_buf.buf_item.ms[i].size),
				(unsigned int)p_hdl->private_buf.buf_item.ms[i].size, p_hdl->private_buf.buf_item.ms[i].name);
		}
		/* MS ROI */
		for (i = 0; i < CTL_IPP_BUF_3DNR_NUM; i++) {
			if (p_hdl->private_buf.buf_item.ms_roi[i].size != 0) {
				str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.ms_roi[i].lofs;
				dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
					(unsigned int)str_addr, (unsigned int)(str_addr + p_hdl->private_buf.buf_item.ms_roi[i].size),
					(unsigned int)p_hdl->private_buf.buf_item.ms_roi[i].size, p_hdl->private_buf.buf_item.ms_roi[i].name);
			}
		}

		if (p_hdl->private_buf.buf_info.func_en & CTL_IPP_FUNC_3DNR_STA) {
			/* STA */
			for (i = 0; i < CTL_IPP_BUF_3DNR_STA_NUM; i++) {
				if (p_hdl->private_buf.buf_item._3dnr_sta[i].size != 0) {
					str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item._3dnr_sta[i].lofs;
					dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
						(unsigned int)str_addr,  (unsigned int)(str_addr + p_hdl->private_buf.buf_item._3dnr_sta[i].size),
						(unsigned int)p_hdl->private_buf.buf_item._3dnr_sta[i].size, p_hdl->private_buf.buf_item._3dnr_sta[i].name);
				}
			}
		}
	}

	{
		/* defog, always allocate for ipe_lce function */
		for (i = 0; i < CTL_IPP_BUF_DFG_NUM; i++) {
			str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.defog_subout[i].lofs;
			dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
				(unsigned int)str_addr,  (unsigned int)(str_addr + p_hdl->private_buf.buf_item.defog_subout[i].size),
				(unsigned int)p_hdl->private_buf.buf_item.defog_subout[i].size, p_hdl->private_buf.buf_item.defog_subout[i].name);
		}
	}

	if (p_hdl->private_buf.buf_info.func_en & CTL_IPP_FUNC_PM_PIXELIZTION) {
		/* primacy mask */
		for (i = 0; i < CTL_IPP_BUF_PM_NUM; i++) {
			str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.pm[i].lofs;
			dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
				(unsigned int)str_addr,  (unsigned int)(str_addr + p_hdl->private_buf.buf_item.pm[i].size),
				(unsigned int)p_hdl->private_buf.buf_item.pm[i].size, p_hdl->private_buf.buf_item.pm[i].name);
		}
	}

	if (p_hdl->private_buf.buf_info.func_en & CTL_IPP_FUNC_WDR) {
		/* wdr */
		for (i = 0; i < CTL_IPP_BUF_WDR_NUM; i++) {
			str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.wdr_subout[i].lofs;
			dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
				(unsigned int)str_addr, (unsigned int)(str_addr + p_hdl->private_buf.buf_item.wdr_subout[i].size),
				(unsigned int)p_hdl->private_buf.buf_item.wdr_subout[i].size, p_hdl->private_buf.buf_item.wdr_subout[i].name);
		}
	}

	if (p_hdl->private_buf.buf_info.func_en & CTL_IPP_FUNC_VA_SUBOUT) {
		/* va */
		for (i = 0; i < CTL_IPP_BUF_VA_NUM; i++) {
			str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.va[i].lofs;
			dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
				(unsigned int)str_addr, (unsigned int)(str_addr + p_hdl->private_buf.buf_item.va[i].size),
				(unsigned int)p_hdl->private_buf.buf_item.va[i].size, p_hdl->private_buf.buf_item.va[i].name);
		}
	}

	if (p_hdl->flow == CTL_IPP_FLOW_CCIR || p_hdl->flow == CTL_IPP_FLOW_DCE_D2D) {
		/* DUMMY UV */
		for (i = 0; i < CTL_IPP_BUF_DUMMY_UV_NUM; i++) {
			str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.dummy_uv[i].lofs;
			dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
				(unsigned int)str_addr, (unsigned int)(str_addr + p_hdl->private_buf.buf_item.dummy_uv[i].size),
				(unsigned int)p_hdl->private_buf.buf_item.dummy_uv[i].size, p_hdl->private_buf.buf_item.dummy_uv[i].name);
		}
	}

	{
		/* yout */
		for (i = 0; i < CTL_IPP_BUF_YOUT_NUM; i++) {
			if (p_hdl->private_buf.buf_item.yout[i].size != 0) {
				str_addr =  p_hdl->bufcfg.start_addr + p_hdl->private_buf.buf_item.yout[i].lofs;
				dump("0x%.8x ~ 0x%.8x 0x%.8x %s\r\n",
					(unsigned int)str_addr,  (unsigned int)(str_addr + p_hdl->private_buf.buf_item.yout[i].size),
					(unsigned int)p_hdl->private_buf.buf_item.yout[i].size, p_hdl->private_buf.buf_item.yout[i].name);
			}
		}
	}

	/* new failed count */
	dump("\r\n---------crl ipp out buf new fail count---------\r\n");
	for (i = 0; i < CTL_IPP_OUT_PATH_ID_MAX; i++) {
		dump("%s: new failed: %8d; tag failed: %8d\r\n", ctl_ipp_dbg_imepath_tbl[i], (int)p_hdl->buf_new_fail_cnt[i], (int)p_hdl->buf_tag_fail_cnt[i]);
	}
	dump("3dnr reference shift count: %8d, max_frame_diff: %8d\r\n", (int)p_hdl->_3dnr_ref_shift_cnt, (int)p_hdl->max_3dnr_ref_shift_diff);
}

#if 0
#endif

typedef struct {
	CTL_IPP_HANDLE *p_target;
	VOS_FILE fp;
	UINT8 cnt;
	UINT32 **p_reg_buf;

	/* keep callback data for direct mode */
	KDRV_IME_PM_INFO pm_info[KDRV_IME_PM_SET_IDX_MAX];
	UINT32 dtsi_chksum;
} CTL_IPP_DUMP_DTSI_CTL;
static CTL_IPP_DUMP_DTSI_CTL ctl_ipp_dtsi_ctl = {NULL, -1, 0, NULL};

void ctl_ipp_dbg_dump_dtsi_privacy_mask(CTL_IPP_HANDLE *p_hdl, VOS_FILE fd, CHAR *prefix)
{
	KDRV_IME_PM_INFO pm[KDRV_IME_PM_SET_IDX_MAX] = {0};
	CHAR buf[128] = {0};
	UINT32 len = 0;
	UINT32 i = 0;

	for (i = 0; i < KDRV_IME_PM_SET_IDX_MAX; i++) {
		pm[i].set_idx = i;
		if (kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, (KDF_IPP_HANDLE *)p_hdl->kdf_hdl, KDRV_IME_PARAM_IPL_PM, (void *)&pm[i]) != CTL_IPP_E_OK) {
			memset((void *)&pm[i], 0, sizeof(KDRV_IME_PM_INFO));
		}

		ctl_ipp_dtsi_ctl.pm_info[i] = pm[i];


		DBG_DUMP("-## color[%d]: %d,%d,%d-\n",i, ctl_ipp_dtsi_ctl.pm_info[i].color[0], ctl_ipp_dtsi_ctl.pm_info[i].color[1],ctl_ipp_dtsi_ctl.pm_info[i].color[2]);
	}

	len = snprintf(buf, sizeof(buf), "%sprivacy-mask {\r\n", prefix);
	vos_file_write(fd, (void *)buf, len);

	/* enable */
	len = snprintf(buf, sizeof(buf), "%s\tenable = <%d %d %d %d %d %d %d %d>;\r\n", prefix,
			(int)pm[0].enable, (int)pm[1].enable, (int)pm[2].enable, (int)pm[3].enable, (int)pm[4].enable, (int)pm[5].enable, (int)pm[6].enable, (int)pm[7].enable);
	vos_file_write(fd, (void *)buf, len);

	/* coordinate */
	len = snprintf(buf, sizeof(buf), "%s\tcoordinate =\r\n", prefix);
	vos_file_write(fd, (void *)buf, len);
	for (i = 0; i < KDRV_IME_PM_SET_IDX_MAX; i++) {
		if (i == (KDRV_IME_PM_SET_IDX_MAX - 1)) {
			len = snprintf(buf, sizeof(buf), "%s\t\t<%d %d>, <%d %d>, <%d %d>, <%d %d>;\r\n", prefix,
				(int)pm[i].coord[0].x, (int)pm[i].coord[0].y, (int)pm[i].coord[1].x, (int)pm[i].coord[1].y,
				(int)pm[i].coord[2].x, (int)pm[i].coord[2].y, (int)pm[i].coord[3].x, (int)pm[i].coord[3].y);
		} else {
			len = snprintf(buf, sizeof(buf), "%s\t\t<%d %d>, <%d %d>, <%d %d>, <%d %d>,\r\n", prefix,
				(int)pm[i].coord[0].x, (int)pm[i].coord[0].y, (int)pm[i].coord[1].x, (int)pm[i].coord[1].y,
				(int)pm[i].coord[2].x, (int)pm[i].coord[2].y, (int)pm[i].coord[3].x, (int)pm[i].coord[3].y);
		}
		vos_file_write(fd, (void *)buf, len);
	}

	/* alpha */
	len = snprintf(buf, sizeof(buf), "%s\talpha = <%d %d %d %d %d %d %d %d>;\r\n", prefix,
			(int)pm[0].weight, (int)pm[1].weight, (int)pm[2].weight, (int)pm[3].weight, (int)pm[4].weight, (int)pm[5].weight, (int)pm[6].weight, (int)pm[7].weight);
	vos_file_write(fd, (void *)buf, len);

	/* color */
	len = snprintf(buf, sizeof(buf), "%s\tcolor =\r\n", prefix);
	vos_file_write(fd, (void *)buf, len);
	for (i = 0; i < KDRV_IME_PM_SET_IDX_MAX; i++) {
		if (i == (KDRV_IME_PM_SET_IDX_MAX - 1)) {
			len = snprintf(buf, sizeof(buf), "%s\t\t<%d %d %d>;\r\n", prefix,
				(int)pm[i].color[0], (int)pm[i].color[1], (int)pm[i].color[2]);
		} else {
			len = snprintf(buf, sizeof(buf), "%s\t\t<%d %d %d>,\r\n", prefix,
				(int)pm[i].color[0], (int)pm[i].color[1], (int)pm[i].color[2]);
		}
		vos_file_write(fd, (void *)buf, len);
	}

	/* hlw enable */
	len = snprintf(buf, sizeof(buf), "%s\thlw_enable = <%d %d %d %d %d %d %d %d>;\r\n", prefix,
			(int)pm[0].hlw_enable, (int)pm[1].hlw_enable, (int)pm[2].hlw_enable, (int)pm[3].hlw_enable,
			(int)pm[4].hlw_enable, (int)pm[5].hlw_enable, (int)pm[6].hlw_enable, (int)pm[7].hlw_enable);
	vos_file_write(fd, (void *)buf, len);

	/* hlw coordinate */
	len = snprintf(buf, sizeof(buf), "%s\thlw_coordinate =\r\n", prefix);
	vos_file_write(fd, (void *)buf, len);
	for (i = 0; i < KDRV_IME_PM_SET_IDX_MAX; i++) {
		if (i == (KDRV_IME_PM_SET_IDX_MAX - 1)) {
			len = snprintf(buf, sizeof(buf), "%s\t\t<%d %d>, <%d %d>, <%d %d>, <%d %d>;\r\n", prefix,
				(int)pm[i].coord2[0].x, (int)pm[i].coord2[0].y, (int)pm[i].coord2[1].x, (int)pm[i].coord2[1].y,
				(int)pm[i].coord2[2].x, (int)pm[i].coord2[2].y, (int)pm[i].coord2[3].x, (int)pm[i].coord2[3].y);
		} else {
			len = snprintf(buf, sizeof(buf), "%s\t\t<%d %d>, <%d %d>, <%d %d>, <%d %d>,\r\n", prefix,
				(int)pm[i].coord2[0].x, (int)pm[i].coord2[0].y, (int)pm[i].coord2[1].x, (int)pm[i].coord2[1].y,
				(int)pm[i].coord2[2].x, (int)pm[i].coord2[2].y, (int)pm[i].coord2[3].x, (int)pm[i].coord2[3].y);
		}
		vos_file_write(fd, (void *)buf, len);
	}

	len = snprintf(buf, sizeof(buf), "%s};\r\n", prefix);
	vos_file_write(fd, (void *)buf, len);
}

void ctl_ipp_dbg_dump_dtsi2file(UINT32 kdf_hdl)
{
	CTL_IPP_HANDLE *p_hdl;
	UINT32 strlen;
	CHAR strbuf[256];
	UINT32 i = 0;

	if (ctl_ipp_dtsi_ctl.p_target == NULL) {
		return ;
	}

	if (kdf_hdl != ctl_ipp_dtsi_ctl.p_target->kdf_hdl) {
		return ;
	}
	p_hdl = ctl_ipp_dtsi_ctl.p_target;

	/* if p_reg_buf exist --> only write eng register to buffer
		else if fp exist --> write dtsi to file
	*/
	if (ctl_ipp_dtsi_ctl.p_reg_buf) {
		kdrv_ipp_builtin_get_reg_dtsi(ctl_ipp_dtsi_ctl.p_reg_buf);

		for (i = 0; i < KDRV_IME_PM_SET_IDX_MAX; i++) {
			ctl_ipp_dtsi_ctl.pm_info[i].set_idx = i;
			if (kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, (KDF_IPP_HANDLE *)p_hdl->kdf_hdl, KDRV_IME_PARAM_IPL_PM, (void *)&ctl_ipp_dtsi_ctl.pm_info[i]) != CTL_IPP_E_OK) {
				memset((void *)&ctl_ipp_dtsi_ctl.pm_info[i], 0, sizeof(KDRV_IME_PM_INFO));
			}
		}

		ctl_ipp_dtsi_ctl.cnt++;
		ctl_ipp_dtsi_ctl.p_target = NULL;
		return ;
	}

	if (ctl_ipp_dtsi_ctl.fp == (VOS_FILE)(-1)) {
		CTL_IPP_DBG_ERR("no file to dump dtsi\n");
		return ;
	}

	/* write config-k dtsi */
	strlen = snprintf(strbuf, 255, "\t\tconfig-%d {\r\n", ctl_ipp_dtsi_ctl.cnt);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);

	strlen = snprintf(strbuf, 255, "\t\t\thdl_name = \"%s\";\r\n", p_hdl->name);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\tflow = <%d>;\r\n", (int)p_hdl->flow);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\tisp_id = <0x%x>;\r\n", (int)p_hdl->isp_id[CTL_IPP_ALGID_IQ]);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\tfunc_en = <0x%x>;\r\n", (unsigned int)p_hdl->func_en);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t3dnr_ref_path = <0x%x>;\r\n", (unsigned int)p_hdl->ctrl_info.ime_ctrl.tplnr_in_ref_path);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);

	/* private buffer info */
	strlen = snprintf(strbuf, 255, "\t\t\tbuffer_func_en = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_info.func_en);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\tbuffer-size {\r\n");
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t\tlca = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_item.lca[0].size);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t\t3dnr_mv = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_item.mv[0].size);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t\t3dnr_ms = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_item.ms[0].size);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t\t3dnr_ms_roi = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_item.ms_roi[0].size);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t\t3dnr_sta = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_item._3dnr_sta[0].size);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t\tdefog = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_item.defog_subout[0].size);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t\tpm = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_item.pm[0].size);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t\twdr = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_item.wdr_subout[0].size);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t\tva = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_item.va[0].size);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t\tdummy_uv = <0x%x>;\r\n", (unsigned int)p_hdl->private_buf.buf_item.dummy_uv[0].size);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\t};\r\n");
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);

	/* frame rate ctrl dtsi */
	kdrv_ipp_builtin_frc_dump(strbuf, (int)ctl_ipp_dtsi_ctl.fp, "\t\t\t");

	/* privacy mask */
	ctl_ipp_dbg_dump_dtsi_privacy_mask(p_hdl, ctl_ipp_dtsi_ctl.fp, "\t\t\t");

	/* eng register */
	kdrv_ipp_builtin_reg_dump((int)ctl_ipp_dtsi_ctl.fp, "\t\t\t");

	strlen = snprintf(strbuf, 255, "\t\t};\r\n");
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);

	ctl_ipp_dtsi_ctl.cnt++;
	ctl_ipp_dtsi_ctl.p_target = NULL;
}

void ctl_ipp_dbg_dump_dtsi(CHAR *f_name)
{
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_HANDLE *p_hdl;
	UINT32 strlen;
	CHAR strbuf[256];
	UINT32 hdl_num = 0;

	p_hdl = vos_list_entry(ctl_ipp_dbg_hdl_used_head->next, CTL_IPP_HANDLE, list);
	if (p_hdl == NULL) {
		CTL_IPP_DBG_ERR("null handle\r\n");
		return ;
	}

	ctl_ipp_dtsi_ctl.fp = vos_file_open(f_name, O_CREAT|O_WRONLY|O_SYNC, 0);
	if (ctl_ipp_dtsi_ctl.fp == (VOS_FILE)(-1)) {
		CTL_IPP_DBG_ERR("failed in file open:%s\n", f_name);
		return ;
	}

	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		hdl_num++;
	}
	strlen = snprintf(strbuf, 255, "&fastboot {\r\n\tipp {\r\n");
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\tctl {\r\n");
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t\tnum = <%d>;\r\n", (int)hdl_num);
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);
	strlen = snprintf(strbuf, 255, "\t\t};\r\n");
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);

	ctl_ipp_dtsi_ctl.cnt = 0;
	ctl_ipp_dtsi_ctl.p_reg_buf = NULL;
	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);

		ctl_ipp_dtsi_ctl.p_target = p_hdl;
		if (CTL_IPP_IS_DIRECT_FLOW(p_hdl->flow)) {
			/* direct mode has only one handle */
			ctl_ipp_dbg_dump_dtsi2file(p_hdl->kdf_hdl);
			break;
		} else {
			/* wait for d2d flow write dtsi */
			while (ctl_ipp_dtsi_ctl.p_target != NULL) {
				vos_util_delay_ms(10);
			}
		}
	}

	/* closure for &fastboot and ipp */
	strlen = snprintf(strbuf, 255, "\t};\r\n};");
	vos_file_write(ctl_ipp_dtsi_ctl.fp, (void *)strbuf, strlen);

	vos_file_close(ctl_ipp_dtsi_ctl.fp);
	ctl_ipp_dtsi_ctl.fp = -1;
}

void ctl_ipp_dbg_get_dtsi_hdl_list(UINT32 *hdl_list, UINT32 list_size)
{
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_HANDLE *p_hdl;
	UINT32 i = 0;

	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		hdl_list[i] = (UINT32)p_hdl;
		i += 1;
		if (i >= list_size) {
			CTL_IPP_DBG_WRN("list_size(%d) not enough\r\n", list_size);
			break;
		}
	}
}

void ctl_ipp_dbg_get_dtsi_reg(UINT32 hdl, UINT32 cnt, KDRV_IPP_BUILTIN_FDT_GET_INFO* info)
{
	CTL_IPP_HANDLE *p_hdl = (CTL_IPP_HANDLE *)hdl;
	UINT32 i = 0;

	ctl_ipp_dtsi_ctl.cnt = cnt;
	ctl_ipp_dtsi_ctl.p_reg_buf = info->reg_buffer;
	ctl_ipp_dtsi_ctl.p_target = p_hdl;
	if (CTL_IPP_IS_DIRECT_FLOW(p_hdl->flow)) {
		/* direct mode has only one handle */
		ctl_ipp_dbg_dump_dtsi2file(p_hdl->kdf_hdl);

		for (i = 0; i < KDRV_IME_PM_SET_IDX_MAX; i++) {
			ctl_ipp_dtsi_ctl.pm_info[i].set_idx = i;
			if (kdf_ipp_kdrv_get_wrapper(KDF_IPP_ENG_IME, (KDF_IPP_HANDLE *)p_hdl->kdf_hdl, KDRV_IME_PARAM_IPL_PM, (void *)&ctl_ipp_dtsi_ctl.pm_info[i]) != CTL_IPP_E_OK) {
				memset((void *)&ctl_ipp_dtsi_ctl.pm_info[i], 0, sizeof(KDRV_IME_PM_INFO));
			}
		}


	} else {
		/* wait for d2d flow write dtsi */
		while (ctl_ipp_dtsi_ctl.p_target != NULL) {
			vos_util_delay_ms(10);
		}
	}

	for (i = 0; i < KDRV_IME_PM_SET_IDX_MAX; i++)
		info->pm_info[i] = ctl_ipp_dtsi_ctl.pm_info[i];
}

void ctl_ipp_dbg_dtsi_init(void)
{
	KDRV_IPP_BUILTIN_DTSI_CB cb;

	cb.get_hdl_list = ctl_ipp_dbg_get_dtsi_hdl_list;
	cb.get_reg_dtsi = ctl_ipp_dbg_get_dtsi_reg;
	kdrv_ipp_builtin_reg_dtsi_cb(cb);
}

#if 0
#endif

#define CTL_IPP_DBG_SAVEFILE_PATHBASE_LENGTH (64)
static UINT32 ctl_ipp_dbg_savefile_count;
static UINT32 ctl_ipp_dbg_savefile_handle;
static CHAR ctl_ipp_dbg_savefile_path[CTL_IPP_DBG_SAVEFILE_PATHBASE_LENGTH] = CTL_IPP_INT_ROOT_PATH;

void ctl_ipp_dbg_savefile(CHAR *f_name, UINT32 addr, UINT32 size)
{
	VOS_FILE fp;

	if (addr != 0) {
		fp = vos_file_open(f_name, O_CREAT|O_WRONLY|O_SYNC, 0);
		if (fp == (VOS_FILE)(-1)) {
			CTL_IPP_DBG_ERR("failed in file open:%s\n", f_name);
		} else {
			vos_file_write(fp, (void *)addr, size);
			vos_file_close(fp);
			DBG_DUMP("save file %s\r\n", f_name);
		}
	}
}

void ctl_ipp_dbg_saveyuv(UINT32 handle, CTL_IPP_OUT_BUF_INFO *p_buf)
{
	unsigned long loc_cpu_flg;
	CHAR f_name[64];

	loc_cpu(loc_cpu_flg);
	if (handle == ctl_ipp_dbg_savefile_handle &&
		ctl_ipp_dbg_savefile_count > 0) {
		ctl_ipp_dbg_savefile_count -= 1;
		unl_cpu(loc_cpu_flg);

		snprintf(f_name, 64, "%sippout_p%d_%dx%d_fmt0x%.8x_f%lld.RAW",
			ctl_ipp_dbg_savefile_path,
			(int)p_buf->pid,
			(int)p_buf->vdo_frm.size.w,
			(int)p_buf->vdo_frm.size.h,
			(unsigned int)p_buf->vdo_frm.pxlfmt,
			p_buf->vdo_frm.count);
		ctl_ipp_dbg_savefile(f_name, p_buf->buf_addr, p_buf->buf_size);
	} else {
		unl_cpu(loc_cpu_flg);
	}
}

void ctl_ipp_dbg_saveyuv_cfg(CHAR *handle_name, CHAR *filepath, UINT32 count)
{
	#define CTL_IPP_DBG_SAVEYUV_TIMEOUT_MS	(1000)
	unsigned long loc_cpu_flg;
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_HANDLE *p_hdl;
	UINT32 to_cnt;

	vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
		p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
		if (strcmp(p_hdl->name, handle_name) == 0) {
			loc_cpu(loc_cpu_flg);
			ctl_ipp_dbg_savefile_handle = (UINT32)p_hdl;
			ctl_ipp_dbg_savefile_count = count;
			memset((void *)ctl_ipp_dbg_savefile_path, '\0', sizeof(CHAR) * CTL_IPP_DBG_SAVEFILE_PATHBASE_LENGTH);
			strncpy(ctl_ipp_dbg_savefile_path, filepath, (CTL_IPP_DBG_SAVEFILE_PATHBASE_LENGTH - 1));
			unl_cpu(loc_cpu_flg);
		}
	}

	vos_task_delay_ms(500);
	to_cnt = 0;
	while (ctl_ipp_dbg_savefile_count > 0) {
		to_cnt += 1;
		if (to_cnt > (CTL_IPP_DBG_SAVEYUV_TIMEOUT_MS / 100)) {
			loc_cpu(loc_cpu_flg);
			ctl_ipp_dbg_savefile_count = 0;
			ctl_ipp_dbg_savefile_handle = 0;
			unl_cpu(loc_cpu_flg);
			DBG_DUMP("timeout(%d ms)\r\n", CTL_IPP_DBG_SAVEYUV_TIMEOUT_MS);
			break;
		}
		vos_task_delay_ms(100);
	}
}

void ctl_ipp_dbg_save_vdofrm_image(VDO_FRAME *p_vdofrm, CHAR *f_name)
{
	UINT32 file_size;

	if (p_vdofrm->addr[0] != 0) {
		if (VDO_PXLFMT_CLASS(p_vdofrm->pxlfmt) == VDO_PXLFMT_CLASS_RAW ||
			VDO_PXLFMT_CLASS(p_vdofrm->pxlfmt) == VDO_PXLFMT_CLASS_NRX) {
			file_size = p_vdofrm->loff[0] * p_vdofrm->size.h;
		} else {
			file_size = ctl_ipp_util_yuvsize(p_vdofrm->pxlfmt, p_vdofrm->size.w, p_vdofrm->size.h);
		}
		if (file_size != 0) {
			ctl_ipp_dbg_savefile(f_name, p_vdofrm->addr[0], file_size);
		}
	}
}

#if 0
#endif

void ctl_ipp_dbg_set_baseinfo(UINT32 hdl, BOOL flag)
{
	CTL_IPP_HANDLE *p_hdl;
	static UINT32 idx = 0;
	UINT32 idx1 = 0xff00;
	UINT32 i;

	if (flag == TRUE) {
		for (i = 0; i < CTL_IPP_DBG_BASEINFO_NUM; i++) {
			memset((void *)&ctl_ipp_dbg_base_info[idx], 0, sizeof(CTL_IPP_DBG_BASE_INFO));
			memset((void *)&ctl_ipp_dbg_base_info[idx].name, '\0', sizeof(CHAR) * CTL_IPP_HANDLE_NAME_MAX);
		}
	} else {
		p_hdl = (CTL_IPP_HANDLE *) hdl;
		for(i = 0; i < CTL_IPP_DBG_BASEINFO_NUM; i++) {
			if(strcmp(ctl_ipp_dbg_base_info[i].name, p_hdl->name) == 0){
				idx1 = i;
				break;
			}
		}

		if(idx1 == 0xff00) {
			idx1 = idx % CTL_IPP_DBG_BASEINFO_NUM;
			idx++;
		}

		strncpy(ctl_ipp_dbg_base_info[idx1].name, p_hdl->name, (CTL_IPP_HANDLE_NAME_MAX - 1));
		ctl_ipp_dbg_base_info[idx1].flow = p_hdl->flow;
		ctl_ipp_dbg_base_info[idx1].ctl_snd_evt_cnt =  p_hdl->ctl_snd_evt_cnt;
		ctl_ipp_dbg_base_info[idx1].ctl_frm_str_cnt = p_hdl->ctl_frm_str_cnt;
		ctl_ipp_dbg_base_info[idx1].ctl_proc_end_cnt = p_hdl->proc_end_cnt;
		ctl_ipp_dbg_base_info[idx1].ctl_drop_cnt = p_hdl->drop_cnt;
		ctl_ipp_dbg_base_info[idx1].in_buf_re_cnt = p_hdl->in_buf_re_cnt;
		ctl_ipp_dbg_base_info[idx1].in_buf_drop_cnt = p_hdl->in_buf_drop_cnt;
		ctl_ipp_dbg_base_info[idx1].in_buf_re_cb_cnt = p_hdl->in_buf_re_cb_cnt;
		ctl_ipp_dbg_base_info[idx1].in_pro_skip_cnt = p_hdl->in_pro_skip_cnt;
	}
}

#if 0
#endif

#if CTL_IPP_DBG_BUF_LOG_ENABLE
static UINT32 ctl_ipp_dbg_outbuf_cbtime_cnt;
static UINT32 ctl_ipp_dbg_inbuf_cbtime_cnt;
static CTL_IPP_DBG_BUF_CBTIME_LOG ctl_ipp_dbg_outbuf_cbtime_log[CTL_IPP_DBG_BUF_LOG_NUM];
static CTL_IPP_DBG_BUF_CBTIME_LOG ctl_ipp_dbg_inbuf_cbtime_log[CTL_IPP_DBG_BUF_LOG_NUM];

void ctl_ipp_dbg_outbuf_log_set(UINT32 hdl, CTL_IPP_BUF_IO_CFG io_type, CTL_IPP_OUT_BUF_INFO *p_buf, UINT64 ts_start, UINT64 ts_end)
{
	/* cbtime log */
	{
		unsigned long loc_flg;
		UINT32 idx;

		loc_cpu(loc_flg);
		if (ctl_ipp_dbg_outbuf_cbtime_cnt == 0xFFFFFFFF) {
			idx = CTL_IPP_DBG_BUF_LOG_NUM;
		} else {
			idx = ctl_ipp_dbg_outbuf_cbtime_cnt % CTL_IPP_DBG_BUF_LOG_NUM;
			ctl_ipp_dbg_outbuf_cbtime_cnt++;
			if (ctl_ipp_dbg_outbuf_cbtime_cnt == 0xFFFFFFFF) {
				ctl_ipp_dbg_outbuf_cbtime_cnt = 0;
			}
		}
		unl_cpu(loc_flg);

		if (idx < CTL_IPP_DBG_BUF_LOG_NUM) {
			ctl_ipp_dbg_outbuf_cbtime_log[idx].handle = hdl;
			ctl_ipp_dbg_outbuf_cbtime_log[idx].pid = p_buf->pid;
			ctl_ipp_dbg_outbuf_cbtime_log[idx].type = io_type;
			ctl_ipp_dbg_outbuf_cbtime_log[idx].ts_start = ts_start;
			ctl_ipp_dbg_outbuf_cbtime_log[idx].ts_end = ts_end;
			ctl_ipp_dbg_outbuf_cbtime_log[idx].buf_addr = p_buf->buf_addr;
		}
	}
}

void ctl_ipp_dbg_outbuf_cbtime_dump(int (*dump)(const char *fmt, ...))
{
	CHAR *str_io_type[] = {
		"New",
		"Push",
		"Lock",
		"Unlock",
		"Start",
		"End",
		"DramStart",
		"DramEnd",
		""
	};
	CHAR *str_path[] = {
		"PATH_1",
		"PATH_2",
		"PATH_3",
		"PATH_4",
		"PATH_5",
		"PATH_6",
	};
	unsigned long loc_flg;
	UINT32 i, idx, cnt;
	CTL_IPP_DBG_BUF_CBTIME_LOG *p_log;

	loc_cpu(loc_flg);
	cnt = ctl_ipp_dbg_outbuf_cbtime_cnt;
	ctl_ipp_dbg_outbuf_cbtime_cnt = 0xFFFFFFFF;
	unl_cpu(loc_flg);

	dump("---- OUT BUF CALLBACK PROC TIME ----\r\n");
	dump("%10s  %10s%8s%12s%12s%16s%12s\r\n", "handle", "type", "path", "ts_start", "ts_end", "proc_time(us)", "buf_addr");
	for (i = 0; i < CTL_IPP_DBG_BUF_LOG_NUM; i++) {
		if (i > cnt) {
			break;
		}

		idx = (cnt - i - 1) % CTL_IPP_DBG_BUF_LOG_NUM;
		p_log = &ctl_ipp_dbg_outbuf_cbtime_log[idx];
		dump("0x%.8x  %10s%8s%12d%12d%16d  0x%.8x\r\n",
			(unsigned int)p_log->handle, str_io_type[p_log->type], str_path[p_log->pid],
			(int)p_log->ts_start, (int)p_log->ts_end,
			(int)(p_log->ts_end - p_log->ts_start),
			(unsigned int)p_log->buf_addr);
	}

	loc_cpu(loc_flg);
	ctl_ipp_dbg_outbuf_cbtime_cnt = cnt;
	unl_cpu(loc_flg);
}

void ctl_ipp_dbg_inbuf_log_set(UINT32 hdl, UINT32 buf_addr, CTL_IPP_CBEVT_IN_BUF_MSG type, UINT64 ts_start, UINT64 ts_end)
{
	unsigned long loc_flg;
	UINT32 idx;

	loc_cpu(loc_flg);
	if (ctl_ipp_dbg_inbuf_cbtime_cnt == 0xFFFFFFFF) {
		idx = CTL_IPP_DBG_BUF_LOG_NUM;
	} else {
		idx = ctl_ipp_dbg_inbuf_cbtime_cnt % CTL_IPP_DBG_BUF_LOG_NUM;
		ctl_ipp_dbg_inbuf_cbtime_cnt++;
		if (ctl_ipp_dbg_inbuf_cbtime_cnt == 0xFFFFFFFF) {
			ctl_ipp_dbg_inbuf_cbtime_cnt = 0;
		}
	}
	unl_cpu(loc_flg);

	if (idx < CTL_IPP_DBG_BUF_LOG_NUM) {
		ctl_ipp_dbg_inbuf_cbtime_log[idx].handle = hdl;
		ctl_ipp_dbg_inbuf_cbtime_log[idx].buf_addr = buf_addr;
		ctl_ipp_dbg_inbuf_cbtime_log[idx].type = type;
		ctl_ipp_dbg_inbuf_cbtime_log[idx].ts_start = ts_start;
		ctl_ipp_dbg_inbuf_cbtime_log[idx].ts_end = ts_end;
	}
}

void ctl_ipp_dbg_inbuf_cbtime_dump(int (*dump)(const char *fmt, ...))
{
	CHAR *str_io_type[] = {
		"PROC_END",
		"DROP",
		"PROC_START",
		"DIR_PROC_END",
		"DIR_DROP",
		""
	};
	unsigned long loc_flg;
	UINT32 i, idx, cnt;
	CTL_IPP_DBG_BUF_CBTIME_LOG *p_log;

	loc_cpu(loc_flg);
	cnt = ctl_ipp_dbg_inbuf_cbtime_cnt;
	ctl_ipp_dbg_inbuf_cbtime_cnt = 0xFFFFFFFF;
	unl_cpu(loc_flg);

	dump("---- IN BUF CALLBACK PROC TIME ----\r\n");
	dump("%10s  %12s  %10s%12s%12s%16s\r\n", "handle", "type", "addr", "ts_start", "ts_end", "proc_time(us)");
	for (i = 0; i < CTL_IPP_DBG_BUF_LOG_NUM; i++) {
		if (i >= cnt) {
			break;
		}

		idx = (cnt - i - 1) % CTL_IPP_DBG_BUF_LOG_NUM;
		p_log = &ctl_ipp_dbg_inbuf_cbtime_log[idx];
		dump("0x%.8x  %12s  0x%.8x%12d%12d%16d\r\n",
			(unsigned int)p_log->handle, str_io_type[p_log->type], (unsigned int)p_log->buf_addr	,
			p_log->ts_start, p_log->ts_end,
			(p_log->ts_end - p_log->ts_start));
	}

	loc_cpu(loc_flg);
	ctl_ipp_dbg_inbuf_cbtime_cnt = cnt;
	unl_cpu(loc_flg);
}

#else
/* CTL_IPP_DBG_BUF_ENABLE = 0 */
void ctl_ipp_dbg_outbuf_log_set(UINT32 hdl, CTL_IPP_BUF_IO_CFG io_type, CTL_IPP_OUT_BUF_INFO *p_buf, UINT64 ts_start, UINT64 ts_end)
{

}

void ctl_ipp_dbg_outbuf_cbtime_dump(int (*dump)(const char *fmt, ...))
{
	DBG_DUMP("not support when CTL_IPP_DBG_BUF_ENABLE = 0\r\n");
}

void ctl_ipp_dbg_inbuf_log_set(UINT32 hdl, UINT32 buf_addr, CTL_IPP_CBEVT_IN_BUF_MSG type, UINT64 ts_start, UINT64 ts_end)
{

}

void ctl_ipp_dbg_inbuf_cbtime_dump(int (*dump)(const char *fmt, ...))
{
	DBG_DUMP("not support when CTL_IPP_DBG_BUF_ENABLE = 0\r\n");
}
#endif

#if 0
#endif

#define CTL_IPP_DBG_CTXBUF_LOG_NUM (16)
static CTL_IPP_DBG_CTX_BUF_LOG ctl_ipp_dbg_ctxbuf_log[CTL_IPP_DBG_CTXBUF_LOG_NUM];
static UINT32 ctl_ipp_dbg_ctxbuf_cnt;

void ctl_ipp_dbg_ctxbuf_log_set(CHAR *name, CTL_IPP_DBG_CTX_BUF_OP op, UINT32 size, UINT32 addr, UINT32 n)
{
	UINT32 idx;

	idx = ctl_ipp_dbg_ctxbuf_cnt % CTL_IPP_DBG_CTXBUF_LOG_NUM;
	ctl_ipp_dbg_ctxbuf_log[idx].op = op;
	ctl_ipp_dbg_ctxbuf_log[idx].size = size;
	ctl_ipp_dbg_ctxbuf_log[idx].addr = addr;
	ctl_ipp_dbg_ctxbuf_log[idx].cfg_num = n;
	strncpy(ctl_ipp_dbg_ctxbuf_log[idx].name, name, (CTL_IPP_DBG_CTX_BUF_NAME_MAX - 1));
	ctl_ipp_dbg_ctxbuf_cnt++;
}

void ctl_ipp_dbg_ctxbuf_log_dump(int (*dump)(const char *fmt, ...))
{
	CHAR *str_op[] = {
		"QUERY",
		"ALLOC",
		"FREE",
	};
	UINT32 i, idx;
	CTL_IPP_DBG_CTX_BUF_LOG *p_log;

	dump("\r\n---- KFLOW CTX BUFFER INFO ----\r\n");
	dump("%16s  %8s  %10s  %10s  %10s \r\n", "name", "op", "addr", "size", "cfg_num");
	for (i = 0; i < CTL_IPP_DBG_CTXBUF_LOG_NUM; i++) {
		if (i >= ctl_ipp_dbg_ctxbuf_cnt) {
			break;
		}

		idx = (ctl_ipp_dbg_ctxbuf_cnt - i - 1) % CTL_IPP_DBG_CTXBUF_LOG_NUM;
		p_log = &ctl_ipp_dbg_ctxbuf_log[idx];
		dump("%16s  %8s  0x%.8x  0x%.8x  %10d\r\n", p_log->name, str_op[p_log->op], (unsigned int)p_log->addr, (unsigned int)p_log->size, (int)p_log->cfg_num);
	}
}

#if 0
#endif

#if CTL_IPP_DBG_DIRECT_ISR_ENABLE
CTL_IPP_DBG_DIR_ISR_SEQ ctl_ipp_debg_dir_seq[CTL_IPP_DBG_DIRECT_ISR_NUM];
UINT32 ctl_ipp_debg_dir_isr_count = 0;

void ctl_ipp_dbg_set_direct_isr_sequence(UINT32 hdl, CHAR* name, UINT32 addr)
{
	KDF_IPP_HANDLE *p_hdl;

	p_hdl = (KDF_IPP_HANDLE*) hdl;

	strncpy(ctl_ipp_debg_dir_seq[ctl_ipp_debg_dir_isr_count].name, name, (CTL_IPP_DBG_DIR_ISR_NAME_MAX - 1));
	ctl_ipp_debg_dir_seq[ctl_ipp_debg_dir_isr_count].cnt = p_hdl->fra_sta_with_sop_cnt;
	ctl_ipp_debg_dir_seq[ctl_ipp_debg_dir_isr_count].addr = addr;
	ctl_ipp_debg_dir_isr_count++;
	if(ctl_ipp_debg_dir_isr_count >= CTL_IPP_DBG_DIRECT_ISR_NUM) {
		ctl_ipp_debg_dir_isr_count = 0;
	}
}

void ctl_ipp_dbg_dump_direct_isr_sequence(UINT32 hdl)
{
	CTL_IPP_HANDLE *p_hdl;
	KDF_IPP_HANDLE *p_k_hdl;
	UINT32 i;

	p_hdl = (CTL_IPP_HANDLE *) hdl;
	p_k_hdl = (KDF_IPP_HANDLE *) p_hdl->kdf_hdl;

	DBG_DUMP("tag fail %d \r\n", p_k_hdl->fra_sta_with_sop_cnt);
	DBG_DUMP("%10s %5s %12s", "name", "cnt", "addr");
	for (i = 0; i < CTL_IPP_DBG_DIRECT_ISR_NUM; i++ ) {
		DBG_DUMP("%10s %5d 0x%10x", ctl_ipp_debg_dir_seq[i].name, ctl_ipp_debg_dir_seq[i].cnt, ctl_ipp_debg_dir_seq[i].addr);
	}
}
#else
void ctl_ipp_dbg_set_direct_isr_sequence(UINT32 hdl, CHAR* name, UINT32 addr) {}
void ctl_ipp_dbg_dump_direct_isr_sequence(UINT32 hdl) {}
#endif

static UINT32 ctl_ipp_dbg_isp_mode = 0;

void ctl_ipp_dbg_set_isp_mode(UINT32 mode)
{
	ctl_ipp_dbg_isp_mode = mode;
}

UINT32 ctl_ipp_dbg_get_isp_mode(void)
{
	return ctl_ipp_dbg_isp_mode;
}

#if 0
#endif

#define CTL_IPP_DBG_HANDLE_HASH(hdl, n) ((hdl / sizeof(CTL_IPP_HANDLE)) % n)

static CTL_IPP_DBG_PRIMASK_CTL ctl_ipp_dbg_pm_ctl = {DISABLE,
	/* colors */
	{{76, 84, 255}, {149, 43, 21}, {29, 255, 107}, {105, 212, 234}, {255, 0, 148}, {178, 171, 0}, {165, 106, 191}, {142, 191, 117}},

	/* mask size*/
	{160, 160},
};

INT32 ctl_ipp_dbg_primask_cb(UINT32 msg, void *in, void *out)
{
	CTL_IPP_PM_CB_INPUT_INFO *p_in;
	CTL_IPP_PM_CB_OUTPUT_INFO *p_out;
	CTL_IPP_PM *p_mask;
	UINT32 w, h;

	if (!ctl_ipp_dbg_pm_ctl.enable) {
		return 0;
	}

	p_in = (CTL_IPP_PM_CB_INPUT_INFO *)in;
	p_out = (CTL_IPP_PM_CB_OUTPUT_INFO *)out;

	w = ((p_in->img_size.w / 4) < ctl_ipp_dbg_pm_ctl.msk_size.w) ? (p_in->img_size.w / 4) : ctl_ipp_dbg_pm_ctl.msk_size.w;
	h = ((p_in->img_size.h / 4) < ctl_ipp_dbg_pm_ctl.msk_size.h) ? (p_in->img_size.h / 4) : ctl_ipp_dbg_pm_ctl.msk_size.h;

	p_mask = &p_out->mask[0];
	p_mask->func_en = ENABLE;
	p_mask->msk_type = CTL_IPP_PM_MASK_TYPE_YUV;
	p_mask->color[0] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][0];
	p_mask->color[1] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][1];
	p_mask->color[2] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][2];
	p_mask->alpha_weight = 255;
	p_mask->pm_coord[0].x = 0;
	p_mask->pm_coord[0].y = 0;
	p_mask->pm_coord[1].x = p_mask->pm_coord[0].x + w;
	p_mask->pm_coord[1].y = p_mask->pm_coord[0].y;
	p_mask->pm_coord[2].x = p_mask->pm_coord[0].x + w;
	p_mask->pm_coord[2].y = p_mask->pm_coord[0].y + h;
	p_mask->pm_coord[3].x = p_mask->pm_coord[0].x;
	p_mask->pm_coord[3].y = p_mask->pm_coord[0].y + h;

	p_mask = &p_out->mask[1];
	p_mask->func_en = ENABLE;
	p_mask->msk_type = CTL_IPP_PM_MASK_TYPE_YUV;
	p_mask->color[0] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][0];
	p_mask->color[1] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][1];
	p_mask->color[2] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][2];
	p_mask->alpha_weight = 255;
	p_mask->pm_coord[0].x = p_in->img_size.w - w;
	p_mask->pm_coord[0].y = 0;
	p_mask->pm_coord[1].x = p_mask->pm_coord[0].x + w;
	p_mask->pm_coord[1].y = p_mask->pm_coord[0].y;
	p_mask->pm_coord[2].x = p_mask->pm_coord[0].x + w;
	p_mask->pm_coord[2].y = p_mask->pm_coord[0].y + h;
	p_mask->pm_coord[3].x = p_mask->pm_coord[0].x;
	p_mask->pm_coord[3].y = p_mask->pm_coord[0].y + h;

	p_mask = &p_out->mask[2];
	p_mask->func_en = ENABLE;
	p_mask->msk_type = CTL_IPP_PM_MASK_TYPE_YUV;
	p_mask->color[0] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][0];
	p_mask->color[1] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][1];
	p_mask->color[2] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][2];
	p_mask->alpha_weight = 255;
	p_mask->pm_coord[0].x = 0;
	p_mask->pm_coord[0].y = p_in->img_size.h - h;
	p_mask->pm_coord[1].x = p_mask->pm_coord[0].x + w;
	p_mask->pm_coord[1].y = p_mask->pm_coord[0].y;
	p_mask->pm_coord[2].x = p_mask->pm_coord[0].x + w;
	p_mask->pm_coord[2].y = p_mask->pm_coord[0].y + h;
	p_mask->pm_coord[3].x = p_mask->pm_coord[0].x;
	p_mask->pm_coord[3].y = p_mask->pm_coord[0].y + h;

	p_mask = &p_out->mask[3];
	p_mask->func_en = ENABLE;
	p_mask->msk_type = CTL_IPP_PM_MASK_TYPE_YUV;
	p_mask->color[0] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][0];
	p_mask->color[1] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][1];
	p_mask->color[2] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][2];
	p_mask->alpha_weight = 255;
	p_mask->pm_coord[0].x = p_in->img_size.w - w;
	p_mask->pm_coord[0].y = p_in->img_size.h - h;
	p_mask->pm_coord[1].x = p_mask->pm_coord[0].x + w;
	p_mask->pm_coord[1].y = p_mask->pm_coord[0].y;
	p_mask->pm_coord[2].x = p_mask->pm_coord[0].x + w;
	p_mask->pm_coord[2].y = p_mask->pm_coord[0].y + h;
	p_mask->pm_coord[3].x = p_mask->pm_coord[0].x;
	p_mask->pm_coord[3].y = p_mask->pm_coord[0].y + h;

	p_mask = &p_out->mask[4];
	p_mask->func_en = ENABLE;
	p_mask->msk_type = CTL_IPP_PM_MASK_TYPE_YUV;
	p_mask->color[0] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][0];
	p_mask->color[1] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][1];
	p_mask->color[2] = ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH(p_in->ctl_ipp_handle, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][2];
	p_mask->alpha_weight = 255;
	p_mask->pm_coord[0].x = (p_in->img_size.w - w) / 2;
	p_mask->pm_coord[0].y = (p_in->img_size.h - h) / 2;
	p_mask->pm_coord[1].x = p_mask->pm_coord[0].x + w;
	p_mask->pm_coord[1].y = p_mask->pm_coord[0].y;
	p_mask->pm_coord[2].x = p_mask->pm_coord[0].x + w;
	p_mask->pm_coord[2].y = p_mask->pm_coord[0].y + h;
	p_mask->pm_coord[3].x = p_mask->pm_coord[0].x;
	p_mask->pm_coord[3].y = p_mask->pm_coord[0].y + h;

	return 0;
}

void ctl_ipp_dbg_primask_en(UINT32 en)
{
	CTL_IPP_LIST_HEAD *p_list;
	CTL_IPP_LIST_HEAD *n;
	CTL_IPP_HANDLE *p_hdl;

	if (en) {
		vos_list_for_each_safe(p_list, n, ctl_ipp_dbg_hdl_used_head) {
			p_hdl = vos_list_entry(p_list, CTL_IPP_HANDLE, list);
			DBG_DUMP("%16s will use mask yuv color (%d, %d, %d)\r\n", p_hdl->name,
				(int)ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH((UINT32)p_hdl, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][0],
				(int)ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH((UINT32)p_hdl, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][1],
				(int)ctl_ipp_dbg_pm_ctl.color[CTL_IPP_DBG_HANDLE_HASH((UINT32)p_hdl, CTL_IPP_DBG_PRIMASK_COLOR_POOL)][2]);
		}
	}
	ctl_ipp_dbg_pm_ctl.enable = en;
}

void ctl_ipp_dbg_primask_cfg(UINT32 cfg, void *data)
{
	switch (cfg) {
	case CTL_IPP_DBG_PRIMASK_CFG_SIZE:
		ctl_ipp_dbg_pm_ctl.msk_size = *(USIZE *)data;
		if (ctl_ipp_dbg_pm_ctl.msk_size.w == 0) {
			ctl_ipp_dbg_pm_ctl.msk_size.w = 80;
		}
		if (ctl_ipp_dbg_pm_ctl.msk_size.h == 0) {
			ctl_ipp_dbg_pm_ctl.msk_size.h = 80;
		}
		DBG_DUMP("DBG MASK_SIZE(%d %d)\r\n", (int)ctl_ipp_dbg_pm_ctl.msk_size.w, (int)ctl_ipp_dbg_pm_ctl.msk_size.h);
		break;
	default:
		break;
	}
}

#if 0
#endif

#define CTL_IPP_BUF_ADDR_MAKE_WITH_VA(addr) CTL_IPP_BUF_ADDR_MAKE((addr), nvtmpp_sys_va2pa(addr)) // 520, 560 only
static CTL_IPP_DBG_DMA_WP_BUF_TYPE ctl_ipp_dbg_dma_wp_en = CTL_IPP_DBG_DMA_WP_BUF_DISABLE; // bit field of wp buf type
static CTL_IPP_BUF_ADDR ctl_ipp_dbg_dma_wp_addr[WPSET_COUNT] = {0}; // all current wp addr. 0 means no used
static CTL_IPP_DBG_DMA_WP_BUF_TYPE ctl_ipp_dbg_dma_wp_type[WPSET_COUNT] = {CTL_IPP_DBG_DMA_WP_BUF_DISABLE}; // all current wp type, for checking re-protect issue
static CTL_IPP_DBG_DMA_WP_LEVEL ctl_ipp_dbg_dma_wp_level = CTL_IPP_DBG_DMA_WP_LEVEL_AUTO;
CHAR *ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_TYPE type)
{
	switch (type) {
	case CTL_IPP_DBG_DMA_WP_BUF_DISABLE:
		return "DISABLE";
	case CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0:
		return "IFE_IN0";
	case CTL_IPP_DBG_DMA_WP_BUF_IFE_IN1:
		return "IFE_IN1";
	case CTL_IPP_DBG_DMA_WP_BUF_DCE_IN:
		return "DCE_IN";
	case CTL_IPP_DBG_DMA_WP_BUF_IPE_IN:
		return "IPE_IN";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_IN:
		return "IME_IN";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_REF_IN:
		return "IME_REF_IN";
	case CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBIN_WDR:
		return "WDR_IN";
	case CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBIN_DEFOG:
		return "DEFOG_IN";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBIN_LCA:
		return "LCA_IN";
	case CTL_IPP_DBG_DMA_WP_BUF_DCE_OUT:
		return "DCE_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IPE_OUT:
		return "IPE_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_P1_OUT:
		return "IME_P1_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_P2_OUT:
		return "IME_P2_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_P3_OUT:
		return "IME_P3_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_P4_OUT:
		return "IME_P4_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_P5_OUT:
		return "IME_P5_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBOUT_WDR:
		return "WDR_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_DEFOG:
		return "DEFOG_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_VA:
		return "IPE_VA_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_LCA:
		return "LCA_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MV:
		return "MV_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS:
		return "MS_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS_ROI:
		return "MS_ROI_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_PM:
		return "PM_OUT";
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_3DNR_STA:
		return "3DNR_OUT";
	default:
		CTL_IPP_DBG_ERR("type=%llx\r\n", type);
		return "Unknown";
	}
}

CHAR *ctl_ipp_dbg_dma_wp_prot_lv_str(CTL_IPP_DBG_DMA_WP_LEVEL lv)
{
	switch (lv) {
	case CTL_IPP_DBG_DMA_WP_LEVEL_UNWRITE:
		return "UNWRITE";
	case CTL_IPP_DBG_DMA_WP_LEVEL_DETECT:
		return "DETECT";
	case CTL_IPP_DBG_DMA_WP_LEVEL_UNREAD:
		return "UNREAD";
	case CTL_IPP_DBG_DMA_WP_LEVEL_UNRW:
		return "UNRW";
	case CTL_IPP_DBG_DMA_WP_LEVEL_AUTO:
	default:
		return "AUTO";
	}
}

ER ctl_ipp_dbg_dma_wp_set_enable(BOOL en, CTL_IPP_DBG_DMA_WP_BUF_TYPE type)
{
	if (type & ~CTL_IPP_DBG_DMA_WP_BUF_ALL) {
		CTL_IPP_DBG_ERR("invalid type 0x%llx\r\n", type);
		return E_SYS;
	}

	if (en) { // enable
		ctl_ipp_dbg_dma_wp_en |= type;
	} else { // disable
		ctl_ipp_dbg_dma_wp_clear(); // clear protect before disable (unsafe, just for test)
		ctl_ipp_dbg_dma_wp_en &= ~type;
	}
	return E_OK;
}

CTL_IPP_DBG_DMA_WP_BUF_TYPE ctl_ipp_dbg_dma_wp_get_enable(void)
{
	return ctl_ipp_dbg_dma_wp_en;
}

ER ctl_ipp_dbg_dma_wp_set_prot_level(CTL_IPP_DBG_DMA_WP_LEVEL level)
{
	ctl_ipp_dbg_dma_wp_level = level;
	return E_OK;
}

static ER ctl_ipp_dbg_dma_wp_set_attr(DMA_WRITEPROT_ATTR *attrib, CTL_IPP_DBG_DMA_WP_BUF_TYPE type, CTL_IPP_BUF_ADDR addr, UINT32 size, CTL_IPP_DBG_DMA_WP_BUF_EXT *ext_data)
{
	memset((void *)&attrib->mask, 0xff, sizeof(DMA_CH_MSK));

	attrib->protect_rgn_attr[DMA_PROT_RGN0].en = ENABLE;
	attrib->protect_rgn_attr[DMA_PROT_RGN0].starting_addr = addr.pa;
	attrib->protect_rgn_attr[DMA_PROT_RGN0].size = size;

	switch (type) {
	case CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0:
	case CTL_IPP_DBG_DMA_WP_BUF_DCE_IN:
	case CTL_IPP_DBG_DMA_WP_BUF_IPE_IN:
	case CTL_IPP_DBG_DMA_WP_BUF_IME_IN:
	case CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBIN_WDR:
	case CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBIN_DEFOG:
		attrib->level = DMA_WPLEL_UNWRITE;
		break;

	case CTL_IPP_DBG_DMA_WP_BUF_IFE_IN1:
		attrib->level = DMA_WPLEL_UNWRITE;
		if (CTL_IPP_IS_DIRECT_FLOW(ext_data->flow)) {
			attrib->mask.SIE2_0 = 0; // direct mode shdr SIE write into and IFE read from frame buffer at the same time
			attrib->mask.SIE2_1 = 0;
			attrib->mask.SIE2_2 = 0;
			attrib->mask.SIE2_3 = 0;
		}
		break;

	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBIN_LCA:
		attrib->level = DMA_WPLEL_UNWRITE;
		if (CTL_IPP_IS_DIRECT_FLOW(ext_data->flow)) {
			attrib->mask.IME_D = 0; // when CTL_IPP_SINGLE_BUFFER_ENABLE is ENABLE, IME LCA subout may read/write into same buffer address
		}
		break;

	case CTL_IPP_DBG_DMA_WP_BUF_IME_REF_IN:
		attrib->level = DMA_WPLEL_UNWRITE;
		attrib->mask.IME_0 = 0; // IME 3DNR ref path may read/write into same buffer address
		attrib->mask.IME_1 = 0;
		attrib->mask.IME_2 = 0;
		attrib->mask.IME_3 = 0;
		attrib->mask.IME_4 = 0;
		attrib->mask.IME_5 = 0;
		attrib->mask.IME_6 = 0;
		attrib->mask.IME_7 = 0;
		attrib->mask.IME_8 = 0;
		attrib->mask.IME_9 = 0;
		attrib->mask.IME_A = 0;
		attrib->mask.IME_B = 0;
		attrib->mask.IME_C = 0;
		attrib->mask.IME_D = 0;
		attrib->mask.IME_E = 0;
		attrib->mask.IME_F = 0;
		attrib->mask.IME_10 = 0;
		attrib->mask.IME_11 = 0;
		attrib->mask.IME_12 = 0;
		attrib->mask.IME_13 = 0;
		attrib->mask.IME_14 = 0;
		attrib->mask.IME_15 = 0;
		attrib->mask.IME_16 = 0;
		attrib->mask.IME_17 = 0;
		break;

	case CTL_IPP_DBG_DMA_WP_BUF_DCE_OUT:
	case CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBOUT_WDR:
		attrib->level = DMA_RWPLEL_UNRW;
		attrib->mask.DCE_0 = 0;
		attrib->mask.DCE_1 = 0;
		attrib->mask.DCE_2 = 0;
		attrib->mask.DCE_3 = 0;
		attrib->mask.DCE_4 = 0;
		attrib->mask.DCE_5 = 0;
		attrib->mask.DCE_6 = 0;
		attrib->mask.DCE_7 = 0;
		break;

	case CTL_IPP_DBG_DMA_WP_BUF_IPE_OUT:
	case CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_DEFOG:
	case CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_VA:
		attrib->level = DMA_RWPLEL_UNRW;
		attrib->mask.IPE_0 = 0;
		attrib->mask.IPE_1 = 0;
		attrib->mask.IPE_2 = 0;
		attrib->mask.IPE_3 = 0;
		attrib->mask.IPE_4 = 0;
		attrib->mask.IPE_5 = 0;
		attrib->mask.IPE_6 = 0;
		break;

	case CTL_IPP_DBG_DMA_WP_BUF_IME_P1_OUT:
	case CTL_IPP_DBG_DMA_WP_BUF_IME_P2_OUT:
	case CTL_IPP_DBG_DMA_WP_BUF_IME_P3_OUT:
	case CTL_IPP_DBG_DMA_WP_BUF_IME_P4_OUT:
	case CTL_IPP_DBG_DMA_WP_BUF_IME_P5_OUT:
		if (ext_data->one_buf_mode_en) { // one buffer mode share yuv buffer with codec, so deny write op. only
			attrib->level = DMA_WPLEL_UNWRITE;
		} else {
			attrib->level = DMA_RWPLEL_UNRW;
		}
		attrib->mask.IME_0 = 0;
		attrib->mask.IME_1 = 0;
		attrib->mask.IME_2 = 0;
		attrib->mask.IME_3 = 0;
		attrib->mask.IME_4 = 0;
		attrib->mask.IME_5 = 0;
		attrib->mask.IME_6 = 0;
		attrib->mask.IME_7 = 0;
		attrib->mask.IME_8 = 0;
		attrib->mask.IME_9 = 0;
		attrib->mask.IME_A = 0;
		attrib->mask.IME_B = 0;
		attrib->mask.IME_C = 0;
		attrib->mask.IME_D = 0;
		attrib->mask.IME_E = 0;
		attrib->mask.IME_F = 0;
		attrib->mask.IME_10 = 0;
		attrib->mask.IME_11 = 0;
		attrib->mask.IME_12 = 0;
		attrib->mask.IME_13 = 0;
		attrib->mask.IME_14 = 0;
		attrib->mask.IME_15 = 0;
		attrib->mask.IME_16 = 0;
		attrib->mask.IME_17 = 0;
		break;

	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_LCA:
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MV:
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS:
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS_ROI:
	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_3DNR_STA:
		attrib->level = DMA_RWPLEL_UNRW;
		attrib->mask.IME_0 = 0;
		attrib->mask.IME_1 = 0;
		attrib->mask.IME_2 = 0;
		attrib->mask.IME_3 = 0;
		attrib->mask.IME_4 = 0;
		attrib->mask.IME_5 = 0;
		attrib->mask.IME_6 = 0;
		attrib->mask.IME_7 = 0;
		attrib->mask.IME_8 = 0;
		attrib->mask.IME_9 = 0;
		attrib->mask.IME_A = 0;
		attrib->mask.IME_B = 0;
		attrib->mask.IME_C = 0;
		attrib->mask.IME_D = 0;
		attrib->mask.IME_E = 0;
		attrib->mask.IME_F = 0;
		attrib->mask.IME_10 = 0;
		attrib->mask.IME_11 = 0;
		attrib->mask.IME_12 = 0;
		attrib->mask.IME_13 = 0;
		attrib->mask.IME_14 = 0;
		attrib->mask.IME_15 = 0;
		attrib->mask.IME_16 = 0;
		attrib->mask.IME_17 = 0;
		break;

	case CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_PM:
		attrib->level = DMA_WPLEL_UNWRITE; // ise write and ime read at same time
		attrib->mask.ISE_0 = 0;
		attrib->mask.ISE_1 = 0;
		attrib->mask.ISE_2 = 0;
		break;

	default:
		// type == CTL_IPP_DBG_DMA_WP_BUF_DISABLE
		return E_ID;
	}

	// if not auto, set level from user input
	if (ctl_ipp_dbg_dma_wp_level != CTL_IPP_DBG_DMA_WP_LEVEL_AUTO) {
		attrib->level = ctl_ipp_dbg_dma_wp_level;
	}

	return E_OK;
}

ER ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_TYPE type, CTL_IPP_BUF_ADDR addr, UINT32 size, CTL_IPP_DBG_DMA_WP_BUF_EXT *ext_data)
{
	unsigned long loc_cpu_flg;
	DMA_WRITEPROT_ATTR attrib = {0};
	UINT32 i_wp_set;

	if (ctl_ipp_dbg_dma_wp_en == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
		return E_OK;
	}

	if (type & ~CTL_IPP_DBG_DMA_WP_BUF_ALL) {
		CTL_IPP_DBG_ERR("invalid type 0x%llx\r\n", type);
		return E_PAR;
	}

	if (CTL_IPP_BUF_ADDR_IS_ZERO(addr)) {
		CTL_IPP_DBG_WRN("type 0x%llx addr zero (va=%lx, pa=%lx)\r\n", type, addr.va, addr.pa);
		return E_PAR;
	}

	if (ext_data == NULL) {
		CTL_IPP_DBG_ERR("NULL ext_data\r\n");
		return E_PAR;
	}

	//CTL_IPP_DBG_DUMP("type %s (0x%08x, all 0x%08x), addr 0x%lx - 0x%lx  %s\r\n", ctl_ipp_dbg_dma_wp_buf_type_str(type), type, ctl_ipp_dbg_dma_wp_en, addr, addr + size, (type & ctl_ipp_dbg_dma_wp_en) ? "" : "... Skip");
	type &= ctl_ipp_dbg_dma_wp_en;

	// configure DMA_WRITEPROT_ATTR
	if (ctl_ipp_dbg_dma_wp_set_attr(&attrib, type, addr, size, ext_data) != E_OK) {
		// all disable, skip
		return E_OK;
	}

	loc_cpu(loc_cpu_flg);

	// skip protect if buffer address and type is already exist in queue
	for (i_wp_set = 0; i_wp_set < WPSET_COUNT; i_wp_set++) {
		if (CTL_IPP_BUF_ADDR_IS_EQUAL(addr, ctl_ipp_dbg_dma_wp_addr[i_wp_set]) && (type == ctl_ipp_dbg_dma_wp_type[i_wp_set])) {
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_DBG, "[set%u re-protect] type %s (0x%llx, all 0x%llx), addr va 0x%lx - 0x%lx, pa 0x%lx - 0x%lx (0x%x)\r\n", i_wp_set, ctl_ipp_dbg_dma_wp_buf_type_str(type), type, ctl_ipp_dbg_dma_wp_en, addr.va, addr.va + size, addr.pa, addr.pa + size, size);
			unl_cpu(loc_cpu_flg);
			return E_OK;
		}
	}

	for (i_wp_set = 0; i_wp_set < WPSET_COUNT; i_wp_set++) {
		if (CTL_IPP_BUF_ADDR_IS_ZERO(ctl_ipp_dbg_dma_wp_addr[i_wp_set])) {
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_DBG, "[set%u] type %s (0x%llx, all 0x%llx), addr va 0x%lx - 0x%lx, pa 0x%lx - 0x%lx (0x%x)\r\n", i_wp_set, ctl_ipp_dbg_dma_wp_buf_type_str(type), type, ctl_ipp_dbg_dma_wp_en, addr.va, addr.va + size, addr.pa, addr.pa + size, size);
			ctl_ipp_dbg_dma_wp_addr[i_wp_set] = addr;
			ctl_ipp_dbg_dma_wp_type[i_wp_set] = type;
			break;
		}
	}

	if (i_wp_set == WPSET_COUNT) {
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_DBG, "[queue full] type %s (0x%llx, all 0x%llx), addr va 0x%lx - 0x%lx, pa 0x%lx - 0x%lx (0x%x)\r\n", ctl_ipp_dbg_dma_wp_buf_type_str(type), type, ctl_ipp_dbg_dma_wp_en, addr.va, addr.va + size, addr.pa, addr.pa + size, size);
		unl_cpu(loc_cpu_flg);
		return E_OK;
	}

	arb_enable_wp(DDR_ARB_1, i_wp_set, &attrib);

	unl_cpu(loc_cpu_flg);

	return E_OK;
}

ER ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_TYPE type, CTL_IPP_BUF_ADDR addr)
{
	unsigned long loc_cpu_flg;
	UINT32 i_wp_set;

	if (ctl_ipp_dbg_dma_wp_en == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
		return E_OK;
	}

	if (CTL_IPP_BUF_ADDR_IS_ZERO(addr)) {
		CTL_IPP_DBG_WRN("%s addr zero (va=%lx, pa=%lx)\r\n", ctl_ipp_dbg_dma_wp_buf_type_str(type), addr.va, addr.pa);
		return E_PAR;
	}

	loc_cpu(loc_cpu_flg);

	for (i_wp_set = 0; i_wp_set < WPSET_COUNT; i_wp_set++) {
		if (CTL_IPP_BUF_ADDR_IS_EQUAL(addr, ctl_ipp_dbg_dma_wp_addr[i_wp_set]) && (type == ctl_ipp_dbg_dma_wp_type[i_wp_set])) {
			CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_DBG, "[set%u] type %s  addr va=0x%lx pa=0x%lx\r\n", i_wp_set, ctl_ipp_dbg_dma_wp_buf_type_str(type), addr.va, addr.pa);
			ctl_ipp_dbg_dma_wp_addr[i_wp_set] = CTL_IPP_BUF_ADDR_MAKE(0, 0);
			ctl_ipp_dbg_dma_wp_type[i_wp_set] = CTL_IPP_DBG_DMA_WP_BUF_DISABLE;
			break;
		}
	}

	if (i_wp_set == WPSET_COUNT) {
		CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_DBG, "[no addr] type %s  addr va=0x%lx pa=0x%lx\r\n", ctl_ipp_dbg_dma_wp_buf_type_str(type), addr.va, addr.pa);
		unl_cpu(loc_cpu_flg);
		return E_OK;
	}

	arb_disable_wp(DDR_ARB_1, i_wp_set);

	unl_cpu(loc_cpu_flg);

	return E_OK;
}

ER ctl_ipp_dbg_dma_wp_proc_start(CTL_IPP_INFO_LIST_ITEM *ctrl_info)
{
	CTL_IPP_HANDLE *ctrl_hdl;
	CTL_IPP_BASEINFO *p_base;
	CTL_IPP_IME_OUT_IMG *p_ime_path;
	CTL_IPP_DBG_DMA_WP_BUF_EXT ext_data = {0};
	UINT32 i, frm_num;

	if (ctl_ipp_dbg_dma_wp_en == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
		return E_OK;
	}

	if (ctrl_info == NULL) {
		CTL_IPP_DBG_ERR("NULL info\r\n");
		return E_SYS;
	}

	ctrl_hdl = (CTL_IPP_HANDLE *)ctrl_info->owner;
	p_base = &ctrl_info->info;

	if (ctrl_hdl == NULL) {
		CTL_IPP_DBG_ERR("NULL hdl\r\n");
		return E_SYS;
	}

	/* input frame */
	switch (ctrl_hdl->flow) {
	case CTL_IPP_FLOW_RAW:
	case CTL_IPP_FLOW_CAPTURE_RAW:
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ife_ctrl.in_addr[0]), p_base->ife_ctrl.in_lofs[0]*p_base->ife_ctrl.in_size.h, &ext_data) != E_OK) {
			CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0));
		}
		if (p_base->ife_ctrl.shdr_enable) {
			frm_num = VDO_PXLFMT_PLANE(p_base->ife_ctrl.in_fmt);
			for (i = 1; i < frm_num; i++) {
				if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0 << i, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ife_ctrl.in_addr[i]), p_base->ife_ctrl.in_lofs[i]*p_base->ife_ctrl.in_size.h, &ext_data) != E_OK) {
					CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0 << i));
				}
			}
		}
		break;

	case CTL_IPP_FLOW_DIRECT_RAW:
		if (p_base->ife_ctrl.shdr_enable) {
			frm_num = VDO_PXLFMT_PLANE(p_base->ife_ctrl.in_fmt);
			for (i = 1; i < frm_num; i++) {
				ext_data.flow = ctrl_hdl->flow;
				if (CTL_IPP_IS_DIRECT_FLOW(ext_data.flow)) {
					CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_DBG, "SHDR in direct\r\n");
				}
				if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0 << i, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ife_ctrl.in_addr[i]), p_base->ife_ctrl.in_lofs[i]*p_base->ife_ctrl.in_size.h, &ext_data) != E_OK) {
					CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0 << i));
				}
			}
		}
		break;

	case CTL_IPP_FLOW_CCIR:
	case CTL_IPP_FLOW_CAPTURE_CCIR:
	case CTL_IPP_FLOW_DCE_D2D:
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_DCE_IN, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->dce_ctrl.in_addr[0]), ctl_ipp_util_yuvsize(p_base->dce_ctrl.in_fmt, p_base->dce_ctrl.in_lofs[0], p_base->dce_ctrl.in_size.h), &ext_data) != E_OK) {
			CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_DCE_IN));
		}
		break;

	case CTL_IPP_FLOW_IME_D2D:
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IME_IN, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.in_addr[0]), ctl_ipp_util_yuvsize(p_base->ime_ctrl.in_fmt, p_base->ime_ctrl.in_lofs[0], p_base->ime_ctrl.in_size.h), &ext_data) != E_OK) {
			CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IME_IN));
		}
		break;

	case CTL_IPP_FLOW_IPE_D2D:
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IPE_IN, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ipe_ctrl.in_addr[0]), ctl_ipp_util_yuvsize(p_base->ipe_ctrl.in_fmt, p_base->ipe_ctrl.in_lofs[0], p_base->ipe_ctrl.in_size.h), &ext_data) != E_OK) {
			CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IPE_IN));
		}
		break;

	default:
		CTL_IPP_DBG_ERR("Unsupport flow %u\r\n", ctrl_hdl->flow);
		return E_SYS;
	}

	/* output yuv */
	switch (ctrl_hdl->flow) {
	case CTL_IPP_FLOW_RAW:
	case CTL_IPP_FLOW_CAPTURE_RAW:
	case CTL_IPP_FLOW_DIRECT_RAW:
	case CTL_IPP_FLOW_CCIR:
	case CTL_IPP_FLOW_CAPTURE_CCIR:
	case CTL_IPP_FLOW_IME_D2D:
		for (i = CTL_IPP_OUT_PATH_ID_1; i <= CTL_IPP_OUT_PATH_ID_5; i++) {
			p_ime_path = &p_base->ime_ctrl.out_img[i];
			if (p_ime_path->enable && p_ime_path->dma_enable) {
				ext_data.one_buf_mode_en = p_ime_path->one_buf_mode_enable;
				if (ext_data.one_buf_mode_en) {
					CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_DBG, "p%u one buf\r\n", i);
				}
				if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IME_P1_OUT << i, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_ime_path->addr[0]), ctl_ipp_util_yuvsize(p_ime_path->fmt, p_ime_path->lofs[0], p_ime_path->size.h), &ext_data) != E_OK) {
					CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IME_P1_OUT << i));
				}
			}
		}
		break;

	case CTL_IPP_FLOW_DCE_D2D:
		p_ime_path = &p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1];
		if (p_ime_path->enable && p_ime_path->dma_enable) {
			if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_DCE_OUT, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_ime_path->addr[0]), ctl_ipp_util_yuvsize(p_ime_path->fmt, p_ime_path->lofs[0], p_ime_path->size.h), &ext_data) != E_OK) {
				CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_DCE_OUT));
			}
		}
		break;

	case CTL_IPP_FLOW_IPE_D2D:
		p_ime_path = &p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1];
		if (p_ime_path->enable && p_ime_path->dma_enable) {
			if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IPE_OUT, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_ime_path->addr[0]), ctl_ipp_util_yuvsize(p_ime_path->fmt, p_ime_path->lofs[0], p_ime_path->size.h), &ext_data) != E_OK) {
				CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IPE_OUT));
			}
		}
		break;

	default:
		CTL_IPP_DBG_ERR("Unsupport flow %u\r\n", ctrl_hdl->flow);
		return E_SYS;
	}

	/* subout */
	/* wdr */
	if (p_base->dce_ctrl.wdr_enable == TRUE) {
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBOUT_WDR, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->dce_ctrl.wdr_out_addr), ctrl_hdl->private_buf.buf_item.wdr_subout[0].size, &ext_data) != E_OK) {
			CTL_IPP_DBG_WRN("wdr fail\r\n");
		}
	}

	/* defog */
	{
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_DEFOG, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ipe_ctrl.defog_out_addr), ctrl_hdl->private_buf.buf_item.defog_subout[0].size, &ext_data) != E_OK) {
			CTL_IPP_DBG_WRN("defog fail\r\n");
		}
	}

	/* va */
	if (p_base->ipe_ctrl.va_enable == TRUE) {
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_VA, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ipe_ctrl.va_out_addr), ctrl_hdl->private_buf.buf_item.va[0].size, &ext_data) != E_OK) {
			CTL_IPP_DBG_WRN("ipe va fail\r\n");
		}
	}

	/* LCA */
	if (p_base->ime_ctrl.lca_out_enable == TRUE) {
		if (p_base->ime_ctrl.lca_out_addr) {
			if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_LCA, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.lca_out_addr), ctrl_hdl->private_buf.buf_item.lca[0].size, &ext_data) != E_OK) {
				CTL_IPP_DBG_WRN("lca fail\r\n");
			}
		}
	}

	/* 3DNR */
	if (p_base->ime_ctrl.tplnr_enable == TRUE) {
		/* MV */
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MV, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.tplnr_out_mv_addr), ctrl_hdl->private_buf.buf_item.mv[0].size, &ext_data) != E_OK) {
			CTL_IPP_DBG_WRN("mv fail\r\n");
		}

		/* MS */
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.tplnr_out_ms_addr), ctrl_hdl->private_buf.buf_item.ms[0].size, &ext_data) != E_OK) {
			CTL_IPP_DBG_WRN("ms fail\r\n");
		}

		/* MS_ROI */
		if (p_base->ime_ctrl.tplnr_out_ms_roi_enable) {
			if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS_ROI, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.tplnr_out_ms_roi_addr), ctrl_hdl->private_buf.buf_item.ms_roi[0].size, &ext_data) != E_OK) {
				CTL_IPP_DBG_WRN("ms roi fail\r\n");
			}
		}
	}

	return E_OK;
}

ER ctl_ipp_dbg_dma_wp_proc_end(CTL_IPP_INFO_LIST_ITEM *ctrl_info, BOOL update_ref)
{
	CTL_IPP_HANDLE *ctrl_hdl;
	CTL_IPP_BASEINFO *p_base;
	CTL_IPP_DBG_DMA_WP_BUF_EXT ext_data = {0};
	UINT32 i, frm_num;

	if (ctl_ipp_dbg_dma_wp_en == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
		return E_OK;
	}

	if (ctrl_info == NULL) {
		CTL_IPP_DBG_ERR("NULL info\r\n");
		return E_SYS;
	}

	ctrl_hdl = (CTL_IPP_HANDLE *)ctrl_info->owner;
	p_base = &ctrl_info->info;

	if (ctrl_hdl == NULL) {
		CTL_IPP_DBG_ERR("NULL hdl\r\n");
		return E_SYS;
	}

	/* input frame */
	switch (ctrl_hdl->flow) {
	case CTL_IPP_FLOW_RAW:
	case CTL_IPP_FLOW_CAPTURE_RAW:
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ife_ctrl.in_addr[0])) != E_OK) {
			CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0));
		}

		if (p_base->ife_ctrl.shdr_enable) {
			frm_num = VDO_PXLFMT_PLANE(p_base->ife_ctrl.in_fmt);
			for (i = 1; i < frm_num; i++) {
				if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0 << i, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ife_ctrl.in_addr[i])) != E_OK) {
					CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0 << i));
				}
			}
		}
		break;

	case CTL_IPP_FLOW_DIRECT_RAW:
		if (p_base->ife_ctrl.shdr_enable) {
			frm_num = VDO_PXLFMT_PLANE(p_base->ife_ctrl.in_fmt);
			for (i = 1; i < frm_num; i++) {
				if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0 << i, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ife_ctrl.in_addr[i])) != E_OK) {
					CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IFE_IN0 << i));
				}
			}
		}
		break;

	case CTL_IPP_FLOW_CCIR:
	case CTL_IPP_FLOW_CAPTURE_CCIR:
	case CTL_IPP_FLOW_DCE_D2D:
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_DCE_IN, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->dce_ctrl.in_addr[0])) != E_OK) {
			CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_DCE_IN));
		}
		break;

	case CTL_IPP_FLOW_IME_D2D:
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IME_IN, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.in_addr[0])) != E_OK) {
			CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IME_IN));
		}
		break;

	case CTL_IPP_FLOW_IPE_D2D:
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IPE_IN, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ipe_ctrl.in_addr[0])) != E_OK) {
			CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IPE_IN));
		}
		break;

	default:
		CTL_IPP_DBG_ERR("Unsupport flow %u\r\n", ctrl_hdl->flow);
		return E_SYS;
	}

	/* subout */
	/* wdr */
	if (p_base->dce_ctrl.wdr_enable == TRUE) {
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBOUT_WDR, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->dce_ctrl.wdr_out_addr)) != E_OK) {
			CTL_IPP_DBG_WRN("wdr out fail\r\n");
		}
	}

	/* defog */
	{
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_DEFOG, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ipe_ctrl.defog_out_addr)) != E_OK) {
			CTL_IPP_DBG_WRN("defog out fail\r\n");
		}
	}

	/* va */
	if (p_base->ipe_ctrl.va_enable == TRUE) {
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBOUT_VA, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ipe_ctrl.va_out_addr)) != E_OK) {
			CTL_IPP_DBG_WRN("ipe va fail\r\n");
		}
	}

	/* LCA */
	if (p_base->ime_ctrl.lca_out_enable == TRUE) {
		if (p_base->ime_ctrl.lca_out_addr) {
			if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_LCA, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.lca_out_addr)) != E_OK) {
				CTL_IPP_DBG_WRN("lca out fail\r\n");
			}
		}
	}

	/* 3DNR */
	if (p_base->ime_ctrl.tplnr_enable == TRUE) {
		/* MV */
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MV, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.tplnr_out_mv_addr)) != E_OK) {
			CTL_IPP_DBG_WRN("mv fail\r\n");
		}

		/* MS */
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.tplnr_out_ms_addr)) != E_OK) {
			CTL_IPP_DBG_WRN("ms fail\r\n");
		}

		/* MS_ROI */
		if (p_base->ime_ctrl.tplnr_out_ms_roi_enable) {
			if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_MS_ROI, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.tplnr_out_ms_roi_addr)) != E_OK) {
				CTL_IPP_DBG_WRN("ms roi fail\r\n");
			}
		}
	}

	/* subin (protect subout for next frame subin) */
	if (update_ref) { // dont update ref protection at drop, to prevent error occur later
		/* wdr */
		if (p_base->dce_ctrl.wdr_enable == TRUE) {
			if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBIN_WDR, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->dce_ctrl.wdr_out_addr), ctrl_hdl->private_buf.buf_item.wdr_subout[0].size, &ext_data) != E_OK) {
				CTL_IPP_DBG_WRN("wdr in fail\r\n");
			}
		}

		/* defog */
		{
			if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBIN_DEFOG, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ipe_ctrl.defog_out_addr), ctrl_hdl->private_buf.buf_item.defog_subout[0].size, &ext_data) != E_OK) {
				CTL_IPP_DBG_WRN("defog in fail\r\n");
			}
		}

		/* LCA */
		if (p_base->ime_ctrl.lca_out_enable == TRUE) {
			if (p_base->ime_ctrl.lca_out_addr) {
				ext_data.flow = ctrl_hdl->flow;
				if (CTL_IPP_IS_DIRECT_FLOW(ext_data.flow)) {
					CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_DBG, "LCA in direct\r\n");
				}
				if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBIN_LCA, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.lca_out_addr), ctrl_hdl->private_buf.buf_item.lca[0].size, &ext_data) != E_OK) {
					CTL_IPP_DBG_WRN("lca in fail\r\n");
				}
			}
		}
	}

	return E_OK;
}

ER ctl_ipp_dbg_dma_wp_proc_end_ref(CTL_IPP_INFO_LIST_ITEM *last_ctrl_info)
{
	CTL_IPP_BASEINFO *p_base;
	CTL_IPP_IME_OUT_IMG *p_ime_path;

	if (ctl_ipp_dbg_dma_wp_en == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
		return E_OK;
	}

	if (last_ctrl_info == NULL) {
		CTL_IPP_DBG_ERR("NULL info\r\n");
		return E_SYS;
	}

	p_base = &last_ctrl_info->info;

	/* ref in */
	p_ime_path = &p_base->ime_ctrl.out_img[p_base->ime_ctrl.tplnr_in_ref_path];
	if (p_base->ime_ctrl.tplnr_enable && p_ime_path->dma_enable) {
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IME_REF_IN, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_ime_path->addr[0])) != E_OK) {
			CTL_IPP_DBG_WRN("ref in fail\r\n");
		}
	}

	/* subin (last subout is current subin) */
	if (p_base->dce_ctrl.wdr_enable == TRUE) {
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_DCE_SUBIN_WDR, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->dce_ctrl.wdr_out_addr)) != E_OK) {
			CTL_IPP_DBG_WRN("wdr fail\r\n");
		}
	}

	{
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IPE_SUBIN_DEFOG, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ipe_ctrl.defog_out_addr)) != E_OK) {
			CTL_IPP_DBG_WRN("defog fail\r\n");
		}
	}

	if (p_base->ime_ctrl.lca_out_enable == TRUE && p_base->ime_ctrl.lca_out_addr) {
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBIN_LCA, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_base->ime_ctrl.lca_out_addr)) != E_OK) {
			CTL_IPP_DBG_WRN("lca fail\r\n");
		}
	}

	return E_OK;
}

ER ctl_ipp_dbg_dma_wp_proc_end_push(CTL_IPP_INFO_LIST_ITEM *ctrl_info, BOOL update_ref)
{
	CTL_IPP_HANDLE *ctrl_hdl;
	CTL_IPP_BASEINFO *p_base;
	CTL_IPP_IME_OUT_IMG *p_ime_path;
	CTL_IPP_DBG_DMA_WP_BUF_EXT ext_data = {0};
	UINT32 i;

	if (ctl_ipp_dbg_dma_wp_en == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
		return E_OK;
	}

	if (ctrl_info == NULL) {
		CTL_IPP_DBG_ERR("NULL info\r\n");
		return E_SYS;
	}

	ctrl_hdl = (CTL_IPP_HANDLE *)ctrl_info->owner;
	p_base = &ctrl_info->info;

	if (ctrl_hdl == NULL) {
		CTL_IPP_DBG_ERR("NULL hdl\r\n");
		return E_SYS;
	}

	/* output yuv */
	switch (ctrl_hdl->flow) {
	case CTL_IPP_FLOW_RAW:
	case CTL_IPP_FLOW_CAPTURE_RAW:
	case CTL_IPP_FLOW_DIRECT_RAW:
	case CTL_IPP_FLOW_CCIR:
	case CTL_IPP_FLOW_CAPTURE_CCIR:
	case CTL_IPP_FLOW_IME_D2D:
		for (i = CTL_IPP_OUT_PATH_ID_1; i <= CTL_IPP_OUT_PATH_ID_5; i++) {
			p_ime_path = &p_base->ime_ctrl.out_img[i];
			if (p_ime_path->enable && p_ime_path->dma_enable) {
				if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IME_P1_OUT << i, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_ime_path->addr[0])) != E_OK) {
					CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IME_P1_OUT << i));
				}
			}
		}
		break;

	case CTL_IPP_FLOW_DCE_D2D:
		p_ime_path = &p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1];
		if (p_ime_path->enable && p_ime_path->dma_enable) {
			if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_DCE_OUT, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_ime_path->addr[0])) != E_OK) {
				CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_DCE_OUT));
			}
		}
		break;

	case CTL_IPP_FLOW_IPE_D2D:
		p_ime_path = &p_base->ime_ctrl.out_img[CTL_IPP_OUT_PATH_ID_1];
		if (p_ime_path->enable && p_ime_path->dma_enable) {
			if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IPE_OUT, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_ime_path->addr[0])) != E_OK) {
				CTL_IPP_DBG_ERR("(flow%u) %s fail\r\n", ctrl_hdl->flow, ctl_ipp_dbg_dma_wp_buf_type_str(CTL_IPP_DBG_DMA_WP_BUF_IPE_OUT));
			}
		}
		break;

	default:
		CTL_IPP_DBG_ERR("Unsupport flow %u\r\n", ctrl_hdl->flow);
		return E_SYS;
	}

	/* ref in */
	if (update_ref) { // dont update ref protection at drop, to prevent error occur later
		p_ime_path = &p_base->ime_ctrl.out_img[p_base->ime_ctrl.tplnr_in_ref_path];
		if (p_base->ime_ctrl.tplnr_enable && p_ime_path->dma_enable) {
			if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IME_REF_IN, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(p_ime_path->addr[0]), ctl_ipp_util_yuvsize(p_ime_path->fmt, p_ime_path->lofs[0], p_ime_path->size.h), &ext_data) != E_OK) {
				CTL_IPP_DBG_WRN("ref in fail\r\n");
			}
		}
	}

	return E_OK;
}

ER ctl_ipp_dbg_dma_wp_task_start(CTRL_IPP_BUF_ITEM item, CTL_IPP_BUF_ADDR addr, UINT32 size)
{
	// 520, 560 dont set pa in addr. use CTL_IPP_BUF_ADDR_MAKE_WITH_VA(addr.va) only
	CTL_IPP_DBG_DMA_WP_BUF_EXT ext_data = {0};

	if (ctl_ipp_dbg_dma_wp_en == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
		return E_OK;
	}

	switch (item) {
	case CTRL_IPP_BUF_ITEM_PM:
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_PM, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(addr.va), size, &ext_data) != E_OK) {
			CTL_IPP_DBG_WRN("pm fail\r\n");
		}
		break;

	case CTRL_IPP_BUF_ITEM_3DNR_STA:
		if (ctl_ipp_dbg_dma_wp_start(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_3DNR_STA, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(addr.va), size, &ext_data) != E_OK) {
			CTL_IPP_DBG_WRN("3dnr fail\r\n");
		}
		break;

	default:
		CTL_IPP_DBG_ERR("Unknown item %u\r\n", item);
		return E_PAR;
	}

	return E_OK;
}

ER ctl_ipp_dbg_dma_wp_task_end(CTRL_IPP_BUF_ITEM item, CTL_IPP_BUF_ADDR addr)
{
	// 520, 560 dont set pa in addr. use CTL_IPP_BUF_ADDR_MAKE_WITH_VA(addr.va) only
	if (ctl_ipp_dbg_dma_wp_en == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
		return E_OK;
	}

	switch (item) {
	case CTRL_IPP_BUF_ITEM_PM:
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_PM, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(addr.va)) != E_OK) {
			CTL_IPP_DBG_WRN("pm fail\r\n");
		}
		break;

	case CTRL_IPP_BUF_ITEM_3DNR_STA:
		if (ctl_ipp_dbg_dma_wp_end(CTL_IPP_DBG_DMA_WP_BUF_IME_SUBOUT_3DNR_STA, CTL_IPP_BUF_ADDR_MAKE_WITH_VA(addr.va)) != E_OK) {
			CTL_IPP_DBG_WRN("3dnr fail\r\n");
		}
		break;

	default:
		CTL_IPP_DBG_ERR("Unknown item %u\r\n", item);
		return E_PAR;
	}

	return E_OK;
}

ER ctl_ipp_dbg_dma_wp_drop(CTL_IPP_INFO_LIST_ITEM *ctrl_info)
{
	// proc_end wont hanppen when drop, so do proc_end process at here
	CTL_IPP_HANDLE *ctrl_hdl;
	ER rt = E_OK;

	if (ctl_ipp_dbg_dma_wp_en == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
		return E_OK;
	}

	if (ctrl_info == NULL) {
		CTL_IPP_DBG_ERR("NULL info\r\n");
		return E_SYS;
	}

	ctrl_hdl = (CTL_IPP_HANDLE *)ctrl_info->owner;

	if (ctrl_hdl == NULL) {
		CTL_IPP_DBG_ERR("NULL hdl\r\n");
		return E_SYS;
	}

	rt |= ctl_ipp_dbg_dma_wp_proc_end(ctrl_info, FALSE);
	rt |= ctl_ipp_dbg_dma_wp_proc_end_ref(ctrl_hdl->p_last_rdy_ctrl_info);
	rt |= ctl_ipp_dbg_dma_wp_proc_end_push(ctrl_info, FALSE);
	if (ctrl_info->info.ime_ctrl.pm_in_addr) {
		rt |= ctl_ipp_dbg_dma_wp_task_end(CTRL_IPP_BUF_ITEM_PM, CTL_IPP_BUF_ADDR_MAKE(ctrl_info->info.ime_ctrl.pm_in_addr, 0));
	}

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_DBG, "%d\r\n", rt);

	return rt;
}

ER ctl_ipp_dbg_dma_wp_clear(void)
{
	// do un-protect to all buf address at ipp close
	UINT32 i_wp_set;
	ER rt = E_OK;

	if (ctl_ipp_dbg_dma_wp_en == CTL_IPP_DBG_DMA_WP_BUF_DISABLE) {
		return E_OK;
	}

	for (i_wp_set = 0; i_wp_set < WPSET_COUNT; i_wp_set++) {
		if (!CTL_IPP_BUF_ADDR_IS_ZERO(ctl_ipp_dbg_dma_wp_addr[i_wp_set])) {
			rt |= ctl_ipp_dbg_dma_wp_end(ctl_ipp_dbg_dma_wp_type[i_wp_set], ctl_ipp_dbg_dma_wp_addr[i_wp_set]);
		}
	}

	CTL_IPP_DBG_TRC_PART(CTL_IPP_DBG_TRC_DBG, "%d\r\n", rt);

	return rt;
}

void ctl_ipp_dbg_dma_wp_dump(int (*dump)(const char *fmt, ...))
{
	unsigned long loc_cpu_flg;
	CTL_IPP_BUF_ADDR tmp_addr[WPSET_COUNT];
	CTL_IPP_DBG_DMA_WP_BUF_TYPE tmp_type[WPSET_COUNT];

	// copy info into tmp to avoid update when print
	loc_cpu(loc_cpu_flg);
	memcpy((void *)tmp_addr, (void *)ctl_ipp_dbg_dma_wp_addr, sizeof(ctl_ipp_dbg_dma_wp_addr));
	memcpy((void *)tmp_type, (void *)ctl_ipp_dbg_dma_wp_type, sizeof(ctl_ipp_dbg_dma_wp_type));
	unl_cpu(loc_cpu_flg);

	dump("\r\n--------------- DMA_WP (lv:%s) ------------\r\n", ctl_ipp_dbg_dma_wp_prot_lv_str(ctl_ipp_dbg_dma_wp_level));
	dump("%18s  %18s  %18s  %18s  %18s  %18s  %18s\r\n", "type", "addr0", "addr1", "addr2", "addr3", "addr4", "addr5");
	dump("0x%016llx  0x%016lx  0x%016lx  0x%016lx  0x%016lx  0x%016lx  0x%016lx\r\n", ctl_ipp_dbg_dma_wp_en, tmp_addr[0].va, tmp_addr[1].va, tmp_addr[2].va, tmp_addr[3].va, tmp_addr[4].va, tmp_addr[5].va);
	dump("%18s  0x%016lx  0x%016lx  0x%016lx  0x%016lx  0x%016lx  0x%016lx\r\n", "pa", tmp_addr[0].pa, tmp_addr[1].pa, tmp_addr[2].pa, tmp_addr[3].pa, tmp_addr[4].pa, tmp_addr[5].pa);
	dump("%18s  %18s  %18s  %18s  %18s  %18s  %18s\r\n", "type", ctl_ipp_dbg_dma_wp_buf_type_str(tmp_type[0]), ctl_ipp_dbg_dma_wp_buf_type_str(tmp_type[1]), ctl_ipp_dbg_dma_wp_buf_type_str(tmp_type[2]), ctl_ipp_dbg_dma_wp_buf_type_str(tmp_type[3]), ctl_ipp_dbg_dma_wp_buf_type_str(tmp_type[4]), ctl_ipp_dbg_dma_wp_buf_type_str(tmp_type[5]));
}
