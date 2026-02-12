/*
    TS MUXER/TS DEMUXER/HWCOPY Engine Integration module driver

    @file       tse.c
    @ingroup    mIDrvMisc_TSE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#define TSE_BUSY_POLLING 2
#define TSE_POLLING DISABLE
#define TSE_POLLING_MULTI_TRIG	ENABLE
#define TSE_POLLING_CNT 500

#include <plat/top.h>
#include "tse_reg.h"
#include "tse_int.h"
#include "tse_dbg.h"
#include "kwrap/error_no.h"
#include "kwrap/util.h"
#include "comm/tse.h"

#if (TSE_POLLING == ENABLE)
#include "Delay.h"
#endif

/**
    @addtogroup mIDrvMisc_TSE
*/
//@{
void tse_isr(void);

static TSE_BUF_INFO *pin_buf = NULL;
static TSE_BUF_INFO *pout0_buf = NULL;
static TSE_BUF_INFO *pout1_buf = NULL;
static TSE_BUF_INFO *pout2_buf = NULL;
static TSE_BUF_INFO in_buf[TSE_BUF_MAX_CNT];
static TSE_BUF_INFO out0_buf[TSE_BUF_MAX_CNT];
static TSE_BUF_INFO out1_buf[TSE_BUF_MAX_CNT];
static TSE_BUF_INFO out2_buf[TSE_BUF_MAX_CNT];
static UINT32 tse_mux_in_total_size = 0;
static UINT32 tse_status = 0;
static volatile UINT32 tse_int_all_status = 0;
unsigned int tse_debug_level = NVT_DBG_ERR;
#if 0
#endif
static TSE_BUF_INFO* tse_set_buf_list(TSE_BUF_INFO *src, TSE_BUF_INFO *dst)
{
	UINT32 i;
	TSE_BUF_INFO *ptmp;

	i = 0;
	ptmp = src;

	if (ptmp == NULL) {
		return NULL;
	}

	while(ptmp != NULL) {
		dst[i].addr = ptmp->addr;
		dst[i].size = ptmp->size;
		dst[i].pnext = NULL;
		ptmp = ptmp->pnext;
		if (ptmp != NULL) {
			if ((i + 1) < TSE_BUF_MAX_CNT) {
				dst[i].pnext = (TSE_BUF_INFO *)&dst[(i + 1)];
				i = i + 1;
			} else {
				DBG_ERR("int. buffer(%d) overlfow\r\n", TSE_BUF_MAX_CNT);
				return NULL;
			}
		}
	}
	return &dst[0];
}

static void tse_cache_sync_dma_to_dev(TSE_BUF_INFO *buf)
{
	TSE_BUF_INFO *ptmp;

	ptmp = (TSE_BUF_INFO *)buf;
	while(ptmp != NULL)
	{
		tse_platform_cache_sync_dma_to_dev(ptmp->addr, ptmp->size);
		ptmp = ptmp->pnext;
	}
}

static void tse_cache_sync_dma_from_dev(TSE_BUF_INFO *buf)
{
	TSE_BUF_INFO *ptmp;

	ptmp = (TSE_BUF_INFO *)buf;
	while(ptmp != NULL)
	{
		tse_platform_cache_sync_dma_from_dev(ptmp->addr, ptmp->size);
		ptmp = ptmp->pnext;
	}
}

static UINT32 tse_get_buf_size(TSE_BUF_INFO *buf)
{
	TSE_BUF_INFO *ptmp;
	UINT32 rt;

	ptmp = (TSE_BUF_INFO *)buf;
	rt = 0;
	while(ptmp != NULL)
	{
		rt += ptmp->size;
		ptmp = ptmp->pnext;
	}
	return rt;
}

static void tse_set_in_reg(TSE_BUF_INFO *buf)
{
	T_TSE_IN_ADDR_REG in_addr;
	T_TSE_IN_SIZE_REG in_size;

	in_size.reg = 0x0;
	in_addr.reg = 0x0;
	if (buf) {
		in_size.bit.IN_SIZE = buf->size;
		in_addr.bit.IN_ADDR = tse_platform_get_phy_addr(buf->addr);
	}
	TSE_SETREG(TSE_IN_ADDR_REG_OFS, in_addr.reg);
	TSE_SETREG(TSE_IN_SIZE_REG_OFS, in_size.reg);
}

static void tse_set_out0_reg(TSE_BUF_INFO *buf)
{
	T_TSE_OUT0_ADDR_REG out0_addr;
	T_TSE_OUT0_SIZE_LIMITATION_REG out0_size;

	out0_size.reg = 0x0;
	out0_addr.reg = 0x0;
	if (buf) {
		out0_size.bit.OUT0_LIMIT = buf->size;
		out0_addr.bit.OUT0_ADDR = tse_platform_get_phy_addr(buf->addr);
	}
	TSE_SETREG(TSE_OUT0_ADDR_REG_OFS, out0_addr.reg);
	TSE_SETREG(TSE_OUT0_SIZE_LIMITATION_REG_OFS, out0_size.reg);
}

static void tse_set_out1_reg(TSE_BUF_INFO *buf)
{
	T_TSE_OUT1_ADDR_REG out1_addr;
	T_TSE_OUT1_SIZE_LIMITATION_REG out1_size;

	out1_size.reg = 0x0;
	out1_addr.reg = 0x0;
	if (buf) {
		out1_size.bit.OUT1_LIMIT = buf->size;
		out1_addr.bit.OUT1_ADDR = tse_platform_get_phy_addr(buf->addr);
	}
	TSE_SETREG(TSE_OUT1_ADDR_REG_OFS, out1_addr.reg);
	TSE_SETREG(TSE_OUT1_SIZE_LIMITATION_REG_OFS, out1_size.reg);
}

static void tse_set_out2_reg(TSE_BUF_INFO *buf)
{
	T_TSE_OUT2_ADDR_REG out2_addr;
	T_TSE_OUT2_SIZE_LIMITATION_REG out2_size;

	out2_size.reg = 0x0;
	out2_addr.reg = 0x0;
	if (buf) {
		out2_size.bit.OUT2_LIMIT = buf->size;
		out2_addr.bit.OUT2_ADDR = tse_platform_get_phy_addr(buf->addr);
	}
	TSE_SETREG(TSE_OUT2_ADDR_REG_OFS, out2_addr.reg);
	TSE_SETREG(TSE_OUT2_SIZE_LIMITATION_REG_OFS, out2_size.reg);
}

static void tse_isr_trig_next(UINT32 status)
{
	T_TSE_OP_CTRL_REG ctl;
	T_TSE_INTEN_REG int_reg;
	UINT32 trig_flag;

	int_reg.reg = TSE_GETREG(TSE_INTEN_REG_OFS);
	ctl.reg = TSE_GETREG(TSE_OP_CTRL_REG_OFS);
	trig_flag = 0;
	if (status & INT_STS_INPUT_END) {
		if (pin_buf) {
			tse_set_in_reg(pin_buf);
			pin_buf = pin_buf->pnext;
			int_reg.bit.INPUT_END = 0;
			ctl.bit.LAST_BLOCK = 1;
			if (pin_buf) {
				int_reg.bit.INPUT_END = 1;
				ctl.bit.LAST_BLOCK = 0;
			}
			ctl.bit.INPUT_RELOAD = 1;
			trig_flag |= INT_STS_INPUT_END;
		}
	}

	if (status & INT_STS_OUT0_FULL) {
		if (pout0_buf) {
			tse_set_out0_reg(pout0_buf);
			pout0_buf = pout0_buf->pnext;
			int_reg.bit.OUT0_FULL = 0;
			if (pout0_buf) {
				int_reg.bit.OUT0_FULL = 1;
			}
			ctl.bit.OUT0_RELOAD = 1;
			trig_flag |= INT_STS_OUT0_FULL;
		}
	}

	if (status & INT_STS_OUT1_FULL) {
		if (pout1_buf) {
			tse_set_out1_reg(pout1_buf);
			pout1_buf = pout1_buf->pnext;
			int_reg.bit.OUT1_FULL = 0;
			if (pout1_buf) {
				int_reg.bit.OUT1_FULL = 1;
			}
			ctl.bit.OUT1_RELOAD = 1;
			trig_flag |= INT_STS_OUT1_FULL;
		}
	}

	if (status & INT_STS_OUT2_FULL) {
		if (pout2_buf) {
			tse_set_out2_reg(pout2_buf);
			pout2_buf = pout2_buf->pnext;
			int_reg.bit.OUT2_FULL = 0;
			if (pout2_buf) {
				int_reg.bit.OUT2_FULL = 1;
			}
			ctl.bit.OUT2_RELOAD = 1;
			trig_flag |= INT_STS_OUT2_FULL;
		}
	}

	if (trig_flag) {
#if (TSE_POLLING == ENABLE)
		int_reg.reg = 0x0;
#endif
		TSE_SETREG(TSE_INTEN_REG_OFS, int_reg.reg);

		ctl.bit.START = 1;
		TSE_SETREG(TSE_OP_CTRL_REG_OFS, ctl.reg);
	} else {
		DBG_ERR("error no trigger buffer\r\n");
	}
}

static UINT32 tse_wait_end(void)
{
#if (TSE_POLLING == ENABLE)
	UINT32 status, poll_cnt;

#if (TSE_POLLING_MULTI_TRIG == ENABLE)
wait_end_start:
#endif
	poll_cnt = 0;
	while(1) {
		status = TSE_GETREG(TSE_INTSTS_REG_OFS);
		tse_int_all_status |= status;
		TSE_SETREG(TSE_INTSTS_REG_OFS, status);
		DBG_DUMP("tse sts = 0x%.8x\r\n", status);
#if (TSE_POLLING_MULTI_TRIG == ENABLE)
		if (status & INT_STS_COMPLETE) {
			return 1;
		} else if (status & (INT_STS_INPUT_END | INT_STS_OUT0_FULL | INT_STS_OUT1_FULL | INT_STS_OUT2_FULL)) {
#if 0
			DBG_DUMP("trigger next\r\n");
			{
				extern void debug_dumpmem(UINT32 addr, UINT32 length);
				debug_dumpmem(0xf0650000, 0x100);
			}
#endif
			tse_isr_trig_next(status);
			goto wait_end_start;
		}
#else
		if (status & INT_STS_COMPLETE) {
			return 1;
		}
#endif

		poll_cnt += 1;
		if (poll_cnt > TSE_POLLING_CNT) {
			DBG_DUMP("error--------------timeout\r\n");
			{
				extern void debug_dumpmem(UINT32 addr, UINT32 length);
				debug_dumpmem(0xf0650000, 0x100);
			}
			return 0;
		}
		Delay_DelayMs(5);
	}
#else
	tse_platform_wai_flg();
	return 1;
#endif
}

static UINT32 tse_busy_polling_end(void)
{
	T_TSE_INTSTS_REG RegSts;
	UINT32 int_sts, polling_cnt;

	polling_cnt = 0;
	while(polling_cnt < 10000) {

		RegSts.reg = TSE_GETREG(TSE_INTSTS_REG_OFS);
		int_sts = RegSts.reg;

		if (RegSts.reg & TSE_INT_ERR0_STS) {
			DBG_DUMP("ERR0: 0x%.8x\r\n", (int)(RegSts.reg & TSE_INT_ERR0_STS));
		}

		if (RegSts.reg & TSE_INT_ERR1_STS) {
			DBG_DUMP("ERR1: 0x%.8x\r\n", (int)(RegSts.reg & TSE_INT_ERR1_STS));
		}

		if (RegSts.reg & TSE_INT_ERR2_STS) {
			DBG_DUMP("ERR2: 0x%.8x\r\n", (int)(RegSts.reg & TSE_INT_ERR2_STS));
		}

		if (int_sts & INT_STS_COMPLETE) {

			tse_platform_set_flg();
			return 1;

		} else if (int_sts & (INT_STS_INPUT_END | INT_STS_OUT0_FULL | INT_STS_OUT1_FULL | INT_STS_OUT2_FULL)) {
			tse_isr_trig_next(int_sts);
		}
		vos_util_delay_us_polling(10);
		polling_cnt += 1;
	}
	return 0;
}


#if 0
#endif
static ER tse_cfg_set_freq(UINT32 in_val, UINT32 *out_val)
{
	if (in_val <= 240) {
		if (in_val < 240) {
			DBG_WRN("input frequency %d round to 240MHz\r\n", (int)in_val);
		}
		*out_val = 240;
	} else if (in_val <= 300) {
		if (in_val < 300) {
			DBG_WRN("input frequency %d round to 300MHz\r\n", (int)in_val);
		}
		*out_val = 300;
	} else {
		if (in_val > 320) {
			DBG_WRN("input frequency %d round to 320MHz\r\n", (int)in_val);
		}
		*out_val = 320;
	}
	return E_OK;
}

static TSE_CFG_ITEM_INFO cfg_tab[(TSE_CFG_ID_MAX - TSE_CFG_MASK)] = {
	{TSE_CFG_ID_FREQ, 240, 0, tse_cfg_set_freq, NULL, "CFG_FREQ"},
};

static ER tse_cfg_setConfig(TSE_CFG_ID CfgID, UINT32 uiCfgValue)
{
	ER rt = E_OK;
	UINT32 idx, max_cnt;
	TSE_CFG_ITEM_INFO *item;

	max_cnt = TSE_CFG_ID_MAX - TSE_CFG_MASK;
	idx = CfgID - TSE_CFG_MASK;

	if (idx >= max_cnt) {
		DBG_ERR("idx(0x%.8x) overflow < 0x%.8x\r\n", CfgID, TSE_CFG_ID_MAX);
		return E_SYS;
	}

	item = &cfg_tab[idx];

	if (item->id != CfgID) {
		DBG_ERR("id(0x%.8x) mismatch 0x%.8x\r\n", (int)item->id, (int)CfgID);
		return E_SYS;
	}

	if (item->max) {
		if (uiCfgValue > item->max) {
			DBG_ERR("%s(0x%.8x) overflow < 0x%.8x\r\n", item->msg, (int)uiCfgValue, (int)item->max);
			return E_SYS;
		}
		item->val = uiCfgValue;
	} else {
		if (item->set_fp) {
			rt = item->set_fp(uiCfgValue, &item->val);
		}
	}
	return rt;
}

static UINT32 tse_cfg_getConfig(TSE_CFG_ID CfgID)
{
	UINT32 idx, max_cnt;
	TSE_CFG_ITEM_INFO *item;

	max_cnt = TSE_CFG_ID_MAX - TSE_CFG_MASK;
	idx = CfgID - TSE_CFG_MASK;

	if (idx >= max_cnt) {
		DBG_ERR("idx(0x%.8x) overflow < 0x%.8x\r\n", CfgID, TSE_CFG_ID_MAX);
		return 0;
	}

	item = &cfg_tab[idx];

	if (item->id != CfgID) {
		DBG_ERR("id(0x%.8x) mismatch 0x%.8x\r\n", (int)item->id, CfgID);
		return 0;
	}

	if (item->get_fp) {
		return item->get_fp(item->val);
	}
	return item->val;
}

static UINT32 tse_cfg_getVal(TSE_CFG_ID CfgID)
{
	return cfg_tab[(CfgID - TSE_CFG_MASK)].val;
}

#if 0
#endif
static ER tse_mux_set_payload(UINT32 in_val, UINT32 *out_val)
{
	if (in_val == 184) {
		*out_val = 0;
	} else if (in_val == 176) {
		*out_val = 1;
	} else if (in_val == 160) {
		*out_val = 2;
	} else {
		DBG_ERR("data(%d) err\n", (int)in_val);
		return E_SYS;
	}
	return E_OK;
}

static UINT32 tse_mux_get_payload(UINT32 in_val)
{
	UINT32 ret;
	if (in_val == 0) {
		ret = 184;
	} else if (in_val == 1) {
		ret = 176;
	} else if (in_val == 2) {
		ret = 160;
	} else {
		DBG_ERR("data(%d) err\n", (int)in_val);
		ret = 0;
	}
	return ret;
}

static ER tse_mux_set_src_buf(UINT32 in_val, UINT32 *out_val)
{
	TSE_BUF_INFO *ptmp;

	ptmp = tse_set_buf_list((TSE_BUF_INFO *)in_val, &in_buf[0]);
	if (ptmp == NULL) {
		DBG_ERR("error buf = NULL\r\n");
		return E_SYS;
	}

	tse_mux_in_total_size = 0;
	while(ptmp != NULL)
	{
		if (!(ptmp->addr) || (!ptmp->size)) {
			DBG_ERR("error addr(0x%.8x) size(0x%.8x)\r\n", (int)ptmp->addr, (int)ptmp->size);
			return E_SYS;
		}

		if (ptmp->addr & 0x3) {
			DBG_ERR("error addr(0x%.8x) not word aligned\r\n", (int)ptmp->addr);
			return E_SYS;
		}

		if (tse_platform_get_phy_addr(ptmp->addr) > 0x7fffffff) {
			DBG_ERR("error phy addr(0x%.8x) overflow\n", (int)tse_platform_get_phy_addr(ptmp->addr));
			return E_SYS;
		}

		if (ptmp->pnext) {
			if (ptmp->size & 0x3) {
				DBG_ERR("error size(0x%.8x) not word aligned\r\n", (int)ptmp->size);
				return E_SYS;
			}
		}

		tse_mux_in_total_size += ptmp->size;
		ptmp = ptmp->pnext;
	}
	*out_val = in_val;
	return E_OK;
}

static ER tse_mux_set_dst0_buf(UINT32 in_val, UINT32 *out_val)
{
	TSE_BUF_INFO *ptmp;

	ptmp = tse_set_buf_list((TSE_BUF_INFO *)in_val, &out0_buf[0]);
	if (ptmp == NULL) {
		DBG_ERR("error buf = NULL\r\n");
		return E_SYS;
	}

	while(ptmp != NULL)
	{
		if (!(ptmp->addr) || (!ptmp->size)) {
			DBG_ERR("error addr(0x%.8x) size(0x%.8x)\r\n", (int)ptmp->addr, (int)ptmp->size);
			return E_SYS;
		}

		if (ptmp->addr & 0x3) {
			DBG_ERR("error addr(0x%.8x) not word aligned\r\n", (int)ptmp->addr);
			return E_SYS;
		}

		if (tse_platform_get_phy_addr(ptmp->addr) > 0x7fffffff) {
			DBG_ERR("error phy addr(0x%.8x) overflow\n", (int)tse_platform_get_phy_addr(ptmp->addr));
			return E_SYS;
		}

		if (ptmp->size & 0x3) {
			DBG_ERR("error size(0x%.8x) not word aligned\r\n", (int)ptmp->size);
			return E_SYS;
		}

		if (ptmp->size <= 128) {
			DBG_ERR("error size(0x%.8x) must be > 128\r\n", (int)ptmp->size);
			return E_SYS;
		}

		ptmp = ptmp->pnext;
	}

	*out_val = in_val;
	return E_OK;
}

static ER tse_mux_set_last_data_mux_mode(UINT32 in_val, UINT32 *out_val)
{
	if (nvt_get_chip_id() == CHIP_NA51055) {
		if (in_val != 0) {
			DBG_WRN("chip(%d) LAST_DATA_MUX_MODE is not support\r\n", nvt_get_chip_id());
		}
		*out_val = 0;
	} else {
		*out_val = in_val;
	}
	return E_OK;
}

static TSE_CFG_ITEM_INFO mux_tab[(TSMUX_CFG_ID_MAX - TSE_MUX_MASK)] = {
	{TSMUX_CFG_ID_PAYLOADSIZE, 0, 0, tse_mux_set_payload, tse_mux_get_payload, "MUX_PAYLOADSIZE"},
	{TSMUX_CFG_ID_SRC_INFO, 0, 0, tse_mux_set_src_buf, NULL, "MUX_SRC_INFO"},
	{TSMUX_CFG_ID_DST_INFO, 0, 0, tse_mux_set_dst0_buf, NULL, "MUX_DST_INFO"},
	{TSMUX_CFG_ID_MUXING_LEN, 0, 0, NULL, NULL, ""},
	{TSMUX_CFG_ID_SYNC_BYTE, 0x47, 0xff, NULL, NULL, "MUX_SYNC_BYTE"},
	{TSMUX_CFG_ID_CONTINUITY_CNT, 0, 0xf, NULL, NULL, "MUX_CONTINUITY_CNT"},
	{TSMUX_CFG_ID_PID, 0, 0x1fff, NULL, NULL, "MUX_PID"},
	{TSMUX_CFG_ID_TEI, 0, 0x1, NULL, NULL, "MUX_TEI"},
	{TSMUX_CFG_ID_TP, 0, 0x1, NULL, NULL, "MUX_TP"},
	{TSMUX_CFG_ID_SCRAMBLECTRL, 0, 0x3, NULL, NULL, "MUX_SCRAMBLECTRL"},
	{TSMUX_CFG_ID_START_INDICTOR, 0, 0x1, NULL, NULL, "MUX_START_INDICTOR"},
	{TSMUX_CFG_ID_STUFF_VAL, 0xff, 0xff, NULL, NULL, "MUX_STUFF_VAL"},
	{TSMUX_CFG_ID_ADAPT_FLAGS, 0, 0xff, NULL, NULL, "MUX_ADAPT_FLAGS"},
	{TSMUX_CFG_ID_CON_CURR_CNT, 0, 0, NULL, NULL, ""},
	{TSMUX_CFG_ID_LAST_DATA_MUX_MODE, 0, 0, tse_mux_set_last_data_mux_mode, NULL, "MUX_LAST_MODE"},
};
static ER tse_mux_setConfig(TSE_CFG_ID CfgID, UINT32 uiCfgValue)
{
	ER rt = E_OK;
	UINT32 idx, max_cnt;
	TSE_CFG_ITEM_INFO *item;

	max_cnt = TSMUX_CFG_ID_MAX - TSE_MUX_MASK;
	idx = CfgID - TSE_MUX_MASK;

	if (idx >= max_cnt) {
		DBG_ERR("idx(0x%.8x) overflow < 0x%.8x\r\n", CfgID, TSMUX_CFG_ID_MAX);
		return E_SYS;
	}

	item = &mux_tab[idx];

	if (item->id != CfgID) {
		DBG_ERR("id(0x%.8x) mismatch 0x%.8x\r\n", (int)item->id, CfgID);
		return E_SYS;
	}

	if (item->max) {
		if (uiCfgValue > item->max) {
			DBG_ERR("%s(0x%.8x) overflow < 0x%.8x\r\n", item->msg, (int)uiCfgValue, (int)item->max);
			return E_SYS;
		}
		item->val = uiCfgValue;
	} else {
		if (item->set_fp) {
			rt = item->set_fp(uiCfgValue, &item->val);
		}
	}
	return rt;
}

static UINT32 tse_mux_getConfig(TSE_CFG_ID CfgID)
{
	UINT32 idx, max_cnt;
	TSE_CFG_ITEM_INFO *item;

	max_cnt = TSMUX_CFG_ID_MAX - TSE_MUX_MASK;
	idx = CfgID - TSE_MUX_MASK;

	if (idx >= max_cnt) {
		DBG_ERR("idx(0x%.8x) overflow < 0x%.8x\r\n", CfgID, TSMUX_CFG_ID_MAX);
		return 0;
	}

	item = &mux_tab[idx];

	if (item->id != CfgID) {
		DBG_ERR("id(0x%.8x) mismatch 0x%.8x\r\n", (int)item->id, CfgID);
		return 0;
	}

	if (item->get_fp) {
		return item->get_fp(item->val);
	}
	return item->val;
}

static UINT32 tse_mux_getVal(TSE_CFG_ID CfgID)
{
	return mux_tab[(CfgID - TSE_MUX_MASK)].val;
}


static void tse_mux_setVal(TSE_CFG_ID CfgID, UINT32 value)
{
	mux_tab[(CfgID - TSE_MUX_MASK)].val = value;
}

static void tse_mux_end(void)
{
	T_TSE_OUT0_TOTAL_LENGTH_REG out0_len;
	T_TSMUX_CON_CURR_CNT_REG con_curr_cnt;

	/* out length */
	out0_len.reg = TSE_GETREG(TSE_OUT0_TOTAL_LENGTH_REG_OFS);
	tse_mux_setVal(TSMUX_CFG_ID_MUXING_LEN, out0_len.bit.OUT0_LEN);

	/* cont counter */
	con_curr_cnt.reg = TSE_GETREG(TSMUX_CON_CURR_CNT_REG_OFS);
	tse_mux_setVal(TSMUX_CFG_ID_CON_CURR_CNT, con_curr_cnt.bit.CONT_CURR_COUNTER);

	tse_cache_sync_dma_from_dev(&out0_buf[0]);
}


static ER tse_mux_start(BOOL bWait)
{
	T_TSE_CFG_CTRL_REG cfg_ctl;
	T_TSMUX_HEAD_INFO_REG head;
	T_TSMUX_AFINFO1_REG afinfo1;
	T_TSE_INTEN_REG int_reg;
	T_TSE_OP_CTRL_REG ctl;
	T_TSMUX_IN_TOTAL_SIZE_REG totalsize;
	UINT32 out_total_size, pl_size;
	UINT32 in_total_size, rt;

	/* cfg_ctl clear reg & set */
	cfg_ctl.reg = 0x0;
	cfg_ctl.bit.TSE_MODE = TSE_MODE_TSMUX;
	cfg_ctl.bit.TSMUX_PAYLOAD_MODE = tse_mux_getVal(TSMUX_CFG_ID_PAYLOADSIZE);
	TSE_SETREG(TSE_CFG_CTRL_REG_OFS, cfg_ctl.reg);

	/* head clear reg & set */
	head.reg = 0x0;
	head.bit.CONT_COUNTER = tse_mux_getVal(TSMUX_CFG_ID_CONTINUITY_CNT);
	head.bit.SCRAMBLE_CTRL = tse_mux_getVal(TSMUX_CFG_ID_SCRAMBLECTRL);
	head.bit.PID = tse_mux_getVal(TSMUX_CFG_ID_PID);
	head.bit.TP = tse_mux_getVal(TSMUX_CFG_ID_TP);
	head.bit.START_INDICATOR = tse_mux_getVal(TSMUX_CFG_ID_START_INDICTOR);
	head.bit.TEI = tse_mux_getVal(TSMUX_CFG_ID_TEI);
	head.bit.SYNC_BYTE = tse_mux_getVal(TSMUX_CFG_ID_SYNC_BYTE);
	TSE_SETREG(TSMUX_HEAD_INFO_REG_OFS, head.reg);

	/* afinfo1 clear reg & set */
	afinfo1.reg = 0x0;
	afinfo1.bit.ADAPT_FIELD_FLAGS = tse_mux_getVal(TSMUX_CFG_ID_ADAPT_FLAGS);
	afinfo1.bit.LAST_DATA_MUX_MODE = tse_mux_getVal(TSMUX_CFG_ID_LAST_DATA_MUX_MODE);
	afinfo1.bit.STUFF_VALUE = tse_mux_getVal(TSMUX_CFG_ID_STUFF_VAL);
	TSE_SETREG(TSMUX_AFINFO1_REG_OFS, afinfo1.reg);

	/* insize/inaddr clear reg & set */
	tse_cache_sync_dma_to_dev(&in_buf[0]);
	tse_set_in_reg(&in_buf[0]);

	totalsize.reg = 0;
	totalsize.bit.IN_TOTAL_SIZE = tse_mux_in_total_size;
	TSE_SETREG(TSMUX_IN_TOTAL_SIZE_REG_OFS, totalsize.reg);

	/* outsize/outaddr clear reg & set */
	tse_cache_sync_dma_from_dev(&out0_buf[0]);
	tse_set_out0_reg(&out0_buf[0]);

	/* check output buffer size */
	/* 182 + 2(adapt_field_length + adapt_field_flags) = 184 */
	/* data 184 => (header + data 184) */

	/* LAST_DATA_MUX_MODE == 0 */
		/* data 183 => (header + 2 + data 182) + (header + 2 + data 1 + stuff) */
	/* LAST_DATA_MUX_MODE == 1 */
		/* data 183 => (header + 1 + data 183) */

	/* data 182 => (header + 2 + data 182) */
	/*     ... 							   */
	/* data   1 => (header + 2 + data 1)   */
	pl_size = tse_mux_getConfig(TSMUX_CFG_ID_PAYLOADSIZE);
	if (pl_size == 0) {
		DBG_ERR("payload size(%d) error\r\n", pl_size);
		return E_SYS;
	}

	in_total_size = tse_get_buf_size(&in_buf[0]);
	out_total_size = ((in_total_size + (pl_size - 1)) / pl_size) * 188;
	if (afinfo1.bit.LAST_DATA_MUX_MODE == 0) {
		if ((pl_size == 184) && ((in_total_size % 184) == 183)) {
			out_total_size += 188;
		}
	}

	if (tse_get_buf_size(&out0_buf[0]) < out_total_size) {
		DBG_ERR("error out buffer(0x%.8x) too small 0x%.8x\r\n", (int)tse_get_buf_size(&out0_buf[0]), (int)out_total_size);
		return E_SYS;
	}

	/* set in/out pointer to next buffer */
	pin_buf = in_buf[0].pnext;
	pout0_buf = out0_buf[0].pnext;

	/* clear int status */
	TSE_SETREG(TSE_INTSTS_REG_OFS, 0xffffffff);

	/* set int enable */
	int_reg.reg = 0x0;
	int_reg.bit.CMPL = 1;
	if (pin_buf) {
		int_reg.bit.INPUT_END = 1;
	}

	if (pout0_buf) {
		int_reg.bit.OUT0_FULL = 1;
	}

	if (bWait == TSE_BUSY_POLLING) {
		int_reg.reg = 0;
	}

#if (TSE_POLLING == ENABLE)
	int_reg.reg = 0;
#endif
	TSE_SETREG(TSE_INTEN_REG_OFS, int_reg.reg);

	/* trigger */
	tse_platform_clr_flg();
	ctl.reg = 0x0;
	if (pin_buf == NULL) {
		ctl.bit.LAST_BLOCK = 1;
	}
	ctl.bit.START = 1;
	TSE_SETREG(TSE_OP_CTRL_REG_OFS, ctl.reg);

	/* wait end */
	if (bWait) {
		if (bWait == TSE_BUSY_POLLING) {
			rt = tse_busy_polling_end();
		} else {
			rt = tse_wait_end();
		}

		if (rt == 0) {
			return E_TMOUT;
		}
		tse_mux_end();
	}

	return E_OK;
}

#if 0
#endif

static ER tse_demux_set_src_buf(UINT32 in_val, UINT32 *out_val)
{
	TSE_BUF_INFO *ptmp;
	UINT32 ramain_bytes;
	UINT64 in_data_total_size;

	ptmp = tse_set_buf_list((TSE_BUF_INFO *)in_val, &in_buf[0]);
	if (ptmp == NULL) {
		DBG_ERR("error buf = NULL\r\n");
		return E_SYS;
	}

	in_data_total_size = 0;
	ramain_bytes = 0;
	while(ptmp != NULL)
	{
		if (!(ptmp->addr) || (!ptmp->size)) {
			DBG_ERR("error addr(0x%.8x) size(0x%.8x)\r\n", (int)ptmp->addr, (int)ptmp->size);
			return E_SYS;
		}

		if (ptmp->addr & 0x3) {
			DBG_ERR("error addr(0x%.8x) not word aligned\r\n", (int)ptmp->addr);
			return E_SYS;
		}

		if (tse_platform_get_phy_addr(ptmp->addr) > 0x7fffffff) {
			DBG_ERR("error phy addr(0x%.8x) overflow\n", (int)tse_platform_get_phy_addr(ptmp->addr));
			return E_SYS;
		}

		if (ptmp->size & 0x3) {
			DBG_ERR("error size(0x%.8x) not word aligned\r\n", (int)ptmp->size);
			return E_SYS;
		}

		ramain_bytes += (ptmp->size % 188);
		in_data_total_size += ptmp->size;
		ptmp = ptmp->pnext;
	}

	if (ramain_bytes % 188) {
		DBG_ERR("error total size(0x%llx) must be 188xN\r\n", in_data_total_size);
		return E_SYS;
	}

	*out_val = in_val;
	return E_OK;
}

static ER tse_demux_set_dst0_buf(UINT32 in_val, UINT32 *out_val)
{
	TSE_BUF_INFO *ptmp;

	ptmp = tse_set_buf_list((TSE_BUF_INFO *)in_val, &out0_buf[0]);
	if (ptmp == NULL) {
		DBG_ERR("error buf = NULL\r\n");
		return E_SYS;
	}

	while(ptmp != NULL)
	{
		if (!(ptmp->addr) || (!ptmp->size)) {
			DBG_ERR("error addr(0x%.8x) size(0x%.8x)\r\n", (int)ptmp->addr, (int)ptmp->size);
			return E_SYS;
		}

		if (ptmp->addr & 0x3) {
			DBG_ERR("error addr(0x%.8x) not word aligned\r\n", (int)ptmp->addr);
			return E_SYS;
		}

		if (tse_platform_get_phy_addr(ptmp->addr) > 0x7fffffff) {
			DBG_ERR("error phy addr(0x%.8x) overflow\n", (int)tse_platform_get_phy_addr(ptmp->addr));
			return E_SYS;
		}

		if (ptmp->size <= 128) {
			DBG_ERR("error size(0x%.8x) must be > 128\r\n", (int)ptmp->size);
			return E_SYS;
		}

		if (ptmp->pnext) {
			if (ptmp->size & 0x3) {
				DBG_ERR("error size(0x%.8x) not word aligned\r\n", (int)ptmp->size);
				return E_SYS;
			}
		}

		ptmp = ptmp->pnext;
	}

	*out_val = in_val;
	return E_OK;
}

static ER tse_demux_set_dst1_buf(UINT32 in_val, UINT32 *out_val)
{
	TSE_BUF_INFO *ptmp;

	ptmp = tse_set_buf_list((TSE_BUF_INFO *)in_val, &out1_buf[0]);
	if (ptmp == NULL) {
		DBG_ERR("error buf = NULL\r\n");
		return E_SYS;
	}

	while(ptmp != NULL)
	{
		if (!(ptmp->addr) || (!ptmp->size)) {
			DBG_ERR("error addr(0x%.8x) size(0x%.8x)\r\n", (int)ptmp->addr, (int)ptmp->size);
			return E_SYS;
		}

		if (ptmp->addr & 0x3) {
			DBG_ERR("error addr(0x%.8x) not word aligned\r\n", (int)ptmp->addr);
			return E_SYS;
		}

		if (tse_platform_get_phy_addr(ptmp->addr) > 0x7fffffff) {
			DBG_ERR("error phy addr(0x%.8x) overflow\n", (int)tse_platform_get_phy_addr(ptmp->addr));
			return E_SYS;
		}

		if (ptmp->size <= 128) {
			DBG_ERR("error size(0x%.8x) must be > 128\r\n", (int)ptmp->size);
			return E_SYS;
		}

		if (ptmp->pnext) {
			if (ptmp->size & 0x3) {
				DBG_ERR("error size(0x%.8x) not word aligned\r\n", (int)ptmp->size);
				return E_SYS;
			}
		}

		ptmp = ptmp->pnext;
	}

	*out_val = in_val;
	return E_OK;
}

static ER tse_demux_set_dst2_buf(UINT32 in_val, UINT32 *out_val)
{
	TSE_BUF_INFO *ptmp;

	ptmp = tse_set_buf_list((TSE_BUF_INFO *)in_val, &out2_buf[0]);
	if (ptmp == NULL) {
		DBG_ERR("error buf = NULL\r\n");
		return E_SYS;
	}

	while(ptmp != NULL)
	{
		if (!(ptmp->addr) || (!ptmp->size)) {
			DBG_ERR("error addr(0x%.8x) size(0x%.8x)\r\n", (int)ptmp->addr, (int)ptmp->size);
			return E_SYS;
		}

		if (ptmp->addr & 0x3) {
			DBG_ERR("error addr(0x%.8x) not word aligned\r\n", (int)ptmp->addr);
			return E_SYS;
		}

		if (tse_platform_get_phy_addr(ptmp->addr) > 0x7fffffff) {
			DBG_ERR("error phy addr(0x%.8x) overflow\n", (int)tse_platform_get_phy_addr(ptmp->addr));
			return E_SYS;
		}

		if (ptmp->size <= 128) {
			DBG_ERR("error size(0x%.8x) must be > 128\r\n", (int)ptmp->size);
			return E_SYS;
		}

		if (ptmp->pnext) {
			if (ptmp->size & 0x3) {
				DBG_ERR("error size(0x%.8x) not word aligned\r\n", (int)ptmp->size);
				return E_SYS;
			}
		}

		ptmp = ptmp->pnext;
	}

	*out_val = in_val;
	return E_OK;
}


static TSE_CFG_ITEM_INFO demux_tab[(TSDEMUX_CFG_ID_MAX - TSE_DEMUX_MASK)] = {
	{TSDEMUX_CFG_ID_SYNC_BYTE, 0x47, 0xff, NULL, NULL, "DEMUX_SYNC_BYTE"},
	{TSDEMUX_CFG_ID_ADAPTATION_FLAG, 0, 0xff, NULL, NULL, "DEMUX_ADAPTATION_FLAG"},
	{TSDEMUX_CFG_ID_PID0_ENABLE, 0, 0x1, NULL, NULL, "DEMUX_PID0_ENABLE"},
	{TSDEMUX_CFG_ID_PID1_ENABLE, 0, 0x1, NULL, NULL, "DEMUX_PID1_ENABLE"},
	{TSDEMUX_CFG_ID_PID2_ENABLE, 0, 0x1, NULL, NULL, "DEMUX_PID2_ENABLE"},
	{TSDEMUX_CFG_ID_PID0_VALUE, 0, 0x1fff, NULL, NULL, "DEMUX_PID0_VALUE"},
	{TSDEMUX_CFG_ID_PID1_VALUE, 0, 0x1fff, NULL, NULL, "DEMUX_PID1_VALUE"},
	{TSDEMUX_CFG_ID_PID2_VALUE, 0, 0x1fff, NULL, NULL, "DEMUX_PID2_VALUE"},
	{TSDEMUX_CFG_ID_CONTINUITY0_MODE, 0, 0x1, NULL, NULL, "DEMUX_CONTINUITY0_MODE"},
	{TSDEMUX_CFG_ID_CONTINUITY1_MODE, 0, 0x1, NULL, NULL, "DEMUX_CONTINUITY1_MODE"},
	{TSDEMUX_CFG_ID_CONTINUITY2_MODE, 0, 0x1, NULL, NULL, "DEMUX_CONTINUITY2_MODE"},
	{TSDEMUX_CFG_ID_CONTINUITY0_VALUE, 0, 0xf, NULL, NULL, "DEMUX_CONTINUITY0_VALUE"},
	{TSDEMUX_CFG_ID_CONTINUITY1_VALUE, 0, 0xf, NULL, NULL, "DEMUX_CONTINUITY1_VALUE"},
	{TSDEMUX_CFG_ID_CONTINUITY2_VALUE, 0, 0xf, NULL, NULL, "DEMUX_CONTINUITY2_VALUE"},
	{TSDEMUX_CFG_ID_CONTINUITY0_LAST, 0, 0, NULL, NULL, ""},
	{TSDEMUX_CFG_ID_CONTINUITY1_LAST, 0, 0, NULL, NULL, ""},
	{TSDEMUX_CFG_ID_CONTINUITY2_LAST, 0, 0, NULL, NULL, ""},
	{TSDEMUX_CFG_ID_IN_INFO, 0, 0, tse_demux_set_src_buf, NULL, "DEMUX_IN_INFO"},
	{TSDEMUX_CFG_ID_OUT0_INFO, 0, 0, tse_demux_set_dst0_buf, NULL, "DEMUX_OUT0_INFO"},
	{TSDEMUX_CFG_ID_OUT1_INFO, 0, 0, tse_demux_set_dst1_buf, NULL, "DEMUX_OUT1_INFO"},
	{TSDEMUX_CFG_ID_OUT2_INFO, 0, 0, tse_demux_set_dst2_buf, NULL, "DEMUX_OUT2_INFO"},
	{TSDEMUX_CFG_ID_OUT0_TOTAL_LEN, 0, 0, NULL, NULL, "OUT0_TOTAL_LEN"},
	{TSDEMUX_CFG_ID_OUT1_TOTAL_LEN, 0, 0, NULL, NULL, "OUT1_TOTAL_LEN"},
	{TSDEMUX_CFG_ID_OUT2_TOTAL_LEN, 0, 0, NULL, NULL, "OUT2_TOTAL_LEN"},
};

static ER tse_demux_setConfig(TSE_CFG_ID CfgID, UINT32 uiCfgValue)
{
	ER rt = E_OK;
	UINT32 idx, max_cnt;
	TSE_CFG_ITEM_INFO *item;

	max_cnt = TSDEMUX_CFG_ID_MAX - TSE_DEMUX_MASK;
	idx = CfgID - TSE_DEMUX_MASK;

	if (idx >= max_cnt) {
		DBG_ERR("idx(0x%.8x) overflow < 0x%.8x\r\n", CfgID, TSDEMUX_CFG_ID_MAX);
		return E_SYS;
	}

	item = &demux_tab[idx];

	if (item->id != CfgID) {
		DBG_ERR("id(0x%.8x) mismatch 0x%.8x\r\n", (int)item->id, CfgID);
		return E_SYS;
	}

	if (item->max) {
		if (uiCfgValue > item->max) {
			DBG_ERR("%s(0x%.8x) overflow < 0x%.8x\r\n", item->msg, (int)uiCfgValue, (int)item->max);
			return E_SYS;
		}
		item->val = uiCfgValue;
	} else {
		if (item->set_fp) {
			rt = item->set_fp(uiCfgValue, &item->val);
		}
	}
	return rt;
}

static UINT32 tse_demux_getConfig(TSE_CFG_ID CfgID)
{
	UINT32 idx, max_cnt;
	TSE_CFG_ITEM_INFO *item;

	max_cnt = TSDEMUX_CFG_ID_MAX - TSE_DEMUX_MASK;
	idx = CfgID - TSE_DEMUX_MASK;

	if (idx >= max_cnt) {
		DBG_ERR("idx(0x%.8x) overflow < 0x%.8x\r\n", CfgID, TSDEMUX_CFG_ID_MAX);
		return 0;
	}

	item = &demux_tab[idx];

	if (item->id != CfgID) {
		DBG_ERR("id(0x%.8x) mismatch 0x%.8x\r\n", (int)item->id, CfgID);
		return 0;
	}

	if (item->get_fp) {
		return item->get_fp(item->val);
	}
	return item->val;
}

static UINT32 tse_demux_getVal(TSE_CFG_ID CfgID)
{
	return demux_tab[(CfgID - TSE_DEMUX_MASK)].val;
}

static void tse_demux_setVal(TSE_CFG_ID CfgID, UINT32 value)
{
	demux_tab[(CfgID - TSE_DEMUX_MASK)].val = value;
}

static UINT32 tse_demux_set_filt_reg(UINT32 filt_idx)
{
	/* because filter0/1/2 have the same structure, only used filter0 structure */
	T_TSDEMUX_FILTER0_REG filter_x;

	UINT32 flt_tab[][5] = {
		{TSDEMUX_CFG_ID_PID0_ENABLE, TSDEMUX_CFG_ID_PID0_VALUE, TSDEMUX_CFG_ID_CONTINUITY0_MODE, TSDEMUX_CFG_ID_CONTINUITY0_VALUE, TSDEMUX_FILTER0_REG_OFS},
		{TSDEMUX_CFG_ID_PID1_ENABLE, TSDEMUX_CFG_ID_PID1_VALUE, TSDEMUX_CFG_ID_CONTINUITY1_MODE, TSDEMUX_CFG_ID_CONTINUITY1_VALUE, TSDEMUX_FILTER1_REG_OFS},
		{TSDEMUX_CFG_ID_PID2_ENABLE, TSDEMUX_CFG_ID_PID2_VALUE, TSDEMUX_CFG_ID_CONTINUITY2_MODE, TSDEMUX_CFG_ID_CONTINUITY2_VALUE, TSDEMUX_FILTER2_REG_OFS},
	};

	filter_x.reg = 0x0;
	filter_x.bit.FILT0_EN = tse_demux_getVal(flt_tab[filt_idx][0]);
	if (filter_x.bit.FILT0_EN == 1) {
		filter_x.bit.FILT0_PID = tse_demux_getVal(flt_tab[filt_idx][1]);
		filter_x.bit.CONTINUITY0_MODE = tse_demux_getVal(flt_tab[filt_idx][2]);
		if (filter_x.bit.CONTINUITY0_MODE == 1) {
			filter_x.bit.CONTINUITY0_VAL = tse_demux_getVal(flt_tab[filt_idx][3]);
		}
	}
	TSE_SETREG(flt_tab[filt_idx][4], filter_x.reg);
	return filter_x.bit.FILT0_EN;
}


static void tse_demux_end(void)
{
	T_TSE_OUT0_TOTAL_LENGTH_REG out0_len;
	T_TSE_OUT1_TOTAL_LENGTH_REG out1_len;
	T_TSE_OUT2_TOTAL_LENGTH_REG out2_len;
	T_TSDEMUX_FILTER0_REG filt0;
	T_TSDEMUX_FILTER1_REG filt1;
	T_TSDEMUX_FILTER2_REG filt2;

	/* out length */
	out0_len.reg = TSE_GETREG(TSE_OUT0_TOTAL_LENGTH_REG_OFS);
	tse_demux_setVal(TSDEMUX_CFG_ID_OUT0_TOTAL_LEN, out0_len.bit.OUT0_LEN);
	out1_len.reg = TSE_GETREG(TSE_OUT1_TOTAL_LENGTH_REG_OFS);
	tse_demux_setVal(TSDEMUX_CFG_ID_OUT1_TOTAL_LEN, out1_len.bit.OUT1_LEN);
	out2_len.reg = TSE_GETREG(TSE_OUT2_TOTAL_LENGTH_REG_OFS);
	tse_demux_setVal(TSDEMUX_CFG_ID_OUT2_TOTAL_LEN, out2_len.bit.OUT2_LEN);

	/* cont counter */
	filt0.reg = TSE_GETREG(TSDEMUX_FILTER0_REG_OFS);
	tse_demux_setVal(TSDEMUX_CFG_ID_CONTINUITY0_LAST, filt0.bit.LAST_CONTINUITY0_VAL);
	filt1.reg = TSE_GETREG(TSDEMUX_FILTER1_REG_OFS);
	tse_demux_setVal(TSDEMUX_CFG_ID_CONTINUITY1_LAST, filt1.bit.LAST_CONTINUITY1_VAL);
	filt2.reg = TSE_GETREG(TSDEMUX_FILTER2_REG_OFS);
	tse_demux_setVal(TSDEMUX_CFG_ID_CONTINUITY2_LAST, filt2.bit.LAST_CONTINUITY2_VAL);

	if (filt0.bit.FILT0_EN) {
		tse_cache_sync_dma_from_dev(&out0_buf[0]);
	}

	if (filt1.bit.FILT1_EN) {
		tse_cache_sync_dma_from_dev(&out1_buf[0]);
	}

	if (filt2.bit.FILT2_EN) {
		tse_cache_sync_dma_from_dev(&out2_buf[0]);
	}
}


static ER tse_demux_start(BOOL bWait)
{
	UINT32 rt;
	T_TSE_CFG_CTRL_REG cfg_ctl;
	T_TSDEMUX_CFG_REG cfg;

	T_TSE_INTEN_REG int_reg;
	T_TSE_OP_CTRL_REG ctl;

	/* cfg_ctl clear reg & set */
	cfg_ctl.reg = 0x0;
	cfg_ctl.bit.TSE_MODE = TSE_MODE_TSDEMUX;
	TSE_SETREG(TSE_CFG_CTRL_REG_OFS, cfg_ctl.reg);

	/* cfg clear reg & set */
	cfg.reg = 0x0;
	cfg.bit.SYNC_BYTE = tse_demux_getVal(TSDEMUX_CFG_ID_SYNC_BYTE);
	cfg.bit.ADAPTATION_FLAG = tse_demux_getVal(TSDEMUX_CFG_ID_ADAPTATION_FLAG);
	TSE_SETREG(TSDEMUX_CFG_REG_OFS, cfg.reg);

	/* filter0/outsize/outaddr clear reg & set */
	if (tse_demux_set_filt_reg(0) == ENABLE) {

		tse_cache_sync_dma_from_dev(&out0_buf[0]);
		tse_set_out0_reg(&out0_buf[0]);
		pout0_buf = out0_buf[0].pnext;
	} else {

		tse_set_out0_reg(NULL);
		pout0_buf = NULL;
	}

	/* filter1/outsize/outaddr clear reg & set */
	if (tse_demux_set_filt_reg(1) == ENABLE) {

		tse_cache_sync_dma_from_dev(&out1_buf[0]);
		tse_set_out1_reg(&out1_buf[0]);
		pout1_buf = out1_buf[0].pnext;
	} else {

		tse_set_out1_reg(NULL);
		pout1_buf = NULL;
	}

	/* filter2/outsize/outaddr clear reg & set */
	if (tse_demux_set_filt_reg(2) == ENABLE) {

		tse_cache_sync_dma_from_dev(&out2_buf[0]);
		tse_set_out2_reg(&out2_buf[0]);
		pout2_buf = out2_buf[0].pnext;
	} else {

		tse_set_out2_reg(NULL);
		pout2_buf = NULL;
	}

	/* insize/inaddr clear reg & set */
	tse_cache_sync_dma_to_dev(&in_buf[0]);
	tse_set_in_reg(&in_buf[0]);

	/* set in/out pointer to next buffer */
	pin_buf = in_buf[0].pnext;

	/* clear int status */
	TSE_SETREG(TSE_INTSTS_REG_OFS, 0xffffffff);

	/* set int enable */
	int_reg.reg = 0x0;
	int_reg.bit.CMPL = 1;
	if (pin_buf) {
		int_reg.bit.INPUT_END = 1;
	}

	if (pout0_buf) {
		int_reg.bit.OUT0_FULL = 1;
	}

	if (pout1_buf) {
		int_reg.bit.OUT1_FULL = 1;
	}

	if (pout2_buf) {
		int_reg.bit.OUT2_FULL = 1;
	}
	int_reg.reg |= (TSE_INT_ERR0_STS|TSE_INT_ERR1_STS|TSE_INT_ERR2_STS);

	if (bWait == TSE_BUSY_POLLING) {
		int_reg.reg = 0;
	}

#if (TSE_POLLING == ENABLE)
	int_reg.reg = 0;
#endif
	TSE_SETREG(TSE_INTEN_REG_OFS, int_reg.reg);

	/* trigger */
	tse_platform_clr_flg();
	ctl.reg = 0x0;
	if (pin_buf == NULL) {
		ctl.bit.LAST_BLOCK = 1;
	}
	ctl.bit.START = 1;
	TSE_SETREG(TSE_OP_CTRL_REG_OFS, ctl.reg);
	/* wait end */
	if (bWait) {
		if (bWait == TSE_BUSY_POLLING) {
			rt = tse_busy_polling_end();
		} else {
			rt = tse_wait_end();
		}

		if (rt == 0) {
			return E_TMOUT;
		}
		tse_demux_end();
	}
	return E_OK;
}

#if 0
#endif
static ER tse_copy_set_addr(UINT32 in_val, UINT32 *out_val)
{
	if (tse_platform_get_phy_addr(in_val) > 0x7fffffff) {
		DBG_ERR("error phy addr(0x%.8x) overflow\r\n", (int)tse_platform_get_phy_addr(in_val));
		return E_SYS;
	}

	*out_val = in_val;
	return E_OK;
}


static TSE_CFG_ITEM_INFO hwcpy_tab[(HWCOPY_CFG_ID_MAX - TSE_HWCPY_MASK)] = {
	{HWCOPY_CFG_ID_CMD, 0, 2, NULL, NULL, "HWCOPY_CMD"},
	{HWCOPY_CFG_ID_CTEX, 0, 0xffffffff, NULL, NULL, "HWCOPY_CTEX"},
	{HWCOPY_CFG_ID_SRC_ADDR, 0, 0, tse_copy_set_addr, NULL, "HWCOPY_SRC_ADDR"},
	{HWCOPY_CFG_ID_DST_ADDR, 0, 0, tse_copy_set_addr, NULL, "HWCOPY_DST_ADDR"},
	{HWCOPY_CFG_ID_SRC_LEN,  0, 0x3ffffff, NULL, NULL, "HWCOPY_SRC_LEN"},
	{HWCOPY_CFG_ID_TOTAL_LEN, 0, 0, NULL, NULL, "HWCOPY_TOTAL_LEN"},
};

static ER tse_hwcpy_setConfig(TSE_CFG_ID CfgID, UINT32 uiCfgValue)
{
	ER rt = E_OK;
	UINT32 idx, max_cnt;
	TSE_CFG_ITEM_INFO *item;

	max_cnt = HWCOPY_CFG_ID_MAX - TSE_HWCPY_MASK;
	idx = CfgID - TSE_HWCPY_MASK;

	if (idx >= max_cnt) {
		DBG_ERR("idx(0x%.8x) overflow < 0x%.8x\r\n", CfgID, HWCOPY_CFG_ID_MAX);
		return E_SYS;
	}

	item = &hwcpy_tab[idx];

	if (item->id != CfgID) {
		DBG_ERR("id(0x%.8x) mismatch 0x%.8x\r\n", (int)item->id, CfgID);
		return E_SYS;
	}

	if (item->max) {
		if (uiCfgValue > item->max) {
			DBG_ERR("%s(0x%.8x) overflow < 0x%.8x\r\n", item->msg, (int)uiCfgValue, (int)item->max);
			return E_SYS;
		}
		item->val = uiCfgValue;
	} else {
		if (item->set_fp) {
			rt = item->set_fp(uiCfgValue, &item->val);
		}
	}
	return rt;
}

static UINT32 tse_hwcpy_getConfig(TSE_CFG_ID CfgID)
{
	UINT32 idx, max_cnt;
	TSE_CFG_ITEM_INFO *item;

	max_cnt = HWCOPY_CFG_ID_MAX - TSE_HWCPY_MASK;
	idx = CfgID - TSE_HWCPY_MASK;

	if (idx >= max_cnt) {
		DBG_ERR("idx(0x%.8x) overflow < 0x%.8x\r\n", CfgID, HWCOPY_CFG_ID_MAX);
		return 0;
	}

	item = &hwcpy_tab[idx];

	if (item->id != CfgID) {
		DBG_ERR("id(0x%.8x) mismatch 0x%.8x\r\n", (int)item->id, CfgID);
		return 0;
	}

	if (item->get_fp) {
		return item->get_fp(item->val);
	}
	return item->val;
}

static UINT32 tse_hwcpy_getVal(TSE_CFG_ID CfgID)
{
	return hwcpy_tab[(CfgID - TSE_HWCPY_MASK)].val;
}

static void tse_hwcpy_setVal(TSE_CFG_ID CfgID, UINT32 value)
{
	hwcpy_tab[(CfgID - TSE_HWCPY_MASK)].val = value;
}

static void tse_hwcpy_end(void)
{
	T_TSE_OUT0_TOTAL_LENGTH_REG out0_len;

	/* out length */
	out0_len.reg = TSE_GETREG(TSE_OUT0_TOTAL_LENGTH_REG_OFS);
	tse_hwcpy_setVal(HWCOPY_CFG_ID_TOTAL_LEN, out0_len.bit.OUT0_LEN);

	tse_cache_sync_dma_from_dev(&out0_buf[0]);
}

static ER tse_hwcpy_start(BOOL bWait)
{
	UINT32 rt;
	T_TSE_CFG_CTRL_REG cfg_ctl;
	T_TSE_INTEN_REG int_reg;
	T_TSE_OP_CTRL_REG ctl;
	T_HWCOPY_CONST_REG cpy_const;

	/* cfg_ctl clear reg & set */
	cfg_ctl.reg = 0x0;
	cfg_ctl.bit.TSE_MODE = TSE_MODE_HWCOPY;
	cfg_ctl.bit.HWCOPY_MODE = tse_hwcpy_getVal(HWCOPY_CFG_ID_CMD);
	TSE_SETREG(TSE_CFG_CTRL_REG_OFS, cfg_ctl.reg);

	if (cfg_ctl.bit.HWCOPY_MODE == HWCOPY_LINEAR_SET) {

		/* insize/inaddr clear */
		tse_set_in_reg(NULL);

		/* set ctex */
		cpy_const.reg = 0x0;
		cpy_const.bit.CTEX = tse_hwcpy_getVal(HWCOPY_CFG_ID_CTEX);
		TSE_SETREG(HWCOPY_CONST_REG_OFS, cpy_const.reg);

	} else {

		/* insize/inaddr clear reg & set */
		in_buf[0].addr = tse_hwcpy_getVal(HWCOPY_CFG_ID_SRC_ADDR);
		in_buf[0].size = tse_hwcpy_getVal(HWCOPY_CFG_ID_SRC_LEN);
		in_buf[0].pnext = NULL;
		if ((!in_buf[0].size) || (!in_buf[0].addr)) {
			DBG_ERR("error IN_SIZE(0x%.8x) IN_ADDR(0x%.8x)\r\n", (int)in_buf[0].size, (int)in_buf[0].addr);
			return E_SYS;
		}
		tse_cache_sync_dma_to_dev(&in_buf[0]);
		tse_set_in_reg(&in_buf[0]);
	}

	/* outsize/outaddr clear reg & set */
	out0_buf[0].addr = tse_hwcpy_getVal(HWCOPY_CFG_ID_DST_ADDR);
	out0_buf[0].size = tse_hwcpy_getVal(HWCOPY_CFG_ID_SRC_LEN);
	out0_buf[0].pnext = NULL;

	if ((!out0_buf[0].size) || (!out0_buf[0].addr)) {
		DBG_ERR("error OUT0_LIMIT(0x%.8x) OUT0_ADDR(0x%.8x)\r\n", (int)out0_buf[0].size, (int)out0_buf[0].addr);
		return E_SYS;
	}
	tse_cache_sync_dma_from_dev(&out0_buf[0]);
	tse_set_out0_reg(&out0_buf[0]);

	/* set in/out pointer to next buffer */
	pin_buf = in_buf[0].pnext;
	pout0_buf = out0_buf[0].pnext;

	/* clear int status */
	TSE_SETREG(TSE_INTSTS_REG_OFS, 0xffffffff);

	/* set int enable */
	int_reg.reg = 0x0;
	int_reg.bit.CMPL = 1;

	if (bWait == TSE_BUSY_POLLING) {
		int_reg.reg = 0;
	}

#if (TSE_POLLING == ENABLE)
	int_reg.reg = 0;
#endif
	TSE_SETREG(TSE_INTEN_REG_OFS, int_reg.reg);

	/* trigger */
	tse_platform_clr_flg();
	ctl.reg = 0x0;
	ctl.bit.START = 1;
	ctl.bit.LAST_BLOCK = 1;
	TSE_SETREG(TSE_OP_CTRL_REG_OFS, ctl.reg);

	/* wait end */
	if (bWait) {
		if (bWait == TSE_BUSY_POLLING) {
			rt = tse_busy_polling_end();
		} else {
			rt = tse_wait_end();
		}

		if (rt == 0) {
			return E_TMOUT;
		}
		tse_hwcpy_end();
	}
	return E_OK;
}

#if 0
#endif

static TSE_MODE_INFO mode_tab[] = {
	{tse_mux_start, tse_mux_end},
	{tse_demux_start, tse_demux_end},
	{tse_hwcpy_start, tse_hwcpy_end},
};

/*
    TSE ISR

    This function is called by the IST.
*/
void tse_isr(void)
{
	T_TSE_INTSTS_REG RegSts;

	UINT32 int_sts;

	// Get the interrupt status and AND with the enable bits
	// Because we only handle the enabled interrupt status bits only.
	RegSts.reg = TSE_GETREG(TSE_INTSTS_REG_OFS);
	//DBG_DUMP("tse int sts = 0x%.8x\r\n", RegSts.reg);
	tse_int_all_status |= RegSts.reg;
	// Clear interrupt status
	TSE_SETREG(TSE_INTSTS_REG_OFS, RegSts.reg);
	int_sts = (RegSts.reg & TSE_GETREG(TSE_INTEN_REG_OFS));

	if (RegSts.reg & TSE_INT_ERR0_STS) {
		DBG_DUMP("ERR0: 0x%.8x\r\n", (int)(RegSts.reg & TSE_INT_ERR0_STS));
	}

	if (RegSts.reg & TSE_INT_ERR1_STS) {
		DBG_DUMP("ERR1: 0x%.8x\r\n", (int)(RegSts.reg & TSE_INT_ERR1_STS));
	}

	if (RegSts.reg & TSE_INT_ERR2_STS) {
		DBG_DUMP("ERR2: 0x%.8x\r\n", (int)(RegSts.reg & TSE_INT_ERR2_STS));
	}

	if (int_sts & INT_STS_COMPLETE) {

		tse_platform_set_flg();

	} else if (int_sts & (INT_STS_INPUT_END | INT_STS_OUT0_FULL | INT_STS_OUT1_FULL | INT_STS_OUT2_FULL)) {
		tse_isr_trig_next(int_sts);
	}
}

/*
    TSE ReSource Lock
*/
static ER tse_lock(void)
{
	ER ret;

	ret = tse_platform_wait_sem();
	return ret;
}

/*
    TSE ReSource Un-Lock
*/
static ER tse_unlock(void)
{
	ER ret;

	ret = tse_platform_sig_sem();
	return ret;
}

/**
    Open TSE Module

    This API would open TSE's source clock/interrupt/init-reset, and also
    Set default value of SYNC=0x47, TEI=0, TP=0, PID=0x1FFF, SCRAMBLE_CTRL=0.

    @return
     - @b E_OK:    tsmux open done and success.
     - @b E_SYS:   OS system error due to semaphore lock fail.
*/
ER tse_open(void)
{
	unsigned long flag;
	ER ret = E_OK;
	UINT32 clk;

	// Use Semaphore lock for flow control protect
	tse_lock();

	flag = tse_platform_loc_cpu();
	if ((tse_status & TSE_STS_OPENED) == TSE_STS_OPENED) {
		DBG_ERR("tse already opened\r\n");
		ret = E_SYS;
	}

	if (ret != E_OK) {
		tse_platform_unl_cpu(flag);
		return ret;
	}

	tse_status |= TSE_STS_OPENED;
	tse_platform_unl_cpu(flag);

	clk = tse_cfg_getVal(TSE_CFG_ID_FREQ);
	tse_platform_attach(clk);

	// Clear interrupt Status and Enable bits
	TSE_SETREG(TSE_INTEN_REG_OFS,  0x00000000);
	TSE_SETREG(TSE_INTSTS_REG_OFS, 0xFFFFFFFF);

	// clear the interrupt flag
	tse_platform_clr_flg();

	// Enable interrupt
	tse_platform_enable_int();

	return ret;
}

/**
    Close TSE Module

    Close TSE Module access.

    @return
     - @b E_OK:    tse close done and success.
     - @b Others:  OS system error due to semaphore unlock fail.
*/
ER tse_close(void)
{
	unsigned long flag;
	ER ret = E_OK;

	flag = tse_platform_loc_cpu();

	if ((tse_status & TSE_STS_BUSY) == TSE_STS_BUSY){
		DBG_ERR("tse is busy\r\n");
		ret = E_SYS;
	}

	if ((tse_status & TSE_STS_OPENED) != TSE_STS_OPENED) {
		DBG_ERR("tse is not opened!\r\n");
		ret = E_SYS;
	}

	if (ret != E_OK) {
		tse_platform_unl_cpu(flag);
		return ret;
	}
	tse_status &= ~TSE_STS_OPENED;
	tse_platform_unl_cpu(flag);

	tse_platform_disable_int();

	tse_platform_detach();

	tse_unlock();
	return ret;
}

/**
    Check if TSE is opened or not.

    Check if TSE is opened or not.

    @return
     - @b TRUE:     TSE is already opened.
     - @b FALSE:    TSE is not opened.
*/
BOOL tse_isOpened(void)
{
	unsigned long flag;
	BOOL ret = FALSE;

	flag = tse_platform_loc_cpu();
	if ((tse_status & TSE_STS_OPENED) == TSE_STS_OPENED) {
		ret = TRUE;
	}
	tse_platform_unl_cpu(flag);

	return ret;
}

/**
    Start TSE

    Start TSE

    @param[in]  OP_MODE
     - TSE operation mode select
     - 0x0: TSE_MODE_TSMUX
     - 0x1: TSE_MODE_TSDEMUX
     - 0x2: TSE_MODE_HWCOPY
     - 0x3: Reserved

    @param[in]  bWait
     - @b 2: No Wait the muxing operation done and disable interrupt. Return immediately (Asynchronous).
                 User can check if done by tsmux_pollDone().
     - @b 1:  Wait the muxing operation done and then return from this API (Synchronous).
     - @b 0: No Wait the muxing operation done. Return immediately (Asynchronous).
                 User can check if done by tsmux_waitDone().

    @return
     - @b E_OK:     Operation done.
     - @b E_OACV:   TSE no opened or parameters error.
*/
ER tse_start(UINT32 bWait, TSE_MODE_NUM OP_MODE)
{
	unsigned long flag;
	ER rt = E_SYS;

	flag = tse_platform_loc_cpu();
	if ((tse_status & TSE_STS_OPENED) != TSE_STS_OPENED) {
		DBG_ERR("tse is not opened!\r\n");
		tse_platform_unl_cpu(flag);
		return rt;
	}

	if ((tse_status & TSE_STS_BUSY) == TSE_STS_BUSY) {
		DBG_ERR("tse is busy!\r\n");
		tse_platform_unl_cpu(flag);
		return rt;
	}

	tse_status |= TSE_STS_BUSY;
	tse_platform_unl_cpu(flag);

	pin_buf = NULL;
	pout0_buf = NULL;
	pout1_buf = NULL;
	pout2_buf = NULL;
	tse_int_all_status = 0;

	if (OP_MODE < TSE_MODE_MAX_NUM) {

		rt = mode_tab[OP_MODE].trig(bWait);

		flag = tse_platform_loc_cpu();
		if ((bWait == TRUE) || (rt != E_OK)) {
			tse_status &= ~TSE_STS_BUSY;
		}
		tse_platform_unl_cpu(flag);

	} else {
		DBG_ERR("error op_mode = %d\n", OP_MODE);
	}
	return rt;
}

/**
    Wait TSE operation done

    If tse_start()'s bWait is set to FALSE, this API can be used to check if the
    TSE operation is done.

    @return
     - @b E_OK:  Operation is done.
*/
ER tse_waitDone(void)
{
	unsigned long flag;
	ER ret = E_OK;
	T_TSE_CFG_CTRL_REG cfg_ctl;

	flag = tse_platform_loc_cpu();

	if ((tse_status & TSE_STS_OPENED) != TSE_STS_OPENED) {
		DBG_ERR("tse is not opened!\r\n");
		ret = E_SYS;
	}

	if ((tse_status & TSE_STS_BUSY) != TSE_STS_BUSY) {
		DBG_ERR("must be trigger tse at first\r\n");
		ret = E_SYS;
	}

	tse_platform_unl_cpu(flag);

	if (ret != E_OK) {
		return ret;
	}

	//coverity[check_return]
	tse_wait_end();

	cfg_ctl.reg = TSE_GETREG(TSE_CFG_CTRL_REG_OFS);
	mode_tab[cfg_ctl.bit.TSE_MODE].done();

	flag = tse_platform_loc_cpu();
	tse_status &= ~TSE_STS_BUSY;
	tse_platform_unl_cpu(flag);
	return ret;
}

/**
    Set TSE Configurations

    Set TSE Configurations, such as payload-size/SRC-Buf-Addr/DST-Buf-Addr/
    Source-Data-Size/CONTINUITY-init-value settings. Please refer to TSE_CFG_ID for details.

    @param[in]  CfgID       Configuration selection.
    @param[in]  uiCfgValue  Configuration Values.

    @return
     - @b E_OACV:   TSE not opened.
     - @b E_PAR:    No-exist CfgID.
     - @b E_OK:     Config done.
*/
ER tse_setConfig(TSE_CFG_ID CfgID, UINT32 uiCfgValue)
{
	unsigned long flag;
	ER ret = E_OK;

	flag = tse_platform_loc_cpu();

	if ((tse_status & TSE_STS_OPENED) != TSE_STS_OPENED) {

		if (CfgID != TSE_CFG_ID_FREQ) {
			ret = E_SYS;
			DBG_ERR("tse is not opened!\r\n");
			tse_platform_unl_cpu(flag);
			return ret;
		}
	}

	if (CfgID & TSE_CFG_MASK) {
		ret = tse_cfg_setConfig(CfgID, uiCfgValue);
	} else if (CfgID & TSE_MUX_MASK) {
		ret = tse_mux_setConfig(CfgID, uiCfgValue);
	} else if (CfgID & TSE_DEMUX_MASK) {
		ret = tse_demux_setConfig(CfgID, uiCfgValue);
	} else if (CfgID & TSE_HWCPY_MASK) {
		ret = tse_hwcpy_setConfig(CfgID, uiCfgValue);
	} else {
		ret = E_SYS;
		DBG_ERR("not support CfgID = 0x%.8x\r\n", CfgID);
	}

	tse_platform_unl_cpu(flag);
	return ret;
}

/**
    Get TSE Configurations

    Get TSE Configurations, such as payload-size/SRC-Buf-Addr/DST-Buf-Addr/
    Source-Data-Size/CONTINUITY-init-value settings. Please refer to TSE_CFG_ID for details.

    @param[in]  CfgID       Configuration selection.

    @return Configuration Values.
*/
UINT32 tse_getConfig(TSE_CFG_ID CfgID)
{
	UINT32 rt;

	rt = 0;

	if (CfgID & TSE_CFG_MASK) {
		rt = tse_cfg_getConfig(CfgID);
	} else if (CfgID & TSE_MUX_MASK) {
		rt = tse_mux_getConfig(CfgID);
	} else if (CfgID & TSE_DEMUX_MASK) {
		rt = tse_demux_getConfig(CfgID);
	} else if (CfgID & TSE_HWCPY_MASK) {
		rt = tse_hwcpy_getConfig(CfgID);
	} else {
		DBG_ERR("not support CfgID = 0x%.8x\r\n", CfgID);
	}

	return rt;
}

/**
    Get TSE int status

    @return int status.
*/
UINT32 tse_getIntStatus(void)
{
	return tse_int_all_status;
}

/**
    tse init(only for free rtos)

    @return int status.
*/
UINT32 tse_init(void)
{
#if (defined __FREERTOS)
	tse_platform_set_resource();
#endif
	return 0;
}

/**
    tse uninit(only for free rtos)

    @return unint status.
*/
UINT32 tse_uninit(void)
{
#if (defined __FREERTOS)
	tse_platform_release_resource();
#endif
	return 0;
}
//@}

