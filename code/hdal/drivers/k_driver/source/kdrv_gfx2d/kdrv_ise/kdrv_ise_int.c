#include "kwrap/util.h"
#include "comm/hwclock.h"
#include "kdrv_gfx2d/kdrv_ise.h"
#include "kdrv_gfx2d/kdrv_ise_lmt.h"
#include "kdrv_ise_int_tsk.h"
#include "kdrv_ise_int.h"
#include "kdrv_ise_int_dbg.h"
#include "ise_eng.h"

#define KDRV_ISE_ENG_DFT_IN_BURST_LENGTH (ISE_ENG_BURST_32W)
#define KDRV_ISE_ENG_DFT_OUT_BURST_LENGTH (ISE_ENG_BURST_32W)

static BOOL kdrv_ise_chk_align(UINT32 val, UINT32 align)
{
	if (align == 0) {
		return TRUE;
	}

	return ((val % align) == 0);
}

static INT32 kdrv_ise_typecast_eng_scl_method(KDRV_ISE_SCALE_METHOD method)
{
	switch (method) {
	case KDRV_ISE_BILINEAR:
		return ISE_ENG_SCALE_BILINEAR;

	case KDRV_ISE_NEAREST:
		return ISE_ENG_SCALE_NEAREST;

	case KDRV_ISE_INTEGRATION:
		return ISE_ENG_SCALE_INTEGRATION;

	default:
		DBG_WRN("Unknown scale method %d\r\n", (int)method);
		return -1;
	}
}

static INT32 kdrv_ise_typecast_eng_io_fmt(KDRV_ISE_IO_PACK_SEL fmt)
{
	switch (fmt) {
	case KDRV_ISE_Y8:
		return ISE_ENG_IOFMT_Y8;

	case KDRV_ISE_Y4:
		return ISE_ENG_IOFMT_Y4;

	case KDRV_ISE_Y1:
		return ISE_ENG_IOFMT_Y1;

	case KDRV_ISE_UVP:
		return ISE_ENG_IOFMT_UVP;

	case KDRV_ISE_RGB565:
		return ISE_ENG_IOFMT_RGB565;

	case KDRV_ISE_ARGB8888:
		return ISE_ENG_IOFMT_ARGB8888;

	case KDRV_ISE_ARGB1555:
		return ISE_ENG_IOFMT_ARGB1555;

	case KDRV_ISE_ARGB4444:
		return ISE_ENG_IOFMT_ARGB4444;

	case KDRV_ISE_YUVP:
		return ISE_ENG_IOFMT_YUVP;

	default:
		DBG_WRN("Unknown iofmt %d\r\n", (int)fmt);
		return -1;
	}
}

static INT32 kdrv_ise_typecast_eng_argb_outmode(KDRV_ISE_ARGB_OUTMODE_SEL mode)
{
	switch (mode) {
	case KDRV_ISE_OUTMODE_ARGB8888:
		return ISE_ENG_OUTMODE_ARGB8888;

	case KDRV_ISE_OUTMODE_RGB888:
		return ISE_ENG_OUTMODE_RGB8888;

	case KDRV_ISE_OUTMODE_A8:
		return ISE_ENG_OUTMODE_A8;

	default:
		DBG_WRN("Unknown argb outmode sel %d\r\n", (int)mode);
		return -1;
	}
}

static INT32 kdrv_ise_int_get_limitation(KDRV_ISE_IO_PACK_SEL fmt, KDRV_ISE_LMT *p_lmt)
{
	switch (fmt) {
	case KDRV_ISE_Y8:
		p_lmt->in_w_min = ISE_SRCBUF_Y8BIT_WMIN;
		p_lmt->in_w_max = ISE_SRCBUF_Y8BIT_WMAX;
		p_lmt->in_w_align = ISE_SRCBUF_Y8BIT_WALIGN;
		p_lmt->in_h_min = ISE_SRCBUF_Y8BIT_HMIN;
		p_lmt->in_h_max = ISE_SRCBUF_Y8BIT_HMAX;
		p_lmt->in_h_align = ISE_SRCBUF_Y8BIT_HALIGN;
		p_lmt->in_lofs_align = ISE_SRCBUF_Y8BIT_LOFF_ALIGN;
		p_lmt->in_addr_align = ISE_SRCBUF_Y8BIT_ADDR_ALIGN;

		p_lmt->out_w_min = ISE_DSTBUF_Y8BIT_WMIN;
		p_lmt->out_w_max = ISE_DSTBUF_Y8BIT_WMAX;
		p_lmt->out_w_align = ISE_DSTBUF_Y8BIT_WALIGN;
		p_lmt->out_h_min = ISE_DSTBUF_Y8BIT_HMIN;
		p_lmt->out_h_max = ISE_DSTBUF_Y8BIT_HMAX;
		p_lmt->out_h_align = ISE_DSTBUF_Y8BIT_HALIGN;
		p_lmt->out_lofs_align = ISE_DSTBUF_Y8BIT_LOFF_ALIGN;
		p_lmt->out_addr_align = ISE_DSTBUF_Y8BIT_ADDR_ALIGN;
		break;

	case KDRV_ISE_Y4:
		p_lmt->in_w_min = ISE_SRCBUF_Y4BIT_WMIN;
		p_lmt->in_w_max = ISE_SRCBUF_Y4BIT_WMAX;
		p_lmt->in_w_align = ISE_SRCBUF_Y4BIT_WALIGN;
		p_lmt->in_h_min = ISE_SRCBUF_Y4BIT_HMIN;
		p_lmt->in_h_max = ISE_SRCBUF_Y4BIT_HMAX;
		p_lmt->in_h_align = ISE_SRCBUF_Y4BIT_HALIGN;
		p_lmt->in_lofs_align = ISE_SRCBUF_Y4BIT_LOFF_ALIGN;
		p_lmt->in_addr_align = ISE_SRCBUF_Y4BIT_ADDR_ALIGN;

		p_lmt->out_w_min = ISE_DSTBUF_Y4BIT_WMIN;
		p_lmt->out_w_max = ISE_DSTBUF_Y4BIT_WMAX;
		p_lmt->out_w_align = ISE_DSTBUF_Y4BIT_WALIGN;
		p_lmt->out_h_min = ISE_DSTBUF_Y4BIT_HMIN;
		p_lmt->out_h_max = ISE_DSTBUF_Y4BIT_HMAX;
		p_lmt->out_h_align = ISE_DSTBUF_Y4BIT_HALIGN;
		p_lmt->out_lofs_align = ISE_DSTBUF_Y4BIT_LOFF_ALIGN;
		p_lmt->out_addr_align = ISE_DSTBUF_Y4BIT_ADDR_ALIGN;
		break;

	case KDRV_ISE_Y1:
		/* TODO: CHECK Y1 LIMITATION */
		p_lmt->in_w_min = ISE_SRCBUF_Y8BIT_WMIN;
		p_lmt->in_w_max = ISE_SRCBUF_Y8BIT_WMAX;
		p_lmt->in_w_align = ISE_SRCBUF_Y8BIT_WALIGN;
		p_lmt->in_h_min = ISE_SRCBUF_Y8BIT_HMIN;
		p_lmt->in_h_max = ISE_SRCBUF_Y8BIT_HMAX;
		p_lmt->in_h_align = ISE_SRCBUF_Y8BIT_HALIGN;
		p_lmt->in_lofs_align = ISE_SRCBUF_Y8BIT_LOFF_ALIGN;
		p_lmt->in_addr_align = ISE_SRCBUF_Y8BIT_ADDR_ALIGN;

		p_lmt->out_w_min = ISE_DSTBUF_Y8BIT_WMIN;
		p_lmt->out_w_max = ISE_DSTBUF_Y8BIT_WMAX;
		p_lmt->out_w_align = ISE_DSTBUF_Y8BIT_WALIGN;
		p_lmt->out_h_min = ISE_DSTBUF_Y8BIT_HMIN;
		p_lmt->out_h_max = ISE_DSTBUF_Y8BIT_HMAX;
		p_lmt->out_h_align = ISE_DSTBUF_Y8BIT_HALIGN;
		p_lmt->out_lofs_align = ISE_DSTBUF_Y8BIT_LOFF_ALIGN;
		p_lmt->out_addr_align = ISE_DSTBUF_Y8BIT_ADDR_ALIGN;
		break;

	case KDRV_ISE_UVP:
		p_lmt->in_w_min = ISE_SRCBUF_UVP_WMIN;
		p_lmt->in_w_max = ISE_SRCBUF_UVP_WMAX;
		p_lmt->in_w_align = ISE_SRCBUF_UVP_WALIGN;
		p_lmt->in_h_min = ISE_SRCBUF_UVP_HMIN;
		p_lmt->in_h_max = ISE_SRCBUF_UVP_HMAX;
		p_lmt->in_h_align = ISE_SRCBUF_UVP_HALIGN;
		p_lmt->in_lofs_align = ISE_SRCBUF_UVP_LOFF_ALIGN;
		p_lmt->in_addr_align = ISE_SRCBUF_UVP_ADDR_ALIGN;

		p_lmt->out_w_min = ISE_DSTBUF_UVP_WMIN;
		p_lmt->out_w_max = ISE_DSTBUF_UVP_WMAX;
		p_lmt->out_w_align = ISE_DSTBUF_UVP_WALIGN;
		p_lmt->out_h_min = ISE_DSTBUF_UVP_HMIN;
		p_lmt->out_h_max = ISE_DSTBUF_UVP_HMAX;
		p_lmt->out_h_align = ISE_DSTBUF_UVP_HALIGN;
		p_lmt->out_lofs_align = ISE_DSTBUF_UVP_LOFF_ALIGN;
		p_lmt->out_addr_align = ISE_DSTBUF_UVP_ADDR_ALIGN;
		break;

	case KDRV_ISE_RGB565:
		p_lmt->in_w_min = ISE_SRCBUF_RGB565_WMIN;
		p_lmt->in_w_max = ISE_SRCBUF_RGB565_WMAX;
		p_lmt->in_w_align = ISE_SRCBUF_RGB565_WALIGN;
		p_lmt->in_h_min = ISE_SRCBUF_RGB565_HMIN;
		p_lmt->in_h_max = ISE_SRCBUF_RGB565_HMAX;
		p_lmt->in_h_align = ISE_SRCBUF_RGB565_HALIGN;
		p_lmt->in_lofs_align = ISE_SRCBUF_RGB565_LOFF_ALIGN;
		p_lmt->in_addr_align = ISE_SRCBUF_RGB565_ADDR_ALIGN;

		p_lmt->out_w_min = ISE_DSTBUF_RGB565_WMIN;
		p_lmt->out_w_max = ISE_DSTBUF_RGB565_WMAX;
		p_lmt->out_w_align = ISE_DSTBUF_RGB565_WALIGN;
		p_lmt->out_h_min = ISE_DSTBUF_RGB565_HMIN;
		p_lmt->out_h_max = ISE_DSTBUF_RGB565_HMAX;
		p_lmt->out_h_align = ISE_DSTBUF_RGB565_HALIGN;
		p_lmt->out_lofs_align = ISE_DSTBUF_RGB565_LOFF_ALIGN;
		p_lmt->out_addr_align = ISE_DSTBUF_RGB565_ADDR_ALIGN;
		break;

	case KDRV_ISE_ARGB8888:
		p_lmt->in_w_min = ISE_SRCBUF_RGB8888_WMIN;
		p_lmt->in_w_max = ISE_SRCBUF_RGB8888_WMAX;
		p_lmt->in_w_align = ISE_SRCBUF_RGB8888_WALIGN;
		p_lmt->in_h_min = ISE_SRCBUF_RGB8888_HMIN;
		p_lmt->in_h_max = ISE_SRCBUF_RGB8888_HMAX;
		p_lmt->in_h_align = ISE_SRCBUF_RGB8888_HALIGN;
		p_lmt->in_lofs_align = ISE_SRCBUF_RGB8888_LOFF_ALIGN;
		p_lmt->in_addr_align = ISE_SRCBUF_RGB8888_ADDR_ALIGN;

		p_lmt->out_w_min = ISE_DSTBUF_RGB8888_WMIN;
		p_lmt->out_w_max = ISE_DSTBUF_RGB8888_WMAX;
		p_lmt->out_w_align = ISE_DSTBUF_RGB8888_WALIGN;
		p_lmt->out_h_min = ISE_DSTBUF_RGB8888_HMIN;
		p_lmt->out_h_max = ISE_DSTBUF_RGB8888_HMAX;
		p_lmt->out_h_align = ISE_DSTBUF_RGB8888_HALIGN;
		p_lmt->out_lofs_align = ISE_DSTBUF_RGB8888_LOFF_ALIGN;
		p_lmt->out_addr_align = ISE_DSTBUF_RGB8888_ADDR_ALIGN;
		break;

	case KDRV_ISE_ARGB1555:
		p_lmt->in_w_min = ISE_SRCBUF_RGB1555_WMIN;
		p_lmt->in_w_max = ISE_SRCBUF_RGB1555_WMAX;
		p_lmt->in_w_align = ISE_SRCBUF_RGB1555_WALIGN;
		p_lmt->in_h_min = ISE_SRCBUF_RGB1555_HMIN;
		p_lmt->in_h_max = ISE_SRCBUF_RGB1555_HMAX;
		p_lmt->in_h_align = ISE_SRCBUF_RGB1555_HALIGN;
		p_lmt->in_lofs_align = ISE_SRCBUF_RGB1555_LOFF_ALIGN;
		p_lmt->in_addr_align = ISE_SRCBUF_RGB1555_ADDR_ALIGN;

		p_lmt->out_w_min = ISE_DSTBUF_RGB1555_WMIN;
		p_lmt->out_w_max = ISE_DSTBUF_RGB1555_WMAX;
		p_lmt->out_w_align = ISE_DSTBUF_RGB1555_WALIGN;
		p_lmt->out_h_min = ISE_DSTBUF_RGB1555_HMIN;
		p_lmt->out_h_max = ISE_DSTBUF_RGB1555_HMAX;
		p_lmt->out_h_align = ISE_DSTBUF_RGB1555_HALIGN;
		p_lmt->out_lofs_align = ISE_DSTBUF_RGB1555_LOFF_ALIGN;
		p_lmt->out_addr_align = ISE_DSTBUF_RGB1555_ADDR_ALIGN;
		break;

	case KDRV_ISE_ARGB4444:
		p_lmt->in_w_min = ISE_SRCBUF_RGB4444_WMIN;
		p_lmt->in_w_max = ISE_SRCBUF_RGB4444_WMAX;
		p_lmt->in_w_align = ISE_SRCBUF_RGB4444_WALIGN;
		p_lmt->in_h_min = ISE_SRCBUF_RGB4444_HMIN;
		p_lmt->in_h_max = ISE_SRCBUF_RGB4444_HMAX;
		p_lmt->in_h_align = ISE_SRCBUF_RGB4444_HALIGN;
		p_lmt->in_lofs_align = ISE_SRCBUF_RGB4444_LOFF_ALIGN;
		p_lmt->in_addr_align = ISE_SRCBUF_RGB4444_ADDR_ALIGN;

		p_lmt->out_w_min = ISE_DSTBUF_RGB4444_WMIN;
		p_lmt->out_w_max = ISE_DSTBUF_RGB4444_WMAX;
		p_lmt->out_w_align = ISE_DSTBUF_RGB4444_WALIGN;
		p_lmt->out_h_min = ISE_DSTBUF_RGB4444_HMIN;
		p_lmt->out_h_max = ISE_DSTBUF_RGB4444_HMAX;
		p_lmt->out_h_align = ISE_DSTBUF_RGB4444_HALIGN;
		p_lmt->out_lofs_align = ISE_DSTBUF_RGB4444_LOFF_ALIGN;
		p_lmt->out_addr_align = ISE_DSTBUF_RGB4444_ADDR_ALIGN;
		break;

	case KDRV_ISE_YUVP:
		p_lmt->in_w_min = ISE_SRCBUF_YUVP_WMIN;
		p_lmt->in_w_max = ISE_SRCBUF_YUVP_WMAX;
		p_lmt->in_w_align = ISE_SRCBUF_YUVP_WALIGN;
		p_lmt->in_h_min = ISE_SRCBUF_YUVP_HMIN;
		p_lmt->in_h_max = ISE_SRCBUF_YUVP_HMAX;
		p_lmt->in_h_align = ISE_SRCBUF_YUVP_HALIGN;
		p_lmt->in_lofs_align = ISE_SRCBUF_YUVP_LOFF_ALIGN;
		p_lmt->in_addr_align = ISE_SRCBUF_YUVP_ADDR_ALIGN;

		p_lmt->out_w_min = ISE_DSTBUF_YUVP_WMIN;
		p_lmt->out_w_max = ISE_DSTBUF_YUVP_WMAX;
		p_lmt->out_w_align = ISE_DSTBUF_YUVP_WALIGN;
		p_lmt->out_h_min = ISE_DSTBUF_YUVP_HMIN;
		p_lmt->out_h_max = ISE_DSTBUF_YUVP_HMAX;
		p_lmt->out_h_align = ISE_DSTBUF_YUVP_HALIGN;
		p_lmt->out_lofs_align = ISE_DSTBUF_YUVP_LOFF_ALIGN;
		p_lmt->out_addr_align = ISE_DSTBUF_YUVP_ADDR_ALIGN;
		break;

	default:
		DBG_ERR("Unknown iofmt %d\r\n", (int)fmt);
		return E_PAR;
	}

	return E_OK;
}

static INT32 kdrv_ise_int_write_ll_upd_cmd(KDRV_ISE_LL_BLK *p_ll_blk, ISE_ENG_REG *p_reg)
{
	KDRV_ISE_LL_CMD *p_cmd;

	if (p_ll_blk->cur_cmd_idx >= p_ll_blk->max_cmd_num) {
		DBG_ERR("ll cmd index overflow(%d >= %d)\r\n", (int)p_ll_blk->cur_cmd_idx, (int)p_ll_blk->max_cmd_num);
		return -1;
	}

	p_cmd = (KDRV_ISE_LL_CMD *)p_ll_blk->cmd_buf_addr;

	/*
		0b100(4) for update cmd
		always update all byte
	*/
	p_cmd[p_ll_blk->cur_cmd_idx].val = 0;
	p_cmd[p_ll_blk->cur_cmd_idx].upd_bit.reg_val = p_reg->val;
	p_cmd[p_ll_blk->cur_cmd_idx].upd_bit.reg_ofs = p_reg->ofs;
	p_cmd[p_ll_blk->cur_cmd_idx].upd_bit.byte_en = 0xF;
	p_cmd[p_ll_blk->cur_cmd_idx].upd_bit.cmd = KDRV_ISE_LL_CMD_TYPE_UPD;
	p_ll_blk->cur_cmd_idx += 1;

	return E_OK;
}

static INT32 kdrv_ise_int_write_ll_null_cmd(KDRV_ISE_LL_BLK *p_ll_blk, UINT32 idx)
{
	KDRV_ISE_LL_CMD *p_cmd;

	if (p_ll_blk->cur_cmd_idx >= p_ll_blk->max_cmd_num) {
		DBG_ERR("ll cmd index overflow(%d >= %d)\r\n", (int)p_ll_blk->cur_cmd_idx, (int)p_ll_blk->max_cmd_num);
		return -1;
	}

	p_cmd = (KDRV_ISE_LL_CMD *)p_ll_blk->cmd_buf_addr;
	p_cmd[p_ll_blk->cur_cmd_idx].val = 0;
	p_cmd[p_ll_blk->cur_cmd_idx].null_bit.table_index = idx;
	p_cmd[p_ll_blk->cur_cmd_idx].null_bit.cmd = KDRV_ISE_LL_CMD_TYPE_NULL;
	p_ll_blk->cur_cmd_idx += 1;

	return E_OK;
}

static INT32 kdrv_ise_int_write_ll_nxtll_cmd(KDRV_ISE_LL_BLK *p_ll_blk, UINT32 idx, UINT32 next_ll)
{
	KDRV_ISE_LL_CMD *p_cmd;

	if (p_ll_blk->cur_cmd_idx >= p_ll_blk->max_cmd_num) {
		DBG_ERR("ll cmd index overflow(%d >= %d)\r\n", (int)p_ll_blk->cur_cmd_idx, (int)p_ll_blk->max_cmd_num);
		return -1;
	}

	p_cmd = (KDRV_ISE_LL_CMD *)p_ll_blk->cmd_buf_addr;
	p_cmd[p_ll_blk->cur_cmd_idx].val = 0;
	p_cmd[p_ll_blk->cur_cmd_idx].nxtll_bit.table_index = idx;
	p_cmd[p_ll_blk->cur_cmd_idx].nxtll_bit.next_ll_addr = next_ll;
	p_cmd[p_ll_blk->cur_cmd_idx].nxtll_bit.cmd = KDRV_ISE_LL_CMD_TYPE_NXTLL;
	p_ll_blk->cur_cmd_idx += 1;

	return E_OK;
}

static INT32 kdrv_ise_int_write_reg(ISE_ENG_HANDLE *p_eng, ISE_ENG_REG reg, KDRV_ISE_LL_BLK *p_ll_blk)
{
	if (p_eng == NULL)
		return E_PAR;

	if (p_ll_blk == NULL) {
		ise_eng_write_reg(p_eng, reg.ofs, reg.val);
	} else {
		kdrv_ise_int_write_ll_upd_cmd(p_ll_blk, &reg);
	}

	return E_OK;
}

static INT32 kdrv_ise_job_iocfg_check(KDRV_ISE_IO_CFG *p_iocfg)
{
	KDRV_ISE_LMT lmt;

	if (kdrv_ise_int_get_limitation(p_iocfg->io_pack_fmt, &lmt) != E_OK) {
		return E_PAR;
	}

	if (p_iocfg->in_width < lmt.in_w_min) {
		DBG_ERR("in_width %d < min %d\r\n", (int)p_iocfg->in_width, (int)lmt.in_w_min);
		return E_PAR;
	}

	if (p_iocfg->in_width > lmt.in_w_max) {
		DBG_ERR("in_width %d > max %d\r\n", (int)p_iocfg->in_width, (int)lmt.in_w_max);
		return E_PAR;
	}

	if (kdrv_ise_chk_align(p_iocfg->in_width, lmt.in_w_align) != TRUE) {
		DBG_ERR("in_width %d not align to %d\r\n", (int)p_iocfg->in_width, (int)lmt.in_w_align);
		return E_PAR;
	}

	if (p_iocfg->in_height < lmt.in_h_min) {
		DBG_ERR("in_height %d < min %d\r\n", (int)p_iocfg->in_height, (int)lmt.in_h_min);
		return E_PAR;
	}

	if (p_iocfg->in_height > lmt.in_h_max) {
		DBG_ERR("in_height %d > max %d\r\n", (int)p_iocfg->in_height, (int)lmt.in_h_max);
		return E_PAR;
	}

	if (kdrv_ise_chk_align(p_iocfg->in_height, lmt.in_h_align) != TRUE) {
		DBG_ERR("in_height %d not align to %d\r\n", (int)p_iocfg->in_height, (int)lmt.in_h_align);
		return E_PAR;
	}

	if (kdrv_ise_chk_align(p_iocfg->in_lofs, lmt.in_lofs_align) != TRUE) {
		DBG_ERR("in_lofs %d not align to %d\r\n", (int)p_iocfg->in_lofs, (int)lmt.in_lofs_align);
		return E_PAR;
	}

	if (kdrv_ise_chk_align(p_iocfg->in_addr, lmt.in_addr_align) != TRUE) {
		DBG_ERR("in_addr 0x%.8x not align to %d\r\n", (unsigned int)p_iocfg->in_addr, (int)lmt.in_lofs_align);
		return E_PAR;
	}

	if (p_iocfg->out_width < lmt.out_w_min) {
		DBG_ERR("out_width %d < min %d\r\n", (int)p_iocfg->out_width, (int)lmt.out_w_min);
		return E_PAR;
	}

	if (p_iocfg->out_width > lmt.out_w_max) {
		DBG_ERR("out_width %d > max %d\r\n", (int)p_iocfg->out_width, (int)lmt.out_w_max);
		return E_PAR;
	}

	if (kdrv_ise_chk_align(p_iocfg->out_width, lmt.out_w_align) != TRUE) {
		DBG_ERR("out_width %d not align to %d\r\n", (int)p_iocfg->out_width, (int)lmt.out_w_align);
		return E_PAR;
	}

	if (p_iocfg->out_height < lmt.out_h_min) {
		DBG_ERR("out_height %d < min %d\r\n", (int)p_iocfg->out_height, (int)lmt.out_h_min);
		return E_PAR;
	}

	if (p_iocfg->out_height > lmt.out_h_max) {
		DBG_ERR("out_height %d > max %d\r\n", (int)p_iocfg->out_height, (int)lmt.out_h_max);
		return E_PAR;
	}

	if (kdrv_ise_chk_align(p_iocfg->out_height, lmt.out_h_align) != TRUE) {
		DBG_ERR("out_height %d not align to %d\r\n", (int)p_iocfg->out_height, (int)lmt.out_h_align);
		return E_PAR;
	}

	if (kdrv_ise_chk_align(p_iocfg->out_lofs, lmt.out_lofs_align) != TRUE) {
		DBG_ERR("out_lofs %d not align to %d\r\n", (int)p_iocfg->out_lofs, (int)lmt.out_lofs_align);
		return E_PAR;
	}

	if (kdrv_ise_chk_align(p_iocfg->out_addr, lmt.out_addr_align) != TRUE) {
		DBG_ERR("out_addr 0x%.8x not align to %d\r\n", (unsigned int)p_iocfg->out_addr, (int)lmt.out_lofs_align);
		return E_PAR;
	}

	/* scale method check with scale up, integration not support scale up */
	if (p_iocfg->scl_method == KDRV_ISE_INTEGRATION) {
		if ((p_iocfg->out_width > p_iocfg->in_width) ||
			(p_iocfg->out_height > p_iocfg->in_height)) {
			DBG_ERR("integration scale only support down-scale\r\n");
			return E_NOSPT;
		}
	}

	/* scale method check with format */
	switch (p_iocfg->scl_method) {
	case KDRV_ISE_BILINEAR:
		if (p_iocfg->io_pack_fmt == KDRV_ISE_Y1) {
			DBG_ERR("bilinear not support format %d\r\n", p_iocfg->io_pack_fmt);
			return E_NOSPT;
		}
		break;

	case KDRV_ISE_NEAREST:

		break;

	case KDRV_ISE_INTEGRATION:
		if (p_iocfg->io_pack_fmt == KDRV_ISE_Y1 ||
			p_iocfg->io_pack_fmt == KDRV_ISE_Y4 ||
			p_iocfg->io_pack_fmt == KDRV_ISE_RGB565 ||
			p_iocfg->io_pack_fmt == KDRV_ISE_ARGB8888 ||
			p_iocfg->io_pack_fmt == KDRV_ISE_ARGB1555 ||
			p_iocfg->io_pack_fmt == KDRV_ISE_ARGB4444) {
			DBG_ERR("integration not support format %d\r\n", p_iocfg->io_pack_fmt);
			return E_NOSPT;
		}
		break;

	default:
		DBG_ERR("Unknown scale method %d\r\n", p_iocfg->scl_method);
		break;
	}

	if (p_iocfg->phy_addr_en) {
		if (p_iocfg->in_buf_flush == KDRV_ISE_DO_BUF_FLUSH) {
			DBG_ERR("skip in_buf_flush when phy_addr\r\n");
			p_iocfg->in_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;
		}

		if (p_iocfg->out_buf_flush == KDRV_ISE_DO_BUF_FLUSH) {
			DBG_ERR("skip out_buf_flush when phy_addr\r\n");
			p_iocfg->out_buf_flush = KDRV_ISE_NOTDO_BUF_FLUSH;
		}
	}

	return E_OK;
}

INT32 kdrv_ise_job_io_cfg(KDRV_ISE_JOB_CFG *p_cfg, ISE_ENG_HANDLE *p_eng, KDRV_ISE_LL_BLK *p_ll_blk)
{
	ISE_ENG_REG reg;
	ISE_ENG_STRIPE_INFO strp_info = {0};
	ISE_ENG_SCALE_FACTOR_INFO scl_factor = {0};
	ISE_ENG_SCALE_FILTER_INFO scl_filter = {0};
	KDRV_ISE_IO_CFG *p_iocfg;
	INT32 eng_scl_method;
	INT32 eng_iofmt;
	INT32 eng_argb_outmode;
	UINT32 buf_size;

	if (p_cfg == NULL || p_eng == NULL) {
		DBG_ERR(" NULL p_cfg or p_eng\r\n");
		return E_PAR;
	}

	p_iocfg = &p_cfg->io_cfg;
	if (kdrv_ise_job_iocfg_check(p_iocfg) != E_OK) {
		DBG_ERR(" kdrv_ise_job_iocfg_check fail\r\n");
		return E_PAR;
	}

	eng_scl_method = kdrv_ise_typecast_eng_scl_method(p_iocfg->scl_method);
	eng_iofmt = kdrv_ise_typecast_eng_io_fmt(p_iocfg->io_pack_fmt);
	if (eng_iofmt == KDRV_ISE_ARGB8888) {
		eng_argb_outmode = kdrv_ise_typecast_eng_argb_outmode(p_iocfg->argb_out_mode);
	} else {
		eng_argb_outmode = 0;
	}
	if (eng_scl_method < 0 || eng_iofmt < 0) {
		DBG_ERR(" eng_scl_method or eng_iofmt error\r\n");
		return E_PAR;
	}

	/* parameter calculation */
	ise_eng_cal_stripe(p_iocfg->in_width, p_iocfg->out_width, eng_scl_method, &strp_info);
	ise_eng_cal_scale_factor(p_iocfg->in_width, p_iocfg->in_height, p_iocfg->out_width, p_iocfg->out_height, eng_scl_method, &scl_factor);
	ise_eng_cal_scale_filter(p_iocfg->in_width, p_iocfg->in_height, p_iocfg->out_width, p_iocfg->out_height, &scl_filter);

	/* flush input buffer before hw trigger if need */
	if (p_iocfg->in_buf_flush == KDRV_ISE_DO_BUF_FLUSH) {
		buf_size = p_iocfg->in_lofs * p_iocfg->in_height;
		buf_size = ALIGN_CEIL(buf_size, VOS_ALIGN_BYTES);
		vos_cpu_dcache_sync(p_iocfg->in_addr, buf_size, VOS_DMA_TO_DEVICE);
	}

	/* flush output buffer before hw trigger if need, lofs may not equal to width */
	if (p_iocfg->out_buf_flush == KDRV_ISE_DO_BUF_FLUSH) {
		buf_size = p_iocfg->out_lofs * p_iocfg->out_height;
		buf_size = ALIGN_CEIL(buf_size, VOS_ALIGN_BYTES);
		vos_cpu_dcache_sync(p_iocfg->out_addr, buf_size, VOS_DMA_BIDIRECTIONAL);
	}

	/* keep output buffer flush info for linklist, flush when job end */
	if (p_ll_blk) {
		p_ll_blk->out_buf_flush = p_iocfg->out_buf_flush;
		p_ll_blk->out_buf_addr = p_iocfg->out_addr;
		p_ll_blk->out_buf_size = p_iocfg->out_lofs * p_iocfg->out_height;
		p_ll_blk->out_buf_size = ALIGN_CEIL(p_ll_blk->out_buf_size, VOS_ALIGN_BYTES);
	}

	/* register configuration */
	/* interrupt enable */
	if (p_ll_blk ==NULL) {
		reg = ise_eng_gen_int_en_reg(ISE_ENG_INTERRUPT_FMD);
	} else {
		reg = ise_eng_gen_int_en_reg(ISE_ENG_INTERRUPT_LLEND);
	}
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	/* input image */
	reg = ise_eng_gen_in_ctrl_reg(eng_iofmt, eng_scl_method, eng_argb_outmode,
							KDRV_ISE_ENG_DFT_IN_BURST_LENGTH, KDRV_ISE_ENG_DFT_OUT_BURST_LENGTH, strp_info.overlap_mode);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_in_size_reg(p_iocfg->in_width, p_iocfg->in_height);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_in_strp_reg(strp_info.stripe_size, strp_info.overlap_size_sel);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_in_lofs_reg(p_iocfg->in_lofs);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	if (p_iocfg->phy_addr_en) {
		reg = ise_eng_gen_in_addr_reg(p_iocfg->in_addr);
	} else {
		UINT32 addr = 0;
		addr =vos_cpu_get_phy_addr((VOS_ADDR)p_iocfg->in_addr);
		if( (INT32)addr < 0){
			DBG_ERR("in_addr va_to_pa_fail:%x\n", p_iocfg->in_addr);
			return E_PAR;
		}
		else
			reg = ise_eng_gen_in_addr_reg(addr);
	}
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	/* output image */
	reg = ise_eng_gen_out_size_reg(p_iocfg->out_width, p_iocfg->out_height);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_out_lofs_reg(p_iocfg->out_lofs);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	if (p_iocfg->phy_addr_en) {
		reg = ise_eng_gen_out_addr_reg(p_iocfg->out_addr);
	} else {
		UINT32 addr = 0;
		addr = vos_cpu_get_phy_addr((VOS_ADDR)p_iocfg->out_addr);
		if( (INT32)addr < 0){
			DBG_ERR("out_addr va_to_pa_fail:%x\n", p_iocfg->out_addr);
			return E_PAR;
		}
		else
			reg = ise_eng_gen_out_addr_reg(addr);
	}
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_out_ctrl_reg0(&scl_factor, &scl_filter);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_out_ctrl_reg1(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	/* scale parameters */
	reg = ise_eng_gen_scl_ofs_reg0(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_ofs_reg1(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	reg = ise_eng_gen_scl_isd_base_reg(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_fact_reg0(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_fact_reg1(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_fact_reg2(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	reg = ise_eng_gen_scl_isd_coef_ctrl_reg(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_h_reg0(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_h_reg1(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_h_reg2(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_h_reg3(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_h_reg4(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_h_reg5(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_h_reg6(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	reg = ise_eng_gen_scl_isd_coef_v_reg0(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_v_reg1(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_v_reg2(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_v_reg3(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_v_reg4(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_v_reg5(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_v_reg6(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	reg = ise_eng_gen_scl_isd_coef_h_all(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_h_half(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	reg = ise_eng_gen_scl_isd_coef_v_all(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);
	reg = ise_eng_gen_scl_isd_coef_v_half(&scl_factor);
	kdrv_ise_int_write_reg(p_eng, reg, p_ll_blk);

	return E_OK;
}

INT32 kdrv_ise_job_process_cpu(KDRV_ISE_JOB_CFG *p_cfg, ISE_ENG_HANDLE *p_eng)
{
	INT32 rt;

	if (p_cfg == NULL || p_eng == NULL) {
		DBG_ERR(" NULL p_cfg or p_eng\r\n");
		return E_PAR;
	}

	rt = kdrv_ise_job_io_cfg(p_cfg, p_eng, NULL);

	if (rt == E_OK) {
		/* register cb & trigger ise */
		ise_eng_reg_isr_callback(p_eng, p_cfg->eng_isr_cb);
		ise_eng_trig_single(p_eng);
	} else {
		DBG_ERR(" kdrv_ise_job_io_cfg fail\r\n");
	}

	return rt;
}

INT32 kdrv_ise_job_process_ll(KDRV_ISE_JOB_CFG *p_cfg, ISE_ENG_HANDLE *p_eng, KDRV_ISE_LL_BLK *p_ll_blk, KDRV_ISE_LL_BLK *p_nxt_ll_blk)
{
	INT32 rt;

	if (p_cfg == NULL || p_eng == NULL) {
		DBG_ERR(" NULL p_cfg or p_eng\r\n");
		return E_PAR;
	}

	rt = kdrv_ise_job_io_cfg(p_cfg, p_eng, p_ll_blk);

	if (rt == E_OK) {
		if (p_nxt_ll_blk == NULL) {
			kdrv_ise_int_write_ll_null_cmd(p_ll_blk, p_ll_blk->blk_idx);
		} else {
			UINT32 addr = 0;
			addr = vos_cpu_get_phy_addr((VOS_ADDR)p_nxt_ll_blk->cmd_buf_addr);
			if( (INT32)addr < 0){
				DBG_ERR("ll va_to_pa fail:%x\n", p_nxt_ll_blk->cmd_buf_addr);
				return E_PAR;
			}
			else
				kdrv_ise_int_write_ll_nxtll_cmd(p_ll_blk, p_ll_blk->blk_idx, addr);
		}

		/* flush cmd table */
		vos_cpu_dcache_sync(p_ll_blk->cmd_buf_addr, (p_ll_blk->max_cmd_num * KDRV_ISE_LL_CMD_SIZE) , VOS_DMA_TO_DEVICE);
	} else {
		DBG_ERR(" kdrv_ise_job_io_cfg fail\r\n");
	}

	return rt;
}

