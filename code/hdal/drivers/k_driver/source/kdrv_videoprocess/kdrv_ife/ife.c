/*
    IFE module driver

    NT96680 IFE driver.

    @file       IFE.c
    @ingroup    mIDrvIPP_IFE

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/



#include    "ife_lib.h"
#include    "ife_reg.h"
#include    "ife_int.h"
#include    "ife_platform.h"

//#define IFE_CLK 0
//#define pll_set_clock_freq(x)
//#define pll_enable_clock(x)
//#define pll_disable_clock(x)
//#define pll_set_clock_rate(x,y)
//#define pll_getPLLEn(x,y)
//#define pll_setPLLEn(x)
//#define Perf_Get_current(x) 0
//#define pll_get_clock_freq(x)
//#define dma_flush_write_cache(x,y)
//#define dma_get_phy_addr(parm) fmem_lookup_pa(parm)
//#define dma_flush_read_cache(x,y)
//#define dma_flush_read_cache_width_neq_line_offset(x,y)

#if defined (_NVT_EMULATION_)
#include "comm/timer.h"
static TIMER_ID ife_timer_id;
BOOL ife_end_time_out_status = FALSE;
#define IFE_POWER_SAVING_EN 0//do not enable power saving in emulation
#else
#define IFE_POWER_SAVING_EN 1
#endif

#define DRV_SUPPORT_IST 0

//UINT32 _IFE_REG_BASE_ADDR;

#if 0//(DRV_SUPPORT_IST == ENABLE)
#else
static void (*pf_ife_int_cb)(UINT32 ui_int_status);
#endif

//void ife_isr(void);

static IFE_ENGINE_STATUS   g_ui_ife_engine_status  = IFE_ENGINE_IDLE;
static ID       g_ife_lock_status     = NO_TASK_LOCKED;

static BOOL     g_b_polling = FALSE;
static IFE_OPMODE  ife_mode;
static BOOL g_ife_clk_en          = FALSE;


static ER ife_chk_state_machine(IFEOPERATION Op, IFESTATUSUPDATE Update);
static ER ife_attach(void);
static ER ife_detach(void);
static ER ife_lock(void);
static ER ife_unlock(void);
static ER ife_set_cfa_pat(IFE_CFASEL cfa_pat);
static ER ife_set_bayer_fmt(IFE_BAYERFMTSEL bayer_fmt_sel);
static ER ife_bitsel(IFE_BITDEPTH in_bit_depth, IFE_BITDEPTH out_bit_depth);
static ER ife_set_fnum(UINT8 fusion_num);
static ER ife_set_ifecontrol(IFE_CONTROL func_enable);
static ER ife_set_ring_buf(IFE_RING_BUF *ring_info);
static ER ife_set_dma_wait_sie2_st_disable(BOOL hdr_wait_sie2_st_disable);
static ER ife_set_rde_default(IFE_RDESET *p_rde);
static ER ife_set_rde_ctrl(IFE_RDESET *p_rde);
static ER ife_set_rde_rate(IFE_RDESET *p_rde);
static ER ife_set_rde_qtable_inx(IFE_RDESET *p_rde);
static ER ife_set_rde_rand_init(IFE_RDESET *p_rde);
static ER ife_set_rde_degamma(IFE_RDESET *p_rde);

static ER ife_set_nrs_ctrl(IFE_NRSSET *nrs_set);
static ER ife_set_nrs_order(IFE_NRSSET *nrs_set);
static ER ife_set_nrs_bilat(IFE_NRSSET *nrs_set);

static ER ife_set_fcurve_ctrl(IFE_FCURVESET *fcurve_set);
static ER ife_set_fcurve_yweight(IFE_FCURVESET *fcurve_set);
static ER ife_set_fcurve_index(IFE_FCURVESET *fcurve_set);
static ER ife_set_fcurve_split(IFE_FCURVESET *fcurve_set);
static ER ife_set_fcurve_value(IFE_FCURVESET *fcurve_set);

static ER ife_set_fusion_ctrl(IFE_FUSIONSET *fusion_set);
static ER ife_set_fusion_blend_curve(IFE_FUSIONSET *fusion_set);
static ER ife_set_fusion_diff_weight(IFE_FUSIONSET *fusion_set);
static ER ife_set_fusion_dark_saturation_reduction(IFE_FUSIONSET *fusion_set);
static ER ife_set_fusion_short_exposure_compress(IFE_FUSIONSET *fusion_set);
static void ife_set_fusion_color_gain(IFE_FCGAINSET *p_fcgain);

static ER ife_set2dnr_rbfill(IFE_RBFill_PARAM *p_rbfill_set);
static ER ife_set2dnr_rbfill_ratio_mode(IFE_RBFill_PARAM *p_rbfill_set);
static ER ife_set2dnr_spatial_w(UINT32 *spatial_weight);
//static ER ife_set_sonly(BOOL b_enable, IFE_SONLYLEN Length);
//static ER ife_set_filtermode(IFE_FILTMODE filter_mode);
static ER ife_set_binning(IFE_BINNSEL binn_sel);
static ER ife_set2dnr_nlm_parameter(IFE_RANGESETA *p_range_set);
static ER ife_set2dnr_bilat_parameter(IFE_RANGESETB *p_range_set);
static ER ife_set2dnr_center_modify(IFE_CENMODSET *p_center_mod_set);
//static ER ife_set_nlm_ker(IFE_NLM_KER *p_nlm_ker_set);
//static ER ife_set_nlm_weight_lut(IFE_NLM_LUT *p_nlm_lut_set);
static ER ife_set2dnr_range_weight(UINT32 rth_w);
static ER ife_set2dnr_blend_weight(UINT32 bilat_w);
static ER ife_chg_burst_length(IFE_BURST_LENGTH *p_burst_len);
static ER ife_set_in_size(UINT32 hsize, UINT32 vsize);
static ER ife_set_crop(UINT32 crop_width, UINT32 crop_height, UINT32 crop_hpos, UINT32 crop_vpos);
static ER ife_set_clamp(IFE_CLAMPSET *p_clamp_weight);
//static ER ife_set_crv_map(IFE_CRVMAPSET *p_crv_map);
static void ife_set_color_gain(IFE_CGAINSET *p_cgain, IFE_CFASEL cfa_pat);
static ER ife_set_outlier(IFE_OUTLSET *p_outlier);
//static ER ife_set_row_def_conc(IFE_ROWDEFSET *p_row_def_set);
//static ER ife_set_bit_dither(UINT8 en);
static ER ife_set_vignette(IFE_VIG_PARAM *pvig_set);
static ER ife_set_gbal(IFE_GBAL_PARAM *pgbal_set);
static BOOL ife_is_busy(void);
static ER ife_set_stripe(IFE_STRIPE stripe_info);
static void ife_set_load(void);
static ER ife_stop(void);

static BOOL ife_ring_buf_msg_flag = FALSE;
static UINT32 err_msg_cnt[6];
static UINT32 frame_cnt;
#define IFE_ERR_MSG_FREQ 90
#define IFE_ERR_MSG_CNT_MAX 60



//for FPGA use
ER      ife_clr(void);
ER      ife_set_dma_in_addr(UINT32 in_addr, UINT32 line_ofs);
ER      ife_set_dma_in_addr_2(UINT32 in_addr, UINT32 line_ofs);
ER      ife_set_hshift(UINT8 h_shift);
ER      ife_set_dma_out_addr(UINT32 out_addr, UINT32 line_ofs);
UINT32  ife_get_dram_in_addr(void);
#if (defined(_NVT_EMULATION_) == ON)
UINT32  ife_get_dram_in_addr2(void);
#endif
UINT32  ife_get_dram_in_lofs(void);
UINT32  ife_get_dram_in_lofs2(void);
UINT32  ife_get_dram_out_addr(void);
UINT32  ife_get_dram_out_lofs(void);
UINT32  ife_get_in_vsize(void);
UINT32  ife_get_in_hsize(void);
ER      ife_set_op_mode(IFE_OPMODE mode);

#if (defined(_NVT_EMULATION_) == OFF)
static BOOL g_ife_first_frame_flg = TRUE;
static UINT32  dmaloop_enable_layer1 = 0;
#endif

UINT32 ife_fw_power_saving_en = TRUE;

//void    ife_set2ready(void);


/*
void ife_create_resource(void)
{
    OS_CONFIG_FLAG(FLG_ID_IFE);
    SEM_CREATE(SEMID_IFE, 1);
}
void ife_release_resource(void)
{
    rel_flg(FLG_ID_IFE);
    SEM_DESTROY(SEMID_IFE);
}

void ife_set_base_addr(UINT32 ui_addr)
{
    _IFE_REG_BASE_ADDR = ui_addr;
}
*/
#if defined (_NVT_EMULATION_)
static void ife_time_out_cb(UINT32 event)
{
	DBG_ERR("IFE time out!\r\n");
	ife_end_time_out_status = TRUE;
	ife_platform_flg_set(FLGPTN_IFE_FRAMEEND);
}
#endif
/*
    IFE interrupt handler

    IFE interrupt service routine.
*/
void ife_isr(void)
{
	UINT32 ui_ife_int_status_raw, ui_ife_int_status, ui_ife_int_enable;
	UINT32 CBStatus;
	UINT32 ring_err_debug_reg, sie_line_count, ife_line_count;
	FLGPTN flg = 0x0;

	ui_ife_int_status_raw = ife_get_int_status();

	if (ife_platform_get_init_status() == FALSE) {
		ife_clear_int(ui_ife_int_status_raw);
		return;
	}

	ui_ife_int_enable = ife_get_int_enable();
	ui_ife_int_status = ui_ife_int_status_raw & ui_ife_int_enable;
	ife_clear_int(ui_ife_int_status);

	if (ui_ife_int_status == 0) {
		return;
	}


	CBStatus = 0;
	flg = 0x0;

	if (ui_ife_int_status & IFE_INT_SIE_FRM_START) {
		//ife_platform_flg_set(FLGPTN_IFE_SIE_FRM_START);
		//if (ife_get_int_enable() & IFE_INTE_SIE_FRM_START) {
		//  CBStatus |= IFE_INT_SIE_FRM_START;
		//}

		flg |= FLGPTN_IFE_SIE_FRM_START;
		CBStatus |= IFE_INT_SIE_FRM_START;

#if (defined(_NVT_EMULATION_) == OFF)
		dmaloop_enable_layer1 = (IFE_GETREG(0x5c) & 0x01000000) >> 24;
#endif
		frame_cnt ++;
		if(frame_cnt > IFE_ERR_MSG_FREQ){
			frame_cnt = 1;
		}
	}

	if (ui_ife_int_status & IFE_INT_FRMEND) {
		//nvt_dbg(INFO,"ife frame-end!!\r\n");
		//ife_platform_flg_set(FLGPTN_IFE_FRAMEEND);
		//if (ife_get_int_enable() & IFE_INTE_FRMEND) {
		//  CBStatus |= IFE_INT_FRMEND;
		//}

		flg |= FLGPTN_IFE_FRAMEEND;
		CBStatus |= IFE_INT_FRMEND;
#if (defined(_NVT_EMULATION_) == OFF)
		if (g_ui_ife_engine_status == IFE_ENGINE_PAUSE) {
			ui_ife_int_enable &= (~IFE_INTE_RING_BUF_ERR);
			IFE_SETREG(FILTER_INTERRUPT_ENABLE_REGISTER_OFS, ui_ife_int_enable);
			g_ife_first_frame_flg = TRUE;
		} else {
			g_ife_first_frame_flg = FALSE;
		}
#endif
	}

	if (ui_ife_int_status & IFE_INT_LLEND) {
		//ife_platform_flg_set(FLGPTN_IFE_LLEND);
		//DBG_IND("LL end set\r\n");
		//if (ife_get_int_enable() & IFE_INTE_LLEND) {
		//  CBStatus |= IFE_INT_LLEND;
		//}

		flg |= FLGPTN_IFE_LLEND;
		CBStatus |= IFE_INT_LLEND;
	}
	
	if (ui_ife_int_status & IFE_INT_FRAME_ERR) {
		//ife_platform_flg_set(FLGPTN_IFE_FRAME_ERR);
		//if (ife_get_int_enable() & IFE_INTE_FRAME_ERR) {
		//  CBStatus |= IFE_INT_FRAME_ERR;
		//}

		flg |= FLGPTN_IFE_FRAME_ERR;
		CBStatus |= IFE_INT_FRAME_ERR;

		if(frame_cnt == IFE_ERR_MSG_FREQ && err_msg_cnt[0] < IFE_ERR_MSG_CNT_MAX){
			DBG_ERR("IFE frame error!\r\n");
			err_msg_cnt[0]++;
		}

	}

	ife_platform_flg_set(flg);


	if (ui_ife_int_status & IFE_INT_DEC1_ERR) {
		//if (ife_get_int_enable() & IFE_INTE_DEC1_ERR) {
		//  CBStatus |= IFE_INT_DEC1_ERR;
		//}

		CBStatus |= IFE_INT_DEC1_ERR;

		if(frame_cnt == IFE_ERR_MSG_FREQ && err_msg_cnt[1] < IFE_ERR_MSG_CNT_MAX){
			DBG_WRN("IFE Decode error1!\r\n");
			err_msg_cnt[1]++;
		}		
	}
	if (ui_ife_int_status & IFE_INT_DEC2_ERR) {
		//if (ife_get_int_enable() & IFE_INTE_DEC2_ERR) {
		//  CBStatus |= IFE_INT_DEC2_ERR;
		//}
		
		CBStatus |= IFE_INT_DEC2_ERR;
		if(frame_cnt == IFE_ERR_MSG_FREQ && err_msg_cnt[2] < IFE_ERR_MSG_CNT_MAX){
			DBG_WRN("IFE Decode error2!\r\n");
			err_msg_cnt[2]++;
		}				
	}

	if (ui_ife_int_status & IFE_INT_LLERR) {
		//if (ife_get_int_enable() & IFE_INTE_LLERR) {
		//  CBStatus |= IFE_INT_LLERR;
		//}

		CBStatus |= IFE_INT_LLERR;
	}
	if (ui_ife_int_status & IFE_INT_LLERR2) {
		CBStatus |= IFE_INT_LLERR2;
		if(frame_cnt == IFE_ERR_MSG_FREQ && err_msg_cnt[3] < IFE_ERR_MSG_CNT_MAX){
			DBG_WRN("IFE LL error2!\r\n");
			err_msg_cnt[3]++;
		}						


		//if (ife_get_int_enable() & IFE_INTE_LLERR2) {
		//  CBStatus |= IFE_INT_LLERR2;
		//}
	}
	if (ui_ife_int_status & IFE_INT_LLJOBEND) {
		CBStatus |= IFE_INT_LLJOBEND;

		//if (ife_get_int_enable() & IFE_INTE_LLJOBEND) {
		//  CBStatus |= IFE_INT_LLJOBEND;
		//}
	}
	if (ui_ife_int_status & IFE_INT_BUFOVFL) {
		CBStatus |= IFE_INT_BUFOVFL;

		if(frame_cnt == IFE_ERR_MSG_FREQ && err_msg_cnt[4] < IFE_ERR_MSG_CNT_MAX){
			DBG_ERR("IFE Buffer overflow!\r\n");
			err_msg_cnt[4]++;
		}						

		//if (ife_get_int_enable() & IFE_INTE_BUFOVFL) {
		//  CBStatus |= IFE_INT_BUFOVFL;
		//}
	}
	if (ui_ife_int_status & IFE_INT_RING_BUF_ERR) {
        #if (defined(_NVT_EMULATION_) == OFF)
    		//if (ife_get_int_enable() & IFE_INTE_RING_BUF_ERR) {
    		//  CBStatus |= IFE_INT_RING_BUF_ERR;
    		//}
    		CBStatus |= IFE_INT_RING_BUF_ERR;
    		if (g_ife_first_frame_flg == FALSE && dmaloop_enable_layer1 == 1) {

				if(ife_ring_buf_msg_flag == FALSE){
					ring_err_debug_reg = IFE_GETREG(0x0478);
					sie_line_count = ((ring_err_debug_reg & 0xFFFF0000) >> 16);
					ife_line_count = (ring_err_debug_reg & 0x0000FFFF);
					DBG_DUMP("IFE ring buffer Info:\r\n");
					DBG_DUMP("SIE line count: 0x%04x, IFE line count: 0x%04x\r\n", (unsigned int)sie_line_count, (unsigned int)ife_line_count);
				}
				ife_ring_buf_msg_flag = TRUE;
			
				/*if(frame_cnt == IFE_ERR_MSG_FREQ && err_msg_cnt[5] < IFE_ERR_MSG_CNT_MAX){
					ring_err_debug_reg = IFE_GETREG(0x0478);
					sie_line_count = ((ring_err_debug_reg & 0xFFFF0000) >> 16);
					ife_line_count = (ring_err_debug_reg & 0x0000FFFF);
					DBG_DUMP("IFE ring buffer error!\r\n");
					DBG_DUMP("SIE line count: %04x, IFE line count: %04x\r\n", (unsigned int)sie_line_count, (unsigned int)ife_line_count);
					err_msg_cnt[5]++;
				}*/					
    		}
        #else
    		//if (ife_get_int_enable() & IFE_INTE_RING_BUF_ERR) {
    		//  CBStatus |= IFE_INT_RING_BUF_ERR;
    		//}
    		CBStatus |= IFE_INT_RING_BUF_ERR;

			if(ife_ring_buf_msg_flag == FALSE){
				ring_err_debug_reg = IFE_GETREG(0x0478);
				sie_line_count = ((ring_err_debug_reg & 0xFFFF0000) >> 16);
				ife_line_count = (ring_err_debug_reg & 0x0000FFFF);
				DBG_DUMP("IFE ring buffer Info:\r\n");
				DBG_DUMP("SIE line count: 0x%04x, IFE line count: 0x%04x\r\n", (unsigned int)sie_line_count, (unsigned int)ife_line_count);
			}
			ife_ring_buf_msg_flag = TRUE;

			/*if(frame_cnt == IFE_ERR_MSG_FREQ && err_msg_cnt[5] < IFE_ERR_MSG_CNT_MAX){
				ring_err_debug_reg = IFE_GETREG(0x0478);
				sie_line_count = ((ring_err_debug_reg & 0xFFFF0000) >> 16);
				ife_line_count = (ring_err_debug_reg & 0x0000FFFF);
				DBG_WRN("IFE ring buffer error!\r\n");
				DBG_WRN("SIE line count: %04x, IFE line count: %04x\r\n", (unsigned int)sie_line_count, (unsigned int)ife_line_count);
				err_msg_cnt[5]++;				
			}*/					
        #endif
	}


	//removed in NT96680
	//if(ui_ife_int_status & IFE_INT_ROWDEFFAIL)
	//{
	//    DBG_WRN("Row Defect WARNING\r\n");
	//}

#if (DRV_SUPPORT_IST == ENABLE)
	if (pf_ist_cb_ife != NULL) {
		drv_set_ist_event(DRV_IST_MODULE_IFE, CBStatus);
	}
#else
	if (pf_ife_int_cb != NULL) {
		pf_ife_int_cb(CBStatus);
	}
#endif

}

/**
  Open IFE driver

  This function should be called before calling any other functions

  @param[in] p_objCB ISR callback function

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
ER ife_open(IFE_OPENOBJ *p_objCB)
{
	ER er_return;
	IFE_BURST_LENGTH burst_len;

	er_return = ife_lock();
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = ife_chk_state_machine(IFE_OP_OPEN, NOTUPDATE);
	if (er_return != E_OK) {
		// release resource when state_machine fail
		ife_unlock();
		return er_return;
	}

	// Turn on power
	//pmc_turnon_power(PMC_MODULE_IFE);//no power domain at NT96680

	// Select PLL clock source & enable PLL clock source
	ife_platform_set_clk_rate(p_objCB->ui_ife_clock_sel);

	// clock prepare
	//ife_platform_prepare_clk();

	er_return = ife_attach();
	if (er_return != E_OK) {
		return er_return;
	}

	//Enable SRAM
	//pll_disable_sram_shut_down(IFE_RSTN);
	ife_platform_disable_sram_shutdown();

	//enable interrupt
	//drv_enable_int(DRV_INT_IFE);
	ife_platform_int_enable();

#if 0 // remove for builtin
	// Pre-set the interrupt flag
	er_return = ife_platform_flg_clear(FLGPTN_IFE_FRAMEEND | FLGPTN_IFE_DEC1_ERR | FLGPTN_IFE_DEC2_ERR | FLGPTN_IFE_BUFOVFL |
									   FLGPTN_IFE_RING_BUF_ERR | FLGPTN_IFE_FRAME_ERR | FLGPTN_IFE_SIE_FRM_START);
	if (er_return != E_OK) {
		return er_return;
	}

	ife_enable_int(IFE_INTE_ALL);
	ife_clear_int(IFE_INT_ALL);

	ife_clr();
	burst_len.burst_len_input = IFE_IN_BURST_64W;
	burst_len.burst_len_output = IFE_OUT_BURST_32W;
	ife_chg_burst_length(&burst_len);
#endif


#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ife_ctrl_flow_to_do == ENABLE) {
		// Pre-set the interrupt flag
		er_return = ife_platform_flg_clear(FLGPTN_IFE_FRAMEEND | FLGPTN_IFE_DEC1_ERR | FLGPTN_IFE_DEC2_ERR | FLGPTN_IFE_BUFOVFL |
										   FLGPTN_IFE_RING_BUF_ERR | FLGPTN_IFE_FRAME_ERR | FLGPTN_IFE_SIE_FRM_START);
		if (er_return != E_OK) {
			return er_return;
		}

		ife_enable_int(IFE_INTE_ALL);
		ife_clear_int(IFE_INT_ALL);

		ife_clr();
		burst_len.burst_len_input = IFE_IN_BURST_64W;
		burst_len.burst_len_output = IFE_OUT_BURST_32W;
		ife_chg_burst_length(&burst_len);
	}
#else

	// Pre-set the interrupt flag
	er_return = ife_platform_flg_clear(FLGPTN_IFE_FRAMEEND | FLGPTN_IFE_DEC1_ERR | FLGPTN_IFE_DEC2_ERR | FLGPTN_IFE_BUFOVFL |
									   FLGPTN_IFE_RING_BUF_ERR | FLGPTN_IFE_FRAME_ERR | FLGPTN_IFE_SIE_FRM_START);
	if (er_return != E_OK) {
		return er_return;
	}

	ife_enable_int(IFE_INTE_ALL);
	ife_clear_int(IFE_INT_ALL);

	ife_clr();
	burst_len.burst_len_input = IFE_IN_BURST_64W;
	burst_len.burst_len_output = IFE_OUT_BURST_32W;
	ife_chg_burst_length(&burst_len);

#endif

	pf_ife_int_cb = p_objCB->FP_IFEISR_CB;

	ife_ring_buf_msg_flag = FALSE;


	//nvt_dbg(INFO, "5\r\n");

	er_return = ife_chk_state_machine(IFE_OP_OPEN, UPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	return E_OK;
}

/**
    check IFE is opened.

    check if IFE module is opened.

    @return
        -@b TRUE: IFE is open.
        -@b FALSE: IFE is closed.
*/
BOOL ife_is_opened(void)
{
	return g_ife_lock_status;
}

/**
  Close IFE driver

  Release IFE driver

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
ER ife_close(void)
{
	ER er_return;

	er_return = ife_chk_state_machine(IFE_OP_CLOSE, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	// Check lock status
	if (g_ife_lock_status != TASK_LOCKED) {
		return E_SYS;
	}

	// Release interrupt
	//linux drv_disable_int(DRV_INT_IFE);
	//ife_set_op_mode(IFE_OPMODE_D2D);

	ife_platform_int_disable();

	//Disable SRAM
	//pll_enable_sram_shut_down(IFE_RSTN);
	ife_platform_enable_sram_shutdown();

	er_return = ife_detach();
	if (er_return != E_OK) {
		return er_return;
	}

	// clock prepare
	//ife_platform_unprepare_clk();

	er_return = ife_chk_state_machine(IFE_OP_CLOSE, UPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	ife_unlock();

	return E_OK;
}

/**
  IFE interrupt enable selection.

  IFE interrupt enable selection.

  @param[in] ui_intr 1's in this word will activate corresponding interrupt.

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
ER ife_enable_int(UINT32 intr)
{
	T_FILTER_INTERRUPT_ENABLE_REGISTER localreg;

	localreg.reg = intr;

	if (g_b_polling) {
		localreg.reg &= (~IFE_INTE_FRMEND);
	} else {
		localreg.reg |= IFE_INTE_FRMEND;
	}
	IFE_SETREG(FILTER_INTERRUPT_ENABLE_REGISTER_OFS, localreg.reg);

	return E_OK;
}

/**
  IFE interrupt enable status.

  Check IFE interrupt enable status.

  @return IFE interrupt enable status.
*/
UINT32 ife_get_int_enable(void)
{
	T_FILTER_INTERRUPT_ENABLE_REGISTER  localreg;

	localreg.reg = IFE_GETREG(FILTER_INTERRUPT_ENABLE_REGISTER_OFS);
	return localreg.reg;
}

/**
  IFE interrupt status.

  Check IFE interrupt status.

  @return IFE interrupt status readout.
*/
UINT32 ife_get_int_status(void)
{
	T_FILTER_INTERRUPT_STATUS_REGISTER  localreg;

	localreg.reg = IFE_GETREG(FILTER_INTERRUPT_STATUS_REGISTER_OFS);
	return localreg.reg;
}

/**
  IFE clear interrupt status.

  IFE clear interrupt status.

  @param[in] ui_intr 1's in this word will clear corresponding interrupt

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
ER ife_clear_int(UINT32 intr)
{
	T_FILTER_INTERRUPT_STATUS_REGISTER  localreg;

	localreg.reg = intr;
	IFE_SETREG(FILTER_INTERRUPT_STATUS_REGISTER_OFS, localreg.reg);

	return E_OK;
}


VOID ife_set_builtin_flow_disable(VOID)
{
	ife_ctrl_flow_to_do = ENABLE;
}
//-------------------------------------------------------------------------------


/**
  set IFE mode

  Set IFE mode to execute

  @param[in] p_filter_info Parameter for IFE execution
  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER ife_set_mode(IFE_PARAM *p_filter_info)
{
	ER er_return;

	//IFE_ROWDEFSET Rdset;
#if (IFE_DMA_CACHE_HANDLE == 1)
	UINT32 ui_addr, ui_size, ui_phy_addr;
#endif


	if (ife_ctrl_flow_to_do == DISABLE) {
	    ife_set_op_mode(p_filter_info->mode);
	    
		goto SKIP_PARAMS;
	}

	switch (p_filter_info->mode) {
	case IFE_OPMODE_D2D:
		break;
	case IFE_OPMODE_IPP:
		break;
	case IFE_OPMODE_ALL_DIRECT:
		p_filter_info->out_bit = IFE_12BIT;
		break;
	default:
		//no change
		break;
	}

	ife_set_dma_wait_sie2_st_disable(p_filter_info->b_ife_dma_wait_sie2_st_disable);

	ife_set_stripe(p_filter_info->ife_stripe_info);

	{

		//------------ IFE Control setting --------------//
		ife_set_op_mode(p_filter_info->mode);
		er_return = ife_chk_state_machine(IFE_OP_SETMODE, NOTUPDATE);
		if (er_return != E_OK) {
			return er_return;
		}
		ife_set_ifecontrol(p_filter_info->ife_ctrl);
		ife_set_cfa_pat(p_filter_info->cfa_pat);
		ife_bitsel(p_filter_info->in_bit, p_filter_info->out_bit);
		ife_set_fnum(p_filter_info->fusion_number);
		ife_set_binning(p_filter_info->binn);
		ife_set_bayer_fmt((IFE_BAYERFMTSEL)p_filter_info->bayer_format);

		ife_set_ring_buf(&p_filter_info->ife_ringbuf_info);
		if (p_filter_info->ife_ctrl.b_fusion_en) {
			/*if (p_filter_info->ife_ctrl.b_fcgain_en == FALSE) {
				DBG_WRN("Fusion enable but FCgain not enable!!\r\n");
			}*/
			if (p_filter_info->ife_ctrl.b_fusion_en == FALSE) {
				DBG_WRN("Fusion enable but Fcurve not enable!!\r\n");
			}
		} else {
			/*if (p_filter_info->ife_ctrl.b_fcgain_en == TRUE) {
				DBG_WRN("Fusion not enable but FCgain enable!!\r\n");
			}*/
			if (p_filter_info->ife_ctrl.b_scompression_en == TRUE) {
				DBG_WRN("Fusion not enable but S_compression enable!!\r\n");
			}
		}

		//------------- IFE RDE setting ---------------//
		ife_set_rde_default(&p_filter_info->rde_set);
		ife_set_rde_ctrl(&p_filter_info->rde_set);
		ife_set_rde_rate(&p_filter_info->rde_set);
		ife_set_rde_qtable_inx(&p_filter_info->rde_set);
		ife_set_rde_rand_init(&p_filter_info->rde_set);
		ife_set_rde_degamma(&p_filter_info->rde_set);

		//------------- IFE NRS setting ---------------//
		ife_set_nrs_ctrl(&p_filter_info->nrs_set);
		ife_set_nrs_order(&p_filter_info->nrs_set);
		ife_set_nrs_bilat(&p_filter_info->nrs_set);

		//------------ IFE Fcurve setting -------------//
		ife_set_fcurve_ctrl(&p_filter_info->fcurve_set);
		ife_set_fcurve_yweight(&p_filter_info->fcurve_set);
		ife_set_fcurve_index(&p_filter_info->fcurve_set);
		ife_set_fcurve_split(&p_filter_info->fcurve_set);
		ife_set_fcurve_value(&p_filter_info->fcurve_set);

		//---------- IFE Fusion (Sensor HDR) ----------//
		ife_set_ifecontrol(p_filter_info->ife_ctrl);
		ife_set_fusion_ctrl(&p_filter_info->fusion_set);
		ife_set_fusion_blend_curve(&p_filter_info->fusion_set);
		ife_set_fusion_diff_weight(&p_filter_info->fusion_set);
		ife_set_fusion_dark_saturation_reduction(&p_filter_info->fusion_set);
		//--------- IFE F Color gain setting ----------//
		ife_set_fusion_color_gain(&p_filter_info->fcgain_set);
		//--------- IFE S Compression setting ---------//
		ife_set_fusion_short_exposure_compress(&p_filter_info->fusion_set);

		//------------ IFE Outlier setting ------------//
		ife_set_outlier(&p_filter_info->outl_set);

		//------------- IFE 2DNR setting --------------//
		ife_set2dnr_spatial_w(p_filter_info->p_spatial_weight);
		ife_set2dnr_nlm_parameter(&p_filter_info->rng_th_a);
		ife_set2dnr_bilat_parameter(&p_filter_info->rng_th_b);

		ife_set2dnr_center_modify(&p_filter_info->center_mod_set);

		ife_set2dnr_blend_weight(p_filter_info->bilat_w);
		ife_set2dnr_range_weight(p_filter_info->rth_w);
		ife_set2dnr_rbfill(&p_filter_info->rb_fill_set);
		ife_set2dnr_rbfill_ratio_mode(&p_filter_info->rb_fill_set);

		ife_set_clamp(&p_filter_info->clamp_set);

		//---------- IFE Color gain setting -----------//
		ife_set_color_gain(&p_filter_info->cgain_set, p_filter_info->cfa_pat);

		//----------- IFE Vignette setting ------------//
		ife_set_vignette(&p_filter_info->vig_set);

		//--------- IFE G balance setting (Crosstalk setting)---------//
		ife_set_gbal(&p_filter_info->gbal_set);

		//-------------------------------------------//

		ife_set_in_size(p_filter_info->width, p_filter_info->height);
		ife_set_crop(p_filter_info->crop_width, p_filter_info->crop_height, p_filter_info->crop_hpos, p_filter_info->crop_vpos);

#if 0  // remove for builtin
		ife_enable_int(p_filter_info->intr_en);
#endif

#if defined (__KERNEL__)
		//if (kdrv_builtin_is_fastboot() == DISABLE) {
		if (ife_ctrl_flow_to_do == ENABLE) {
			ife_enable_int(p_filter_info->intr_en);
		}
#else
		ife_enable_int(p_filter_info->intr_en);
#endif

#if (IFE_DMA_CACHE_HANDLE == 1)
		// Input Dram DMA/Cache Handle
		ui_addr = p_filter_info->in_addr0;
		ui_size = p_filter_info->in_ofs0 * p_filter_info->height;
/*
#if defined (_NVT_EMULATION_)
//		if (1) {
#else

#endif
*/
        if (p_filter_info->mode != IFE_OPMODE_ALL_DIRECT) {
			if (ui_addr == 0) {
				DBG_ERR("Input1 ui_addr can not be 0 !!");
			}

			if (ui_addr != 0) {
                ui_size = ALIGN_CEIL(ui_size, VOS_ALIGN_BYTES);
           		if(p_filter_info->mode == IFE_OPMODE_D2D) {
    				ife_platform_dma_flush_mem2dev(ui_addr, ui_size, 0);
           		}else{
       				ife_platform_dma_flush_mem2dev(ui_addr, ui_size, 1);
           		}
			}
			ui_phy_addr = ife_platform_va2pa(ui_addr);
			ife_set_dma_in_addr(ui_phy_addr, p_filter_info->in_ofs0);
		}

		if (p_filter_info->fusion_number == 1) {
			ui_addr = p_filter_info->in_addr1;
			ui_size = p_filter_info->in_ofs1 * p_filter_info->height;
			if (ui_addr != 0) {
                ui_size = ALIGN_CEIL(ui_size, VOS_ALIGN_BYTES);
                if(p_filter_info->mode == IFE_OPMODE_D2D){
	    			ife_platform_dma_flush_mem2dev(ui_addr, ui_size, 0);
                }else{
                    ife_platform_dma_flush_mem2dev(ui_addr, ui_size, 1);
                }
			}
			if (ui_addr == 0) {
				DBG_ERR("Input2 ui_addr can not be 0 !!");
			}
			ui_phy_addr = ife_platform_va2pa(ui_addr);
			ife_set_dma_in_addr_2(ui_phy_addr, p_filter_info->in_ofs1);
		}

		ife_set_hshift(p_filter_info->h_shift);

		// Output Dram DMA/Cache Handle
		if (p_filter_info->mode == IFE_OPMODE_D2D) {
			ui_addr = p_filter_info->out_addr;
			ui_size = p_filter_info->out_ofs * p_filter_info->height;
			if (ui_addr != 0) {
				//if (p_filter_info->out_ofs == p_filter_info->width) {
				//  ife_platform_dma_flush_mem2dev(ui_addr, ui_size);
				//} else {
				ui_size = ALIGN_CEIL(ui_size, VOS_ALIGN_BYTES);
				ife_platform_dma_flush_dev2mem(ui_addr, ui_size, 0);
				//}
			}
			ui_phy_addr = ife_platform_va2pa(ui_addr);
			er_return = ife_set_dma_out_addr(ui_phy_addr, p_filter_info->out_ofs);
		}

		if (er_return != E_OK) {
			return er_return;
		}

#else
		if (p_filter_info->mode != IFE_OPMODE_ALL_DIRECT) {
			ife_set_dma_in_addr(p_filter_info->in_addr0, p_filter_info->in_ofs0);
		}

		ife_set_dma_in_addr_2(p_filter_info->in_addr1, p_filter_info->in_ofs1);
		if (p_filter_info->mode == IFE_OPMODE_D2D) {
			er_return = ife_set_dma_out_addr(p_filter_info->out_addr, p_filter_info->out_ofs);
		}
#endif
		er_return = ife_chk_state_machine(IFE_OP_SETMODE, UPDATE);

	}
	return er_return;
	//return E_OK;

SKIP_PARAMS:
	er_return = ife_chk_state_machine(IFE_OP_SETMODE, UPDATE);
	return er_return;
}

#if 0
/**
  change IFE all parameter

  Set IFE parameter

  @param[in] p_filter_info Parameter for IFE execution
  @param[in] Reg_type select FD/nonFDlatch register type for update
  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER ife_change_all(IFE_PARAM *p_filter_info, IFE_REGTYPE reg_type)
{
	//IFE_ROWDEFSET Rdset;
	//UINT32 ui_start_shift;
	BOOL b_fdlatch = TRUE, b_non_fdlatch = TRUE;
#if (IFE_DMA_CACHE_HANDLE == 1)
	UINT32 ui_addr, ui_size, ui_phy_addr;
#endif

	switch (reg_type) {
	case IFE_FD_LATCHED:
		b_fdlatch = TRUE;
		b_non_fdlatch = FALSE;
		break;
	case IFE_NON_FD_LATCHED:
		b_fdlatch = FALSE;
		b_non_fdlatch = TRUE;
		break;
	default:
		b_fdlatch = TRUE;
		b_non_fdlatch = TRUE;
		break;
	}

	/*if (p_filter_info->mode == IFE_OPMODE_UNKNOWN) {
	    DBG_ERR("Invalid operation!! ife_change_all()\r\n");
	    return E_PAR;
	}*/
	if (p_filter_info->mode != IFE_OPMODE_D2D) {
	} else {
		DBG_ERR("Please use ife_set_mode() for D2D mode!!\r\n");
		return E_PAR;
	}

	p_filter_info->cfa_pat = IFE_PAT0;
	if (b_fdlatch == TRUE) {
		switch (p_filter_info->mode) {
		case IFE_OPMODE_IPP:
			break;
		case IFE_OPMODE_ALL_DIRECT:
			p_filter_info->in_bit = IFE_12BIT;
			p_filter_info->out_bit = IFE_12BIT;
			break;
		default:
			//no change
			break;
		}
	}
	{
		//ife_set_op_mode(p_filter_info->mode);
		//ife_clr();
		//FD latched part
		if (b_fdlatch == TRUE) {
			ife_set_cfa_pat(p_filter_info->cfa_pat);
			ife_bitsel(p_filter_info->in_bit, p_filter_info->out_bit);
			//ife_set_bit_dither(p_filter_info->ui_bit_dither);
			if (p_filter_info->ife_ctrl.b_outl_en) {
				ife_set_outlier(&p_filter_info->outl_set);
				ife_enable_function(ENABLE, IFE_OUTL_EN);
			} else {
				ife_enable_function(DISABLE, IFE_OUTL_EN);
			}
			if (p_filter_info->ife_ctrl.b_cgain_en) {
				ife_set_color_gain(&p_filter_info->cgain_set, p_filter_info->cfa_pat);
				ife_enable_function(ENABLE, IFE_CGAIN_EN);
			} else {
				ife_enable_function(DISABLE, IFE_CGAIN_EN);
			}
			ife_set_in_size(p_filter_info->width, p_filter_info->height);
			ife_enable_int(p_filter_info->intr_en);

#if (IFE_DMA_CACHE_HANDLE == 1)
			// Input Dram DMA/Cache Handle
			ui_addr = p_filter_info->in_addr0;
			ui_size = p_filter_info->in_ofs0 * p_filter_info->height;
			if (ui_addr != 0) {
				ife_platform_dma_flush_mem2dev(ui_addr, ui_size);
			}
			if (p_filter_info->mode != IFE_OPMODE_ALL_DIRECT) {
				if (ui_addr == 0) {
					DBG_IND("Input1 ui_addr can not be 0 !!");
				}
				ui_phy_addr = ife_platform_va2pa(ui_addr);
				ife_set_dma_in_addr(ui_phy_addr, p_filter_info->in_ofs0);
			}
			if (p_filter_info->fusion_number == 1) {
				ui_addr = p_filter_info->in_addr1;
				ui_size = p_filter_info->in_ofs1 * p_filter_info->height;
				if (ui_addr != 0) {
					ife_platform_dma_flush_mem2dev(ui_addr, ui_size);
				}
				if (ui_addr == 0) {
					DBG_IND("Input2 ui_addr can not be 0 !!");
				}
				ui_phy_addr = ife_platform_va2pa(ui_addr);
				ife_set_dma_in_addr_2(ui_phy_addr, p_filter_info->in_ofs1);
			}

			ife_set_hshift(p_filter_info->h_shift);
#else
			if (p_filter_info->mode != IFE_OPMODE_ALL_DIRECT) {
				ife_set_dma_in_addr(p_filter_info->in_addr0, p_filter_info->in_ofs0);
			}
			ife_set_dma_in_addr_2(p_filter_info->in_addr1, p_filter_info->in_ofs1);
#endif
		}
		if (p_filter_info->ife_ctrl.b_filter_en) {
			if (b_fdlatch == TRUE) {
				ife_set2dnr_spatial_w(p_filter_info->p_spatial_weight);
				ife_set_binning(p_filter_info->binn);
				ife_set2dnr_range_weight(p_filter_info->rth_w);
				ife_set_clamp(&p_filter_info->clamp_set);
				ife_enable_function(ENABLE, IFE_FILTER_EN);

				if (p_filter_info->ife_ctrl.b_cen_mod_en) {
					ife_set2dnr_center_modify(&p_filter_info->center_mod_set);
					ife_enable_function(ENABLE, IFE_CENTERMOD_EN);
				} else {
					ife_enable_function(DISABLE, IFE_CENTERMOD_EN);
				}
			}
			if (b_non_fdlatch == TRUE) {
				ife_set2dnr_nlm_parameter(&p_filter_info->rng_th_a);
				ife_set2dnr_bilat_parameter(&p_filter_info->rng_th_b);
			}
		} else {
			if (b_fdlatch == TRUE) {
				ife_enable_function(DISABLE, IFE_FILTER_EN);
			}
		}
		if (p_filter_info->ife_ctrl.b_vig_en) {
			if (b_fdlatch == TRUE) {
				ife_enable_function(ENABLE, IFE_VIG_EN);
			}
			if (b_non_fdlatch == TRUE) {
				ife_set_vignette(&p_filter_info->vig_set);
			}
		} else {
			if (b_fdlatch == TRUE) {
				ife_enable_function(DISABLE, IFE_VIG_EN);
			}
		}
		if (p_filter_info->ife_ctrl.b_gbal_en) {
			if (b_fdlatch == TRUE) {
				if (p_filter_info->bayer_format == 1) {
					ife_set_bayer_fmt(IFE_BAYER_RGBIR);
					ife_set_color_gain(&p_filter_info->cgain_set, p_filter_info->cfa_pat);
				} else {
					ife_set_bayer_fmt(IFE_BAYER_RGGB);
				}
				ife_enable_function(ENABLE, IFE_GBAL_EN);
			}
			if (b_non_fdlatch == TRUE) {
				ife_set_gbal(&p_filter_info->gbal_set);
			}
		} else {
			if (b_fdlatch == TRUE) {
				ife_enable_function(DISABLE, IFE_GBAL_EN);
			}
		}
	}
	return E_OK;
}
#endif
/**
  change IFE parameters

  Dynamically change IFE parameters when direct path

  @param[in] p_filt_para IFE paramters

  @return void
*/
ER ife_change_param(IFE_PARAM *pIFEPara)
{
	//IFE_ROWDEFSET Rdset;
	//UINT32 ip;
	//BOOL end = FALSE;
	UINT32 i;
	ER er_return;

	er_return = ife_chk_state_machine(IFE_OP_CHGPARAM, NOTUPDATE);
	if (ife_check_op_mode() != IFE_OPMODE_D2D) {

		//--------- IFE Ring Buffer setting ----------//
		if (pIFEPara->ife_set_sel[IFE_SET_RING_BUF]) {
			ife_set_ring_buf(&pIFEPara->ife_ringbuf_info);
		}
		if (pIFEPara->ife_set_sel[IFE_SET_WAIT_SIE2]) {
			ife_set_dma_wait_sie2_st_disable(pIFEPara->b_ife_dma_wait_sie2_st_disable);
		}
		//------------- IFE RDE setting ---------------//
		if (pIFEPara->ife_set_sel[IFE_SET_RDE]) {

			ife_set_rde_rate(&pIFEPara->rde_set);

			//ife_set_rde_default(&pIFEPara->rde_set);
			//ife_set_rde_ctrl(&pIFEPara->rde_set);
			//ife_set_rde_qtable_inx(&pIFEPara->rde_set);
			//ife_set_rde_rand_init(&pIFEPara->rde_set);
			//ife_set_rde_degamma(&pIFEPara->rde_set);
		}
		//------------- IFE NRS setting ---------------//
		if (pIFEPara->ife_set_sel[IFE_SET_NRS]) {
			ife_set_nrs_ctrl(&pIFEPara->nrs_set);
			ife_set_nrs_order(&pIFEPara->nrs_set);
			ife_set_nrs_bilat(&pIFEPara->nrs_set);
		}
		//------------ IFE Fcurve setting -------------//
		if (pIFEPara->ife_set_sel[IFE_SET_FCURVE]) {
			ife_set_fcurve_ctrl(&pIFEPara->fcurve_set);
			ife_set_fcurve_yweight(&pIFEPara->fcurve_set);
			ife_set_fcurve_index(&pIFEPara->fcurve_set);
			ife_set_fcurve_split(&pIFEPara->fcurve_set);
			ife_set_fcurve_value(&pIFEPara->fcurve_set);
		}

		//---------- IFE Fusion (Sensor HDR) ----------//
		if (pIFEPara->ife_set_sel[IFE_SET_FUSION]) {
			ife_set_fusion_ctrl(&pIFEPara->fusion_set);
			ife_set_fnum(pIFEPara->fusion_number);
			ife_set_fusion_blend_curve(&pIFEPara->fusion_set);
			ife_set_fusion_diff_weight(&pIFEPara->fusion_set);
			ife_set_fusion_dark_saturation_reduction(&pIFEPara->fusion_set);
			//--------- IFE S Compression setting ---------//
			ife_set_fusion_short_exposure_compress(&pIFEPara->fusion_set);
			ife_set_fusion_color_gain(&pIFEPara->fcgain_set);
		}

		if (pIFEPara->ife_set_sel[IFE_SET_OUTL]) {
			ife_set_outlier(&pIFEPara->outl_set);
		}

		if (pIFEPara->ife_set_sel[IFE_SET_FILTER]) {
			ife_set2dnr_spatial_w(pIFEPara->p_spatial_weight);
			ife_set_binning(pIFEPara->binn);
			ife_set2dnr_nlm_parameter(&pIFEPara->rng_th_a);
			ife_set2dnr_bilat_parameter(&pIFEPara->rng_th_b);

			ife_set2dnr_blend_weight(pIFEPara->bilat_w);
			ife_set2dnr_range_weight(pIFEPara->rth_w);
			ife_set_clamp(&pIFEPara->clamp_set);
			ife_set2dnr_center_modify(&pIFEPara->center_mod_set);
			ife_set2dnr_rbfill_ratio_mode(&pIFEPara->rb_fill_set);
		}

		if (pIFEPara->ife_set_sel[IFE_SET_CGAIN]) {
			ife_set_color_gain(&pIFEPara->cgain_set, pIFEPara->cfa_pat);
		}

		if (pIFEPara->ife_set_sel[IFE_SET_VIG] || pIFEPara->ife_set_sel[IFE_SET_VIG_CENTER]) {
			ife_set_vignette(&pIFEPara->vig_set);
		}
		if (pIFEPara->ife_set_sel[IFE_SET_GBAL]) {
			ife_set_gbal(&pIFEPara->gbal_set);
		}

		if (pIFEPara->ife_set_sel[IFE_SET_MIRROR] ||
			pIFEPara->ife_set_sel[IFE_SET_RDE]    ||
			pIFEPara->ife_set_sel[IFE_SET_NRS]    ||
			pIFEPara->ife_set_sel[IFE_SET_FUSION] ||
			pIFEPara->ife_set_sel[IFE_SET_FCURVE] ||
			pIFEPara->ife_set_sel[IFE_SET_OUTL]   ||
			pIFEPara->ife_set_sel[IFE_SET_FILTER] ||
			pIFEPara->ife_set_sel[IFE_SET_CGAIN]  ||
			pIFEPara->ife_set_sel[IFE_SET_VIG]    ||
			pIFEPara->ife_set_sel[IFE_SET_VIG_CENTER] ||
			pIFEPara->ife_set_sel[IFE_SET_GBAL]) {
			ife_set_ifecontrol(pIFEPara->ife_ctrl);
		}

		if (pIFEPara->ife_ctrl.b_fusion_en) {
			/*if (pIFEPara->ife_ctrl.b_fcgain_en == FALSE) {
				DBG_WRN("Fusion enable but FCgain not enable!!\r\n");
			}*/
			if (pIFEPara->ife_ctrl.b_fcurve_en == FALSE) {
				DBG_WRN("Fusion enable but Fcurve not enable!!\r\n");
			}
		} else {
			/*if (pIFEPara->ife_ctrl.b_fcgain_en == TRUE) {
				DBG_WRN("Fusion not enable but FCgain enable!!\r\n");
			}*/
			if (pIFEPara->ife_ctrl.b_scompression_en == TRUE) {
				DBG_WRN("Fusion not enable but S_compression enable!!\r\n");
			}
		}
		for (i = IFE_SET_MIRROR; i < IFE_SET_ALL; i++) {
			pIFEPara->ife_set_sel[i] = FALSE;
		}

		ife_set_load();

		er_return = ife_chk_state_machine(IFE_OP_CHGPARAM, UPDATE);
	} else {
		DBG_ERR("ife_change_param() doesn't support D2D mode\r\n");
	}

	return er_return;
}

/**
  change IFE Size

  Dynamically change IFE size when direct path

  @param[in] p_size_para IFE paramters

  @return void
*/
/*
void ife_change_size(IFE_PARAM *p_size_para)
{
    ife_set_in_size(p_size_para->width, p_size_para->height);
    ife_set_load();
}
*/
/**
  change IFE ROI

  Dynamically change IFE Address and ROI

  @param[in] p_size_para IFE paramters

  @return void
*/
ER ife_change_roi(IFE_PARAM *pROIPara)
{
	ER er_return;
	UINT32 phy_addr_in0, phy_addr_in1, phy_addr_out;
	er_return = ife_chk_state_machine(IFE_OP_CHGSIZE, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	if (pROIPara->in_addr0 != 0) {
		phy_addr_in0 = ife_platform_va2pa(pROIPara->in_addr0);
	} else {
		phy_addr_in0 = pROIPara->in_addr0;
	}
	if (pROIPara->in_addr1 != 0) {
		phy_addr_in1 = ife_platform_va2pa(pROIPara->in_addr1);
	} else {
		phy_addr_in1 = pROIPara->in_addr1;
	}
	if (pROIPara->out_addr != 0) {
		phy_addr_out = ife_platform_va2pa(pROIPara->out_addr);
	} else {
		phy_addr_out = pROIPara->out_addr;
	}

	switch (ife_check_op_mode()) {
	case IFE_OPMODE_D2D:
		//phy_addr_out   = ife_platform_va2pa(pROIPara->out_addr);
		ife_set_dma_in_addr(phy_addr_in0, pROIPara->in_ofs0);
		ife_set_dma_in_addr_2(phy_addr_in1, pROIPara->in_ofs1);
		ife_set_dma_out_addr(phy_addr_out, pROIPara->out_ofs);
		break;
	case IFE_OPMODE_IPP:
		ife_set_dma_in_addr(phy_addr_in0, pROIPara->in_ofs0);
		ife_set_dma_in_addr_2(phy_addr_in1, pROIPara->in_ofs1);
		break;
	case IFE_OPMODE_ALL_DIRECT:
		ife_set_dma_in_addr(phy_addr_in0, pROIPara->in_ofs0);
		ife_set_dma_in_addr_2(phy_addr_in1, pROIPara->in_ofs1);
		pROIPara->out_bit = IFE_12BIT;
		break;
	default :
		DBG_ERR("IFE changeROI Operation ERROR!!\r\n");
		break;
	}
	ife_set_in_size(pROIPara->width, pROIPara->height);
	ife_set_crop(pROIPara->crop_width, pROIPara->crop_height, pROIPara->crop_hpos, pROIPara->crop_vpos);
	ife_bitsel(pROIPara->in_bit, pROIPara->out_bit);
	ife_set_cfa_pat(pROIPara->cfa_pat);

	ife_set_load();

	er_return = ife_chk_state_machine(IFE_OP_CHGSIZE, UPDATE);

	return er_return;
}

/**
    IFE Enable Functions

    Enable/Disable selected IFE functions

    @param[in] b_enable choose to enable or disable specified functions.
        \n-@b ENABLE: to enable.
        \n-@b DISABLE: to disable.
    @param[in] ui_func indicate the function(s)

    @return None.
*/
ER ife_enable_function(BOOL b_enable, UINT32 ui_func)
{
	T_CONTROL_REGISTER localreg;
	localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);

	ui_func &= (IFE_FUNC_ALL);

	if (b_enable) {
		localreg.reg |= ui_func;
	} else {
		localreg.reg &= (~ui_func);
	}
	IFE_SETREG(CONTROL_REGISTER_OFS, localreg.reg);

	return E_OK;
}

/**
    IFE Check Function Enable/Disable Status

    Check if the one specified IFE funtion is enabled or not

    @param[in] ui_func indicate the function to check

    @return
        \n-@b ENABLE: the function is enabled.
        \n-@b DISABLE: the function is disabled.
*/
BOOL ife_check_function_enable(UINT32 ui_func)
{
	T_CONTROL_REGISTER localreg;
	UINT32 ui_func_st;

	localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);

	ui_func_st = localreg.reg;

	return (BOOL)((ui_func_st & ui_func) != 0);
}

/**
  Wait IFE Processing done

  Wait IFE Processing done

  @return void
*/
void ife_wait_flag_frame_end(void)
{
	FLGPTN ui_flag;
	T_FILTER_INTERRUPT_STATUS_REGISTER  localreg;

	if (g_b_polling) {
		localreg.reg = 0;
		while (localreg.bit.int_frmend != 1) {
			localreg.reg = IFE_GETREG(FILTER_INTERRUPT_STATUS_REGISTER_OFS);
		}
		ife_clear_int(IFE_INT_FRMEND);

	} else {
#if defined (_NVT_EMULATION_)
		if (timer_open(&ife_timer_id, ife_time_out_cb) != E_OK) {
			DBG_WRN("IFE allocate timer fail\r\n");
		} else {
			timer_cfg(ife_timer_id, 6000000, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		}
#endif

		ife_platform_flg_wait(&ui_flag, FLGPTN_IFE_FRAMEEND);
		DBG_IND("ife_wait_flag_frame_end done \r\n");
		//g_uiIFEngine_status = IFE_ENGINE_READY;

#if defined (_NVT_EMULATION_)
		timer_close(ife_timer_id);
#endif
	}
}

/*
  Reset IFE

  Reset IFE

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER ife_clr(void)
{
	volatile T_FILTER_OPERATION_CONTROL_REGISTER    localreg;

	localreg.reg = IFE_GETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS);
	localreg.bit.ife_sw_rst = 1;
	IFE_SETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS, localreg.reg);
	while (1) {
		localreg.reg = IFE_GETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS);
		if (!localreg.bit.ife_sw_rst) {
			break;
		}
	}

	err_msg_cnt[0] = 0;
	err_msg_cnt[1] = 0;
	err_msg_cnt[2] = 0;
	err_msg_cnt[3] = 0;
	err_msg_cnt[4] = 0;
	err_msg_cnt[5] = 0;
	frame_cnt = 0;

	return E_OK;
}

/*
  Start IFE

  Set IFE start bit to 1.

  @param None.

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER ife_start(void)
{
	ER er_return;
	T_FILTER_OPERATION_CONTROL_REGISTER    localreg;

	er_return = ife_chk_state_machine(IFE_OP_START, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

#if IFE_POWER_SAVING_EN
    if(ife_fw_power_saving_en){
    	//for power saving
    	if (ife_mode != IFE_OPMODE_ALL_DIRECT) {
    		//pll_disable_sram_shut_down(IFE_RSTN);//enable SRAM
    		ife_platform_disable_sram_shutdown();
    		ife_attach();//enable engine clock
    	}
    }
#endif

#if 0  // remove for builtin
	ife_platform_flg_clear(FLGPTN_IFE_FRAMEEND);
	er_return = ife_chk_state_machine(IFE_OP_START, UPDATE);
	if (er_return != E_OK) {
		return er_return;
	}
	localreg.reg = IFE_GETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS);
	localreg.bit.ife_start = 1;
	localreg.bit.ife_load_start = 1;
	IFE_SETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS, localreg.reg);
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ife_ctrl_flow_to_do == ENABLE) {
		ife_platform_flg_clear(FLGPTN_IFE_FRAMEEND);
		er_return = ife_chk_state_machine(IFE_OP_START, UPDATE);
		if (er_return != E_OK) {
			return er_return;
		}
		localreg.reg = IFE_GETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS);
		localreg.bit.ife_start = 1;
		localreg.bit.ife_load_start = 1;
        #if (defined(_NVT_EMULATION_) == OFF)
            dmaloop_enable_layer1 = (IFE_GETREG(0x5c) & 0x01000000) >> 24;
        #endif
		IFE_SETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS, localreg.reg);
	} else {
		if (ife_mode == IFE_OPMODE_ALL_DIRECT) {
			er_return = ife_chk_state_machine(IFE_OP_START, UPDATE);
			#if (defined(_NVT_EMULATION_) == OFF)
				dmaloop_enable_layer1 = (IFE_GETREG(0x5c) & 0x01000000) >> 24;
			#endif
			if (er_return != E_OK) {
				return er_return;
			}
		} else {
			ife_platform_flg_clear(FLGPTN_IFE_FRAMEEND);
			er_return = ife_chk_state_machine(IFE_OP_START, UPDATE);
			if (er_return != E_OK) {
				return er_return;
			}
			localreg.reg = IFE_GETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS);
			localreg.bit.ife_start = 1;
			localreg.bit.ife_load_start = 1;
            #if (defined(_NVT_EMULATION_) == OFF)
                dmaloop_enable_layer1 = (IFE_GETREG(0x5c) & 0x01000000) >> 24;
            #endif
			IFE_SETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS, localreg.reg);
		}
	}
#else
	ife_platform_flg_clear(FLGPTN_IFE_FRAMEEND);
	er_return = ife_chk_state_machine(IFE_OP_START, UPDATE);
	if (er_return != E_OK) {
		return er_return;
	}
	localreg.reg = IFE_GETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS);
	localreg.bit.ife_start = 1;
	localreg.bit.ife_load_start = 1;
    #if (defined(_NVT_EMULATION_) == OFF)
        dmaloop_enable_layer1 = (IFE_GETREG(0x5c) & 0x01000000) >> 24;
    #endif
	IFE_SETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS, localreg.reg);
#endif

	ife_ctrl_flow_to_do = ENABLE;

#if defined (_NVT_EMULATION_)
	ife_end_time_out_status = FALSE;
#endif

	return E_OK;
}

ER ife_dbg_start(UINT32 loadtype)
{
	ER er_return;
	T_FILTER_OPERATION_CONTROL_REGISTER    localreg;

	er_return = ife_chk_state_machine(IFE_OP_START, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}
	ife_platform_flg_clear(FLGPTN_IFE_FRAMEEND);
	er_return = ife_chk_state_machine(IFE_OP_START, UPDATE);
	if (er_return != E_OK) {
		return er_return;
	}
	localreg.reg = IFE_GETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS);
	localreg.bit.ife_start = 1;
	if (loadtype == 1) {
		localreg.bit.ife_load_start = 1;
		DBG_WRN("IFE Start load\r\n");
	} else if (loadtype == 2) {
		localreg.bit.ife_load_fd = 1;
		DBG_WRN("IFE FD load\r\n");
	}
	//DBG_ERR("IFE Start!!\r\n");
	//DBG_WRN( "IFE start reg %x\r\n", localreg.reg);
	IFE_SETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS, localreg.reg);

	return E_OK;
}

static ER ife_stop(void)
{
	T_FILTER_OPERATION_CONTROL_REGISTER    localreg;

	localreg.reg = IFE_GETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS);
	localreg.bit.ife_start = 0;
	IFE_SETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS, localreg.reg);
	return E_OK;
}

/**
    IFE pause

    Pause IFE operation

    @param None.

    @return ER IFE pause status.
*/
ER ife_pause(void)
{
	ER      er_return;

	er_return = ife_chk_state_machine(IFE_OP_PAUSE, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	ife_stop();

#if IFE_POWER_SAVING_EN
    if(ife_fw_power_saving_en){
    	//for power saving
    	if (ife_mode != IFE_OPMODE_ALL_DIRECT) {
    		//pll_enable_sram_shut_down(IFE_RSTN);//disable SRAM
    		ife_platform_enable_sram_shutdown();
    		ife_detach();//disable engine clock
    	}
    }
#endif

	er_return = ife_chk_state_machine(IFE_OP_PAUSE, UPDATE);
	if (er_return != E_OK) {
		return er_return;
	}
#if (defined(_NVT_EMULATION_) == OFF)
	g_ife_first_frame_flg = TRUE;
#endif

	return E_OK;
}

/**
    IFE Reset

    Reset IFE operation

    @param None.

    @return ER IFE reset status.
*/
ER ife_reset(VOID)
{
	ER erReturn;

	erReturn = ife_chk_state_machine(IFE_OP_HWRESET, NOTUPDATE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	ife_stop();

	ife_detach();
	ife_platform_unprepare_clk();

	ife_platform_prepare_clk();
	ife_attach();

	erReturn = ife_platform_flg_clear(FLGPTN_IFE_FRAMEEND);
	if (erReturn != E_OK) {
		return erReturn;
	}
	// Clear engine interrupt status
	ife_clear_int(IFE_INT_ALL);
	ife_clr();

	ife_ring_buf_msg_flag = FALSE;
	
	erReturn = ife_chk_state_machine(IFE_OP_HWRESET, UPDATE);
	if (erReturn != E_OK) {
		return erReturn;
	}
	return E_OK;
}

/**
    IFE load

    Set IFE load bit to 1.

    @param None.

    @return None.
*/
static void ife_set_load(void)
{
	T_FILTER_OPERATION_CONTROL_REGISTER localreg;

	localreg.reg = IFE_GETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS);
	if (g_ui_ife_engine_status == IFE_ENGINE_RUN) {
		if (ife_mode == IFE_OPMODE_ALL_DIRECT) {
			localreg.bit.ife_load_fd = 0;
			localreg.bit.ife_load_frmstart = 1;
		} else {
			localreg.bit.ife_load_fd = 1;
			localreg.bit.ife_load_frmstart = 0;
		}
		IFE_SETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS, localreg.reg);
	}
}


/**
  Check IFE Operation mode

  Check if IFE operates from DRAM to DRAM or IFE is embedded in IPE

  @return
        \n-@b IFE_OPMODE_D2D:       Input from DRAM and output to DRAM.
        \n-@b IFE_OPMODE_SIE_IPP: All Direct path from SIE.
        \n-@b IFE_OPMODE_RHE_IPP :  RHE input and DCE output.
*/
IFE_OPMODE ife_check_op_mode(void)
{
	T_CONTROL_REGISTER localreg;

	localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);
	if (localreg.bit.ife_mode == 0) {
		return IFE_OPMODE_D2D;
	} else if (localreg.bit.ife_mode == 1) {
		return IFE_OPMODE_IPP;
	} else {
		return IFE_OPMODE_ALL_DIRECT;
	}
#if 0
	if (localreg.bit.IFE_OUT_PATH == IFE_TO_DRAM) {
		if (localreg.bit.IFE_IN_PATH == IFE_FROM_DRAM) {
			return IFE_OPMODE_D2D;
		} else {
			nvt_dbg(ERR, "IFE In/Out Operation ERROR!!\r\n");
			return IFE_OPMODE_UNKNOWN;
		}
	} else {
		if (localreg.bit.IFE_IN_PATH == IFE_FROM_DRAM) {
			nvt_dbg(ERR, "IFE In/Out Operation ERROR!!\r\n");
			return IFE_OPMODE_UNKNOWN;
		}
		//else if(localreg.bit.IFE_IN_PATH == IFE_FROM_RDE)//removed in NT96680
		//{
		//    return IFE_OPMODE_RDE_IPP;
		//}
		else if (localreg.bit.IFE_IN_PATH == IFE_FROM_RHE) {
			return IFE_OPMODE_RHE_IPP;
		} else {
			return IFE_OPMODE_SIE_IPP;
		}
	}
#endif
}

//----Layer 0 Static function-------------------------------------------------------------------------
/*
  Check IFE State Machine

  This function check IFE state machine

  @param[in] Op:        operation mode
  @param[in] Update:    IFE Operation update state machine

  @return
        - @b E_OK:      Done with no error.
        - Others:       Error occured.
*/
static ER ife_chk_state_machine(IFEOPERATION Op, IFESTATUSUPDATE Update)
{
	switch (Op) {
	case IFE_OP_OPEN:
		switch (g_ui_ife_engine_status) {
		case IFE_ENGINE_IDLE:
			if (Update) {
				g_ui_ife_engine_status = IFE_ENGINE_READY;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("IFE Operation ERROR!!Operate %d from State %u\r\n", (unsigned int)Op, (unsigned int)g_ui_ife_engine_status);
			return E_SYS;
			break;
		}
		break;
	case IFE_OP_CLOSE:
		switch (g_ui_ife_engine_status) {
		case IFE_ENGINE_PAUSE:
		case IFE_ENGINE_READY:
			if (Update) {
				g_ui_ife_engine_status = IFE_ENGINE_IDLE;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("IFE Operation ERROR!!Operate %d from State %d\r\n", (unsigned int)Op, (unsigned int)g_ui_ife_engine_status);
			return E_SYS;
			break;
		}
		break;
	case IFE_OP_SETMODE:
		switch (g_ui_ife_engine_status) {
		case IFE_ENGINE_READY:
		case IFE_ENGINE_PAUSE:
			if (Update) {
				g_ui_ife_engine_status = IFE_ENGINE_PAUSE;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("IFE Operation ERROR!!Operate %d from State %d\r\n", (unsigned int)Op, (unsigned int)g_ui_ife_engine_status);
			return E_SYS;
			break;
		}
		break;
	case IFE_OP_START:
		switch (g_ui_ife_engine_status) {
		case IFE_ENGINE_PAUSE:
			if (Update) {
				g_ui_ife_engine_status = IFE_ENGINE_RUN;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("IFE Operation ERROR!!Operate %d from State %d\r\n", (unsigned int)Op, (unsigned int)g_ui_ife_engine_status);
			return E_SYS;
			break;
		}
		break;
	case IFE_OP_CHGPARAM:
		switch (g_ui_ife_engine_status) {
		case IFE_ENGINE_READY:
		case IFE_ENGINE_PAUSE:
		case IFE_ENGINE_RUN:
			return E_OK;
			break;
		default:
			DBG_ERR("IFE Operation ERROR!!Operate %d from State %d\r\n", (unsigned int)Op, (unsigned int)g_ui_ife_engine_status);
			return E_SYS;
			break;
		}
		break;
	case IFE_OP_CHGSIZE:
		switch (g_ui_ife_engine_status) {
		case IFE_ENGINE_READY:
		case IFE_ENGINE_PAUSE:
		case IFE_ENGINE_RUN:
			return E_OK;
			break;
		default:
			DBG_ERR("IFE Operation ERROR!!Operate %d from State %d\r\n", (unsigned int)Op, (unsigned int)g_ui_ife_engine_status);
			return E_SYS;
			break;
		}
		break;
	case IFE_OP_PAUSE:
		switch (g_ui_ife_engine_status) {
		case IFE_ENGINE_RUN:
		case IFE_ENGINE_PAUSE:
			if (Update) {
				g_ui_ife_engine_status = IFE_ENGINE_PAUSE;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("IFE Operation ERROR!!Operate %d from State %d\r\n", (unsigned int)Op, (unsigned int)g_ui_ife_engine_status);
			return E_SYS;
			break;
		}
		break;
	case IFE_OP_HWRESET:
		switch (g_ui_ife_engine_status) {
		case IFE_ENGINE_IDLE:
		case IFE_ENGINE_READY:
		case IFE_ENGINE_PAUSE:
		case IFE_ENGINE_RUN:
			if (Update) {
				g_ui_ife_engine_status = IFE_ENGINE_PAUSE;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("IFE Operation ERROR!!Operate %d from State %d\r\n", (unsigned int)Op, (unsigned int)g_ui_ife_engine_status);
			return E_SYS;
			break;
		}
		break;
	default :
		DBG_ERR("IFE Operation ERROR!!\r\n");
		return E_SYS;
		break;
	}
	return E_OK;
}

/*
    IFE attach.

    attach IFE.

    @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
static ER ife_attach(void)
{
	if (g_ife_clk_en == FALSE) {
		ife_platform_enable_clk();
		g_ife_clk_en = TRUE;
	}
	return E_OK;
}

/*
    IFE detach.

    detach IFE.

    @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
static ER ife_detach(void)
{
	if (g_ife_clk_en == TRUE) {
		ife_platform_disable_clk();
		g_ife_clk_en = FALSE;
	}
	return E_OK;
}

/*
  Lock IFE

  This function lock IFE module

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
static ER ife_lock(void)
{
	ER er_return;

	er_return = ife_platform_sem_wait();

	if (er_return != E_OK) {
		return er_return;
	}

	g_ife_lock_status = TASK_LOCKED;

	return E_OK;
}

/*
  Unlock IFE

  This function unlock IFE module

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
static ER ife_unlock(void)
{
	ER er_return;

	g_ife_lock_status = NO_TASK_LOCKED;
	er_return = ife_platform_sem_signal();

	return er_return;
}

/*
  Set IFE register

  Select IFE operation mode and related register

  @param[in] IFE_OPMODE IPP operation mode

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER ife_set_op_mode(IFE_OPMODE mode)
{
	T_CONTROL_REGISTER localreg;

	if (!ife_is_busy()) {
		localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);
		localreg.bit.ife_mode = mode;
		IFE_SETREG(CONTROL_REGISTER_OFS, localreg.reg);
		ife_mode = mode;
		return E_OK;
	} else {
		if (mode == IFE_OPMODE_D2D) {
			DBG_ERR("Please change IFE mode at IFE frame end.\r\n");
		} else { // if(mode == IFE_OPMODE_DIRECT)
			DBG_ERR("Cannot change IFE mode when IFE is busy.\r\n");
		}
		return E_SYS;
	}
}

static ER ife_set_ifecontrol(IFE_CONTROL func_enable)
{

	T_CONTROL_REGISTER localreg;
	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1 localreg1;

	localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);
	localreg1.reg = IFE_GETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1_OFS);


	localreg.bit.outl_en          = func_enable.b_outl_en;
	localreg.bit.filter_en        = func_enable.b_filter_en;
	localreg.bit.cgain_en         = func_enable.b_cgain_en;
	localreg.bit.vig_en           = func_enable.b_vig_en;
	localreg.bit.gbal_en          = func_enable.b_gbal_en;
	localreg.bit.rgbir_rb_nrfill  = func_enable.b_rb_fill_en;
	localreg.bit.bilat_th_en      = func_enable.b_cen_mod_en;
	localreg.bit.f_nrs_en         = func_enable.b_nrs_en;
	localreg.bit.f_cg_en          = func_enable.b_fcgain_en;
	localreg.bit.f_fusion_en      = func_enable.b_fusion_en;
	localreg.bit.f_fc_en          = func_enable.b_fcurve_en;
	localreg.bit.mirror_en        = func_enable.b_mirror_en;
	localreg.bit.r_decode_en      = func_enable.b_decode_en;

	localreg1.bit.ife_f_fusion_short_exposure_compress_enable = func_enable.b_scompression_en;

	IFE_SETREG(CONTROL_REGISTER_OFS, localreg.reg);
	IFE_SETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1_OFS, localreg1.reg);

	return E_OK;
}
static ER ife_set_fnum(UINT8 fusion_num)
{
	T_CONTROL_REGISTER localreg;
	localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);
	localreg.bit.f_fusion_fnum = fusion_num;
	IFE_SETREG(CONTROL_REGISTER_OFS, localreg.reg);
	return E_OK;
}

static ER ife_set_stripe(IFE_STRIPE stripe_info)
{
	T_STRIPE_REGISTER localreg;

	localreg.reg = IFE_GETREG(STRIPE_REGISTER_OFS);
	localreg.bit.ife_hn = stripe_info.hn;
	localreg.bit.ife_hl = stripe_info.hl;
	localreg.bit.ife_hm = stripe_info.hm;
	IFE_SETREG(STRIPE_REGISTER_OFS, localreg.reg);
	return E_OK;
}

static ER ife_set_dma_wait_sie2_st_disable(BOOL ife_dma_wait_sie2_st_disable)
{
	T_DEBUG_REGISTER localreg;
	localreg.reg = IFE_GETREG(DEBUG_REGISTER_OFS);
	localreg.bit.ife_dma_wait_sie2_start_disable = ife_dma_wait_sie2_st_disable;
	IFE_SETREG(DEBUG_REGISTER_OFS, localreg.reg);
	return E_OK;
}


/**
  Select input CFA Pattern

  Select input CFA pattern

  @param[in] cfa_pat CFA pattern 0: R  GR  1: GR R   2: GB B   3: B  GB
                                   GB B      B  GB     R  GR     GR R

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_cfa_pat(IFE_CFASEL cfa_pat)
{
	T_CONTROL_REGISTER localreg;

	localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);
	localreg.bit.cfapat = cfa_pat;
	IFE_SETREG(CONTROL_REGISTER_OFS, localreg.reg);

	return E_OK;
}

/**
  Select input Bayer format

  Select input Bayer format

  @param[in]

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_bayer_fmt(IFE_BAYERFMTSEL bayer_fmt_sel)
{
	T_CONTROL_REGISTER localreg;
	localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);
	localreg.bit.bayer_fmt = bayer_fmt_sel;
	IFE_SETREG(CONTROL_REGISTER_OFS, localreg.reg);

	return E_OK;
}

/**
  Select input/output bitdepth

  Select input/output bitdepth

  @param[in] InBitDepth   input bitdepth 0: 8bit; 1:12bit ;2:16bit
  @param[in] OutBitDepth  output bitdepth 0: 8bit; 1:12bit ;2:16bit
         outbit cannot be '1' if inbit equal to '2'

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_bitsel(IFE_BITDEPTH in_bit_depth, IFE_BITDEPTH out_bit_depth)
{
	T_CONTROL_REGISTER localreg;

	localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);
	localreg.bit.inbit_depth = in_bit_depth;
	localreg.bit.outbit_depth = out_bit_depth;

	IFE_SETREG(CONTROL_REGISTER_OFS, localreg.reg);

	return E_OK;
}

/**
  Setting ring buffer en and dram line

  Setting ring buffer en and dram line

  @param[in]

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_ring_buf(IFE_RING_BUF *ring_info)
{
	T_DRAM_SETTINGS localreg;

	localreg.reg = IFE_GETREG(DRAM_SETTINGS_OFS);
	localreg.bit.dmaloop_en   = ring_info->dmaloop_en;
	localreg.bit.dmaloop_line = ring_info->dmaloop_line;
	IFE_SETREG(DRAM_SETTINGS_OFS, localreg.reg);

	return E_OK;
}

/*
  Set IFE spatial weighting

  Set IFE spatial weighting

  @param[in] *weights 10 spatial weighting coefficients for 9x9 area, each can be 0~31
           w9 w8 w7 w7 w7 w7 w7 w8 w9
           w8 w7 w6 w5 w5 w5 w6 w7 w8
           w7 w6 w5 w4 w3 w4 w5 w6 w7
           w7 w5 w4 w2 w1 w2 w4 w5 w7
         [ w7 w5 w3 w1 w0 w1 w3 w5 w7 ]
           w7 w5 w4 w2 w1 w2 w4 w5 w7
           w7 w6 w5 w4 w3 w4 w5 w6 w7
           w8 w7 w6 w5 w5 w5 w6 w7 w8
           w9 w8 w7 w7 w7 w7 w7 w8 w9
  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set2dnr_spatial_w(UINT32 *p_spatial_weight)
{
	T_SPATIAL_FILTER_REGISTER_1 localreg1;
	T_SPATIAL_FILTER_REGISTER_2 localreg2;

	localreg1.reg = IFE_GETREG(SPATIAL_FILTER_REGISTER_1_OFS);
	localreg1.bit.ife_spatial_weight0 = p_spatial_weight[0];
	localreg1.bit.ife_spatial_weight1 = p_spatial_weight[1];
	localreg1.bit.ife_spatial_weight2 = p_spatial_weight[2];
	localreg1.bit.ife_spatial_weight3 = p_spatial_weight[3];
	IFE_SETREG(SPATIAL_FILTER_REGISTER_1_OFS, localreg1.reg);

	localreg2.reg = IFE_GETREG(SPATIAL_FILTER_REGISTER_2_OFS);
	localreg2.bit.ife_spatial_weight4 = p_spatial_weight[4];
	localreg2.bit.ife_spatial_weight5 = p_spatial_weight[5];
	IFE_SETREG(SPATIAL_FILTER_REGISTER_2_OFS, localreg2.reg);

	return E_OK;
}
#if 0//removed in NT98520
/*
  Set spatial filter only

  Set spatial filter only

  @param[in] en         0:disable; 1:enable
  @param[in] length     spatial filter only length

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_sonly(BOOL b_enable, IFE_SONLYLEN length)
{
#if 0
	T_SPATIAL_FILTER_REGISTER_1 localreg;
	T_SPATIAL_FILTER_REGISTER_2 localreg2;

	localreg.reg = IFE_GETREG(SPATIAL_FILTER_REGISTER_1_OFS);
	localreg.bit.S_ONLY_EN = b_enable;
	IFE_SETREG(SPATIAL_FILTER_REGISTER_1_OFS, localreg.reg);

	localreg2.reg = IFE_GETREG(SPATIAL_FILTER_REGISTER_2_OFS);
	localreg2.bit.S_ONLY_LEN = length;
	IFE_SETREG(SPATIAL_FILTER_REGISTER_2_OFS, localreg2.reg);
#endif
	return E_OK;
}
#endif
#if 0//removed in NT96680
/*
  Set filter mode

  Set filter mode

  @param[in] filter_mode  filter mode

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_filtermode(IFE_FILTMODE filter_mode)
{
	T_CONTROL_REGISTER localreg;

	localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);
	localreg.bit.FILT_MODE = filter_mode;
	IFE_SETREG(CONTROL_REGISTER_OFS, localreg.reg);

	return E_OK;
}
#endif
/*
  Set binning

  Set binning

  @param[in] binn_sel  binning level

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_binning(IFE_BINNSEL binn_sel)
{
	T_CONTROL_REGISTER    localreg;

	localreg.reg = IFE_GETREG(CONTROL_REGISTER_OFS);
	localreg.bit.binning = binn_sel;
	IFE_SETREG(CONTROL_REGISTER_OFS, localreg.reg);

	return E_OK;
}


static ER ife_set_rde_default(IFE_RDESET *p_rde)
{
#if (defined(_NVT_EMULATION_) == OFF)
	p_rde->b_degamma_en   = 1;
	p_rde->b_dithering_en = 1;
	//p_rde->encode_rate   = 0;
	p_rde->dither_reset  = 0;

	p_rde->rand1_init1 = 0xA;
	p_rde->rand1_init2 = 0x2AAA;
	p_rde->rand2_init1 = 0xA;
	p_rde->rand2_init2 = 0x2AAA;

	p_rde->p_dct_qtable_inx[0] = 0;
	p_rde->p_dct_qtable_inx[1] = 4;
	p_rde->p_dct_qtable_inx[2] = 8;
	p_rde->p_dct_qtable_inx[3] = 0xc;
	p_rde->p_dct_qtable_inx[4] = 0x10;
	p_rde->p_dct_qtable_inx[5] = 0x14;
	p_rde->p_dct_qtable_inx[6] = 0x18;
	p_rde->p_dct_qtable_inx[7] = 0x1c;

	p_rde->p_degamma_table[0]  = 0;
	p_rde->p_degamma_table[1]  = 32;
	p_rde->p_degamma_table[2]  = 64;
	p_rde->p_degamma_table[3]  = 96;
	p_rde->p_degamma_table[4]  = 128;
	p_rde->p_degamma_table[5]  = 160;
	p_rde->p_degamma_table[6]  = 192;
	p_rde->p_degamma_table[7]  = 224;
	p_rde->p_degamma_table[8]  = 256;
	p_rde->p_degamma_table[9]  = 324;
	p_rde->p_degamma_table[10] = 400;
	p_rde->p_degamma_table[11] = 484;
	p_rde->p_degamma_table[12] = 576;
	p_rde->p_degamma_table[13] = 676;
	p_rde->p_degamma_table[14] = 784;
	p_rde->p_degamma_table[15] = 900;
	p_rde->p_degamma_table[16] = 1024;
	p_rde->p_degamma_table[17] = 1156;
	p_rde->p_degamma_table[18] = 1296;
	p_rde->p_degamma_table[19] = 1444;
	p_rde->p_degamma_table[20] = 1600;
	p_rde->p_degamma_table[21] = 1764;
	p_rde->p_degamma_table[22] = 1936;
	p_rde->p_degamma_table[23] = 2116;
	p_rde->p_degamma_table[24] = 2304;
	p_rde->p_degamma_table[25] = 2500;
	p_rde->p_degamma_table[26] = 2704;
	p_rde->p_degamma_table[27] = 2916;
	p_rde->p_degamma_table[28] = 3136;
	p_rde->p_degamma_table[29] = 3364;
	p_rde->p_degamma_table[30] = 3600;
	p_rde->p_degamma_table[31] = 3844;
	p_rde->p_degamma_table[32] = 4095;
#endif
	return E_OK;
}
/*
  Set RDE

  Set RDE

  @param[in] pRDE  set RDE control

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_rde_ctrl(IFE_RDESET *p_rde)
{
	T_RDE_CONTROL    localreg;

	localreg.reg = IFE_GETREG(RDE_CONTROL_OFS);
	localreg.bit.ife_r_degamma_en = p_rde->b_degamma_en;
	localreg.bit.ife_r_dith_en    = p_rde->b_dithering_en;
	localreg.bit.ife_r_dith_rst   = p_rde->dither_reset;
	IFE_SETREG(RDE_CONTROL_OFS, localreg.reg);

	return E_OK;
}

/*
  Set RDE

  Set RDE

  @param[in] pRDE  set RDE encode rate

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_rde_rate(IFE_RDESET *p_rde)
{
	T_RDE_CONTROL    localreg;
	localreg.reg = IFE_GETREG(RDE_CONTROL_OFS);
	localreg.bit.ife_r_segbitno   = p_rde->encode_rate;
	IFE_SETREG(RDE_CONTROL_OFS, localreg.reg);

	return E_OK;
}

/*
  Set RDE

  Set RDE

  @param[in] pRDE  set RDE DCT Q table index

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_rde_qtable_inx(IFE_RDESET *p_rde)
{
	T_DCT_QTBL_REGISTER0   localreg0;
	T_DCT_QTBL_REGISTER1   localreg1;

	localreg0.reg = IFE_GETREG(DCT_QTBL_REGISTER0_OFS);
	localreg1.reg = IFE_GETREG(DCT_QTBL_REGISTER1_OFS);

	localreg0.bit.ife_r_dct_qtbl0_idx  = p_rde->p_dct_qtable_inx[0];
	localreg0.bit.ife_r_dct_qtbl1_idx  = p_rde->p_dct_qtable_inx[1];
	localreg0.bit.ife_r_dct_qtbl2_idx  = p_rde->p_dct_qtable_inx[2];
	localreg0.bit.ife_r_dct_qtbl3_idx  = p_rde->p_dct_qtable_inx[3];
	localreg1.bit.ife_r_dct_qtbl4_idx  = p_rde->p_dct_qtable_inx[4];
	localreg1.bit.ife_r_dct_qtbl5_idx  = p_rde->p_dct_qtable_inx[5];
	localreg1.bit.ife_r_dct_qtbl6_idx  = p_rde->p_dct_qtable_inx[6];
	localreg1.bit.ife_r_dct_qtbl7_idx  = p_rde->p_dct_qtable_inx[7];

	IFE_SETREG(DCT_QTBL_REGISTER0_OFS, localreg0.reg);
	IFE_SETREG(DCT_QTBL_REGISTER1_OFS, localreg1.reg);

	return E_OK;
}

/*
  Set RDE

  Set RDE

  @param[in] p_rde  set RDE Dithering initial

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_rde_rand_init(IFE_RDESET *p_rde)
{
	T_DITHERING_INITIAL_REGISTER0   localreg0;
	T_DITHERING_INITIAL_REGISTER1   localreg1;

	localreg0.reg = IFE_GETREG(DITHERING_INITIAL_REGISTER0_OFS);
	localreg1.reg = IFE_GETREG(DITHERING_INITIAL_REGISTER1_OFS);

	localreg0.bit.ife_r_out_rand1_init1  = p_rde->rand1_init1;
	localreg0.bit.ife_r_out_rand1_init2  = p_rde->rand1_init2;
	localreg1.bit.ife_r_out_rand2_init1  = p_rde->rand2_init1;
	localreg1.bit.ife_r_out_rand2_init2  = p_rde->rand2_init2;

	IFE_SETREG(DITHERING_INITIAL_REGISTER0_OFS, localreg0.reg);
	IFE_SETREG(DITHERING_INITIAL_REGISTER1_OFS, localreg1.reg);

	return E_OK;
}

/*
  Set RDE

  Set RDE

  @param[in] p_rde  set RDE degamma

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_rde_degamma(IFE_RDESET *p_rde)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_DEGAMMA_TBL_REGISTER0   localreg0;
	T_DEGAMMA_TBL_REGISTER1   localreg1;
	T_DEGAMMA_TBL_REGISTER2   localreg2;
	T_DEGAMMA_TBL_REGISTER3   localreg3;
	T_DEGAMMA_TBL_REGISTER4   localreg4;
	T_DEGAMMA_TBL_REGISTER5   localreg5;
	T_DEGAMMA_TBL_REGISTER6   localreg6;
	T_DEGAMMA_TBL_REGISTER7   localreg7;
	T_DEGAMMA_TBL_REGISTER8   localreg8;
	T_DEGAMMA_TBL_REGISTER9   localreg9;
	T_DEGAMMA_TBL_REGISTER10  localreg10;
	T_DEGAMMA_TBL_REGISTER11  localreg11;
	T_DEGAMMA_TBL_REGISTER12  localreg12;
	T_DEGAMMA_TBL_REGISTER13  localreg13;
	T_DEGAMMA_TBL_REGISTER14  localreg14;
	T_DEGAMMA_TBL_REGISTER15  localreg15;
#endif
	T_DEGAMMA_TBL_REGISTER16  localreg16;

#if 0
	localreg0.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER0_OFS);
	localreg1.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER1_OFS);
	localreg2.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER2_OFS);
	localreg3.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER3_OFS);
	localreg4.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER4_OFS);
	localreg5.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER5_OFS);
	localreg6.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER6_OFS);
	localreg7.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER7_OFS);
	localreg8.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER8_OFS);
	localreg9.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER9_OFS);
	localreg10.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER10_OFS);
	localreg11.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER11_OFS);
	localreg12.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER12_OFS);
	localreg13.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER13_OFS);
	localreg14.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER14_OFS);
	localreg15.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER15_OFS);
#endif



#if 0
	localreg0.bit.ife_r_degamma_tbl0  = p_rde->p_degamma_table[0];
	localreg0.bit.ife_r_degamma_tbl1  = p_rde->p_degamma_table[1];
	localreg1.bit.ife_r_degamma_tbl2  = p_rde->p_degamma_table[2];
	localreg1.bit.ife_r_degamma_tbl3  = p_rde->p_degamma_table[3];
	localreg2.bit.ife_r_degamma_tbl4  = p_rde->p_degamma_table[4];
	localreg2.bit.ife_r_degamma_tbl5  = p_rde->p_degamma_table[5];
	localreg3.bit.ife_r_degamma_tbl6  = p_rde->p_degamma_table[6];
	localreg3.bit.ife_r_degamma_tbl7  = p_rde->p_degamma_table[7];
	localreg4.bit.ife_r_degamma_tbl8  = p_rde->p_degamma_table[8];
	localreg4.bit.ife_r_degamma_tbl9  = p_rde->p_degamma_table[9];
	localreg5.bit.ife_r_degamma_tbl10  = p_rde->p_degamma_table[10];
	localreg5.bit.ife_r_degamma_tbl11  = p_rde->p_degamma_table[11];
	localreg6.bit.ife_r_degamma_tbl12  = p_rde->p_degamma_table[12];
	localreg6.bit.ife_r_degamma_tbl13  = p_rde->p_degamma_table[13];
	localreg7.bit.ife_r_degamma_tbl14  = p_rde->p_degamma_table[14];
	localreg7.bit.ife_r_degamma_tbl15  = p_rde->p_degamma_table[15];
	localreg8.bit.ife_r_degamma_tbl16  = p_rde->p_degamma_table[16];
	localreg8.bit.ife_r_degamma_tbl17  = p_rde->p_degamma_table[17];
	localreg9.bit.ife_r_degamma_tbl18  = p_rde->p_degamma_table[18];
	localreg9.bit.ife_r_degamma_tbl19  = p_rde->p_degamma_table[19];
	localreg10.bit.ife_r_degamma_tbl20  = p_rde->p_degamma_table[20];
	localreg10.bit.ife_r_degamma_tbl21  = p_rde->p_degamma_table[21];
	localreg11.bit.ife_r_degamma_tbl22  = p_rde->p_degamma_table[22];
	localreg11.bit.ife_r_degamma_tbl23  = p_rde->p_degamma_table[23];
	localreg12.bit.ife_r_degamma_tbl24  = p_rde->p_degamma_table[24];
	localreg12.bit.ife_r_degamma_tbl25  = p_rde->p_degamma_table[25];
	localreg13.bit.ife_r_degamma_tbl26  = p_rde->p_degamma_table[26];
	localreg13.bit.ife_r_degamma_tbl27  = p_rde->p_degamma_table[27];
	localreg14.bit.ife_r_degamma_tbl28  = p_rde->p_degamma_table[28];
	localreg14.bit.ife_r_degamma_tbl29  = p_rde->p_degamma_table[29];
	localreg15.bit.ife_r_degamma_tbl30  = p_rde->p_degamma_table[30];
	localreg15.bit.ife_r_degamma_tbl31  = p_rde->p_degamma_table[31];
#endif



#if 0
	IFE_SETREG(DEGAMMA_TBL_REGISTER0_OFS, localreg0.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER1_OFS, localreg1.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER2_OFS, localreg2.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER3_OFS, localreg3.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER4_OFS, localreg4.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER5_OFS, localreg5.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER6_OFS, localreg6.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER7_OFS, localreg7.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER8_OFS, localreg8.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER9_OFS, localreg9.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER10_OFS, localreg10.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER11_OFS, localreg11.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER12_OFS, localreg12.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER13_OFS, localreg13.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER14_OFS, localreg14.reg);
	IFE_SETREG(DEGAMMA_TBL_REGISTER15_OFS, localreg15.reg);
#endif



	for (i = 0; i < 16; i++) {
		reg_set_val = 0;

		reg_ofs = DEGAMMA_TBL_REGISTER0_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = ((UINT32)(p_rde->p_degamma_table[idx] & 0x0FFF)) + ((UINT32)(p_rde->p_degamma_table[idx + 1] & 0x0FFF) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

	localreg16.reg = IFE_GETREG(DEGAMMA_TBL_REGISTER16_OFS);
	localreg16.bit.ife_r_degamma_tbl32  = p_rde->p_degamma_table[32];
	IFE_SETREG(DEGAMMA_TBL_REGISTER16_OFS, localreg16.reg);

	return E_OK;
}
/*
  Set IFE range filter for filter NLM

  Set IFE range filter for filter NLM

  @param[in] *p_range_set range filter setting: thresholds and delta

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set2dnr_nlm_parameter(IFE_RANGESETA *p_range_set)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;
	UINT32 ip;

#if 0
	T_RANGE_FILTER_REGISTER_1   localreg1;
	T_RANGE_FILTER_REGISTER_2   localreg2;
	T_RANGE_FILTER_REGISTER_3   localreg3;

	T_RANGE_FILTER_REGISTER_5   localreg5;
	T_RANGE_FILTER_REGISTER_6   localreg6;
	T_RANGE_FILTER_REGISTER_7   localreg7;

	T_RANGE_FILTER_REGISTER_9    localreg9;
	T_RANGE_FILTER_REGISTER_10   localreg10;
	T_RANGE_FILTER_REGISTER_11   localreg11;

	T_RANGE_FILTER_REGISTER_13   localreg13;
	T_RANGE_FILTER_REGISTER_14   localreg14;
	T_RANGE_FILTER_REGISTER_15   localreg15;


	T_RANGE_FILTER_REGISTER_32  localreg32;
	T_RANGE_FILTER_REGISTER_33  localreg33;
	T_RANGE_FILTER_REGISTER_34  localreg34;
	T_RANGE_FILTER_REGISTER_35  localreg35;
	T_RANGE_FILTER_REGISTER_36  localreg36;
	T_RANGE_FILTER_REGISTER_37  localreg37;
	T_RANGE_FILTER_REGISTER_38  localreg38;
	T_RANGE_FILTER_REGISTER_39  localreg39;

	T_RANGE_FILTER_REGISTER_41  localreg41;
	T_RANGE_FILTER_REGISTER_42  localreg42;
	T_RANGE_FILTER_REGISTER_43  localreg43;
	T_RANGE_FILTER_REGISTER_44  localreg44;
	T_RANGE_FILTER_REGISTER_45  localreg45;
	T_RANGE_FILTER_REGISTER_46  localreg46;
	T_RANGE_FILTER_REGISTER_47  localreg47;
	T_RANGE_FILTER_REGISTER_48  localreg48;

	T_RANGE_FILTER_REGISTER_50  localreg50;
	T_RANGE_FILTER_REGISTER_51  localreg51;
	T_RANGE_FILTER_REGISTER_52  localreg52;
	T_RANGE_FILTER_REGISTER_53  localreg53;
	T_RANGE_FILTER_REGISTER_54  localreg54;
	T_RANGE_FILTER_REGISTER_55  localreg55;
	T_RANGE_FILTER_REGISTER_56  localreg56;
	T_RANGE_FILTER_REGISTER_57  localreg57;

	T_RANGE_FILTER_REGISTER_59  localreg59;
	T_RANGE_FILTER_REGISTER_60  localreg60;
	T_RANGE_FILTER_REGISTER_61  localreg61;
	T_RANGE_FILTER_REGISTER_62  localreg62;
	T_RANGE_FILTER_REGISTER_63  localreg63;
	T_RANGE_FILTER_REGISTER_64  localreg64;
	T_RANGE_FILTER_REGISTER_65  localreg65;
	T_RANGE_FILTER_REGISTER_66  localreg66;
#endif

	T_RANGE_FILTER_REGISTER_40  localreg40;
	T_RANGE_FILTER_REGISTER_49  localreg49;
	T_RANGE_FILTER_REGISTER_58  localreg58;
	T_RANGE_FILTER_REGISTER_67  localreg67;


	for (ip = 0; ip < 6; ip++) {
		if (p_range_set->p_rngth_c0[ip] > 1023) {
			p_range_set->p_rngth_c0[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rngth_c1[ip] > 1023) {
			p_range_set->p_rngth_c1[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rngth_c2[ip] > 1023) {
			p_range_set->p_rngth_c2[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rngth_c3[ip] > 1023) {
			p_range_set->p_rngth_c3[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
	}
	for (ip = 0; ip <= 16; ip++) {
		if (p_range_set->p_rnglut_c0[ip] > 1023) {
			p_range_set->p_rnglut_c0[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rnglut_c1[ip] > 1023) {
			p_range_set->p_rnglut_c1[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rnglut_c2[ip] > 1023) {
			p_range_set->p_rnglut_c2[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rnglut_c3[ip] > 1023) {
			p_range_set->p_rnglut_c3[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
	}

#if 0
	localreg1.reg = IFE_GETREG(RANGE_FILTER_REGISTER_1_OFS);
	localreg2.reg = IFE_GETREG(RANGE_FILTER_REGISTER_2_OFS);
	localreg3.reg = IFE_GETREG(RANGE_FILTER_REGISTER_3_OFS);
	localreg1.bit.ife_rth_nlm_c0_0 = p_range_set->p_rngth_c0[0];
	localreg1.bit.ife_rth_nlm_c0_1 = p_range_set->p_rngth_c0[1];

	localreg2.bit.ife_rth_nlm_c0_2 = p_range_set->p_rngth_c0[2];
	localreg2.bit.ife_rth_nlm_c0_3 = p_range_set->p_rngth_c0[3];

	localreg3.bit.ife_rth_nlm_c0_4 = p_range_set->p_rngth_c0[4];
	localreg3.bit.ife_rth_nlm_c0_5 = p_range_set->p_rngth_c0[5];
#endif

	for (i = 0; i < 3; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_1_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rngth_c0[idx]) + ((p_range_set->p_rngth_c0[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

#if 0
	localreg5.reg = IFE_GETREG(RANGE_FILTER_REGISTER_5_OFS);
	localreg6.reg = IFE_GETREG(RANGE_FILTER_REGISTER_6_OFS);
	localreg7.reg = IFE_GETREG(RANGE_FILTER_REGISTER_7_OFS);
	localreg5.bit.ife_rth_nlm_c1_0 = p_range_set->p_rngth_c1[0];
	localreg5.bit.ife_rth_nlm_c1_1 = p_range_set->p_rngth_c1[1];
	localreg6.bit.ife_rth_nlm_c1_2 = p_range_set->p_rngth_c1[2];
	localreg6.bit.ife_rth_nlm_c1_3 = p_range_set->p_rngth_c1[3];
	localreg7.bit.ife_rth_nlm_c1_4 = p_range_set->p_rngth_c1[4];
	localreg7.bit.ife_rth_nlm_c1_5 = p_range_set->p_rngth_c1[5];
#endif

	for (i = 0; i < 3; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_5_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rngth_c1[idx]) + ((p_range_set->p_rngth_c1[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

#if 0
	localreg9.reg = IFE_GETREG(RANGE_FILTER_REGISTER_9_OFS);
	localreg10.reg = IFE_GETREG(RANGE_FILTER_REGISTER_10_OFS);
	localreg11.reg = IFE_GETREG(RANGE_FILTER_REGISTER_11_OFS);
	localreg9.bit.ife_rth_nlm_c2_0 = p_range_set->p_rngth_c2[0];
	localreg9.bit.ife_rth_nlm_c2_1 = p_range_set->p_rngth_c2[1];
	localreg10.bit.ife_rth_nlm_c2_2 = p_range_set->p_rngth_c2[2];
	localreg10.bit.ife_rth_nlm_c2_3 = p_range_set->p_rngth_c2[3];
	localreg11.bit.ife_rth_nlm_c2_4 = p_range_set->p_rngth_c2[4];
	localreg11.bit.ife_rth_nlm_c2_5 = p_range_set->p_rngth_c2[5];
#endif

	for (i = 0; i < 3; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_9_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rngth_c2[idx]) + ((p_range_set->p_rngth_c2[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

#if 0
	localreg13.reg = IFE_GETREG(RANGE_FILTER_REGISTER_13_OFS);
	localreg14.reg = IFE_GETREG(RANGE_FILTER_REGISTER_14_OFS);
	localreg15.reg = IFE_GETREG(RANGE_FILTER_REGISTER_15_OFS);
	localreg13.bit.ife_rth_nlm_c3_0 = p_range_set->p_rngth_c3[0];
	localreg13.bit.ife_rth_nlm_c3_1 = p_range_set->p_rngth_c3[1];
	localreg14.bit.ife_rth_nlm_c3_2 = p_range_set->p_rngth_c3[2];
	localreg14.bit.ife_rth_nlm_c3_3 = p_range_set->p_rngth_c3[3];
	localreg15.bit.ife_rth_nlm_c3_4 = p_range_set->p_rngth_c3[4];
	localreg15.bit.ife_rth_nlm_c3_5 = p_range_set->p_rngth_c3[5];
#endif

	for (i = 0; i < 3; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_13_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rngth_c3[idx]) + ((p_range_set->p_rngth_c3[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}


#if 0
	localreg32.reg = IFE_GETREG(RANGE_FILTER_REGISTER_32_OFS);
	localreg33.reg = IFE_GETREG(RANGE_FILTER_REGISTER_33_OFS);
	localreg34.reg = IFE_GETREG(RANGE_FILTER_REGISTER_34_OFS);
	localreg35.reg = IFE_GETREG(RANGE_FILTER_REGISTER_35_OFS);
	localreg36.reg = IFE_GETREG(RANGE_FILTER_REGISTER_36_OFS);
	localreg37.reg = IFE_GETREG(RANGE_FILTER_REGISTER_37_OFS);
	localreg38.reg = IFE_GETREG(RANGE_FILTER_REGISTER_38_OFS);
	localreg39.reg = IFE_GETREG(RANGE_FILTER_REGISTER_39_OFS);
	localreg32.bit.ife_rth_nlm_c0_lut_0 = p_range_set->p_rnglut_c0[0];
	localreg32.bit.ife_rth_nlm_c0_lut_1 = p_range_set->p_rnglut_c0[1];
	localreg33.bit.ife_rth_nlm_c0_lut_2 = p_range_set->p_rnglut_c0[2];
	localreg33.bit.ife_rth_nlm_c0_lut_3 = p_range_set->p_rnglut_c0[3];
	localreg34.bit.ife_rth_nlm_c0_lut_4 = p_range_set->p_rnglut_c0[4];
	localreg34.bit.ife_rth_nlm_c0_lut_5 = p_range_set->p_rnglut_c0[5];
	localreg35.bit.ife_rth_nlm_c0_lut_6 = p_range_set->p_rnglut_c0[6];
	localreg35.bit.ife_rth_nlm_c0_lut_7 = p_range_set->p_rnglut_c0[7];
	localreg36.bit.ife_rth_nlm_c0_lut_8 = p_range_set->p_rnglut_c0[8];
	localreg36.bit.ife_rth_nlm_c0_lut_9 = p_range_set->p_rnglut_c0[9];
	localreg37.bit.ife_rth_nlm_c0_lut_10 = p_range_set->p_rnglut_c0[10];
	localreg37.bit.ife_rth_nlm_c0_lut_11 = p_range_set->p_rnglut_c0[11];
	localreg38.bit.ife_rth_nlm_c0_lut_12 = p_range_set->p_rnglut_c0[12];
	localreg38.bit.ife_rth_nlm_c0_lut_13 = p_range_set->p_rnglut_c0[13];
	localreg39.bit.ife_rth_nlm_c0_lut_14 = p_range_set->p_rnglut_c0[14];
	localreg39.bit.ife_rth_nlm_c0_lut_15 = p_range_set->p_rnglut_c0[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_32_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rnglut_c0[idx]) + ((p_range_set->p_rnglut_c0[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	localreg40.reg = IFE_GETREG(RANGE_FILTER_REGISTER_40_OFS);
	localreg40.bit.ife_rth_nlm_c0_lut_16 = p_range_set->p_rnglut_c0[16];
	IFE_SETREG(RANGE_FILTER_REGISTER_40_OFS, localreg40.reg);


#if 0
	localreg41.reg = IFE_GETREG(RANGE_FILTER_REGISTER_41_OFS);
	localreg42.reg = IFE_GETREG(RANGE_FILTER_REGISTER_42_OFS);
	localreg43.reg = IFE_GETREG(RANGE_FILTER_REGISTER_43_OFS);
	localreg44.reg = IFE_GETREG(RANGE_FILTER_REGISTER_44_OFS);
	localreg45.reg = IFE_GETREG(RANGE_FILTER_REGISTER_45_OFS);
	localreg46.reg = IFE_GETREG(RANGE_FILTER_REGISTER_46_OFS);
	localreg47.reg = IFE_GETREG(RANGE_FILTER_REGISTER_47_OFS);
	localreg48.reg = IFE_GETREG(RANGE_FILTER_REGISTER_48_OFS);
	localreg41.bit.ife_rth_nlm_c1_lut_0 = p_range_set->p_rnglut_c1[0];
	localreg41.bit.ife_rth_nlm_c1_lut_1 = p_range_set->p_rnglut_c1[1];
	localreg42.bit.ife_rth_nlm_c1_lut_2 = p_range_set->p_rnglut_c1[2];
	localreg42.bit.ife_rth_nlm_c1_lut_3 = p_range_set->p_rnglut_c1[3];
	localreg43.bit.ife_rth_nlm_c1_lut_4 = p_range_set->p_rnglut_c1[4];
	localreg43.bit.ife_rth_nlm_c1_lut_5 = p_range_set->p_rnglut_c1[5];
	localreg44.bit.ife_rth_nlm_c1_lut_6 = p_range_set->p_rnglut_c1[6];
	localreg44.bit.ife_rth_nlm_c1_lut_7 = p_range_set->p_rnglut_c1[7];
	localreg45.bit.ife_rth_nlm_c1_lut_8 = p_range_set->p_rnglut_c1[8];
	localreg45.bit.ife_rth_nlm_c1_lut_9 = p_range_set->p_rnglut_c1[9];
	localreg46.bit.ife_rth_nlm_c1_lut_10 = p_range_set->p_rnglut_c1[10];
	localreg46.bit.ife_rth_nlm_c1_lut_11 = p_range_set->p_rnglut_c1[11];
	localreg47.bit.ife_rth_nlm_c1_lut_12 = p_range_set->p_rnglut_c1[12];
	localreg47.bit.ife_rth_nlm_c1_lut_13 = p_range_set->p_rnglut_c1[13];
	localreg48.bit.ife_rth_nlm_c1_lut_14 = p_range_set->p_rnglut_c1[14];
	localreg48.bit.ife_rth_nlm_c1_lut_15 = p_range_set->p_rnglut_c1[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_41_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rnglut_c1[idx]) + ((p_range_set->p_rnglut_c1[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	localreg49.reg = IFE_GETREG(RANGE_FILTER_REGISTER_49_OFS);
	localreg49.bit.ife_rth_nlm_c1_lut_16 = p_range_set->p_rnglut_c1[16];
	IFE_SETREG(RANGE_FILTER_REGISTER_49_OFS, localreg49.reg);

#if 0
	localreg50.reg = IFE_GETREG(RANGE_FILTER_REGISTER_50_OFS);
	localreg51.reg = IFE_GETREG(RANGE_FILTER_REGISTER_51_OFS);
	localreg52.reg = IFE_GETREG(RANGE_FILTER_REGISTER_52_OFS);
	localreg53.reg = IFE_GETREG(RANGE_FILTER_REGISTER_53_OFS);
	localreg54.reg = IFE_GETREG(RANGE_FILTER_REGISTER_54_OFS);
	localreg55.reg = IFE_GETREG(RANGE_FILTER_REGISTER_55_OFS);
	localreg56.reg = IFE_GETREG(RANGE_FILTER_REGISTER_56_OFS);
	localreg57.reg = IFE_GETREG(RANGE_FILTER_REGISTER_57_OFS);
	localreg50.bit.ife_rth_nlm_c2_lut_0 = p_range_set->p_rnglut_c2[0];
	localreg50.bit.ife_rth_nlm_c2_lut_1 = p_range_set->p_rnglut_c2[1];
	localreg51.bit.ife_rth_nlm_c2_lut_2 = p_range_set->p_rnglut_c2[2];
	localreg51.bit.ife_rth_nlm_c2_lut_3 = p_range_set->p_rnglut_c2[3];
	localreg52.bit.ife_rth_nlm_c2_lut_4 = p_range_set->p_rnglut_c2[4];
	localreg52.bit.ife_rth_nlm_c2_lut_5 = p_range_set->p_rnglut_c2[5];
	localreg53.bit.ife_rth_nlm_c2_lut_6 = p_range_set->p_rnglut_c2[6];
	localreg53.bit.ife_rth_nlm_c2_lut_7 = p_range_set->p_rnglut_c2[7];
	localreg54.bit.ife_rth_nlm_c2_lut_8 = p_range_set->p_rnglut_c2[8];
	localreg54.bit.ife_rth_nlm_c2_lut_9 = p_range_set->p_rnglut_c2[9];
	localreg55.bit.ife_rth_nlm_c2_lut_10 = p_range_set->p_rnglut_c2[10];
	localreg55.bit.ife_rth_nlm_c2_lut_11 = p_range_set->p_rnglut_c2[11];
	localreg56.bit.ife_rth_nlm_c2_lut_12 = p_range_set->p_rnglut_c2[12];
	localreg56.bit.ife_rth_nlm_c2_lut_13 = p_range_set->p_rnglut_c2[13];
	localreg57.bit.ife_rth_nlm_c2_lut_14 = p_range_set->p_rnglut_c2[14];
	localreg57.bit.ife_rth_nlm_c2_lut_15 = p_range_set->p_rnglut_c2[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_50_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rnglut_c2[idx]) + ((p_range_set->p_rnglut_c2[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	localreg58.reg = IFE_GETREG(RANGE_FILTER_REGISTER_58_OFS);
	localreg58.bit.ife_rth_nlm_c2_lut_16 = p_range_set->p_rnglut_c2[16];
	IFE_SETREG(RANGE_FILTER_REGISTER_58_OFS, localreg58.reg);

#if 0
	localreg59.reg = IFE_GETREG(RANGE_FILTER_REGISTER_59_OFS);
	localreg60.reg = IFE_GETREG(RANGE_FILTER_REGISTER_60_OFS);
	localreg61.reg = IFE_GETREG(RANGE_FILTER_REGISTER_61_OFS);
	localreg62.reg = IFE_GETREG(RANGE_FILTER_REGISTER_62_OFS);
	localreg63.reg = IFE_GETREG(RANGE_FILTER_REGISTER_63_OFS);
	localreg64.reg = IFE_GETREG(RANGE_FILTER_REGISTER_64_OFS);
	localreg65.reg = IFE_GETREG(RANGE_FILTER_REGISTER_65_OFS);
	localreg66.reg = IFE_GETREG(RANGE_FILTER_REGISTER_66_OFS);
	localreg59.bit.ife_rth_nlm_c3_lut_0 = p_range_set->p_rnglut_c3[0];
	localreg59.bit.ife_rth_nlm_c3_lut_1 = p_range_set->p_rnglut_c3[1];
	localreg60.bit.ife_rth_nlm_c3_lut_2 = p_range_set->p_rnglut_c3[2];
	localreg60.bit.ife_rth_nlm_c3_lut_3 = p_range_set->p_rnglut_c3[3];
	localreg61.bit.ife_rth_nlm_c3_lut_4 = p_range_set->p_rnglut_c3[4];
	localreg61.bit.ife_rth_nlm_c3_lut_5 = p_range_set->p_rnglut_c3[5];
	localreg62.bit.ife_rth_nlm_c3_lut_6 = p_range_set->p_rnglut_c3[6];
	localreg62.bit.ife_rth_nlm_c3_lut_7 = p_range_set->p_rnglut_c3[7];
	localreg63.bit.ife_rth_nlm_c3_lut_8 = p_range_set->p_rnglut_c3[8];
	localreg63.bit.ife_rth_nlm_c3_lut_9 = p_range_set->p_rnglut_c3[9];
	localreg64.bit.ife_rth_nlm_c3_lut_10 = p_range_set->p_rnglut_c3[10];
	localreg64.bit.ife_rth_nlm_c3_lut_11 = p_range_set->p_rnglut_c3[11];
	localreg65.bit.ife_rth_nlm_c3_lut_12 = p_range_set->p_rnglut_c3[12];
	localreg65.bit.ife_rth_nlm_c3_lut_13 = p_range_set->p_rnglut_c3[13];
	localreg66.bit.ife_rth_nlm_c3_lut_14 = p_range_set->p_rnglut_c3[14];
	localreg66.bit.ife_rth_nlm_c3_lut_15 = p_range_set->p_rnglut_c3[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_59_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rnglut_c3[idx]) + ((p_range_set->p_rnglut_c3[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	localreg67.reg = IFE_GETREG(RANGE_FILTER_REGISTER_67_OFS);
	localreg67.bit.ife_rth_nlm_c3_lut_16 = p_range_set->p_rnglut_c3[16];
	IFE_SETREG(RANGE_FILTER_REGISTER_67_OFS, localreg67.reg);

#if 0
	IFE_SETREG(RANGE_FILTER_REGISTER_1_OFS, localreg1.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_2_OFS, localreg2.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_3_OFS, localreg3.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_5_OFS, localreg5.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_6_OFS, localreg6.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_7_OFS, localreg7.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_9_OFS, localreg9.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_10_OFS, localreg10.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_11_OFS, localreg11.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_13_OFS, localreg13.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_14_OFS, localreg14.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_15_OFS, localreg15.reg);

	IFE_SETREG(RANGE_FILTER_REGISTER_32_OFS, localreg32.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_33_OFS, localreg33.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_34_OFS, localreg34.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_35_OFS, localreg35.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_36_OFS, localreg36.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_37_OFS, localreg37.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_38_OFS, localreg38.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_39_OFS, localreg39.reg);

	IFE_SETREG(RANGE_FILTER_REGISTER_41_OFS, localreg41.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_42_OFS, localreg42.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_43_OFS, localreg43.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_44_OFS, localreg44.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_45_OFS, localreg45.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_46_OFS, localreg46.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_47_OFS, localreg47.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_48_OFS, localreg48.reg);

	IFE_SETREG(RANGE_FILTER_REGISTER_50_OFS, localreg50.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_51_OFS, localreg51.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_52_OFS, localreg52.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_53_OFS, localreg53.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_54_OFS, localreg54.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_55_OFS, localreg55.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_56_OFS, localreg56.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_57_OFS, localreg57.reg);

	IFE_SETREG(RANGE_FILTER_REGISTER_59_OFS, localreg59.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_60_OFS, localreg60.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_61_OFS, localreg61.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_62_OFS, localreg62.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_63_OFS, localreg63.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_64_OFS, localreg64.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_65_OFS, localreg65.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_66_OFS, localreg66.reg);
#endif

	return E_OK;
}

static ER ife_set2dnr_range_weight(UINT32 rth_w)
{
	T_RANGE_FILTER_REGISTER_0 localreg;

	localreg.reg = IFE_GETREG(RANGE_FILTER_REGISTER_0_OFS);
	localreg.bit.ife_rth_w = rth_w;

	IFE_SETREG(RANGE_FILTER_REGISTER_0_OFS, localreg.reg);
	return E_OK;
}

static ER ife_set2dnr_blend_weight(UINT32 bilat_w)
{
	T_RANGE_FILTER_REGISTER_0 localreg;

	localreg.reg = IFE_GETREG(RANGE_FILTER_REGISTER_0_OFS);
	localreg.bit.ife_bilat_w = bilat_w;
	IFE_SETREG(RANGE_FILTER_REGISTER_0_OFS, localreg.reg);
	return E_OK;
}
/*
  Set IFE range filter for filter B

  Set IFE range filter for filter B

  @param[in] *p_range_set range filter setting: thresholds and delta

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set2dnr_bilat_parameter(IFE_RANGESETB *p_range_set)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;
	UINT32 ip;

#if 0
	T_RANGE_FILTER_REGISTER_17  localreg17;
	T_RANGE_FILTER_REGISTER_18  localreg18;
	T_RANGE_FILTER_REGISTER_19  localreg19;
	T_RANGE_FILTER_REGISTER_21  localreg21;
	T_RANGE_FILTER_REGISTER_22  localreg22;
	T_RANGE_FILTER_REGISTER_23  localreg23;
	T_RANGE_FILTER_REGISTER_25  localreg25;
	T_RANGE_FILTER_REGISTER_26  localreg26;
	T_RANGE_FILTER_REGISTER_27  localreg27;
	T_RANGE_FILTER_REGISTER_29  localreg29;
	T_RANGE_FILTER_REGISTER_30  localreg30;
	T_RANGE_FILTER_REGISTER_31  localreg31;

	T_RANGE_FILTER_REGISTER_68  localreg68;
	T_RANGE_FILTER_REGISTER_69  localreg69;
	T_RANGE_FILTER_REGISTER_70  localreg70;
	T_RANGE_FILTER_REGISTER_71  localreg71;
	T_RANGE_FILTER_REGISTER_72  localreg72;
	T_RANGE_FILTER_REGISTER_73  localreg73;
	T_RANGE_FILTER_REGISTER_74  localreg74;
	T_RANGE_FILTER_REGISTER_75  localreg75;

	T_RANGE_FILTER_REGISTER_77  localreg77;
	T_RANGE_FILTER_REGISTER_78  localreg78;
	T_RANGE_FILTER_REGISTER_79  localreg79;
	T_RANGE_FILTER_REGISTER_80  localreg80;
	T_RANGE_FILTER_REGISTER_81  localreg81;
	T_RANGE_FILTER_REGISTER_82  localreg82;
	T_RANGE_FILTER_REGISTER_83  localreg83;
	T_RANGE_FILTER_REGISTER_84  localreg84;

	T_RANGE_FILTER_REGISTER_86  localreg86;
	T_RANGE_FILTER_REGISTER_87  localreg87;
	T_RANGE_FILTER_REGISTER_88  localreg88;
	T_RANGE_FILTER_REGISTER_89  localreg89;
	T_RANGE_FILTER_REGISTER_90  localreg90;
	T_RANGE_FILTER_REGISTER_91  localreg91;
	T_RANGE_FILTER_REGISTER_92  localreg92;
	T_RANGE_FILTER_REGISTER_93  localreg93;

	T_RANGE_FILTER_REGISTER_95  localreg95;
	T_RANGE_FILTER_REGISTER_96  localreg96;
	T_RANGE_FILTER_REGISTER_97  localreg97;
	T_RANGE_FILTER_REGISTER_98  localreg98;
	T_RANGE_FILTER_REGISTER_99  localreg99;
	T_RANGE_FILTER_REGISTER_100 localreg100;
	T_RANGE_FILTER_REGISTER_101 localreg101;
	T_RANGE_FILTER_REGISTER_102 localreg102;
#endif

	T_RANGE_FILTER_REGISTER_76  localreg76;
	T_RANGE_FILTER_REGISTER_85  localreg85;
	T_RANGE_FILTER_REGISTER_94  localreg94;
	T_RANGE_FILTER_REGISTER_103 localreg103;



	for (ip = 0; ip < 6; ip++) {
		if (p_range_set->p_rngth_c0[ip] > 1023) {
			p_range_set->p_rngth_c0[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rngth_c0[ip] > 1023) {
			p_range_set->p_rngth_c0[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rngth_c2[ip] > 1023) {
			p_range_set->p_rngth_c2[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rngth_c3[ip] > 1023) {
			p_range_set->p_rngth_c3[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
	}
	for (ip = 0; ip <= 16; ip++) {
		if (p_range_set->p_rnglut_c0[ip] > 1023) {
			p_range_set->p_rnglut_c0[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rnglut_c0[ip] > 1023) {
			p_range_set->p_rnglut_c0[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rnglut_c2[ip] > 1023) {
			p_range_set->p_rnglut_c2[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
		if (p_range_set->p_rnglut_c3[ip] > 1023) {
			p_range_set->p_rnglut_c3[ip] = 1023;
			DBG_WRN("Range threshold should < 1024\r\n");
		}
	}

#if 0
	localreg17.reg = IFE_GETREG(RANGE_FILTER_REGISTER_17_OFS);
	localreg18.reg = IFE_GETREG(RANGE_FILTER_REGISTER_18_OFS);
	localreg19.reg = IFE_GETREG(RANGE_FILTER_REGISTER_19_OFS);
	localreg17.bit.ife_rth_bilat_c0_0 = p_range_set->p_rngth_c0[0];
	localreg17.bit.ife_rth_bilat_c0_1 = p_range_set->p_rngth_c0[1];
	localreg18.bit.ife_rth_bilat_c0_2 = p_range_set->p_rngth_c0[2];
	localreg18.bit.ife_rth_bilat_c0_3 = p_range_set->p_rngth_c0[3];
	localreg19.bit.ife_rth_bilat_c0_4 = p_range_set->p_rngth_c0[4];
	localreg19.bit.ife_rth_bilat_c0_5 = p_range_set->p_rngth_c0[5];
#endif

	for (i = 0; i < 3; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_17_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rngth_c0[idx]) + (p_range_set->p_rngth_c0[idx + 1] << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}


#if 0
	localreg21.reg = IFE_GETREG(RANGE_FILTER_REGISTER_21_OFS);
	localreg22.reg = IFE_GETREG(RANGE_FILTER_REGISTER_22_OFS);
	localreg23.reg = IFE_GETREG(RANGE_FILTER_REGISTER_23_OFS);
	localreg21.bit.ife_rth_bilat_c1_0 = p_range_set->p_rngth_c1[0];
	localreg21.bit.ife_rth_bilat_c1_1 = p_range_set->p_rngth_c1[1];
	localreg22.bit.ife_rth_bilat_c1_2 = p_range_set->p_rngth_c1[2];
	localreg22.bit.ife_rth_bilat_c1_3 = p_range_set->p_rngth_c1[3];
	localreg23.bit.ife_rth_bilat_c1_4 = p_range_set->p_rngth_c1[4];
	localreg23.bit.ife_rth_bilat_c1_5 = p_range_set->p_rngth_c1[5];
#endif

	for (i = 0; i < 3; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_21_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rngth_c1[idx]) + (p_range_set->p_rngth_c1[idx + 1] << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

#if 0
	localreg25.reg = IFE_GETREG(RANGE_FILTER_REGISTER_25_OFS);
	localreg26.reg = IFE_GETREG(RANGE_FILTER_REGISTER_26_OFS);
	localreg27.reg = IFE_GETREG(RANGE_FILTER_REGISTER_27_OFS);
	localreg25.bit.ife_rth_bilat_c2_0 = p_range_set->p_rngth_c2[0];
	localreg25.bit.ife_rth_bilat_c2_1 = p_range_set->p_rngth_c2[1];
	localreg26.bit.ife_rth_bilat_c2_2 = p_range_set->p_rngth_c2[2];
	localreg26.bit.ife_rth_bilat_c2_3 = p_range_set->p_rngth_c2[3];
	localreg27.bit.ife_rth_bilat_c2_4 = p_range_set->p_rngth_c2[4];
	localreg27.bit.ife_rth_bilat_c2_5 = p_range_set->p_rngth_c2[5];
#endif

	for (i = 0; i < 3; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_25_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rngth_c2[idx]) + (p_range_set->p_rngth_c2[idx + 1] << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

#if 0
	localreg29.reg = IFE_GETREG(RANGE_FILTER_REGISTER_29_OFS);
	localreg30.reg = IFE_GETREG(RANGE_FILTER_REGISTER_30_OFS);
	localreg31.reg = IFE_GETREG(RANGE_FILTER_REGISTER_31_OFS);
	localreg29.bit.ife_rth_bilat_c3_0 = p_range_set->p_rngth_c3[0];
	localreg29.bit.ife_rth_bilat_c3_1 = p_range_set->p_rngth_c3[1];
	localreg30.bit.ife_rth_bilat_c3_2 = p_range_set->p_rngth_c3[2];
	localreg30.bit.ife_rth_bilat_c3_3 = p_range_set->p_rngth_c3[3];
	localreg31.bit.ife_rth_bilat_c3_4 = p_range_set->p_rngth_c3[4];
	localreg31.bit.ife_rth_bilat_c3_5 = p_range_set->p_rngth_c3[5];
#endif

	for (i = 0; i < 3; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_29_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rngth_c3[idx]) + (p_range_set->p_rngth_c3[idx + 1] << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

#if 0
	localreg68.reg = IFE_GETREG(RANGE_FILTER_REGISTER_68_OFS);
	localreg69.reg = IFE_GETREG(RANGE_FILTER_REGISTER_69_OFS);
	localreg70.reg = IFE_GETREG(RANGE_FILTER_REGISTER_70_OFS);
	localreg71.reg = IFE_GETREG(RANGE_FILTER_REGISTER_71_OFS);
	localreg72.reg = IFE_GETREG(RANGE_FILTER_REGISTER_72_OFS);
	localreg73.reg = IFE_GETREG(RANGE_FILTER_REGISTER_73_OFS);
	localreg74.reg = IFE_GETREG(RANGE_FILTER_REGISTER_74_OFS);
	localreg75.reg = IFE_GETREG(RANGE_FILTER_REGISTER_75_OFS);
	localreg68.bit.ife_rth_bilat_c0_lut_0 = p_range_set->p_rnglut_c0[0];
	localreg68.bit.ife_rth_bilat_c0_lut_1 = p_range_set->p_rnglut_c0[1];
	localreg69.bit.ife_rth_bilat_c0_lut_2 = p_range_set->p_rnglut_c0[2];
	localreg69.bit.ife_rth_bilat_c0_lut_3 = p_range_set->p_rnglut_c0[3];
	localreg70.bit.ife_rth_bilat_c0_lut_4 = p_range_set->p_rnglut_c0[4];
	localreg70.bit.ife_rth_bilat_c0_lut_5 = p_range_set->p_rnglut_c0[5];
	localreg71.bit.ife_rth_bilat_c0_lut_6 = p_range_set->p_rnglut_c0[6];
	localreg71.bit.ife_rth_bilat_c0_lut_7 = p_range_set->p_rnglut_c0[7];
	localreg72.bit.ife_rth_bilat_c0_lut_8 = p_range_set->p_rnglut_c0[8];
	localreg72.bit.ife_rth_bilat_c0_lut_9 = p_range_set->p_rnglut_c0[9];
	localreg73.bit.ife_rth_bilat_c0_lut_10 = p_range_set->p_rnglut_c0[10];
	localreg73.bit.ife_rth_bilat_c0_lut_11 = p_range_set->p_rnglut_c0[11];
	localreg74.bit.ife_rth_bilat_c0_lut_12 = p_range_set->p_rnglut_c0[12];
	localreg74.bit.ife_rth_bilat_c0_lut_13 = p_range_set->p_rnglut_c0[13];
	localreg75.bit.ife_rth_bilat_c0_lut_14 = p_range_set->p_rnglut_c0[14];
	localreg75.bit.ife_rth_bilat_c0_lut_15 = p_range_set->p_rnglut_c0[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_68_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rnglut_c0[idx]) + (p_range_set->p_rnglut_c0[idx + 1] << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	localreg76.reg = IFE_GETREG(RANGE_FILTER_REGISTER_76_OFS);
	localreg76.bit.ife_rth_bilat_c0_lut_16 = p_range_set->p_rnglut_c0[16];
	IFE_SETREG(RANGE_FILTER_REGISTER_76_OFS, localreg76.reg);


#if 0
	localreg77.reg = IFE_GETREG(RANGE_FILTER_REGISTER_77_OFS);
	localreg78.reg = IFE_GETREG(RANGE_FILTER_REGISTER_78_OFS);
	localreg79.reg = IFE_GETREG(RANGE_FILTER_REGISTER_79_OFS);
	localreg80.reg = IFE_GETREG(RANGE_FILTER_REGISTER_80_OFS);
	localreg81.reg = IFE_GETREG(RANGE_FILTER_REGISTER_81_OFS);
	localreg82.reg = IFE_GETREG(RANGE_FILTER_REGISTER_82_OFS);
	localreg83.reg = IFE_GETREG(RANGE_FILTER_REGISTER_83_OFS);
	localreg84.reg = IFE_GETREG(RANGE_FILTER_REGISTER_84_OFS);
	localreg77.bit.ife_rth_bilat_c1_lut_0 = p_range_set->p_rnglut_c1[0];
	localreg77.bit.ife_rth_bilat_c1_lut_1 = p_range_set->p_rnglut_c1[1];
	localreg78.bit.ife_rth_bilat_c1_lut_2 = p_range_set->p_rnglut_c1[2];
	localreg78.bit.ife_rth_bilat_c1_lut_3 = p_range_set->p_rnglut_c1[3];
	localreg79.bit.ife_rth_bilat_c1_lut_4 = p_range_set->p_rnglut_c1[4];
	localreg79.bit.ife_rth_bilat_c1_lut_5 = p_range_set->p_rnglut_c1[5];
	localreg80.bit.ife_rth_bilat_c1_lut_6 = p_range_set->p_rnglut_c1[6];
	localreg80.bit.ife_rth_bilat_c1_lut_7 = p_range_set->p_rnglut_c1[7];
	localreg81.bit.ife_rth_bilat_c1_lut_8 = p_range_set->p_rnglut_c1[8];
	localreg81.bit.ife_rth_bilat_c1_lut_9 = p_range_set->p_rnglut_c1[9];
	localreg82.bit.ife_rth_bilat_c1_lut_10 = p_range_set->p_rnglut_c1[10];
	localreg82.bit.ife_rth_bilat_c1_lut_11 = p_range_set->p_rnglut_c1[11];
	localreg83.bit.ife_rth_bilat_c1_lut_12 = p_range_set->p_rnglut_c1[12];
	localreg83.bit.ife_rth_bilat_c1_lut_13 = p_range_set->p_rnglut_c1[13];
	localreg84.bit.ife_rth_bilat_c1_lut_14 = p_range_set->p_rnglut_c1[14];
	localreg84.bit.ife_rth_bilat_c1_lut_15 = p_range_set->p_rnglut_c1[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_77_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rnglut_c1[idx]) + (p_range_set->p_rnglut_c1[idx + 1] << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	localreg85.reg = IFE_GETREG(RANGE_FILTER_REGISTER_85_OFS);
	localreg85.bit.ife_rth_bilat_c1_lut_16 = p_range_set->p_rnglut_c1[16];
	IFE_SETREG(RANGE_FILTER_REGISTER_85_OFS, localreg85.reg);


#if 0
	localreg86.reg = IFE_GETREG(RANGE_FILTER_REGISTER_86_OFS);
	localreg87.reg = IFE_GETREG(RANGE_FILTER_REGISTER_87_OFS);
	localreg88.reg = IFE_GETREG(RANGE_FILTER_REGISTER_88_OFS);
	localreg89.reg = IFE_GETREG(RANGE_FILTER_REGISTER_89_OFS);
	localreg90.reg = IFE_GETREG(RANGE_FILTER_REGISTER_90_OFS);
	localreg91.reg = IFE_GETREG(RANGE_FILTER_REGISTER_91_OFS);
	localreg92.reg = IFE_GETREG(RANGE_FILTER_REGISTER_92_OFS);
	localreg93.reg = IFE_GETREG(RANGE_FILTER_REGISTER_93_OFS);
	localreg86.bit.ife_rth_bilat_c2_lut_0 = p_range_set->p_rnglut_c2[0];
	localreg86.bit.ife_rth_bilat_c2_lut_1 = p_range_set->p_rnglut_c2[1];
	localreg87.bit.ife_rth_bilat_c2_lut_2 = p_range_set->p_rnglut_c2[2];
	localreg87.bit.ife_rth_bilat_c2_lut_3 = p_range_set->p_rnglut_c2[3];
	localreg88.bit.ife_rth_bilat_c2_lut_4 = p_range_set->p_rnglut_c2[4];
	localreg88.bit.ife_rth_bilat_c2_lut_5 = p_range_set->p_rnglut_c2[5];
	localreg89.bit.ife_rth_bilat_c2_lut_6 = p_range_set->p_rnglut_c2[6];
	localreg89.bit.ife_rth_bilat_c2_lut_7 = p_range_set->p_rnglut_c2[7];
	localreg90.bit.ife_rth_bilat_c2_lut_8 = p_range_set->p_rnglut_c2[8];
	localreg90.bit.ife_rth_bilat_c2_lut_9 = p_range_set->p_rnglut_c2[9];
	localreg91.bit.ife_rth_bilat_c2_lut_10 = p_range_set->p_rnglut_c2[10];
	localreg91.bit.ife_rth_bilat_c2_lut_11 = p_range_set->p_rnglut_c2[11];
	localreg92.bit.ife_rth_bilat_c2_lut_12 = p_range_set->p_rnglut_c2[12];
	localreg92.bit.ife_rth_bilat_c2_lut_13 = p_range_set->p_rnglut_c2[13];
	localreg93.bit.ife_rth_bilat_c2_lut_14 = p_range_set->p_rnglut_c2[14];
	localreg93.bit.ife_rth_bilat_c2_lut_15 = p_range_set->p_rnglut_c2[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_86_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rnglut_c2[idx]) + (p_range_set->p_rnglut_c2[idx + 1] << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	localreg94.reg = IFE_GETREG(RANGE_FILTER_REGISTER_94_OFS);
	localreg94.bit.ife_rth_bilat_c2_lut_16 = p_range_set->p_rnglut_c2[16];
	IFE_SETREG(RANGE_FILTER_REGISTER_94_OFS, localreg94.reg);

#if 0
	localreg95.reg = IFE_GETREG(RANGE_FILTER_REGISTER_95_OFS);
	localreg96.reg = IFE_GETREG(RANGE_FILTER_REGISTER_96_OFS);
	localreg97.reg = IFE_GETREG(RANGE_FILTER_REGISTER_97_OFS);
	localreg98.reg = IFE_GETREG(RANGE_FILTER_REGISTER_98_OFS);
	localreg99.reg = IFE_GETREG(RANGE_FILTER_REGISTER_99_OFS);
	localreg100.reg = IFE_GETREG(RANGE_FILTER_REGISTER_100_OFS);
	localreg101.reg = IFE_GETREG(RANGE_FILTER_REGISTER_101_OFS);
	localreg102.reg = IFE_GETREG(RANGE_FILTER_REGISTER_102_OFS);

	localreg95.bit.ife_rth_bilat_c3_lut_0 = p_range_set->p_rnglut_c3[0];
	localreg95.bit.ife_rth_bilat_c3_lut_1 = p_range_set->p_rnglut_c3[1];
	localreg96.bit.ife_rth_bilat_c3_lut_2 = p_range_set->p_rnglut_c3[2];
	localreg96.bit.ife_rth_bilat_c3_lut_3 = p_range_set->p_rnglut_c3[3];
	localreg97.bit.ife_rth_bilat_c3_lut_4 = p_range_set->p_rnglut_c3[4];
	localreg97.bit.ife_rth_bilat_c3_lut_5 = p_range_set->p_rnglut_c3[5];
	localreg98.bit.ife_rth_bilat_c3_lut_6 = p_range_set->p_rnglut_c3[6];
	localreg98.bit.ife_rth_bilat_c3_lut_7 = p_range_set->p_rnglut_c3[7];
	localreg99.bit.ife_rth_bilat_c3_lut_8 = p_range_set->p_rnglut_c3[8];
	localreg99.bit.ife_rth_bilat_c3_lut_9 = p_range_set->p_rnglut_c3[9];
	localreg100.bit.ife_rth_bilat_c3_lut_10 = p_range_set->p_rnglut_c3[10];
	localreg100.bit.ife_rth_bilat_c3_lut_11 = p_range_set->p_rnglut_c3[11];
	localreg101.bit.ife_rth_bilat_c3_lut_12 = p_range_set->p_rnglut_c3[12];
	localreg101.bit.ife_rth_bilat_c3_lut_13 = p_range_set->p_rnglut_c3[13];
	localreg102.bit.ife_rth_bilat_c3_lut_14 = p_range_set->p_rnglut_c3[14];
	localreg102.bit.ife_rth_bilat_c3_lut_15 = p_range_set->p_rnglut_c3[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = RANGE_FILTER_REGISTER_95_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_range_set->p_rnglut_c3[idx]) + (p_range_set->p_rnglut_c3[idx + 1] << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	localreg103.reg = IFE_GETREG(RANGE_FILTER_REGISTER_103_OFS);
	localreg103.bit.ife_rth_bilat_c3_lut_16 = p_range_set->p_rnglut_c3[16];
	IFE_SETREG(RANGE_FILTER_REGISTER_103_OFS, localreg103.reg);

#if 0
	IFE_SETREG(RANGE_FILTER_REGISTER_17_OFS, localreg17.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_18_OFS, localreg18.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_19_OFS, localreg19.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_21_OFS, localreg21.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_22_OFS, localreg22.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_23_OFS, localreg23.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_25_OFS, localreg25.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_26_OFS, localreg26.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_27_OFS, localreg27.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_29_OFS, localreg29.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_30_OFS, localreg30.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_31_OFS, localreg31.reg);


	IFE_SETREG(RANGE_FILTER_REGISTER_68_OFS, localreg68.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_69_OFS, localreg69.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_70_OFS, localreg70.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_71_OFS, localreg71.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_72_OFS, localreg72.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_73_OFS, localreg73.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_74_OFS, localreg74.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_75_OFS, localreg75.reg);

	IFE_SETREG(RANGE_FILTER_REGISTER_77_OFS, localreg77.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_78_OFS, localreg78.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_79_OFS, localreg79.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_80_OFS, localreg80.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_81_OFS, localreg81.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_82_OFS, localreg82.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_83_OFS, localreg83.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_84_OFS, localreg84.reg);

	IFE_SETREG(RANGE_FILTER_REGISTER_86_OFS, localreg86.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_87_OFS, localreg87.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_88_OFS, localreg88.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_89_OFS, localreg89.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_90_OFS, localreg90.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_91_OFS, localreg91.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_92_OFS, localreg92.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_93_OFS, localreg93.reg);

	IFE_SETREG(RANGE_FILTER_REGISTER_95_OFS, localreg95.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_96_OFS, localreg96.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_97_OFS, localreg97.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_98_OFS, localreg98.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_99_OFS, localreg99.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_100_OFS, localreg100.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_101_OFS, localreg101.reg);
	IFE_SETREG(RANGE_FILTER_REGISTER_102_OFS, localreg102.reg);
#endif

	return E_OK;
}

static ER ife_set2dnr_center_modify(IFE_CENMODSET *p_center_mod_set)
{
	T_RANGE_FILTER_REGISTER_0 localreg1;

	localreg1.reg = IFE_GETREG(RANGE_FILTER_REGISTER_0_OFS);

	localreg1.bit.ife_bilat_th1 = p_center_mod_set->bilat_th1;
	localreg1.bit.ife_bilat_th2 = p_center_mod_set->bilat_th2;

	IFE_SETREG(RANGE_FILTER_REGISTER_0_OFS, localreg1.reg);

	return E_OK;
}

/*
  Set IFE burst length

  @param[in] burst_len    input/output burst length

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_chg_burst_length(IFE_BURST_LENGTH *p_burst_len)
{
	T_DRAM_SETTINGS    localreg1;

	localreg1.reg = IFE_GETREG(DRAM_SETTINGS_OFS);
	switch (p_burst_len->burst_len_input) {
	case IFE_IN_BURST_64W:
		localreg1.bit.input_burst_mode = 0;
		break;
	case IFE_IN_BURST_32W:
		localreg1.bit.input_burst_mode = 1;
		break;
	default:
		localreg1.bit.input_burst_mode = 0;
		break;
	}
	switch (p_burst_len->burst_len_output) {
	case IFE_OUT_BURST_32W:
		localreg1.bit.output_burst_mode = 0;
		break;
	case IFE_OUT_BURST_16W:
		localreg1.bit.output_burst_mode = 1;
		break;
	default:
		localreg1.bit.output_burst_mode = 0;
		break;
	}
	IFE_SETREG(DRAM_SETTINGS_OFS, localreg1.reg);

	return E_OK;
}

/*
  Set input address and lineoffset

  Set input address and lineoffset

  @param[in] in_addr    input address
  @param[in] ui_line_ofs   input lineoffset

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER ife_set_dma_in_addr(UINT32 in_addr, UINT32 line_ofs)
{
	T_SOURCE_ADDRESS_REGISTER_0     localreg1;
	T_SOURCE_LINE_OFFSET_REGISTER_0   localreg2;

	localreg1.reg = IFE_GETREG(SOURCE_ADDRESS_REGISTER_0_OFS);
	localreg2.reg = IFE_GETREG(SOURCE_LINE_OFFSET_REGISTER_0_OFS);
	if (((in_addr & 0x3) != 0) || ((line_ofs & 0x3) != 0)) {
		DBG_WRN("Input1 Address and lineoffset must be word align!!\r\n");
	}
	localreg1.bit.dram_sai0 = in_addr >> 2;
	localreg2.bit.dram_ofsi0 = line_ofs >> 2;

	IFE_SETREG(SOURCE_ADDRESS_REGISTER_0_OFS, localreg1.reg);
	IFE_SETREG(SOURCE_LINE_OFFSET_REGISTER_0_OFS, localreg2.reg);

	return E_OK;
}

ER ife_set_dma_in_addr_2(UINT32 in_addr, UINT32 line_ofs)
{
	T_SOURCE_ADDRESS_REGISTER_1     localreg1;
	T_SOURCE_LINE_OFFSET_REGISTER_1   localreg2;

	localreg1.reg = IFE_GETREG(SOURCE_ADDRESS_REGISTER_1_OFS);
	localreg2.reg = IFE_GETREG(SOURCE_LINE_OFFSET_REGISTER_1_OFS);
	if (((in_addr & 0x3) != 0) || ((line_ofs & 0x3) != 0)) {
		DBG_WRN("Input2 Address and lineoffset must be word align!!\r\n");
	}
	localreg1.bit.dram_sai1 = in_addr >> 2;
	localreg2.bit.dram_ofsi1 = line_ofs >> 2;

	IFE_SETREG(SOURCE_ADDRESS_REGISTER_1_OFS, localreg1.reg);
	IFE_SETREG(SOURCE_LINE_OFFSET_REGISTER_1_OFS, localreg2.reg);

	return E_OK;
}

ER ife_set_hshift(UINT8 h_shift)
{
	T_SOURCE_LINE_OFFSET_REGISTER_0   localreg;

	localreg.reg = IFE_GETREG(SOURCE_LINE_OFFSET_REGISTER_0_OFS);
	localreg.bit.h_start_shift = h_shift;
	IFE_SETREG(SOURCE_LINE_OFFSET_REGISTER_0_OFS, localreg.reg);

	return E_OK;
}



/*
  Set output address and lineoffset

  Set output address and lineoffset

  @param[in] out_addr    output address
  @param[in] ui_line_ofs    output lineoffset

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER ife_set_dma_out_addr(UINT32 out_addr, UINT32 line_ofs)
{
	T_DESTINATION_ADDRESS_REGISTER localreg1;
	T_DESTINATION_LINE_OFFSET_REGISTER localreg2;

	localreg1.reg = IFE_GETREG(DESTINATION_ADDRESS_REGISTER_OFS);
	localreg2.reg = IFE_GETREG(DESTINATION_LINE_OFFSET_REGISTER_OFS);
	if (out_addr == 0) {
		//out_addr = 0x800000;
		DBG_ERR("Output Address might be used before setting!!\r\n");
		return E_PAR;
	}
	if (((out_addr & 0x3) != 0) || ((line_ofs & 0x3) != 0)) {
		DBG_WRN("Output Address and lineoffset must be word align!!\r\n");
	}
	DBG_IND("out_addr = 0x%08x\r\n", (unsigned int)out_addr);
	localreg1.bit.dram_sao = out_addr >> 2;
	localreg2.bit.dram_ofso = line_ofs >> 2;
	IFE_SETREG(DESTINATION_ADDRESS_REGISTER_OFS, localreg1.reg);
	IFE_SETREG(DESTINATION_LINE_OFFSET_REGISTER_OFS, localreg2.reg);

	return E_OK;
}

/*
  Set input size

  Set input size

  @param[in] hsize  input horizontal size, should be 4x
  @param[in] vsize  input vertical size, should be 2x

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_in_size(UINT32 hsize, UINT32 vsize)
{
	T_SOURCE_SIZE_REGISTER_0    localreg;

	localreg.reg = IFE_GETREG(SOURCE_SIZE_REGISTER_0_OFS);
	if ((hsize & 0x3) != 0) {
		DBG_WRN("Input width should be 4x!!\r\n");
	}
	if ((vsize & 0x1) != 0) {
		DBG_WRN("Input height should be 2x!!\r\n");
	}
	localreg.bit.height = vsize >> 1;
	localreg.bit.width = hsize >> 2;
	IFE_SETREG(SOURCE_SIZE_REGISTER_0_OFS, localreg.reg);

	return E_OK;
}

static ER ife_set_crop(UINT32 crop_hsize, UINT32 crop_vsize, UINT32 crop_hstart, UINT32 crop_vstart)
{
	T_SOURCE_SIZE_REGISTER_1    localreg1;
	T_SOURCE_SIZE_REGISTER_2    localreg2;

	localreg1.reg = IFE_GETREG(SOURCE_SIZE_REGISTER_1_OFS);
	localreg2.reg = IFE_GETREG(SOURCE_SIZE_REGISTER_2_OFS);
	if ((crop_hsize & 0x3) != 0) {
		DBG_WRN("Input crop width should be 4x!!\r\n");
	}
	if ((crop_vsize & 0x1) != 0) {
		DBG_WRN("Input crop height should be 2x!!\r\n");
	}
	localreg1.bit.crop_height = crop_vsize >> 1;
	localreg1.bit.crop_width  = crop_hsize >> 2;

	localreg2.bit.crop_vpos  = crop_vstart;
	localreg2.bit.crop_hpos  = crop_hstart;
	IFE_SETREG(SOURCE_SIZE_REGISTER_1_OFS, localreg1.reg);
	IFE_SETREG(SOURCE_SIZE_REGISTER_2_OFS, localreg2.reg);

	return E_OK;
}

/*
  Set Clamping and Weighting

  Set clamp thresholds and weighting with original

  @param[in] p_clamp_weight  clamp threshold and weight multiplication

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ife_set_clamp(IFE_CLAMPSET *p_clamp_weight)
{
	T_OUTPUT_FILTER_REGISTER localreg;

	localreg.reg = IFE_GETREG(OUTPUT_FILTER_REGISTER_OFS);
	localreg.bit.ife_clamp_th = p_clamp_weight->clamp_th;
	localreg.bit.ife_clamp_mul = p_clamp_weight->clamp_mul;
	localreg.bit.ife_clamp_dlt = p_clamp_weight->clamp_dlt;
	IFE_SETREG(OUTPUT_FILTER_REGISTER_OFS, localreg.reg);

	return E_OK;
}

/**
    Set IFE color gain

    Set IFE color gain.

    @param p_cgain Channel 0/1/2/3 color gain.

    @return None.
*/
static void ife_set_color_gain(IFE_CGAINSET *p_cgain, IFE_CFASEL cfa_pat)
{
	T_COLOR_GAIN_REGISTER_0     localreg0;
	T_COLOR_GAIN_REGISTER_1     localreg1;
	T_COLOR_GAIN_REGISTER_2     localreg2;
	T_COLOR_GAIN_REGISTER_3     localreg3;

	T_COLOR_OFFSET_REGISTER_0   localreg4;
	T_COLOR_OFFSET_REGISTER_1   localreg5;
	T_COLOR_OFFSET_REGISTER_2   localreg6;
	//UINT32 ch0, ch1, ch2, ch3;

	localreg0.reg = IFE_GETREG(COLOR_GAIN_REGISTER_0_OFS);
	localreg1.reg = IFE_GETREG(COLOR_GAIN_REGISTER_1_OFS);
	localreg2.reg = IFE_GETREG(COLOR_GAIN_REGISTER_2_OFS);
	localreg3.reg = IFE_GETREG(COLOR_GAIN_REGISTER_3_OFS);

	localreg4.reg = IFE_GETREG(COLOR_OFFSET_REGISTER_0_OFS);
	localreg5.reg = IFE_GETREG(COLOR_OFFSET_REGISTER_1_OFS);
	localreg6.reg = IFE_GETREG(COLOR_OFFSET_REGISTER_2_OFS);

	localreg0.bit.cgain_inv   = p_cgain->b_cgain_inv;
	localreg0.bit.cgain_hinv  = p_cgain->b_cgain_hinv;
	localreg0.bit.cgain_range = p_cgain->cgain_range;
	localreg0.bit.cgain_mask  = p_cgain->cgain_mask;

	localreg1.bit.ife_cgain_r  = p_cgain->p_cgain[0];
	localreg1.bit.ife_cgain_gr = p_cgain->p_cgain[1];
	localreg2.bit.ife_cgain_gb = p_cgain->p_cgain[2];
	localreg2.bit.ife_cgain_b  = p_cgain->p_cgain[3];
	localreg3.bit.ife_cgain_ir = p_cgain->p_cgain[4];

	localreg4.bit.ife_cofs_r  = p_cgain->p_cofs[0];
	localreg4.bit.ife_cofs_gr = p_cgain->p_cofs[1];
	localreg5.bit.ife_cofs_gb = p_cgain->p_cofs[2];
	localreg5.bit.ife_cofs_b  = p_cgain->p_cofs[3];
	localreg6.bit.ife_cofs_ir = p_cgain->p_cofs[4];

	IFE_SETREG(COLOR_GAIN_REGISTER_0_OFS, localreg0.reg);
	IFE_SETREG(COLOR_GAIN_REGISTER_1_OFS, localreg1.reg);
	IFE_SETREG(COLOR_GAIN_REGISTER_2_OFS, localreg2.reg);
	IFE_SETREG(COLOR_GAIN_REGISTER_3_OFS, localreg3.reg);
	IFE_SETREG(COLOR_OFFSET_REGISTER_0_OFS, localreg4.reg);
	IFE_SETREG(COLOR_OFFSET_REGISTER_1_OFS, localreg5.reg);
	IFE_SETREG(COLOR_OFFSET_REGISTER_2_OFS, localreg6.reg);

}

/**
    Set IFE Outlier

    Set IFE Outlier.

    @param p_out_lier Outlier parameter.

    @return None.
*/
static ER ife_set_outlier(IFE_OUTLSET *p_outlier)
{
	T_OUTLIER_THRESHOLD_REGISTER_0 localreg0;
	T_OUTLIER_THRESHOLD_REGISTER_1 localreg1;
	T_OUTLIER_THRESHOLD_REGISTER_2 localreg2;
	T_OUTLIER_THRESHOLD_REGISTER_3 localreg3;
	T_OUTLIER_THRESHOLD_REGISTER_4 localreg4;
	T_OUTLIER_THRESHOLD_REGISTER_5 localreg5;
	T_OUTLIER_THRESHOLD_REGISTER_6 localreg6;

	T_OUTLIER_ORDER_REGISTER_0 localreg7;
	T_OUTLIER_ORDER_REGISTER_1 localreg8;
	T_OUTLIER_ORDER_REGISTER_2 localreg9;

	localreg0.reg = IFE_GETREG(OUTLIER_THRESHOLD_REGISTER_0_OFS);
	localreg0.bit.ife_outl_bright_ofs  = p_outlier->bright_ofs;
	localreg0.bit.ife_outl_dark_ofs    = p_outlier->dark_ofs;
	IFE_SETREG(OUTLIER_THRESHOLD_REGISTER_0_OFS, localreg0.reg);

	localreg1.reg = IFE_GETREG(OUTLIER_THRESHOLD_REGISTER_1_OFS);
	localreg1.bit.ife_outlth_bri0  = p_outlier->p_bri_th[0];
	localreg1.bit.ife_outlth_dark0 = p_outlier->p_dark_th[0];
	IFE_SETREG(OUTLIER_THRESHOLD_REGISTER_1_OFS, localreg1.reg);

	localreg2.reg = IFE_GETREG(OUTLIER_THRESHOLD_REGISTER_2_OFS);
	localreg2.bit.ife_outlth_bri1  = p_outlier->p_bri_th[1];
	localreg2.bit.ife_outlth_dark1 = p_outlier->p_dark_th[1];
	IFE_SETREG(OUTLIER_THRESHOLD_REGISTER_2_OFS, localreg2.reg);

	localreg3.reg = IFE_GETREG(OUTLIER_THRESHOLD_REGISTER_3_OFS);
	localreg3.bit.ife_outlth_bri2  = p_outlier->p_bri_th[2];
	localreg3.bit.ife_outlth_dark2 = p_outlier->p_dark_th[2];
	IFE_SETREG(OUTLIER_THRESHOLD_REGISTER_3_OFS, localreg3.reg);

	localreg4.reg = IFE_GETREG(OUTLIER_THRESHOLD_REGISTER_4_OFS);
	localreg4.bit.ife_outlth_bri3  = p_outlier->p_bri_th[3];
	localreg4.bit.ife_outlth_dark3 = p_outlier->p_dark_th[3];
	IFE_SETREG(OUTLIER_THRESHOLD_REGISTER_4_OFS, localreg4.reg);

	localreg5.reg = IFE_GETREG(OUTLIER_THRESHOLD_REGISTER_5_OFS);
	localreg5.bit.ife_outlth_bri4  = p_outlier->p_bri_th[4];
	localreg5.bit.ife_outlth_dark4 = p_outlier->p_dark_th[4];
	IFE_SETREG(OUTLIER_THRESHOLD_REGISTER_5_OFS, localreg5.reg);

	localreg6.reg = IFE_GETREG(OUTLIER_THRESHOLD_REGISTER_6_OFS);
	localreg6.bit.ife_outl_avg_mode = p_outlier->b_avg_mode;
	localreg6.bit.ife_outl_weight   = p_outlier->outl_w;
	localreg6.bit.ife_outl_cnt1     = p_outlier->p_outl_cnt[0];
	localreg6.bit.ife_outl_cnt2     = p_outlier->p_outl_cnt[1];
	IFE_SETREG(OUTLIER_THRESHOLD_REGISTER_6_OFS, localreg6.reg);

	localreg7.reg = IFE_GETREG(OUTLIER_ORDER_REGISTER_0_OFS);
	localreg7.bit.ife_ord_range_bright   = p_outlier->ord_range_bri;
	localreg7.bit.ife_ord_range_dark     = p_outlier->ord_range_dark;
	localreg7.bit.ife_ord_protect_th     = p_outlier->ord_protect_th;
	localreg7.bit.ife_ord_blend_weight   = p_outlier->ord_blend_w;
	IFE_SETREG(OUTLIER_ORDER_REGISTER_0_OFS, localreg7.reg);

	localreg8.reg = IFE_GETREG(OUTLIER_ORDER_REGISTER_1_OFS);
	localreg8.bit.ife_ord_bright_weight_lut0     = p_outlier->p_ord_bri_w[0];
	localreg8.bit.ife_ord_bright_weight_lut1     = p_outlier->p_ord_bri_w[1];
	localreg8.bit.ife_ord_bright_weight_lut2     = p_outlier->p_ord_bri_w[2];
	localreg8.bit.ife_ord_bright_weight_lut3     = p_outlier->p_ord_bri_w[3];
	localreg8.bit.ife_ord_bright_weight_lut4     = p_outlier->p_ord_bri_w[4];
	localreg8.bit.ife_ord_bright_weight_lut5     = p_outlier->p_ord_bri_w[5];
	localreg8.bit.ife_ord_bright_weight_lut6     = p_outlier->p_ord_bri_w[6];
	localreg8.bit.ife_ord_bright_weight_lut7     = p_outlier->p_ord_bri_w[7];
	IFE_SETREG(OUTLIER_ORDER_REGISTER_1_OFS, localreg8.reg);

	localreg9.reg = IFE_GETREG(OUTLIER_ORDER_REGISTER_2_OFS);
	localreg9.bit.ife_ord_dark_weight_lut0     = p_outlier->p_ord_dark_w[0];
	localreg9.bit.ife_ord_dark_weight_lut1     = p_outlier->p_ord_dark_w[1];
	localreg9.bit.ife_ord_dark_weight_lut2     = p_outlier->p_ord_dark_w[2];
	localreg9.bit.ife_ord_dark_weight_lut3     = p_outlier->p_ord_dark_w[3];
	localreg9.bit.ife_ord_dark_weight_lut4     = p_outlier->p_ord_dark_w[4];
	localreg9.bit.ife_ord_dark_weight_lut5     = p_outlier->p_ord_dark_w[5];
	localreg9.bit.ife_ord_dark_weight_lut6     = p_outlier->p_ord_dark_w[6];
	localreg9.bit.ife_ord_dark_weight_lut7     = p_outlier->p_ord_dark_w[7];
	IFE_SETREG(OUTLIER_ORDER_REGISTER_2_OFS, localreg9.reg);

	return E_OK;
}

static ER ife_set_nrs_ctrl(IFE_NRSSET *p_nrs)
{
	T_NRS_REGISTER                    localreg0;

	localreg0.reg = IFE_GETREG(NRS_REGISTER_OFS);
	localreg0.bit.ife_f_nrs_ord_en           = p_nrs->b_ord_en;
	localreg0.bit.ife_f_nrs_bilat_en         = p_nrs->b_bilat_en;
	localreg0.bit.ife_f_nrs_gbilat_en        = p_nrs->b_gbilat_en;
	localreg0.bit.ife_f_nrs_bilat_strength   = p_nrs->bilat_str;
	localreg0.bit.ife_f_nrs_gbilat_strength  = p_nrs->gbilat_str;
	localreg0.bit.ife_f_nrs_gbilat_weight    = p_nrs->gbilat_w;
	IFE_SETREG(NRS_REGISTER_OFS, localreg0.reg);

	return E_OK;
}

static ER ife_set_nrs_order(IFE_NRSSET *p_nrs)
{
	T_RHE_NRS_S_REGISTER              localreg1;
	T_RHE_NRS_ORDER_LUT_DARK_WEIGHT   localreg2;
	T_RHE_NRS_ORDER_LUT_BRIGHT_WEIGHT localreg3;


	localreg1.reg = IFE_GETREG(RHE_NRS_S_REGISTER_OFS);
	localreg1.bit.ife_f_nrs_ord_range_bright  = p_nrs->ord_range_bri;
	localreg1.bit.ife_f_nrs_ord_range_dark    = p_nrs->ord_range_dark;
	localreg1.bit.ife_f_nrs_ord_diff_thr      = p_nrs->ord_diff_th;
	IFE_SETREG(RHE_NRS_S_REGISTER_OFS, localreg1.reg);

	localreg2.reg = IFE_GETREG(RHE_NRS_ORDER_LUT_DARK_WEIGHT_OFS);
	localreg2.bit.ife_f_nrs_ord_dark_weight_lut0  = p_nrs->p_ord_dark_w[0];
	localreg2.bit.ife_f_nrs_ord_dark_weight_lut1  = p_nrs->p_ord_dark_w[1];
	localreg2.bit.ife_f_nrs_ord_dark_weight_lut2  = p_nrs->p_ord_dark_w[2];
	localreg2.bit.ife_f_nrs_ord_dark_weight_lut3  = p_nrs->p_ord_dark_w[3];
	localreg2.bit.ife_f_nrs_ord_dark_weight_lut4  = p_nrs->p_ord_dark_w[4];
	localreg2.bit.ife_f_nrs_ord_dark_weight_lut5  = p_nrs->p_ord_dark_w[5];
	localreg2.bit.ife_f_nrs_ord_dark_weight_lut6  = p_nrs->p_ord_dark_w[6];
	localreg2.bit.ife_f_nrs_ord_dark_weight_lut7  = p_nrs->p_ord_dark_w[7];
	IFE_SETREG(RHE_NRS_ORDER_LUT_DARK_WEIGHT_OFS, localreg2.reg);

	localreg3.reg = IFE_GETREG(RHE_NRS_ORDER_LUT_BRIGHT_WEIGHT_OFS);
	localreg3.bit.ife_f_nrs_ord_bright_weight_lut0  = p_nrs->p_ord_bri_w[0];
	localreg3.bit.ife_f_nrs_ord_bright_weight_lut1  = p_nrs->p_ord_bri_w[1];
	localreg3.bit.ife_f_nrs_ord_bright_weight_lut2  = p_nrs->p_ord_bri_w[2];
	localreg3.bit.ife_f_nrs_ord_bright_weight_lut3  = p_nrs->p_ord_bri_w[3];
	localreg3.bit.ife_f_nrs_ord_bright_weight_lut4  = p_nrs->p_ord_bri_w[4];
	localreg3.bit.ife_f_nrs_ord_bright_weight_lut5  = p_nrs->p_ord_bri_w[5];
	localreg3.bit.ife_f_nrs_ord_bright_weight_lut6  = p_nrs->p_ord_bri_w[6];
	localreg3.bit.ife_f_nrs_ord_bright_weight_lut7  = p_nrs->p_ord_bri_w[7];
	IFE_SETREG(RHE_NRS_ORDER_LUT_BRIGHT_WEIGHT_OFS, localreg3.reg);

	return E_OK;
}

static ER ife_set_nrs_bilat(IFE_NRSSET *p_nrs)
{
	T_RHE_NRS_BILATERAL_LUT_OFFSET0   localreg4;
	T_RHE_NRS_BILATERAL_LUT_OFFSET1   localreg5;
	T_RHE_NRS_BILATERAL_LUT_WEIGHT    localreg6;
	T_RHE_NRS_BILATERAL_LUT_RANGE0    localreg7;
	T_RHE_NRS_BILATERAL_LUT_RANGE1    localreg8;
	T_RHE_NRS_BILATERAL_LUT_RANGE2    localreg9;
	T_RHE_NRS_GBILATERAL_LUT_OFFSET0  localreg10;
	T_RHE_NRS_GBILATERAL_LUT_OFFSET1  localreg11;
	T_RHE_NRS_GBILATERAL_LUT_WEIGHT   localreg12;
	T_RHE_NRS_GBILATERAL_LUT_RANGE0   localreg13;
	T_RHE_NRS_GBILATERAL_LUT_RANGE1   localreg14;
	T_RHE_NRS_GBILATERAL_LUT_RANGE2   localreg15;

	localreg4.reg = IFE_GETREG(RHE_NRS_BILATERAL_LUT_OFFSET0_OFS);
	localreg4.bit.ife_f_nrs_bilat_lut_offset0  = p_nrs->p_bilat_offset[0];
	localreg4.bit.ife_f_nrs_bilat_lut_offset1  = p_nrs->p_bilat_offset[1];
	localreg4.bit.ife_f_nrs_bilat_lut_offset2  = p_nrs->p_bilat_offset[2];
	localreg4.bit.ife_f_nrs_bilat_lut_offset3  = p_nrs->p_bilat_offset[3];
	IFE_SETREG(RHE_NRS_BILATERAL_LUT_OFFSET0_OFS, localreg4.reg);

	localreg5.reg = IFE_GETREG(RHE_NRS_BILATERAL_LUT_OFFSET1_OFS);
	localreg5.bit.ife_f_nrs_bilat_lut_offset4  = p_nrs->p_bilat_offset[4];
	localreg5.bit.ife_f_nrs_bilat_lut_offset5  = p_nrs->p_bilat_offset[5];
	IFE_SETREG(RHE_NRS_BILATERAL_LUT_OFFSET1_OFS, localreg5.reg);

	localreg6.reg = IFE_GETREG(RHE_NRS_BILATERAL_LUT_WEIGHT_OFS);
	localreg6.bit.ife_f_nrs_bilat_lut_weight0  = p_nrs->p_bilat_weight[0];
	localreg6.bit.ife_f_nrs_bilat_lut_weight1  = p_nrs->p_bilat_weight[1];
	localreg6.bit.ife_f_nrs_bilat_lut_weight2  = p_nrs->p_bilat_weight[2];
	localreg6.bit.ife_f_nrs_bilat_lut_weight3  = p_nrs->p_bilat_weight[3];
	localreg6.bit.ife_f_nrs_bilat_lut_weight4  = p_nrs->p_bilat_weight[4];
	localreg6.bit.ife_f_nrs_bilat_lut_weight5  = p_nrs->p_bilat_weight[5];
	IFE_SETREG(RHE_NRS_BILATERAL_LUT_WEIGHT_OFS, localreg6.reg);

	localreg7.reg = IFE_GETREG(RHE_NRS_BILATERAL_LUT_RANGE0_OFS);
	localreg7.bit.ife_f_nrs_bilat_lut_range1  = p_nrs->p_bilat_range[0];
	localreg7.bit.ife_f_nrs_bilat_lut_range2  = p_nrs->p_bilat_range[1];
	IFE_SETREG(RHE_NRS_BILATERAL_LUT_RANGE0_OFS, localreg7.reg);

	localreg8.reg = IFE_GETREG(RHE_NRS_BILATERAL_LUT_RANGE1_OFS);
	localreg8.bit.ife_f_nrs_bilat_lut_range3  = p_nrs->p_bilat_range[2];
	localreg8.bit.ife_f_nrs_bilat_lut_range4  = p_nrs->p_bilat_range[3];
	IFE_SETREG(RHE_NRS_BILATERAL_LUT_RANGE1_OFS, localreg8.reg);

	localreg9.reg = IFE_GETREG(RHE_NRS_BILATERAL_LUT_RANGE2_OFS);
	localreg9.bit.ife_f_nrs_bilat_lut_range5  = p_nrs->p_bilat_range[4];

	localreg9.bit.ife_f_nrs_bilat_lut_th1     = p_nrs->p_bilat_th[0];
	localreg9.bit.ife_f_nrs_bilat_lut_th2     = p_nrs->p_bilat_th[1];
	localreg9.bit.ife_f_nrs_bilat_lut_th3     = p_nrs->p_bilat_th[2];
	localreg9.bit.ife_f_nrs_bilat_lut_th4     = p_nrs->p_bilat_th[3];
	localreg9.bit.ife_f_nrs_bilat_lut_th5     = p_nrs->p_bilat_th[4];
	IFE_SETREG(RHE_NRS_BILATERAL_LUT_RANGE2_OFS, localreg9.reg);

	localreg10.reg = IFE_GETREG(RHE_NRS_GBILATERAL_LUT_OFFSET0_OFS);
	localreg10.bit.ife_f_nrs_gbilat_lut_offset0  = p_nrs->p_gbilat_offset[0];
	localreg10.bit.ife_f_nrs_gbilat_lut_offset1  = p_nrs->p_gbilat_offset[1];
	localreg10.bit.ife_f_nrs_gbilat_lut_offset2  = p_nrs->p_gbilat_offset[2];
	localreg10.bit.ife_f_nrs_gbilat_lut_offset3  = p_nrs->p_gbilat_offset[3];
	IFE_SETREG(RHE_NRS_GBILATERAL_LUT_OFFSET0_OFS, localreg10.reg);

	localreg11.reg = IFE_GETREG(RHE_NRS_GBILATERAL_LUT_OFFSET1_OFS);
	localreg11.bit.ife_f_nrs_gbilat_lut_offset4  = p_nrs->p_gbilat_offset[4];
	localreg11.bit.ife_f_nrs_gbilat_lut_offset5  = p_nrs->p_gbilat_offset[5];
	IFE_SETREG(RHE_NRS_GBILATERAL_LUT_OFFSET1_OFS, localreg11.reg);

	localreg12.reg = IFE_GETREG(RHE_NRS_GBILATERAL_LUT_WEIGHT_OFS);
	localreg12.bit.ife_f_nrs_gbilat_lut_weight0  = p_nrs->p_gbilat_weight[0];
	localreg12.bit.ife_f_nrs_gbilat_lut_weight1  = p_nrs->p_gbilat_weight[1];
	localreg12.bit.ife_f_nrs_gbilat_lut_weight2  = p_nrs->p_gbilat_weight[2];
	localreg12.bit.ife_f_nrs_gbilat_lut_weight3  = p_nrs->p_gbilat_weight[3];
	localreg12.bit.ife_f_nrs_gbilat_lut_weight4  = p_nrs->p_gbilat_weight[4];
	localreg12.bit.ife_f_nrs_gbilat_lut_weight5  = p_nrs->p_gbilat_weight[5];
	IFE_SETREG(RHE_NRS_GBILATERAL_LUT_WEIGHT_OFS, localreg12.reg);

	localreg13.reg = IFE_GETREG(RHE_NRS_GBILATERAL_LUT_RANGE0_OFS);
	localreg13.bit.ife_f_nrs_gbilat_lut_range1  = p_nrs->p_gbilat_range[0];
	localreg13.bit.ife_f_nrs_gbilat_lut_range2  = p_nrs->p_gbilat_range[1];
	IFE_SETREG(RHE_NRS_GBILATERAL_LUT_RANGE0_OFS, localreg13.reg);

	localreg14.reg = IFE_GETREG(RHE_NRS_GBILATERAL_LUT_RANGE1_OFS);
	localreg14.bit.ife_f_nrs_gbilat_lut_range3  = p_nrs->p_gbilat_range[2];
	localreg14.bit.ife_f_nrs_gbilat_lut_range4  = p_nrs->p_gbilat_range[3];
	IFE_SETREG(RHE_NRS_GBILATERAL_LUT_RANGE1_OFS, localreg14.reg);

	localreg15.reg = IFE_GETREG(RHE_NRS_GBILATERAL_LUT_RANGE2_OFS);
	localreg15.bit.ife_f_nrs_gbilat_lut_range5  = p_nrs->p_gbilat_range[4];

	localreg15.bit.ife_f_nrs_gbilat_lut_th1     = p_nrs->p_gbilat_th[0];
	localreg15.bit.ife_f_nrs_gbilat_lut_th2     = p_nrs->p_gbilat_th[1];
	localreg15.bit.ife_f_nrs_gbilat_lut_th3     = p_nrs->p_gbilat_th[2];
	localreg15.bit.ife_f_nrs_gbilat_lut_th4     = p_nrs->p_gbilat_th[3];
	localreg15.bit.ife_f_nrs_gbilat_lut_th5     = p_nrs->p_gbilat_th[4];
	IFE_SETREG(RHE_NRS_GBILATERAL_LUT_RANGE2_OFS, localreg15.reg);

	return E_OK;
}

static ER ife_set_fcurve_ctrl(IFE_FCURVESET *p_fcurve)
{
	T_RHE_FCURVE_CTRL                 localreg0;

	localreg0.reg = IFE_GETREG(RHE_FCURVE_CTRL_OFS);
	localreg0.bit.ife_f_fcurve_ymean_select  = p_fcurve->y_mean_select;
	localreg0.bit.ife_f_fcurve_yvweight      = p_fcurve->yv_weight;
	IFE_SETREG(RHE_FCURVE_CTRL_OFS, localreg0.reg);

	return E_OK;
}

static ER ife_set_fcurve_yweight(IFE_FCURVESET *p_fcurve)
{
	T_RHE_FCURVE_Y_WEIGHT_REGISTER0          localreg0;
	T_RHE_FCURVE_Y_WEIGHT_REGISTER1          localreg1;
	T_RHE_FCURVE_Y_WEIGHT_REGISTER2          localreg2;
	T_RHE_FCURVE_Y_WEIGHT_REGISTER3          localreg3;
	T_RHE_FCURVE_Y_WEIGHT_REGISTER4          localreg4;

	localreg0.reg = IFE_GETREG(RHE_FCURVE_Y_WEIGHT_REGISTER0_OFS);
	localreg0.bit.ife_f_fcurve_yweight_lut0     = p_fcurve->p_y_weight_lut[0];
	localreg0.bit.ife_f_fcurve_yweight_lut1     = p_fcurve->p_y_weight_lut[1];
	localreg0.bit.ife_f_fcurve_yweight_lut2     = p_fcurve->p_y_weight_lut[2];
	localreg0.bit.ife_f_fcurve_yweight_lut3     = p_fcurve->p_y_weight_lut[3];
	IFE_SETREG(RHE_FCURVE_Y_WEIGHT_REGISTER0_OFS, localreg0.reg);

	localreg1.reg = IFE_GETREG(RHE_FCURVE_Y_WEIGHT_REGISTER1_OFS);
	localreg1.bit.ife_f_fcurve_yweight_lut4     = p_fcurve->p_y_weight_lut[4];
	localreg1.bit.ife_f_fcurve_yweight_lut5     = p_fcurve->p_y_weight_lut[5];
	localreg1.bit.ife_f_fcurve_yweight_lut6     = p_fcurve->p_y_weight_lut[6];
	localreg1.bit.ife_f_fcurve_yweight_lut7     = p_fcurve->p_y_weight_lut[7];
	IFE_SETREG(RHE_FCURVE_Y_WEIGHT_REGISTER1_OFS, localreg1.reg);

	localreg2.reg = IFE_GETREG(RHE_FCURVE_Y_WEIGHT_REGISTER2_OFS);
	localreg2.bit.ife_f_fcurve_yweight_lut8     = p_fcurve->p_y_weight_lut[8];
	localreg2.bit.ife_f_fcurve_yweight_lut9     = p_fcurve->p_y_weight_lut[9];
	localreg2.bit.ife_f_fcurve_yweight_lut10     = p_fcurve->p_y_weight_lut[10];
	localreg2.bit.ife_f_fcurve_yweight_lut11     = p_fcurve->p_y_weight_lut[11];
	IFE_SETREG(RHE_FCURVE_Y_WEIGHT_REGISTER2_OFS, localreg2.reg);

	localreg3.reg = IFE_GETREG(RHE_FCURVE_Y_WEIGHT_REGISTER3_OFS);
	localreg3.bit.ife_f_fcurve_yweight_lut12     = p_fcurve->p_y_weight_lut[12];
	localreg3.bit.ife_f_fcurve_yweight_lut13     = p_fcurve->p_y_weight_lut[13];
	localreg3.bit.ife_f_fcurve_yweight_lut14     = p_fcurve->p_y_weight_lut[14];
	localreg3.bit.ife_f_fcurve_yweight_lut15     = p_fcurve->p_y_weight_lut[15];
	IFE_SETREG(RHE_FCURVE_Y_WEIGHT_REGISTER3_OFS, localreg3.reg);

	localreg4.reg = IFE_GETREG(RHE_FCURVE_Y_WEIGHT_REGISTER3_OFS);
	localreg4.bit.ife_f_fcurve_yweight_lut16     = p_fcurve->p_y_weight_lut[16];
	IFE_SETREG(RHE_FCURVE_Y_WEIGHT_REGISTER4_OFS, localreg4.reg);

	return E_OK;
}

static ER ife_set_fcurve_index(IFE_FCURVESET *p_fcurve)
{
	T_RHE_FCURVE_INDEX_REGISTER0          localreg0;
	T_RHE_FCURVE_INDEX_REGISTER1          localreg1;
	T_RHE_FCURVE_INDEX_REGISTER2          localreg2;
	T_RHE_FCURVE_INDEX_REGISTER3          localreg3;
	T_RHE_FCURVE_INDEX_REGISTER4          localreg4;
	T_RHE_FCURVE_INDEX_REGISTER5          localreg5;
	T_RHE_FCURVE_INDEX_REGISTER6          localreg6;
	T_RHE_FCURVE_INDEX_REGISTER7          localreg7;

	localreg0.reg = IFE_GETREG(RHE_FCURVE_INDEX_REGISTER0_OFS);
	localreg0.bit.ife_f_fcurve_index_lut0     = p_fcurve->p_index_lut[0];
	localreg0.bit.ife_f_fcurve_index_lut1     = p_fcurve->p_index_lut[1];
	localreg0.bit.ife_f_fcurve_index_lut2     = p_fcurve->p_index_lut[2];
	localreg0.bit.ife_f_fcurve_index_lut3     = p_fcurve->p_index_lut[3];
	IFE_SETREG(RHE_FCURVE_INDEX_REGISTER0_OFS, localreg0.reg);

	localreg1.reg = IFE_GETREG(RHE_FCURVE_INDEX_REGISTER1_OFS);
	localreg1.bit.ife_f_fcurve_index_lut4     = p_fcurve->p_index_lut[4];
	localreg1.bit.ife_f_fcurve_index_lut5     = p_fcurve->p_index_lut[5];
	localreg1.bit.ife_f_fcurve_index_lut6     = p_fcurve->p_index_lut[6];
	localreg1.bit.ife_f_fcurve_index_lut7     = p_fcurve->p_index_lut[7];
	IFE_SETREG(RHE_FCURVE_INDEX_REGISTER1_OFS, localreg1.reg);

	localreg2.reg = IFE_GETREG(RHE_FCURVE_INDEX_REGISTER2_OFS);
	localreg2.bit.ife_f_fcurve_index_lut8     = p_fcurve->p_index_lut[8];
	localreg2.bit.ife_f_fcurve_index_lut9     = p_fcurve->p_index_lut[9];
	localreg2.bit.ife_f_fcurve_index_lut10     = p_fcurve->p_index_lut[10];
	localreg2.bit.ife_f_fcurve_index_lut11     = p_fcurve->p_index_lut[11];
	IFE_SETREG(RHE_FCURVE_INDEX_REGISTER2_OFS, localreg2.reg);

	localreg3.reg = IFE_GETREG(RHE_FCURVE_INDEX_REGISTER3_OFS);
	localreg3.bit.ife_f_fcurve_index_lut12     = p_fcurve->p_index_lut[12];
	localreg3.bit.ife_f_fcurve_index_lut13     = p_fcurve->p_index_lut[13];
	localreg3.bit.ife_f_fcurve_index_lut14     = p_fcurve->p_index_lut[14];
	localreg3.bit.ife_f_fcurve_index_lut15     = p_fcurve->p_index_lut[15];
	IFE_SETREG(RHE_FCURVE_INDEX_REGISTER3_OFS, localreg3.reg);

	localreg4.reg = IFE_GETREG(RHE_FCURVE_INDEX_REGISTER4_OFS);
	localreg4.bit.ife_f_fcurve_index_lut16     = p_fcurve->p_index_lut[16];
	localreg4.bit.ife_f_fcurve_index_lut17     = p_fcurve->p_index_lut[17];
	localreg4.bit.ife_f_fcurve_index_lut18     = p_fcurve->p_index_lut[18];
	localreg4.bit.ife_f_fcurve_index_lut19     = p_fcurve->p_index_lut[19];
	IFE_SETREG(RHE_FCURVE_INDEX_REGISTER4_OFS, localreg4.reg);

	localreg5.reg = IFE_GETREG(RHE_FCURVE_INDEX_REGISTER5_OFS);
	localreg5.bit.ife_f_fcurve_index_lut20     = p_fcurve->p_index_lut[20];
	localreg5.bit.ife_f_fcurve_index_lut21     = p_fcurve->p_index_lut[21];
	localreg5.bit.ife_f_fcurve_index_lut22     = p_fcurve->p_index_lut[22];
	localreg5.bit.ife_f_fcurve_index_lut23     = p_fcurve->p_index_lut[23];
	IFE_SETREG(RHE_FCURVE_INDEX_REGISTER5_OFS, localreg5.reg);

	localreg6.reg = IFE_GETREG(RHE_FCURVE_INDEX_REGISTER6_OFS);
	localreg6.bit.ife_f_fcurve_index_lut24     = p_fcurve->p_index_lut[24];
	localreg6.bit.ife_f_fcurve_index_lut25     = p_fcurve->p_index_lut[25];
	localreg6.bit.ife_f_fcurve_index_lut26     = p_fcurve->p_index_lut[26];
	localreg6.bit.ife_f_fcurve_index_lut27     = p_fcurve->p_index_lut[27];
	IFE_SETREG(RHE_FCURVE_INDEX_REGISTER6_OFS, localreg6.reg);

	localreg7.reg = IFE_GETREG(RHE_FCURVE_INDEX_REGISTER7_OFS);
	localreg7.bit.ife_f_fcurve_index_lut28     = p_fcurve->p_index_lut[28];
	localreg7.bit.ife_f_fcurve_index_lut29     = p_fcurve->p_index_lut[29];
	localreg7.bit.ife_f_fcurve_index_lut30     = p_fcurve->p_index_lut[30];
	localreg7.bit.ife_f_fcurve_index_lut31     = p_fcurve->p_index_lut[31];
	IFE_SETREG(RHE_FCURVE_INDEX_REGISTER7_OFS, localreg7.reg);

	return E_OK;
}

static ER ife_set_fcurve_split(IFE_FCURVESET *p_fcurve)
{
	T_RHE_FCURVE_SPLIT_REGISTER0          localreg0;
	T_RHE_FCURVE_SPLIT_REGISTER1          localreg1;

	localreg0.reg = IFE_GETREG(RHE_FCURVE_SPLIT_REGISTER0_OFS);
	localreg0.bit.ife_f_fcurve_split_lut0     = p_fcurve->p_split_lut[0];
	localreg0.bit.ife_f_fcurve_split_lut1     = p_fcurve->p_split_lut[1];
	localreg0.bit.ife_f_fcurve_split_lut2     = p_fcurve->p_split_lut[2];
	localreg0.bit.ife_f_fcurve_split_lut3     = p_fcurve->p_split_lut[3];
	localreg0.bit.ife_f_fcurve_split_lut4     = p_fcurve->p_split_lut[4];
	localreg0.bit.ife_f_fcurve_split_lut5     = p_fcurve->p_split_lut[5];
	localreg0.bit.ife_f_fcurve_split_lut6     = p_fcurve->p_split_lut[6];
	localreg0.bit.ife_f_fcurve_split_lut7     = p_fcurve->p_split_lut[7];
	localreg0.bit.ife_f_fcurve_split_lut8     = p_fcurve->p_split_lut[8];
	localreg0.bit.ife_f_fcurve_split_lut9     = p_fcurve->p_split_lut[9];
	localreg0.bit.ife_f_fcurve_split_lut10     = p_fcurve->p_split_lut[10];
	localreg0.bit.ife_f_fcurve_split_lut11     = p_fcurve->p_split_lut[11];
	localreg0.bit.ife_f_fcurve_split_lut12     = p_fcurve->p_split_lut[12];
	localreg0.bit.ife_f_fcurve_split_lut13     = p_fcurve->p_split_lut[13];
	localreg0.bit.ife_f_fcurve_split_lut14     = p_fcurve->p_split_lut[14];
	localreg0.bit.ife_f_fcurve_split_lut15     = p_fcurve->p_split_lut[15];
	IFE_SETREG(RHE_FCURVE_SPLIT_REGISTER0_OFS, localreg0.reg);

	localreg1.reg = IFE_GETREG(RHE_FCURVE_SPLIT_REGISTER1_OFS);
	localreg1.bit.ife_f_fcurve_split_lut16     = p_fcurve->p_split_lut[16];
	localreg1.bit.ife_f_fcurve_split_lut17     = p_fcurve->p_split_lut[17];
	localreg1.bit.ife_f_fcurve_split_lut18     = p_fcurve->p_split_lut[18];
	localreg1.bit.ife_f_fcurve_split_lut19     = p_fcurve->p_split_lut[19];
	localreg1.bit.ife_f_fcurve_split_lut20     = p_fcurve->p_split_lut[20];
	localreg1.bit.ife_f_fcurve_split_lut21     = p_fcurve->p_split_lut[21];
	localreg1.bit.ife_f_fcurve_split_lut22     = p_fcurve->p_split_lut[22];
	localreg1.bit.ife_f_fcurve_split_lut23     = p_fcurve->p_split_lut[23];
	localreg1.bit.ife_f_fcurve_split_lut24     = p_fcurve->p_split_lut[24];
	localreg1.bit.ife_f_fcurve_split_lut25     = p_fcurve->p_split_lut[25];
	localreg1.bit.ife_f_fcurve_split_lut26     = p_fcurve->p_split_lut[26];
	localreg1.bit.ife_f_fcurve_split_lut27     = p_fcurve->p_split_lut[27];
	localreg1.bit.ife_f_fcurve_split_lut28     = p_fcurve->p_split_lut[28];
	localreg1.bit.ife_f_fcurve_split_lut29     = p_fcurve->p_split_lut[29];
	localreg1.bit.ife_f_fcurve_split_lut30     = p_fcurve->p_split_lut[30];
	localreg1.bit.ife_f_fcurve_split_lut31     = p_fcurve->p_split_lut[31];
	IFE_SETREG(RHE_FCURVE_SPLIT_REGISTER1_OFS, localreg1.reg);

	return E_OK;
}

static ER ife_set_fcurve_value(IFE_FCURVESET *p_fcurve)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_RHE_FCURVE_VALUE_REGISTER0           localreg0;
	T_RHE_FCURVE_VALUE_REGISTER1           localreg1;
	T_RHE_FCURVE_VALUE_REGISTER2           localreg2;
	T_RHE_FCURVE_VALUE_REGISTER3           localreg3;
	T_RHE_FCURVE_VALUE_REGISTER4           localreg4;
	T_RHE_FCURVE_VALUE_REGISTER5           localreg5;
	T_RHE_FCURVE_VALUE_REGISTER6           localreg6;
	T_RHE_FCURVE_VALUE_REGISTER7           localreg7;
	T_RHE_FCURVE_VALUE_REGISTER8           localreg8;
	T_RHE_FCURVE_VALUE_REGISTER9           localreg9;
	T_RHE_FCURVE_VALUE_REGISTER10          localreg10;
	T_RHE_FCURVE_VALUE_REGISTER11          localreg11;
	T_RHE_FCURVE_VALUE_REGISTER12          localreg12;
	T_RHE_FCURVE_VALUE_REGISTER13          localreg13;
	T_RHE_FCURVE_VALUE_REGISTER14          localreg14;
	T_RHE_FCURVE_VALUE_REGISTER15          localreg15;
	T_RHE_FCURVE_VALUE_REGISTER16          localreg16;
	T_RHE_FCURVE_VALUE_REGISTER17          localreg17;
	T_RHE_FCURVE_VALUE_REGISTER18          localreg18;
	T_RHE_FCURVE_VALUE_REGISTER19          localreg19;
	T_RHE_FCURVE_VALUE_REGISTER20          localreg20;
	T_RHE_FCURVE_VALUE_REGISTER21          localreg21;
	T_RHE_FCURVE_VALUE_REGISTER22          localreg22;
	T_RHE_FCURVE_VALUE_REGISTER23          localreg23;
	T_RHE_FCURVE_VALUE_REGISTER24          localreg24;
	T_RHE_FCURVE_VALUE_REGISTER25          localreg25;
	T_RHE_FCURVE_VALUE_REGISTER26          localreg26;
	T_RHE_FCURVE_VALUE_REGISTER27          localreg27;
	T_RHE_FCURVE_VALUE_REGISTER28          localreg28;
	T_RHE_FCURVE_VALUE_REGISTER29          localreg29;
	T_RHE_FCURVE_VALUE_REGISTER30          localreg30;
	T_RHE_FCURVE_VALUE_REGISTER31          localreg31;
#endif

	T_RHE_FCURVE_VALUE_REGISTER32          localreg32;

#if 0
	localreg0.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER0_OFS);
	localreg0.bit.ife_f_fcurve_val_lut0     = p_fcurve->p_value_lut[0];
	localreg0.bit.ife_f_fcurve_val_lut1     = p_fcurve->p_value_lut[1];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER0_OFS, localreg0.reg);

	localreg1.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER1_OFS);
	localreg1.bit.ife_f_fcurve_val_lut2     = p_fcurve->p_value_lut[2];
	localreg1.bit.ife_f_fcurve_val_lut3     = p_fcurve->p_value_lut[3];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER1_OFS, localreg1.reg);

	localreg2.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER2_OFS);
	localreg2.bit.ife_f_fcurve_val_lut4     = p_fcurve->p_value_lut[4];
	localreg2.bit.ife_f_fcurve_val_lut5     = p_fcurve->p_value_lut[5];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER2_OFS, localreg2.reg);

	localreg3.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER3_OFS);
	localreg3.bit.ife_f_fcurve_val_lut6     = p_fcurve->p_value_lut[6];
	localreg3.bit.ife_f_fcurve_val_lut7     = p_fcurve->p_value_lut[7];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER3_OFS, localreg3.reg);

	localreg4.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER4_OFS);
	localreg4.bit.ife_f_fcurve_val_lut8     = p_fcurve->p_value_lut[8];
	localreg4.bit.ife_f_fcurve_val_lut9     = p_fcurve->p_value_lut[9];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER4_OFS, localreg4.reg);

	localreg5.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER5_OFS);
	localreg5.bit.ife_f_fcurve_val_lut10     = p_fcurve->p_value_lut[10];
	localreg5.bit.ife_f_fcurve_val_lut11     = p_fcurve->p_value_lut[11];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER5_OFS, localreg5.reg);

	localreg6.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER6_OFS);
	localreg6.bit.ife_f_fcurve_val_lut12     = p_fcurve->p_value_lut[12];
	localreg6.bit.ife_f_fcurve_val_lut13     = p_fcurve->p_value_lut[13];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER6_OFS, localreg6.reg);

	localreg7.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER7_OFS);
	localreg7.bit.ife_f_fcurve_val_lut14     = p_fcurve->p_value_lut[14];
	localreg7.bit.ife_f_fcurve_val_lut15     = p_fcurve->p_value_lut[15];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER7_OFS, localreg7.reg);

	localreg8.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER8_OFS);
	localreg8.bit.ife_f_fcurve_val_lut16     = p_fcurve->p_value_lut[16];
	localreg8.bit.ife_f_fcurve_val_lut17     = p_fcurve->p_value_lut[17];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER8_OFS, localreg8.reg);

	localreg9.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER9_OFS);
	localreg9.bit.ife_f_fcurve_val_lut18     = p_fcurve->p_value_lut[18];
	localreg9.bit.ife_f_fcurve_val_lut19     = p_fcurve->p_value_lut[19];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER9_OFS, localreg9.reg);

	localreg10.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER10_OFS);
	localreg10.bit.ife_f_fcurve_val_lut20     = p_fcurve->p_value_lut[20];
	localreg10.bit.ife_f_fcurve_val_lut21     = p_fcurve->p_value_lut[21];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER10_OFS, localreg10.reg);

	localreg11.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER11_OFS);
	localreg11.bit.ife_f_fcurve_val_lut22     = p_fcurve->p_value_lut[22];
	localreg11.bit.ife_f_fcurve_val_lut23     = p_fcurve->p_value_lut[23];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER11_OFS, localreg11.reg);

	localreg12.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER12_OFS);
	localreg12.bit.ife_f_fcurve_val_lut24     = p_fcurve->p_value_lut[24];
	localreg12.bit.ife_f_fcurve_val_lut25     = p_fcurve->p_value_lut[25];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER12_OFS, localreg12.reg);

	localreg13.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER13_OFS);
	localreg13.bit.ife_f_fcurve_val_lut26     = p_fcurve->p_value_lut[26];
	localreg13.bit.ife_f_fcurve_val_lut27     = p_fcurve->p_value_lut[27];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER13_OFS, localreg13.reg);

	localreg14.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER14_OFS);
	localreg14.bit.ife_f_fcurve_val_lut28     = p_fcurve->p_value_lut[28];
	localreg14.bit.ife_f_fcurve_val_lut29     = p_fcurve->p_value_lut[29];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER14_OFS, localreg14.reg);

	localreg15.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER15_OFS);
	localreg15.bit.ife_f_fcurve_val_lut30     = p_fcurve->p_value_lut[30];
	localreg15.bit.ife_f_fcurve_val_lut31     = p_fcurve->p_value_lut[31];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER15_OFS, localreg15.reg);

	localreg16.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER16_OFS);
	localreg16.bit.ife_f_fcurve_val_lut32     = p_fcurve->p_value_lut[32];
	localreg16.bit.ife_f_fcurve_val_lut33     = p_fcurve->p_value_lut[33];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER16_OFS, localreg16.reg);

	localreg17.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER17_OFS);
	localreg17.bit.ife_f_fcurve_val_lut34     = p_fcurve->p_value_lut[34];
	localreg17.bit.ife_f_fcurve_val_lut35     = p_fcurve->p_value_lut[35];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER17_OFS, localreg17.reg);

	localreg18.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER18_OFS);
	localreg18.bit.ife_f_fcurve_val_lut36     = p_fcurve->p_value_lut[36];
	localreg18.bit.ife_f_fcurve_val_lut37     = p_fcurve->p_value_lut[37];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER18_OFS, localreg18.reg);

	localreg19.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER19_OFS);
	localreg19.bit.ife_f_fcurve_val_lut38     = p_fcurve->p_value_lut[38];
	localreg19.bit.ife_f_fcurve_val_lut39     = p_fcurve->p_value_lut[39];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER19_OFS, localreg19.reg);

	localreg20.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER20_OFS);
	localreg20.bit.ife_f_fcurve_val_lut40     = p_fcurve->p_value_lut[40];
	localreg20.bit.ife_f_fcurve_val_lut41     = p_fcurve->p_value_lut[41];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER20_OFS, localreg20.reg);

	localreg21.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER21_OFS);
	localreg21.bit.ife_f_fcurve_val_lut42     = p_fcurve->p_value_lut[42];
	localreg21.bit.ife_f_fcurve_val_lut43     = p_fcurve->p_value_lut[43];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER21_OFS, localreg21.reg);

	localreg22.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER22_OFS);
	localreg22.bit.ife_f_fcurve_val_lut44     = p_fcurve->p_value_lut[44];
	localreg22.bit.ife_f_fcurve_val_lut45     = p_fcurve->p_value_lut[45];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER22_OFS, localreg22.reg);

	localreg23.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER23_OFS);
	localreg23.bit.ife_f_fcurve_val_lut46     = p_fcurve->p_value_lut[46];
	localreg23.bit.ife_f_fcurve_val_lut47     = p_fcurve->p_value_lut[47];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER23_OFS, localreg23.reg);

	localreg24.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER24_OFS);
	localreg24.bit.ife_f_fcurve_val_lut48     = p_fcurve->p_value_lut[48];
	localreg24.bit.ife_f_fcurve_val_lut49     = p_fcurve->p_value_lut[49];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER24_OFS, localreg24.reg);

	localreg25.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER25_OFS);
	localreg25.bit.ife_f_fcurve_val_lut50     = p_fcurve->p_value_lut[50];
	localreg25.bit.ife_f_fcurve_val_lut51     = p_fcurve->p_value_lut[51];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER25_OFS, localreg25.reg);

	localreg26.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER26_OFS);
	localreg26.bit.ife_f_fcurve_val_lut52     = p_fcurve->p_value_lut[52];
	localreg26.bit.ife_f_fcurve_val_lut53     = p_fcurve->p_value_lut[53];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER26_OFS, localreg26.reg);

	localreg27.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER27_OFS);
	localreg27.bit.ife_f_fcurve_val_lut54     = p_fcurve->p_value_lut[54];
	localreg27.bit.ife_f_fcurve_val_lut55     = p_fcurve->p_value_lut[55];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER27_OFS, localreg27.reg);

	localreg28.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER28_OFS);
	localreg28.bit.ife_f_fcurve_val_lut56     = p_fcurve->p_value_lut[56];
	localreg28.bit.ife_f_fcurve_val_lut57     = p_fcurve->p_value_lut[57];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER28_OFS, localreg28.reg);

	localreg29.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER29_OFS);
	localreg29.bit.ife_f_fcurve_val_lut58     = p_fcurve->p_value_lut[58];
	localreg29.bit.ife_f_fcurve_val_lut59     = p_fcurve->p_value_lut[59];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER29_OFS, localreg29.reg);

	localreg30.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER30_OFS);
	localreg30.bit.ife_f_fcurve_val_lut60     = p_fcurve->p_value_lut[60];
	localreg30.bit.ife_f_fcurve_val_lut61     = p_fcurve->p_value_lut[61];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER30_OFS, localreg30.reg);

	localreg31.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER31_OFS);
	localreg31.bit.ife_f_fcurve_val_lut62     = p_fcurve->p_value_lut[62];
	localreg31.bit.ife_f_fcurve_val_lut63     = p_fcurve->p_value_lut[63];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER31_OFS, localreg31.reg);
#endif

	for (i = 0; i < 32; i++) {
		reg_set_val = 0;

		reg_ofs = RHE_FCURVE_VALUE_REGISTER0_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_fcurve->p_value_lut[idx]) + (p_fcurve->p_value_lut[idx + 1] << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

	localreg32.reg = IFE_GETREG(RHE_FCURVE_VALUE_REGISTER32_OFS);
	localreg32.bit.ife_f_fcurve_val_lut64     = p_fcurve->p_value_lut[64];
	IFE_SETREG(RHE_FCURVE_VALUE_REGISTER32_OFS, localreg32.reg);

	return E_OK;
}

static ER ife_set_fusion_ctrl(IFE_FUSIONSET *p_fusion)
{
	T_RHE_FUSION_REGISTER          localreg0;

	localreg0.reg = IFE_GETREG(RHE_FUSION_REGISTER_OFS);
	localreg0.bit.ife_f_fusion_ymean_sel                  = p_fusion->y_mean_sel;
	localreg0.bit.ife_f_fusion_normal_blend_curve_sel     = p_fusion->nor_blendcur_sel;
	localreg0.bit.ife_f_fusion_diff_blend_curve_sel       = p_fusion->diff_blendcur_sel;
	localreg0.bit.ife_f_fusion_mode                       = p_fusion->mode;
	localreg0.bit.ife_f_fusion_evratio                    = p_fusion->ev_ratio;

	IFE_SETREG(RHE_FUSION_REGISTER_OFS, localreg0.reg);

	return E_OK;
}


static ER ife_set_fusion_blend_curve(IFE_FUSIONSET *p_fusion)
{
	T_RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE          localreg0;
	T_RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE         localreg1;
	T_RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE            localreg2;
	T_RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE           localreg3;

#if (defined(_NVT_EMULATION_) == OFF)
	UINT32 point1_value = 0;
#endif

	localreg0.reg = IFE_GETREG(RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE_OFS);
	localreg0.bit.ife_f_fusion_long_exp_normal_blend_curve_knee_point0     = p_fusion->p_long_nor_blendcur_knee[0];
	localreg0.bit.ife_f_fusion_long_exp_normal_blend_curve_range           = p_fusion->long_nor_blendcur_range;
#if (defined(_NVT_EMULATION_) == ON)
	localreg0.bit.ife_f_fusion_long_exp_normal_blend_curve_knee_point1     = p_fusion->p_long_nor_blendcur_knee[1];//p_fusion->p_long_nor_blendcur_knee[0] + (1 << p_fusion->long_nor_blendcur_range); //
#else
	point1_value = (p_fusion->p_long_nor_blendcur_knee[0] + (1 << p_fusion->long_nor_blendcur_range));
	if(point1_value > 4095)
		point1_value = 4095;
	localreg0.bit.ife_f_fusion_long_exp_normal_blend_curve_knee_point1     = point1_value;
#endif
	localreg0.bit.ife_f_fusion_long_exp_normal_blend_curve_wedge           = p_fusion->long_nor_blendcur_wedge;
	IFE_SETREG(RHE_FUSION_LONG_EXP_NORMAL_BLEND_CURVE_OFS, localreg0.reg);

	localreg1.reg = IFE_GETREG(RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE_OFS);
	localreg1.bit.ife_f_fusion_short_exp_normal_blend_curve_knee_point0     = p_fusion->p_short_nor_blendcur_knee[0];
	localreg1.bit.ife_f_fusion_short_exp_normal_blend_curve_range           = p_fusion->short_nor_blendcur_range;
#if (defined(_NVT_EMULATION_) == ON)
	localreg1.bit.ife_f_fusion_short_exp_normal_blend_curve_knee_point1     = p_fusion->p_short_nor_blendcur_knee[1];
#else
	point1_value = (p_fusion->p_short_nor_blendcur_knee[0] + (1 << p_fusion->short_nor_blendcur_range));
	if(point1_value > 4095)
		point1_value = 4095;
	localreg1.bit.ife_f_fusion_short_exp_normal_blend_curve_knee_point1     = point1_value;
#endif
	localreg1.bit.ife_f_fusion_short_exp_normal_blend_curve_wedge           = p_fusion->short_nor_blendcur_wedge;
	IFE_SETREG(RHE_FUSION_SHORT_EXP_NORMAL_BLEND_CURVE_OFS, localreg1.reg);

	localreg2.reg = IFE_GETREG(RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE_OFS);
	localreg2.bit.ife_f_fusion_long_exp_diff_blend_curve_knee_point0     = p_fusion->p_long_diff_blendcur_knee[0];
	localreg2.bit.ife_f_fusion_long_exp_diff_blend_curve_range           = p_fusion->long_diff_blendcur_range;
#if (defined(_NVT_EMULATION_) == ON)
	localreg2.bit.ife_f_fusion_long_exp_diff_blend_curve_knee_point1     = p_fusion->p_long_diff_blendcur_knee[1];
#else
	point1_value = (p_fusion->p_long_diff_blendcur_knee[0] + (1 << p_fusion->long_diff_blendcur_range));
	if(point1_value > 4095)
		point1_value = 4095;
	localreg2.bit.ife_f_fusion_long_exp_diff_blend_curve_knee_point1     = point1_value;
#endif
	localreg2.bit.ife_f_fusion_long_exp_diff_blend_curve_wedge           = p_fusion->long_diff_blendcur_wedge;
	IFE_SETREG(RHE_FUSION_LONG_EXP_DIFF_BLEND_CURVE_OFS, localreg2.reg);

	localreg3.reg = IFE_GETREG(RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE_OFS);
	localreg3.bit.ife_f_fusion_short_exp_diff_blend_curve_knee_point0     = p_fusion->p_short_diff_blendcur_knee[0];
	localreg3.bit.ife_f_fusion_short_exp_diff_blend_curve_range           = p_fusion->short_diff_blendcur_range;
#if (defined(_NVT_EMULATION_) == ON)
	localreg3.bit.ife_f_fusion_short_exp_diff_blend_curve_knee_point1     = p_fusion->p_short_diff_blendcur_knee[1];
#else
	point1_value = (p_fusion->p_short_diff_blendcur_knee[0] + (1 << p_fusion->short_diff_blendcur_range));
	if(point1_value > 4095)
		point1_value = 4095;
	localreg3.bit.ife_f_fusion_short_exp_diff_blend_curve_knee_point1     = point1_value;
#endif
	localreg3.bit.ife_f_fusion_short_exp_diff_blend_curve_wedge           = p_fusion->short_diff_blendcur_wedge;
	IFE_SETREG(RHE_FUSION_SHORT_EXP_DIFF_BLEND_CURVE_OFS, localreg3.reg);

	return E_OK;
}


static ER ife_set_fusion_diff_weight(IFE_FUSIONSET *p_fusion)
{
	T_RHE_FUSION_MOTION_COMPENSATION                localregmc;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_0          localregmc0;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_1          localregmc1;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_2          localregmc2;

	T_RHE_FUSION_MOTION_COMPENSATION_LUT_3          localregmc3;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_4          localregmc4;
	T_RHE_FUSION_MOTION_COMPENSATION_LUT_5          localregmc5;

	localregmc.reg = IFE_GETREG(RHE_FUSION_MOTION_COMPENSATION_OFS);
	localregmc.bit.ife_f_fusion_mc_lumthr          = p_fusion->mc_lum_th;
	localregmc.bit.ife_f_fusion_mc_diff_ratio      = p_fusion->mc_diff_ratio;
	IFE_SETREG(RHE_FUSION_MOTION_COMPENSATION_OFS, localregmc.reg);

	localregmc0.reg = IFE_GETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_0_OFS);
	localregmc0.bit.ife_f_fusion_mc_lut_positive_diff_weight0     = p_fusion->p_mc_pos_diff_w[0];
	localregmc0.bit.ife_f_fusion_mc_lut_positive_diff_weight1     = p_fusion->p_mc_pos_diff_w[1];
	localregmc0.bit.ife_f_fusion_mc_lut_positive_diff_weight2     = p_fusion->p_mc_pos_diff_w[2];
	localregmc0.bit.ife_f_fusion_mc_lut_positive_diff_weight3     = p_fusion->p_mc_pos_diff_w[3];
	localregmc0.bit.ife_f_fusion_mc_lut_positive_diff_weight4     = p_fusion->p_mc_pos_diff_w[4];
	localregmc0.bit.ife_f_fusion_mc_lut_positive_diff_weight5     = p_fusion->p_mc_pos_diff_w[5];
	IFE_SETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_0_OFS, localregmc0.reg);

	localregmc1.reg = IFE_GETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_1_OFS);
	localregmc1.bit.ife_f_fusion_mc_lut_positive_diff_weight6     = p_fusion->p_mc_pos_diff_w[6];
	localregmc1.bit.ife_f_fusion_mc_lut_positive_diff_weight7     = p_fusion->p_mc_pos_diff_w[7];
	localregmc1.bit.ife_f_fusion_mc_lut_positive_diff_weight8     = p_fusion->p_mc_pos_diff_w[8];
	localregmc1.bit.ife_f_fusion_mc_lut_positive_diff_weight9     = p_fusion->p_mc_pos_diff_w[9];
	localregmc1.bit.ife_f_fusion_mc_lut_positive_diff_weight10     = p_fusion->p_mc_pos_diff_w[10];
	localregmc1.bit.ife_f_fusion_mc_lut_positive_diff_weight11     = p_fusion->p_mc_pos_diff_w[11];
	IFE_SETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_1_OFS, localregmc1.reg);

	localregmc2.reg = IFE_GETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_2_OFS);
	localregmc2.bit.ife_f_fusion_mc_lut_positive_diff_weight12     = p_fusion->p_mc_pos_diff_w[12];
	localregmc2.bit.ife_f_fusion_mc_lut_positive_diff_weight13     = p_fusion->p_mc_pos_diff_w[13];
	localregmc2.bit.ife_f_fusion_mc_lut_positive_diff_weight14     = p_fusion->p_mc_pos_diff_w[14];
	localregmc2.bit.ife_f_fusion_mc_lut_positive_diff_weight15     = p_fusion->p_mc_pos_diff_w[15];
	localregmc2.bit.ife_f_fusion_mc_lut_difflumth_diff_weight     = p_fusion->mc_diff_lumth_diff_w;
	IFE_SETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_2_OFS, localregmc2.reg);

	localregmc3.reg = IFE_GETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_3_OFS);
	localregmc3.bit.ife_f_fusion_mc_lut_negative_diff_weight0     = p_fusion->p_mc_neg_diff_w[0];
	localregmc3.bit.ife_f_fusion_mc_lut_negative_diff_weight1     = p_fusion->p_mc_neg_diff_w[1];
	localregmc3.bit.ife_f_fusion_mc_lut_negative_diff_weight2     = p_fusion->p_mc_neg_diff_w[2];
	localregmc3.bit.ife_f_fusion_mc_lut_negative_diff_weight3     = p_fusion->p_mc_neg_diff_w[3];
	localregmc3.bit.ife_f_fusion_mc_lut_negative_diff_weight4     = p_fusion->p_mc_neg_diff_w[4];
	localregmc3.bit.ife_f_fusion_mc_lut_negative_diff_weight5     = p_fusion->p_mc_neg_diff_w[5];
	IFE_SETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_3_OFS, localregmc3.reg);

	localregmc4.reg = IFE_GETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_4_OFS);
	localregmc4.bit.ife_f_fusion_mc_lut_negative_diff_weight6     = p_fusion->p_mc_neg_diff_w[6];
	localregmc4.bit.ife_f_fusion_mc_lut_negative_diff_weight7     = p_fusion->p_mc_neg_diff_w[7];
	localregmc4.bit.ife_f_fusion_mc_lut_negative_diff_weight8     = p_fusion->p_mc_neg_diff_w[8];
	localregmc4.bit.ife_f_fusion_mc_lut_negative_diff_weight9     = p_fusion->p_mc_neg_diff_w[9];
	localregmc4.bit.ife_f_fusion_mc_lut_negative_diff_weight10     = p_fusion->p_mc_neg_diff_w[10];
	localregmc4.bit.ife_f_fusion_mc_lut_negative_diff_weight11     = p_fusion->p_mc_neg_diff_w[11];
	IFE_SETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_4_OFS, localregmc4.reg);

	localregmc5.reg = IFE_GETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_5_OFS);
	localregmc5.bit.ife_f_fusion_mc_lut_negative_diff_weight12     = p_fusion->p_mc_neg_diff_w[12];
	localregmc5.bit.ife_f_fusion_mc_lut_negative_diff_weight13     = p_fusion->p_mc_neg_diff_w[13];
	localregmc5.bit.ife_f_fusion_mc_lut_negative_diff_weight14     = p_fusion->p_mc_neg_diff_w[14];
	localregmc5.bit.ife_f_fusion_mc_lut_negative_diff_weight15     = p_fusion->p_mc_neg_diff_w[15];
	IFE_SETREG(RHE_FUSION_MOTION_COMPENSATION_LUT_5_OFS, localregmc5.reg);

	return E_OK;
}

static ER ife_set_fusion_dark_saturation_reduction(IFE_FUSIONSET *p_fusion)
{

	T_RHE_FUSION_PATH0_DARK_SATURATION_REDUCTION          localreg0;
	T_RHE_FUSION_PATH1_DARK_SATURATION_REDUCTION          localreg1;

	localreg0.reg = IFE_GETREG(RHE_FUSION_PATH0_DARK_SATURATION_REDUCTION_OFS);
	localreg0.bit.ife_f_dark_sat_reduction0_th              = p_fusion->p_dark_sat_reduce_th[0];
	localreg0.bit.ife_f_dark_sat_reduction0_step            = p_fusion->p_dark_sat_reduce_step[0];
	localreg0.bit.ife_f_dark_sat_reduction0_low_bound       = p_fusion->p_dark_sat_reduce_lowbound[0];
	IFE_SETREG(RHE_FUSION_PATH0_DARK_SATURATION_REDUCTION_OFS, localreg0.reg);

	localreg1.reg = IFE_GETREG(RHE_FUSION_PATH1_DARK_SATURATION_REDUCTION_OFS);
	localreg1.bit.ife_f_dark_sat_reduction1_th              = p_fusion->p_dark_sat_reduce_th[1];
	localreg1.bit.ife_f_dark_sat_reduction1_step            = p_fusion->p_dark_sat_reduce_step[1];
	localreg1.bit.ife_f_dark_sat_reduction1_low_bound       = p_fusion->p_dark_sat_reduce_lowbound[1];
	IFE_SETREG(RHE_FUSION_PATH1_DARK_SATURATION_REDUCTION_OFS, localreg1.reg);

	return E_OK;
}

static ER ife_set_fusion_short_exposure_compress(IFE_FUSIONSET *p_fusion)
{

	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_0          localreg_scom0;
	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1          localreg_scom1;
	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_2          localreg_scom2;
	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_3          localreg_scom3;
	T_RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_4          localreg_scom4;

	localreg_scom0.reg = IFE_GETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_0_OFS);
	localreg_scom0.bit.ife_f_fusion_short_exposure_compress_knee_point0     = p_fusion->p_short_comp_knee_point[0];
	localreg_scom0.bit.ife_f_fusion_short_exposure_compress_knee_point1     = p_fusion->p_short_comp_knee_point[1];
	IFE_SETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_0_OFS, localreg_scom0.reg);

	localreg_scom1.reg = IFE_GETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1_OFS);
	localreg_scom1.bit.ife_f_fusion_short_exposure_compress_knee_point2     = p_fusion->p_short_comp_knee_point[2];
	IFE_SETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_1_OFS, localreg_scom1.reg);

	localreg_scom2.reg = IFE_GETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_2_OFS);
	localreg_scom2.bit.ife_f_fusion_short_exposure_compress_subtract_point0     = p_fusion->p_short_comp_sub_point[0];
	localreg_scom2.bit.ife_f_fusion_short_exposure_compress_subtract_point1     = p_fusion->p_short_comp_sub_point[1];
	IFE_SETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_2_OFS, localreg_scom2.reg);

	localreg_scom3.reg = IFE_GETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_3_OFS);
	localreg_scom3.bit.ife_f_fusion_short_exposure_compress_subtract_point2     = p_fusion->p_short_comp_sub_point[2];
	localreg_scom3.bit.ife_f_fusion_short_exposure_compress_subtract_point3     = p_fusion->p_short_comp_sub_point[3];
	IFE_SETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_3_OFS, localreg_scom3.reg);

	localreg_scom4.reg = IFE_GETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_4_OFS);
	localreg_scom4.bit.ife_f_fusion_short_exposure_compress_shift_bit0     = p_fusion->p_short_comp_shift_bit[0];
	localreg_scom4.bit.ife_f_fusion_short_exposure_compress_shift_bit1     = p_fusion->p_short_comp_shift_bit[1];
	localreg_scom4.bit.ife_f_fusion_short_exposure_compress_shift_bit2     = p_fusion->p_short_comp_shift_bit[2];
	localreg_scom4.bit.ife_f_fusion_short_exposure_compress_shift_bit3     = p_fusion->p_short_comp_shift_bit[3];
	IFE_SETREG(RHE_FUSION_SHORT_EXPOSURE_COMPRESS_CTRL_4_OFS, localreg_scom4.reg);

	return E_OK;
}

static void ife_set_fusion_color_gain(IFE_FCGAINSET *p_fcgain)
{
	T_COLOR_GAIN_REGISTER_0     localreg0;

	T_COLOR_GAIN_REGISTER_4    localreg4;
	T_COLOR_GAIN_REGISTER_5    localreg5;
	T_COLOR_GAIN_REGISTER_6    localreg6;
	T_COLOR_GAIN_REGISTER_7    localreg7;
	T_COLOR_GAIN_REGISTER_8    localreg8;
	T_COLOR_GAIN_REGISTER_9    localreg9;

	T_COLOR_OFFSET_REGISTER_3  localreg13;
	T_COLOR_OFFSET_REGISTER_4  localreg14;
	T_COLOR_OFFSET_REGISTER_5  localreg15;
	T_COLOR_OFFSET_REGISTER_6  localreg16;
	T_COLOR_OFFSET_REGISTER_7  localreg17;
	T_COLOR_OFFSET_REGISTER_8  localreg18;

	localreg0.reg = IFE_GETREG(COLOR_GAIN_REGISTER_0_OFS);
	localreg4.reg = IFE_GETREG(COLOR_GAIN_REGISTER_4_OFS);
	localreg5.reg = IFE_GETREG(COLOR_GAIN_REGISTER_5_OFS);
	localreg6.reg = IFE_GETREG(COLOR_GAIN_REGISTER_6_OFS);
	localreg7.reg = IFE_GETREG(COLOR_GAIN_REGISTER_7_OFS);
	localreg8.reg = IFE_GETREG(COLOR_GAIN_REGISTER_8_OFS);
	localreg9.reg = IFE_GETREG(COLOR_GAIN_REGISTER_9_OFS);

	localreg13.reg = IFE_GETREG(COLOR_OFFSET_REGISTER_3_OFS);
	localreg14.reg = IFE_GETREG(COLOR_OFFSET_REGISTER_4_OFS);
	localreg15.reg = IFE_GETREG(COLOR_OFFSET_REGISTER_5_OFS);
	localreg16.reg = IFE_GETREG(COLOR_OFFSET_REGISTER_6_OFS);
	localreg17.reg = IFE_GETREG(COLOR_OFFSET_REGISTER_7_OFS);
	localreg18.reg = IFE_GETREG(COLOR_OFFSET_REGISTER_8_OFS);

	localreg0.bit.ife_f_cgain_range     = p_fcgain->fcgain_range;

	localreg4.bit.ife_f_p0_cgain_r      = p_fcgain->p_fusion_cgain_path0[0];
	localreg4.bit.ife_f_p0_cgain_gr     = p_fcgain->p_fusion_cgain_path0[1];
	localreg5.bit.ife_f_p0_cgain_gb     = p_fcgain->p_fusion_cgain_path0[2];
	localreg5.bit.ife_f_p0_cgain_b      = p_fcgain->p_fusion_cgain_path0[3];
	localreg6.bit.ife_f_p0_cgain_ir     = p_fcgain->p_fusion_cgain_path0[4];

	localreg7.bit.ife_f_p1_cgain_r      = p_fcgain->p_fusion_cgain_path1[0];
	localreg7.bit.ife_f_p1_cgain_gr     = p_fcgain->p_fusion_cgain_path1[1];
	localreg8.bit.ife_f_p1_cgain_gb     = p_fcgain->p_fusion_cgain_path1[2];
	localreg8.bit.ife_f_p1_cgain_b      = p_fcgain->p_fusion_cgain_path1[3];
	localreg9.bit.ife_f_p1_cgain_ir     = p_fcgain->p_fusion_cgain_path1[4];

	localreg13.bit.ife_f_p0_cofs_r      = p_fcgain->p_fusion_cofs_path0[0];
	localreg13.bit.ife_f_p0_cofs_gr     = p_fcgain->p_fusion_cofs_path0[1];
	localreg14.bit.ife_f_p0_cofs_gb     = p_fcgain->p_fusion_cofs_path0[2];
	localreg14.bit.ife_f_p0_cofs_b      = p_fcgain->p_fusion_cofs_path0[3];
	localreg15.bit.ife_f_p0_cofs_ir     = p_fcgain->p_fusion_cofs_path0[4];

	localreg16.bit.ife_f_p1_cofs_r      = p_fcgain->p_fusion_cofs_path1[0];
	localreg16.bit.ife_f_p1_cofs_gr     = p_fcgain->p_fusion_cofs_path1[1];
	localreg17.bit.ife_f_p1_cofs_gb     = p_fcgain->p_fusion_cofs_path1[2];
	localreg17.bit.ife_f_p1_cofs_b      = p_fcgain->p_fusion_cofs_path1[3];
	localreg18.bit.ife_f_p1_cofs_ir     = p_fcgain->p_fusion_cofs_path1[4];

	IFE_SETREG(COLOR_GAIN_REGISTER_0_OFS, localreg0.reg);
	IFE_SETREG(COLOR_GAIN_REGISTER_4_OFS, localreg4.reg);
	IFE_SETREG(COLOR_GAIN_REGISTER_5_OFS, localreg5.reg);
	IFE_SETREG(COLOR_GAIN_REGISTER_6_OFS, localreg6.reg);
	IFE_SETREG(COLOR_GAIN_REGISTER_7_OFS, localreg7.reg);
	IFE_SETREG(COLOR_GAIN_REGISTER_8_OFS, localreg8.reg);
	IFE_SETREG(COLOR_GAIN_REGISTER_9_OFS, localreg9.reg);

	IFE_SETREG(COLOR_OFFSET_REGISTER_3_OFS, localreg13.reg);
	IFE_SETREG(COLOR_OFFSET_REGISTER_4_OFS, localreg14.reg);
	IFE_SETREG(COLOR_OFFSET_REGISTER_5_OFS, localreg15.reg);
	IFE_SETREG(COLOR_OFFSET_REGISTER_6_OFS, localreg16.reg);
	IFE_SETREG(COLOR_OFFSET_REGISTER_7_OFS, localreg17.reg);
	IFE_SETREG(COLOR_OFFSET_REGISTER_8_OFS, localreg18.reg);
}
#if 0
static ER ife_set_row_def_conc(IFE_ROWDEFSET *p_row_def_set)
{
	T_ROW_DEFECT_CONCEILMENT_REGISTER_1 localreg;
	T_ROW_DEFECT_CONCEILMENT_REGISTER_2 localreg1;

	UINT32 i;

	localreg.reg = IFE_GETREG(ROW_DEFECT_CONCEILMENT_REGISTER_1_OFS);
	localreg.bit.ROWDEF_FACT = p_row_def_set->row_def_fact;
	localreg.bit.ROWDEF_STIDX = p_row_def_set->ui_st_idx;
	localreg.bit.ROWDEF_OFFS = p_row_def_set->ui_offs;
	IFE_SETREG(ROW_DEFECT_CONCEILMENT_REGISTER_1_OFS, localreg.reg);
	for (i = 0; i < 8; i++) {
		localreg1.reg = IFE_GETREG(ROW_DEFECT_CONCEILMENT_REGISTER_2_OFS + (i << 2));
		localreg1.bit.ROW0 = p_row_def_set->p_row_def_tbl[(i << 1)];
		localreg1.bit.ROW1 = p_row_def_set->p_row_def_tbl[(i << 1) + 1];
		IFE_SETREG(ROW_DEFECT_CONCEILMENT_REGISTER_2_OFS + (i << 2), localreg1.reg);
	}
	return E_OK;
}
#endif

static ER ife_set2dnr_rbfill_ratio_mode(IFE_RBFill_PARAM *p_rbfill_set)
{
	T_RB_RATIO_REGISTER_7         localreg17;

	localreg17.reg = IFE_GETREG(RB_RATIO_REGISTER_7_OFS);

	localreg17.bit.rbratio_mode     = p_rbfill_set->rbfill_ratio_mode;

	IFE_SETREG(RB_RATIO_REGISTER_7_OFS, localreg17.reg);

	return E_OK;
}

static ER ife_set2dnr_rbfill(IFE_RBFill_PARAM *p_rbfill_set)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_RB_LUMINANCE_REGISTER_0     localreg0;
	T_RB_LUMINANCE_REGISTER_1     localreg1;
	T_RB_LUMINANCE_REGISTER_2     localreg2;
	T_RB_LUMINANCE_REGISTER_3     localreg3;

	T_RB_RATIO_REGISTER_0         localreg10;
	T_RB_RATIO_REGISTER_1         localreg11;
	T_RB_RATIO_REGISTER_2         localreg12;
	T_RB_RATIO_REGISTER_3         localreg13;
	T_RB_RATIO_REGISTER_4         localreg14;
	T_RB_RATIO_REGISTER_5         localreg15;
	T_RB_RATIO_REGISTER_6         localreg16;
	T_RB_RATIO_REGISTER_7         localreg17;
#endif

	T_RB_LUMINANCE_REGISTER_4     localreg4;



#if 0
	localreg0.reg = IFE_GETREG(RB_LUMINANCE_REGISTER_0_OFS);
	localreg1.reg = IFE_GETREG(RB_LUMINANCE_REGISTER_1_OFS);
	localreg2.reg = IFE_GETREG(RB_LUMINANCE_REGISTER_2_OFS);
	localreg3.reg = IFE_GETREG(RB_LUMINANCE_REGISTER_3_OFS);

	localreg0.bit.ife_rbluma00    = p_rbfill_set->p_rbfill_rbluma[0];
	localreg0.bit.ife_rbluma01    = p_rbfill_set->p_rbfill_rbluma[1];
	localreg0.bit.ife_rbluma02    = p_rbfill_set->p_rbfill_rbluma[2];
	localreg0.bit.ife_rbluma03    = p_rbfill_set->p_rbfill_rbluma[3];
	localreg1.bit.ife_rbluma04    = p_rbfill_set->p_rbfill_rbluma[4];
	localreg1.bit.ife_rbluma05    = p_rbfill_set->p_rbfill_rbluma[5];
	localreg1.bit.ife_rbluma06    = p_rbfill_set->p_rbfill_rbluma[6];
	localreg1.bit.ife_rbluma07    = p_rbfill_set->p_rbfill_rbluma[7];
	localreg2.bit.ife_rbluma08    = p_rbfill_set->p_rbfill_rbluma[8];
	localreg2.bit.ife_rbluma09    = p_rbfill_set->p_rbfill_rbluma[9];
	localreg2.bit.ife_rbluma10    = p_rbfill_set->p_rbfill_rbluma[10];
	localreg2.bit.ife_rbluma11    = p_rbfill_set->p_rbfill_rbluma[11];
	localreg3.bit.ife_rbluma12    = p_rbfill_set->p_rbfill_rbluma[12];
	localreg3.bit.ife_rbluma13    = p_rbfill_set->p_rbfill_rbluma[13];
	localreg3.bit.ife_rbluma14    = p_rbfill_set->p_rbfill_rbluma[14];
	localreg3.bit.ife_rbluma15    = p_rbfill_set->p_rbfill_rbluma[15];
#endif

	for (i = 0; i < 4; i++) {
		reg_set_val = 0;

		reg_ofs = RB_LUMINANCE_REGISTER_0_OFS + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_rbfill_set->p_rbfill_rbluma[idx] & 0x1F) + ((UINT32)(p_rbfill_set->p_rbfill_rbluma[idx + 1] & 0x1F) << 8) + ((UINT32)(p_rbfill_set->p_rbfill_rbluma[idx + 2] & 0x1F) << 16) + ((UINT32)(p_rbfill_set->p_rbfill_rbluma[idx + 3] & 0x1F) << 24);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	localreg4.reg = IFE_GETREG(RB_LUMINANCE_REGISTER_4_OFS);
	localreg4.bit.ife_rbluma16    = p_rbfill_set->p_rbfill_rbluma[16];
	IFE_SETREG(RB_LUMINANCE_REGISTER_4_OFS, localreg4.reg);

#if 0
	localreg10.reg = IFE_GETREG(RB_RATIO_REGISTER_0_OFS);
	localreg11.reg = IFE_GETREG(RB_RATIO_REGISTER_1_OFS);
	localreg12.reg = IFE_GETREG(RB_RATIO_REGISTER_2_OFS);
	localreg13.reg = IFE_GETREG(RB_RATIO_REGISTER_3_OFS);
	localreg14.reg = IFE_GETREG(RB_RATIO_REGISTER_4_OFS);
	localreg15.reg = IFE_GETREG(RB_RATIO_REGISTER_5_OFS);
	localreg16.reg = IFE_GETREG(RB_RATIO_REGISTER_6_OFS);
	localreg17.reg = IFE_GETREG(RB_RATIO_REGISTER_7_OFS);

	localreg10.bit.ife_rbratio00    = p_rbfill_set->p_rbfill_rbratio[0];
	localreg10.bit.ife_rbratio01    = p_rbfill_set->p_rbfill_rbratio[1];
	localreg10.bit.ife_rbratio02    = p_rbfill_set->p_rbfill_rbratio[2];
	localreg10.bit.ife_rbratio03    = p_rbfill_set->p_rbfill_rbratio[3];
	localreg11.bit.ife_rbratio04    = p_rbfill_set->p_rbfill_rbratio[4];
	localreg11.bit.ife_rbratio05    = p_rbfill_set->p_rbfill_rbratio[5];
	localreg11.bit.ife_rbratio06    = p_rbfill_set->p_rbfill_rbratio[6];
	localreg11.bit.ife_rbratio07    = p_rbfill_set->p_rbfill_rbratio[7];
	localreg12.bit.ife_rbratio08    = p_rbfill_set->p_rbfill_rbratio[8];
	localreg12.bit.ife_rbratio09    = p_rbfill_set->p_rbfill_rbratio[9];
	localreg12.bit.ife_rbratio10    = p_rbfill_set->p_rbfill_rbratio[10];
	localreg12.bit.ife_rbratio11    = p_rbfill_set->p_rbfill_rbratio[11];
	localreg13.bit.ife_rbratio12    = p_rbfill_set->p_rbfill_rbratio[12];
	localreg13.bit.ife_rbratio13    = p_rbfill_set->p_rbfill_rbratio[13];
	localreg13.bit.ife_rbratio14    = p_rbfill_set->p_rbfill_rbratio[14];
	localreg13.bit.ife_rbratio15    = p_rbfill_set->p_rbfill_rbratio[15];
	localreg14.bit.ife_rbratio16    = p_rbfill_set->p_rbfill_rbratio[16];
	localreg14.bit.ife_rbratio17    = p_rbfill_set->p_rbfill_rbratio[17];
	localreg14.bit.ife_rbratio18    = p_rbfill_set->p_rbfill_rbratio[18];
	localreg14.bit.ife_rbratio19    = p_rbfill_set->p_rbfill_rbratio[19];
	localreg15.bit.ife_rbratio20    = p_rbfill_set->p_rbfill_rbratio[20];
	localreg15.bit.ife_rbratio21    = p_rbfill_set->p_rbfill_rbratio[21];
	localreg15.bit.ife_rbratio22    = p_rbfill_set->p_rbfill_rbratio[22];
	localreg15.bit.ife_rbratio23    = p_rbfill_set->p_rbfill_rbratio[23];
	localreg16.bit.ife_rbratio24    = p_rbfill_set->p_rbfill_rbratio[24];
	localreg16.bit.ife_rbratio25    = p_rbfill_set->p_rbfill_rbratio[25];
	localreg16.bit.ife_rbratio26    = p_rbfill_set->p_rbfill_rbratio[26];
	localreg16.bit.ife_rbratio27    = p_rbfill_set->p_rbfill_rbratio[27];
	localreg17.bit.ife_rbratio28    = p_rbfill_set->p_rbfill_rbratio[28];
	localreg17.bit.ife_rbratio29    = p_rbfill_set->p_rbfill_rbratio[29];
	localreg17.bit.ife_rbratio30    = p_rbfill_set->p_rbfill_rbratio[30];
	localreg17.bit.ife_rbratio31    = p_rbfill_set->p_rbfill_rbratio[31];
//    localreg17.bit.rbratio_mode     = p_rbfill_set->rbfill_ratio_mode;
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = RB_RATIO_REGISTER_0_OFS + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_rbfill_set->p_rbfill_rbratio[idx] & 0x1F) + ((UINT32)(p_rbfill_set->p_rbfill_rbratio[idx + 1] & 0x1F) << 8) + ((UINT32)(p_rbfill_set->p_rbfill_rbratio[idx + 2] & 0x1F) << 16) + ((UINT32)(p_rbfill_set->p_rbfill_rbratio[idx + 3] & 0x1F) << 24);

		IFE_SETREG(reg_ofs, reg_set_val);
	}


#if 0
	IFE_SETREG(RB_LUMINANCE_REGISTER_0_OFS, localreg0.reg);
	IFE_SETREG(RB_LUMINANCE_REGISTER_1_OFS, localreg1.reg);
	IFE_SETREG(RB_LUMINANCE_REGISTER_2_OFS, localreg2.reg);
	IFE_SETREG(RB_LUMINANCE_REGISTER_3_OFS, localreg3.reg);

	IFE_SETREG(RB_RATIO_REGISTER_0_OFS, localreg10.reg);
	IFE_SETREG(RB_RATIO_REGISTER_1_OFS, localreg11.reg);
	IFE_SETREG(RB_RATIO_REGISTER_2_OFS, localreg12.reg);
	IFE_SETREG(RB_RATIO_REGISTER_3_OFS, localreg13.reg);
	IFE_SETREG(RB_RATIO_REGISTER_4_OFS, localreg14.reg);
	IFE_SETREG(RB_RATIO_REGISTER_5_OFS, localreg15.reg);
	IFE_SETREG(RB_RATIO_REGISTER_6_OFS, localreg16.reg);
	IFE_SETREG(RB_RATIO_REGISTER_7_OFS, localreg17.reg);
#endif

	return E_OK;
}

static ER ife_set_vignette(IFE_VIG_PARAM *p_vig_set)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	T_VIGNETTE_SETTING_REGISTER_0 localreg0;
	T_VIGNETTE_SETTING_REGISTER_1 localreg1;
	T_VIGNETTE_SETTING_REGISTER_2 localreg2;
	T_VIGNETTE_SETTING_REGISTER_3 localreg3;
	T_VIGNETTE_SETTING_REGISTER_4 localreg4;
	T_VIGNETTE_SETTING_REGISTER_5 localreg5;

#if 0
	T_VIGNETTE_REGISTER_0         lutreg0;
	T_VIGNETTE_REGISTER_1         lutreg1;
	T_VIGNETTE_REGISTER_2         lutreg2;
	T_VIGNETTE_REGISTER_3         lutreg3;
	T_VIGNETTE_REGISTER_4         lutreg4;
	T_VIGNETTE_REGISTER_5         lutreg5;
	T_VIGNETTE_REGISTER_6         lutreg6;
	T_VIGNETTE_REGISTER_7         lutreg7;

	T_VIGNETTE_REGISTER_9         lutreg9;
	T_VIGNETTE_REGISTER_10        lutreg10;
	T_VIGNETTE_REGISTER_11        lutreg11;
	T_VIGNETTE_REGISTER_12        lutreg12;
	T_VIGNETTE_REGISTER_13        lutreg13;
	T_VIGNETTE_REGISTER_14        lutreg14;
	T_VIGNETTE_REGISTER_15        lutreg15;
	T_VIGNETTE_REGISTER_16        lutreg16;

	T_VIGNETTE_REGISTER_18        lutreg18;
	T_VIGNETTE_REGISTER_19        lutreg19;
	T_VIGNETTE_REGISTER_20        lutreg20;
	T_VIGNETTE_REGISTER_21        lutreg21;
	T_VIGNETTE_REGISTER_22        lutreg22;
	T_VIGNETTE_REGISTER_23        lutreg23;
	T_VIGNETTE_REGISTER_24        lutreg24;
	T_VIGNETTE_REGISTER_25        lutreg25;

	T_VIGNETTE_REGISTER_27        lutreg27;
	T_VIGNETTE_REGISTER_28        lutreg28;
	T_VIGNETTE_REGISTER_29        lutreg29;
	T_VIGNETTE_REGISTER_30        lutreg30;
	T_VIGNETTE_REGISTER_31        lutreg31;
	T_VIGNETTE_REGISTER_32        lutreg32;
	T_VIGNETTE_REGISTER_33        lutreg33;
	T_VIGNETTE_REGISTER_34        lutreg34;
#endif

	T_VIGNETTE_REGISTER_8         lutreg8;
	T_VIGNETTE_REGISTER_17        lutreg17;
	T_VIGNETTE_REGISTER_26        lutreg26;
	T_VIGNETTE_REGISTER_35        lutreg35;

	localreg0.reg = IFE_GETREG(VIGNETTE_SETTING_REGISTER_0_OFS);
	localreg1.reg = IFE_GETREG(VIGNETTE_SETTING_REGISTER_1_OFS);
	localreg2.reg = IFE_GETREG(VIGNETTE_SETTING_REGISTER_2_OFS);
	localreg3.reg = IFE_GETREG(VIGNETTE_SETTING_REGISTER_3_OFS);
	localreg4.reg = IFE_GETREG(VIGNETTE_SETTING_REGISTER_4_OFS);
	localreg5.reg = IFE_GETREG(VIGNETTE_SETTING_REGISTER_5_OFS);

	localreg0.bit.ife_distvgtx_c0 = p_vig_set->p_vig_x[0];
	localreg0.bit.ife_distvgty_c0 = p_vig_set->p_vig_y[0];
	localreg0.bit.ife_distgain    = p_vig_set->vig_tab_gain;
	localreg1.bit.ife_distvgtx_c1 = p_vig_set->p_vig_x[1];
	localreg1.bit.ife_distvgty_c1 = p_vig_set->p_vig_y[1];
	localreg2.bit.ife_distvgtx_c2 = p_vig_set->p_vig_x[2];
	localreg2.bit.ife_distvgty_c2 = p_vig_set->p_vig_y[2];
	localreg3.bit.ife_distvgtx_c3 = p_vig_set->p_vig_x[3];
	localreg3.bit.ife_distvgty_c3 = p_vig_set->p_vig_y[3];

	localreg4.bit.ife_distvgxdiv  = p_vig_set->vig_xdiv;
	localreg4.bit.ife_distvgydiv  = p_vig_set->vig_ydiv;
	localreg5.bit.distthr     = p_vig_set->vig_dist_th;
	localreg5.bit.distdthr_en  = p_vig_set->b_vig_dither_en;
	localreg5.bit.distdthr_rst = p_vig_set->b_vig_dither_rst;

	localreg5.bit.ife_vig_fisheye_gain_en = p_vig_set->vig_fisheye_gain_en;
	localreg5.bit.ife_vig_fisheye_slope   = p_vig_set->vig_fisheye_slope;
	localreg5.bit.ife_vig_fisheye_radius  = p_vig_set->vig_fisheye_radius;

	IFE_SETREG(VIGNETTE_SETTING_REGISTER_0_OFS, localreg0.reg);
	IFE_SETREG(VIGNETTE_SETTING_REGISTER_1_OFS, localreg1.reg);
	IFE_SETREG(VIGNETTE_SETTING_REGISTER_2_OFS, localreg2.reg);
	IFE_SETREG(VIGNETTE_SETTING_REGISTER_3_OFS, localreg3.reg);
	IFE_SETREG(VIGNETTE_SETTING_REGISTER_4_OFS, localreg4.reg);
	IFE_SETREG(VIGNETTE_SETTING_REGISTER_5_OFS, localreg5.reg);


#if 0
	lutreg0.reg = IFE_GETREG(VIGNETTE_REGISTER_0_OFS);
	lutreg1.reg = IFE_GETREG(VIGNETTE_REGISTER_1_OFS);
	lutreg2.reg = IFE_GETREG(VIGNETTE_REGISTER_2_OFS);
	lutreg3.reg = IFE_GETREG(VIGNETTE_REGISTER_3_OFS);
	lutreg4.reg = IFE_GETREG(VIGNETTE_REGISTER_4_OFS);
	lutreg5.reg = IFE_GETREG(VIGNETTE_REGISTER_5_OFS);
	lutreg6.reg = IFE_GETREG(VIGNETTE_REGISTER_6_OFS);
	lutreg7.reg = IFE_GETREG(VIGNETTE_REGISTER_7_OFS);

	lutreg0.bit.ife_vig_c0_lut_0 = p_vig_set->p_vig_lut_c0[0];
	lutreg0.bit.ife_vig_c0_lut_1 = p_vig_set->p_vig_lut_c0[1];

	lutreg1.bit.ife_vig_c0_lut_2 = p_vig_set->p_vig_lut_c0[2];
	lutreg1.bit.ife_vig_c0_lut_3 = p_vig_set->p_vig_lut_c0[3];

	lutreg2.bit.ife_vig_c0_lut_4 = p_vig_set->p_vig_lut_c0[4];
	lutreg2.bit.ife_vig_c0_lut_5 = p_vig_set->p_vig_lut_c0[5];

	lutreg3.bit.ife_vig_c0_lut_6 = p_vig_set->p_vig_lut_c0[6];
	lutreg3.bit.ife_vig_c0_lut_7 = p_vig_set->p_vig_lut_c0[7];

	lutreg4.bit.ife_vig_c0_lut_8 = p_vig_set->p_vig_lut_c0[8];
	lutreg4.bit.ife_vig_c0_lut_9 = p_vig_set->p_vig_lut_c0[9];

	lutreg5.bit.ife_vig_c0_lut_10 = p_vig_set->p_vig_lut_c0[10];
	lutreg5.bit.ife_vig_c0_lut_11 = p_vig_set->p_vig_lut_c0[11];

	lutreg6.bit.ife_vig_c0_lut_12 = p_vig_set->p_vig_lut_c0[12];
	lutreg6.bit.ife_vig_c0_lut_13 = p_vig_set->p_vig_lut_c0[13];

	lutreg7.bit.ife_vig_c0_lut_14 = p_vig_set->p_vig_lut_c0[14];
	lutreg7.bit.ife_vig_c0_lut_15 = p_vig_set->p_vig_lut_c0[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = VIGNETTE_REGISTER_0_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_vig_set->p_vig_lut_c0[idx]) + ((UINT32)(p_vig_set->p_vig_lut_c0[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	lutreg8.reg = IFE_GETREG(VIGNETTE_REGISTER_8_OFS);
	lutreg8.bit.ife_vig_c0_lut_16 = p_vig_set->p_vig_lut_c0[16];
	IFE_SETREG(VIGNETTE_REGISTER_8_OFS,  lutreg8.reg);

#if 0
	lutreg9.reg = IFE_GETREG(VIGNETTE_REGISTER_9_OFS);
	lutreg10.reg = IFE_GETREG(VIGNETTE_REGISTER_10_OFS);
	lutreg11.reg = IFE_GETREG(VIGNETTE_REGISTER_11_OFS);
	lutreg12.reg = IFE_GETREG(VIGNETTE_REGISTER_12_OFS);
	lutreg13.reg = IFE_GETREG(VIGNETTE_REGISTER_13_OFS);
	lutreg14.reg = IFE_GETREG(VIGNETTE_REGISTER_14_OFS);
	lutreg15.reg = IFE_GETREG(VIGNETTE_REGISTER_15_OFS);
	lutreg16.reg = IFE_GETREG(VIGNETTE_REGISTER_16_OFS);
	lutreg9.bit.ife_vig_c1_lut_0 = p_vig_set->p_vig_lut_c1[0];
	lutreg9.bit.ife_vig_c1_lut_1 = p_vig_set->p_vig_lut_c1[1];
	lutreg10.bit.ife_vig_c1_lut_2 = p_vig_set->p_vig_lut_c1[2];
	lutreg10.bit.ife_vig_c1_lut_3 = p_vig_set->p_vig_lut_c1[3];
	lutreg11.bit.ife_vig_c1_lut_4 = p_vig_set->p_vig_lut_c1[4];
	lutreg11.bit.ife_vig_c1_lut_5 = p_vig_set->p_vig_lut_c1[5];
	lutreg12.bit.ife_vig_c1_lut_6 = p_vig_set->p_vig_lut_c1[6];
	lutreg12.bit.ife_vig_c1_lut_7 = p_vig_set->p_vig_lut_c1[7];
	lutreg13.bit.ife_vig_c1_lut_8 = p_vig_set->p_vig_lut_c1[8];
	lutreg13.bit.ife_vig_c1_lut_9 = p_vig_set->p_vig_lut_c1[9];
	lutreg14.bit.ife_vig_c1_lut_10 = p_vig_set->p_vig_lut_c1[10];
	lutreg14.bit.ife_vig_c1_lut_11 = p_vig_set->p_vig_lut_c1[11];
	lutreg15.bit.ife_vig_c1_lut_12 = p_vig_set->p_vig_lut_c1[12];
	lutreg15.bit.ife_vig_c1_lut_13 = p_vig_set->p_vig_lut_c1[13];
	lutreg16.bit.ife_vig_c1_lut_14 = p_vig_set->p_vig_lut_c1[14];
	lutreg16.bit.ife_vig_c1_lut_15 = p_vig_set->p_vig_lut_c1[15];
#endif
	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = VIGNETTE_REGISTER_9_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_vig_set->p_vig_lut_c1[idx]) + ((UINT32)(p_vig_set->p_vig_lut_c1[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

	lutreg17.reg = IFE_GETREG(VIGNETTE_REGISTER_17_OFS);
	lutreg17.bit.ife_vig_c1_lut_16 = p_vig_set->p_vig_lut_c1[16];
	IFE_SETREG(VIGNETTE_REGISTER_17_OFS, lutreg17.reg);

#if 0
	lutreg18.reg = IFE_GETREG(VIGNETTE_REGISTER_18_OFS);
	lutreg19.reg = IFE_GETREG(VIGNETTE_REGISTER_19_OFS);
	lutreg20.reg = IFE_GETREG(VIGNETTE_REGISTER_20_OFS);
	lutreg21.reg = IFE_GETREG(VIGNETTE_REGISTER_21_OFS);
	lutreg22.reg = IFE_GETREG(VIGNETTE_REGISTER_22_OFS);
	lutreg23.reg = IFE_GETREG(VIGNETTE_REGISTER_23_OFS);
	lutreg24.reg = IFE_GETREG(VIGNETTE_REGISTER_24_OFS);
	lutreg25.reg = IFE_GETREG(VIGNETTE_REGISTER_25_OFS);
	lutreg18.bit.ife_vig_c2_lut_0 = p_vig_set->p_vig_lut_c2[0];
	lutreg18.bit.ife_vig_c2_lut_1 = p_vig_set->p_vig_lut_c2[1];
	lutreg19.bit.ife_vig_c2_lut_2 = p_vig_set->p_vig_lut_c2[2];
	lutreg19.bit.ife_vig_c2_lut_3 = p_vig_set->p_vig_lut_c2[3];
	lutreg20.bit.ife_vig_c2_lut_4 = p_vig_set->p_vig_lut_c2[4];
	lutreg20.bit.ife_vig_c2_lut_5 = p_vig_set->p_vig_lut_c2[5];
	lutreg21.bit.ife_vig_c2_lut_6 = p_vig_set->p_vig_lut_c2[6];
	lutreg21.bit.ife_vig_c2_lut_7 = p_vig_set->p_vig_lut_c2[7];
	lutreg22.bit.ife_vig_c2_lut_8 = p_vig_set->p_vig_lut_c2[8];
	lutreg22.bit.ife_vig_c2_lut_9 = p_vig_set->p_vig_lut_c2[9];
	lutreg23.bit.ife_vig_c2_lut_10 = p_vig_set->p_vig_lut_c2[10];
	lutreg23.bit.ife_vig_c2_lut_11 = p_vig_set->p_vig_lut_c2[11];
	lutreg24.bit.ife_vig_c2_lut_12 = p_vig_set->p_vig_lut_c2[12];
	lutreg24.bit.ife_vig_c2_lut_13 = p_vig_set->p_vig_lut_c2[13];
	lutreg25.bit.ife_vig_c2_lut_14 = p_vig_set->p_vig_lut_c2[14];
	lutreg25.bit.ife_vig_c2_lut_15 = p_vig_set->p_vig_lut_c2[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = VIGNETTE_REGISTER_18_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_vig_set->p_vig_lut_c2[idx]) + ((UINT32)(p_vig_set->p_vig_lut_c2[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}
	lutreg26.reg = IFE_GETREG(VIGNETTE_REGISTER_26_OFS);
	lutreg26.bit.ife_vig_c2_lut_16 = p_vig_set->p_vig_lut_c2[16];
	IFE_SETREG(VIGNETTE_REGISTER_26_OFS, lutreg26.reg);


#if 0
	lutreg27.reg = IFE_GETREG(VIGNETTE_REGISTER_27_OFS);
	lutreg28.reg = IFE_GETREG(VIGNETTE_REGISTER_28_OFS);
	lutreg29.reg = IFE_GETREG(VIGNETTE_REGISTER_29_OFS);
	lutreg30.reg = IFE_GETREG(VIGNETTE_REGISTER_30_OFS);
	lutreg31.reg = IFE_GETREG(VIGNETTE_REGISTER_31_OFS);
	lutreg32.reg = IFE_GETREG(VIGNETTE_REGISTER_32_OFS);
	lutreg33.reg = IFE_GETREG(VIGNETTE_REGISTER_33_OFS);
	lutreg34.reg = IFE_GETREG(VIGNETTE_REGISTER_34_OFS);

	lutreg27.bit.ife_vig_c3_lut_0 = p_vig_set->p_vig_lut_c3[0];
	lutreg27.bit.ife_vig_c3_lut_1 = p_vig_set->p_vig_lut_c3[1];
	lutreg28.bit.ife_vig_c3_lut_2 = p_vig_set->p_vig_lut_c3[2];
	lutreg28.bit.ife_vig_c3_lut_3 = p_vig_set->p_vig_lut_c3[3];
	lutreg29.bit.ife_vig_c3_lut_4 = p_vig_set->p_vig_lut_c3[4];
	lutreg29.bit.ife_vig_c3_lut_5 = p_vig_set->p_vig_lut_c3[5];
	lutreg30.bit.ife_vig_c3_lut_6 = p_vig_set->p_vig_lut_c3[6];
	lutreg30.bit.ife_vig_c3_lut_7 = p_vig_set->p_vig_lut_c3[7];
	lutreg31.bit.ife_vig_c3_lut_8 = p_vig_set->p_vig_lut_c3[8];
	lutreg31.bit.ife_vig_c3_lut_9 = p_vig_set->p_vig_lut_c3[9];
	lutreg32.bit.ife_vig_c3_lut_10 = p_vig_set->p_vig_lut_c3[10];
	lutreg32.bit.ife_vig_c3_lut_11 = p_vig_set->p_vig_lut_c3[11];
	lutreg33.bit.ife_vig_c3_lut_12 = p_vig_set->p_vig_lut_c3[12];
	lutreg33.bit.ife_vig_c3_lut_13 = p_vig_set->p_vig_lut_c3[13];
	lutreg34.bit.ife_vig_c3_lut_14 = p_vig_set->p_vig_lut_c3[14];
	lutreg34.bit.ife_vig_c3_lut_15 = p_vig_set->p_vig_lut_c3[15];
#endif

	for (i = 0; i < 8; i++) {
		reg_set_val = 0;

		reg_ofs = VIGNETTE_REGISTER_27_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_vig_set->p_vig_lut_c3[idx]) + ((UINT32)(p_vig_set->p_vig_lut_c3[idx + 1]) << 16);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

	lutreg35.reg = IFE_GETREG(VIGNETTE_REGISTER_35_OFS);
	lutreg35.bit.ife_vig_c3_lut_16 = p_vig_set->p_vig_lut_c3[16];
	IFE_SETREG(VIGNETTE_REGISTER_35_OFS, lutreg35.reg);




#if 0
	IFE_SETREG(VIGNETTE_REGISTER_0_OFS,  lutreg0.reg);
	IFE_SETREG(VIGNETTE_REGISTER_1_OFS,  lutreg1.reg);
	IFE_SETREG(VIGNETTE_REGISTER_2_OFS,  lutreg2.reg);
	IFE_SETREG(VIGNETTE_REGISTER_3_OFS,  lutreg3.reg);
	IFE_SETREG(VIGNETTE_REGISTER_4_OFS,  lutreg4.reg);
	IFE_SETREG(VIGNETTE_REGISTER_5_OFS,  lutreg5.reg);
	IFE_SETREG(VIGNETTE_REGISTER_6_OFS,  lutreg6.reg);
	IFE_SETREG(VIGNETTE_REGISTER_7_OFS,  lutreg7.reg);

	IFE_SETREG(VIGNETTE_REGISTER_9_OFS,  lutreg9.reg);
	IFE_SETREG(VIGNETTE_REGISTER_10_OFS, lutreg10.reg);
	IFE_SETREG(VIGNETTE_REGISTER_11_OFS, lutreg11.reg);
	IFE_SETREG(VIGNETTE_REGISTER_12_OFS, lutreg12.reg);
	IFE_SETREG(VIGNETTE_REGISTER_13_OFS, lutreg13.reg);
	IFE_SETREG(VIGNETTE_REGISTER_14_OFS, lutreg14.reg);
	IFE_SETREG(VIGNETTE_REGISTER_15_OFS, lutreg15.reg);
	IFE_SETREG(VIGNETTE_REGISTER_16_OFS, lutreg16.reg);

	IFE_SETREG(VIGNETTE_REGISTER_18_OFS, lutreg18.reg);
	IFE_SETREG(VIGNETTE_REGISTER_19_OFS, lutreg19.reg);
	IFE_SETREG(VIGNETTE_REGISTER_20_OFS, lutreg20.reg);
	IFE_SETREG(VIGNETTE_REGISTER_21_OFS, lutreg21.reg);
	IFE_SETREG(VIGNETTE_REGISTER_22_OFS, lutreg22.reg);
	IFE_SETREG(VIGNETTE_REGISTER_23_OFS, lutreg23.reg);
	IFE_SETREG(VIGNETTE_REGISTER_24_OFS, lutreg24.reg);
	IFE_SETREG(VIGNETTE_REGISTER_25_OFS, lutreg25.reg);

	IFE_SETREG(VIGNETTE_REGISTER_27_OFS, lutreg27.reg);
	IFE_SETREG(VIGNETTE_REGISTER_28_OFS, lutreg28.reg);
	IFE_SETREG(VIGNETTE_REGISTER_29_OFS, lutreg29.reg);
	IFE_SETREG(VIGNETTE_REGISTER_30_OFS, lutreg30.reg);
	IFE_SETREG(VIGNETTE_REGISTER_31_OFS, lutreg31.reg);
	IFE_SETREG(VIGNETTE_REGISTER_32_OFS, lutreg32.reg);
	IFE_SETREG(VIGNETTE_REGISTER_33_OFS, lutreg33.reg);
	IFE_SETREG(VIGNETTE_REGISTER_34_OFS, lutreg34.reg);
#endif

	return E_OK;
}

static ER ife_set_gbal(IFE_GBAL_PARAM *p_gbal_set)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	T_GBAL_REGISTER_0 localreg0;
	T_GBAL_REGISTER_1 localreg1;
	T_GBAL_REGISTER_2 localreg2;

#if 0
	T_GBAL_REGISTER_3 localreg3;
	T_GBAL_REGISTER_4 localreg4;
	T_GBAL_REGISTER_5 localreg5;
	T_GBAL_REGISTER_6 localreg6;
#endif
	T_GBAL_REGISTER_7 localreg7;

	localreg0.reg = IFE_GETREG(GBAL_REGISTER_0_OFS);
	localreg1.reg = IFE_GETREG(GBAL_REGISTER_1_OFS);
	localreg2.reg = IFE_GETREG(GBAL_REGISTER_2_OFS);


	localreg0.bit.ife_gbal_edge_protect_en = p_gbal_set->b_protect_en;
	localreg0.bit.ife_gbal_diff_thr_str    = p_gbal_set->diff_th_str;
	localreg0.bit.ife_gbal_diff_w_max      = p_gbal_set->diff_w_max;

	localreg1.bit.ife_gbal_edge_thr_1 = p_gbal_set->edge_protect_th1;
	localreg1.bit.ife_gbal_edge_thr_0 = p_gbal_set->edge_protect_th0;

	localreg2.bit.ife_gbal_edge_w_max = p_gbal_set->edge_w_max;
	localreg2.bit.ife_gbal_edge_w_min = p_gbal_set->edge_w_min;

	IFE_SETREG(GBAL_REGISTER_0_OFS, localreg0.reg);
	IFE_SETREG(GBAL_REGISTER_1_OFS, localreg1.reg);
	IFE_SETREG(GBAL_REGISTER_2_OFS, localreg2.reg);

#if 0
	localreg3.reg = IFE_GETREG(GBAL_REGISTER_3_OFS);
	localreg4.reg = IFE_GETREG(GBAL_REGISTER_4_OFS);
	localreg5.reg = IFE_GETREG(GBAL_REGISTER_5_OFS);
	localreg6.reg = IFE_GETREG(GBAL_REGISTER_6_OFS);

	localreg3.bit.ife_gbal_ofs_lut_00 = p_gbal_set->p_gbal_ofs[0];
	localreg3.bit.ife_gbal_ofs_lut_01 = p_gbal_set->p_gbal_ofs[1];
	localreg3.bit.ife_gbal_ofs_lut_02 = p_gbal_set->p_gbal_ofs[2];
	localreg3.bit.ife_gbal_ofs_lut_03 = p_gbal_set->p_gbal_ofs[3];

	localreg4.bit.ife_gbal_ofs_lut_04 = p_gbal_set->p_gbal_ofs[4];
	localreg4.bit.ife_gbal_ofs_lut_05 = p_gbal_set->p_gbal_ofs[5];
	localreg4.bit.ife_gbal_ofs_lut_06 = p_gbal_set->p_gbal_ofs[6];
	localreg4.bit.ife_gbal_ofs_lut_07 = p_gbal_set->p_gbal_ofs[7];

	localreg5.bit.ife_gbal_ofs_lut_08 = p_gbal_set->p_gbal_ofs[8];
	localreg5.bit.ife_gbal_ofs_lut_09 = p_gbal_set->p_gbal_ofs[9];
	localreg5.bit.ife_gbal_ofs_lut_10 = p_gbal_set->p_gbal_ofs[10];
	localreg5.bit.ife_gbal_ofs_lut_11 = p_gbal_set->p_gbal_ofs[11];

	localreg6.bit.ife_gbal_ofs_lut_12 = p_gbal_set->p_gbal_ofs[12];
	localreg6.bit.ife_gbal_ofs_lut_13 = p_gbal_set->p_gbal_ofs[13];
	localreg6.bit.ife_gbal_ofs_lut_14 = p_gbal_set->p_gbal_ofs[14];
	localreg6.bit.ife_gbal_ofs_lut_15 = p_gbal_set->p_gbal_ofs[15];

	IFE_SETREG(GBAL_REGISTER_3_OFS, localreg3.reg);
	IFE_SETREG(GBAL_REGISTER_4_OFS, localreg4.reg);
	IFE_SETREG(GBAL_REGISTER_5_OFS, localreg5.reg);
	IFE_SETREG(GBAL_REGISTER_6_OFS, localreg6.reg);
#endif

	for (i = 0; i < 4; i++) {
		reg_set_val = 0;

		reg_ofs = GBAL_REGISTER_3_OFS + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_gbal_set->p_gbal_ofs[idx]) + ((UINT32)(p_gbal_set->p_gbal_ofs[idx + 1]) << 8) + ((UINT32)(p_gbal_set->p_gbal_ofs[idx + 2]) << 16) + ((UINT32)(p_gbal_set->p_gbal_ofs[idx + 3]) << 24);

		IFE_SETREG(reg_ofs, reg_set_val);
	}

	localreg7.reg = IFE_GETREG(GBAL_REGISTER_7_OFS);
	localreg7.bit.ife_gbal_ofs_lut_16 = p_gbal_set->p_gbal_ofs[16];
	IFE_SETREG(GBAL_REGISTER_7_OFS, localreg7.reg);

	return E_OK;
}
/*
static ER ife_set_nlm_ker(IFE_NLM_KER *p_nlm_ker_set)
{
    T_RANGE_FILTER_REGISTER_18 localreg0;
    T_RANGE_FILTER_REGISTER_19 localreg1;
    T_RANGE_FILTER_REGISTER_20 localreg2;

    localreg0.reg = IFE_GETREG(RANGE_FILTER_REGISTER_18_OFS);
    localreg1.reg = IFE_GETREG(RANGE_FILTER_REGISTER_19_OFS);
    localreg2.reg = IFE_GETREG(RANGE_FILTER_REGISTER_20_OFS);

    localreg0.bit.NLM_KER_EN = p_nlm_ker_set->b_nlm_ker_en;
    localreg0.bit.KER_SLOPE0 = p_nlm_ker_set->ui_ker_slope0;
    localreg0.bit.KER_SLOPE1 = p_nlm_ker_set->ui_ker_slope1;
    localreg0.bit.LOCW_EN    = p_nlm_ker_set->b_locw_en;
    localreg0.bit.BILAT_W_D1 = p_nlm_ker_set->ui_bilat_w_d1;
    localreg0.bit.BILAT_W_D2 = p_nlm_ker_set->ui_bilat_w_d2;

    localreg1.bit.KER_RADIUS0 = p_nlm_ker_set->pui_ker_radius[0];
    localreg1.bit.KER_RADIUS1 = p_nlm_ker_set->pui_ker_radius[1];
    localreg1.bit.KER_RADIUS2 = p_nlm_ker_set->pui_ker_radius[2];
    localreg1.bit.KER_RADIUS3 = p_nlm_ker_set->pui_ker_radius[3];
    localreg2.bit.KER_RADIUS4 = p_nlm_ker_set->pui_ker_radius[4];
    localreg2.bit.KER_RADIUS5 = p_nlm_ker_set->pui_ker_radius[5];
    localreg2.bit.KER_RADIUS6 = p_nlm_ker_set->pui_ker_radius[6];
    localreg2.bit.KER_RADIUS7 = p_nlm_ker_set->pui_ker_radius[7];

    IFE_SETREG(RANGE_FILTER_REGISTER_18_OFS, localreg0.reg);
    IFE_SETREG(RANGE_FILTER_REGISTER_19_OFS, localreg1.reg);
    IFE_SETREG(RANGE_FILTER_REGISTER_20_OFS, localreg2.reg);
    return E_OK;
}

static ER ife_set_nlm_weight_lut(IFE_NLM_LUT *p_nlm_lut_set)
{
    T_RANGE_FILTER_REGISTER_21 localreg0;
    T_RANGE_FILTER_REGISTER_22 localreg1;
    T_RANGE_FILTER_REGISTER_23 localreg2;
    T_RANGE_FILTER_REGISTER_24 localreg3;
    T_RANGE_FILTER_REGISTER_25 localreg4;
    T_RANGE_FILTER_REGISTER_26 localreg5;
    T_RANGE_FILTER_REGISTER_27 localreg6;
    T_RANGE_FILTER_REGISTER_28 localreg7;
    T_RANGE_FILTER_REGISTER_29 localreg8;

    localreg0.reg = IFE_GETREG(RANGE_FILTER_REGISTER_21_OFS);
    localreg1.reg = IFE_GETREG(RANGE_FILTER_REGISTER_22_OFS);
    localreg2.reg = IFE_GETREG(RANGE_FILTER_REGISTER_23_OFS);
    localreg3.reg = IFE_GETREG(RANGE_FILTER_REGISTER_24_OFS);
    localreg4.reg = IFE_GETREG(RANGE_FILTER_REGISTER_25_OFS);
    localreg5.reg = IFE_GETREG(RANGE_FILTER_REGISTER_26_OFS);
    localreg6.reg = IFE_GETREG(RANGE_FILTER_REGISTER_27_OFS);
    localreg7.reg = IFE_GETREG(RANGE_FILTER_REGISTER_28_OFS);
    localreg8.reg = IFE_GETREG(RANGE_FILTER_REGISTER_29_OFS);

    localreg0.bit.BILAT_WA0 = p_nlm_lut_set->pui_bilat_wa[0];
    localreg0.bit.BILAT_WA1 = p_nlm_lut_set->pui_bilat_wa[1];
    localreg0.bit.BILAT_WA2 = p_nlm_lut_set->pui_bilat_wa[2];
    localreg0.bit.BILAT_WA3 = p_nlm_lut_set->pui_bilat_wa[3];
    localreg0.bit.BILAT_WA4 = p_nlm_lut_set->pui_bilat_wa[4];
    localreg0.bit.BILAT_WA5 = p_nlm_lut_set->pui_bilat_wa[5];
    localreg0.bit.BILAT_WA6 = p_nlm_lut_set->pui_bilat_wa[6];
    localreg0.bit.BILAT_WA7 = p_nlm_lut_set->pui_bilat_wa[7];

    localreg1.bit.BILAT_WB0 = p_nlm_lut_set->pui_bilat_wb[0];
    localreg1.bit.BILAT_WB1 = p_nlm_lut_set->pui_bilat_wb[1];
    localreg1.bit.BILAT_WB2 = p_nlm_lut_set->pui_bilat_wb[2];
    localreg1.bit.BILAT_WB3 = p_nlm_lut_set->pui_bilat_wb[3];
    localreg1.bit.MW_TH0    = p_nlm_lut_set->pui_mwth[0];
    localreg1.bit.MW_TH1    = p_nlm_lut_set->pui_mwth[1];

    localreg2.bit.BILAT_WC0 = p_nlm_lut_set->pui_bilat_wc[0];
    localreg2.bit.BILAT_WC1 = p_nlm_lut_set->pui_bilat_wc[1];
    localreg2.bit.BILAT_WC2 = p_nlm_lut_set->pui_bilat_wc[2];
    localreg2.bit.BILAT_WC3 = p_nlm_lut_set->pui_bilat_wc[3];
    localreg2.bit.BILAT_WC4 = p_nlm_lut_set->pui_bilat_wc[4];
    localreg2.bit.BILAT_WC5 = p_nlm_lut_set->pui_bilat_wc[5];
    localreg2.bit.BILAT_WC6 = p_nlm_lut_set->pui_bilat_wc[6];

    localreg3.bit.BILAT_WBL0 = p_nlm_lut_set->pui_bilat_wbl[0];
    localreg3.bit.BILAT_WBL1 = p_nlm_lut_set->pui_bilat_wbl[1];
    localreg3.bit.BILAT_WBL2 = p_nlm_lut_set->pui_bilat_wbl[2];
    localreg3.bit.BILAT_WBL3 = p_nlm_lut_set->pui_bilat_wbl[3];
    localreg4.bit.BILAT_WBL4 = p_nlm_lut_set->pui_bilat_wbl[4];
    localreg4.bit.BILAT_WBL5 = p_nlm_lut_set->pui_bilat_wbl[5];
    localreg4.bit.BILAT_WBL6 = p_nlm_lut_set->pui_bilat_wbl[6];
    localreg4.bit.BILAT_WBL7 = p_nlm_lut_set->pui_bilat_wbl[7];

    localreg5.bit.BILAT_WBM0 = p_nlm_lut_set->pui_bilat_wbm[0];
    localreg5.bit.BILAT_WBM1 = p_nlm_lut_set->pui_bilat_wbm[1];
    localreg5.bit.BILAT_WBM2 = p_nlm_lut_set->pui_bilat_wbm[2];
    localreg5.bit.BILAT_WBM3 = p_nlm_lut_set->pui_bilat_wbm[3];
    localreg6.bit.BILAT_WBM4 = p_nlm_lut_set->pui_bilat_wbm[4];
    localreg6.bit.BILAT_WBM5 = p_nlm_lut_set->pui_bilat_wbm[5];
    localreg6.bit.BILAT_WBM6 = p_nlm_lut_set->pui_bilat_wbm[6];
    localreg6.bit.BILAT_WBM7 = p_nlm_lut_set->pui_bilat_wbm[7];

    localreg7.bit.BILAT_WBH0 = p_nlm_lut_set->pui_bilat_wbh[0];
    localreg7.bit.BILAT_WBH1 = p_nlm_lut_set->pui_bilat_wbh[1];
    localreg7.bit.BILAT_WBH2 = p_nlm_lut_set->pui_bilat_wbh[2];
    localreg7.bit.BILAT_WBH3 = p_nlm_lut_set->pui_bilat_wbh[3];
    localreg8.bit.BILAT_WBH4 = p_nlm_lut_set->pui_bilat_wbh[4];
    localreg8.bit.BILAT_WBH5 = p_nlm_lut_set->pui_bilat_wbh[5];
    localreg8.bit.BILAT_WBH6 = p_nlm_lut_set->pui_bilat_wbh[6];
    localreg8.bit.BILAT_WBH7 = p_nlm_lut_set->pui_bilat_wbh[7];

    IFE_SETREG(RANGE_FILTER_REGISTER_21_OFS, localreg0.reg);
    IFE_SETREG(RANGE_FILTER_REGISTER_22_OFS, localreg1.reg);
    IFE_SETREG(RANGE_FILTER_REGISTER_23_OFS, localreg2.reg);
    IFE_SETREG(RANGE_FILTER_REGISTER_24_OFS, localreg3.reg);
    IFE_SETREG(RANGE_FILTER_REGISTER_25_OFS, localreg4.reg);
    IFE_SETREG(RANGE_FILTER_REGISTER_26_OFS, localreg5.reg);
    IFE_SETREG(RANGE_FILTER_REGISTER_27_OFS, localreg6.reg);
    IFE_SETREG(RANGE_FILTER_REGISTER_28_OFS, localreg7.reg);
    IFE_SETREG(RANGE_FILTER_REGISTER_29_OFS, localreg8.reg);
    return E_OK;
}
*/
UINT32 ife_get_stripe_cnt(void)
{
	//T_DEBUG_REGISTER  localreg;

	//localreg.reg = IFE_GETREG(DEBUG_REGISTER_OFS);
	//return localreg.bit.IFE_STRIPECNT;
	return 0;
}

static BOOL ife_is_busy(void)
{
	T_DEBUG_REGISTER  localreg;

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ife_ctrl_flow_to_do == ENABLE) {
		localreg.reg = IFE_GETREG(DEBUG_REGISTER_OFS);
		return localreg.bit.ife_busy;
	} else {
		return 0;
	}
#else
	localreg.reg = IFE_GETREG(DEBUG_REGISTER_OFS);
	return localreg.bit.ife_busy;
#endif
}

BOOL ife_d2d_is_busy(void)
{
	T_FILTER_OPERATION_CONTROL_REGISTER    localreg;

	localreg.reg = IFE_GETREG(FILTER_OPERATION_CONTROL_REGISTER_OFS);
	return localreg.bit.ife_start;
}

UINT32 ife_get_dram_in_addr(void)
{
	T_SOURCE_ADDRESS_REGISTER_0     localreg1;
	UINT32 ui_addr, ui_phy_addr;

	localreg1.reg = IFE_GETREG(SOURCE_ADDRESS_REGISTER_0_OFS);
	ui_phy_addr = localreg1.bit.dram_sai0 << 2;

#if (IFE_DMA_CACHE_HANDLE == 1)
	ui_addr = dma_getNonCacheAddr(ui_phy_addr);
#else
	ui_addr = ui_phy_addr;
#endif

	return ui_addr;
}
#if (defined(_NVT_EMULATION_) == ON)
UINT32 ife_get_dram_in_addr2(void)
{
	T_SOURCE_ADDRESS_REGISTER_1     localreg1;
	UINT32 ui_addr, ui_phy_addr;

	localreg1.reg = IFE_GETREG(SOURCE_ADDRESS_REGISTER_1_OFS);
	ui_phy_addr = localreg1.bit.dram_sai1 << 2;

#if (IFE_DMA_CACHE_HANDLE == 1)
	ui_addr = dma_getNonCacheAddr(ui_phy_addr);
#else
	ui_addr = ui_phy_addr;
#endif

	return ui_addr;
}
#endif
UINT32 ife_get_dram_in_lofs(void)
{
	T_SOURCE_LINE_OFFSET_REGISTER_0       localreg1;

	localreg1.reg = IFE_GETREG(SOURCE_LINE_OFFSET_REGISTER_0_OFS);
	return (localreg1.bit.dram_ofsi0 << 2);
}

UINT32 ife_get_dram_in_lofs2(void)
{
	T_SOURCE_LINE_OFFSET_REGISTER_0       localreg1;

	localreg1.reg = IFE_GETREG(0x3c);
	return (localreg1.bit.dram_ofsi0 << 2);
}
UINT32 ife_get_dram_out_addr(void)
{
	T_DESTINATION_ADDRESS_REGISTER       localreg1;
	UINT32 addr, phy_addr;

	localreg1.reg = IFE_GETREG(DESTINATION_ADDRESS_REGISTER_OFS);
	phy_addr = localreg1.bit.dram_sao << 2;

#if (IFE_DMA_CACHE_HANDLE == 1)
	addr = dma_getNonCacheAddr(phy_addr);
#else
	addr = phy_addr;
#endif

	return addr;
}

UINT32 ife_get_dram_out_lofs(void)
{
	T_DESTINATION_LINE_OFFSET_REGISTER       localreg1;

	localreg1.reg = IFE_GETREG(DESTINATION_LINE_OFFSET_REGISTER_OFS);
	return (localreg1.bit.dram_ofso << 2);
}

UINT32 ife_get_in_vsize(void)
{
	T_SOURCE_SIZE_REGISTER_0    localreg;

	localreg.reg = IFE_GETREG(SOURCE_SIZE_REGISTER_0_OFS);
	DBG_MSG("localreg.bit.height = %d\r\n", (unsigned int)localreg.bit.height);
	return (localreg.bit.height << 1);
}

UINT32 ife_get_in_hsize(void)
{
	T_SOURCE_SIZE_REGISTER_0    localreg;

	localreg.reg = IFE_GETREG(SOURCE_SIZE_REGISTER_0_OFS);
	return (localreg.bit.width << 2);
}

ER ife_get_burst_length(IFE_BURST_LENGTH *p_burst_len)
{
	T_DRAM_SETTINGS    localreg1;

	localreg1.reg = IFE_GETREG(DRAM_SETTINGS_OFS);
//Need add input burst and output burst
	p_burst_len->burst_len_input = (IFE_IN_BURST_SEL)localreg1.bit.input_burst_mode;
	p_burst_len->burst_len_output = (IFE_OUT_BURST_SEL)localreg1.bit.output_burst_mode;

	IFE_SETREG(DRAM_SETTINGS_OFS, localreg1.reg);

	return E_OK;
}
//void ife_set2Ready(void)
//{
//     uiIFEngine_status = IFE_ENGINE_READY;
//}





#ifdef __KERNEL__
EXPORT_SYMBOL(ife_open);
EXPORT_SYMBOL(ife_start);
EXPORT_SYMBOL(ife_pause);
EXPORT_SYMBOL(ife_set_mode);
EXPORT_SYMBOL(ife_close);
EXPORT_SYMBOL(ife_reset);

EXPORT_SYMBOL(ife_set_builtin_flow_disable);
#endif
