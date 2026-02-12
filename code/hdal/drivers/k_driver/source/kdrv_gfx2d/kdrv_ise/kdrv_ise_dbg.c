#include "kwrap/cpu.h"
#include "kdrv_ise_int_dbg.h"

unsigned int kdrv_ise_debug_level = NVT_DBG_ERR;
static KDRV_ISE_CTL *p_kdrv_ise_ctl;
static UINT32 g_kdrv_ise_dbg_dump_ts_cnt;

void kdrv_ise_dbg_init(KDRV_ISE_CTL *p_ctl)
{
	p_kdrv_ise_ctl = p_ctl;
}

void kdrv_ise_dbg_uninit(void)
{
	p_kdrv_ise_ctl = NULL;
}

void kdrv_ise_dump_ll_blk_cmd(KDRV_ISE_LL_BLK *p_ll_blk)
{
	UINT32 i;
	KDRV_ISE_LL_CMD *p_cmd;

	p_cmd = (KDRV_ISE_LL_CMD *)p_ll_blk->cmd_buf_addr;

	DBG_DUMP("cmd buffer 0x%.8x, phy 0x%.8x\r\n", p_ll_blk->cmd_buf_addr, vos_cpu_get_phy_addr(p_ll_blk->cmd_buf_addr));
	for (i = 0; i < p_ll_blk->cur_cmd_idx; i++) {
		if (p_cmd[i].null_bit.cmd == KDRV_ISE_LL_CMD_TYPE_NULL) {
			DBG_DUMP("NULL idx %d\r\n", p_cmd[i].null_bit.table_index);
		} else if (p_cmd[i].null_bit.cmd == KDRV_ISE_LL_CMD_TYPE_NXTLL) {
			DBG_DUMP("NXT_LL addr 0x%.8x, idx %d\r\n", p_cmd[i].nxtll_bit.next_ll_addr, p_cmd[i].nxtll_bit.table_index);
		} else if (p_cmd[i].null_bit.cmd == KDRV_ISE_LL_CMD_TYPE_UPD) {
			DBG_DUMP("UPD ofs = 0x%.8x, val = 0x%.8x\r\n", p_cmd[i].upd_bit.reg_ofs, p_cmd[i].upd_bit.reg_val);
		}
	}
}

void kdrv_ise_dump_cfg(KDRV_ISE_JOB_CFG *p_job_cfg)
{
	CHAR *p_fmt_str[KDRV_ISE_IO_PACK_SEL_MAX] = {
		"Y8",
		"Y4",
		"Y1",
		"UVP",
		"RGB565",
		"ARGB8888",
		"ARGB1555",
		"ARGB4444",
		"YUVP"
	};
	CHAR *p_scl_str[KDRV_ISE_SCALE_METHOD_MAX] = {
		"UNKNOWN",
		"BILENAR",
		"NEAREST",
		"INTEGRATION"
	};
	CHAR *p_argb_str[KDRV_ISE_ARGB_OUTMODE_MAX] = {
		"ARGB8888",
		"RGB888",
		"A8"
	};
	KDRV_ISE_IO_CFG *p_cfg = &p_job_cfg->io_cfg;

	DBG_DUMP("kdrv ise iocfg 0x%.8x\r\n", (unsigned int)p_job_cfg);
	DBG_DUMP("%12s %8s %8s %8s %10s %10s\r\n", "fmt","in_w", "in_h", "in_lof", "in_addr", "in_flush");
	DBG_DUMP("%12s %8d %8d %8d 0x%.8x %10d\r\n",
				p_fmt_str[p_cfg->io_pack_fmt],
				(int)p_cfg->in_width,
				(int)p_cfg->in_height,
				(int)p_cfg->in_lofs,
				(unsigned int)p_cfg->in_addr,
				(int)p_cfg->in_buf_flush);

	DBG_DUMP("%12s %8s %8s %8s %10s %10s %10s %8s\r\n", "scl","out_w", "out_h", "out_lof", "out_addr", "out_flush", "argb_mode", "pa_en");
	DBG_DUMP("%12s %8d %8d %8d 0x%.8x %10d %10s %8d\r\n",
				p_scl_str[p_cfg->scl_method],
				(int)p_cfg->out_width,
				(int)p_cfg->out_height,
				(int)p_cfg->out_lofs,
				(unsigned int)p_cfg->out_addr,
				(int)p_cfg->out_buf_flush,
				p_argb_str[p_cfg->argb_out_mode],
				(int)p_cfg->phy_addr_en);
}

void kdrv_ise_set_dump_ts(UINT32 cnt)
{
	g_kdrv_ise_dbg_dump_ts_cnt = (0xFFFF0000 | (cnt & 0xFFFF));
}

void kdrv_ise_dump_ts(KDRV_ISE_JOB_HEAD *p_job)
{
	if (g_kdrv_ise_dbg_dump_ts_cnt & (0xFFFF0000)) {
		DBG_DUMP("\r\nJOBHEAD TIME STAMP\r\n");
		DBG_DUMP("%8s %8s %16s %16s(%8s) %16s(%8s) %16s(%8s) %16s(%8s)\r\n", "job_id", "job_num",
			"gennode",
			"putjob", "diff",
			"start", "diff",
			"load", "diff",
			"end", "diff");

		g_kdrv_ise_dbg_dump_ts_cnt &= ~(0xFFFF0000);
	}

	if (g_kdrv_ise_dbg_dump_ts_cnt > 0) {
		DBG_DUMP("%8d %8d %16d %16d(%+8d) %16d(%+8d) %16d(%+8d) %16d(%+8d)\r\n", (int)p_job->job_id, (int)p_job->job_num,
			(int)p_job->timestamp[0],
			(int)p_job->timestamp[1], (int)(p_job->timestamp[1] - p_job->timestamp[0]),
			(int)p_job->timestamp[2], (int)(p_job->timestamp[2] - p_job->timestamp[1]),
			(int)p_job->timestamp[3], (int)(p_job->timestamp[3] - p_job->timestamp[2]),
			(int)p_job->timestamp[4], (int)(p_job->timestamp[4] - p_job->timestamp[3]));

		g_kdrv_ise_dbg_dump_ts_cnt--;
	}
}

void kdrv_ise_dump_all(void)
{
	unsigned long loc_flg;
	UINT32 i;
	KDRV_ISE_MEM_POOL *p_pool[3];

	if (p_kdrv_ise_ctl == NULL) {
		return ;
	}

	DBG_DUMP("----- [KDRV_ISE] DUMP START -----\r\n");

	/* common info */
	DBG_DUMP("chip_num: %4d, eng_num: %4d, total_ch: %4d\r\n", (int)p_kdrv_ise_ctl->chip_num, (int)p_kdrv_ise_ctl->eng_num, (int)p_kdrv_ise_ctl->total_ch);
	DBG_DUMP("\r\n");

	/* mem pool info */
	p_pool[0] = &p_kdrv_ise_ctl->job_head_pool;
	p_pool[1] = &p_kdrv_ise_ctl->job_cfg_pool;
	p_pool[2] = &p_kdrv_ise_ctl->ll_blk_pool;
	DBG_DUMP("%16s %16s %16s %16s %16s %16s\r\n", "resource", "start_addr", "total_size", "blk_size", "blk_num", "max_used_num");
	for (i = 0; i < 3; i++) {
		DBG_DUMP("%16s %6s0x%.8x %6s0x%.8x %6s0x%.8x %16d %16d\r\n",
			p_pool[i]->name,
			"", (unsigned int)p_pool[i]->start_addr,
			"", (unsigned int)p_pool[i]->total_size,
			"", (unsigned int)p_pool[i]->blk_size,
			(int)p_pool[i]->blk_num,
			(int)p_pool[i]->max_used_num);
	}
	DBG_DUMP("\r\n");

	/* handle info */
	DBG_DUMP("%10s %10s %10s %12s %10s %10s\r\n", "chip_id", "eng_id", "job_in_cnt", "job_out_cnt", "cur_job", "fired_ll");
	for (i = 0; i < p_kdrv_ise_ctl->total_ch; i++) {
		DBG_DUMP("0x%.8x 0x%.8x %10d %12d 0x%.8x 0x%.8x\r\n",
			(unsigned int)p_kdrv_ise_ctl->p_hdl[i].chip_id,
			(unsigned int)p_kdrv_ise_ctl->p_hdl[i].eng_id,
			(int)p_kdrv_ise_ctl->p_hdl[i].job_in_cnt,
			(int)p_kdrv_ise_ctl->p_hdl[i].job_out_cnt,
			(unsigned int)p_kdrv_ise_ctl->p_hdl[i].p_cur_job,
			(unsigned int)p_kdrv_ise_ctl->p_hdl[i].p_fired_ll);
	}
	DBG_DUMP("\r\n");

	/* dump ll/cfg in fired */
	for (i = 0; i < p_kdrv_ise_ctl->total_ch; i++) {
		KDRV_ISE_HANDLE *p_hdl = &p_kdrv_ise_ctl->p_hdl[i];

		vk_spin_lock_irqsave(&p_hdl->lock, loc_flg);
		if (p_hdl->p_fired_cfg) {
			DBG_DUMP("chip 0x%.8x, eng 0x%.8x, fired_cfg info\r\n", p_hdl->chip_id, p_hdl->eng_id);
			kdrv_ise_dump_cfg(p_hdl->p_fired_cfg);
		}
		if (p_hdl->p_fired_ll) {
			DBG_DUMP("chip 0x%.8x, eng 0x%.8x, fired_ll info\r\n", p_hdl->chip_id, p_hdl->eng_id);
			kdrv_ise_dump_ll_blk_cmd(p_hdl->p_fired_ll);
		}
		vk_spin_unlock_irqrestore(&p_hdl->lock, loc_flg);
	}

	DBG_DUMP("----- [KDRV_ISE] DUMP END -----\r\n");

	ise_eng_dump();
}

void kdrv_ise_set_dbg_level(UINT32 lv)
{
	DBG_DUMP("set kdrv_ise debug level to %d\r\n", lv);
	kdrv_ise_debug_level = lv;
}