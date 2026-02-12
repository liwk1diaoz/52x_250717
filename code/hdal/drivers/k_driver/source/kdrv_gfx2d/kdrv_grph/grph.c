/*
    Graphic module driver

    (NT96680 version)

    @file       grph.c
    @ingroup    mIDrvIPP_Graph
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#if defined __UITRON || defined __ECOS

//#include "limits.h"
//#include "interrupt.h"
//#include "top.h"
//#include "dma.h"
//#include "nvtDrvProtected.h"
#include "grph.h"

#else

//#include <mach/rcw_macro.h>
//#include "uitron_wrapper/type.h"//a header for basic variable type
//#include "uitron_wrapper/semaphore.h"
//#include "uitron_wrapper/flag.h"
#include "grph_compatible.h"


#endif

#if defined(__FREERTOS)
#define EXPORT_SYMBOL(x)
#endif

//#include "kdrv_gfx2d/kdrv_grph.h"
//#include "grph.h"
//#include "grph_protected.h"
#include "graphic_reg.h"
#include "grph_int.h"

//
//  Graphic Internal APIs
//

static volatile GRPH_ENGINE_STATUS  v_engine_status[] = {GRPH_ENGINE_IDLE, GRPH_ENGINE_IDLE};
static          UINT32              v_lock_status[]   = {0, 0};

#if !(defined __UITRON || defined __ECOS)
//static ID vGraphFlgId[] = {FLG_ID_GRAPHIC, FLG_ID_GRAPHIC2};
#else
//static const ID vGraphFlgId[] = {FLG_ID_GRAPHIC, FLG_ID_GRAPHIC2};
//static const DRV_SEM vGraphSem[] = {SEMID_GRAPHIC, SEMID_GRAPHIC2};
//static const CG_EN vGraphClkEn[] = {GRAPH_CLK, GRAPH2_CLK};
#endif
static const FLGPTN v_isr_flag[] = {FLGPTN_GRAPHIC, FLGPTN_GRAPHIC2};
//static const DRV_SEM vGraphSem[] = {SEMID_GRAPHIC, SEMID_GRAPHIC2};
//static const CG_EN vGraphClkEn[] = {GRAPH_CLK, GRAPH2_CLK};
//static const DRV_INT_NUM vGraphIntEn[] = {DRV_INT_GRAPHIC, DRV_INT_GRAPHIC2};
//static UINT32 vGraphClkSel[] = {PLL_CLKSEL_GRAPHIC_80, PLL_CLKSEL_GRAPHIC2_80};
static UINT32 v_grph_freq[] = {480, 480};

//static KDRV_CALLBACK_FUNC v_grph_callback[GRPH_ID_2+1] = {0};

static PGRPH_IMG vp_image_a[] = {NULL, NULL};
static PGRPH_IMG vp_image_b[] = {NULL, NULL};
static PGRPH_IMG vp_image_c[] = {NULL, NULL};

// Default comparative for video cover
static GRPH_VDOCOV_COMPARATIVE g_graph_comparative = GRPH_VDOCOV_COMPARATIVE_GELT;

// memory buffer to store link list descriptors
//static _ALIGNED(32) UINT8 grph_ll_buffer[GRPH_ID_2 + 1][GRPH_LL_BUF_SIZE];
// memory buffer to temporarily store register setting before write to LL buffer
//static UINT32 reg_temp_buffer[GRPH_ID_2+1][0x340/sizeof(UINT32)];
static GRPH_REG_CACHE reg_cache[GRPH_ID_2 + 1][0x340 / sizeof(UINT32)];
static UINT8 *p_grph_ll_buffer[GRPH_ID_2 + 1];
static UINT64 *p_curr_linklist[GRPH_ID_2 + 1];
static UINT32 ll_req_index[GRPH_ID_2 + 1];

// memory buffer to store ACC dma path result
//static _ALIGNED(32) UINT8 grph_acc_buffer[GRPH_LL_MAX_COUNT][32];   // only GRPH_ID_1 support ACC

static GRPH_REG_DOMAIN grph_reg_domain[] = {GRPH_REG_DOMAIN_LL, GRPH_REG_DOMAIN_LL};

// ISR bottom handler for video cover
//static VCOV_ISR_CONTEXT v_vcov_context[GRPH_ID_2+1];

#if (_EMULATION_ == ENABLE)
static BOOL b_lineparam_directpath = FALSE;
static UINT32 vcov_line0reg1;
static UINT32 vcov_line0reg2;
static UINT32 vcov_line1reg1;
static UINT32 vcov_line1reg2;
static UINT32 vcov_line2reg1;
static UINT32 vcov_line2reg2;
static UINT32 vcov_line3reg1;
static UINT32 vcov_line3reg2;
#endif
static ER graph_request_gop(GRPH_ID id, PGRPH_REQUEST p_request);
extern void graph_isr(void);
extern void graph2_isr(void);


static REGVALUE graph_get_reg(GRPH_ID id, UINT32 offset);
static ER request_post_handler(GRPH_ID id, PGRPH_REQUEST p_request);

/**
    @addtogroup mIDrvIPP_Graph
*/
//@{

static void ll_update(T_GE_LL_UPDATE *p_update, UINT32 ofs, UINT32 value)
{
	p_update->bit.cmd = GRPH_LL_CMD_UPDATE;
	p_update->bit.byte_en = 0xF;
	p_update->bit.reg_ofs = ofs;
	p_update->bit.reg_val = value;
}

static void ll_null(T_GE_LL_NULL *p_null)
{
	p_null->bit.cmd = GRPH_LL_CMD_NULL;
}

static void ll_next_job(T_GE_LL_NEXT_JOB *p_next_job, UINT32 phy_addr)
{
	p_next_job->bit.cmd = GRPH_LL_CMD_NEXT_JOB;
	p_next_job->bit.next_job_addr = phy_addr;
}

/*
    Reset register pointer (before each graph_request())

    @param[in] id           graphic controller ID

    @return void
*/
static void graph_reset_reg_cache(GRPH_ID id)
{
	UINT32 i;
	GRPH_REG_DOMAIN bak_domain;
	const UINT32 reg_buf_size = sizeof(reg_cache[id]) / sizeof(reg_cache[id][0]);

	p_grph_ll_buffer[id] = &grph_ll_buffer[id][0];

	memset(&reg_cache[id][0], 0, sizeof(reg_cache[id]));

	bak_domain = grph_reg_domain[id];
	grph_reg_domain[id] = GRPH_REG_DOMAIN_APB;

	// fill sw cache with APB settings
	for (i = 0; i < reg_buf_size; i++) {
		reg_cache[id][i].value = graph_get_reg(id, i * 4);
	}

	grph_reg_domain[id] = bak_domain;

	DBG_IND("%s: reg buffer size %d\r\n", __func__,
			sizeof(reg_cache[id]) / sizeof(reg_cache[id][0]));

	DBG_IND("%s: acc buffer addr 0x%p, 0x%p\r\n",
		__func__,
		&grph_acc_buffer[0],
		&grph_acc_buffer[ACC_BUF_UNIT]);
}

/*
    Reset lint list buffer

    @param[in] id           graphic controller ID

    @note graph_init_ll() must be invoked before graph_reg2ll()

    @return void
*/
static void graph_init_ll(GRPH_ID id)
{
	p_curr_linklist[id] = (UINT64 *)p_grph_ll_buffer[id];
	ll_req_index[id] = 0;
}

/*
    Transfer registers of a single frame to ll buffer

    @param[in] id           graphic controller ID

    @return void
*/
static void graph_reg2ll(GRPH_ID id)
{
//    UINT64 *p_linklist = (UINT64*)p_grph_ll_buffer[id];
	UINT32 i;
	const UINT32 reg_buf_size = sizeof(reg_cache[id]) / sizeof(reg_cache[id][0]);

	for (i = 0; i < reg_buf_size; i++) {
		if (!(reg_cache[id][i].flag & GRPH_REG_FLG_DIRTY)) {
			continue;
		}

		ll_update((T_GE_LL_UPDATE *)p_curr_linklist[id], i * 4, reg_cache[id][i].value);
		reg_cache[id][i].flag &= ~GRPH_REG_FLG_DIRTY;
		p_curr_linklist[id]++;
	}
	ll_next_job((T_GE_LL_NEXT_JOB *)p_curr_linklist[id],
				grph_platform_va2pa((UINT32)(p_curr_linklist[id] + 1)));
	p_curr_linklist[id]++;
//    ll_null((T_GE_LL_NULL*)p_curr_linklist[id]);
//	dma_flushWriteCache((UINT32)p_grph_ll_buffer[id], sizeof(grph_ll_buffer[id]));
//    DBG_ERR("%s: ll base 0x%x, size 0x%x\r\n", __func__, p_grph_ll_buffer[id], sizeof(grph_ll_buffer[id]));
}

/*
    Finalize link list

    @param[in] id           graphic controller ID

    @note graph_ll_finalize() must be invoked after ALL graph_reg2ll()

    @return void
*/
static void graph_ll_finalize(GRPH_ID id)
{
	UINT32 update_size;

	p_curr_linklist[id]--;
	ll_null((T_GE_LL_NULL *)p_curr_linklist[id]);
	update_size = (UINT32)p_curr_linklist[id] - (UINT32)p_grph_ll_buffer[id] + sizeof(T_GE_LL_NULL);
	if (grph_platform_dma_flush_mem2dev((UINT32)p_grph_ll_buffer[id], update_size)) {
		DBG_WRN("%s: flush fail\r\n", __func__);
	}
}

/*
    Get graphic controller register. (from APB domain).

    @param[in] id           graphic controller ID
    @param[in] offset       register offset in graphic controller (word alignment)

    @return register value
*/
static REGVALUE apb_get_reg(GRPH_ID id, UINT32 offset)
{
	if (id == GRPH_ID_1) {
		return GRPH_GETREG(offset);
	} else {
		return GRPH2_GETREG(offset);
	}
}

/*
    Set graphic controller register. (to APB domain).

    @param[in] id           graphic controller ID
    @param[in] offset       register offset in graphic controller (word alignment)
    @param[in] value        register value

    @return void
*/
static void apb_set_reg(GRPH_ID id, UINT32 offset, REGVALUE value)
{
	if (id == GRPH_ID_1) {
		GRPH_SETREG(offset, value);
	} else {
		GRPH2_SETREG(offset, value);
	}
}

/*
    Get graphic controller register.

    @param[in] id           graphic controller ID
    @param[in] offset       register offset in graphic controller (word alignment)

    @return register value
*/
static REGVALUE graph_get_reg(GRPH_ID id, UINT32 offset)
{
	if (grph_reg_domain[id] == GRPH_REG_DOMAIN_APB) {
		return apb_get_reg(id, offset);
	} else {
		UINT32 word_idx = offset / sizeof(UINT32);

		if (offset >= sizeof(reg_cache[id]) / sizeof(reg_cache[id][0])) {
			return 0;
		}

		return reg_cache[id][word_idx].value;
	}
}

/*
    Set graphic controller register.

    @param[in] id           graphic controller ID
    @param[in] offset       register offset in graphic controller (word alignment)
    @param[in] value        register value

    @return void
*/
static void graph_set_reg(GRPH_ID id, UINT32 offset, REGVALUE value)
{
//	DBG_ERR("%s %d: grph %d, offset 0x%x, value 0x%x\r\n", __func__, grph_reg_domain[id],
//			id+1, offset, value);

	if (grph_reg_domain[id] == GRPH_REG_DOMAIN_APB) {
		apb_set_reg(id, offset, value);
	} else {
		UINT32 word_idx = offset / sizeof(UINT32);

//		DBG_ERR("%s: grph %d, offset 0x%x, value 0x%x\r\n", __func__,
//			id+1, offset, value);

		if (word_idx >= sizeof(reg_cache[id]) / sizeof(reg_cache[id][0])) {
			DBG_ERR("%s: offset out side 0x%x\r\n", __func__, sizeof(reg_cache[id]) / sizeof(reg_cache[id][0]));
			DBG_ERR("%s %d: grph %d, offset 0x%x, value 0x%x\r\n", __func__,
					(int)grph_reg_domain[id],
					(int)id + 1, (unsigned int)offset, (unsigned int)value);
			return;
		}

		reg_cache[id][word_idx].value = value;
		reg_cache[id][word_idx].flag |= GRPH_REG_FLG_DIRTY;
	}
}

/*
    Check pointer valiation

    @param[in] addr Checked pointer

    @return
	- @b TRUE   : validate pointer
	- @b FALSE  : invalidate pointer
*/
static BOOL graph_chk_ptr(UINT32 addr)
{
	if ((addr != 0) && (grph_platform_is_valid_va(addr) != 0)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

#if 0
static void trigger_wait_single_frame(GRPH_ID id)
{
	FLGPTN              uiFlag;
	T_GRPH_CTRL_REG     ctrl = {0};

	if (grph_reg_domain[id] == GRPH_REG_DOMAIN_APB) {
		ctrl.bit.TRIG_OP = TRUE;
		apb_set_reg(id, GRPH_CTRL_REG_OFS, ctrl.reg);

		wai_flg((PFLGPTN)&uiFlag, vGraphFlgId[id], vGraphFlag[id], TWF_ORW | TWF_CLR);
	}
}

static void trigger_wait_multi_frame(GRPH_ID id)
{
	FLGPTN              uiFlag;
	T_GRPH_CTRL_REG     ctrl = {0};

	DBG_IND("%s: enter\r\n", __func__);

	if (grph_reg_domain[id] != GRPH_REG_DOMAIN_APB) {
		// Only LL support multi frame
		ctrl.bit.LL_FIRE = TRUE;
		apb_set_reg(id, GRPH_CTRL_REG_OFS, ctrl.reg);

		DBG_IND("%s: trigger LL\r\n", __func__);

		wai_flg((PFLGPTN)&uiFlag, vGraphFlgId[id], vGraphFlag[id], TWF_ORW | TWF_CLR);
	}
}
#endif

static void trigger_multi_frame(GRPH_ID id)
{
//	FLGPTN              uiFlag;
	T_GRPH_CTRL_REG     ctrl = {0};

	DBG_IND("%s: enter\r\n", __func__);

	if (grph_reg_domain[id] != GRPH_REG_DOMAIN_APB) {
		// Only LL support multi frame
		ctrl.bit.LL_FIRE = TRUE;
		apb_set_reg(id, GRPH_CTRL_REG_OFS, ctrl.reg);

		DBG_IND("%s: trigger LL\r\n", __func__);
	}
}

/*
    Auto calculate start address and line offset for GOP
    This is internal API.

    @param[in] id           graphic controller identifier
    @param[in] precision    Operation precision 4/8/16bits
    @param[in] gop_mode     GRPH_GOP_MODE type. Select GOP operation mode

    @return void
*/
#if 1
static void graph_autoCalcGOP(GRPH_ID id, UINT32 uiBitPrc, GRPH_CMD gopMode)
{
	T_GRPH_LCNT_REG RegHeight;

	RegHeight.reg = graph_get_reg(id, GRPH_LCNT_REG_OFS);


	if ((gopMode == GRPH_CMD_ROT_90) ||
		(gopMode == GRPH_CMD_HRZ_FLIP_ROT_90)) {
		UINT32 offset;          //Block Height
		T_GRPH_IMG1_LOFF_REG    RegImg1Loff;
		T_GRPH_IMG1_ADDR_REG    RegImg1Addr;

		// Get block height
		switch (uiBitPrc) {
		case 1:
			offset = 16;
			break;
		case 8:
			offset = 8;
			break;
		case 16:
			offset = 8;
			break;
		case 32:
		default:
			offset = 8;
			break;
		}

		RegImg1Loff.reg = graph_get_reg(id, GRPH_IMG1_LOFF_REG_OFS);
		RegImg1Addr.reg = graph_get_reg(id, GRPH_IMG1_ADDR_REG_OFS);


		RegImg1Addr.bit.IMG1_ADDR += RegImg1Loff.bit.IMG1_LOFF * (RegHeight.bit.ACT_HEIGHT - offset);
		graph_set_reg(id, GRPH_IMG1_ADDR_REG_OFS, RegImg1Addr.reg);
	} else if ((gopMode == GRPH_CMD_ROT_180) || (gopMode == GRPH_CMD_VTC_FLIP)) {
		T_GRPH_IMG3_LOFF_REG    RegImg3Loff;
		T_GRPH_IMG3_ADDR_REG    RegImg3Addr;

		RegImg3Loff.reg = graph_get_reg(id, GRPH_IMG3_LOFF_REG_OFS);
		RegImg3Addr.reg = graph_get_reg(id, GRPH_IMG3_ADDR_REG_OFS);

		// { X_l_bottom, Y_l_bottom}
		RegImg3Addr.bit.IMG3_ADDR += RegImg3Loff.bit.IMG3_LOFF * (RegHeight.bit.ACT_HEIGHT - 1);
		graph_set_reg(id, GRPH_IMG3_ADDR_REG_OFS, RegImg3Addr.reg);
	}
}
#endif

/*
    Graphic image list handler

    @param[in] id           graphic controller identifier
    @param[in] p_images     list of images

    @return
	- @b E_OK: Done with no errors.
*/
static ER graph_handle_images(GRPH_ID id, PGRPH_REQUEST p_request, PGRPH_IMG p_images)
{
	UINT32 img_counts = 3;
	T_GRPH_IN1_REG in1reg = {0};
	T_GRPH_IN1_2_REG in1_2reg = {0};
	T_GRPH_IN2_REG in2reg = {0};
	T_GRPH_IN2_2_REG in2_2reg = {0};
	T_GRPH_OUT3_REG out3reg = {0};
	T_GRPH_OUT3_2_REG out3_2reg = {0};

	while (graph_chk_ptr((UINT32)p_images) && img_counts) {
		UINT32 op_counts;   // IN/OUT OP remaining counts
//		struct GRPH_INOUTOP *p_inout;
		PGRPH_INOUTOP p_inout;
		PGRPH_INOUTOP p_inout_u = NULL;
		PGRPH_INOUTOP p_inout_v = NULL;

		op_counts = 2;
		p_inout = p_images->p_inoutop;
		while (graph_chk_ptr((UINT32)p_inout) && op_counts) {
			switch (p_inout->id) {
			case GRPH_INOUT_ID_IN_A:
			case GRPH_INOUT_ID_IN_A_U:
			case GRPH_INOUT_ID_IN_B:
			case GRPH_INOUT_ID_IN_B_U:
			case GRPH_INOUT_ID_OUT_C:
			case GRPH_INOUT_ID_OUT_C_U:
				p_inout_u = p_inout;
				break;
			case GRPH_INOUT_ID_IN_A_V:
			case GRPH_INOUT_ID_IN_B_V:
			case GRPH_INOUT_ID_OUT_C_V:
				p_inout_v = p_inout;
				break;
			default:
				DBG_ERR("invalid IN operation ID 0x%x\r\n", (unsigned int)p_inout->id);
				return E_NOSPT;
			}

			op_counts--;
			p_inout = p_inout->p_next;
		}

		switch (p_images->img_id) {
		case GRPH_IMG_ID_A: {
				T_GRPH_IMG1_ADDR_REG addr_reg = {0};
				T_GRPH_IMG1_LOFF_REG lineoffset_reg = {0};
				T_GRPH_LCNT_REG height_reg = {0};
				T_GRPH_XRGN_REG width_reg = {0};

				vp_image_a[id] = p_images;
				if (p_request->is_buf_pa) {
					addr_reg.bit.IMG1_ADDR = p_images->dram_addr;
				} else {
					addr_reg.bit.IMG1_ADDR = grph_platform_va2pa(p_images->dram_addr);
				}
				graph_set_reg(id, GRPH_IMG1_ADDR_REG_OFS, addr_reg.reg);

				if (p_images->lineoffset & 0x03) {
					DBG_ERR("grph%d: img A lineoffset 0x%x should be word alignment\r\n",
						(int)id, (unsigned int)p_images->lineoffset);
					return E_NOSPT;
				}
				lineoffset_reg.bit.IMG1_LOFF = p_images->lineoffset;
				graph_set_reg(id, GRPH_IMG1_LOFF_REG_OFS, lineoffset_reg.reg);

				height_reg.bit.ACT_HEIGHT = p_images->height;
				graph_set_reg(id, GRPH_LCNT_REG_OFS, height_reg.reg);

				width_reg.bit.ACT_WIDTH = p_images->width;
				graph_set_reg(id, GRPH_XRGN_REG_OFS, width_reg.reg);

				if (graph_chk_ptr((UINT32)p_inout_u) &&
					((p_inout_u->id == GRPH_INOUT_ID_IN_A) || (p_inout_u->id == GRPH_INOUT_ID_IN_A_U))) {

					in1reg.bit.IN1_OP = p_inout_u->op;
					in1reg.bit.INSHF_1 = p_inout_u->shifts;
					in1reg.bit.INCONST_1 = p_inout_u->constant;
				}

				if ((p_inout_v != NULL) &&
					graph_chk_ptr((UINT32)p_inout_v) && (p_inout_v->id == GRPH_INOUT_ID_IN_A_V)) {

					in1_2reg.bit.IN1_OP = p_inout_v->op;
					in1_2reg.bit.INSHF_1 = p_inout_v->shifts;
					in1_2reg.bit.INCONST_1 = p_inout_v->constant;
				}
			}
			break;
		case GRPH_IMG_ID_B: {
				T_GRPH_IMG2_ADDR_REG addr_reg = {0};
				T_GRPH_IMG2_LOFF_REG lineoffset_reg = {0};

				vp_image_b[id] = p_images;
				if (p_request->is_buf_pa) {
					addr_reg.bit.IMG2_ADDR = p_images->dram_addr;
				} else {
					addr_reg.bit.IMG2_ADDR = grph_platform_va2pa(p_images->dram_addr);
				}
				graph_set_reg(id, GRPH_IMG2_ADDR_REG_OFS, addr_reg.reg);

				if (p_images->lineoffset & 0x03) {
					DBG_ERR("grph%d: img B lineoffset 0x%x should be word alignment\r\n",
						(int)id, (unsigned int)p_images->lineoffset);
					return E_NOSPT;
				}
				lineoffset_reg.bit.IMG2_LOFF = p_images->lineoffset;
				graph_set_reg(id, GRPH_IMG2_LOFF_REG_OFS, lineoffset_reg.reg);

				if (graph_chk_ptr((UINT32)p_inout_u) &&
					((p_inout_u->id == GRPH_INOUT_ID_IN_B) || (p_inout_u->id == GRPH_INOUT_ID_IN_B_U))) {

					in2reg.bit.IN2_OP = p_inout_u->op;
					in2reg.bit.INSHF_2 = p_inout_u->shifts;
					in2reg.bit.INCONST_2 = p_inout_u->constant;
				}

				if ((p_inout_v != NULL)
					&& graph_chk_ptr((UINT32)p_inout_v) && (p_inout_v->id == GRPH_INOUT_ID_IN_B_V)) {

					in2_2reg.bit.IN2_OP = p_inout_v->op;
					in2_2reg.bit.INSHF_2 = p_inout_v->shifts;
					in2_2reg.bit.INCONST_2 = p_inout_v->constant;
				}
			}
			break;
		case GRPH_IMG_ID_C: {
				T_GRPH_IMG3_ADDR_REG addr_reg = {0};
				T_GRPH_IMG3_LOFF_REG lineoffset_reg = {0};

				vp_image_c[id] = p_images;
				if (p_request->is_buf_pa) {
					addr_reg.bit.IMG3_ADDR = p_images->dram_addr;
				} else {
					addr_reg.bit.IMG3_ADDR = grph_platform_va2pa(p_images->dram_addr);
				}
				graph_set_reg(id, GRPH_IMG3_ADDR_REG_OFS, addr_reg.reg);

				if (p_images->lineoffset & 0x03) {
					DBG_ERR("grph%d: img C lineoffset 0x%x should be word alignment\r\n",
						(int)id, (unsigned int)p_images->lineoffset);
					return E_NOSPT;
				}
				lineoffset_reg.bit.IMG3_LOFF = p_images->lineoffset;
				graph_set_reg(id, GRPH_IMG3_LOFF_REG_OFS, lineoffset_reg.reg);

				if (graph_chk_ptr((UINT32)p_inout_u) &&
					((p_inout_u->id == GRPH_INOUT_ID_OUT_C) || (p_inout_u->id == GRPH_INOUT_ID_OUT_C_U))) {

					out3reg.bit.OUT_OP = p_inout_u->op;
					out3reg.bit.OUTSHF = p_inout_u->shifts;
					out3reg.bit.OUTCONST = p_inout_u->constant;
				}

				if ((p_inout_v != NULL) && graph_chk_ptr((UINT32)p_inout_v) && (p_inout_v->id == GRPH_INOUT_ID_OUT_C_V)) {

					out3_2reg.bit.OUT_OP = p_inout_v->op;
					out3_2reg.bit.OUTSHF = p_inout_v->shifts;
					out3_2reg.bit.OUTCONST = p_inout_v->constant;
				}
			}
			break;
		default:
			DBG_ERR("invalid image ID 0x%x\r\n", (unsigned int)p_images->img_id);
			return E_NOSPT;
		}

		img_counts--;
		p_images = p_images->p_next;
	}

	graph_set_reg(id, GRPH_IN1_REG_OFS, in1reg.reg);
	graph_set_reg(id, GRPH_IN1_2_REG_OFS, in1_2reg.reg);
	graph_set_reg(id, GRPH_IN2_REG_OFS, in2reg.reg);
	graph_set_reg(id, GRPH_IN2_2_REG_OFS, in2_2reg.reg);
	graph_set_reg(id, GRPH_OUT3_REG_OFS, out3reg.reg);
	graph_set_reg(id, GRPH_OUT3_2_REG_OFS, out3_2reg.reg);

	return E_OK;
}

static ER graph_postHandleImageList(GRPH_ID id, PGRPH_IMG p_images)
{
	UINT32 uiTraveImage = 3;

	while (graph_chk_ptr((UINT32)p_images) && uiTraveImage) {

		switch (p_images->img_id) {
		case GRPH_IMG_ID_A:
			vp_image_a[id] = p_images;
			break;
		case GRPH_IMG_ID_B:
			vp_image_b[id] = p_images;
			break;
		case GRPH_IMG_ID_C:
			vp_image_c[id] = p_images;
			break;
		default:
			DBG_ERR("invalid image ID 0x%x\r\n", (unsigned int)p_images->img_id);
			return E_NOSPT;
		}

		uiTraveImage--;
		p_images = p_images->p_next;
	}


	return E_OK;
}

/*
    Fetch property value from list

    @param[in] p_propty         Input property list
    @param[in] id               Property ID to be found from pProperties
    @param[out] p_found         Pointer to found property (NULL if NOT found)

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: NOT found
*/
static ER graph_fetch_property(PGRPH_PROPERTY p_propty, GRPH_PROPERTY_ID id,
							   PGRPH_PROPERTY *p_found)
{
	UINT32 depth = GRPH_PRPTY_MAX_CNT;

	while (graph_chk_ptr((UINT32)p_propty) && depth) {
		if (p_propty->id == id) {
			*p_found = p_propty;
			return E_OK;
		}

		depth--;
		p_propty = p_propty->p_next;
	}

	return E_NOSPT;
}

// caculate center point between point queue
static void GET_CENTER_POINT(struct struct_point *p_point,
				struct struct_point *p_center)
{
	INT32 count;
	struct struct_point *p_queue = p_point;

	if (p_center == NULL) {
		return;
	}
	if (p_point == NULL) {
		return;
	}

	count = 0;
	p_center->x = 0;
	p_center->y = 0;
	while (1) {
		count++;

		p_center->x += p_queue->x;
		p_center->y += p_queue->y;

		if (p_queue->p_next == p_point) {
			break;
		}

		p_queue = p_queue->p_next;
	}

	p_center->x = (p_center->x + (count / 2)) / count;
	p_center->y = (p_center->y + (count / 2)) / count;

	return;
}

static void GENERATE_LINE_PARAMS(struct struct_point *p_src,
				 struct struct_point *p_dst,
				 struct struct_point *p_point,
				 INT32 *p_param_a, INT32 *p_param_b,
				 INT32 *p_param_c,
				 UINT32 *p_compare)
{
	*p_param_a = p_dst->y - p_src->y;
	*p_param_b = p_src->x - p_dst->x;
	*p_param_c = p_src->x * p_dst->y - p_dst->x * p_src->y;

	// Force param C >= 0
	if (*p_param_c < 0) {
		*p_param_a *= -1;
		*p_param_b *= -1;
		*p_param_c *= -1;
	}

//	DBG_IND("comparative 0x%x\r\n", g_graph_comparative);
	switch (g_graph_comparative) {
	case GRPH_VDOCOV_COMPARATIVE_GTLT:
		if (((*p_param_a)*p_point->x + (*p_param_b)*p_point->y) > (*p_param_c)) {
			*p_compare = 2;
		} else {
			*p_compare = 3;
		}
		break;
	case GRPH_VDOCOV_COMPARATIVE_GTLE:
		if (((*p_param_a)*p_point->x + (*p_param_b)*p_point->y) > (*p_param_c)) {
			*p_compare = 2;
		} else {
			*p_compare = 1;
		}
		break;
	case GRPH_VDOCOV_COMPARATIVE_GELT:
		if (((*p_param_a)*p_point->x + (*p_param_b)*p_point->y) >= (*p_param_c)) {
			*p_compare = 0;
		} else {
			*p_compare = 3;
#if (_EMULATION_ == DISABLE)
			if (*p_param_c == 0) {
				*p_compare = 1;
			}
#endif
		}
		break;
	default:
	case GRPH_VDOCOV_COMPARATIVE_GELE:
		if (((*p_param_a)*p_point->x + (*p_param_b)*p_point->y) >= (*p_param_c)) {
			*p_compare = 0;
		} else {
			*p_compare = 1;
		}
		break;
	}
//	DBG_IND("Result 0x%x\r\n", *p_compare);

}

static UINT32 invert_vcov_comparative(UINT32 input)
{
	switch (input) {
	case 0:         // >=
		return 1;
	case 1:         // <=
		return 0;
	case 2:         // >
		return 3;
	case 3:         // <
	default :
		return 2;
	}
}

static INT32 graph_get_region_width(STRUCT_POINT *p_points, INT32 *p_min_x)
{
	INT32 min_x = LONG_MAX;
	INT32 max_x = LONG_MIN;
	STRUCT_POINT *p_start;

	p_start = p_points;

	while (1) {
		if (p_points->x > max_x) {
			max_x = p_points->x;
		}
		if (p_points->x < min_x) {
			min_x = p_points->x;
		}

		if (p_points->p_next == p_start) {
			break;
		}
		p_points = p_points->p_next;
	}

	*p_min_x = min_x;

	return max_x - min_x + 1;
}

static ER adjust_vcov_origin(GRPH_ID id, GRPH_QUAD_TYPE quad_type,
				INT32 origin_offset)
{
	// line0: top_left <--> top_right
	INT32               line0_a, line0_c;
	// line1: top_right <--> buttom_right
	INT32               line1_a, line1_c;
	// line2: bottom_right <--> bottom_left
	INT32               line2_a, line2_c;
	// line3: bottom_left <--> top_left
	INT32               line3_a, line3_c;
	const UINT32 LINE_REG_BASE_ADDR = (quad_type == GRPH_QUAD_TYPE_NORMAL) ?
						  GRPH_GOP8_LINE0_REG1_OFS :
						  GRPH_GOP8_INNER_LINE0_REG1_OFS;
	T_GRPH_GOP8_LINE_REG1 line0reg1 = {0};
	T_GRPH_GOP8_LINE_REG1 line1reg1 = {0};
	T_GRPH_GOP8_LINE_REG1 line2reg1 = {0};
	T_GRPH_GOP8_LINE_REG1 line3reg1 = {0};
	T_GRPH_GOP8_LINE_REG2 line0reg2 = {0};
	T_GRPH_GOP8_LINE_REG2 line1reg2 = {0};
	T_GRPH_GOP8_LINE_REG2 line2reg2 = {0};
	T_GRPH_GOP8_LINE_REG2 line3reg2 = {0};

	if (origin_offset == 0) {
		return E_OK;
	}

	// If origin point is horizontaly shifted by offset
	// a*(x-offset) + b*y = c - a*offset

	line0reg1.reg = graph_get_reg(id, LINE_REG_BASE_ADDR);
	line0reg2.reg = graph_get_reg(id, LINE_REG_BASE_ADDR + 0x04);
	line1reg1.reg = graph_get_reg(id, LINE_REG_BASE_ADDR + 0x08);
	line1reg2.reg = graph_get_reg(id, LINE_REG_BASE_ADDR + 0x0C);
	line2reg1.reg = graph_get_reg(id, LINE_REG_BASE_ADDR + 0x10);
	line2reg2.reg = graph_get_reg(id, LINE_REG_BASE_ADDR + 0x14);
	line3reg1.reg = graph_get_reg(id, LINE_REG_BASE_ADDR + 0x18);
	line3reg2.reg = graph_get_reg(id, LINE_REG_BASE_ADDR + 0x1C);

	line0_a = line0reg1.bit.A_PARAM;
	if (line0reg1.bit.A_PARAM_SIGN_BIT) {
		line0_a *= -1;
	}
	line0_c = line0reg2.bit.C_PARAM;

	line1_a = line1reg1.bit.A_PARAM;
	if (line1reg1.bit.A_PARAM_SIGN_BIT) {
		line1_a *= -1;
	}
	line1_c = line1reg2.bit.C_PARAM;

	line2_a = line2reg1.bit.A_PARAM;
	if (line2reg1.bit.A_PARAM_SIGN_BIT) {
		line2_a *= -1;
	}
	line2_c = line2reg2.bit.C_PARAM;

	line3_a = line3reg1.bit.A_PARAM;
	if (line3reg1.bit.A_PARAM_SIGN_BIT) {
		line3_a *= -1;
	}
	line3_c = line3reg2.bit.C_PARAM;

	line0_c -= line0_a * origin_offset;
	line1_c -= line1_a * origin_offset;
	line2_c -= line2_a * origin_offset;
	line3_c -= line3_a * origin_offset;

	if (line0_c < 0) {
		// no sign bit for C, thus invert sign bit of A, B
		line0reg1.bit.A_PARAM_SIGN_BIT = !line0reg1.bit.A_PARAM_SIGN_BIT;
		line0reg1.bit.B_PARAM_SIGN_BIT = !line0reg1.bit.B_PARAM_SIGN_BIT;

		line0reg2.bit.COMPARE = invert_vcov_comparative(line0reg2.bit.COMPARE);
	}
	line0reg2.bit.C_PARAM = abs(line0_c);

	if (line1_c < 0) {
		// no sign bit for C, thus invert sign bit of A, B
		line1reg1.bit.A_PARAM_SIGN_BIT = !line1reg1.bit.A_PARAM_SIGN_BIT;
		line1reg1.bit.B_PARAM_SIGN_BIT = !line1reg1.bit.B_PARAM_SIGN_BIT;

		line1reg2.bit.COMPARE = invert_vcov_comparative(line1reg2.bit.COMPARE);
	}
	line1reg2.bit.C_PARAM = abs(line1_c);

	if (line2_c < 0) {
		// no sign bit for C, thus invert sign bit of A, B
		line2reg1.bit.A_PARAM_SIGN_BIT = !line2reg1.bit.A_PARAM_SIGN_BIT;
		line2reg1.bit.B_PARAM_SIGN_BIT = !line2reg1.bit.B_PARAM_SIGN_BIT;

		line2reg2.bit.COMPARE = invert_vcov_comparative(line2reg2.bit.COMPARE);
	}
	line2reg2.bit.C_PARAM = abs(line2_c);

	if (line3_c < 0) {
		// no sign bit for C, thus invert sign bit of A, B
		line3reg1.bit.A_PARAM_SIGN_BIT = !line3reg1.bit.A_PARAM_SIGN_BIT;
		line3reg1.bit.B_PARAM_SIGN_BIT = !line3reg1.bit.B_PARAM_SIGN_BIT;

		line3reg2.bit.COMPARE = invert_vcov_comparative(line3reg2.bit.COMPARE);
	}
	line3reg2.bit.C_PARAM = abs(line3_c);

	graph_set_reg(id, LINE_REG_BASE_ADDR, line0reg1.reg);
	graph_set_reg(id, LINE_REG_BASE_ADDR + 0x04, line0reg2.reg);
	graph_set_reg(id, LINE_REG_BASE_ADDR + 0x08, line1reg1.reg);
	graph_set_reg(id, LINE_REG_BASE_ADDR + 0x0C, line1reg2.reg);
	graph_set_reg(id, LINE_REG_BASE_ADDR + 0x10, line2reg1.reg);
	graph_set_reg(id, LINE_REG_BASE_ADDR + 0x14, line2reg2.reg);
	graph_set_reg(id, LINE_REG_BASE_ADDR + 0x18, line3reg1.reg);
	graph_set_reg(id, LINE_REG_BASE_ADDR + 0x1C, line3reg2.reg);

	return E_OK;
}

/*
    Graphic GOP request for video cover

    @param[in] id               graphic controller identifier
    @param[in] p_request        Description of request
    @param[in] b_righ_half      inform to draw right half part of a region width > 2048
				- @b FALSE: normal
				- @b TRUE: draw right half

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER request_gop_vcov(GRPH_ID id, PGRPH_REQUEST p_request, BOOL b_righ_half,
			UINT32 x_coord_offset)
{
	UINT32              is_411 = 0;
	UINT32              width = 0, height = 0, mosaic_en = 0;
	UINT32              filled_data = 0;
	// line0: top_left <--> top_right
	INT32               line0_a, line0_b, line0_c;
	UINT32              line0_inequality;
	// line1: top_right <--> buttom_right
	INT32               line1_a, line1_b, line1_c;
	UINT32              line1_inequality;
	// line2: bottom_right <--> bottom_left
	INT32               line2_a, line2_b, line2_c;
	UINT32              line2_inequality;
	// line3: bottom_left <--> top_left
	INT32               line3_a, line3_b, line3_c;
	UINT32              line3_inequality;
	INT32               region_width, min_x;
	STRUCT_POINT        center_p = {0};
	STRUCT_POINT        points[4];
	STRUCT_POINT        *p_topleft, *p_topright;
	STRUCT_POINT        *p_bottomleft, *p_bottomright;
	PGRPH_PROPERTY      p_property = 0;
	PGRPH_QUAD_DESC     p_quad_desc;
	PGRPH_QUAD_DESC     p_inner_quad = NULL;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_CONST_REG2   const_reg2 = {0};
	T_GRPH_GOP8_LINE_REG1 line0reg1 = {0};
	T_GRPH_GOP8_LINE_REG1 line1reg1 = {0};
	T_GRPH_GOP8_LINE_REG1 line2reg1 = {0};
	T_GRPH_GOP8_LINE_REG1 line3reg1 = {0};
	T_GRPH_GOP8_LINE_REG2 line0reg2 = {0};
	T_GRPH_GOP8_LINE_REG2 line1reg2 = {0};
	T_GRPH_GOP8_LINE_REG2 line2reg2 = {0};
	T_GRPH_GOP8_LINE_REG2 line3reg2 = {0};

	// Find YUV format information
	if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_YUVFMT,
				&p_property) != E_OK) {
		DBG_ERR("GRPH_PROPERTY_ID_YUVFMT property not found\r\n");
		return E_NOSPT;
	}
	if (p_property->property == GRPH_YUV_411) {
		is_411 = 1;
	}

	// Find quarilateral information
	if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_QUAD_PTR,
				&p_property) != E_OK) {
		DBG_ERR("GRPH_PROPERTY_ID_QUAD_PTR property not found\r\n");
		return E_NOSPT;
	}
	if (graph_chk_ptr(p_property->property) == FALSE) {
		DBG_ERR("Input GRPH_QUAD_DESC pointer is not valid 0x%x\r\n",
			(unsigned int)p_property->property);
		return E_NOSPT;
	}
	p_quad_desc = (GRPH_QUAD_DESC *)p_property->property;


	if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_QUAD_INNER_PTR,
				&p_property) == E_OK) {
		if (graph_chk_ptr(p_property->property) == FALSE) {
			DBG_ERR("Input GRPH_PROPERTY_ID_QUAD_INNER_PTR pointer is not valid 0x%x\r\n",
				(unsigned int)p_property->property);
			return E_NOSPT;
		}
		p_inner_quad = (GRPH_QUAD_DESC *)p_property->property;
	}

	switch (p_quad_desc->mosaic_width) {
	case 0:
		break;
	case 8:
	case 16:
	case 32:
	case 64:
	case 128:
		width = __builtin_ctz(p_quad_desc->mosaic_width) - 3;
		break;
	default:
		DBG_ERR("Mosaic width %d is not valid\r\n",
				(int)p_quad_desc->mosaic_width);
		return E_NOSPT;
	}

	switch (p_quad_desc->mosaic_height) {
	case 0:
		break;
	case 4:
	case 8:
	case 16:
	case 32:
	case 64:
		height = __builtin_ctz(p_quad_desc->mosaic_height) - 2;
		break;
	default:
		DBG_ERR("Mosaic height %d is not valid\r\n",
				(int)p_quad_desc->mosaic_height);
		return E_NOSPT;
	}

	if ((p_quad_desc->mosaic_width == 0) || (p_quad_desc->mosaic_height == 0)) {
		mosaic_en = 0;
	} else {
		mosaic_en = 1;
	}

	const_reg2.reg = p_quad_desc->alpha & 0xFF;

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
		if ((mosaic_en == 0) &&
			(graph_fetch_property(p_request->p_property,
					GRPH_PROPERTY_ID_NORMAL,
					&p_property) != E_OK)) {
			DBG_ERR("GRPH_PROPERTY_ID_NORMAL property not found\r\n");
			return E_NOSPT;
		}
		filled_data = p_property->property;

		const_reg.reg = (filled_data & 0xFF) |
						(width << 16) |
						(height << 20) |
						(p_quad_desc->blend_en << 24) |
						(is_411 << 26) |
						(mosaic_en << 31);
		break;
	case GRPH_FORMAT_16BITS:
		if ((mosaic_en == 0) &&
			(graph_fetch_property(p_request->p_property,
					GRPH_PROPERTY_ID_U, &p_property) != E_OK)) {
			DBG_ERR("GRPH_PROPERTY_ID_U property not found\r\n");
			return E_NOSPT;
		}
		filled_data = p_property->property & 0xFF;

		if ((mosaic_en == 0) &&
			(graph_fetch_property(p_request->p_property,
					GRPH_PROPERTY_ID_V, &p_property) != E_OK)) {
			DBG_ERR("GRPH_PROPERTY_ID_V property not found\r\n");
			return E_NOSPT;
		}
		filled_data |= (p_property->property & 0xFF) << 8;

		const_reg.reg = filled_data |
						(width << 16) |
						(height << 20) |
						(p_quad_desc->blend_en << 24) |
						(is_411 << 26) |
						(mosaic_en << 31);

		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	// If image B existed, we assume user need mosaic from image B
	if (graph_chk_ptr((UINT32)vp_image_b[id])) {
		ER ret = E_OK;

		if ((p_quad_desc->mosaic_width == 0) ||
			(p_quad_desc->mosaic_height == 0)) {
			DBG_ERR("Img B is assigned, but mosaic width is %d, height is %d\r\n",
					(int)p_quad_desc->mosaic_width,
					(int)p_quad_desc->mosaic_height);
			ret = E_NOSPT;
		} else {
			const UINT32 PIXEL_SIZE =
				(p_request->format == GRPH_FORMAT_8BITS) ? 1 : 2;
			const UINT32 IMGB_WIDTH =
				(vp_image_a[id]->width * PIXEL_SIZE + p_quad_desc->mosaic_width - 1) / p_quad_desc->mosaic_width;
			PGRPH_PROPERTY p_fmt_property = NULL;

			if (vp_image_b[id]->lineoffset < IMGB_WIDTH) {
				DBG_ERR("Img B lineoffset %d is too small, image A width %d, image B should be %d\r\n",
						(int)vp_image_b[id]->lineoffset, (int)vp_image_a[id]->width,
						(int)IMGB_WIDTH);
				ret = E_NOSPT;
			}
			if (graph_fetch_property(p_request->p_property,
						GRPH_PROPERTY_ID_MOSAIC_SRC_FMT,
						&p_fmt_property) == E_OK) {
				if (p_fmt_property->property ==
						GRPH_MOSAIC_FMT_YUV444) {
					const_reg.reg |= 1 << 27;
				}
			}
			const_reg.reg |= 1 << 25;
		}

		if (ret != E_OK) {
			return ret;
		}
	}

	// Calculate line equations
	p_topleft = &points[0];
	p_topright = &points[1];
	p_bottomright = &points[2];
	p_bottomleft = &points[3];

	p_topleft->x = p_quad_desc->top_left.x - (INT32)x_coord_offset;
	p_topleft->y = p_quad_desc->top_left.y;
	p_topright->x = p_quad_desc->top_right.x - (INT32)x_coord_offset;
	p_topright->y = p_quad_desc->top_right.y;
	p_bottomright->x = p_quad_desc->bottom_right.x - (INT32)x_coord_offset;
	p_bottomright->y = p_quad_desc->bottom_right.y;
	p_bottomleft->x = p_quad_desc->bottom_left.x - (INT32)x_coord_offset;
	p_bottomleft->y = p_quad_desc->bottom_left.y;

#if 0
	DBG_IND("Top Left %d, %d\r\n", (int)p_topleft->x, (int)p_topleft->y);
	DBG_IND("Top Right %d, %d\r\n", (int)p_topright->x, (int)p_topright->y);
	DBG_IND("Bottom Right %d, %d\r\n", (int)p_bottomright->x, (int)p_bottomright->y);
	DBG_IND("Bottom Left %d, %d\r\n", (int)p_bottomleft->x, (int)p_bottomleft->y);
#endif

	p_topleft->p_clockwise = p_topleft->p_next = p_topright;
	p_topleft->p_counterclock = p_topleft->p_prev = p_bottomleft;
	p_topright->p_clockwise = p_topright->p_next = p_bottomright;
	p_topright->p_counterclock = p_topright->p_prev = p_topleft;
	p_bottomright->p_clockwise = p_bottomright->p_next = p_bottomleft;
	p_bottomright->p_counterclock = p_bottomright->p_prev = p_topright;
	p_bottomleft->p_clockwise = p_bottomleft->p_next = p_topleft;
	p_bottomleft->p_counterclock = p_bottomleft->p_prev = p_bottomright;

	region_width = graph_get_region_width(p_topleft, &min_x);
#if 0
	if (region_width > 2048) {
		DBG_WRN("quadrilateral width %d > 2048\r\n", (int)region_width);

		if (b_righ_half) {
			p_topleft->x = min_x + 2048;
			p_bottomleft->x = min_x + 2048;
		} else {
			p_topright->x = min_x + 2048;
			p_bottomright->x = min_x + 2048;
		}
	}
#endif

	GET_CENTER_POINT(p_topleft, &center_p);

	// line0 (p_topleft --> p_topright)
	GENERATE_LINE_PARAMS(p_topleft, p_topright, &center_p,
				 &line0_a, &line0_b, &line0_c, &line0_inequality);

	// line1 (p_topright --> p_bottomright)
	GENERATE_LINE_PARAMS(p_topright, p_bottomright, &center_p,
				 &line1_a, &line1_b, &line1_c, &line1_inequality);

	// line2 (p_bottomright --> p_bottomleft)
	GENERATE_LINE_PARAMS(p_bottomright, p_bottomleft, &center_p,
				 &line2_a, &line2_b, &line2_c, &line2_inequality);

	// line3 (p_bottomleft --> p_topleft)
	GENERATE_LINE_PARAMS(p_bottomleft, p_topleft, &center_p,
				 &line3_a, &line3_b, &line3_c, &line3_inequality);

	line0reg1.bit.A_PARAM = abs(line0_a);
	line0reg1.bit.A_PARAM_SIGN_BIT = (line0_a < 0) ? 1 : 0;
	line0reg1.bit.B_PARAM = abs(line0_b);
	line0reg1.bit.B_PARAM_SIGN_BIT = (line0_b < 0) ? 1 : 0;
	line0reg2.bit.C_PARAM = abs(line0_c);
	line0reg2.bit.COMPARE = line0_inequality;

	line1reg1.bit.A_PARAM = abs(line1_a);
	line1reg1.bit.A_PARAM_SIGN_BIT = (line1_a < 0) ? 1 : 0;
	line1reg1.bit.B_PARAM = abs(line1_b);
	line1reg1.bit.B_PARAM_SIGN_BIT = (line1_b < 0) ? 1 : 0;
	line1reg2.bit.C_PARAM = abs(line1_c);
	line1reg2.bit.COMPARE = line1_inequality;

	line2reg1.bit.A_PARAM = abs(line2_a);
	line2reg1.bit.A_PARAM_SIGN_BIT = (line2_a < 0) ? 1 : 0;
	line2reg1.bit.B_PARAM = abs(line2_b);
	line2reg1.bit.B_PARAM_SIGN_BIT = (line2_b < 0) ? 1 : 0;
	line2reg2.bit.C_PARAM = abs(line2_c);
	line2reg2.bit.COMPARE = line2_inequality;

	line3reg1.bit.A_PARAM = abs(line3_a);
	line3reg1.bit.A_PARAM_SIGN_BIT = (line3_a < 0) ? 1 : 0;
	line3reg1.bit.B_PARAM = abs(line3_b);
	line3reg1.bit.B_PARAM_SIGN_BIT = (line3_b < 0) ? 1 : 0;
	line3reg2.bit.C_PARAM = abs(line3_c);
	line3reg2.bit.COMPARE = line3_inequality;

#if (_EMULATION_ == ENABLE)
#if 0
	if (b_lineparam_directpath == TRUE) {
		line0reg1.reg = vcov_line0reg1;
		line0reg2.reg = vcov_line0reg2;
		line1reg1.reg = vcov_line1reg1;
		line1reg2.reg = vcov_line1reg2;
		line2reg1.reg = vcov_line2reg1;
		line2reg2.reg = vcov_line2reg2;
		line3reg1.reg = vcov_line3reg1;
		line3reg2.reg = vcov_line3reg2;
	}
#endif
#endif

	graph_set_reg(id, GRPH_GOP8_LINE0_REG1_OFS, line0reg1.reg);
	graph_set_reg(id, GRPH_GOP8_LINE0_REG2_OFS, line0reg2.reg);
	graph_set_reg(id, GRPH_GOP8_LINE1_REG1_OFS, line1reg1.reg);
	graph_set_reg(id, GRPH_GOP8_LINE1_REG2_OFS, line1reg2.reg);
	graph_set_reg(id, GRPH_GOP8_LINE2_REG1_OFS, line2reg1.reg);
	graph_set_reg(id, GRPH_GOP8_LINE2_REG2_OFS, line2reg2.reg);
	graph_set_reg(id, GRPH_GOP8_LINE3_REG1_OFS, line3reg1.reg);
	graph_set_reg(id, GRPH_GOP8_LINE3_REG2_OFS, line3reg2.reg);

	if (p_inner_quad) {
		p_topleft->x = p_inner_quad->top_left.x - (INT32)x_coord_offset;
		p_topleft->y = p_inner_quad->top_left.y;
		p_topright->x = p_inner_quad->top_right.x - (INT32)x_coord_offset;
		p_topright->y = p_inner_quad->top_right.y;
		p_bottomright->x = p_inner_quad->bottom_right.x -
					(INT32)x_coord_offset;
		p_bottomright->y = p_inner_quad->bottom_right.y;
		p_bottomleft->x = p_inner_quad->bottom_left.x - (INT32)x_coord_offset;
		p_bottomleft->y = p_inner_quad->bottom_left.y;

		DBG_IND("Inner Top Left %d, %d\r\n", (int)p_topleft->x, (int)p_topleft->y);
		DBG_IND("Inner Top Right %d, %d\r\n", (int)p_topright->x, (int)p_topright->y);
		DBG_IND("Inner Bottom Right %d, %d\r\n", (int)p_bottomright->x,
							(int)p_bottomright->y);
		DBG_IND("Inner Bottom Left %d, %d\r\n", (int)p_bottomleft->x,
							(int)p_bottomleft->y);

		p_topleft->p_clockwise = p_topleft->p_next = p_topright;
		p_topleft->p_counterclock = p_topleft->p_prev = p_bottomleft;
		p_topright->p_clockwise = p_topright->p_next = p_bottomright;
		p_topright->p_counterclock = p_topright->p_prev = p_topleft;
		p_bottomright->p_clockwise = p_bottomright->p_next = p_bottomleft;
		p_bottomright->p_counterclock = p_bottomright->p_prev = p_topright;
		p_bottomleft->p_clockwise = p_bottomleft->p_next = p_topleft;
		p_bottomleft->p_counterclock = p_bottomleft->p_prev = p_bottomright;

		region_width = graph_get_region_width(p_topleft, &min_x);

		GET_CENTER_POINT(p_topleft, &center_p);

		// line0 (pTopLeft --> pTopRight)
		GENERATE_LINE_PARAMS(p_topleft, p_topright, &center_p,
				 &line0_a, &line0_b, &line0_c, &line0_inequality);

		// line1 (pTopRight --> pBottomRight)
		GENERATE_LINE_PARAMS(p_topright, p_bottomright, &center_p,
				 &line1_a, &line1_b, &line1_c, &line1_inequality);

		// line2 (pBottomRight --> pBottomLeft)
		GENERATE_LINE_PARAMS(p_bottomright, p_bottomleft, &center_p,
				 &line2_a, &line2_b, &line2_c, &line2_inequality);

		// line3 (pBottomLeft --> pTopLeft)
		GENERATE_LINE_PARAMS(p_bottomleft, p_topleft, &center_p,
				 &line3_a, &line3_b, &line3_c, &line3_inequality);

		line0reg1.bit.A_PARAM = abs(line0_a);
		line0reg1.bit.A_PARAM_SIGN_BIT = (line0_a < 0) ? 1 : 0;
		line0reg1.bit.B_PARAM = abs(line0_b);
		line0reg1.bit.B_PARAM_SIGN_BIT = (line0_b < 0) ? 1 : 0;
		line0reg2.bit.C_PARAM = abs(line0_c);
		line0reg2.bit.COMPARE = line0_inequality;

		line1reg1.bit.A_PARAM = abs(line1_a);
		line1reg1.bit.A_PARAM_SIGN_BIT = (line1_a < 0) ? 1 : 0;
		line1reg1.bit.B_PARAM = abs(line1_b);
		line1reg1.bit.B_PARAM_SIGN_BIT = (line1_b < 0) ? 1 : 0;
		line1reg2.bit.C_PARAM = abs(line1_c);
		line1reg2.bit.COMPARE = line1_inequality;

		line2reg1.bit.A_PARAM = abs(line2_a);
		line2reg1.bit.A_PARAM_SIGN_BIT = (line2_a < 0) ? 1 : 0;
		line2reg1.bit.B_PARAM = abs(line2_b);
		line2reg1.bit.B_PARAM_SIGN_BIT = (line2_b < 0) ? 1 : 0;
		line2reg2.bit.C_PARAM = abs(line2_c);
		line2reg2.bit.COMPARE = line2_inequality;

		line3reg1.bit.A_PARAM = abs(line3_a);
		line3reg1.bit.A_PARAM_SIGN_BIT = (line3_a < 0) ? 1 : 0;
		line3reg1.bit.B_PARAM = abs(line3_b);
		line3reg1.bit.B_PARAM_SIGN_BIT = (line3_b < 0) ? 1 : 0;
		line3reg2.bit.C_PARAM = abs(line3_c);
		line3reg2.bit.COMPARE = line3_inequality;

		graph_set_reg(id, GRPH_GOP8_INNER_LINE0_REG1_OFS, line0reg1.reg);
		graph_set_reg(id, GRPH_GOP8_INNER_LINE0_REG2_OFS, line0reg2.reg);
		graph_set_reg(id, GRPH_GOP8_INNER_LINE1_REG1_OFS, line1reg1.reg);
		graph_set_reg(id, GRPH_GOP8_INNER_LINE1_REG2_OFS, line1reg2.reg);
		graph_set_reg(id, GRPH_GOP8_INNER_LINE2_REG1_OFS, line2reg1.reg);
		graph_set_reg(id, GRPH_GOP8_INNER_LINE2_REG2_OFS, line2reg2.reg);
		graph_set_reg(id, GRPH_GOP8_INNER_LINE3_REG1_OFS, line3reg1.reg);
		graph_set_reg(id, GRPH_GOP8_INNER_LINE3_REG2_OFS, line3reg2.reg);

		const_reg.reg |= 1 << 30;        // enable drawing hollow
	}

	graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
	graph_set_reg(id, GRPH_CONST_REG2_OFS, const_reg2.reg);

#if (_EMULATION_ == ENABLE)
	if (b_lineparam_directpath == TRUE) {
		return E_OK;
	}
#endif
	if (region_width > 2048) {
		return E_NOMEM;
	}

	return E_OK;
}

static ER graphic_wrap_vcov(GRPH_ID id, PGRPH_REQUEST p_request)
{
	UINT32 x_coord_ofs = 0;
	UINT32 slice_ofs = 0;
	ER                  ret = E_OK;
	UINT32              remain_width = vp_image_a[id]->width;
	PGRPH_PROPERTY      p_property = 0;
	PGRPH_QUAD_DESC     p_quad_desc;
	PGRPH_QUAD_DESC     p_inner_quad = NULL;
	GRPH_IMG img_c = {GRPH_IMG_ID_C, 0, 0, 0, 0, NULL, NULL};
	GRPH_IMG img_a = {GRPH_IMG_ID_A, 0, 0, 0, 0, NULL, &img_c};
	const UINT32 bak_imga_addr = vp_image_a[id]->dram_addr;
	const UINT32 bak_imga_loft = vp_image_a[id]->lineoffset;
	const UINT32 bak_imga_height = vp_image_a[id]->height;
	const UINT32 bak_imgc_addr = vp_image_c[id]->dram_addr;
	const UINT32 bak_imgc_loft = vp_image_c[id]->lineoffset;


	// Find quarilateral information
	if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_QUAD_PTR,
				&p_property) != E_OK) {
		DBG_ERR("GRPH_PROPERTY_ID_QUAD_PTR property not found\r\n");
		return E_NOSPT;
	}
	if (graph_chk_ptr(p_property->property) == FALSE) {
		DBG_ERR("Input GRPH_QUAD_DESC pointer is not valid 0x%x\r\n",
				(unsigned int)p_property->property);
		return E_NOSPT;
	}
	p_quad_desc = (GRPH_QUAD_DESC *)p_property->property;

	if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_QUAD_INNER_PTR,
				&p_property) == E_OK) {
		if (graph_chk_ptr(p_property->property) == FALSE) {
			DBG_ERR("Input GRPH_PROPERTY_ID_QUAD_INNER_PTR pointer is not valid 0x%x\r\n",
				(unsigned int)p_property->property);
			return E_NOSPT;
		}
		p_inner_quad = (GRPH_QUAD_DESC *)p_property->property;
	}

#if defined(_NVT_EMULATION_)
	if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_QUAD_COMP,
				&p_property) == E_OK) {
		g_graph_comparative = p_property->property;
		if (g_graph_comparative > GRPH_VDOCOV_COMPARATIVE_GELE) {
			g_graph_comparative = GRPH_VDOCOV_COMPARATIVE_GELE;
		}
		DBG_WRN("%s: modify comparative rule to  0x%x\r\n", __func__,
			(unsigned int)g_graph_comparative);
	}
#endif


	request_gop_vcov(id, p_request, FALSE, x_coord_ofs);

	// At most need 2 iteration: (when width > 2048) draw left half and right half
	while (remain_width) {
		// If draw mosaic
		// If no image B => mosaic from TOP/LEFT corner
		// If
		if (((p_quad_desc->mosaic_width != 0) ||
				(p_quad_desc->mosaic_height != 0)) &&
				(graph_chk_ptr((UINT32)vp_image_b[id]) == FALSE)) {
			UINT32 width_slice;

			// HW line buffer is 4k byte
			if (p_request->format == GRPH_FORMAT_8BITS) {
				// If 8 bit, hw limit width 4k pixels
				if (remain_width > 4096) {
					width_slice = (4096 /
						p_quad_desc->mosaic_width) *
						p_quad_desc->mosaic_width;
				} else {
					width_slice = remain_width;
				}
			} else {
				// if 16 bit, hw limit width 2k pixels
				if (remain_width > 2048) {
					width_slice = (2048 /
						p_quad_desc->mosaic_width) *
                                                p_quad_desc->mosaic_width;
					DBG_IND("Cut slice width to %d\r\n",
						(int)width_slice);
				} else {
					width_slice = remain_width;
				}
			}

			img_a.dram_addr = bak_imga_addr + x_coord_ofs;
			img_a.lineoffset = bak_imga_loft;
			img_a.width = width_slice;
			img_a.height = bak_imga_height;
			img_c.dram_addr = bak_imgc_addr + x_coord_ofs;
			img_c.lineoffset = bak_imgc_loft;
			img_a.p_next = &img_c;

			ret = graph_handle_images(id, p_request, &img_a);
			if (ret != E_OK) {
				DBG_ERR("invalid pointer in input images\r\n");
				return ret;
			}

			adjust_vcov_origin(id, GRPH_QUAD_TYPE_NORMAL,
					(INT32)slice_ofs);
			if (p_inner_quad) {
				adjust_vcov_origin(id, GRPH_QUAD_TYPE_INNER,
						(INT32)slice_ofs);
			}

			DBG_IND("Remain %d, slice %d\r\n", (int)remain_width, (int)width_slice);

			x_coord_ofs += width_slice;
			remain_width -= width_slice;
			slice_ofs = width_slice;

		} else {
			remain_width = 0;

		}

//		clr_flg(vGraphFlgId[id], vGraphFlag[id]);

		DBG_IND("%s: remain %d\r\n", __func__, (int)remain_width);
		graph_reg2ll(id);
        //HH: linux KDRV not support APB trigger
//		trigger_wait_single_frame(id);
	}

	return E_OK;
}

/*
    Graphic AOP request for minus/plus shift

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_mpshf(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = 0;
	T_GRPH_IN2_REG      in2reg = {0};
	T_GRPH_IN2_2_REG    in2_2reg = {0};
	T_GRPH_CONST_REG    const_reg = {0};

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
		// If no IN2_OP is enabled, graph will use IN2_OP to do SHF
		in2reg.reg = graph_get_reg(id, GRPH_IN2_REG_OFS);
		if (in2reg.reg == 0) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
				return E_OK;
			}
			in2reg.bit.IN2_OP = GRPH_INOP_SHIFTR_ADD;
			in2reg.bit.INSHF_2 = p_property->property;
			in2reg.bit.INCONST_2 = 0;
			graph_set_reg(id, GRPH_IN2_REG_OFS, in2reg.reg);
		}
		break;
	case GRPH_FORMAT_16BITS:
		if (vp_image_a[id]->dram_addr & 0x1) {
			DBG_ERR("AOP%d image A address 0x%x should be 2 byte align\r\n",
				(unsigned int)p_request->command, (unsigned int)vp_image_a[id]->dram_addr);
			return E_NOSPT;
		}
		if (vp_image_b[id]->dram_addr & 0x1) {
			DBG_ERR("AOP%d image B address 0x%x should be 2 byte align\r\n",
				(unsigned int)p_request->command, (unsigned int)vp_image_b[id]->dram_addr);
			return E_NOSPT;
		}
		if (vp_image_c[id]->dram_addr & 0x1) {
			DBG_ERR("AOP%d image C address 0x%x should be 2 byte align\r\n",
				(unsigned int)p_request->command, (unsigned int)vp_image_c[id]->dram_addr);
			return E_NOSPT;
		}
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
		} else {
			const_reg.reg = p_property->property & GRPH_SHF_MSK;
		}
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_U)) {
			// If no IN2_OP is enabled, graph will use IN2_OP to do SHF on U plane
			in2reg.reg = graph_get_reg(id, GRPH_IN2_REG_OFS);
			if (in2reg.reg == 0) {
				if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_U, &p_property) != E_OK) {
					return E_OK;
				}
				in2reg.bit.IN2_OP = GRPH_INOP_SHIFTR_ADD;
				in2reg.bit.INSHF_2 = p_property->property;
				in2reg.bit.INCONST_2 = 0;
				graph_set_reg(id, GRPH_IN2_REG_OFS, in2reg.reg);
			}
		}
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_V)) {
			// If no IN2_2_OP is enabled, graph will use IN2_2_OP to do SHF on V plane
			in2_2reg.reg = graph_get_reg(id, GRPH_IN2_2_REG_OFS);
			if (in2_2reg.reg == 0) {
				if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_V, &p_property) != E_OK) {
					return E_OK;
				}
				in2_2reg.bit.IN2_OP = GRPH_INOP_SHIFTR_ADD;
				in2_2reg.bit.INSHF_2 = p_property->property;
				in2_2reg.bit.INCONST_2 = 0;
				graph_set_reg(id, GRPH_IN2_2_REG_OFS, in2_2reg.reg);
			}
		}
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for ABS minus shift

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_abs_minus_shf(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = 0;
	T_GRPH_IN2_REG      in2reg = {0};
	T_GRPH_IN2_2_REG    in2_2reg = {0};
	T_GRPH_OUT3_REG     out3reg = {0};
	T_GRPH_OUT3_2_REG   out3_2reg = {0};
	T_GRPH_CONST_REG    const_reg = {0};

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
		// If no IN2_OP, OUT3_OP are enabled, graph will use IN2_OP, OUT3_OP to do SHF, ABS
		in2reg.reg = graph_get_reg(id, GRPH_IN2_REG_OFS);
		out3reg.reg = graph_get_reg(id, GRPH_OUT3_REG_OFS);
		if ((in2reg.reg == 0) && (out3reg.reg == 0)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
				DBG_ERR("property not found\r\n");
				return E_NOSPT;
			}
			in2reg.bit.IN2_OP = GRPH_INOP_SHIFTR_ADD;
			in2reg.bit.INSHF_2 = p_property->property;
			in2reg.bit.INCONST_2 = 0;
			out3reg.bit.OUT_OP = GRPH_OUTOP_ABS;
			graph_set_reg(id, GRPH_IN2_REG_OFS, in2reg.reg);
			graph_set_reg(id, GRPH_OUT3_REG_OFS, out3reg.reg);
		}
		break;
	case GRPH_FORMAT_16BITS:
		if (vp_image_a[id]->dram_addr & 0x1) {
			DBG_ERR("AOP%d image A address 0x%x should be 2 byte align\r\n",
				(int)p_request->command,
				(unsigned int)vp_image_a[id]->dram_addr);
			return E_NOSPT;
		}
		if (vp_image_b[id]->dram_addr & 0x1) {
			DBG_ERR("AOP%d image B address 0x%x should be 2 byte align\r\n",
				(int)p_request->command,
				(unsigned int)vp_image_b[id]->dram_addr);
			return E_NOSPT;
		}
		if (vp_image_c[id]->dram_addr & 0x1) {
			DBG_ERR("AOP%d image C address 0x%x should be 2 byte align\r\n",
				(int)p_request->command,
				(unsigned int)vp_image_c[id]->dram_addr);
			return E_NOSPT;
		}
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = (p_property->property & GRPH_SHF_MSK) | 0x80000000;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_U)) {
			// If no IN2_OP, OUT3_OP are enabled, graph will use IN2_OP, OUT3_OP to do SHF, ABS on U plane
			in2reg.reg = graph_get_reg(id, GRPH_IN2_REG_OFS);
			out3reg.reg = graph_get_reg(id, GRPH_OUT3_REG_OFS);
			if ((in2reg.reg == 0) && (out3reg.reg == 0)) {
				if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_U, &p_property) != E_OK) {
					DBG_ERR("U property not found\r\n");
					return E_NOSPT;
				}
				in2reg.bit.IN2_OP = GRPH_INOP_SHIFTR_ADD;
				in2reg.bit.INSHF_2 = p_property->property;
				in2reg.bit.INCONST_2 = 0;
				out3reg.bit.OUT_OP = GRPH_OUTOP_ABS;
				graph_set_reg(id, GRPH_IN2_REG_OFS, in2reg.reg);
				graph_set_reg(id, GRPH_OUT3_REG_OFS, out3reg.reg);
			}
		}
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_V)) {
			// If no IN2_2_OP, OUT3_2_OP are enabled, graph will use IN2_2_OP, OUT3_2_OP to do SHF, ABS on V plane
			in2_2reg.reg = graph_get_reg(id, GRPH_IN2_2_REG_OFS);
			out3_2reg.reg = graph_get_reg(id, GRPH_OUT3_2_REG_OFS);
			if ((in2_2reg.reg == 0) && (out3_2reg.reg == 0)) {
				if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_V, &p_property) != E_OK) {
					DBG_ERR("V property not found\r\n");
					return E_NOSPT;
				}
				in2_2reg.bit.IN2_OP = GRPH_INOP_SHIFTR_ADD;
				in2_2reg.bit.INSHF_2 = p_property->property;
				in2_2reg.bit.INCONST_2 = 0;
				graph_set_reg(id, GRPH_IN2_2_REG_OFS, in2_2reg.reg);
				graph_set_reg(id, GRPH_OUT3_2_REG_OFS, out3_2reg.reg);
			}
		}
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for Color Key

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_colorkey(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = 0;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_CONST_REG2   const2reg = {0};

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & GRPH_CK_MSK;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & GRPH_CK16BIT_MSK;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		graph_set_reg(id, GRPH_CONST_REG_OFS, 0);
		graph_set_reg(id, GRPH_CONST_REG2_OFS, 0);
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_U)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_U, &p_property) != E_OK) {
				DBG_ERR("U property not found\r\n");
				return E_NOSPT;
			}
			const_reg.reg = p_property->property & GRPH_CK_MSK;
			graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		}
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_V)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_V, &p_property) != E_OK) {
				DBG_ERR("V property not found\r\n");
				return E_NOSPT;
			}
			const2reg.reg = p_property->property & GRPH_CK_MSK;
			graph_set_reg(id, GRPH_CONST_REG2_OFS, const2reg.reg);
		}
		break;
	case GRPH_FORMAT_32BITS_ARGB8888_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B, &p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & GRPH_CK_MSK;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G, &p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & GRPH_CK_MSK) << 8;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R, &p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & GRPH_CK_MSK) << 16;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_ARGB1555_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B, &p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & GRPH_CK_RGB5_MSK;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G, &p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & GRPH_CK_RGB5_MSK) << 8;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R, &p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & GRPH_CK_RGB5_MSK) << 16;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_ARGB4444_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B, &p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & GRPH_CK_RGB4_MSK;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G, &p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & GRPH_CK_RGB4_MSK) << 8;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R, &p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & GRPH_CK_RGB4_MSK) << 16;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for Color Key (Greater Equal version)

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_colorkey_ge(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = 0;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_CONST_REG2   const2reg = {0};

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = (p_property->property & GRPH_CK_MSK) | 0x80000000;;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_U)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_U, &p_property) != E_OK) {
				DBG_ERR("U property not found\r\n");
				return E_NOSPT;
			}
			const_reg.reg = (p_property->property & GRPH_CK_MSK) | 0x80000000;
			graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		}
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_V)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_V, &p_property) != E_OK) {
				DBG_ERR("V property not found\r\n");
				return E_NOSPT;
			}
			const2reg.reg = (p_property->property & GRPH_CK_MSK) | 0x80000000;
			graph_set_reg(id, GRPH_CONST_REG2_OFS, const2reg.reg);
			if (p_request->format == GRPH_FORMAT_16BITS_UVPACK_V) {  /* Temp fix */
				const_reg.reg |= 0x80000000;
				graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
			}
		}
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for Color Filter

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_color_filter(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = 0;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_CONST_REG2   const2reg = {0};
	T_GRPH_CONST_REG3   const3reg = {0};
	T_GRPH_CONST_REG4   const4reg = {0};

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
	case GRPH_FORMAT_16BITS_UVPACK:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		{
			if (graph_chk_ptr((UINT32)(p_request->p_ckeyfilter)) == FALSE) {
				DBG_ERR("invalid p_ckeyfilter pontr 0x%x\r\n",
					(unsigned int)p_request->p_ckeyfilter);
				return E_PAR;
			}
			const_reg.reg = p_property->property | GRPH_CKEY_FILTER_MSK;
			const2reg.reg = p_request->p_ckeyfilter->FKEY1;
			const3reg.reg = p_request->p_ckeyfilter->FKEY2;
			const4reg.reg = p_request->p_ckeyfilter->RKEY;
			graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
			graph_set_reg(id, GRPH_CONST_REG2_OFS, const2reg.reg);
			graph_set_reg(id, GRPH_CONST_REG3_OFS, const3reg.reg);
			graph_set_reg(id, GRPH_CONST_REG4_OFS, const4reg.reg);
		}
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for Text Operation

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_PAR: width/height NOT pass through image C
	- @b E_NOSPT: Operation no support.
*/
static ER aop_text(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = 0;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_CONST_REG2   const2reg = {0};
	T_GRPH_LCNT_REG height_reg = {0};
	T_GRPH_XRGN_REG width_reg = {0};

	if (p_request->command == GRPH_CMD_TEXT_COPY) {
		// Text Copy only has image C, and width/height are pass through image C
		if (graph_chk_ptr((UINT32)vp_image_c[id])) {
			if ((vp_image_c[id]->height == 0) || (vp_image_c[id]->width == 0)) {
				DBG_ERR("grph cmd 0x%x should pass width/height through image C\r\n",
					(unsigned int)p_request->command);
				return E_PAR;
			}
			height_reg.bit.ACT_HEIGHT = vp_image_c[id]->height;
			graph_set_reg(id, GRPH_LCNT_REG_OFS, height_reg.reg);

			width_reg.bit.ACT_WIDTH = vp_image_c[id]->width;
			graph_set_reg(id, GRPH_XRGN_REG_OFS, width_reg.reg);
		}
	}

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) == E_OK) {
			const_reg.reg = p_property->property;
			graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		}
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_U)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_U, &p_property) == E_OK) {
				const_reg.reg = p_property->property;
				graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
			}
		}
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_V)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_V, &p_property) == E_OK) {
				const2reg.reg = p_property->property;
				graph_set_reg(id, GRPH_CONST_REG2_OFS, const2reg.reg);
			}
		}
		break;
	case GRPH_FORMAT_32BITS_ARGB8888_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B, &p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & 0xFF;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G, &p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0xFF) << 8;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R, &p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0xFF) << 16;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_32BITS_ARGB8888_A:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_A, &p_property) != E_OK) {
			DBG_ERR("A property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = (p_property->property & 0xFF) << 24;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_ARGB1555_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B, &p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & 0x1F;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G, &p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0x1F) << 5;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R, &p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0x1F) << 10;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_ARGB1555_A:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_A, &p_property) != E_OK) {
			DBG_ERR("A property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = (p_property->property & 0x1) << 15;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_ARGB4444_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B, &p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & 0xF;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G, &p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0xF) << 4;
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R, &p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0xF) << 8;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_ARGB4444_A:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_A, &p_property) != E_OK) {
			DBG_ERR("A property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = (p_property->property & 0xF) << 12;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for Blending

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_blend(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = 0;
	PGRPH_PROPERTY      p_property_r = NULL;
	PGRPH_PROPERTY      p_property_g = NULL;
	PGRPH_PROPERTY      p_property_b = NULL;

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		graph_set_reg(id, GRPH_BLD_REG_OFS, p_property->property & GRPH_BLD_WGT_MSK2);
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_U)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_U, &p_property) != E_OK) {
				DBG_ERR("U property not found\r\n");
				return E_NOSPT;
			}
			graph_set_reg(id, GRPH_BLD_REG_OFS, p_property->property & GRPH_BLD_WGT_MSK2);
		}
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_V)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_V, &p_property) != E_OK) {
				DBG_ERR("V property not found\r\n");
				return E_NOSPT;
			}
			graph_set_reg(id, GRPH_BLD2_REG_OFS, p_property->property & GRPH_BLD_WGT_MSK2);
		}
		break;
	case GRPH_FORMAT_16BITS_RGB565:
	case GRPH_FORMAT_32BITS_ARGB8888_RGB:
	case GRPH_FORMAT_16BITS_ARGB1555_RGB:
	case GRPH_FORMAT_16BITS_ARGB4444_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R, &p_property_r) == E_OK) {
			if (p_property_r) {
				graph_set_reg(id, GRPH_BLD_REG_OFS, p_property_r->property & GRPH_BLD_WGT_MSK2);
			}
		} else if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G, &p_property_g) == E_OK) {
			if (p_property_g) {
				graph_set_reg(id, GRPH_BLD_REG_OFS, p_property_g->property & GRPH_BLD_WGT_MSK2);
			}
		} else if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B, &p_property_b) == E_OK) {
			if (p_property_b) {
				graph_set_reg(id, GRPH_BLD_REG_OFS, p_property_b->property & GRPH_BLD_WGT_MSK2);
			}
		} else {
			DBG_ERR("RGB property not found\r\n");
			return E_NOSPT;
		}
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for ACC

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_acc(GRPH_ID id, PGRPH_REQUEST p_request)
{
	T_GRPH_ACCU2_REG    acc2_reg = {0};

	if (graph_chk_ptr((UINT32)vp_image_c[id])) {
		// If user assign image C, we think user want acc result
		// stored in DRAM

	} else {
		T_GRPH_IMG3_ADDR_REG addr_reg = {0};
		T_GRPH_IMG3_LOFF_REG lineofs_reg = {0};

		memset(&grph_acc_buffer[ll_req_index[id]*ACC_BUF_UNIT+0], 0, 8);

		addr_reg.bit.IMG3_ADDR = grph_platform_va2pa((UINT32)(&grph_acc_buffer[ll_req_index[id]*ACC_BUF_UNIT+0]));
		graph_set_reg(id, GRPH_IMG3_ADDR_REG_OFS, addr_reg.reg);
//		DBG_WRN("AOPACC: local addr 0x%x\r\n", addr_reg.reg);

		lineofs_reg.bit.IMG3_LOFF = 8;
		graph_set_reg(id, GRPH_IMG3_LOFF_REG_OFS, lineofs_reg.reg);

		if (grph_platform_dma_flush_dev2mem_width_neq_loff((UINT32)&grph_acc_buffer[ll_req_index[id]*ACC_BUF_UNIT+0], ACC_BUF_UNIT)) {
			DBG_WRN("%s: flush fail\r\n", __func__);
		}
	}
	// because support link list, always dump acc result to DRAM
	acc2_reg.bit.ACC_TO_DRAM = 1;
	graph_set_reg(id, GRPH_ACCU2_REG_OFS, acc2_reg.reg);

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
	case GRPH_FORMAT_16BITS:
	case GRPH_FORMAT_16BITS_UVPACK:
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for Multiplication

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_mult(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = 0;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_CONST_REG2   const2reg = {0};

	switch (p_request->format) {
	case GRPH_FORMAT_16BITS:
		if (vp_image_a[id]->dram_addr & 0x1) {
			DBG_ERR("AOP%d image A address 0x%x should be 2 byte align\r\n",
				(int)p_request->command,
				(unsigned int)vp_image_a[id]->dram_addr);
			return E_NOSPT;
		}
		if (vp_image_b[id]->dram_addr & 0x1) {
			DBG_ERR("AOP%d image B address 0x%x should be 2 byte align\r\n",
				(int)p_request->command,
				(unsigned int)vp_image_b[id]->dram_addr);
			return E_NOSPT;
		}
		if (vp_image_c[id]->dram_addr & 0x1) {
			DBG_ERR("AOP%d image C address 0x%x should be 2 byte align\r\n",
				(int)p_request->command,
				(unsigned int)vp_image_c[id]->dram_addr);
			return E_NOSPT;
		}
	case GRPH_FORMAT_8BITS:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & (GRPH_SHF_MSK | GRPH_SQ1_MSK | GRPH_SQ2_MSK | 0x80000000);
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		graph_set_reg(id, GRPH_CONST_REG_OFS, 0);
		graph_set_reg(id, GRPH_CONST_REG2_OFS, 0);
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_U)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_U, &p_property) != E_OK) {
				DBG_ERR("U property not found\r\n");
				return E_NOSPT;
			}
			const_reg.reg = p_property->property & (GRPH_SHF_MSK | GRPH_SQ1_MSK | GRPH_SQ2_MSK | 0x80000000);
			graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		}
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_V)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_V, &p_property) != E_OK) {
				DBG_ERR("V property not found\r\n");
				return E_NOSPT;
			}
			const2reg.reg = p_property->property & (GRPH_SHF_MSK | GRPH_SQ1_MSK | GRPH_SQ2_MSK | 0x80000000);
			graph_set_reg(id, GRPH_CONST_REG2_OFS, const2reg.reg);
		}
		break;
	default:
		DBG_ERR("not supported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for Text Multiplication

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_text_mult(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = 0;
	PGRPH_PROPERTY      p_property_r = NULL;
	PGRPH_PROPERTY      p_property_g = NULL;
	PGRPH_PROPERTY      p_property_b = NULL;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_CONST_REG2   const2reg = {0};

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & 0x40000FFF;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		graph_set_reg(id, GRPH_CONST_REG_OFS, 0);
		graph_set_reg(id, GRPH_CONST_REG2_OFS, 0);
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_U)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_U, &p_property) != E_OK) {
				DBG_ERR("U property not found\r\n");
				return E_NOSPT;
			}
			const_reg.reg = p_property->property & 0x40000FFF;
			graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		}
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_V)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_V, &p_property) != E_OK) {
				DBG_ERR("V property not found\r\n");
				return E_NOSPT;
			}
			const2reg.reg = p_property->property & 0x40000FFF;
			graph_set_reg(id, GRPH_CONST_REG2_OFS, const2reg.reg);
		}
		break;
	case GRPH_FORMAT_32BITS_ARGB8888_RGB:
	case GRPH_FORMAT_16BITS_ARGB1555_RGB:
	case GRPH_FORMAT_16BITS_ARGB4444_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R, &p_property_r) == E_OK) {
			if (p_property_r) {
				const_reg.reg = p_property_r->property & 0x40000FFF;
			}
		} else if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G, &p_property_g) == E_OK) {
			if (p_property_g) {
				const_reg.reg = p_property_g->property & 0x40000FFF;
			}
		} else if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B, &p_property_b) == E_OK) {
			if (p_property_b) {
				const_reg.reg = p_property_b->property & 0x40000FFF;
			}
		} else {
			DBG_ERR("RGB property not found\r\n");
			return E_NOSPT;
		}
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	case GRPH_FORMAT_32BITS_ARGB8888_A:
	case GRPH_FORMAT_16BITS_ARGB4444_A:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_A, &p_property) != E_OK) {
			DBG_ERR("A property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg = p_property->property & 0x40000FFF;
		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
		break;
	default:
		DBG_ERR("not supported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for Alpha Plane operations

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
	- @b E_PAR: width/height NOT pass through image C
*/
static ER aop_alpha_plane(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = NULL;
	PGRPH_PROPERTY      p_property_r = NULL;
	PGRPH_PROPERTY      p_property_g = NULL;
	PGRPH_PROPERTY      p_property_b = NULL;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_LCNT_REG height_reg = {0};
	T_GRPH_XRGN_REG width_reg = {0};

	// width/height are pass through image C
	if (graph_chk_ptr((UINT32)vp_image_c[id])) {
		if ((vp_image_c[id]->height == 0) || (vp_image_c[id]->width == 0)) {
			DBG_ERR("grph cmd 0x%x should pass width/height through image C\r\n",
				(unsigned int)p_request->command);
			return E_PAR;
		}
		height_reg.bit.ACT_HEIGHT = vp_image_c[id]->height;
		graph_set_reg(id, GRPH_LCNT_REG_OFS, height_reg.reg);

		width_reg.bit.ACT_WIDTH = vp_image_c[id]->width;
		graph_set_reg(id, GRPH_XRGN_REG_OFS, width_reg.reg);
	}

	graph_set_reg(id, GRPH_BLD_REG_OFS, 0);
	graph_set_reg(id, GRPH_BLD2_REG_OFS, 0);

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
	case GRPH_FORMAT_16BITS_RGB565:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			graph_set_reg(id, GRPH_BLD_REG_OFS, graph_get_reg(id, GRPH_BLD_REG_OFS) & (~GRPH_COLOR_ALPHA_MASK));
		} else {
			if (p_property->property & GRPH_COLOR_ALPHA_MASK) {
				graph_set_reg(id, GRPH_BLD_REG_OFS, p_property->property & GRPH_COLOR_ALPHA_MASK);
				const_reg.reg = (p_property->property & GRPH_CONST_MASK) << 8;
				graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
			} else {
				graph_set_reg(id, GRPH_BLD_REG_OFS, graph_get_reg(id, GRPH_BLD_REG_OFS) & (~GRPH_COLOR_ALPHA_MASK));
			}
		}
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_U)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_U, &p_property) != E_OK) {
				graph_set_reg(id, GRPH_BLD_REG_OFS, graph_get_reg(id, GRPH_BLD_REG_OFS) & (~GRPH_COLOR_ALPHA_MASK));
			} else {
				if (p_property->property & GRPH_COLOR_ALPHA_MASK) {
					graph_set_reg(id, GRPH_BLD_REG_OFS, p_property->property & GRPH_COLOR_ALPHA_MASK);
					const_reg.reg = (p_property->property & GRPH_CONST_MASK) << 8;
					graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
				} else {
					graph_set_reg(id, GRPH_BLD_REG_OFS, graph_get_reg(id, GRPH_BLD_REG_OFS) & (~GRPH_COLOR_ALPHA_MASK));
				}
			}
		}
		if ((p_request->format == GRPH_FORMAT_16BITS_UVPACK) ||
			(p_request->format == GRPH_FORMAT_16BITS_UVPACK_V)) {
			if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_V, &p_property) != E_OK) {
				graph_set_reg(id, GRPH_BLD2_REG_OFS, graph_get_reg(id, GRPH_BLD2_REG_OFS) & (~GRPH_COLOR_ALPHA_MASK));
			} else {
				if (p_property->property & GRPH_COLOR_ALPHA_MASK) {
					graph_set_reg(id, GRPH_BLD2_REG_OFS, p_property->property & GRPH_COLOR_ALPHA_MASK);
					const_reg.reg = (p_property->property & GRPH_CONST_MASK) << 8;
					graph_set_reg(id, GRPH_CONST_REG2_OFS, const_reg.reg);
				} else {
					graph_set_reg(id, GRPH_BLD2_REG_OFS, graph_get_reg(id, GRPH_BLD2_REG_OFS) & (~GRPH_COLOR_ALPHA_MASK));
				}
			}
		}
		break;
//    case GRPH_FORMAT_16BITS_RGB565:
//        break;
	case GRPH_FORMAT_32BITS_ARGB8888_RGB:
	case GRPH_FORMAT_16BITS_ARGB1555_RGB:
	case GRPH_FORMAT_16BITS_ARGB4444_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R, &p_property_r) == E_OK) {
			if (p_property_r) {
				graph_set_reg(id, GRPH_BLD_REG_OFS, p_property_r->property & GRPH_ENABLE_ALPHA_SRC);
			}
		} else if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G, &p_property_g) == E_OK) {
			if (p_property_g) {
				graph_set_reg(id, GRPH_BLD_REG_OFS, p_property_g->property & GRPH_ENABLE_ALPHA_SRC);
			}
		} else if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B, &p_property_b) == E_OK) {
			if (p_property_b) {
				graph_set_reg(id, GRPH_BLD_REG_OFS, p_property_b->property & GRPH_ENABLE_ALPHA_SRC);
			}
		} else if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) == E_OK) {
			if (p_property) {
				graph_set_reg(id, GRPH_BLD_REG_OFS, p_property->property & GRPH_ENABLE_ALPHA_SRC);
			}
		} else {
			DBG_ERR("RGB property not found\r\n");
			return E_NOSPT;
		}

		// mantis 56842: image C address is required to write to image B
		{
			UINT32 addr, ofs;

			addr = graph_get_reg(id, GRPH_IMG3_ADDR_REG_OFS);
			graph_set_reg(id, GRPH_IMG2_ADDR_REG_OFS, addr);

			ofs = graph_get_reg(id, GRPH_IMG3_LOFF_REG_OFS);
			graph_set_reg(id, GRPH_IMG2_LOFF_REG_OFS, ofs);
		}
		break;
	default:
		DBG_ERR("not supported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for 1D LUT

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_1d_lut(GRPH_ID id, PGRPH_REQUEST p_request)
{
	UINT32 i;
	UINT32 *p_lut;
	PGRPH_PROPERTY      p_property = 0;
//    T_GRPH_CONST_REG    const_reg = {0};

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_LUT_BUF, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		p_lut = (UINT32 *)p_property->property;
		if (graph_chk_ptr((UINT32)p_lut) == FALSE) {
			DBG_WRN("AOP22: invalid LUT address 0x%p\r\n", p_lut);
		}
		for (i = 0; i < 256 / 4; i++) {
			graph_set_reg(id, GRPH_LUT_REG_OFS + i * 4, p_lut[i]);
		}
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_LUT_BUF, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		p_lut = (UINT32 *)p_property->property;
		if (graph_chk_ptr((UINT32)p_lut) == FALSE) {
			DBG_WRN("AOP22: invalid LUT address 0x%p\r\n", p_lut);
		}
		for (i = 0; i < (129 * 2 + 3) / 4; i++) {
			graph_set_reg(id, GRPH_LUT_REG_OFS + i * 4, p_lut[i]);
		}
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for 2D LUT

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_2d_lut(GRPH_ID id, PGRPH_REQUEST p_request)
{
	UINT32 i;
	UINT32 *p_lut;
	PGRPH_PROPERTY      p_property = 0;

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
	case GRPH_FORMAT_16BITS_UVPACK_U:
	case GRPH_FORMAT_16BITS_UVPACK_V:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_LUT_BUF, &p_property) != E_OK) {
			DBG_ERR("property not found\r\n");
			return E_NOSPT;
		}
		p_lut = (UINT32 *)p_property->property;
		if (graph_chk_ptr((UINT32)p_lut) == FALSE) {
			DBG_WRN("AOP22: invalid LUT address 0x%p\r\n", p_lut);
		}
		for (i = 0; i < (17 * 17 + 3) / 4; i++) {
			graph_set_reg(id, GRPH_LUT_REG_OFS + i * 4, p_lut[i]);
		}
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	return E_OK;
}

/*
    Graphic AOP request for RGB+YUV Blending

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_rgb_yuv_blend(GRPH_ID id, PGRPH_REQUEST p_request)
{
	UINT32 i, buf_size;
	UINT32 *ptr;
	PGRPH_PROPERTY      p_property = NULL;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_CONST_REG3   const3reg = {0};

	if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_YUVFMT,
			&p_property) != E_OK) {
		DBG_ERR("YUV property not found\r\n");
		return E_NOSPT;
	}
	const_reg.reg = (p_property->property == GRPH_YUV_411) ? 1 : 0;

	if (graph_fetch_property(p_request->p_property,
			GRPH_PROPERTY_ID_UV_SUBSAMPLE, &p_property) == E_OK) {
		// If GRPH_PROPERTY_ID_UV_SUBSAMPLE is found,
		// apply setting from it
		if (p_property->property == GRPH_UV_CENTERED) {
			if (nvt_get_chip_id() == CHIP_NA51055) {
				DBG_WRN("%s: NA51055 not support UV centered\r\n",
					__func__);
			} else {
				const_reg.reg |= 1 << 5;
			}
		}
	}

	switch (p_request->format) {
	case GRPH_FORMAT_16BITS_ARGB1555_RGB:
		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_ALPHA0_INDEX, &p_property) != E_OK) {
			DBG_ERR("ALPHA0_INDEX property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0xF) << 8;

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_ALPHA1_INDEX, &p_property) != E_OK) {
			DBG_ERR("ALPHA1_INDEX property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0xF) << 12;

		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) == E_OK) {
			// If GRPH_PROPERTY_ID_INVRGB is found,
			// enable AOP24_RGB_INVERT
			const_reg.reg |= 1 << 2;

			// alpha invert
			if (p_property->property & (1 << 8)) {
				DBG_WRN("GRPH_CMD_RGBYUV_BLEND not support alpha invert\r\n");
			}

			const3reg.reg = p_property->property & 0xFF;
		}
		break;
	case GRPH_FORMAT_16BITS_ARGB4444_RGB:
	case GRPH_FORMAT_32BITS_ARGB8888_RGB:
		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) == E_OK) {
			// If GRPH_PROPERTY_ID_INVRGB is found,
			// enable AOP24_RGB_INVERT
			const_reg.reg |= 1 << 2;

			// alpha invert
			if (p_property->property & (1 << 8)) {
				DBG_WRN("GRPH_CMD_RGBYUV_BLEND not support alpha invert\r\n");
			}

			const3reg.reg = p_property->property & 0xFF;
		}
		break;
	case GRPH_FORMAT_16BITS_RGB565:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			DBG_ERR("Normal property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0xFF) << 8;

		graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) == E_OK) {
			// If GRPH_PROPERTY_ID_INVRGB is found,
			// enable AOP24_RGB_INVERT
			const_reg.reg |= 1 << 2;

			if (p_property->property & (1 << 8)) {
				DBG_WRN("GRPH_CMD_RGBYUV_BLEND not support alpha invert\r\n");
			}

			const3reg.reg = p_property->property & 0xFF;
		}
		break;
	case GRPH_FORMAT_PALETTE_1BIT:
	case GRPH_FORMAT_PALETTE_2BITS:
	case GRPH_FORMAT_PALETTE_4BITS:
		// alpha invert
		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) == E_OK) {
			// If GRPH_PROPERTY_ID_INVRGB is found,
			// enable AOP24_RGB_INVERT
			const_reg.reg |= 1 << 2;

			if (p_property->property & (1 << 8)) {
				DBG_WRN("GRPH_CMD_RGBYUV_BLEND not support alpha invert\r\n");
			}

			const3reg.reg = p_property->property & 0xFF;
		}

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_PAL_BUF, &p_property) != E_OK) {
			DBG_ERR("GRPH_PROPERTY_ID_PAL_BUF not found\r\n");
			return E_NOSPT;
		}
		ptr = (UINT32 *)p_property->property;
		if (graph_chk_ptr((UINT32)ptr) == FALSE) {
			DBG_WRN("AOP22: invalid LUT address 0x%p\r\n", ptr);
		}
		if (p_request->format == GRPH_FORMAT_PALETTE_4BITS) {
			buf_size = 16;
		} else if (p_request->format == GRPH_FORMAT_PALETTE_2BITS) {
			buf_size = 4;
		} else { // GRPH_FORMAT_PALETTE_1BIT
			buf_size = 2;
		}
		for (i = 0; i < buf_size; i++) {
			graph_set_reg(id, GRPH_PAL_REG_OFS + i * 4, ptr[i]);
		}
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
	graph_set_reg(id, GRPH_CONST_REG3_OFS, const3reg.reg);

	return E_OK;
}

/*
    Graphic AOP request for RGB+YUV color key

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_rgb_yuv_colorkey(GRPH_ID id, PGRPH_REQUEST p_request)
{
	UINT32 i, buf_size;
	UINT32 *ptr;
	PGRPH_PROPERTY      p_property = NULL;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_CONST_REG2   const2reg = {0};
	T_GRPH_CONST_REG3   const3reg = {0};
	UINT32              color_key;

	if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_YUVFMT,
				&p_property) != E_OK) {
		DBG_ERR("YUV property not found\r\n");
		return E_NOSPT;
	}
	const_reg.reg = (p_property->property == GRPH_YUV_411) ? 1 : 0;

	switch (p_request->format) {
	case GRPH_FORMAT_16BITS_RGB565:
		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_NORMAL, &p_property) != E_OK) {
			DBG_ERR("Normal property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0xFF) << 8;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B,
				&p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		color_key = (p_property->property & 0x1F) << 0;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G,
				&p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		color_key |= (p_property->property & 0x3F) << 5;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R,
				&p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		color_key |= (p_property->property & 0x1F) << 11;
		const2reg.reg = color_key;

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) == E_OK) {
			// If GRPH_PROPERTY_ID_INVRGB is found,
			// enable AOP25_RGB_INVERT
			const_reg.reg |= 1 << 2;

			// alpha invert
			if (p_property->property & (1 << 8)) {
				DBG_WRN("RGB565 not support alpha invert\r\n");
			}

			const3reg.reg = p_property->property & 0xFF;
		}
		break;
	case GRPH_FORMAT_16BITS_ARGB1555_RGB:
		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_ALPHA0_INDEX, &p_property) != E_OK) {
			DBG_ERR("ALPHA0_INDEX property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0xF) << 8;

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_ALPHA1_INDEX, &p_property) != E_OK) {
			DBG_ERR("ALPHA1_INDEX property not found\r\n");
			return E_NOSPT;
		}
		const_reg.reg |= (p_property->property & 0xF) << 12;

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_B, &p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		color_key = (p_property->property & 0x1F) << 0;

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_G, &p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		color_key |= (p_property->property & 0x1F) << 5;

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_R, &p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		color_key |= (p_property->property & 0x1F) << 10;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_A,
				&p_property) == E_OK) {
			// If user assigns alpha, driver considers user need
			// to compare alpha
			const_reg.reg |= 1 << 4;

			color_key |= (p_property->property & 0x1) << 15;
		}
		const2reg.reg = color_key;

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) == E_OK) {
			// If GRPH_PROPERTY_ID_INVRGB is found,
			// enable AOP25_RGB_INVERT
			const_reg.reg |= 1 << 2;

			// alpha invert
			if (p_property->property & (1 << 8)) {
				const_reg.reg |= 1 << 3;
			}

			const3reg.reg = p_property->property & 0xFF;
		}
		break;
	case GRPH_FORMAT_16BITS_ARGB4444_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B,
				&p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		color_key = (p_property->property & 0xF) << 0;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G,
				&p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		color_key |= (p_property->property & 0xF) << 4;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R,
				&p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		color_key |= (p_property->property & 0xF) << 8;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_A,
				&p_property) == E_OK) {
			// If user assigns alpha, driver considers user need
			// to compare alpha
			const_reg.reg |= 1 << 4;

			color_key |= (p_property->property & 0xF) << 12;
		}
		const2reg.reg = color_key;

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) == E_OK) {
			// If GRPH_PROPERTY_ID_INVRGB is found,
			// enable AOP25_RGB_INVERT
			const_reg.reg |= 1 << 2;

			// alpha invert
			if (p_property->property & (1 << 8)) {
				const_reg.reg |= 1 << 3;
			}

			const3reg.reg = p_property->property & 0xFF;
		}
		break;
	case GRPH_FORMAT_32BITS_ARGB8888_RGB:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B,
				&p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		color_key = (p_property->property & 0xFF) << 0;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G,
				&p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		color_key |= (p_property->property & 0xFF) << 8;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R,
				&p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		color_key |= (p_property->property & 0xFF) << 16;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_A,
				&p_property) == E_OK) {
			// If user assigns alpha, driver considers user need
			// to compare alpha
			const_reg.reg |= 1 << 4;

			color_key |= (p_property->property & 0xFF) << 24;
		}
		const2reg.reg = color_key;

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) == E_OK) {
			// If GRPH_PROPERTY_ID_INVRGB is found,
			// enable AOP25_RGB_INVERT
			const_reg.reg |= 1 << 2;

			// alpha invert
			if (p_property->property & (1 << 8)) {
				const_reg.reg |= 1 << 3;
			}

			const3reg.reg = p_property->property & 0xFF;
		}
		break;
	case GRPH_FORMAT_PALETTE_1BIT:
	case GRPH_FORMAT_PALETTE_2BITS:
	case GRPH_FORMAT_PALETTE_4BITS:
		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_B,
				&p_property) != E_OK) {
			DBG_ERR("B property not found\r\n");
			return E_NOSPT;
		}
		color_key = (p_property->property & 0xFF) << 0;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_G,
				&p_property) != E_OK) {
			DBG_ERR("G property not found\r\n");
			return E_NOSPT;
		}
		color_key |= (p_property->property & 0xFF) << 8;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_R,
				&p_property) != E_OK) {
			DBG_ERR("R property not found\r\n");
			return E_NOSPT;
		}
		color_key |= (p_property->property & 0xFF) << 16;

		if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_A,
				&p_property) == E_OK) {
			// If user assigns alpha, driver considers user need
			// to compare alpha
			const_reg.reg |= 1 << 4;

			color_key |= (p_property->property & 0xFF) << 24;
		}
		const2reg.reg = color_key;

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) == E_OK) {
			// If GRPH_PROPERTY_ID_INVRGB is found,
			// enable AOP25_RGB_INVERT
			const_reg.reg |= 1 << 2;

			// alpha invert
			if (p_property->property & (1 << 8)) {
				const_reg.reg |= 1 << 3;
			}

			const3reg.reg = p_property->property & 0xFF;
		}

		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_PAL_BUF, &p_property) != E_OK) {
			DBG_ERR("GRPH_PROPERTY_ID_PAL_BUF not found\r\n");
			return E_NOSPT;
		}
		ptr = (UINT32 *)p_property->property;
		if (graph_chk_ptr((UINT32)ptr) == FALSE) {
			DBG_WRN("AOP22: invalid LUT address 0x%p\r\n", ptr);
		}
		if (p_request->format == GRPH_FORMAT_PALETTE_4BITS) {
			buf_size = 16;
		} else if (p_request->format == GRPH_FORMAT_PALETTE_2BITS) {
			buf_size = 4;
		} else { // GRPH_FORMAT_PALETTE_1BIT
			buf_size = 2;
		}
		for (i = 0; i < buf_size; i++) {
			graph_set_reg(id, GRPH_PAL_REG_OFS + i * 4, ptr[i]);
		}
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
	graph_set_reg(id, GRPH_CONST_REG2_OFS, const2reg.reg);
	graph_set_reg(id, GRPH_CONST_REG3_OFS, const3reg.reg);

	return E_OK;
}

/*
    Graphic AOP request for RGB invert

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
        - @b E_OK: Done with no errors.
        - @b E_NOSPT: Operation no support.
*/
static ER aop_rgb_invert(GRPH_ID id, PGRPH_REQUEST p_request)
{
	PGRPH_PROPERTY      p_property = NULL;
	T_GRPH_CONST_REG    const_reg = {0};
	T_GRPH_CONST_REG3   const3reg = {0};

	switch (p_request->format) {
	case GRPH_FORMAT_16BITS_RGB565:
		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) != E_OK) {
			DBG_ERR("GRPH_PROPERTY_ID_INVRGB not found\r\n");
			return E_NOSPT;
		}
		const3reg.reg = p_property->property & 0xFF;
		break;
	case GRPH_FORMAT_16BITS_ARGB1555_RGB:
		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) != E_OK) {
			DBG_ERR("GRPH_PROPERTY_ID_INVRGB not found\r\n");
			return E_NOSPT;
		}

		// alpha invert
		if (p_property->property & (1 << 8)) {
			const_reg.reg |= 1 << 3;
		}

		const3reg.reg = p_property->property & 0xFF;
		break;
	case GRPH_FORMAT_16BITS_ARGB4444_RGB:
		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) != E_OK) {
			DBG_ERR("GRPH_PROPERTY_ID_INVRGB not found\r\n");
			return E_NOSPT;
		}

		// alpha invert
		if (p_property->property & (1 << 8)) {
			const_reg.reg |= 1 << 3;
		}

		const3reg.reg = p_property->property & 0xFF;
		break;
	case GRPH_FORMAT_32BITS_ARGB8888_RGB:
		if (graph_fetch_property(p_request->p_property,
				GRPH_PROPERTY_ID_INVRGB, &p_property) != E_OK) {
			DBG_ERR("GRPH_PROPERTY_ID_INVRGB not found\r\n");
			return E_NOSPT;
		}

		// alpha invert
		if (p_property->property & (1 << 8)) {
			const_reg.reg |= 1 << 3;
		}

		const3reg.reg = p_property->property & 0xFF;
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	graph_set_reg(id, GRPH_CONST_REG_OFS, const_reg.reg);
	graph_set_reg(id, GRPH_CONST_REG3_OFS, const3reg.reg);

	return E_OK;
}

/*
    Flush cache for AOP operations

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER aop_flush_cache(GRPH_ID id, PGRPH_REQUEST p_request)
{
	switch (p_request->command) {
	case GRPH_CMD_A_COPY:
	case GRPH_CMD_TEXT_AND_A:
	case GRPH_CMD_TEXT_OR_A:
	case GRPH_CMD_TEXT_XOR_A:
//    case GRPH_CMD_TEXT_MUL:
	case GRPH_CMD_1D_LUT:
	case GRPH_CMD_RGB_INVERT:
		// A -> C
		do {
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					// Output image width <--> lineoffset may contain data not allowed to modify by graphic
					// Use clean and invalidate to ensure this region will not be invalidated
					if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * vp_image_a[id]->height)) {
						// All DRAM are clean and invalidated, not need to flush image A anymore
						// Just break
						break;
					}
				}
			}
			if (graph_chk_ptr((UINT32)vp_image_a[id])) {
				if (grph_platform_is_valid_va(vp_image_a[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image A addr: 0x%x\r\n",
						(unsigned int)vp_image_a[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_flush_mem2dev(
						vp_image_a[id]->dram_addr,
						vp_image_a[id]->lineoffset * vp_image_a[id]->height)) {
					break;
				}
			}
		} while (0);
		break;
	case GRPH_CMD_PLUS_SHF:
	case GRPH_CMD_MINUS_SHF:
	case GRPH_CMD_MINUS_SHF_ABS:
	case GRPH_CMD_COLOR_EQ:
	case GRPH_CMD_COLOR_LE:
	case GRPH_CMD_COLOR_MR:
	case GRPH_CMD_A_AND_B:
	case GRPH_CMD_A_OR_B:
	case GRPH_CMD_A_XOR_B:
	case GRPH_CMD_TEXT_AND_AB:
	case GRPH_CMD_BLENDING:
	case GRPH_CMD_MULTIPLY_DIV:
	case GRPH_CMD_PACKING:
	case GRPH_CMD_TEXT_MUL:
	case GRPH_CMD_2D_LUT:
		// A, B -> C
		do {
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					// Output image width <--> lineoffset may contain data not allowed to modify by graphic
					// Use clean and invalidate to ensure this region will not be invalidated
					if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * vp_image_a[id]->height)) {
						// All DRAM are clean and invalidated, not need to flush image A, B anymore
						// Just break
						break;
					}
				}
			}
			if (graph_chk_ptr((UINT32)vp_image_a[id])) {
				if (grph_platform_is_valid_va(vp_image_a[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image A addr: 0x%x\r\n",
						(unsigned int)vp_image_a[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_a[id]->dram_addr)) {
					if (grph_platform_dma_flush_mem2dev(vp_image_a[id]->dram_addr, (vp_image_a[id]->lineoffset * vp_image_a[id]->height))) {
						// All DRAM are clean and invalidated, not need to flush image B anymore
						// Just break
						break;
					}
				}
			}
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_b[id])) {
				if (grph_platform_is_valid_va(vp_image_b[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image B addr: 0x%x\r\n",
						(unsigned int)vp_image_b[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_flush_mem2dev(
						vp_image_b[id]->dram_addr,
						(vp_image_b[id]->lineoffset * vp_image_a[id]->height))) {
					break;
				}
			}
		} while (0);
		break;
	case GRPH_CMD_TEXT_COPY:
		// reg -> C
		if (graph_chk_ptr((UINT32)vp_image_c[id])) {
			if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
				DBG_ERR("Invalid image C addr: 0x%x\r\n",
					(unsigned int)vp_image_c[id]->dram_addr);
				return E_NOSPT;
			}
			if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
				// Output image width <--> lineoffset may contain data not allowed to modify by graphic
				// Use clean and invalidate to ensure this region will not be invalidated
				if ((vp_image_c[id]->width != vp_image_c[id]->lineoffset) ||    // width != lineOffset => need to clean region between width to lineOffset
					(p_request->format == GRPH_FORMAT_16BITS_UVPACK_U) ||    // UV U only => need to clean region of V parts, else V part will be accidentally invalidated
					(p_request->format == GRPH_FORMAT_16BITS_UVPACK_V) ||    // UV V only => need to clean region of U parts, else U part will be accidentally invalidated
					(p_request->format == GRPH_FORMAT_32BITS_ARGB8888_RGB) ||// RGB only => need to clean region of A parts, else A part will be accidentally invalidated
					(p_request->format == GRPH_FORMAT_32BITS_ARGB8888_A) ||  // A only => need to clean region of RGB parts, else RGB part will be accidentally invalidated
					(p_request->format == GRPH_FORMAT_16BITS_ARGB1555_RGB) ||// RGB only => need to clean region of A parts, else A part will be accidentally invalidated
					(p_request->format == GRPH_FORMAT_16BITS_ARGB1555_A) ||  // A only => need to clean region of RGB parts, else RGB part will be accidentally invalidated
					(p_request->format == GRPH_FORMAT_16BITS_ARGB4444_RGB) ||// RGB only => need to clean region of A parts, else A part will be accidentally invalidated
					(p_request->format == GRPH_FORMAT_16BITS_ARGB4444_A)     // A only => need to clean region of RGB parts, else RGB part will be accidentally invalidated
				   ) {
					if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * vp_image_c[id]->height)) {
						break;
					}
				} else {
					grph_platform_dma_flush_dev2mem(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * vp_image_c[id]->height);
				}
			}
		}
		break;
	case GRPH_CMD_ACC:
		// A -> reg  or A -> C
		do {
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
//                DBG_WRN("GRPH ACC flush img C\r\n");
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					// Output image width <--> lineoffset may contain data not allowed to modify by graphic
					// Use clean and invalidate to ensure this region will not be invalidated
					if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * 1)) {
						// All DRAM are clean and invalidated, not need to flush image A anymore
						// Just break
						break;
					}
				}
			}

			if (graph_chk_ptr((UINT32)vp_image_a[id])) {
				if (grph_platform_is_valid_va(vp_image_a[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image A addr: 0x%x\r\n",
						(unsigned int)vp_image_a[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_flush_mem2dev(vp_image_a[id]->dram_addr, vp_image_a[id]->lineoffset * vp_image_a[id]->height)) {
					break;
				}
			}
		} while (0);
		break;
	case GRPH_CMD_DEPACKING:
		// A -> B, C
		do {
			// Image A should be cleaned first
			// Else it may be invalidate by following image B/C flush flow
			if (graph_chk_ptr((UINT32)vp_image_a[id])) {
				if (grph_platform_is_valid_va(vp_image_a[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image A addr: 0x%x\r\n",
						(unsigned int)vp_image_a[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_a[id]->dram_addr)) {
					if (grph_platform_dma_flush_mem2dev(vp_image_a[id]->dram_addr, vp_image_a[id]->lineoffset * vp_image_a[id]->height)) {
						break;
					}
				}
			}

			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_b[id])) {
				if (grph_platform_is_valid_va(vp_image_b[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image B addr: 0x%x\r\n",
						(unsigned int)vp_image_b[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_b[id]->dram_addr)) {
					BOOL is_flush_all = FALSE;

					// Output image width <--> lineoffset may contain data not allowed to modify by graphic
					// Use clean and invalidate to ensure this region will not be invalidated
					if ((vp_image_a[id]->width / 2) != vp_image_b[id]->lineoffset) {
						is_flush_all = grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_b[id]->dram_addr, vp_image_b[id]->lineoffset * vp_image_a[id]->height);
					} else {
						is_flush_all = grph_platform_dma_flush_dev2mem(vp_image_b[id]->dram_addr, vp_image_b[id]->lineoffset * vp_image_a[id]->height);
					}
					if (is_flush_all) {
						// All DRAM are clean and invalidated, not need to flush image A, C anymore
						// Just break
						break;
					}
				}
			}
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					// Output image width <--> lineoffset may contain data not allowed to modify by graphic
					// Use clean and invalidate to ensure this region will not be invalidated
					if ((vp_image_a[id]->width / 2) != vp_image_c[id]->lineoffset) {
						if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * vp_image_a[id]->height)) {
							break;
						}
					} else {
						grph_platform_dma_flush_dev2mem(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * vp_image_a[id]->height);
					}
				}
			}
		} while (0);
		break;
	case GRPH_CMD_PLANE_BLENDING:
		// [A], [B], [C] -> C
		do {
			if (graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					// Image C is either input or output channel, use clean and invalidate to flush cache
					if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_c[id]->dram_addr, (vp_image_c[id]->lineoffset * vp_image_c[id]->height))) {
						// All DRAM are clean and invalidated, not need to flush image A, B anymore
						// Just break
						break;
					}
				}
			}
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_a[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image A addr: 0x%x\r\n",
						(unsigned int)vp_image_a[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_a[id]->dram_addr)) {
					if (grph_platform_dma_flush_mem2dev(vp_image_a[id]->dram_addr, (vp_image_a[id]->lineoffset * vp_image_c[id]->height))) {
						break;
					}
				}
			}
			if (graph_chk_ptr((UINT32)vp_image_b[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_b[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image B addr: 0x%x\r\n",
						(unsigned int)vp_image_b[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_flush_mem2dev(
						vp_image_b[id]->dram_addr,
						(vp_image_b[id]->lineoffset * vp_image_c[id]->height))) {
					break;
				}
			}
		} while (0);
		break;
	case GRPH_CMD_COLOR_FILTER:
		do {

		if (p_request->format == GRPH_FORMAT_8BITS) {
			// A, B, C -> A, B, C
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					// Image C is either input or output channel, use clean and invalidate to flush cache
					if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_c[id]->dram_addr, (vp_image_c[id]->lineoffset * vp_image_a[id]->height))) {
						// All DRAM are clean and invalidated, not need to flush image A, B anymore
						// Just break
						break;
					}
				}
			}
		} else {
			// A, B -> A, B
		}
		if (graph_chk_ptr((UINT32)vp_image_a[id])) {
			if (grph_platform_is_valid_va(vp_image_a[id]->dram_addr) == 0) {
				DBG_ERR("Invalid image A addr: 0x%x\r\n",
					(unsigned int)vp_image_a[id]->dram_addr);
				return E_NOSPT;
			}
			if (grph_platform_dma_is_cacheable(vp_image_a[id]->dram_addr)) {
				// Image A is either input or output channel, use clean and invalidate to flush cache
				if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_a[id]->dram_addr, (vp_image_a[id]->lineoffset * vp_image_a[id]->height))) {
					// All DRAM are clean and invalidated, not need to flush image B anymore
					// Just break
					break;
				}
			}
		}
		if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_b[id])) {
			if (grph_platform_is_valid_va(vp_image_b[id]->dram_addr) == 0) {
				DBG_ERR("Invalid image B addr: 0x%x\r\n",
					(unsigned int)vp_image_b[id]->dram_addr);
				return E_NOSPT;
			}
			if (grph_platform_dma_is_cacheable(vp_image_b[id]->dram_addr)) {
				// Image B is either input or output channel, use clean and invalidate to flush cache
				if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_b[id]->dram_addr, (vp_image_b[id]->lineoffset * vp_image_a[id]->height))) {
					break;
				}
			}
		}

		} while (0);
		break;
	case GRPH_CMD_RGBYUV_BLEND:
	case GRPH_CMD_RGBYUV_COLORKEY:
		do {
			// A, B, C -> B, C
			if (graph_chk_ptr((UINT32)vp_image_a[id])) {
				if (grph_platform_is_valid_va(vp_image_a[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image A addr: 0x%x\r\n",
						(unsigned int)vp_image_a[id]->dram_addr);
					return E_NOSPT;
				}
				if (vp_image_a[id]->width & 0x03) {
					DBG_ERR("Invalid image A width: 0x%x\r\n",
						(unsigned int)vp_image_a[id]->width);
					DBG_ERR("GRPH_CMD_RGBYUV_xxx ONLY support 4 bytes aglin width\r\n");
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_a[id]->dram_addr)) {
					// Image A is either input or output channel, use clean and invalidate to flush cache
					if (grph_platform_dma_flush_mem2dev(vp_image_a[id]->dram_addr, vp_image_a[id]->lineoffset * vp_image_a[id]->height)) {
						// All DRAM are clean and invalidated, not need to flush image B, C anymore
						// Just break
						break;
					}
				}
			}

			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_b[id])) {
				if (grph_platform_is_valid_va(vp_image_b[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image B addr: 0x%x\r\n",
						(unsigned int)vp_image_b[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_b[id]->dram_addr)) {
					// Image B is either input or output channel, use clean and invalidate to flush cache
					if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_b[id]->dram_addr, (vp_image_b[id]->lineoffset * vp_image_a[id]->height))) {
						// All DRAM are clean and invalidated, not need to flush image C anymore
						// Just break
						break;
					}
				}
			}


			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					PGRPH_PROPERTY      p_property = NULL;
					UINT32              height;

					if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_YUVFMT, &p_property) != E_OK) {
						DBG_ERR("YUV property not found\r\n");
						return E_NOSPT;
					}
					height = (p_property->property == GRPH_YUV_411) ? (vp_image_a[id]->height / 2) : vp_image_a[id]->height;
					// Image B is either input or output channel, use clean and invalidate to flush cache
					if (grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_c[id]->dram_addr, (vp_image_c[id]->lineoffset * height))) {
						break;
					}
				}
			}
		} while (0);
		break;
	default:
		return E_NOSPT;
	}

	// Due to performance considerations, graphic driver use API in dma.c instead of dma_protected.h
	// Thus manually DSB here
	__asm__ __volatile__("dsb\n\t");
	return E_OK;
}

static ER aop_post_flush_cache(GRPH_ID id, PGRPH_REQUEST p_request)
{
	switch (p_request->command) {
	case GRPH_CMD_A_COPY:
	case GRPH_CMD_TEXT_AND_A:
	case GRPH_CMD_TEXT_OR_A:
	case GRPH_CMD_TEXT_XOR_A:
	case GRPH_CMD_1D_LUT:
	case GRPH_CMD_RGB_INVERT:
		// A -> C
		do {
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					// Output image width <--> lineoffset may contain data not allowed to modify by graphic
					// Use clean and invalidate to ensure this region will not be invalidated
					grph_platform_dma_post_flush_dev2mem(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * vp_image_a[id]->height);
				}
			}
		} while (0);
		break;
	case GRPH_CMD_PLUS_SHF:
	case GRPH_CMD_MINUS_SHF:
	case GRPH_CMD_MINUS_SHF_ABS:
	case GRPH_CMD_COLOR_EQ:
	case GRPH_CMD_COLOR_LE:
	case GRPH_CMD_COLOR_MR:
	case GRPH_CMD_A_AND_B:
	case GRPH_CMD_A_OR_B:
	case GRPH_CMD_A_XOR_B:
	case GRPH_CMD_TEXT_AND_AB:
	case GRPH_CMD_BLENDING:
	case GRPH_CMD_MULTIPLY_DIV:
	case GRPH_CMD_PACKING:
	case GRPH_CMD_TEXT_MUL:
	case GRPH_CMD_2D_LUT:
		// A, B -> C
		do {
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					// Output image width <--> lineoffset may contain data not allowed to modify by graphic
					// Use clean and invalidate to ensure this region will not be invalidated
					grph_platform_dma_post_flush_dev2mem(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * vp_image_a[id]->height);
				}
			}
		} while (0);
		break;
	case GRPH_CMD_TEXT_COPY:
		// reg -> C
		if (graph_chk_ptr((UINT32)vp_image_c[id])) {
			if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
				DBG_ERR("Invalid image C addr: 0x%x\r\n",
					(unsigned int)vp_image_c[id]->dram_addr);
				return E_NOSPT;
			}
			if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
				grph_platform_dma_post_flush_dev2mem(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * vp_image_c[id]->height);
			}
		}
		break;
	case GRPH_CMD_ACC:
		// A -> reg or A -> C
		do {
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					// Output image width <--> lineoffset may contain data not allowed to modify by graphic
					// Use clean and invalidate to ensure this region will not be invalidated
					grph_platform_dma_post_flush_dev2mem(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * 1);
				}
			}

		} while (0);
		break;
	case GRPH_CMD_DEPACKING:
		// A -> B, C
		do {
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_b[id])) {
				if (grph_platform_is_valid_va(vp_image_b[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image B addr: 0x%x\r\n",
						(unsigned int)vp_image_b[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_b[id]->dram_addr)) {
					grph_platform_dma_post_flush_dev2mem(vp_image_b[id]->dram_addr, vp_image_b[id]->lineoffset * vp_image_a[id]->height);
				}
			}
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					grph_platform_dma_post_flush_dev2mem(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * vp_image_a[id]->height);
				}
			}
		} while (0);
		break;
	case GRPH_CMD_PLANE_BLENDING:
		// [A], [B], [C] -> C
		do {
			if (graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					// Image C is either input or output channel, use clean and invalidate to flush cache
					grph_platform_dma_post_flush_dev2mem(vp_image_c[id]->dram_addr, (vp_image_c[id]->lineoffset * vp_image_c[id]->height));
				}
			}
		} while (0);
		break;
	case GRPH_CMD_COLOR_FILTER:
		// removed
		break;
	case GRPH_CMD_RGBYUV_BLEND:
	case GRPH_CMD_RGBYUV_COLORKEY:
		do {
			// A, B, C -> B, C
			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_b[id])) {
				if (grph_platform_is_valid_va(vp_image_b[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image B addr: 0x%x\r\n",
						(unsigned int)vp_image_b[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_b[id]->dram_addr)) {
					// Image B is either input or output channel, use clean and invalidate to flush cache
					grph_platform_dma_post_flush_dev2mem(vp_image_b[id]->dram_addr, (vp_image_b[id]->lineoffset * vp_image_a[id]->height));
				}
			}


			if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
				if (grph_platform_is_valid_va(vp_image_c[id]->dram_addr) == 0) {
					DBG_ERR("Invalid image C addr: 0x%x\r\n",
						(unsigned int)vp_image_c[id]->dram_addr);
					return E_NOSPT;
				}
				if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
					PGRPH_PROPERTY      p_property = NULL;
					UINT32              height;

					if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_YUVFMT, &p_property) != E_OK) {
						DBG_ERR("YUV property not found\r\n");
						return E_NOSPT;
					}
					height = (p_property->property == GRPH_YUV_411) ? (vp_image_a[id]->height / 2) : vp_image_a[id]->height;
					// Image B is either input or output channel, use clean and invalidate to flush cache
					grph_platform_dma_post_flush_dev2mem(vp_image_c[id]->dram_addr, (vp_image_c[id]->lineoffset * height));
				}
			}
		} while (0);
		break;
	default:
		return E_NOSPT;
	}

	return E_OK;
}


/*
    Flush cache for GOP operations

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
        - @b E_OK: Done with no errors.
        - @b E_NOSPT: Operation no support.
*/
static ER gop_flush_cache(GRPH_ID id, PGRPH_REQUEST p_request)
{
	UINT32 dst_width;
	UINT32 dst_height;
	UINT32 bit_src = 8;

	if (p_request->command <= GRPH_CMD_ROT_0) {
		switch (p_request->format) {
		case GRPH_FORMAT_1BIT:
			bit_src = 1;
			if (vp_image_a[id]->width & 0x03) {
				DBG_ERR("GOP%d image A width %d should be 4 bytes align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->width);
				return E_NOSPT;
			}
			if (vp_image_a[id]->height & 0x1F) {
				DBG_ERR("GOP%d image A height %d should be 32 lines align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->height);
				return E_NOSPT;
			}
			break;
		case GRPH_FORMAT_8BITS:
			bit_src = 8;
			if (vp_image_a[id]->width & 0x03) {
				DBG_ERR("GOP%d image A width %d should be 4 bytes align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->width);
				return E_NOSPT;
			}
			if (vp_image_a[id]->height & 0x03) {
				DBG_ERR("GOP%d image A height %d should be 4 lines align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->height);
				return E_NOSPT;
			}
			break;
		case GRPH_FORMAT_16BITS:
			bit_src = 16;
			if (vp_image_a[id]->width & 0x03) {
				DBG_ERR("GOP%d image A width %d should be 4 bytes align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->width);
				return E_NOSPT;
			}
			if (vp_image_a[id]->height & 0x01) {
				DBG_ERR("GOP%d image A height %d should be 2 lines align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->height);
				return E_NOSPT;
			}
			break;
		case GRPH_FORMAT_32BITS:
			bit_src = 32;
			if (vp_image_a[id]->width & 0x03) {
				DBG_ERR("GOP%d image A width %d should be 4 bytes align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->width);
				return E_NOSPT;
			}
			// 32 bit height no limitation
			break;
		default:
			DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
			return E_NOSPT;
		}
	}

	switch (p_request->command) {
	case GRPH_CMD_ROT_90:
	case GRPH_CMD_ROT_270:
	case GRPH_CMD_HRZ_FLIP_ROT_90:
	case GRPH_CMD_HRZ_FLIP_ROT_270:
		dst_width = vp_image_a[id]->height * bit_src / 8;
		dst_height = vp_image_a[id]->width * 8 / bit_src;
		break;
	case GRPH_CMD_ROT_180:
	case GRPH_CMD_HRZ_FLIP:
	case GRPH_CMD_VTC_FLIP:
	case GRPH_CMD_ROT_0:
	case GRPH_CMD_VCOV:
	default:
		dst_width = vp_image_a[id]->width;
		dst_height = vp_image_a[id]->height;
		break;
	}

	do {
		if (graph_chk_ptr((UINT32)vp_image_a[id])) {
			if (grph_platform_dma_is_cacheable(vp_image_a[id]->dram_addr)) {
				if (grph_platform_dma_flush_mem2dev(vp_image_a[id]->dram_addr, vp_image_a[id]->lineoffset * vp_image_a[id]->height)) {
					// If clean & invalidate all, skip flush remained image
					break;
				}
			}
		}

		// VCOV may has additional image B
		if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_b[id])) {
			if (grph_platform_dma_is_cacheable(vp_image_b[id]->dram_addr)) {
				UINT32 imgb_height;
				PGRPH_PROPERTY      p_property = 0;
				PGRPH_QUAD_DESC     p_quad_desc;

				// Find quarilateral information
				if (graph_fetch_property(p_request->p_property, GRPH_PROPERTY_ID_QUAD_PTR, &p_property) != E_OK) {
					DBG_ERR("GRPH_PROPERTY_ID_QUAD_PTR property not found\r\n");
					return E_NOSPT;
				}
				if (graph_chk_ptr(p_property->property) == FALSE) {
					DBG_ERR("Input GRPH_QUAD_DESC pointer is not valid 0x%x\r\n",
						(unsigned int)p_property->property);
					return E_NOSPT;
				}
				p_quad_desc = (GRPH_QUAD_DESC *)p_property->property;

				imgb_height = (vp_image_a[id]->height + p_quad_desc->mosaic_height) / p_quad_desc->mosaic_height;

				if (grph_platform_dma_flush_mem2dev(vp_image_b[id]->dram_addr, vp_image_b[id]->lineoffset * imgb_height)) {
					// If clean & invalidate all, skip flush remained image
					break;
				}
			}
		}

		if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
			if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
				BOOL is_flush_all = FALSE;

				// Output image width <--> lineoffset may contain data not allowed to modify by graphic
				// Use clean and invalidate to ensure this region will not be invalidated
				if (dst_width != vp_image_c[id]->lineoffset) {
					is_flush_all = grph_platform_dma_flush_dev2mem_width_neq_loff(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * dst_height);
				} else {
					is_flush_all = grph_platform_dma_flush_dev2mem(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * dst_height);
				}
				if (is_flush_all) {
					break;
				}
			}
		}
	} while (0);

	return E_OK;
}

static ER gop_post_flush_cache(GRPH_ID id, PGRPH_REQUEST p_request)
{
//	UINT32 dst_width;
	UINT32 dst_height;
	UINT32 bit_src = 8;

	if (p_request->command <= GRPH_CMD_ROT_0) {
		switch (p_request->format) {
		case GRPH_FORMAT_1BIT:
			bit_src = 1;
			if (vp_image_a[id]->width & 0x03) {
				DBG_ERR("GOP%d image A width %d should be 4 bytes align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->width);
				return E_NOSPT;
			}
			if (vp_image_a[id]->height & 0x1F) {
				DBG_ERR("GOP%d image A height %d should be 32 lines align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->height);
				return E_NOSPT;
			}
			break;
		case GRPH_FORMAT_8BITS:
			bit_src = 8;
			if (vp_image_a[id]->width & 0x03) {
				DBG_ERR("GOP%d image A width %d should be 4 bytes align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->width);
				return E_NOSPT;
			}
			if (vp_image_a[id]->height & 0x03) {
				DBG_ERR("GOP%d image A height %d should be 4 lines align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->height);
				return E_NOSPT;
			}
			break;
		case GRPH_FORMAT_16BITS:
			bit_src = 16;
			if (vp_image_a[id]->width & 0x03) {
				DBG_ERR("GOP%d image A width %d should be 4 bytes align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->width);
				return E_NOSPT;
			}
			if (vp_image_a[id]->height & 0x01) {
				DBG_ERR("GOP%d image A height %d should be 2 lines align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->height);
				return E_NOSPT;
			}
			break;
		case GRPH_FORMAT_32BITS:
			bit_src = 32;
			if (vp_image_a[id]->width & 0x03) {
				DBG_ERR("GOP%d image A width %d should be 4 bytes align\r\n",
					(int)p_request->command, (int)vp_image_a[id]->width);
				return E_NOSPT;
			}
			// 32 bit height no limitation
			break;
		default:
			DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
			return E_NOSPT;
		}
	}

	switch (p_request->command) {
	case GRPH_CMD_ROT_90:
	case GRPH_CMD_ROT_270:
	case GRPH_CMD_HRZ_FLIP_ROT_90:
	case GRPH_CMD_HRZ_FLIP_ROT_270:
//		dst_width = vp_image_a[id]->height * bit_src / 8;
		dst_height = vp_image_a[id]->width * 8 / bit_src;
		break;
	case GRPH_CMD_ROT_180:
	case GRPH_CMD_HRZ_FLIP:
	case GRPH_CMD_VTC_FLIP:
	case GRPH_CMD_ROT_0:
	case GRPH_CMD_VCOV:
	default:
//		dst_width = vp_image_a[id]->width;
		dst_height = vp_image_a[id]->height;
		break;
	}

	do {
		if (graph_chk_ptr((UINT32)vp_image_a[id]) && graph_chk_ptr((UINT32)vp_image_c[id])) {
			if (grph_platform_dma_is_cacheable(vp_image_c[id]->dram_addr)) {
//				DBG_WRN("%s: invalidate after dma end\r\n", __func__);
				grph_platform_dma_post_flush_dev2mem(vp_image_c[id]->dram_addr, vp_image_c[id]->lineoffset * dst_height);

			}
		}
	} while (0);

	return E_OK;
}


#if 0
/*
    Graphic isr vcov handler

    Video Cover bottom half handler in ISR.

    @return
	- @b 0: all slice done
	- @b -1: something error
	- @b 1: next slice settled
*/
static INT32 graph_isr_vcov(GRPH_ID id)
{
	T_GRPH_CTRL_REG     ctrl_reg = {0};
	GRPH_IMG img_c = {GRPH_IMG_ID_C, 0, 0, 0, 0, NULL, NULL};
        GRPH_IMG img_a = {GRPH_IMG_ID_A, 0, 0, 0, 0, NULL, &img_c};
	UINT32 width_slice;
	ER ret;

//	printk("%s: remain %d\r\n", __func__, v_vcov_context[id].remain_width);

	if (v_vcov_context[id].remain_width == 0) return 0;

	// HW line buffer is 4k byte
	if (v_vcov_context[id].fmt == GRPH_FORMAT_8BITS) { /* If 8 bit, hw limit width 4k pixels */
		if (v_vcov_context[id].remain_width > 4096) {
			width_slice = (4096 / v_vcov_context[id].mosaic_width) * v_vcov_context[id].mosaic_width;
		} else {
			width_slice = v_vcov_context[id].remain_width;
		}
	} else {                                    // if 16 bit, hw limit width 2k pixels
		if (v_vcov_context[id].remain_width > 2048) {
			width_slice = (2048 / v_vcov_context[id].mosaic_width) * v_vcov_context[id].mosaic_width;
			DBG_IND("Cut slice width to %d\r\n", width_slice);
		} else {
			width_slice = v_vcov_context[id].remain_width;
		}
	}

	img_a.dram_addr = v_vcov_context[id].img_a.dram_addr +
				v_vcov_context[id].x_coord_ofs;
	img_a.lineoffset = v_vcov_context[id].img_a.lineoffset;
	img_a.width = width_slice;
	img_a.height = v_vcov_context[id].img_a.height;
	img_c.dram_addr = v_vcov_context[id].img_c.dram_addr +
				v_vcov_context[id].x_coord_ofs;
	img_c.lineoffset = v_vcov_context[id].img_c.lineoffset;
	img_a.p_next = &img_c;

	ret = graph_handle_images(id, &img_a);
	if (ret != E_OK) {
		DBG_ERR("invalid pointer in input images\r\n");
		return -1;
	}

	adjust_vcov_origin(id, (INT32)v_vcov_context[id].slice_ofs);

	DBG_IND("Remain %d, slice %d\r\n",
		v_vcov_context[id].remain_width,
		width_slice);

	v_vcov_context[id].x_coord_ofs += width_slice;
	v_vcov_context[id].remain_width -= width_slice;
	v_vcov_context[id].slice_ofs = width_slice;


	// Must clear IN/OUT to default values
	graph_set_reg(id, GRPH_IN1_REG_OFS, 0);
	graph_set_reg(id, GRPH_IN2_REG_OFS, 0);
	graph_set_reg(id, GRPH_OUT3_REG_OFS, 0);
	graph_set_reg(id, GRPH_IN1_2_REG_OFS, 0);
	graph_set_reg(id, GRPH_IN2_2_REG_OFS, 0);
	graph_set_reg(id, GRPH_OUT3_2_REG_OFS, 0);


	// Trigger next slice
	ctrl_reg.bit.TRIG_OP = TRUE;
	graph_set_reg(id, GRPH_CTRL_REG_OFS, ctrl_reg.reg);

	return 1;
}
#endif

void grph_isr_bottom(GRPH_ID id, UINT32 events)
{
	GRPH_REQ_LIST_NODE *p_node;
	PGRPH_REQUEST p_request = NULL;
	BOOL is_list_empty;
	UINT32 spin_flags;

        p_node = grph_platform_get_head(id);
        if (p_node == NULL) {
                DBG_ERR("%s: get head fail\r\n", __func__);
                return;
        }

        p_request = (PGRPH_REQUEST)(&(p_node->v_ll_req[0].trig_param));
        request_post_handler(id, p_request);

        if (p_node->callback.callback == NULL) {
//                grph_platform_flg_set(id, v_isr_flag[id]);
        } else {
                // HH note: future should consider UV dual ACC result
		T_GRPH_ACCU_REG     acc1_reg;

		acc1_reg.reg = graph_get_reg(id, GRPH_ACCU_REG_OFS);

                p_node->cb_info.timestamp = hwclock_get_counter();
//                p_node->cb_info.timestamp = jiffies;
                p_node->cb_info.acc_result = acc1_reg.bit.ACC_RESULT;
                p_node->callback.callback(&p_node->cb_info, NULL);
        }


	// Enter critical section
        spin_flags = grph_platform_spin_lock(id);

        grph_platform_del_list(id);
	is_list_empty = grph_platform_list_empty(id);
	if (is_list_empty == FALSE) {
		graph_trigger(id);
	}

	// Leave critical section
	grph_platform_spin_unlock(id, spin_flags);

	if (p_node->callback.callback == NULL) {
		grph_platform_flg_set(id, v_isr_flag[id]);
	}

#if 0
	if (is_list_empty == FALSE) {
                // Not empty => process next node

#if 0           // debug
                GRPH_REQ_LIST_NODE *p_node = NULL;

                p_node = grph_platform_get_head(id);
                if (p_node->callback.callback != NULL) {
                        printk("%s %d: callback not NULL, cmd 0x%x\r\n",
				__func__, id, p_node->trig_param.command);
                } else {
                        printk("%s %d: callback NULL, cmd 0x%x\r\n",
				__func__, id, p_node->trig_param.command);
		}
#endif

                graph_trigger(id);
        } else {
#if 0		// debug
		printk("%s %d: last Queue, cmd 0x%x\r\n",
			__func__, id, p_node->trig_param.command);
#endif
	}
#endif
}

/*
    Graphic interrupt handler

    Disable status interrupt and set flag
    This is internal API.

    @return void
*/
void graph_isr(void)
{
	T_GRPH_INTSTS_REG sts;
	T_GRPH_INTEN_REG int_en;

	sts.reg = apb_get_reg(GRPH_ID_1, GRPH_INTSTS_REG_OFS);
	int_en.reg = apb_get_reg(GRPH_ID_1, GRPH_INTEN_REG_OFS);
	sts.reg &= int_en.reg;
	if (sts.reg == 0) {
		return;
	}

	// Clear interrupt status
//	sts.bit.INT_STS = 1;
	apb_set_reg(GRPH_ID_1, GRPH_INTSTS_REG_OFS, sts.reg);

	v_engine_status[GRPH_ID_1] = GRPH_ENGINE_READY;

//	grph_platform_isr_bottom(GRPH_ID_1, 0);

	grph_platform_set_ist_event(GRPH_ID_1);
//	grph_platform_flg_set(GRPH_ID_1, FLGPTN_GRAPHIC);
//	DBG_IND("done\r\n");
//	iset_flg(FLG_ID_GRAPHIC, (FLGPTN)FLGPTN_GRAPHIC);
}

/*
    Graphic 2 interrupt handler

    Disable status interrupt and set flag
    This is internal API.

    @return void
*/
void graph2_isr(void)
{
	T_GRPH_INTSTS_REG sts;
	T_GRPH_INTEN_REG int_en;

//dump_stack();

	sts.reg = apb_get_reg(GRPH_ID_2, GRPH_INTSTS_REG_OFS);
	int_en.reg = apb_get_reg(GRPH_ID_2, GRPH_INTEN_REG_OFS);
//	printk("%s: sts 0x%x\r\n", __func__, sts.reg);
	sts.reg &= int_en.reg;
	if (sts.reg == 0) {
		return;
	}

	// Clear interrupt status
//	sts.bit.INT_STS = 1;
	apb_set_reg(GRPH_ID_2, GRPH_INTSTS_REG_OFS, sts.reg);

    if (1) {
//	if (graph_isr_vcov(GRPH_ID_2) != 1) {
		// all vcov is completed or something error
		// => let user or callback know job complete
//	DBG_IND("branch\r\n");
		v_engine_status[GRPH_ID_2] = GRPH_ENGINE_READY;
//		grph_platform_isr_bottom(GRPH_ID_2, 0);
//		grph_platform_flg_set(GRPH_ID_2, FLGPTN_GRAPHIC2);
		grph_platform_set_ist_event(GRPH_ID_2);
	}

//	v_engine_status[GRPH_ID_2] = GRPH_ENGINE_READY;

//	grph_platform_flg_set(GRPH_ID_2, FLGPTN_GRAPHIC2);
//	DBG_IND("done\r\n");
//	iset_flg(FLG_ID_GRAPHIC2, (FLGPTN)FLGPTN_GRAPHIC2);
}

/*
    Lock Graphic

    This function lock 2D Graphic module
    This is internal API.

    @return
	- @b E_OK: Done with no error.
	- Others: Error occured.
*/
static ER graph_lock(GRPH_ID id)
{
	ER ret;

	if (id > GRPH_ID_2) {
		return E_NOSPT;
	}

	ret = grph_platform_sem_wait(id);

	if (ret != E_OK) {
		return ret;
	}

	return E_OK;
}

/*
    Un-Lock Graphic

    This function unlock 2D Graphic module
    This is internal API.

    @param[in] id               graphic controller identifier

    @return
	- @b E_OK: Done with no error.
	- Others: Error occured.
*/
static ER graph_unlock(GRPH_ID id)
{
	if (id > GRPH_ID_2) {
		return E_NOSPT;
	}

//    vGrphLockStatus[id] = NO_TASK_LOCKED;
	return grph_platform_sem_signal(id);
}

#if 0
#pragma mark -
#endif

/*
    grph attach.

    attach grph.

    @param[in] id               graphic controller identifier

    @return
	- @b E_OK: Done with no error.
	- Others: Error occured.
*/
static ER graph_attach(GRPH_ID id)
{
	if (id > GRPH_ID_2) {
		return E_NOSPT;
	}

	// Enable Graphic Clock
	grph_platform_clk_enable(id);

	return E_OK;
}

/*
    grph detach.

    detach grph.

    @param[in] id               graphic controller identifier

    @return
	- @b E_OK: Done with no error.
	- Others: Error occured.
*/
static ER graph_detach(GRPH_ID id)
{
	if (id > GRPH_ID_2) {
		return E_NOSPT;
	}

	// Disable Graphic Clock
	grph_platform_clk_disable(id);

	return E_OK;
}

/**
    Open graphic module

    Enable Graphic system interrupt and define interrupt handler call-back function.
    grph_attach() must be called before this api to enable graphic clock.

    @param[in] id               graphic controller identifier

    @return
	- @b E_OK: Done with no error.
	- Others: Error occured.
*/
ER graph_open(GRPH_ID id)
{
	T_GRPH_INTSTS_REG sts = {0};
	T_GRPH_INTEN_REG int_en = {0};
	ER ret;

	if (id > GRPH_ID_2) {
		return E_NOSPT;
	}

	// Enter critical section
	ret = grph_platform_oc_sem_wait(id);
	if (ret != E_OK) {
		DBG_ERR("%s: %d lock fail\r\n", __func__, id);
		return ret;
	}

	if (++v_lock_status[id] > 1) {
		// Leave critical section
		ret = grph_platform_oc_sem_signal(id);
		if (ret != E_OK) {
			DBG_ERR("%s: %d unlock fail\r\n", __func__, id);
			return ret;
		}
		return E_OK;
	}

	p_grph_ll_buffer[id] = &grph_ll_buffer[id][0];
	apb_set_reg(id, LL_DMA_ADDR_REG_OFS,
			grph_platform_va2pa((UINT32)p_grph_ll_buffer[id]));

	graph_reset_reg_cache(id);

	vp_image_a[id] = vp_image_b[id] = vp_image_c[id] = NULL;

	v_engine_status[id] = GRPH_ENGINE_READY;

	// Turn on power
	grph_platform_sram_enable(id);
	if (id == GRPH_ID_1) {
//		pll_disableSramShutDown(GRAPH_RSTN);
//		pmc_turnonPower(PMC_MODULE_GRAPHIC);
	} else {
//		pll_disableSramShutDown(GRAPH2_RSTN);
//		pmc_turnonPower(PMC_MODULE_GRAPHIC2);
	}

	// Select clock source
	grph_platform_clk_set_freq(id, v_grph_freq[id]);

	// Add the grph_attach as patch to backward compatible
	graph_attach(id);

	// Clear Interrupt Status
	sts.bit.INT_STS = 1;
	apb_set_reg(id, GRPH_INTSTS_REG_OFS, sts.reg);

	// Enable Interrupt
	if (grph_reg_domain[id] == GRPH_REG_DOMAIN_APB) {
		int_en.bit.INT_EN = 1;
	} else {
		int_en.bit.LLEND_INT_EN = 1;
		int_en.bit.LLERR_INT_EN = 1;
	}
	apb_set_reg(id, GRPH_INTEN_REG_OFS, int_en.reg);

	// Enable Graphic interrupt
	grph_platform_int_enable(id);

	// Leave critical section
	ret = grph_platform_oc_sem_signal(id);
	if (ret != E_OK) {
		DBG_ERR("%s: %d unlock fail\r\n", __func__, id);
		return ret;
	}

	return E_OK;
}
EXPORT_SYMBOL(graph_open);

/**
    check grph is opened.

    check if graphic module is opened.

    @param[in] id               graphic controller identifier

    @return
	- @b TRUE: grph is open.
	- @b FALSE: grph is closed.
*/
BOOL graph_is_opened(GRPH_ID id)
{
	if (id > GRPH_ID_2) {
		return FALSE;
	}

	return (v_lock_status[id] != 0);
}
EXPORT_SYMBOL(graph_is_opened);

/**
    Close Graphic module

    Close the graphic system interrupt and disable the clock.
    grph_detach() should be called after this api to turn-off graphic clock.

    @param[in] id               graphic controller identifier

    @return
	- @b E_OK: Done with no error.
	- Others: Error occured.
*/
ER graph_close(GRPH_ID id)
{
	ER ret;

	if (id > GRPH_ID_2) {
		return E_NOSPT;
	}

	// Enter critical section
	ret = grph_platform_oc_sem_wait(id);
	if (ret != E_OK) {
		DBG_ERR("%s: %d lock fail\r\n", __func__, id);
		return ret;
	}

	if (v_lock_status[id] == 0) {
		// Leave critical section
		ret = grph_platform_oc_sem_signal(id);
		if (ret != E_OK) {
			DBG_ERR("%s: %d unlock fail\r\n", __func__, id);
			return ret;
		}
		return E_OK;
	} else if (--v_lock_status[id] != 0) {
		// Leave critical section
		ret = grph_platform_oc_sem_signal(id);
		if (ret != E_OK) {
			DBG_ERR("%s: %d unlock fail\r\n", __func__, id);
			return ret;
		}
		return E_OK;
	}

	// Release interrupt
	grph_platform_int_disable(id);

	v_engine_status[id] = GRPH_ENGINE_IDLE;

	// Add the grph_detach as patch to backward compatible
	graph_detach(id);

#if ((_EMULATION_ == ENABLE) && (_FPGA_EMULATION_ == DISABLE))
	// Real chip verification, try to turn off power for PMC verification
	// Turn off power
/*
	if (id == GRPH_ID_1) {
		pmc_turnoffPower(PMC_MODULE_GRAPHIC);
		pll_enableSramShutDown(GRAPH_RSTN);
	} else {
		pmc_turnoffPower(PMC_MODULE_GRAPHIC2);
		pll_enableSramShutDown(GRAPH2_RSTN);
	}
*/
#endif
	// Leave critical section
	ret = grph_platform_oc_sem_signal(id);
	if (ret != E_OK) {
		DBG_ERR("%s: %d unlock fail\r\n", __func__, id);
		return ret;
	}

	return E_OK;
}
EXPORT_SYMBOL(graph_close);

/**
    Set graphic configuration

    @param[in] id       controller identifier
    @param[in] config       configuration identifier
    @param[in] setting      configuration context for config

    @return
	- @b E_OK: set configuration success
	- @b E_NOSPT: config is not supported
*/
ER graph_set_config(GRPH_ID id, GRPH_CONFIG_ID config, UINT32 setting)
{
	if (id > GRPH_ID_2) {
		return E_NOSPT;
	}

	switch ((UINT32)config) {
	case GRPH_CONFIG_ID_FREQ:
		if (setting < 240) {
			DBG_WRN("%d: input frequency %d round to 240MHz\r\n", (int)id, (int)setting);
			setting = 240;
		} else if (setting == 240) {
			setting = 240;
		} else if (setting < 320) {
			DBG_WRN("%d: input frequency %d round to 240MHz\r\n", (int)id, (int)setting);
			setting = 240;
		} else if (setting == 320) {
			setting = 320;
		} else if (setting < 360) {
			DBG_WRN("%d: input frequency %d round to 320MHz\r\n", (int)id, (int)setting);
			setting = 320;
		} else if (setting == 360) {
			setting = 360;
		} else if (setting < 480) {
			DBG_WRN("%d: input frequency %d round to 360MHz(PLL13)\r\n", (int)id, (int)setting);
			setting = 360;
		} else if (setting == 480) {
			setting = 480;
		} else {
			DBG_WRN("%d: input frequency %d round to 480MHz\r\n", (int)id, (int)setting);
			setting = 480;
		}

		v_grph_freq[id] = setting;
		break;
#if (_EMULATION_ == ENABLE)
	case GRPH_PROT_CONFIG_ID_DIRECT_COMPARATIVE:
		switch (setting) {
		case 0:
			g_graph_comparative = GRPH_VDOCOV_COMPARATIVE_GTLT;
			break;
		case 1:
			g_graph_comparative = GRPH_VDOCOV_COMPARATIVE_GTLE;
			break;
		case 2:
			g_graph_comparative = GRPH_VDOCOV_COMPARATIVE_GELT;
			break;
		case 3:
		default:
			g_graph_comparative = GRPH_VDOCOV_COMPARATIVE_GELE;
			break;
		}
		break;
	case GRPH_PROT_CONFIG_ID_DIRECT_VCOV:
		if (setting == TRUE) {
			b_lineparam_directpath = TRUE;
		} else {
			b_lineparam_directpath = FALSE;
		}
		break;
	case GRPH_PROT_CONFIG_ID_DIRECT_LINE0_REG1:
		vcov_line0reg1 = setting;
		break;
	case GRPH_PROT_CONFIG_ID_DIRECT_LINE0_REG2:
		vcov_line0reg2 = setting;
		break;
	case GRPH_PROT_CONFIG_ID_DIRECT_LINE1_REG1:
		vcov_line1reg1 = setting;
		break;
	case GRPH_PROT_CONFIG_ID_DIRECT_LINE1_REG2:
		vcov_line1reg2 = setting;
		break;
	case GRPH_PROT_CONFIG_ID_DIRECT_LINE2_REG1:
		vcov_line2reg1 = setting;
		break;
	case GRPH_PROT_CONFIG_ID_DIRECT_LINE2_REG2:
		vcov_line2reg2 = setting;
		break;
	case GRPH_PROT_CONFIG_ID_DIRECT_LINE3_REG1:
		vcov_line3reg1 = setting;
		break;
	case GRPH_PROT_CONFIG_ID_DIRECT_LINE3_REG2:
		vcov_line3reg2 = setting;
		break;
#endif
	default:
		return E_NOSPT;
	}

	return E_OK;
}
EXPORT_SYMBOL(graph_set_config);

/**
    Get graphic configuration

    @param[in] id       controller identifier
    @param[in] config       configuration identifier
    @param[out] p_context   configuration context for config

    @return
	- @b E_OK: set configuration success
	- @b E_NOSPT: config is not supported
*/
ER graph_get_config(GRPH_ID id, GRPH_CONFIG_ID config, UINT32 *p_context)
{
	if (id > GRPH_ID_2) {
		return E_NOSPT;
	}
	if (config != GRPH_CONFIG_ID_FREQ) {
		return E_NOSPT;
	}

	grph_platform_clk_get_freq(id, p_context);

	return E_OK;
}
EXPORT_SYMBOL(graph_get_config);

#if 0
#pragma mark -
#endif

/*
    Graphic GOP request

    This is generic version that can support GOP supported by graphic engine.
    The destination buffer is fixed to Image3 (i.e. Image C).

    @param[in] id       graphic controller identifier
    @param[in] p_request    Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER graph_request_gop(GRPH_ID id, PGRPH_REQUEST p_request)
{
	UINT32              bit_src;
	T_GRPH_CONFIG_REG   config_reg = {0};
	ER                  ret = E_OK;

	if (id != GRPH_ID_2) {
		DBG_ERR("graphic ID 0x%x not support GOP\r\n", (unsigned int)id);
		return E_NOSPT;
	}
	if (p_request->command > GRPH_CMD_VCOV) {
		DBG_ERR("unsupported GOP: 0x%x\r\n", (unsigned int)p_request->command);
		return E_NOSPT;
	}

	if (vp_image_a[id]->dram_addr & 0x03) {
		DBG_ERR("GOP%d image A address 0x%x should be word align\r\n", (int)p_request->command,
			(unsigned int)vp_image_a[id]->dram_addr);
		return E_NOSPT;
	}
	if (vp_image_c[id]->dram_addr & 0x03) {
		DBG_ERR("GOP%d imageC address 0x%x should be word align\r\n", (int)p_request->command,
			(unsigned int)vp_image_c[id]->dram_addr);
		return E_NOSPT;
	}

	switch (p_request->format) {
	case GRPH_FORMAT_1BIT:
		config_reg.bit.OP_PRECISION = OP_PRECISION_1BIT;
		bit_src = 1;
		break;
	case GRPH_FORMAT_8BITS:
		config_reg.bit.OP_PRECISION = OP_PRECISION_8BITS;
		bit_src = 8;
		break;
	case GRPH_FORMAT_16BITS:
		config_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		bit_src = 16;
		break;
	case GRPH_FORMAT_32BITS:
		config_reg.bit.OP_PRECISION = OP_PRECISION_32BITS;
		bit_src = 32;
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}
	graph_autoCalcGOP(id, bit_src, p_request->command);

	// Flush cache
	if ((p_request->is_skip_cache_flush == FALSE) &&
		(p_request->is_buf_pa == FALSE)) {
		ret = gop_flush_cache(id, p_request);
		if (ret != E_OK) {
			return ret;
		}
	}

	config_reg.bit.GE_MODE = p_request->command;
	config_reg.bit.OP_TYPE = OP_TYPE_GOP;
	graph_set_reg(id, GRPH_CONFIG_REG_OFS, config_reg.reg);

	// Must clear IN/OUT to default values
	graph_set_reg(id, GRPH_IN1_REG_OFS, 0);
	graph_set_reg(id, GRPH_IN2_REG_OFS, 0);
	graph_set_reg(id, GRPH_OUT3_REG_OFS, 0);
	graph_set_reg(id, GRPH_IN1_2_REG_OFS, 0);
	graph_set_reg(id, GRPH_IN2_2_REG_OFS, 0);
	graph_set_reg(id, GRPH_OUT3_2_REG_OFS, 0);

	switch (p_request->command) {
	case GRPH_CMD_VCOV:
		graphic_wrap_vcov(id, p_request);
		break;
	default:
		graph_reg2ll(id);
        // HH: linux KDRV not support APB trigger
//		trigger_wait_single_frame(id);
		break;
	}

	return E_OK;
}

/*
    Graphic AOP request

    This is generic version that can support AOP supported by graphic engine.
    The destination buffer is fixed to Image3 (i.e. Image C).

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
*/
static ER graph_request_aop(GRPH_ID id, PGRPH_REQUEST p_request)
{
//	FLGPTN              uiFlag;
	GRPH_CMD            command;
	ER                  ret = E_OK;
//	T_GRPH_CTRL_REG     ctrl_reg = {0};
	T_GRPH_CONFIG_REG   cfg_reg = {0};

	if (id == GRPH_ID_1) {
		if (p_request->command < GRPH_CMD_A_COPY) {
			DBG_ERR("GRPH ID %d NOT support GOP 0x%x\r\n",
				(int)id, (unsigned int)p_request->command);
			return E_NOSPT;
		}
	} else {
		switch (p_request->command) {
		case GRPH_CMD_VCOV:
		case GRPH_CMD_A_COPY:
		case GRPH_CMD_COLOR_EQ:
		case GRPH_CMD_COLOR_LE:
		case GRPH_CMD_COLOR_MR:
		case GRPH_CMD_TEXT_COPY:
		case GRPH_CMD_BLENDING:
			break;
		default:
			DBG_ERR("GRPH ID %d NOT support AOP 0x%x\r\n",
				(int)id, (unsigned int)p_request->command);
			return E_NOSPT;
		}
	}

	switch (p_request->format) {
	case GRPH_FORMAT_8BITS:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_8BITS;
		break;
	case GRPH_FORMAT_16BITS:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		cfg_reg.bit.FORMAT_16BIT = FORMAT_16BIT_NORMAL;
		break;
	case GRPH_FORMAT_16BITS_UVPACK:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		cfg_reg.bit.FORMAT_16BIT = FORMAT_16BIT_UVPACK;
		cfg_reg.bit.UVPACK_ATTRIB = UVPACK_ATTRIB_BOTH;
		break;
	case GRPH_FORMAT_16BITS_UVPACK_U:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		cfg_reg.bit.FORMAT_16BIT = FORMAT_16BIT_UVPACK;
		cfg_reg.bit.UVPACK_ATTRIB = UVPACK_ATTRIB_U;
		break;
	case GRPH_FORMAT_16BITS_UVPACK_V:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		cfg_reg.bit.FORMAT_16BIT = FORMAT_16BIT_UVPACK;
		cfg_reg.bit.UVPACK_ATTRIB = UVPACK_ATTRIB_V;
		break;
	case GRPH_FORMAT_16BITS_RGB565:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		cfg_reg.bit.FORMAT_16BIT = FORMAT_16BIT_RGBPACK;
		break;
	case GRPH_FORMAT_32BITS_ARGB8888_RGB:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_32BITS;
		cfg_reg.bit.FORMAT_32BIT = FORMAT_32BIT_ARGB8888;
		cfg_reg.bit.ARGBPACK_ATTRIB = ARGB_ATTRIB_RGB;
		break;
	case GRPH_FORMAT_32BITS_ARGB8888_A:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_32BITS;
		cfg_reg.bit.FORMAT_32BIT = FORMAT_32BIT_ARGB8888;
		cfg_reg.bit.ARGBPACK_ATTRIB = ARGB_ATTRIB_A;
		break;
	case GRPH_FORMAT_16BITS_ARGB1555_RGB:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		cfg_reg.bit.FORMAT_16BIT = FORMAT_16BIT_ARGB1555;
		cfg_reg.bit.ARGBPACK_ATTRIB = ARGB_ATTRIB_RGB;
		break;
	case GRPH_FORMAT_16BITS_ARGB1555_A:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		cfg_reg.bit.FORMAT_16BIT = FORMAT_16BIT_ARGB1555;
		cfg_reg.bit.ARGBPACK_ATTRIB = ARGB_ATTRIB_A;
		break;
	case GRPH_FORMAT_16BITS_ARGB4444_RGB:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		cfg_reg.bit.FORMAT_16BIT = FORMAT_16BIT_ARGB4444;
		cfg_reg.bit.ARGBPACK_ATTRIB = ARGB_ATTRIB_RGB;
		break;
	case GRPH_FORMAT_16BITS_ARGB4444_A:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_16BITS;
		cfg_reg.bit.FORMAT_16BIT = FORMAT_16BIT_ARGB4444;
		cfg_reg.bit.ARGBPACK_ATTRIB = ARGB_ATTRIB_A;
		break;
	case GRPH_FORMAT_PALETTE_1BIT:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_PALETTE;
		cfg_reg.bit.PALETTE_FORMAT = FORMAT_PAL_ENUM_1BIT;
		break;
	case GRPH_FORMAT_PALETTE_2BITS:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_PALETTE;
		cfg_reg.bit.PALETTE_FORMAT = FORMAT_PAL_ENUM_2BITS;
		break;
	case GRPH_FORMAT_PALETTE_4BITS:
		cfg_reg.bit.OP_PRECISION = OP_PRECISION_PALETTE;
		cfg_reg.bit.PALETTE_FORMAT = FORMAT_PAL_ENUM_4BITS;
		break;
	default:
		DBG_ERR("unsupported request format 0x%x\r\n", (unsigned int)p_request->format);
		return E_NOSPT;
	}

	command = p_request->command;
	switch (command) {
	case GRPH_CMD_A_COPY:
		switch (p_request->format) {
		case GRPH_FORMAT_8BITS:
		case GRPH_FORMAT_32BITS_ARGB8888_RGB:
		case GRPH_FORMAT_32BITS_ARGB8888_A:
		case GRPH_FORMAT_16BITS_ARGB1555_RGB:
		case GRPH_FORMAT_16BITS_ARGB1555_A:
		case GRPH_FORMAT_16BITS_ARGB4444_RGB:
		case GRPH_FORMAT_16BITS_ARGB4444_A:
			break;
		default:
			DBG_ERR("A_COPY only support 8bits format: 0x%x\r\n", (unsigned int)p_request->format);
			return E_NOSPT;
		}
		break;
	case GRPH_CMD_PLUS_SHF:
	case GRPH_CMD_MINUS_SHF:
		// support IN/OUT
		ret = aop_mpshf(id, p_request);
		break;
	case GRPH_CMD_MINUS_SHF_ABS:
		// support IN/OUT
		command = GRPH_CMD_MINUS_SHF;
		ret = aop_abs_minus_shf(id, p_request);
		break;
	case GRPH_CMD_COLOR_EQ:
	case GRPH_CMD_COLOR_LE:
		ret = aop_colorkey(id, p_request);
		break;
	case GRPH_CMD_COLOR_MR:
		command = GRPH_CMD_COLOR_LE;
		ret = aop_colorkey_ge(id, p_request);
		break;
	case GRPH_CMD_COLOR_FILTER:
		command = GRPH_CMD_COLOR_LE;
		ret = aop_color_filter(id, p_request);
		break;
	case GRPH_CMD_A_AND_B:
	case GRPH_CMD_A_OR_B:
	case GRPH_CMD_A_XOR_B:
		if (p_request->format != GRPH_FORMAT_8BITS) {
			DBG_ERR("AOP 0x%x only support 8bits format: 0x%x\r\n", (unsigned int)command,
				(unsigned int)p_request->format);
			return E_NOSPT;
		}
		break;
	case GRPH_CMD_TEXT_COPY:
	case GRPH_CMD_TEXT_AND_A:
	case GRPH_CMD_TEXT_OR_A:
	case GRPH_CMD_TEXT_XOR_A:
	case GRPH_CMD_TEXT_AND_AB:
		ret = aop_text(id, p_request);
		break;
	case GRPH_CMD_BLENDING:
		ret = aop_blend(id, p_request);
		break;
	case GRPH_CMD_ACC:
		ret = aop_acc(id, p_request);
		break;
	case GRPH_CMD_MULTIPLY_DIV:
		ret = aop_mult(id, p_request);
		break;
	case GRPH_CMD_PACKING:
		break;
	case GRPH_CMD_DEPACKING:
		if (vp_image_b[id]->dram_addr & 0x3) {
			DBG_ERR("AOP%d image B address 0x%x should be 4 byte align\r\n",
				(int)p_request->command,
				(unsigned int)vp_image_b[id]->dram_addr);
			return E_NOSPT;
		}
		if (vp_image_c[id]->dram_addr & 0x3) {
			DBG_ERR("AOP%d image C address 0x%x should be 4 byte align\r\n",
				(int)p_request->command,
				(unsigned int)vp_image_c[id]->dram_addr);
			return E_NOSPT;
		}
		break;
	case GRPH_CMD_TEXT_MUL:
		ret = aop_text_mult(id, p_request);
		break;
	case GRPH_CMD_PLANE_BLENDING:
		ret = aop_alpha_plane(id, p_request);
		break;
	case GRPH_CMD_1D_LUT:
		ret = aop_1d_lut(id, p_request);
		break;
	case GRPH_CMD_2D_LUT:
		ret = aop_2d_lut(id, p_request);
		break;
	case GRPH_CMD_RGBYUV_BLEND:
		ret = aop_rgb_yuv_blend(id, p_request);
		break;
	case GRPH_CMD_RGBYUV_COLORKEY:
		ret = aop_rgb_yuv_colorkey(id, p_request);
		break;
	case GRPH_CMD_RGB_INVERT:
		ret = aop_rgb_invert(id, p_request);
		break;
	default:
		DBG_ERR("unsupported AOP: 0x%x\r\n", (unsigned int)p_request->command);
		return E_NOSPT;
	}
	if (ret != E_OK) {
		return ret;
	}
	if ((p_request->is_skip_cache_flush == FALSE) &&
		(p_request->is_buf_pa == FALSE)) {
		ret = aop_flush_cache(id, p_request);
		if (ret != E_OK) {
			return ret;
		}
	}

	cfg_reg.bit.AOP_MODE = command - 0x10000;
	cfg_reg.bit.OP_TYPE = OP_TYPE_AOP;
	graph_set_reg(id, GRPH_CONFIG_REG_OFS, cfg_reg.reg);

//	grph_platform_flg_clear(id, v_isr_flag[id]);
//	clr_flg(vGraphFlgId[id], vGraphFlag[id]);

//printk("%s: grph %d trig\r\n", __func__, id);
//	ctrl_reg.bit.TRIG_OP = TRUE;
//	graph_set_reg(id, GRPH_CTRL_REG_OFS, ctrl_reg.reg);
	graph_reg2ll(id);
	// HH: linux KDRV not support APB trigger
//	trigger_wait_single_frame(id);

	return E_OK;
}

/*
    Graphic request post handler

    Handle misc jobs after link list dma is done

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
        - @b E_OK: Done with no errors.
        - @b E_NOSPT: Operation no support.
*/
static ER request_post_handler(GRPH_ID id, PGRPH_REQUEST p_request)
{
	GRPH_REQUEST *p_next_req = p_request;
	UINT32 idx_req = 0;

	do {
		vp_image_a[id] = vp_image_b[id] = vp_image_c[id] = NULL;
		graph_postHandleImageList(id, p_next_req->p_images);
		if ((p_request->is_skip_cache_flush == FALSE) &&
			(p_request->is_buf_pa == FALSE)) {
			if (p_next_req->command < GRPH_CMD_GOPMAX) {
				gop_post_flush_cache(id, p_next_req);
			} else {
				aop_post_flush_cache(id, p_next_req);
			}
		}

		// check image parameters
		if (p_next_req->command == GRPH_CMD_ACC) {
			PGRPH_PROPERTY      p_property = 0;
			UINT32 *p_acc = (UINT32*)(&grph_acc_buffer[idx_req*ACC_BUF_UNIT+0]);

			grph_platform_dma_post_flush_dev2mem((UINT32)p_acc, 8);

			DBG_IND("%s: word0 0x%x, word1 0x%x\r\n", __func__,
				(unsigned int)p_acc[0],
				(unsigned int)p_acc[1]);
			DBG_IND("%s: addr word0 0x%p, word1 0x%p\r\n", __func__, &p_acc[0], &p_acc[1]);

			if (graph_fetch_property(p_next_req->p_property, GRPH_PROPERTY_ID_ACC_RESULT, &p_property) == E_OK) {
				p_property->property = p_acc[0];
			}

			if (graph_fetch_property(p_next_req->p_property, GRPH_PROPERTY_ID_ACC_RESULT2, &p_property) == E_OK) {
				p_property->property = p_acc[1];
			}
		}

		p_next_req = p_next_req->p_next;
		idx_req++;
		if (idx_req == GRPH_LL_MAX_COUNT) {
			DBG_WRN("max support link list count is %d, input exceed\r\n",
				(int)GRPH_LL_MAX_COUNT);
			break;
		}
	} while (p_next_req != NULL);

	return E_OK;
}

/*
    ACC result post handler

    Used for block mode (versus non-blocking mode)
    Get result from ISR bottom half, and fill it to structure from caller.

    @param[in] id           graphic controller identifier
    @param[in] p_request    Description of request passed from caller
    @param[in] p_queued     Description of request handled by bottom half

    @return void
*/
static void post_handle_acc(GRPH_ID id, PGRPH_REQUEST p_request, PGRPH_REQUEST p_queued)
{
	GRPH_REQUEST *p_next_caller = p_request;
	GRPH_REQUEST *p_next_queued = p_queued;

	do {
		if (p_next_caller && p_next_queued &&
			p_next_caller->command == GRPH_CMD_ACC) {
			ER ret1, ret2;
			PGRPH_PROPERTY      p_property = 0;
			PGRPH_PROPERTY      p_ll_property = 0;

			ret1 = graph_fetch_property(p_next_caller->p_property,
				GRPH_PROPERTY_ID_ACC_RESULT,
				&p_property);
			ret2 = graph_fetch_property(p_next_queued->p_property,
				GRPH_PROPERTY_ID_ACC_RESULT,
				&p_ll_property);

			if ((ret1==E_OK) && (ret2==E_OK)) {
				p_property->property = p_ll_property->property;
			}

			ret1 = graph_fetch_property(p_next_caller->p_property,
				GRPH_PROPERTY_ID_ACC_RESULT2,
				&p_property);
			ret2 = graph_fetch_property(p_next_queued->p_property,
				GRPH_PROPERTY_ID_ACC_RESULT2,
				&p_ll_property);

			if ((ret1==E_OK) && (ret2==E_OK)) {
				p_property->property = p_ll_property->property;
			}
		}

		if (p_next_caller) p_next_caller = p_next_caller->p_next;
		if (p_next_queued) p_next_queued = p_next_queued->p_next;
	} while (p_next_caller != NULL);
}

#if 0
/**
    Add request descriptor to service queue
*/
static ER grph_req_add_list(KDRV_GRPH_TRIGGER_PARAM* p_param)
{
	return E_OK;
}
#endif

/**
    Graphic internal trigger

    Graphic driver internal API.
    This function is shared by graph_request() and kdrv_grph_trigger().

    @param[in] id           graphic controller identifier
    @param[in] p_param      Description of request

    @return
        - @b E_OK: Done with no errors.
        - @b E_NOSPT: Operation no support.
        - @b E_NOEXS: Engine not in the ready state.
        - @b E_PAR: width/height NOT pass through image C for GRPH_CMD_TEXT_COPY/GRPH_CMD_PLANE_BLENDING/GRPH_CMD_ALPHA_SWITCH/
*/
ER graph_trigger(GRPH_ID id)
//ER graph_trigger(GRPH_ID id, KDRV_GRPH_TRIGGER_PARAM *p_param,
//		KDRV_CALLBACK_FUNC *p_cb_func)
{
	ER ret = E_OK;
//        ER ret_unlock = E_OK;
	PGRPH_REQUEST p_request = NULL;
	GRPH_REQUEST *p_next_req;
	GRPH_REQ_LIST_NODE *p_node = NULL;

        if (id > GRPH_ID_2) {
                DBG_ERR("invalid controller ID: %d\r\n", (int)id);
                return E_NOSPT;
        }

	p_node = grph_platform_get_head(id);
	if (p_node == NULL) return E_NOSPT;

	p_request = (PGRPH_REQUEST)(&p_node->v_ll_req[0].trig_param);
//    p_request = (PGRPH_REQUEST)(&p_node->trig_param);

	if (p_request == NULL) return E_NOSPT;
	p_next_req = p_request;

#if 0
printk("%s: cmd 0x%x\r\n", __func__, p_request->command);
	memset(&v_grph_callback[id], 0, sizeof(KDRV_CALLBACK_FUNC));
	if (p_cb_func) {
		memcpy(&v_grph_callback[id], p_cb_func,
			sizeof(KDRV_CALLBACK_FUNC));
	}
#endif

	graph_init_ll(id);

	do {
		// check image parameters
		vp_image_a[id] = vp_image_b[id] = vp_image_c[id] = NULL;
		ret = graph_handle_images(id, p_next_req, p_next_req->p_images);
		if (ret != E_OK) {
			DBG_ERR("invalid pointer in input images\r\n");
			break;
		}
//printk("%s: %d command %x\r\n", __func__, id, p_request->command);

		if (p_next_req->command < GRPH_CMD_GOPMAX) {
			ret = graph_request_gop(id, p_next_req);
		} else {
			ret = graph_request_aop(id, p_next_req);
		}

		p_next_req = p_next_req->p_next;
		ll_req_index[id]++;
		if (ll_req_index[id] == GRPH_LL_MAX_COUNT) {
			DBG_WRN("max support link list count is %d, input exceed\r\n",
				(int)GRPH_LL_MAX_COUNT);
			break;
		}
	} while (p_next_req != NULL);

	graph_ll_finalize(id);
	trigger_multi_frame(id);

	vp_image_a[id] = vp_image_b[id] = vp_image_c[id] = NULL;

	return ret;
}

/**
    Graphic request enqueue

    Graphic driver internal API.
    This function is shared by graph_request() and kdrv_grph_trigger().

    @param[in] id           graphic controller identifier
    @param[in] p_param      Description of request
    @param[in] p_cb_func    request complete callback

    @return
        - @b E_OK: Done with no errors.
        - @b E_NOSPT: Operation no support.
        - @b E_NOEXS: Engine not in the ready state.
        - @b E_PAR: width/height NOT pass through image C for GRPH_CMD_TEXT_COPY/GRPH_CMD_PLANE_BLENDING/GRPH_CMD_ALPHA_SWITCH/
*/
ER graph_enqueue(GRPH_ID id, KDRV_GRPH_TRIGGER_PARAM *p_param,
                KDRV_CALLBACK_FUNC *p_cb_func)
{
	ER ret = E_OK;
	ER ret_unlock = E_OK;
	PGRPH_REQUEST p_request = NULL;		// point to that passed from caller
	PGRPH_REQUEST p_queued_request = NULL;	// point to internal queued that refered by bottom half
	UINT32 spin_flags;

 	if (id > GRPH_ID_2) {
		DBG_ERR("invalid controller ID: %d\r\n", (int)id);
		return E_NOSPT;
	}

	if (p_param == NULL) {
		DBG_ERR("NULL p_param\r\n");
		return E_NOEXS;
	}

	if ((p_cb_func == NULL) || (p_cb_func->callback == NULL)) {
		ret = graph_lock(id);
		if (ret != E_OK) {
			DBG_ERR("%s: %d lock fail\r\n", __func__, (int)id);
			return ret;
		}
	}

	//
	// enqueue p_param (enter critical section)
	//
	spin_flags = grph_platform_spin_lock(id);

        if (grph_platform_list_empty(id)) {
                // if queue is empty, immediately fed to hw and trigger
                p_request = (PGRPH_REQUEST)p_param;
        }

        if (grph_platform_add_list(id, p_param, p_cb_func) != E_OK) {
                grph_platform_spin_unlock(id, spin_flags);
                if ((p_cb_func == NULL) || (p_cb_func->callback == NULL)) {
                        ret =  graph_unlock(id);
                        if (ret != E_OK) {
				DBG_ERR("%s: %d lock fail2\r\n", __func__, (int)id);
                                return ret;
                        }
                }
                DBG_ERR("%s: grph %d add list fail\r\n", __func__, (int)id);
                return E_SYS;
        }

	if (p_request != NULL) {
		GRPH_REQ_LIST_NODE *p_node;

		p_node = grph_platform_get_head(id);
	        if (p_node == NULL) {
			grph_platform_spin_unlock(id, spin_flags);
			DBG_ERR("%s: get queued head fail\r\n", __func__);
			if ((p_cb_func == NULL) || (p_cb_func->callback == NULL)) {
	                        ret =  graph_unlock(id);
	                        if (ret != E_OK) {
					DBG_ERR("%s: %d lock fail3\r\n", __func__, (int)id);
	                                return ret;
	                        }
	                }

	                return E_SYS;
	        }

	        p_queued_request = (PGRPH_REQUEST)(&(p_node->v_ll_req[0].trig_param));

		ret = graph_trigger(id);
	}

	//
	// enqueue p_param (exit critical section)
	//
        grph_platform_spin_unlock(id, spin_flags);
//if ((id == GRPH_ID_1) && (p_param->command == GRPH_CMD_BLENDING))
//	printk("%s: %d after un\r\n", __func__, id);

	// If queue is empty before we assert, trigger hw now
#if 0
	if (p_request != NULL) {
		ret = graph_trigger(id);
#if 0
		// Unlock Graphic module
		if (p_cb_func == NULL || p_cb_func->callback == NULL) {
			ret_unlock = graph_unlock(id);
			if (ret_unlock != E_OK) {
				return ret_unlock;
			}
		}

		return ret;
#endif
	} else {
//		return E_OK;
	}
#endif
	// Unlock Graphic module
	if ((p_cb_func == NULL) || (p_cb_func->callback == NULL)) {

		if (p_request && (ret == E_OK)) {
			grph_platform_flg_wait(id, v_isr_flag[id]);

//			request_post_handler(id, p_request);

#if 1
			post_handle_acc(id, p_request, p_queued_request);
#else
			if (p_param->command == GRPH_CMD_ACC) {
				PGRPH_PROPERTY      p_property = 0;
				T_GRPH_ACCU_REG     acc1_reg;

				acc1_reg.reg = GRPH_GETREG(GRPH_ACCU_REG_OFS);

				if (graph_fetch_property(p_request->p_property,
						GRPH_PROPERTY_ID_ACC_RESULT,
						&p_property) == E_OK) {
					p_property->property = acc1_reg.bit.ACC_RESULT;
				}
			}
#endif
		}

		ret_unlock = graph_unlock(id);
		if (ret_unlock != E_OK) {
			return ret_unlock;
		}
	}

	return ret;
}

/**
    Graphic operation request

    This is generic version that can support ALL operation/command supported by graphic engine.
    The destination buffer is fixed to Image3 (i.e. Image C).

    @param[in] id           graphic controller identifier
    @param[in] p_request     Description of request

    @return
	- @b E_OK: Done with no errors.
	- @b E_NOSPT: Operation no support.
	- @b E_NOEXS: Engine not in the ready state.
	- @b E_PAR: width/height NOT pass through image C for GRPH_CMD_TEXT_COPY/GRPH_CMD_PLANE_BLENDING/GRPH_CMD_ALPHA_SWITCH/
*/
ER graph_request(GRPH_ID id, PGRPH_REQUEST p_request)
{
	return graph_enqueue(id, (KDRV_GRPH_TRIGGER_PARAM*)p_request, NULL);
//	return graph_trigger(id, (KDRV_GRPH_TRIGGER_PARAM*)p_request, NULL);
}
EXPORT_SYMBOL(graph_request);

//@}
