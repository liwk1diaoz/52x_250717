/*
    IPE module driver

    NT96520 IPE driver.

    @file       IPE.c
    @ingroup    mIDrvIPP_IPE
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#include "plat/top.h"
#include "ipe_reg.h"
#include "ipe_lib.h"
#include "ipe_int.h"
#include "ipe_platform.h"

#if (defined(_NVT_EMULATION_) == ON)
#include "comm/timer.h"
#define IPE_POWER_SAVING_EN 0//do not enable power saving in emulation
#else
#define IPE_POWER_SAVING_EN 1
#endif

volatile NT98520_IPE_REGISTER_STRUCT *ipeg = NULL;


#define ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                 ///< Align Floor
#define ALIGN_64BYTE(value)       (ALIGN_FLOOR((value) + ((16)-1), 16))   ///< Align Ceil


#define IPE_DMA_CACHE_HANDLE    (1)
#define IPE_DRAM_GAMMA_BYTE ((ALIGN_64BYTE(195))*4) //(780)
#define IPE_DRAM_3DLUT_BYTE (3600)
#define IPE_DRAM_YCURVE_BYTE ((ALIGN_64BYTE(65))*4) //(260)
#define IPE_DRAM_VA_BYTE (16*16*3*4)
#define IPE_VA_WIN_MIN_H (4)
#define IPE_VA_WIN_MIN_V (1)//(2) //sc
#define IPE_VA_WIN_MAX_H (255)
#define IPE_VA_WIN_MAX_V (255)//(127)
#define IPE_INDVA_WIN_MIN_H (1)
#define IPE_INDVA_WIN_MIN_V (1)
#define IPE_INDVA_WIN_MAX_H (511)
#define IPE_INDVA_WIN_MAX_V (511)//(255)

typedef struct {
	UINT32 enable;
	UINT32 addr;
	UINT32 size;
} IPE_DRAM_ADDR_SIZE;

typedef enum {
	IPE_IN_Y,
	IPE_IN_C,
	IPE_IN_SUBIMG,
	IPE_OUT_Y,
	IPE_OUT_C,
	IPE_OUT_SUBIMG,
	IPE_OUT_VA,
	IPE_IO_MAX
} IPE_DMA_IO;

static IPE_DRAM_ADDR_SIZE g_ipe_dramio[IPE_IO_MAX] = {0};

//UINT16 g_uiIpeVaC=0;
//UINT32 g_ipe_clock;

static UINT32 g_ipe_status  = IPE_STATUS_IDLE;
static UINT32 g_ipe_lock_status  = NO_TASK_LOCKED;

static IPE_REGTYPE g_reg_type = IPE_ALL_REG;
static IPE_DMAOSEL g_cur_dma_outsel = 256;
static IPE_STRPINFO g_strp_info = {IPE_OPMODE_D2D, {1920, 1080}, IPE_YCC444, IPE_YCC444, ENABLE, IPE_ORIGINAL, {ETH_OUT_ORIGINAL, ETH_OUT_2BITS} };
static IPE_OPMODE ipe_opmode = IPE_OPMODE_IPP;
static IPE_LOAD_TYPE g_ipe_load_type = IPE_FRMEND_LOAD;
static BOOL g_ipe_clk_en = FALSE;

static IPE_VA_SETTING va_ring_setting[2] = {0};

#if (defined(_NVT_EMULATION_) == ON)
static TIMER_ID ipe_timer_id;
BOOL ipe_end_time_out_status = FALSE;
#endif

#if 0//(DRV_SUPPORT_IST == ENABLE)
#else
static void (*g_pfIpeIntHdlCB)(UINT32 intstatus);
#endif

UINT32 g_ipe_frm_start_cnt = 0;
UINT32 g_ipe_frm_end_cnt = 0;
UINT32 g_ipe_stp_end_cnt = 0;
UINT32 g_ipe_yout_cnt = 0;
UINT32 g_ipe_gamma_load_end_cnt = 0;
UINT32 g_ipe_defog_end_cnt = 0;
UINT32 g_ipe_va_out_cnt = 0;
UINT32 g_ipe_ll_done_cnt = 0;
UINT32 g_ipe_ll_job_end_cnt = 0;
UINT32 g_ipe_ll_error_cnt = 0;
UINT32 g_ipe_ll_error2_cnt = 0;


// stripe size limitation
static UINT32 default_hn = 0, ipe_maxhstrp = 0, ipe_minhstrp = 0;


//ISR
extern void ipe_isr(void);
//Cache flush
#if (IPE_DMA_CACHE_HANDLE)
static void ipe_flush_cache_end(void);
#endif
//Control
static void ipe_reset_on(void);
static void ipe_reset_off(void);
static void ipe_set_ll_fire(void);
static void ipe_set_ll_terminate(void);
static void ipe_set_start(void);
static void ipe_set_stop(void);
static void ipe_set_load(IPE_LOAD_TYPE type);
static void ipe_set_gamma_rw_en(IPE_RW_SRAM_ENUM rw_sel, IPE_RWGAM_OPT opt);
//Function ON/OFF
static void ipe_enable_feature(UINT32 features);
static void ipe_disable_feature(UINT32 features);
static void ipe_set_feature(UINT32 features);
//Size
void ipe_set_h_stripe(STR_STRIPE_INFOR hstripe_infor);
void ipe_set_v_stripe(STR_STRIPE_INFOR vstripe_infor);
static ER ipe_cal_stripe(IPE_STRPINFO *p_strp_info);
//I/O
static void ipe_set_output_switch(BOOL to_ime_en, BOOL to_dram_en);
static void ipe_set_dma_outsel(IPE_DMAOSEL sel);
//static void ipe_enableDirectIFE2(IPE_ENABLE en);
static void ipe_set_in_format(IPE_INOUT_FORMAT imat);
static void ipe_set_out_format(IPE_INOUT_FORMAT omat, IPE_HSUB_SEL_ENUM sub_h_sel);
static void ipe_set_hovlp_opt(IPE_HOVLP_SEL_ENUM opt);
//mode
static void ipe_set_op_mode(IPE_OPMODE mode);
//DRAM I/O
static void ipe_set_dram_out_mode(IPE_DRAM_OUTPUT_MODE mode);
static void ipe_set_dram_single_channel_en(IPE_DRAM_SINGLE_OUT_EN ch);
static void ipe_set_dma_in_rand(IPE_ENABLE en, IPE_ENABLE reset);
void ipe_set_linked_list_addr(UINT32 sai);
void ipe_set_dma_in_addr(UINT32 addr_in_y, UINT32 addr_in_c);
void ipe_set_dma_in_defog_addr(UINT32 sai);
void ipe_set_dma_in_gamma_lut_addr(UINT32 sai);
void ipe_set_dma_in_ycurve_lut_addr(UINT32 sai);
static void ipe_set_dma_in_offset(UINT32 ofsi0, UINT32 ofsi1);
static void ipe_set_dma_in_defog_offset(UINT32 ofsi);
void ipe_set_dma_out_addr_y(UINT32 sao_y);
void ipe_set_dma_out_addr_c(UINT32 sao_c);
static void ipe_set_dma_out_offset_y(UINT32 ofso_y);
static void ipe_set_dma_out_offset_c(UINT32 ofso_c);
//Interrupt
static void ipe_enable_int(UINT32 intr);
//Eext
static void ipe_set_tone_remap(UINT32 *p_tonemap_lut);
static void ipe_set_edge_extract_sel(IPE_EEXT_GAM_ENUM gam_sel, IPE_EEXT_CH_ENUM ch_sel);
static void ipe_set_edge_extract_b(INT16 *p_eext_ker, UINT32 eext_div_b, UINT8 eext_enh_b);
static void ipe_set_eext_kerstrength(IPE_EEXT_KER_STRENGTH *ker_str);
static void ipe_set_eext_engcon(IPE_EEXT_ENG_CON *p_eext_engcon);
static void ipe_set_ker_thickness(IPE_KER_THICKNESS *p_ker_thickness);
static void ipe_set_ker_thickness_hld(IPE_KER_THICKNESS *p_ker_thickness_hld);
static void ipe_set_region(IPE_REGION_PARAM *p_region);
static void ipe_set_region_slope(UINT16 slope, UINT16 slope_hld);
static void ipe_set_overshoot(IPE_OVERSHOOT_PARAM *p_overshoot);
static void ipe_set_esmap_th(IPE_ESMAP_INFOR *p_esmap);
static void ipe_set_estab(UINT8 *p_estab);
static void ipe_set_edgemap_th(IPE_EDGEMAP_INFOR *p_edmap);
static void ipe_set_edgemap_tab(UINT8 *p_edtab);
//RGBLPF
static void ipe_set_rgblpf(IPE_RGBLPF_PARAM *p_lpf_info);
//PFR
static void ipe_set_pfr_general(IPE_PFR_PARAM *p_pfr_info);
static void ipe_set_pfr_color(UINT32 idx, IPE_PFR_COLOR_WET *p_pfr_color_info);
static void ipe_set_pfr_luma(UINT32 luma_th, UINT32 *luma_table);
//CC
static void ipe_set_color_correct(IPE_CC_PARAM *p_cc_param);
static void ipe_set_fstab(UINT8 *p_fstab_lut);
static void ipe_set_fdtab(UINT8 *p_fdtab_lut);
//CST
static void ipe_set_cst(IPE_CST_PARAM *p_cst);
//CCTRL
static void ipe_set_int_sat_offset(INT16 int_ofs, INT16 sat_ofs);
static void ipe_set_hue_rotate(BOOL hue_rot_en);
static void ipe_set_hue_c2g_en(BOOL c2g_en);
static void ipe_set_color_sup(IPE_CCTRL_SEL_ENUM cctrl_sel, UINT8 vdetdiv);
static void ipe_set_cctrl_hue(UINT8 *p_hue_tbl);
static void ipe_set_cctrl_int(INT32 *p_int_tbl);
static void ipe_set_cctrl_sat(INT32 *p_sat_tbl);
static void ipe_set_cctrl_edge(UINT8 *p_edge_tbl);
static void ipe_set_cctrl_dds(UINT8 *p_dds_tbl);
//CADJ
static void ipe_set_edge_enhance(UINT32 y_enh_p, UINT32 y_enh_n);
static void ipe_set_edge_inv(BOOL b_einv_p, BOOL b_einv_n);
static void ipe_set_ycon(UINT32 con_y);
static void ipe_set_yc_rand(IPE_CADJ_RAND_PARAM *p_ycrand);
static void ipe_set_yfix_th(STR_YTH1_INFOR *p_yth1, STR_YTH2_INFOR *p_yth2);
static void ipe_set_yc_mask(UINT8 mask_y, UINT8 mask_cb, UINT8 mask_cr);
static void ipe_set_cbcr_offset(UINT32 cb_ofs, UINT32 cr_ofs);
static void ipe_set_cbcr_con(UINT32 con_c);
static void ipe_set_cbcr_contab(UINT32 *p_ccontab);
static void ipe_set_cbcr_contab_sel(IPE_CCONTAB_SEL ccontab_sel);


void ipe_set_edge_region_strength_enable(BOOL set_en);
void ipe_set_edge_region_kernel_strength(UINT8 set_thin_str, UINT8 set_robust_str);
void ipe_et_edge_region_flat_strength(INT16 set_slop, UINT8 set_str);
void ipe_et_edge_region_edge_strength(INT16 set_slop, UINT8 set_str);




static void ipe_set_cbcr_fixth(STR_CTH_INFOR *p_cth);
//CSTP
static void ipe_set_cstp(UINT32 ratio);
//Gamma/Ycurve
static void ipe_set_gam_y_rand(IPE_GAMYRAND *p_gamy_rand);
static void ipe_set_ycurve_sel(IPE_YCURVE_SEL ycurve_sel);
//defog
static void ipe_set_defog_rand(IPE_DEFOGROUND *p_defog_rnd);
//edge debug
static void ipe_set_edge_dbg_sel(IPE_EDGEDBG_SEL mode_sel);
//Eth
static void ipe_set_eth_size(IPE_ETH_SIZE *p_eth);
static void ipe_set_eth_param(IPE_ETH_PARAM *p_eth);
//VA
void ipe_set_dma_out_va_addr(UINT32 sao);
static void ipe_set_dma_out_va_offset(UINT32 ofso);
static void ipe_set_va_out_sel(BOOL en);
static void ipe_set_va_filter_g1(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g1);
static void ipe_set_va_filter_g2(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g2);
static void ipe_set_va_mode_enable(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g1, IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g2);
static void ipe_set_va_win_info(IPE_VA_WIN_PARAM *p_stcs_win_info);
void ipe_set_va_indep_win(IPE_INDEP_VA_PARAM  *p_indep_va_win_info, UINT32 win_idx);
//defog
void ipe_set_dma_out_defog_addr(UINT32 sao);
static void ipe_set_dma_out_defog_offset(UINT32 ofso);
static void ipe_set_dfg_subimg_size(IPE_IMG_SIZE subimg_size);
static void ipe_set_dfg_subout_param(UINT32 blk_sizeh, UINT32 blk_cent_h_fact, UINT32 blk_sizev, UINT32 blk_cent_v_fact);
static void ipe_set_dfg_input_bld(UINT8 *input_bld_wt);
static void ipe_set_dfg_scal_factor(UINT32 scalfact_h, UINT32 scalfact_v);
static void ipe_set_dfg_scal_filtcoeff(UINT8 *p_filtcoef);
static void ipe_set_dfg_scalg_edgeinterplut(UINT8 *p_interp_diff_lut);
static void ipe_set_defog_scal_edgeinterp(UINT8 wdist, UINT8 wout, UINT8 wcenter, UINT8 wsrc);
static void ipe_set_defog_atmospherelight(UINT16 val0, UINT16 val1, UINT16 val2);
static void ipe_set_defog_fog_protect(BOOL self_cmp_en, UINT16 min_diff);
static void ipe_set_defog_fog_mod(UINT16 *p_fog_mod_lut);
static void ipe_set_defog_fog_target_lut(UINT16 *p_fog_target_lut);
static void ipe_set_defog_strength(IPE_DEFOG_METHOD_SEL modesel, UINT8 fog_ratio, UINT8 dgain, UINT8 gain_th);
static void ipe_set_defog_outbld_lum_lut(BOOL lum_bld_ref, UINT8 *p_outbld_lum_lut);
static void ipe_set_defog_outbld_diff_lut(UINT8 *p_outbld_diff_lut);
static void ipe_set_defog_outbld_local(UINT32 local_en, UINT32 max_nfactor);
static void ipe_set_defog_stcs_pcnt(UINT32 pixelcnt);
//lce
static void ipe_set_lce_diff_param(UINT8 wt_avg, UINT8 wt_pos, UINT8 wt_neg);
static void ipe_set_lce_lum_lut(UINT8 *p_lce_lum_lut);

//Attach & detach
static ER ipe_attach(VOID);
static ER ipe_detach(VOID);


//For FPGA verify only
void ipe_set_rw_en(IPE_RW_SRAM_ENUM rw_sel);
ER ipe_start_without_load(void);
ER ipe_start_with_frmend_load(void);
ER ipe_change_interrupt_without_load(UINT32 int_en);
void ipe_set_start_frmend_load(void);


UINT16 ipe_int_to_2comp(INT16 val, UINT16 bits)
{
    UINT16 tmp = 0;
    UINT16 mask = (1 << bits) - 1;

    if(val >= 0)
    {
        tmp = (((UINT16)val) & mask);
    }
    else
    {
        //cout << ((unsigned int)(~(-1*val)) & mask) << endl;
        tmp = ((UINT16)(~(-1*val)) & mask) + 1;
    }

    return tmp;
}
//----------------------------------------------------------------------------------


INT16 ipe_2comp_to_int(UINT16 val, UINT16 bits)
{
    INT16 pmax = (1 << (bits - 1));
    INT16 tmp = 0, out = 0;

    INT16 mask = (1 << bits) - 1;

    if(val < pmax)
    {
        out = val;
    }
    else
    {
        tmp = ((val - 1) ^ mask);
        out = -1 * tmp;
    }

    return out;
}


/**
    IPE state machine

    Status and operation check mechanism for IPE.

    @param CurrentStatus Current IPE status.
    @param Operation Operation to be executed.

    @return ER Error code of state machine check result.
*/
static ER ipe_state_machine(IPE_OPERATION op, IPESTATUSUPDATE update)
{
	switch (op) {
	case IPE_OP_OPEN:
		switch (g_ipe_status) {
		case IPE_STATUS_IDLE:
			if (update) {
				g_ipe_status = IPE_STATUS_READY;
			}
			return E_OK;
		//break;
		default :
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	case IPE_OP_CLOSE:
		switch (g_ipe_status) {
		case IPE_STATUS_READY:
		case IPE_STATUS_PAUSE:
			if (update) {
				g_ipe_status = IPE_STATUS_IDLE;
			}
			return E_OK;
		//break;
		default :
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	case IPE_OP_SETMODE:
		switch (g_ipe_status) {
		case IPE_STATUS_READY:
		case IPE_STATUS_PAUSE:
			if (update) {
				g_ipe_status = IPE_STATUS_PAUSE;
			}
			return E_OK;
		//break;
		default :
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	case IPE_OP_START:
		switch (g_ipe_status) {
		case IPE_STATUS_PAUSE:
			if (update) {
				g_ipe_status = IPE_STATUS_RUN;
			}
			return E_OK;
		//break;
		default :
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	case IPE_OP_PAUSE:
		switch (g_ipe_status) {
		case IPE_STATUS_RUN:
		case IPE_STATUS_READY:
			if (update) {
				g_ipe_status = IPE_STATUS_PAUSE;
			}
			return E_OK;
		//break;
		case IPE_STATUS_PAUSE:
			//DBG_ERR("IPE: multiple pause operations!\r\n");
			return E_OK;
		//break;
		default :
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	case IPE_OP_HWRESET:
		switch (g_ipe_status) {
		case IPE_STATUS_IDLE:
		case IPE_STATUS_READY:
		case IPE_STATUS_PAUSE:
		case IPE_STATUS_RUN:
			if (update) {
				g_ipe_status = IPE_STATUS_READY;
			}
			return E_OK;
		//break;
		default :
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	case IPE_OP_CHGSIZE:
		switch (g_ipe_status) {
		case IPE_STATUS_RUN:
		case IPE_STATUS_PAUSE:
		case IPE_STATUS_READY:
			//if(update) {
			//    g_ipe_status = IPE_STATUS_PAUSE;
			//}
			return E_OK;
		//break;
		default :
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	case IPE_OP_CHGPARAM:
	case IPE_OP_CHGPARAMALL:
		switch (g_ipe_status) {
		case IPE_STATUS_RUN:
		case IPE_STATUS_PAUSE:
		case IPE_STATUS_READY:
			//if(update) {
			//    g_ipe_status = IPE_STATUS_PAUSE;
			//}
			return E_OK;
		//break;
		default :
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	case IPE_OP_CHGINOUT:
		switch (g_ipe_status) {
		case IPE_STATUS_RUN:
		case IPE_STATUS_PAUSE:
		case IPE_STATUS_READY:
			//if(update) {
			//    g_ipe_status = IPE_STATUS_PAUSE;
			//}
			return E_OK;
		//break;
		default :
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	case IPE_OP_LOADLUT:
		switch (g_ipe_status) {
		case IPE_STATUS_RUN:
		case IPE_STATUS_PAUSE:
		case IPE_STATUS_READY:
			//if(update) {
			//    g_ipe_status = IPE_STATUS_PAUSE;
			//}
			return E_OK;
		default:
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	case IPE_OP_READLUT:
		switch (g_ipe_status) {
		case IPE_STATUS_READY:
		case IPE_STATUS_PAUSE:
			//if(update) {
			//    g_ipe_status = IPE_STATUS_PAUSE;
			//}
			return E_OK;
		//break;
		default :
			DBG_ERR("ERROR!!Operate %d from State %d\r\n", (int)op, (int)g_ipe_status);
			return E_SYS;
			//break;
		}
	//break;
	default :
		DBG_ERR("IPE Operation ERROR!!\r\n");
		return E_SYS;
		//break;
	}
	return E_OK;
}

/**
    IPE lock

    Get semiphore for IPE operation start.

    @param None.

    @return ER Error code of wait semiphore result.
*/
static ER ipe_lock(void)
{
	ER er_return = E_OK;

	er_return = ipe_platform_sem_wait();
	if (er_return != E_OK) {
		return er_return;
	}

	g_ipe_lock_status = TASK_LOCKED;

	return er_return;
}

/**
    IPE unlock

    Release semiphore for IPE operation end.

    @param None.

    @return ER Error code of release semiphore result.
*/
static ER ipe_unlock(void)
{
	ER er_return = E_OK;

	g_ipe_lock_status = NO_TASK_LOCKED;

	er_return = ipe_platform_sem_signal();
	if (er_return != E_OK) {
		return E_SYS;
	}
	return er_return;
}

/*
    IPE interrupt handler

    IPE interrupt service routine.

    @param None.

    @return None.
*/
void ipe_isr(void)
{
	unsigned int ipe_int_status;
	FLGPTN  flag;

	ipe_int_status = ipe_get_int_status();

	if (ipe_platform_get_init_status() == FALSE) {
		ipe_clear_int(ipe_int_status);
		return;
	}

	ipe_int_status = ipe_int_status & ipe_get_int_enable();
	ipe_clear_int(ipe_int_status);

	if (ipe_int_status == 0) {
		return;
	}


	flag = 0;

	//nvt_dbg(INFO,"IpeIntStatus = 0x%08x \r\n",IpeIntStatus);

	if (ipe_int_status & IPE_INT_FRMSTART) {
		flag |= FLGPTN_IPE_FRMSTART;
		g_ipe_frm_start_cnt++;
	}

	if (ipe_int_status & IPE_INT_FRMEND) {
		flag |= FLGPTN_IPE_FRAMEEND;
		g_ipe_frm_end_cnt++;
		//DBG_ERR("IPE_INT_FRM \r\n");

		va_ring_setting[1].va_en = va_ring_setting[0].va_en;
		va_ring_setting[1].outsel = va_ring_setting[0].outsel;
		va_ring_setting[1].win_num_x = va_ring_setting[0].win_num_x;
		va_ring_setting[1].win_num_y = va_ring_setting[0].win_num_y;
		va_ring_setting[1].address = va_ring_setting[0].address;
		va_ring_setting[1].lineoffset = va_ring_setting[0].lineoffset;
	}

	if (ipe_int_status & IPE_INT_ST) {
		flag |= FLGPTN_IPE_STRPEND;
		g_ipe_stp_end_cnt++;
	}

	if (ipe_int_status & IPE_INT_LL_DONE) {
		flag |= FLGPTN_IPE_LL_DONE;
		g_ipe_ll_done_cnt++;
	}

	if (ipe_int_status & IPE_INT_LL_JOB_END) {
		flag |= FLGPTN_IPE_LL_JOB_END;
		g_ipe_ll_job_end_cnt++;
	}

	if (ipe_int_status & IPE_INT_YCOUTEND) {
		flag |= FLGPTN_IPE_YCOUTEND;
		g_ipe_yout_cnt++;
	}

	if (ipe_int_status & IPE_INT_DMAIN0END) {
		flag |= FLGPTN_IPE_DMAIN0END;
		g_ipe_gamma_load_end_cnt++;
	}

	if (ipe_int_status & IPE_INT_DFG_SUBOUT_END) {
		flag |= FLGPTN_IPE_DMAIN1END;
		g_ipe_defog_end_cnt++;
	}

	if (ipe_int_status & IPE_INT_VAOUTEND) {
		flag |= FLGPTN_IPE_VAOUTEND;
		g_ipe_va_out_cnt++;
	}

	if (ipe_int_status & IPE_INT_LL_ERROR) {
		g_ipe_ll_error_cnt++;
	}

	if (ipe_int_status & IPE_INT_LL_ERROR2) {
		g_ipe_ll_error2_cnt++;
	}
	/*
	if(ipe_int_status & IPE_INT_GAMERR)
	{
	    flag |= FLGPTN_IPE_GAMERR;
	}
	if(ipe_int_status & IPE_INT_3DCCERR)
	{
	    flag |= FLGPTN_IPE_3DCCERR;
	}
	*/

#if 0//(DRV_SUPPORT_IST == ENABLE)
	if (pfIstCB_IPE != NULL) {
		ipe_int_status &= (~(IPE_INT_DMAIN0END));
		if (ipe_int_status) {
			drv_setIstEvent(DRV_IST_MODULE_IPE, ipe_int_status);
		}
	}
#else
	if (g_pfIpeIntHdlCB != NULL) {
		ipe_int_status &= (~(IPE_INT_DMAIN0END));
		if (ipe_int_status) {
			g_pfIpeIntHdlCB(ipe_int_status);
		}
	}
#endif

	if (flag != 0) {
		ipe_platform_flg_set(flag);
	}
}

/**
    IPE clear frame end

    Clear IPE frame end flag.

    @param None.

    @return None.
*/
void ipe_clr_frame_end(void)
{
	ipe_platform_flg_clear(FLGPTN_IPE_FRAMEEND);
}

#if (defined(_NVT_EMULATION_) == ON)
static void ipe_time_out_cb(UINT32 event)
{
	DBG_ERR("IPE time out!\r\n");
	ipe_end_time_out_status = TRUE;
	ipe_platform_flg_set(FLGPTN_IPE_FRAMEEND);
}
#endif

/**
    IPE wait frame end

    Wait IPE frame end flag.

    @param clr_flg_sel IPE_NOCLRFLG : Don't clear flag before wait, IPE_CLRFLG : Clear flag before wait.

    @return None.
*/
void ipe_wait_frame_end(IPE_CLR_FLG_ENUM clr_flg_sel)
{
	FLGPTN flag;

	if (clr_flg_sel) {
		ipe_platform_flg_clear(FLGPTN_IPE_FRAMEEND);
	}

#if (defined(_NVT_EMULATION_) == ON)
	if (timer_open(&ipe_timer_id, ipe_time_out_cb) != E_OK) {
		DBG_WRN("IPE allocate timer fail\r\n");
	} else {
		timer_cfg(ipe_timer_id, 100000000, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	}
#endif

	ipe_platform_flg_wait(&flag, FLGPTN_IPE_FRAMEEND);

#if (IPE_DMA_CACHE_HANDLE == 1)
	ipe_flush_cache_end();
#endif

#if (defined(_NVT_EMULATION_) == ON)
	timer_close(ipe_timer_id);
#endif
}

/**
    IPE clear frame start

    Clear IPE frame start flag.

    @param None.

    @return None.
*/
void ipe_clr_frame_start(void)
{
	ipe_platform_flg_clear(FLGPTN_IPE_FRMSTART);
}

/**
    IPE wait frame start

    Wait IPE frame start flag.

    @param clr_flg_sel IPE_NOCLRFLG : Don't clear flag before wait, IPE_CLRFLG : Clear flag before wait.

    @return None.
*/
void ipe_wait_frame_start(IPE_CLR_FLG_ENUM clr_flg_sel)
{
	FLGPTN flag;

	if (clr_flg_sel) {
		ipe_platform_flg_clear(FLGPTN_IPE_FRMSTART);
	}

	ipe_platform_flg_wait(&flag, FLGPTN_IPE_FRMSTART);
}

/**
    IPE clear DMA end

    Clear IPE DMA end flag.

    @param None.

    @return None.
*/
void ipe_clr_dma_end(void)
{
	ipe_platform_flg_clear(FLGPTN_IPE_YCOUTEND | FLGPTN_IPE_VAOUTEND);
}

/**
    IPE wait DMA end

    Wait IPE DMA end flag.

    @param clr_flg_sel IPE_NOCLRFLG : Don't clear flag before wait, IPE_CLRFLG : Clear flag before wait.

    @return None.
*/
void ipe_wait_dma_end(IPE_CLR_FLG_ENUM clr_flg_sel)
{
	FLGPTN flag;

	if (clr_flg_sel) {
		ipe_platform_flg_clear(FLGPTN_IPE_YCOUTEND | FLGPTN_IPE_VAOUTEND);
	}
	ipe_platform_flg_wait(&flag, FLGPTN_IPE_YCOUTEND | FLGPTN_IPE_VAOUTEND);
}

void ipe_clr_gamma_in_end(void)
{
	ipe_platform_flg_clear(FLGPTN_IPE_DMAIN0END);
}

void ipe_wait_gamma_in_end(void)
{
	FLGPTN flag;

	ipe_platform_flg_wait(&flag, FLGPTN_IPE_DMAIN0END);
}

/**
    IPE clear LL job end

    Clear IPE LL job end flag.

    @param None.

    @return None.
*/
void ipe_clr_ll_job_end(void)
{
	ipe_platform_flg_clear(FLGPTN_IPE_LL_JOB_END);
}

/**
    IPE wait LL job end

    Wait IPE LL job end flag.

    @param clr_flg_sel IPE_NOCLRFLG : Don't clear flag before wait, IPE_CLRFLG : Clear flag before wait.

    @return None.
*/
void ipe_wait_ll_job_end(IPE_CLR_FLG_ENUM clr_flg_sel)
{
	FLGPTN flag;

	if (clr_flg_sel) {
		ipe_platform_flg_clear(FLGPTN_IPE_LL_JOB_END);
	}
	ipe_platform_flg_wait(&flag, FLGPTN_IPE_LL_JOB_END);
}

/**
    IPE clear LL done

    Clear IPE LL done flag.

    @param None.

    @return None.
*/
void ipe_clr_ll_done(void)
{
	ipe_platform_flg_clear(FLGPTN_IPE_LL_DONE);
}

/**
    IPE wait LL done

    Wait IPE LL done flag.

    @param clr_flg_sel IPE_NOCLRFLG : Don't clear flag before wait, IPE_CLRFLG : Clear flag before wait.

    @return None.
*/
void ipe_wait_ll_done(IPE_CLR_FLG_ENUM clr_flg_sel)
{
	FLGPTN flag;

	if (clr_flg_sel) {
		ipe_platform_flg_clear(FLGPTN_IPE_LL_DONE);
	}
	ipe_platform_flg_wait(&flag, FLGPTN_IPE_LL_DONE);
}

/**
    IPE attach.

    attach IPE

    @return
        @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ipe_attach(VOID)
{
	if (g_ipe_clk_en == FALSE) {
		ipe_platform_enable_clk();
		g_ipe_clk_en = TRUE;
	}
	return E_OK;
}

/**
    IPE detach.

    detach IPE

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER ipe_detach(VOID)
{
	if (g_ipe_clk_en == TRUE) {
		ipe_platform_disable_clk();
		g_ipe_clk_en = FALSE;
	}
	return E_OK;
}


/**
    IPE open

    Initial flow for IPE operation.

    @param IPE_OPENOBJ IPE open object, includes clock select and ISR call back.

    @return ER IPE open status.
*/
ER ipe_open(IPE_OPENOBJ *p_ipe_openinfo)
{
	ER er_return;
	UINT32 config_clock; //, pllClock;

	// Lock semaphore
	er_return = ipe_lock();
	if (er_return != E_OK) {
		return er_return;
	}

	// Check state-machine
	er_return = ipe_state_machine(IPE_OP_OPEN, NOTUPDATE); //er_return = ipe_state_machine(g_ipe_status, IPE_OP_CLOSE);
	if (er_return != E_OK) {
		ipe_unlock();
		return er_return;
	}

	// NOTE: XXXXX
#if 0
	// Power on module
#ifdef __KERNEL__
#else
	pmc_turnonPower(PMC_MODULE_IPE);
#endif
#endif

	// Select PLL clock source & enable PLL clock source
	config_clock = p_ipe_openinfo->ipe_clk_sel;
	ipe_platform_set_clk_rate(config_clock);

	// Attach
	ipe_attach();

	// Disable Sram shut down
	ipe_platform_disable_sram_shutdown();

	// Enable engine interrupt
	ipe_platform_int_enable();

#if 0
	// Clear engine interrupt status
	ipe_clear_int(IPE_INTE_ALL);

	// Clear SW flag
	er_return = ipe_platform_flg_clear(FLGPTN_IPE_FRAMEEND | FLGPTN_IPE_STRPEND | FLGPTN_IPE_FRMSTART |
									   FLGPTN_IPE_YCOUTEND | FLGPTN_IPE_DMAIN0END | FLGPTN_IPE_DMAIN1END |
									   FLGPTN_IPE_GAMERR | FLGPTN_IPE_3DCCERR);

	if (er_return != E_OK) {
		return er_return;
	}

	// SW reset
	ipe_reset_on();
	//ipe_reset_off();

	// Clear engine interrupt status
	ipe_clear_int(IPE_INTE_ALL);  // 98520: fixed defog subout dummy interrupt
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ipe_ctrl_flow_to_do == ENABLE) {
		// Clear engine interrupt status
		ipe_clear_int(IPE_INTE_ALL);

		// Clear SW flag
		er_return = ipe_platform_flg_clear(FLGPTN_IPE_FRAMEEND | FLGPTN_IPE_STRPEND | FLGPTN_IPE_FRMSTART |
										   FLGPTN_IPE_YCOUTEND | FLGPTN_IPE_DMAIN0END | FLGPTN_IPE_DMAIN1END |
										   FLGPTN_IPE_GAMERR | FLGPTN_IPE_3DCCERR);

		if (er_return != E_OK) {
			return er_return;
		}

		// SW reset
		ipe_reset_on();
		//ipe_reset_off();

		// Clear engine interrupt status
		ipe_clear_int(IPE_INTE_ALL);  // 98520: fixed defog subout dummy interrupt
	}
#else
	// Clear engine interrupt status
	ipe_clear_int(IPE_INTE_ALL);

	// Clear SW flag
	er_return = ipe_platform_flg_clear(FLGPTN_IPE_FRAMEEND | FLGPTN_IPE_STRPEND | FLGPTN_IPE_FRMSTART |
									   FLGPTN_IPE_YCOUTEND | FLGPTN_IPE_DMAIN0END | FLGPTN_IPE_DMAIN1END |
									   FLGPTN_IPE_GAMERR | FLGPTN_IPE_3DCCERR);

	if (er_return != E_OK) {
		return er_return;
	}

	// SW reset
	ipe_reset_on();
	//ipe_reset_off();

	// Clear engine interrupt status
	ipe_clear_int(IPE_INTE_ALL);  // 98520: fixed defog subout dummy interrupt
#endif

	default_hn = 0, ipe_maxhstrp = 0, ipe_minhstrp = 0;

	if (nvt_get_chip_id() == CHIP_NA51055)  {
		default_hn = 672;
		ipe_maxhstrp = default_hn;
		ipe_minhstrp = 4;
	} else if (nvt_get_chip_id() == CHIP_NA51084) {
		default_hn = 1024;
		ipe_maxhstrp = default_hn;
		ipe_minhstrp = 4;
	} else {
        DBG_ERR("IPE: get chip id %d out of range\r\n", nvt_get_chip_id());
        return E_PAR;
    }

	// Hook call-back function
#if 0//(DRV_SUPPORT_IST == ENABLE)
	pfIstCB_IPE     = p_ipe_openinfo->FP_IPEISR_CB;
#else
	g_pfIpeIntHdlCB = p_ipe_openinfo->FP_IPEISR_CB;
#endif

	// update state-machine
	er_return = ipe_state_machine(IPE_OP_OPEN, UPDATE);

	//return E_OK;
	return er_return;
}

/**
    IPE close

    Close flow for IPE operation.

    @param None.

    @return ER IPE close status.
*/
ER ipe_close(void)
{
	ER er_return;

	// Check state-machine
	er_return = ipe_state_machine(IPE_OP_CLOSE, NOTUPDATE); //er_return = ipe_state_machine(g_ipe_status, IPE_OP_CLOSE);
	if (er_return != E_OK) {
		return er_return;
	}

	// Check semaphore
	if (g_ipe_lock_status != TASK_LOCKED) {
		return E_SYS;
	}

	// Diable engine interrupt
	ipe_platform_int_disable();

	// Enable Sram shut down
	ipe_platform_enable_sram_shutdown();

	// Detach
	ipe_detach();

	// update state-machine
	//g_ipe_status = IPE_STATUS_IDLE;
	er_return = ipe_state_machine(IPE_OP_CLOSE, UPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	// Unlock semaphore
	er_return = ipe_unlock();
	if (er_return != E_OK) {
		return er_return;
	}

	return E_OK;
}

/**
    IPE change param

    IPE IQ parameters configuration.

    @param p_iq_info IQ parameters, including NR, edge, color, special effect, threolding, and VA.

    @return ER IPE change parameters status.
*/
ER ipe_change_param(IPE_IQINFO *p_iq_info)
{

	ER er_return;

	//IPE_EEXT_PARAM *eext_param;
	//IPE_EPROC_PARAM *eproc_param;
	//IPE_RGBLPF_PARAM *rgblpf_param;
	//IPE_PFR_PARAM *pfr_param;
	//IPE_CC_PARAM *cc_param;
	//IPE_CST_PARAM *cst_param;
	//IPE_CCTRL_PARAM *cctrl_param;
	//IPE_CADJ_EENH_PARAM *cadj_eenh_param;
	//IPE_CADJ_EINV_PARAM *cadj_einv_param;
	//IPE_CADJ_YCCON_PARAM *cadj_yccon_param;
	//IPE_CADJ_COFS_PARAM *cadj_cofs_param;
	//IPE_CADJ_RAND_PARAM *cadj_rand_param;
	//IPE_CADJ_FIXTH_PARAM *cadj_fixth_param;
	//IPE_CADJ_MASK_PARAM *cadj_mask_param;
	//IPE_CSTP_PARAM *cstp_param;
	//IPE_DEFOG_SUBIMG_PARAM *defog_subout_param;
	//IPE_DEFOG_PARAM *defog_param;
	//DEFOG_SCAL_PARAM *defog_scal_param;
	//DEFOG_ENV_ESTIMATION *defog_env_param;
	//DEFOG_STRENGTH_PARAM *defog_str_param;
	//DEFOG_OUTBLD_PARAM *defog_outbld_param;
	//DEFOG_STCS_PARAM *defog_stcs;
	//IPE_LCE_PARAM *lce_param;

	//IPE_GAMYRAND *gamycur_rand;
	//IPE_DEFOGROUND *defog_rand;
	//IPE_ETH_PARAM *eth;
	//IPE_VA_FLTR_GROUP_PARAM *va_filt_g1;//sc
	//IPE_VA_FLTR_GROUP_PARAM *va_filt_g2;
	//IPE_VA_WIN_PARAM    *va_win_info;
	//IPE_INDEP_VA_PARAM va_indep_win_info[5];
	UINT32 phy_addr0, phy_addr1;
#if (IPE_DMA_CACHE_HANDLE)
	//UINT32 addr0, size0;
#endif

	//auto mode
	int slope_con_eng, slope_con_eng_hld, wlow, whigh, th_edge, th_flat;
	UINT32 blk_size_h, blk_size_v, blk_cent_fact_h, blk_cent_fact_v;
	UINT32 subout_sizeh, subout_sizev, img_width, img_height;
	UINT32 scaling_h, scaling_v;
	UINT32 airlight_nfactor, airlight_max;
	UINT16 airlight_val[3];

	er_return = ipe_state_machine(IPE_OP_CHGPARAM, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	//eext_param = p_iq_info->p_eext_param;
	//eproc_param = p_iq_info->p_eproc_param;
	//rgblpf_param = p_iq_info->p_rgblpf_param;
	//pfr_param = p_iq_info->p_pfr_param;
	//cc_param = p_iq_info->p_cc_param;
	//cst_param = p_iq_info->p_cst_param;
	//cctrl_param = p_iq_info->p_cctrl_param;
	//cadj_eenh_param = p_iq_info->p_cadj_eenh_param;
	//cadj_einv_param = p_iq_info->p_cadj_einv_param;
	//cadj_yccon_param = p_iq_info->p_cadj_yccon_param;
	//cadj_cofs_param = p_iq_info->p_cadj_cofs_param;
	//cadj_rand_param = p_iq_info->p_cadj_rand_param;
	//cadj_fixth_param = p_iq_info->p_cadj_fixth_param;
	//cadj_mask_param = p_iq_info->p_cadj_mask_param;
	//cstp_param = p_iq_info->p_cstp_param;
	//defog_subout_param = p_iq_info->p_dfg_subimg_param;
	//defog_param = p_iq_info->p_defog_param;
	//lce_param = p_iq_info->p_lce_param;
	//gamycur_rand = p_iq_info->p_gamycur_rand;
	//defog_rand = p_iq_info->p_defog_rand;
	//eth = p_iq_info->p_eth;
	//va_filt_g1 = p_iq_info->p_va_filt_g1;//sc
	//va_filt_g2 = p_iq_info->p_va_filt_g2;
	//va_win_info = p_iq_info->p_va_win_info;
	//va_indep_win_info[0] = p_iq_info->p_va_indep_win_info[0];
	//va_indep_win_info[1] = p_iq_info->p_va_indep_win_info[1];
	//va_indep_win_info[2] = p_iq_info->p_va_indep_win_info[2];
	//va_indep_win_info[3] = p_iq_info->p_va_indep_win_info[3];
	//va_indep_win_info[4] = p_iq_info->p_va_indep_win_info[4];

	//DBG_ERR("driver para update 0x%x\r\n", p_iq_info->param_update_sel);

	//DBG_USER("ipe_change_param() param_update_sel 0x%08x\r\n", (unsigned int)p_iq_info->param_update_sel);

	//Gamma & Ycurve
	if (p_iq_info->param_update_sel & (IPE_SET_GAMMA | IPE_SET_YCURVE)) {

#if (IPE_DMA_CACHE_HANDLE)
		if (p_iq_info->param_update_sel & IPE_SET_GAMMA) {
			//addr0 = p_iq_info->addr_gamma;
			//size0 = IPE_DRAM_GAMMA_BYTE;
			if (p_iq_info->addr_gamma != 0) {
				ipe_platform_dma_flush_mem2dev(p_iq_info->addr_gamma, IPE_DRAM_GAMMA_BYTE);
				//dma_flushWriteCache(addr0, size0);
			}
			phy_addr0 = ipe_platform_va2pa(p_iq_info->addr_gamma);
			//DBG_USER("ycurve addr_gamma = 0x%08x, phy_addr0 = 0x%08x\r\n", (unsigned int)p_iq_info->addr_gamma, (unsigned int)phy_addr0);

		} else {
			phy_addr0 = 0;
		}
		if (p_iq_info->param_update_sel & IPE_SET_YCURVE) {
			//addr0 = p_iq_info->p_ycurve_param->addr_ycurve;
			//size0 = IPE_DRAM_YCURVE_BYTE;
			if (p_iq_info->p_ycurve_param->addr_ycurve != 0) {
				ipe_platform_dma_flush_mem2dev(p_iq_info->p_ycurve_param->addr_ycurve, IPE_DRAM_YCURVE_BYTE);
				//dma_flushWriteCache(addr0, size0);
			}
			phy_addr1 = ipe_platform_va2pa(p_iq_info->p_ycurve_param->addr_ycurve);
			//DBG_USER("ycurve addr_ycurve = 0x%08x, phy_addr1 = 0x%08x\r\n", (unsigned int)p_iq_info->p_ycurve_param->addr_ycurve, (unsigned int)phy_addr1);
		} else {
			phy_addr1 = 0;
		}
#else
		phy_addr0 = p_iq_info->addr_gamma;
		phy_addr1 = p_iq_info->p_ycurve_param->addr_ycurve;
#endif

		if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
			if (p_iq_info->param_update_sel & IPE_SET_GAMMA) {
				ipe_set_dma_in_gamma_lut_addr(phy_addr0);
				ipe_set_gam_y_rand(p_iq_info->p_gamycur_rand);
			}
			if (p_iq_info->param_update_sel & IPE_SET_YCURVE) {
				ipe_set_ycurve_sel(p_iq_info->p_ycurve_param->ycurve_sel);
				ipe_set_dma_in_ycurve_lut_addr(phy_addr1);
				ipe_set_gam_y_rand(p_iq_info->p_gamycur_rand);
			}
		}
	}

	//Edge extract parameters
	if (p_iq_info->param_update_sel & IPE_SET_EDGEEXT) {
		if (p_iq_info->p_eext_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_edge_extract_sel(p_iq_info->p_eext_param->eext_gam_sel, p_iq_info->p_eext_param->eext_ch_sel);
				if (p_iq_info->p_eext_param->p_edgeker_b != NULL) {
					ipe_set_edge_extract_b(p_iq_info->p_eext_param->p_edgeker_b, p_iq_info->p_eext_param->p_edgeker_b[10], p_iq_info->p_eext_param->p_edgeker_b[11]);
				}
				ipe_set_eext_kerstrength(&p_iq_info->p_eext_param->eext_kerstrength);
				ipe_set_eext_engcon(&p_iq_info->p_eext_param->eext_engcon);
				ipe_set_ker_thickness(&p_iq_info->p_eext_param->ker_thickness);
				ipe_set_ker_thickness_hld(&p_iq_info->p_eext_param->ker_thickness_hld);
				ipe_set_region(&p_iq_info->p_eext_param->eext_region);
				if (p_iq_info->p_eext_param->eext_region.region_slope_mode == IPE_PARAM_AUTO_MODE) {

					wlow = p_iq_info->p_eext_param->eext_region.wt_low;
					whigh = p_iq_info->p_eext_param->eext_region.wt_high;
					th_edge = p_iq_info->p_eext_param->eext_region.th_edge;
					th_flat = p_iq_info->p_eext_param->eext_region.th_flat;

					slope_con_eng = (th_edge == th_flat) ? 65535 : (((whigh -  wlow) * 1024) / abs(th_edge - th_flat));

					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_EEXT), "eext: wlow, whigh = (%d, %d) \r\n", wlow, whigh);
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_EEXT), "eext: th_flat, th_edge = (%d, %d) \r\n", th_edge, th_flat);
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_EEXT), "eext: slope_con_eng = (%d) \r\n", slope_con_eng);

					wlow = p_iq_info->p_eext_param->eext_region.wt_low_hld;
					whigh = p_iq_info->p_eext_param->eext_region.wt_high_hld;
					th_edge = p_iq_info->p_eext_param->eext_region.th_edge_hld;
					th_flat = p_iq_info->p_eext_param->eext_region.th_flat_hld;
					slope_con_eng_hld = (th_edge == th_flat) ? 65535 : (((whigh -  wlow) * 1024) / abs(th_edge - th_flat));

					ipe_set_region_slope((UINT16)slope_con_eng, (UINT16)slope_con_eng_hld);

					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_EEXT), "eext: wlow, whigh = (%d, %d) \r\n", wlow, whigh);
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_EEXT), "eext: th_flat, th_edge = (%d, %d) \r\n", th_edge, th_flat);
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_EEXT), "eext: slope_con_eng_hld = (%d) \r\n", slope_con_eng_hld);


				} else {
					ipe_set_region_slope(p_iq_info->p_eext_param->eext_region.slope_con_eng, p_iq_info->p_eext_param->eext_region.slope_con_eng_hld);
				}

				ipe_set_overshoot(&p_iq_info->p_eext_param->eext_overshoot);

			}
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_NON_FD_LATCHED)) {
				ipe_set_tone_remap(p_iq_info->p_eext_param->p_tonemap_lut);

			}

		}
	}

	//Edge process parameters
	if (p_iq_info->param_update_sel & IPE_SET_EDGEPROC) {
		if (p_iq_info->p_eproc_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_edgemap_th(p_iq_info->p_eproc_param->p_edgemap_th);
				ipe_set_esmap_th(p_iq_info->p_eproc_param->p_esmap_th);
				//ipe_setEdgeMapOutSel(p_iq_info->p_eproc_param->EOutSel);
			}
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_NON_FD_LATCHED)) {
				if (p_iq_info->p_eproc_param->p_edgemap_lut != NULL) {
					ipe_set_edgemap_tab(p_iq_info->p_eproc_param->p_edgemap_lut);
				}

				if (p_iq_info->p_eproc_param->p_esmap_lut != NULL) {
					ipe_set_estab(p_iq_info->p_eproc_param->p_esmap_lut);
				}
			}
		}
	}

	//RGB LPF parameters
	if (p_iq_info->param_update_sel & IPE_SET_RGBLPF) {
		if (p_iq_info->p_rgblpf_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_rgblpf(p_iq_info->p_rgblpf_param);
			}
		}
	}

	//PFR parameters
	if (p_iq_info->param_update_sel & IPE_SET_PFR) {
		if (p_iq_info->p_pfr_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {

				ipe_set_pfr_general(p_iq_info->p_pfr_param);
				ipe_set_pfr_color(0, &p_iq_info->p_pfr_param->color_wet_set[0]);
				ipe_set_pfr_color(1, &p_iq_info->p_pfr_param->color_wet_set[1]);
				ipe_set_pfr_color(2, &p_iq_info->p_pfr_param->color_wet_set[2]);
				ipe_set_pfr_color(3, &p_iq_info->p_pfr_param->color_wet_set[3]);

				if (p_iq_info->p_pfr_param->p_luma_lut != NULL) {
					ipe_set_pfr_luma(p_iq_info->p_pfr_param->luma_th, p_iq_info->p_pfr_param->p_luma_lut);
				} else {
					ipe_set_pfr_luma(p_iq_info->p_pfr_param->luma_th, NULL);
				}
			}
		}
	}


	//CC parameters
	if (p_iq_info->param_update_sel & IPE_SET_COLORCORRECT) {
		if (p_iq_info->p_cc_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_color_correct(p_iq_info->p_cc_param);
			}
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_NON_FD_LATCHED)) {
				if (p_iq_info->p_cc_param->p_fstab != NULL) {
					ipe_set_fstab(p_iq_info->p_cc_param->p_fstab);
				}
				if (p_iq_info->p_cc_param->p_fdtab != NULL) {
					ipe_set_fdtab(p_iq_info->p_cc_param->p_fdtab);
				}
			}
		}
	}

	//CST parameters
	if (p_iq_info->param_update_sel & IPE_SET_CST) {
		if (p_iq_info->p_cst_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_cst(p_iq_info->p_cst_param);
			}
		}
	}

	//CCTRL parameters
	if (p_iq_info->param_update_sel & IPE_SET_CCTRL) {
		if (p_iq_info->p_cctrl_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_int_sat_offset(p_iq_info->p_cctrl_param->int_ofs, p_iq_info->p_cctrl_param->sat_ofs);
				ipe_set_hue_rotate(p_iq_info->p_cctrl_param->hue_rotate_en);
				ipe_set_color_sup(p_iq_info->p_cctrl_param->cctrl_sel, p_iq_info->p_cctrl_param->vdet_div);
				ipe_set_hue_c2g_en(p_iq_info->p_cctrl_param->hue_c2g_en);
			}
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_NON_FD_LATCHED)) {
				if (p_iq_info->p_cctrl_param->p_hue_tab != NULL) {
					ipe_set_cctrl_hue(p_iq_info->p_cctrl_param->p_hue_tab);
				}
				if (p_iq_info->p_cctrl_param->p_int_tab != NULL) {
					ipe_set_cctrl_int(p_iq_info->p_cctrl_param->p_int_tab);
				}
				if (p_iq_info->p_cctrl_param->p_sat_tab != NULL) {
					ipe_set_cctrl_sat(p_iq_info->p_cctrl_param->p_sat_tab);
				}
				if (p_iq_info->p_cctrl_param->p_edge_tab != NULL) {
					ipe_set_cctrl_edge(p_iq_info->p_cctrl_param->p_edge_tab);
				}
				if (p_iq_info->p_cctrl_param->p_dds_tab != NULL) {
					ipe_set_cctrl_dds(p_iq_info->p_cctrl_param->p_dds_tab);
				}
			}
		}
	}

	//CADJ edge enhance parameters
	if (p_iq_info->param_update_sel & IPE_SET_CADJ_EENH) {
		if (p_iq_info->p_cadj_eenh_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_edge_enhance(p_iq_info->p_cadj_eenh_param->enh_p, p_iq_info->p_cadj_eenh_param->enh_n);
			}
		}
	}

	//CADJ edge inverse parameters
	if (p_iq_info->param_update_sel & IPE_SET_CADJ_EINV) {
		if (p_iq_info->p_cadj_einv_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_edge_inv(p_iq_info->p_cadj_einv_param->einv_p_en, p_iq_info->p_cadj_einv_param->einv_n_en);
			}
		}
	}

	//CADJ YC contrast parameters
	if (p_iq_info->param_update_sel & IPE_SET_CADJ_YCCON) {
		if (p_iq_info->p_cadj_yccon_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_ycon(p_iq_info->p_cadj_yccon_param->ycon);
				ipe_set_cbcr_con(p_iq_info->p_cadj_yccon_param->ccon);
				ipe_set_cbcr_contab_sel(p_iq_info->p_cadj_yccon_param->ccontab_sel);
			}
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_NON_FD_LATCHED)) {
				ipe_set_cbcr_contab(p_iq_info->p_cadj_yccon_param->p_ccon_tab);
			}
		}
	}

	//CADJ CbCr offset parameters
	if (p_iq_info->param_update_sel & IPE_SET_CADJ_COFS) {
		if (p_iq_info->p_cadj_cofs_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_cbcr_offset(p_iq_info->p_cadj_cofs_param->cb_ofs, p_iq_info->p_cadj_cofs_param->cr_ofs);
			}
		}
	}

	//CADJ random parameters
	if (p_iq_info->param_update_sel & IPE_SET_CADJ_RAND) {
		if (p_iq_info->p_cadj_rand_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_yc_rand(p_iq_info->p_cadj_rand_param);
			}
		}
	}

	//CADJ fix threshold parameters
	if (p_iq_info->param_update_sel & IPE_SET_CADJ_FIXTH) {
		if (p_iq_info->p_cadj_fixth_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_yfix_th(p_iq_info->p_cadj_fixth_param->p_yth1, p_iq_info->p_cadj_fixth_param->p_yth2);
				ipe_set_cbcr_fixth(p_iq_info->p_cadj_fixth_param->p_cth);
			}
		}
	}

	//CADJ YC mask parameters
	if (p_iq_info->param_update_sel & IPE_SET_CADJ_MASK) {
		if (p_iq_info->p_cadj_mask_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_yc_mask(p_iq_info->p_cadj_mask_param->ymask, p_iq_info->p_cadj_mask_param->cbmask, p_iq_info->p_cadj_mask_param->crmask);
			}
		}
	}

	//CSTP parameters
	if (p_iq_info->param_update_sel & IPE_SET_CSTP) {
		if (p_iq_info->p_cstp_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_cstp(p_iq_info->p_cstp_param->cstp_rto);
			}
		}
	}

	//ETH parameters
	if (p_iq_info->param_update_sel & IPE_SET_ETH) {
		if (p_iq_info->p_eth != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_eth_param(p_iq_info->p_eth);
			}
		}
	}

	//Edge debug parameter
	if (p_iq_info->param_update_sel & IPE_SET_EDGE_DBG) {
		if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
			ipe_set_edge_dbg_sel(p_iq_info->edge_dbg_mode_sel);
		}
	}

	// defog subout
	if (p_iq_info->param_update_sel & IPE_SET_DEFOG_SUBOUT) {
		if (p_iq_info->p_dfg_subimg_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {

				ipe_set_dfg_subimg_size(p_iq_info->p_dfg_subimg_param->dfg_subimg_size);

				if (p_iq_info->p_dfg_subimg_param->subimg_param_mode == IPE_PARAM_AUTO_MODE) {
					img_width = p_iq_info->p_dfg_subimg_param->input_size.h_size;
					img_height = p_iq_info->p_dfg_subimg_param->input_size.v_size;
					subout_sizeh = p_iq_info->p_dfg_subimg_param->dfg_subimg_size.h_size;
					subout_sizev = p_iq_info->p_dfg_subimg_param->dfg_subimg_size.v_size;
					//blk_size_h = (((double)(img_width - 1) / (double)(subout_sizeh)));
					//blk_size_v = (((double)(img_height - 1) / (double)(subout_sizev)));
					//blk_cent_fact_h = (((double)(img_width - 1) * (double)(4096) / (double)(subout_sizeh - 1)) + 0.5);
					//blk_cent_fact_v = (((double)(img_height - 1) * (double)(4096) / (double)(subout_sizev - 1)) + 0.5);

					if (subout_sizeh == 0) {
						subout_sizeh = 16;
						DBG_ERR("subimg size h cannot be 0, modify to 16\r\n");
					}
					if (subout_sizev == 0) {
						subout_sizev = 9;
						DBG_ERR("subimg size v cannot be 0, modify to 9\r\n");
					}
					blk_size_h = (((img_width - 1) / (subout_sizeh)));
					blk_size_v = (((img_height - 1) / (subout_sizev)));
					blk_cent_fact_h = ((((img_width - 1) << 12) / (subout_sizeh - 1)));
					blk_cent_fact_v = ((((img_height - 1) << 12) / (subout_sizev - 1)));

					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFOG), "img size = %d x %d \r\n", (int)img_width, (int)img_height);
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFOG), "subimg size = %d x %d \r\n", (int)subout_sizeh, (int)subout_sizev);
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFOG), "blk_size (%d x %d) \r\n", (int)blk_size_h, (int)blk_size_v);
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFOG), "blk_cent_fact (%d x %d) \r\n", (int)blk_cent_fact_h, (int)blk_cent_fact_v);

					ipe_set_dfg_subout_param(blk_size_h, blk_cent_fact_h, blk_size_v, blk_cent_fact_v);
				} else {

					blk_size_h = p_iq_info->p_dfg_subimg_param->subout_blksize.h_size;
					blk_size_v = p_iq_info->p_dfg_subimg_param->subout_blksize.v_size;
					blk_cent_fact_h = p_iq_info->p_dfg_subimg_param->subout_blk_centfact.factor_h;
					blk_cent_fact_v = p_iq_info->p_dfg_subimg_param->subout_blk_centfact.factor_v;
					ipe_set_dfg_subout_param(blk_size_h, blk_cent_fact_h, blk_size_v, blk_cent_fact_v);
				}
				if (p_iq_info->p_dfg_subimg_param->subimg_param_mode == IPE_PARAM_AUTO_MODE) {
					img_width = p_iq_info->p_dfg_subimg_param->input_size.h_size;
					img_height = p_iq_info->p_dfg_subimg_param->input_size.v_size;
					subout_sizeh = p_iq_info->p_dfg_subimg_param->dfg_subimg_size.h_size;
					subout_sizev = p_iq_info->p_dfg_subimg_param->dfg_subimg_size.v_size;
					if (img_width == 0) {
						img_width = 1920;
						DBG_ERR("img_width cannot be 0, modify to 1920\r\n");
					}
					if (img_height == 0) {
						img_height = 1080;
						DBG_ERR("img_height cannot be 0, modify to 1080\r\n");
					}
					scaling_h = ((((subout_sizeh - 1) << 16) / (img_width - 1)));
					scaling_v = ((((subout_sizev - 1) << 16) / (img_height - 1)));

					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFOG), "scaling (%d x %d) \r\n", (int)scaling_h, (int)scaling_v);

					ipe_set_dfg_scal_factor(scaling_h, scaling_v);
				} else {
					scaling_h = p_iq_info->p_dfg_subimg_param->subin_scaling_factor.factor_h;
					scaling_v = p_iq_info->p_dfg_subimg_param->subin_scaling_factor.factor_v;
					ipe_set_dfg_scal_factor(scaling_h, scaling_v);
				}
			}

			ipe_set_dfg_scal_filtcoeff(p_iq_info->p_dfg_subimg_param->p_subimg_ftrcoef);

		}
		//DBG_USER("change defog subout \r\n");
	}

	//defog random bit
	if (p_iq_info->p_defog_rand != NULL) {
		ipe_set_defog_rand(p_iq_info->p_defog_rand);
	}

	// defog
	if (p_iq_info->param_update_sel & IPE_SET_DEFOG) {
		if (p_iq_info->p_defog_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_dfg_input_bld(p_iq_info->p_defog_param->input_bld.p_in_blend_wt);

				//defog_scal_param = &p_iq_info->p_defog_param->scalup_param;
				ipe_set_dfg_scalg_edgeinterplut(p_iq_info->p_defog_param->scalup_param.p_interp_diff_lut);
				ipe_set_defog_scal_edgeinterp(p_iq_info->p_defog_param->scalup_param.interp_wdist, p_iq_info->p_defog_param->scalup_param.interp_wout, p_iq_info->p_defog_param->scalup_param.interp_wcenter, p_iq_info->p_defog_param->scalup_param.interp_wsrc);

				//defog_env_param = &p_iq_info->p_defog_param->env_estimation;
				ipe_set_defog_atmospherelight(p_iq_info->p_defog_param->env_estimation.p_dfg_airlight[0], p_iq_info->p_defog_param->env_estimation.p_dfg_airlight[1], p_iq_info->p_defog_param->env_estimation.p_dfg_airlight[2]);
				ipe_set_defog_fog_protect(p_iq_info->p_defog_param->env_estimation.dfg_self_comp_en, p_iq_info->p_defog_param->env_estimation.dfg_min_diff);
				ipe_set_defog_fog_mod(p_iq_info->p_defog_param->env_estimation.p_fog_mod_lut);

				//defog_str_param = &p_iq_info->p_defog_param->dfg_strength;
				ipe_set_defog_fog_target_lut(p_iq_info->p_defog_param->dfg_strength.p_target_lut);
				ipe_set_defog_strength(p_iq_info->p_defog_param->dfg_strength.str_mode_sel, p_iq_info->p_defog_param->dfg_strength.fog_ratio, p_iq_info->p_defog_param->dfg_strength.dgain_ratio, p_iq_info->p_defog_param->dfg_strength.gain_th);

				//defog_outbld_param = &p_iq_info->p_defog_param->dfg_outbld;
				if (p_iq_info->p_defog_param->dfg_outbld.airlight_param_mode == IPE_PARAM_AUTO_MODE) {
					ipe_get_defog_airlight_setting(airlight_val);
					airlight_max = max(max((UINT32)airlight_val[0], (UINT32)airlight_val[1]), (UINT32)airlight_val[2]);
					if (airlight_max == 0) {
						airlight_nfactor = (((1 << 20) / 1023));
						DBG_ERR("airlight_max =  0, airlight_val = [%d, %d, %d] \r\n", (int)airlight_val[0], (int)airlight_val[1], (int)airlight_val[2]);
					} else {
						airlight_nfactor = (((1 << 20) / airlight_max));
					}
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFOG), "airlight_val = [%d, %d, %d] \r\n", (int)airlight_val[0], (int)airlight_val[1], (int)airlight_val[2]);
					//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_DEFOG), "airlight_max = %d, airlight_nfactor = %d \r\n", (int)airlight_max, (int)airlight_nfactor);
				} else {
					airlight_nfactor = p_iq_info->p_defog_param->dfg_outbld.airlight_nfactor;
				}
				ipe_set_defog_outbld_local(p_iq_info->p_defog_param->dfg_outbld.outbld_local_en, airlight_nfactor);
				ipe_set_defog_outbld_lum_lut(p_iq_info->p_defog_param->dfg_outbld.outbld_ref_sel, p_iq_info->p_defog_param->dfg_outbld.p_outbld_lum_wt);
				ipe_set_defog_outbld_diff_lut(p_iq_info->p_defog_param->dfg_outbld.p_outbld_diff_wt);

				//defog_stcs = &p_iq_info->p_defog_param->dfg_stcs;
				ipe_set_defog_stcs_pcnt(p_iq_info->p_defog_param->dfg_stcs.dfg_stcs_pixelcnt);
			}
		}
		//DBG_USER("change defog  \r\n");
	}

	// LCE
	if (p_iq_info->param_update_sel & IPE_SET_LCE) {
		if (p_iq_info->p_lce_param != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_lce_lum_lut(p_iq_info->p_lce_param->p_lum_wt_lut);
				ipe_set_lce_diff_param(p_iq_info->p_lce_param->diff_wt_avg, p_iq_info->p_lce_param->diff_wt_pos, p_iq_info->p_lce_param->diff_wt_neg);
			}
		}
		//DBG_USER("change lce \r\n");
	}

	//if VA or indep //sc
	if (p_iq_info->param_update_sel & (IPE_SET_VA | IPE_SET_INDEP_VA)) {
		if (p_iq_info->p_va_filt_g1 != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_va_filter_g1(p_iq_info->p_va_filt_g1);
			}
		}
		if (p_iq_info->p_va_filt_g2 != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_va_filter_g2(p_iq_info->p_va_filt_g2);
			}
		}
		//DBG_USER("change VA filter \r\n");
	}

	//VA parameters
	if (p_iq_info->param_update_sel & IPE_SET_VA) {
		if (p_iq_info->p_va_win_info != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_va_win_info(p_iq_info->p_va_win_info);
			}
		}
		if (p_iq_info->p_va_filt_g1 != NULL || p_iq_info->p_va_filt_g2 != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_va_mode_enable(p_iq_info->p_va_filt_g1, p_iq_info->p_va_filt_g2);
			}
		}
		//DBG_USER("change VA \r\n");
	}

	//indep VA parameters
	if (p_iq_info->param_update_sel & IPE_SET_INDEP_VA) {
		if (&(p_iq_info->p_va_indep_win_info[0]) != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_va_indep_win(&(p_iq_info->p_va_indep_win_info[0]), 0);
			}
		}
		if (&(p_iq_info->p_va_indep_win_info[1]) != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_va_indep_win(&(p_iq_info->p_va_indep_win_info[1]), 1);
			}
		}
		if (&(p_iq_info->p_va_indep_win_info[2]) != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_va_indep_win(&(p_iq_info->p_va_indep_win_info[2]), 2);
			}
		}
		if (&(p_iq_info->p_va_indep_win_info[3]) != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_va_indep_win(&(p_iq_info->p_va_indep_win_info[3]), 3);
			}
		}
		if (&(p_iq_info->p_va_indep_win_info[4]) != NULL) {
			if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
				ipe_set_va_indep_win(&(p_iq_info->p_va_indep_win_info[4]), 4);
			}
		}
		//DBG_USER("change indep VA \r\n");
	}

    // edge region strength control for nt98528 only
    if (p_iq_info->param_update_sel & IPE_SET_EDGE_REGION_STR) {
        if (p_iq_info->p_edge_region_param != NULL) {
            if ((g_reg_type == IPE_ALL_REG) || (g_reg_type == IPE_FD_LATCHED)) {
                ipe_set_edge_region_strength_enable(p_iq_info->p_edge_region_param->region_str_en);

                if (p_iq_info->p_edge_region_param->region_str_en == ENABLE) {
                    ipe_set_edge_region_kernel_strength(p_iq_info->p_edge_region_param->enh_thin, p_iq_info->p_edge_region_param->enh_robust);
                    ipe_et_edge_region_flat_strength(p_iq_info->p_edge_region_param->slope_flat, p_iq_info->p_edge_region_param->str_flat);
                    ipe_et_edge_region_edge_strength(p_iq_info->p_edge_region_param->slope_edge, p_iq_info->p_edge_region_param->str_edge);
                }
            }
        }
    }


	//DBG_USER("driver func en 0x%08x\r\n", (unsigned int)p_iq_info->func_sel);

	ipe_dbg_cmd &= ~(IPE_DBG_DEFOG | IPE_DBG_EEXT);

	switch (p_iq_info->func_update_sel) {
	case IPE_FUNC_ENABLE:
		ipe_enable_feature(p_iq_info->func_sel);
		break;
	case IPE_FUNC_DISABLE:
		ipe_disable_feature(p_iq_info->func_sel);
		break;
	case IPE_FUNC_SET:
		ipe_set_feature(p_iq_info->func_sel);
		break;
	case IPE_FUNC_NOUPDATE:
	default:
		break;
	}

	ipe_set_load(g_ipe_load_type);

	er_return = ipe_state_machine(IPE_OP_CHGPARAM, UPDATE);

	return er_return;

}

/**
    IPE change interrupt

    IPE interrupt configuration.

    @param int_en Interrupt enable setting.

    @return ER IPE change interrupt status.
*/
ER ipe_change_interrupt(UINT32 int_en)
{
	ER er_return;

	er_return = ipe_state_machine(IPE_OP_CHGPARAM, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	ipe_enable_int(int_en);

	ipe_set_load(g_ipe_load_type);

	er_return = ipe_state_machine(IPE_OP_CHGPARAM, UPDATE);

	//return E_OK;
	return er_return;
}

//for FPGA verification only
ER ipe_change_interrupt_without_load(UINT32 int_en)
{
	ER er_return;

	er_return = ipe_state_machine(IPE_OP_CHGPARAM, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	ipe_enable_int(int_en);

	er_return = ipe_state_machine(IPE_OP_CHGPARAM, UPDATE);

	//return E_OK;
	return er_return;
}

ER ipe_cal_stripe(IPE_STRPINFO *p_strp_info)
{
	ER er_return = E_OK;
	STR_STRIPE_INFOR h_stripe, v_stripe;
	UINT8 h_ovlp = 2;

	switch (p_strp_info->ipe_mode) {
	case IPE_OPMODE_D2D:
		v_stripe.hn = 0;
		v_stripe.hm = 0;
		v_stripe.hl = p_strp_info->in_size_y.v_size;
		//DBG_ERR("VL = %d!\r\n", v_stripe.hl);
		if (v_stripe.hl > IPE_MAXVSTRP) {
			DBG_ERR("IPE: max height = %d!\r\n", IPE_MAXVSTRP);
			return E_SYS;
		}
		if (v_stripe.hl < IPE_MINVSTRP) {
			DBG_ERR("IPE: min height = %d!\r\n", IPE_MINVSTRP);
			return E_SYS;
		}
		if (((p_strp_info->in_fmt == IPE_YCC420) || (p_strp_info->out_fmt == IPE_YCC420)) && (p_strp_info->in_size_y.v_size % 2)) {
			v_stripe.hl = (p_strp_info->in_size_y.v_size >> 1) << 1;
			DBG_ERR("IPE: If input or output format is YCC420, height %d must be 2*N!\r\n", (int)p_strp_info->in_size_y.v_size);
		}

		//DBG_ERR("ETH %d, %d, %d, %d\r\n", p_strp_info->yc_to_dram_en,p_strp_info->dram_out_sel,p_strp_info->ethout_info.eth_outfmt,p_strp_info->ethout_info.eth_outsel);
		if ((p_strp_info->yc_to_dram_en == ENABLE) && (p_strp_info->dram_out_sel == IPE_ETH) && (p_strp_info->ethout_info.eth_outfmt == ETH_OUT_2BITS) && (p_strp_info->ethout_info.eth_outsel != ETH_OUT_ORIGINAL)) {
			h_ovlp = 4;
			//if ((p_strp_info->in_size_y.h_size % 8) > 0) {
			//  DBG_ERR("IPE: for downsampled ETH out of 2 bits, width %lu must be 8*N!\r\n", p_strp_info->in_size_y.h_size);
			//  p_strp_info->in_size_y.h_size = (p_strp_info->in_size_y.h_size >> 3) << 3;
			//  er_return = E_SYS;
			//} else {
			//  er_return = E_OK;
			//}
		}

		h_stripe.hn = default_hn;
		if (h_stripe.hn > ipe_maxhstrp) {
			h_stripe.hn = ipe_maxhstrp;
		}

		if (h_stripe.hn * 4 >= p_strp_info->in_size_y.h_size) {
			h_stripe.hn = 0;
			h_stripe.hm = 0;
			h_stripe.hl = (p_strp_info->in_size_y.h_size) / 4;
		} else {
			h_stripe.hm = (p_strp_info->in_size_y.h_size) / 4 / (h_stripe.hn - h_ovlp);
			h_stripe.hl = (p_strp_info->in_size_y.h_size) / 4 - ((h_stripe.hn - h_ovlp) * (h_stripe.hm));

			while ((h_stripe.hl < ipe_minhstrp) && (h_stripe.hn > ipe_minhstrp)) {
				h_stripe.hn--;
				h_stripe.hm = (p_strp_info->in_size_y.h_size) / 4 / (h_stripe.hn - h_ovlp);
				h_stripe.hl = (p_strp_info->in_size_y.h_size) / 4 - ((h_stripe.hn - h_ovlp) * (h_stripe.hm));
			}
		}
		if ((h_stripe.hl > ipe_maxhstrp) || (h_stripe.hn > ipe_maxhstrp)) {
			DBG_ERR("IPE: max HL/HN = %d!\r\n", ipe_maxhstrp);
			return E_SYS;
		}
		if (h_stripe.hl < ipe_minhstrp) {
			DBG_ERR("IPE: min HL = %d!\r\n", ipe_minhstrp);
			return E_SYS;
		}
		break;
	case IPE_OPMODE_IPP: //dont care
		v_stripe.hn = 0;
		v_stripe.hm = 0;
		v_stripe.hl = p_strp_info->in_size_y.v_size;

		h_stripe.hn = 0;
		h_stripe.hm = 0;
		h_stripe.hl = (p_strp_info->in_size_y.h_size) / 4;
		break;
	case IPE_OPMODE_SIE_IPP:
		v_stripe.hn = 0;
		v_stripe.hm = 0;
		v_stripe.hl = p_strp_info->in_size_y.v_size;
		if (v_stripe.hl > IPE_MAXVSTRP) {
			DBG_ERR("IPE: max height = %d!\r\n", IPE_MAXVSTRP);
			return E_SYS;
		}
		if (v_stripe.hl < IPE_MINVSTRP) {
			DBG_ERR("IPE: min height = %d!\r\n", IPE_MINVSTRP);
			return E_SYS;
		}

		if ((p_strp_info->yc_to_dram_en == ENABLE) && (p_strp_info->dram_out_sel == IPE_ETH) && (p_strp_info->ethout_info.eth_outfmt == ETH_OUT_2BITS) && (p_strp_info->ethout_info.eth_outsel != ETH_OUT_ORIGINAL)) {
			//h_ovlp = 4;
			//if ((p_strp_info->in_size_y.h_size % 8) > 0) {
			//  DBG_ERR("IPE: for downsampled ETH out of 2 bits, width %lu must be 8*N!\r\n", p_strp_info->in_size_y.h_size);
			//  p_strp_info->in_size_y.h_size = (p_strp_info->in_size_y.h_size >> 3) << 3;
			//  er_return = E_SYS;
			//} else {
			//  er_return = E_OK;
			//}
		}
		h_stripe.hn = 0;
		h_stripe.hm = 0;
		h_stripe.hl = p_strp_info->in_size_y.h_size / 4;
		if ((h_stripe.hl > ipe_maxhstrp) || (h_stripe.hn > ipe_maxhstrp)) {
			DBG_ERR("IPE: max HL/HN = %d!\r\n", ipe_maxhstrp);
			return E_SYS;
		}
		if (h_stripe.hl < ipe_minhstrp) {
			DBG_ERR("IPE: min HL/HN = %d!\r\n", ipe_minhstrp);
			return E_SYS;
		}
		break;
	default:
		DBG_ERR("IPE: illegal opreation mode %d!\r\n", p_strp_info->ipe_mode);
		return E_SYS;
		//break;
	}

	ipe_set_h_stripe(h_stripe);
	//DBG_ERR("HN = %d, HM = %d, HL = %d, hovlap = %d!\r\n", h_stripe.hn, h_stripe.hm, h_stripe.hl, h_ovlp);

	ipe_set_v_stripe(v_stripe);

	return er_return;
}


/**
    IPE change input information

    IPE input parameters configuration.

    @param ININFO Input parameters, including address, pack bit, and ping pong buffer setting.

    @return ER IPE change input status.
*/
ER ipe_change_input(IPE_INPUTINFO *p_in_info)
{
	ER er_return;
	//STR_STRIPE_INFOR h_stripe, v_stripe;
	UINT32 phy_addr0, phy_addr1, phy_addr2;
	//UINT32 addr, size;

	er_return = ipe_state_machine(IPE_OP_CHGINOUT, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	//DBG_ERR("mode = %d, size %d x %d!\r\n", p_in_info->ipe_mode, p_in_info->in_size_y.h_size, p_in_info->in_size_y.v_size);
#if 0
	switch (p_in_info->ipe_mode) {
	case IPE_OPMODE_D2D:
		v_stripe.hn = 0;
		v_stripe.hm = 0;
		v_stripe.hl = p_in_info->in_size_y.v_size;
		if (v_stripe.hl > IPE_MAXVSTRP) {
			DBG_ERR("IPE: max height = %d!\r\n", IPE_MAXVSTRP);
			return E_SYS;
		}
		if (v_stripe.hl < IPE_MINVSTRP) {
			DBG_ERR("IPE: min height = %d!\r\n", IPE_MINVSTRP);
			return E_SYS;
		}
		if (((p_in_info->in_fmt == IPE_YCC420) || (p_in_info->out_fmt == IPE_YCC420)) && (p_in_info->in_size_y.v_size % 2)) {
			v_stripe.hl = (p_in_info->in_size_y.v_size >> 1) << 1;
			DBG_ERR("IPE: If input or output format is YCC420, height %d must be 2*N!\r\n", p_in_info->in_size_y.v_size);
		}

		h_stripe.hn = default_hn;
		if (h_stripe.hn > ipe_maxhstrp) {
			h_stripe.hn = ipe_maxhstrp;
		}
		if (h_stripe.hn * 4 >= p_in_info->in_size_y.h_size) {
			h_stripe.hn = 0;
			h_stripe.hm = 0;
			h_stripe.hl = (p_in_info->in_size_y.h_size) / 4;
		} else {
			h_stripe.hm = (p_in_info->in_size_y.h_size) / 4 / (h_stripe.hn - 2);
			h_stripe.hl = (p_in_info->in_size_y.h_size) / 4 - ((h_stripe.hn - 2) * (h_stripe.hm));

			while ((h_stripe.hl < ipe_minhstrp) && (h_stripe.hn > ipe_minhstrp)) {
				h_stripe.hn--;
				h_stripe.hm = (p_in_info->in_size_y.h_size) / 4 / (h_stripe.hn - 2);
				h_stripe.hl = (p_in_info->in_size_y.h_size) / 4 - ((h_stripe.hn - 2) * (h_stripe.hm));
			}
		}
		if ((h_stripe.hl > ipe_maxhstrp) || (h_stripe.hn > ipe_maxhstrp)) {
			DBG_ERR("IPE: max HL/HN = %d!\r\n", ipe_maxhstrp);
			return E_SYS;
		}
		if (h_stripe.hl < ipe_minhstrp) {
			DBG_ERR("IPE: min HL = %d!\r\n", ipe_minhstrp);
			return E_SYS;
		}
		break;
	case IPE_OPMODE_IPP: //dont care
	case IPE_OPMODE_RDE_IPP:
	case IPE_OPMODE_RHE_IPP:
		v_stripe.hn = 0;
		v_stripe.hm = 0;
		v_stripe.hl = p_in_info->in_size_y.v_size;

		h_stripe.hn = 0;
		h_stripe.hm = 0;
		h_stripe.hl = (p_in_info->in_size_y.h_size) / 4;
		break;
	case IPE_OPMODE_SIE_IPP:
		v_stripe.hn = 0;
		v_stripe.hm = 0;
		v_stripe.hl = p_in_info->in_size_y.v_size;
		if (v_stripe.hl > IPE_MAXVSTRP) {
			DBG_ERR("IPE: max height = %d!\r\n", IPE_MAXVSTRP);
			return E_SYS;
		}
		if (v_stripe.hl < IPE_MINVSTRP) {
			DBG_ERR("IPE: min height = %d!\r\n", IPE_MINVSTRP);
			return E_SYS;
		}

		h_stripe.hn = 0;
		h_stripe.hm = 0;
		h_stripe.hl = p_in_info->in_size_y.h_size / 4;
		if ((h_stripe.hl > ipe_maxhstrp) || (h_stripe.hn > ipe_maxhstrp)) {
			DBG_ERR("IPE: max HL/HN = %d!\r\n", ipe_maxhstrp);
			return E_SYS;
		}
		if (h_stripe.hl < ipe_minhstrp) {
			DBG_ERR("IPE: min HL/HN = %d!\r\n", ipe_minhstrp);
			return E_SYS;
		}
		break;
	default:
		DBG_ERR("IPE: illegal opreation mode %d!\r\n", p_in_info->ipe_mode);
		return E_SYS;
		//break;
	}
#endif

	ipe_opmode = p_in_info->ipe_mode;
	if (ipe_opmode == IPE_OPMODE_SIE_IPP) {
		g_ipe_load_type = IPE_DIRECT_START_LOAD;
	} else {
		g_ipe_load_type = IPE_FRMEND_LOAD;
	}

	ipe_set_op_mode(p_in_info->ipe_mode);

	g_strp_info.ipe_mode = p_in_info->ipe_mode;
	g_strp_info.in_size_y = p_in_info->in_size_y;
	g_strp_info.in_fmt = p_in_info->in_fmt;
	g_strp_info.out_fmt = p_in_info->out_fmt;
	g_strp_info.yc_to_dram_en = p_in_info->yc_to_dram_en;
	g_strp_info.dram_out_sel = p_in_info->dram_out_sel;
	g_strp_info.ethout_info = p_in_info->ethout_info;
	ipe_cal_stripe(&g_strp_info);
	//ipe_set_h_stripe(h_stripe);
	//ipe_set_v_stripe(v_stripe);

	if (p_in_info->ipe_mode == IPE_OPMODE_D2D) {

#if (IPE_DMA_CACHE_HANDLE == 1)
		phy_addr0 = ipe_platform_va2pa(p_in_info->addr_in0);
		phy_addr1 = ipe_platform_va2pa(p_in_info->addr_in1);

		DBG_USER("ipe in addr0 va= 0x%08x, pa = 0x%08x\r\n", (unsigned int)p_in_info->addr_in0, (unsigned int)phy_addr0);
		DBG_USER("ipe in addr1 va= 0x%08x, pa = 0x%08x\r\n", (unsigned int)p_in_info->addr_in1, (unsigned int)phy_addr1);
#else
		phy_addr0 = p_in_info->addr_in0;
		phy_addr1 = p_in_info->addr_in1;
#endif

		ipe_set_dma_in_addr(phy_addr0, phy_addr1);
		ipe_set_in_format(p_in_info->in_fmt);
		ipe_set_dma_in_offset(p_in_info->lofs_in0, p_in_info->lofs_in1);
		ipe_set_dma_in_rand(p_in_info->in_rand_en, p_in_info->in_rand_rst);
	}

	//defog
	if (p_in_info->defog_subin_en == TRUE) {
#if (IPE_DMA_CACHE_HANDLE)
		phy_addr2 = ipe_platform_va2pa(p_in_info->addr_in_defog);
#else
		phy_addr2 = p_in_info->addr_in_defog;
#endif

		DBG_USER("ipe in addr defog va= 0x%08x, pa = 0x%08x\r\n", (unsigned int)p_in_info->addr_in_defog, (unsigned int)phy_addr2);

		ipe_set_dma_in_defog_addr(phy_addr2);
		ipe_set_dma_in_defog_offset(p_in_info->lofs_in_defog);
	}

	ipe_set_dma_outsel(p_in_info->dram_out_sel);

	ipe_set_hovlp_opt(p_in_info->hovlp_opt);

	ipe_set_load(g_ipe_load_type);

	er_return = ipe_state_machine(IPE_OP_CHGINOUT, UPDATE);

	//return E_OK;
	return er_return;
}

/**
    IPE change output information

    IPE output parameters configuration.

    @param OUTINFO Output parameters, including address and YCbCr format.

    @return ER IPE change input status.
*/
ER ipe_change_output(IPE_OUTYCINFO *p_out_info)
{
	ER er_return;
	UINT32 phy_addr0, phy_addr1, phy_addr2;
	UINT32 i = 0;

	er_return = ipe_state_machine(IPE_OP_CHGINOUT, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	if ((p_out_info->yc_to_ime_en == 0) && (p_out_info->yc_to_dram_en == 0) && (p_out_info->va_to_dram_en == 0)) {
		DBG_ERR("ipe_change_output() output destination error!\r\n");
		return E_SYS;
	}

	//initialize DMA out struct
	for (i = 0; i < IPE_IO_MAX ; i++) {
		g_ipe_dramio[i].enable = 0;
		g_ipe_dramio[i].addr = 0;
		g_ipe_dramio[i].size = 0;
	}

	ipe_set_output_switch(p_out_info->yc_to_ime_en, p_out_info->yc_to_dram_en);
	ipe_set_dram_out_mode(p_out_info->dram_out_mode);
	ipe_set_dram_single_channel_en(p_out_info->single_out_en);

	if (p_out_info->yc_to_dram_en == TRUE) {
		ipe_set_out_format(p_out_info->yc_format, p_out_info->sub_h_sel);
		if ((g_cur_dma_outsel <= IPE_ETH) && (g_cur_dma_outsel != p_out_info->dram_out_sel)) {
			DBG_WRN("DMA out selection is changed from %d to %d!\r\n", (int)g_cur_dma_outsel, (int)p_out_info->dram_out_sel);
		}
		ipe_set_dma_outsel(p_out_info->dram_out_sel);
		ipe_set_eth_size(&(p_out_info->ethout_info));
		ipe_set_dma_out_offset_y(p_out_info->lofs_y);
		ipe_set_dma_out_offset_c(p_out_info->lofs_c);

		//Check DRAM output address
		switch (p_out_info->dram_out_sel) {
		case IPE_ETH:
			switch (p_out_info->ethout_info.eth_outsel) {
			case ETH_OUT_ORIGINAL:
				g_ipe_dramio[IPE_OUT_Y].enable = 0;
				g_ipe_dramio[IPE_OUT_C].enable = 1;
				if (p_out_info->addr_c == 0) {
					DBG_ERR("ipe_change_output() SA1 = 0x0!(IPE output C)\r\n");
				}
				break;
			case ETH_OUT_DOWNSAMPLED:
				g_ipe_dramio[IPE_OUT_Y].enable = 1;
				g_ipe_dramio[IPE_OUT_C].enable = 0;
				if (p_out_info->addr_y == 0) {
					DBG_ERR("ipe_change_output() SAO = 0x0!(IPE output Y)\r\n");
				}
				break;
			case ETH_OUT_BOTH:
			default:
				g_ipe_dramio[IPE_OUT_Y].enable = 1;
				g_ipe_dramio[IPE_OUT_C].enable = 1;
				if ((p_out_info->addr_y == 0) || (p_out_info->addr_c == 0)) {
					DBG_ERR("ipe_change_output() SAO or SA1 = 0x0!(IPE output Y/C)\r\n");
				}
				break;
			}
			break;
		case IPE_REGION:
			g_ipe_dramio[IPE_OUT_Y].enable = 0;
			g_ipe_dramio[IPE_OUT_C].enable = 1;
			if (p_out_info->addr_c == 0) {
				DBG_ERR("ipe_change_output() SA1 = 0x0!(IPE output C)\r\n");
			}
			break;
		case IPE_ORIGINAL:
		default:
			if (p_out_info->yc_format == IPE_YONLY) {
				g_ipe_dramio[IPE_OUT_Y].enable = 1;
				g_ipe_dramio[IPE_OUT_C].enable = 0;
				if (p_out_info->addr_y == 0) {
					DBG_ERR("ipe_change_output() SAO = 0x0!(IPE YONLY)\r\n");
				}
			} else {
				g_ipe_dramio[IPE_OUT_Y].enable = 1;
				g_ipe_dramio[IPE_OUT_C].enable = 1;
				if ((p_out_info->addr_y == 0) || (p_out_info->addr_c == 0)) {
					DBG_ERR("ipe_change_output() SAO = 0x0!(IPE YC out)\r\n");
				}
			}
			break;
		}

		if (p_out_info->dram_out_mode == IPE_DRAM_OUT_SINGLE) {
			g_ipe_dramio[IPE_OUT_Y].enable  = (g_ipe_dramio[IPE_OUT_Y].enable  && p_out_info->single_out_en.single_out_y_en);
			g_ipe_dramio[IPE_OUT_C].enable = (g_ipe_dramio[IPE_OUT_C].enable && p_out_info->single_out_en.single_out_c_en);
		}

#if (IPE_DMA_CACHE_HANDLE)
		if (g_ipe_dramio[IPE_OUT_Y].enable) {
			phy_addr0 = ipe_platform_va2pa(p_out_info->addr_y);
			ipe_set_dma_out_addr_y(phy_addr0);
		}
		if (g_ipe_dramio[IPE_OUT_C].enable) {
			phy_addr1 = ipe_platform_va2pa(p_out_info->addr_c);
			ipe_set_dma_out_addr_c(phy_addr1);
		}
#else
		if (g_ipe_dramio[IPE_OUT_Y].enable) {
			phy_addr0 = p_out_info->addr_y;
			ipe_set_dma_out_addr_y(phy_addr0);
		}
		if (g_ipe_dramio[IPE_OUT_C].enable) {
			phy_addr1 = p_out_info->addr_c;
			ipe_set_dma_out_addr_c(phy_addr1);
		}
#endif

	}

	//VA
	if (p_out_info->dram_out_mode == IPE_DRAM_OUT_SINGLE) {
		g_ipe_dramio[IPE_OUT_VA].enable = (p_out_info->va_to_dram_en && p_out_info->single_out_en.single_out_va_en);
	} else {
		g_ipe_dramio[IPE_OUT_VA].enable = p_out_info->va_to_dram_en;
	}
	va_ring_setting[0].va_en = g_ipe_dramio[IPE_OUT_VA].enable;
	if (g_ipe_dramio[IPE_OUT_VA].enable == TRUE) {

		va_ring_setting[0].outsel = p_out_info->va_outsel;
		va_ring_setting[0].address = p_out_info->addr_va;
		va_ring_setting[0].lineoffset = p_out_info->lofs_va;

		ipe_set_va_out_sel(p_out_info->va_outsel);
		ipe_set_dma_out_va_offset(p_out_info->lofs_va);

		if (p_out_info->addr_va == 0) {
			DBG_ERR("ipe_change_output() SAO = 0x0!(IPE output VA)\r\n");
		} else {
#if (IPE_DMA_CACHE_HANDLE)
			phy_addr2 = ipe_platform_va2pa(p_out_info->addr_va);
#else
			phy_addr2 = p_out_info->addr_va;
#endif
			ipe_set_dma_out_va_addr(phy_addr2);
		}
	}

	//defog
	if (p_out_info->dram_out_mode == IPE_DRAM_OUT_SINGLE) {
		g_ipe_dramio[IPE_OUT_SUBIMG].enable = (p_out_info->defog_to_dram_en && p_out_info->single_out_en.single_out_defog_en);
	} else {
		g_ipe_dramio[IPE_OUT_SUBIMG].enable = p_out_info->defog_to_dram_en;
	}
	if (g_ipe_dramio[IPE_OUT_SUBIMG].enable == TRUE) {
		ipe_set_dma_out_defog_offset(p_out_info->lofs_defog);
		if (p_out_info->addr_defog == 0) {
			DBG_ERR("ipe_change_output() SAO = 0x0!(IPE output defog)\r\n");
		} else {
#if (IPE_DMA_CACHE_HANDLE)
			phy_addr2 = ipe_platform_va2pa(p_out_info->addr_defog);
#else
			phy_addr2 = p_out_info->addr_defog;
#endif
			ipe_set_dma_out_defog_addr(phy_addr2);
		}
	}

	g_strp_info.yc_to_dram_en = p_out_info->yc_to_dram_en;
	g_strp_info.dram_out_sel = p_out_info->dram_out_sel;
	g_strp_info.ethout_info = p_out_info->ethout_info;
	ipe_cal_stripe(&g_strp_info);

	ipe_set_load(g_ipe_load_type);

	er_return = ipe_state_machine(IPE_OP_CHGINOUT, UPDATE);

	//return E_OK;
	return er_return;
}

/**
    IPE flush cache buffer

    IPE flush DRAM In/Out cache buffers.

    @param p_mode_info IPE mode information, including size, input, output, and IQ.

    @return None
*/
void ipe_flush_cache(IPE_MODEINFO *p_mode_info)
{
#if (IPE_DMA_CACHE_HANDLE)
	UINT32 addr0, size0 ;
	UINT32 width_y;

	if (p_mode_info->in_info.ipe_mode == IPE_OPMODE_D2D) {

		//DBG_IND("ipe_flush_cache() IPE DRAM from DRAM, cache flushed!\r\n");
		//Input Y, C
		// Y address
		addr0 = p_mode_info->in_info.addr_in0;
		size0 = IPE_ALIGN_CEIL_32(p_mode_info->in_info.lofs_in0 * p_mode_info->in_info.in_size_y.v_size);
		if (addr0 != 0) {
			ipe_platform_dma_flush_mem2dev(addr0, size0);
		}

		// C address
		addr0 = p_mode_info->in_info.addr_in1;
		switch (p_mode_info->in_info.in_fmt) {
		case IPE_YCC444:
		case IPE_YCC422:
			size0 = IPE_ALIGN_CEIL_32(p_mode_info->in_info.lofs_in1 * p_mode_info->in_info.in_size_y.v_size);
			if (addr0 != 0) {
				ipe_platform_dma_flush_mem2dev(addr0, size0);
			}
			break;
		case IPE_YCC420:
			size0 = IPE_ALIGN_CEIL_32(p_mode_info->in_info.lofs_in1 * p_mode_info->in_info.in_size_y.v_size / 2);
			if (addr0 != 0) {
				ipe_platform_dma_flush_mem2dev(addr0, size0);
			}
			break;
		case IPE_YONLY:
		default:
			break;
		}
	}

	if (p_mode_info->in_info.defog_subin_en == TRUE) {
		//DBG_IND("ipe_flush_cache() IPE defog subin from DRAM, cache flushed!\r\n");
		addr0 = p_mode_info->in_info.addr_in_defog;
		size0 = IPE_ALIGN_CEIL_32(p_mode_info->in_info.lofs_in_defog * p_mode_info->iq_info.p_dfg_subimg_param->dfg_subimg_size.v_size);
		if (addr0 != 0) {
			ipe_platform_dma_flush_mem2dev(addr0, size0);
		}
	}

#if 0
	// flush in ipe_change_param()
	// Y curve address
	addr0 = p_mode_info->iq_info.p_ycurve_param->addr_ycurve;
	size0 = IPE_DRAM_YCURVE_BYTE;
	if (addr0 != 0) {
		dma_flushWriteCache(addr0, size0);
	}

	// Gamma address
	addr0 = p_mode_info->iq_info.addr_gamma;
	size0 = IPE_DRAM_GAMMA_BYTE;
	if (addr0 != 0) {
		dma_flushWriteCache(addr0, size0);
	}
#endif

	//Output
	width_y = p_mode_info->in_info.in_size_y.h_size; //p_mode_info->out_info.uiOutYSizeH;
	if (p_mode_info->out_info.yc_to_dram_en == ENABLE) {
		switch (p_mode_info->out_info.dram_out_sel) {
		case IPE_ORIGINAL:
			//Y address
			g_ipe_dramio[IPE_OUT_Y].addr = p_mode_info->out_info.addr_y;
			g_ipe_dramio[IPE_OUT_Y].size = IPE_ALIGN_CEIL_32(p_mode_info->out_info.lofs_y * p_mode_info->in_info.in_size_y.v_size);
			if (g_ipe_dramio[IPE_OUT_Y].enable && (g_ipe_dramio[IPE_OUT_Y].addr != 0)) {
				if (width_y == p_mode_info->out_info.lofs_y) {
					ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_Y].addr, g_ipe_dramio[IPE_OUT_Y].size);
				} else {
					ipe_platform_dma_flush_dev2mem_width_neq_lofs(g_ipe_dramio[IPE_OUT_Y].addr, g_ipe_dramio[IPE_OUT_Y].size);
				}
			}
			//CbCr address
			g_ipe_dramio[IPE_OUT_C].addr = p_mode_info->out_info.addr_c;
			if (g_ipe_dramio[IPE_OUT_C].enable == 1) {
				switch (p_mode_info->out_info.yc_format) {
				case IPE_YCC444:
					g_ipe_dramio[IPE_OUT_C].size = IPE_ALIGN_CEIL_32(p_mode_info->out_info.lofs_c * p_mode_info->in_info.in_size_y.v_size);
					if (g_ipe_dramio[IPE_OUT_C].addr != 0) {
						if ((width_y * 2) == p_mode_info->out_info.lofs_c) {
							ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_C].addr, g_ipe_dramio[IPE_OUT_C].size);
						} else {
							ipe_platform_dma_flush_dev2mem_width_neq_lofs(g_ipe_dramio[IPE_OUT_C].addr, g_ipe_dramio[IPE_OUT_C].size);
						}
					}
					break;
				case IPE_YCC422:
					g_ipe_dramio[IPE_OUT_C].size = IPE_ALIGN_CEIL_32(p_mode_info->out_info.lofs_c * p_mode_info->in_info.in_size_y.v_size);
					if (g_ipe_dramio[IPE_OUT_C].addr != 0) {
						if (width_y == p_mode_info->out_info.lofs_c) {
							ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_C].addr, g_ipe_dramio[IPE_OUT_C].size);
						} else {
							ipe_platform_dma_flush_dev2mem_width_neq_lofs(g_ipe_dramio[IPE_OUT_C].addr, g_ipe_dramio[IPE_OUT_C].size);
						}
					}
					break;
				case IPE_YCC420:
					g_ipe_dramio[IPE_OUT_C].size = IPE_ALIGN_CEIL_32(p_mode_info->out_info.lofs_c * p_mode_info->in_info.in_size_y.v_size / 2);
					if (g_ipe_dramio[IPE_OUT_C].addr != 0) {
						if (width_y == p_mode_info->out_info.lofs_c) {
							ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_C].addr, g_ipe_dramio[IPE_OUT_C].size);
						} else {
							ipe_platform_dma_flush_dev2mem_width_neq_lofs(g_ipe_dramio[IPE_OUT_C].addr, g_ipe_dramio[IPE_OUT_C].size);
						}
					}
					break;
				default:
					break;
				}
			}
			//DBG_ERR("[YC] in %d x %d\r\n", p_mode_info->in_info.in_size_y.h_size, p_mode_info->in_info.in_size_y.v_size);
			//DBG_ERR("[YC] fmt %d, outw %d, lofsoY %d C %d, out size %d\r\n", p_mode_info->out_info.yc_format, width_y, p_mode_info->out_info.lofs_y, p_mode_info->out_info.lofs_c, size0);
			break;
		case IPE_ETH:
			//Y address
			g_ipe_dramio[IPE_OUT_Y].addr = p_mode_info->out_info.addr_y; //downsampled ETH
			g_ipe_dramio[IPE_OUT_C].addr  = p_mode_info->out_info.addr_c; //original ETH
			g_ipe_dramio[IPE_OUT_Y].size = 0;
			g_ipe_dramio[IPE_OUT_C].size = 0;
			//if (p_mode_info->out_info.ethout_info.eth_outfmt == 0) {
			//2 bits
			//  width_y = width_y >> 2;
			//}

			if (p_mode_info->out_info.ethout_info.eth_outsel == 1 || p_mode_info->out_info.ethout_info.eth_outsel == 2) {
				//downsampled eth
				g_ipe_dramio[IPE_OUT_Y].size = IPE_ALIGN_CEIL_32(p_mode_info->out_info.lofs_y * p_mode_info->in_info.in_size_y.v_size >> 1);
			}

			if (p_mode_info->out_info.ethout_info.eth_outsel == 0 || p_mode_info->out_info.ethout_info.eth_outsel == 2) {
				g_ipe_dramio[IPE_OUT_C].size = IPE_ALIGN_CEIL_32(p_mode_info->out_info.lofs_c * p_mode_info->in_info.in_size_y.v_size);
			}

			if (g_ipe_dramio[IPE_OUT_Y].enable && (g_ipe_dramio[IPE_OUT_Y].addr != 0)) {
				ipe_platform_dma_flush_dev2mem_video_mode(g_ipe_dramio[IPE_OUT_Y].addr, g_ipe_dramio[IPE_OUT_Y].size);
			}
			if (g_ipe_dramio[IPE_OUT_C].enable && (g_ipe_dramio[IPE_OUT_C].addr != 0)) {
				ipe_platform_dma_flush_dev2mem_video_mode(g_ipe_dramio[IPE_OUT_C].addr, g_ipe_dramio[IPE_OUT_C].size);

			}
			//DBG_ERR("[ETH] in %d x %d, outw %d lofso %d, out size %d\r\n", p_mode_info->in_info.in_size_y.h_size, p_mode_info->in_info.in_size_y.v_size,
			//    width_y, p_mode_info->out_info.lofs_y, size0);
			break;
		case IPE_REGION:
			//C address
			g_ipe_dramio[IPE_OUT_C].addr = p_mode_info->out_info.addr_c;
			g_ipe_dramio[IPE_OUT_C].size = IPE_ALIGN_CEIL_32(p_mode_info->out_info.lofs_c * p_mode_info->in_info.in_size_y.v_size); // 2bits/pixel
			if (g_ipe_dramio[IPE_OUT_C].enable && (g_ipe_dramio[IPE_OUT_C].addr != 0)) {
				if ((width_y / 4) == p_mode_info->out_info.lofs_c) {
					ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_C].addr, g_ipe_dramio[IPE_OUT_C].size);
				} else {
					ipe_platform_dma_flush_dev2mem_width_neq_lofs(g_ipe_dramio[IPE_OUT_C].addr, g_ipe_dramio[IPE_OUT_C].size);
				}
			}
			break;
		default:
			break;
		}
	}

	//Output VA
	if (g_ipe_dramio[IPE_OUT_VA].enable == TRUE) {

		g_ipe_dramio[IPE_OUT_VA].addr = p_mode_info->out_info.addr_va;
		g_ipe_dramio[IPE_OUT_VA].size = IPE_ALIGN_CEIL_32(p_mode_info->out_info.lofs_va * p_mode_info->iq_info.p_va_win_info->win_numy);

		width_y = p_mode_info->iq_info.p_va_win_info->win_numx; //p_mode_info->out_info.uiOutYSizeH;

		if (g_ipe_dramio[IPE_OUT_VA].addr != 0) {
			if ((width_y) == p_mode_info->out_info.lofs_va) {
				// NOTE: XXXXX
				ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_VA].addr, g_ipe_dramio[IPE_OUT_VA].size);
			} else {
				ipe_platform_dma_flush_dev2mem_width_neq_lofs(g_ipe_dramio[IPE_OUT_VA].addr, g_ipe_dramio[IPE_OUT_VA].size);
			}
		}
	}

	//Output defog
	if (g_ipe_dramio[IPE_OUT_SUBIMG].enable == TRUE) {

		g_ipe_dramio[IPE_OUT_SUBIMG].addr = p_mode_info->out_info.addr_defog;
		g_ipe_dramio[IPE_OUT_SUBIMG].size = IPE_ALIGN_CEIL_32(p_mode_info->out_info.lofs_defog * p_mode_info->iq_info.p_dfg_subimg_param->dfg_subimg_size.v_size);

		width_y = p_mode_info->iq_info.p_dfg_subimg_param->dfg_subimg_size.h_size; //p_mode_info->out_info.uiOutYSizeH;

		if (g_ipe_dramio[IPE_OUT_SUBIMG].addr != 0) {
			if ((width_y) == p_mode_info->out_info.lofs_defog) {
				ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_SUBIMG].addr, g_ipe_dramio[IPE_OUT_SUBIMG].size);
			} else {
				ipe_platform_dma_flush_dev2mem_width_neq_lofs(g_ipe_dramio[IPE_OUT_SUBIMG].addr, g_ipe_dramio[IPE_OUT_SUBIMG].size);
			}
		}
	}
#endif
}


#if (IPE_DMA_CACHE_HANDLE)
void ipe_flush_cache_end(void)
{
	if (g_ipe_dramio[IPE_OUT_Y].enable == 1) {
		ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_Y].addr, g_ipe_dramio[IPE_OUT_Y].size);
	}

	if (g_ipe_dramio[IPE_OUT_C].enable == 1) {
		ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_C].addr, g_ipe_dramio[IPE_OUT_C].size);
	}

	if (g_ipe_dramio[IPE_OUT_VA].enable == 1) {

		ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_VA].addr, g_ipe_dramio[IPE_OUT_VA].size);
	}

	if (g_ipe_dramio[IPE_OUT_SUBIMG].enable == 1) {
		ipe_platform_dma_flush_dev2mem(g_ipe_dramio[IPE_OUT_SUBIMG].addr, g_ipe_dramio[IPE_OUT_SUBIMG].size);
	}
}
#endif
//------------------------------------------------------------


VOID ipe_set_builtin_flow_disable(VOID)
{
	ipe_ctrl_flow_to_do = ENABLE;
}
//------------------------------------------------------------


/**
    IPE set mode

    Initial setup for IPE prv/cap/vdo mode operation

    @param p_mode_info IPE mode information, including size, input, output, and IQ.

    @return ER IPE set mode status.
*/
ER ipe_set_mode(IPE_MODEINFO *p_mode_info)
{
	ER er_return;

#if 0
#if (IPE_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ipe_power_saving_en == TRUE) {
		if (ipe_opmode != IPE_OPMODE_SIE_IPP) {
			ipe_platform_disable_sram_shutdown();
			ipe_attach(); //enable engine clock
		}
	}
#endif
#endif

	er_return = ipe_state_machine(IPE_OP_SETMODE, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}


	if (ipe_ctrl_flow_to_do == DISABLE) {
	    ipe_opmode = p_mode_info->in_info.ipe_mode;
		goto SKIP_PARAMS;
	}

	//DBG_USER("Check parameter \r\n");
	er_return = ipe_check_param(p_mode_info);
	if (er_return != E_OK) {
		DBG_ERR("IPE: Parameter Check fail!\r\n");
		return er_return;
	}

	//DBG_USER("change input \r\n");
	er_return = ipe_change_input(&p_mode_info->in_info);
	if (er_return != E_OK) {
		DBG_ERR("IPE: Input setting fail!\r\n");
		return er_return;
	}

	//DBG_USER("change output \r\n");
	er_return = ipe_change_output(&p_mode_info->out_info);
	if (er_return != E_OK) {
		DBG_ERR("IPE: Output setting fail!\r\n");
		return er_return;
	}

	g_reg_type = IPE_ALL_REG;
	//DBG_USER("change parameters \r\n");
	er_return = ipe_change_param(&p_mode_info->iq_info);
	if (er_return != E_OK) {
		DBG_ERR("IPE: Parameter change fail!\r\n");
		return er_return;
	}

#if 0
	er_return = ipe_change_interrupt(p_mode_info->intr_en);
	if (er_return != E_OK) {
		DBG_ERR("IPE: Interrupt Change fail!\r\n");
		return er_return;
	}

	ipe_clear_int(IPE_INT_ALL);
	//ipe_reset_on();
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ipe_ctrl_flow_to_do == ENABLE) {
		er_return = ipe_change_interrupt(p_mode_info->intr_en);
		if (er_return != E_OK) {
			DBG_ERR("IPE: Interrupt Change fail!\r\n");
			return er_return;
		}

		ipe_clear_int(IPE_INT_ALL);
		//ipe_reset_on();
	} else {
		if (ipe_opmode != IPE_OPMODE_SIE_IPP) {
			er_return = ipe_change_interrupt(p_mode_info->intr_en);
			if (er_return != E_OK) {
				DBG_ERR("IPE: Interrupt Change fail!\r\n");
				return er_return;
			}

			ipe_clear_int(IPE_INT_ALL);
			//ipe_reset_on();
		}
	}
#else
	er_return = ipe_change_interrupt(p_mode_info->intr_en);
	if (er_return != E_OK) {
		DBG_ERR("IPE: Interrupt Change fail!\r\n");
		return er_return;
	}

	ipe_clear_int(IPE_INT_ALL);
	//ipe_reset_on();
#endif


#if (IPE_DMA_CACHE_HANDLE)
	ipe_flush_cache(p_mode_info);
#endif

	er_return = ipe_state_machine(IPE_OP_SETMODE, UPDATE);

	return er_return;


SKIP_PARAMS:  // for fastboot control flow
	er_return = ipe_state_machine(IPE_OP_SETMODE, UPDATE);

	return er_return;

	//return E_OK;
}

#if 0
/**
    IPE Change all parameters at runtime

    Setup for IPE prv/cap/vdo mode operation

    @param p_mode_info IPE mode information, including size, input, output, and IQ.

    @return ER IPE set mode status.
*/
ER ipe_change_all(IPE_MODEINFO *p_mode_info, IPE_REGTYPE reg_type)
{
	ER er_return;

	er_return = ipe_state_machine(IPE_OP_CHGPARAMALL, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}


	er_return = ipe_change_input(&p_mode_info->in_info);
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = ipe_change_output(&p_mode_info->out_info);
	if (er_return != E_OK) {
		return er_return;
	}

	g_reg_type = reg_type;
	er_return = ipe_change_param(&p_mode_info->iq_info);
	g_reg_type = IPE_ALL_REG;
	if (er_return != E_OK) {
		return er_return;
	}

	er_return = ipe_change_interrupt(p_mode_info->intr_en);
	if (er_return != E_OK) {
		return er_return;
	}

#if (IPE_DMA_CACHE_HANDLE)
	ipe_flush_cache(p_mode_info);
#endif


	er_return = ipe_state_machine(IPE_OP_CHGPARAMALL, UPDATE);

	//return E_OK;
	return er_return;
}
#endif

ER ipe_start_linkedlist(void)
{
#if (defined(_NVT_EMULATION_) == OFF)

	ER      er_return;

	ipe_state_machine(IPE_OP_SETMODE, UPDATE);
	er_return = ipe_state_machine(IPE_OP_START, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

#if (IPE_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ipe_power_saving_en == TRUE) {
		if (ipe_opmode != IPE_OPMODE_SIE_IPP) {
			ipe_platform_disable_sram_shutdown();
			ipe_attach(); //enable engine clock
		}
	}
#endif

	ipe_clr_frame_end();
	ipe_clr_dma_end();

	//ipe_set_load(IPE_START_LOAD);
	//ipe_set_start();
	ipe_set_ll_fire();

	er_return = ipe_state_machine(IPE_OP_START, UPDATE);

	//return E_OK;
	return er_return;
#else

	DBG_ERR("ipe linked-list fire\r\n");
	ipe_set_ll_fire();

	return E_OK;
#endif
}


/**
    IPE start

    Clear IPE flag and trigger start

    @param None.

    @return ER IPE start status.
*/
ER ipe_start(void)
{
	ER er_return;

	er_return = ipe_state_machine(IPE_OP_START, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

#if (IPE_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ipe_power_saving_en == TRUE) {
		if (ipe_opmode != IPE_OPMODE_SIE_IPP) {
			ipe_platform_disable_sram_shutdown();
			ipe_attach(); //enable engine clock
		}
	}
#endif

#if 0
	ipe_clr_frame_end();
	ipe_clr_dma_end();

	//Update state-machine before load & start for multi-thread processing
	er_return = ipe_state_machine(IPE_OP_START, UPDATE);

	ipe_set_load(IPE_START_LOAD);
	ipe_set_start();
#endif

#if defined (__KERNEL__)
	//if (kdrv_builtin_is_fastboot() == DISABLE) {
	if (ipe_ctrl_flow_to_do == ENABLE) {
		ipe_clr_frame_end();
		ipe_clr_dma_end();

		//Update state-machine before load & start for multi-thread processing
		er_return = ipe_state_machine(IPE_OP_START, UPDATE);

		ipe_set_load(IPE_START_LOAD);
		ipe_set_start();
	} else {
		if (ipe_opmode == IPE_OPMODE_SIE_IPP) {
			//Update state-machine before load & start for multi-thread processing
			er_return = ipe_state_machine(IPE_OP_START, UPDATE);
		} else {
			ipe_clr_frame_end();
			ipe_clr_dma_end();

			//Update state-machine before load & start for multi-thread processing
			er_return = ipe_state_machine(IPE_OP_START, UPDATE);

			ipe_set_load(IPE_START_LOAD);
			ipe_set_start();
		}
	}
#else
	ipe_clr_frame_end();
	ipe_clr_dma_end();

	//Update state-machine before load & start for multi-thread processing
	er_return = ipe_state_machine(IPE_OP_START, UPDATE);

	ipe_set_load(IPE_START_LOAD);
	ipe_set_start();
#endif

	ipe_ctrl_flow_to_do = ENABLE;

#if (defined(_NVT_EMULATION_) == ON)
	ipe_end_time_out_status = FALSE;
#endif

	//return E_OK;
	return er_return;
}

//for FPGA verification only
ER ipe_start_without_load(void)
{
	ER      er_return;

	er_return = ipe_state_machine(IPE_OP_START, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

#if (IPE_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ipe_power_saving_en == TRUE) {
		if (ipe_opmode != IPE_OPMODE_SIE_IPP) {
			ipe_platform_disable_sram_shutdown();
			ipe_attach(); //enable engine clock
		}
	}
#endif

	ipe_clr_frame_end();
	ipe_clr_dma_end();

	ipe_set_start();

	er_return = ipe_state_machine(IPE_OP_START, UPDATE);

	//return E_OK;
	return er_return;
}
//for FPGA verification only
ER ipe_start_with_frmend_load(void)
{
	ER      er_return;

	er_return = ipe_state_machine(IPE_OP_START, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

#if (IPE_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ipe_power_saving_en == TRUE) {
		if (ipe_opmode != IPE_OPMODE_SIE_IPP) {
			ipe_platform_disable_sram_shutdown();
			ipe_attach(); //enable engine clock
		}
	}
#endif

	ipe_clr_frame_end();
	ipe_clr_dma_end();

	ipe_set_start_frmend_load();

	er_return = ipe_state_machine(IPE_OP_START, UPDATE);

	//return E_OK;
	return er_return;
}


/**
    IPE pause

    Pause IPE operation

    @param None.

    @return ER IPE pause status.
*/
ER ipe_pause(void)
{
	ER      er_return;

	er_return = ipe_state_machine(IPE_OP_PAUSE, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	ipe_set_stop();

#if (IPE_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ipe_power_saving_en == TRUE) {
		if (ipe_opmode != IPE_OPMODE_SIE_IPP) {
			ipe_platform_enable_sram_shutdown();
			ipe_detach(); //disable engine clock
		}
	}
#endif

	er_return = ipe_state_machine(IPE_OP_PAUSE, UPDATE);

	//return E_OK;
	return er_return;
}

/**
    IPE reset

    Enable/disable IPE SW reset

    @param reset_en TRUE : enable reset, FALSE : disable reset.

    @return ER IPE reset status.
*/
ER ipe_reset(BOOL reset_en)
{
	ER      er_return;

	er_return = ipe_state_machine(IPE_OP_HWRESET, NOTUPDATE);
	if (er_return != E_OK) {
		ipe_unlock();
		return er_return;
	}

	ipe_set_stop();

	if (ipe_platform_is_err_clk()) {
		DBG_ERR("failed to get ipe clock\n");
		return E_PAR;
	} else {
		ipe_detach();
		ipe_platform_unprepare_clk();
	}
	ipe_platform_prepare_clk();
	ipe_attach();

	ipe_platform_flg_clear(FLGPTN_IPE_FRAMEEND);

	// Clear engine interrupt status
	ipe_clear_int(IPE_INTE_ALL);

	if (reset_en) {
		ipe_reset_on();
	} else {
		ipe_reset_off();
	}

	// Clear engine interrupt status again, prevent HW issue
	ipe_clear_int(IPE_INTE_ALL);  // 98520: fixed defog subout dummy interrupt

	er_return = ipe_state_machine(IPE_OP_HWRESET, UPDATE);

	//return E_OK;
	return er_return;
}

ER ipe_load_gamma(IPE_RWGAM_OPT opt)
{
	ER er_return;

#if (IPE_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ipe_power_saving_en == TRUE) {
		if (ipe_opmode != IPE_OPMODE_SIE_IPP) {
			ipe_platform_disable_sram_shutdown();
			ipe_attach(); //enable engine clock
		}
	}
#endif

	er_return = ipe_state_machine(IPE_OP_LOADLUT, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	ipe_set_gamma_rw_en(DMA_WRITE_LUT, opt);

	er_return = ipe_state_machine(IPE_OP_LOADLUT, UPDATE);

	//return E_OK;
	return er_return;
}

ER ipe_read_gamma(BOOL cpu_read_en)
{
	ER er_return;

#if (IPE_POWER_SAVING_EN == 1)
	//for power saving
	if (fw_ipe_power_saving_en == TRUE) {
		if (ipe_opmode != IPE_OPMODE_SIE_IPP) {
			ipe_platform_disable_sram_shutdown();
			ipe_attach(); //enable engine clock
		}
	}
#endif

	er_return = ipe_state_machine(IPE_OP_READLUT, NOTUPDATE);
	if (er_return != E_OK) {
		return er_return;
	}

	if (cpu_read_en) {
		ipe_set_gamma_rw_en(CPU_READ_LUT, LOAD_GAMMA_R);
	} else {
		ipe_set_gamma_rw_en(IPE_READ_LUT, LOAD_GAMMA_R);
	}

	er_return = ipe_state_machine(IPE_OP_READLUT, UPDATE);

	//return E_OK;
	return er_return;
}

//--------------------------------------------------------------------------------------------------------------------------

ER ipe_stop_linkedlist(void)
{
	ipe_set_ll_terminate();

	return E_OK;
}


#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)
/**
    IPE reset on

    Set IPE reset bit to 1.

    @param None.

    @return None.
*/
void ipe_reset_on(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_swrst = ENABLE;

	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
    IPE reset off

    Set IPE reset bit to 0.

    @param None.

    @return None.
*/
void ipe_reset_off(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_swrst = DISABLE;

	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
    IPE set linked list fire

    Set IPE linked list fire bit to 1.

    @param None.

    @return None.
*/
void ipe_set_ll_fire(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ll_fire = ENABLE;

	local_reg.bit.reg_ipe_start = DISABLE;
	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

void ipe_set_ll_terminate(void)
{
	T_LINKED_LIST_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(LINKED_LIST_REGISTER_OFS);
	local_reg.bit.reg_ll_terminate = ENABLE;

	IPE_SETREG(LINKED_LIST_REGISTER_OFS, local_reg.reg);
}

/**
    IPE set start

    Set IPE start bit to 1.

    @param None.

    @return None.
*/
void ipe_set_start(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_start = ENABLE;

	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

//for FPGA verification only
void ipe_set_start_frmend_load(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_start = ENABLE;

	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_fd = ENABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;


	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}


/**
    IPE set stop

    Set IPE reset bit to 0.

    @param None.

    @return None.
*/
void ipe_set_stop(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_start = DISABLE;

	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
    IPE Set load

    Set IPE one of the load bits to 1.

    @param type Load type.

    @return None.
*/
void ipe_set_load(IPE_LOAD_TYPE type)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	switch (type) {
	case IPE_START_LOAD:
		local_reg.bit.reg_ipe_load_start = ENABLE;
		//HW clear load, force write zero to prevent multiple loads
		local_reg.bit.reg_ipe_load_fd = DISABLE;
		local_reg.bit.reg_ipe_load_fs = DISABLE;
		break;
	case IPE_FRMEND_LOAD:
		if (g_ipe_status == IPE_STATUS_RUN) {
			local_reg.bit.reg_ipe_load_fd = ENABLE;
			//HW clear load, force write zero to prevent multiple loads
			local_reg.bit.reg_ipe_load_start = DISABLE;
			local_reg.bit.reg_ipe_load_fs = DISABLE;
		} else {
			local_reg.bit.reg_ipe_load_fd = DISABLE;
			local_reg.bit.reg_ipe_load_start = DISABLE;
			local_reg.bit.reg_ipe_load_fs = DISABLE;
		}
		break;
	case IPE_DIRECT_START_LOAD:
		local_reg.bit.reg_ipe_load_fs = ENABLE;
		//HW clear load, force write zero to prevent multiple loads
		local_reg.bit.reg_ipe_load_fd = DISABLE;
		local_reg.bit.reg_ipe_load_start = DISABLE;
		break;
	default:
		DBG_ERR("ipe_set_load() load type error!\r\n");
		//HW clear load, force write zero to prevent multiple loads
		local_reg.bit.reg_ipe_load_fd = DISABLE;
		local_reg.bit.reg_ipe_load_start = DISABLE;
		local_reg.bit.reg_ipe_load_fs = DISABLE;
		break;

	}
	local_reg.bit.reg_ipe_rwgamma = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
    IPE Gamma read write control

    Enable/disable IPE Gamma SRAM read/write by CPU

    @param rw_sel

    @return None.
*/
void ipe_set_gamma_rw_en(IPE_RW_SRAM_ENUM rw_sel, IPE_RWGAM_OPT opt)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_rwgamma = rw_sel;
	if (rw_sel == DMA_WRITE_LUT) {
		local_reg.bit.reg_ipe_rwgamma_opt = opt;
	}

	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

//for FPGA verify only
void ipe_set_rw_en(IPE_RW_SRAM_ENUM rw_sel)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_rwgamma = rw_sel;

	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
    IPE get version

    Get IPE HW version number.

    @param None.

    @return ipe version number.
*/
UINT32 ipe_get_version(void)
{
	//T_IPE_CONTROL_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);

	return 0;//local_reg.bit.VERSION;
}

/**
    IPE set operation mode

    Set IPE operation mode.

    @param mode 0: D2D, 1: DCE mode, 2: all direct

    @return None.
*/
void ipe_set_op_mode(IPE_OPMODE mode)
{
	T_IPE_MODE_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	switch (mode) {
	case IPE_OPMODE_D2D:
		local_reg.bit.reg_ipe_mode = 0;
		break;
	case IPE_OPMODE_IPP:
		local_reg.bit.reg_ipe_mode = 1;
		break;
	case IPE_OPMODE_SIE_IPP:
		local_reg.bit.reg_ipe_mode = 2;
		break;
	default:
		DBG_ERR("ipe_set_op_mode() illegal mode %d!\r\n", mode);
		local_reg.bit.reg_ipe_mode = 0;
		break;
	}
	IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE dram single output mode

    Dram single output mode selection.

    @return None.
*/
void ipe_set_dram_out_mode(IPE_DRAM_OUTPUT_MODE mode)
{
	T_DMA_TO_IPE_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_0_OFS);
	local_reg.bit.reg_ipe_dram_out_mode = mode;

	IPE_SETREG(DMA_TO_IPE_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE dram single output channel_enable

    Dram single output channel_enable selection.

    @return None.
*/
void ipe_set_dram_single_channel_en(IPE_DRAM_SINGLE_OUT_EN ch)
{
	T_DMA_TO_IPE_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_0_OFS);
	local_reg.bit.reg_ipe_dram_out0_single_en = ch.single_out_y_en;
	local_reg.bit.reg_ipe_dram_out1_single_en = ch.single_out_c_en;
	local_reg.bit.reg_ipe_dram_out2_single_en = ch.single_out_va_en;
	local_reg.bit.reg_ipe_dram_out3_single_en = ch.single_out_defog_en;

	IPE_SETREG(DMA_TO_IPE_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE output switch

    Output channel selection.

    @param to_ime_en 0: disable IPE to IME output,1: enable IPE to IME output.
    @param to_dram_en 0: disable IPE to DRAM output,1: enable IPE to DRAM output.

    @return None.
*/
void ipe_set_output_switch(BOOL to_ime_en, BOOL to_dram_en)
{
	T_IPE_MODE_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	local_reg.bit.reg_ipe_imeon = to_ime_en;
	local_reg.bit.reg_ipe_dmaon = to_dram_en;

	IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE set dram output data type

    Select IPE dram output data type.

    @param sel 0:original, 1:subout, 2:direction.

    @return None.
*/
void ipe_set_dma_outsel(IPE_DMAOSEL sel)
{
	T_IPE_MODE_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	local_reg.bit.reg_dmao_sel = sel;

	IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE set H overlap to fixed 8 pixels

    Select IPE H overlap format.

    @param opt 0:auto calculation, 1: fixed for 8 pixels (MST+SQUARE)

    @return None.
*/
void ipe_set_hovlp_opt(IPE_HOVLP_SEL_ENUM opt)
{
	T_IPE_MODE_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	local_reg.bit.reg_ipe_mst_hovlp = opt;

	IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE set input format

    Select IPE input YCbCr format.

    @param imat 0:444, 1:422, 2:420, 3: Y only.

    @return None.
*/
void ipe_set_in_format(IPE_INOUT_FORMAT imat)
{
	T_IPE_MODE_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	local_reg.bit.reg_ipe_imat = imat;

	IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE set output format

    Select IPE output YCbCr format.

    @param omat 0:444, 1:422, 2:420, 3: Y only.
    @param subhsel 422 and 420 subsample selection,0: drop right pixel,1: drop left pixel.

    @return None.
*/
void ipe_set_out_format(IPE_INOUT_FORMAT omat, IPE_HSUB_SEL_ENUM sub_h_sel)
{
	T_IPE_MODE_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	local_reg.bit.reg_ipe_omat = omat;
	local_reg.bit.reg_ipe_subhsel = sub_h_sel;

	IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE enable features

    Enable IPE function blocks.

    @param features 1's in this input word will turn on corresponding blocks.

    @return None.
*/
void ipe_enable_feature(UINT32 features)
{
	T_IPE_MODE_REGISTER_1 local_reg;

	local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_1_OFS);
	local_reg.reg |= features;

	IPE_SETREG(IPE_MODE_REGISTER_1_OFS, local_reg.reg);
}

/**
    IPe disable features

    Disable IPE function blocks.

    @param features 1's in this input word will turn off corresponding blocks.

    @return None.
*/
void ipe_disable_feature(UINT32 features)
{
	T_IPE_MODE_REGISTER_1 local_reg;

	local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_1_OFS);
	local_reg.reg &= ~features;

	IPE_SETREG(IPE_MODE_REGISTER_1_OFS, local_reg.reg);
}

/**
    IPE set function

    Set IPE function blocks.

    @param features 1's in this input word will turn on corresponding blocks, while 0's will turn off.

    @return None.
*/
void ipe_set_feature(UINT32 features)
{
	T_IPE_MODE_REGISTER_1 local_reg;

	local_reg.reg = features;

	IPE_SETREG(IPE_MODE_REGISTER_1_OFS, local_reg.reg);
}

/**
    Check IPE functions

    IPE check function enable/disable now

    @param ipe_function which function need to check

    @return disable/enable
*/
BOOL ipe_check_function_enable(UINT32 ipe_function)
{
	if (IPE_GETREG(IPE_MODE_REGISTER_1_OFS) & ipe_function) {
		return ENABLE;
	} else {
		return DISABLE;
	}
}

/**
    IPE set h stripe

    Set IPE horizontal stripe.

    @param hstripe_infor Data stucture that contains related horizontal striping information: n, l, m, olap.

    @return None.
*/
void ipe_set_h_stripe(STR_STRIPE_INFOR hstripe_infor)
{
	T_IPE_STRIPE_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(IPE_STRIPE_REGISTER_0_OFS);
	local_reg.bit.reg_hn = hstripe_infor.hn;
	local_reg.bit.reg_hl = hstripe_infor.hl;
	local_reg.bit.reg_hm = hstripe_infor.hm;
	//local_reg.bit.HOLAP = hstripe_infor.Olap;

	IPE_SETREG(IPE_STRIPE_REGISTER_0_OFS, local_reg.reg);
}

/**
    Get IPE h stripe

    IPE horizontal striping information readout.

    @param None.

    @return Data stucture that includes horizontal striping l, m, n.
*/
STR_STRIPE_INFOR ipe_get_h_stripe(void)
{
	T_IPE_STRIPE_REGISTER_0 local_reg;
	STR_STRIPE_INFOR h_stripe;

	local_reg.reg = IPE_GETREG(IPE_STRIPE_REGISTER_0_OFS);
	h_stripe.hn = local_reg.bit.reg_hn;
	h_stripe.hl = local_reg.bit.reg_hl;
	h_stripe.hm = local_reg.bit.reg_hm;
	//h_stripe.Olap = (IPE_OLAP_ENUM)local_reg.bit.HOLAP;

	return h_stripe;
}

/**
    IPE set v stripe

    Set IPE vertical stripe.

    @param vstripe_infor Data stucture that contains related vertical striping information: n, l, m, olap.

    @return None.
*/
void ipe_set_v_stripe(STR_STRIPE_INFOR hstripe_infor)
{
	T_IPE_STRIPE_REGISTER_1 local_reg;

	local_reg.reg = IPE_GETREG(IPE_STRIPE_REGISTER_1_OFS);
	local_reg.bit.reg_vl = hstripe_infor.hl;

	IPE_SETREG(IPE_STRIPE_REGISTER_1_OFS, local_reg.reg);
}

/**
    Get IPE vstripe

    IPE vertical striping information readout.

    @param None.

    @return Data stucture that includes vertical striping l, m, n.
*/
STR_STRIPE_INFOR ipe_get_v_stripe(void)
{
	T_IPE_STRIPE_REGISTER_1 local_reg;
	STR_STRIPE_INFOR v_stripe;

	local_reg.reg = IPE_GETREG(IPE_STRIPE_REGISTER_1_OFS);
	v_stripe.hn = 0;
	v_stripe.hl = local_reg.bit.reg_vl;
	v_stripe.hm = 0;
	//v_stripe.Olap = IPE_OLAP_OFF;

	return v_stripe;
}


void ipe_set_linked_list_addr(UINT32 sai)
{
	T_DMA_TO_IPE_REGISTER_1 local_reg0;

	if ((sai == 0)) {
		DBG_ERR("IMG Input addresses cannot be 0x0!\r\n");
		return;
	}

	local_reg0.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_1_OFS);
	local_reg0.bit.reg_dram_sai_ll = sai >> 2;


	IPE_SETREG(DMA_TO_IPE_REGISTER_1_OFS, local_reg0.reg);
}

/**
    IPE set DMA input address

    Configure DMA input ping-pong addresses.

    @param addr_in_y DMA input channel (DMA to IPE) starting addressing 0 for Y channel.
    @param addr_in_c DMA input channel (DMA to IPE) starting addressing 1 for C channel.

    @return None.
*/
void ipe_set_dma_in_addr(UINT32 addr_in_y, UINT32 addr_in_c)
{
	T_DMA_TO_IPE_REGISTER_2 local_reg0;
	T_DMA_TO_IPE_REGISTER_3 local_reg1;

	if ((addr_in_y == 0) || (addr_in_c == 0)) {
		DBG_ERR("IMG Input addresses cannot be 0x0!\r\n");
		return;
	}

	local_reg0.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_2_OFS);
	local_reg0.bit.reg_dram_sai_y = addr_in_y >> 2;

	local_reg1.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_3_OFS);
	local_reg1.bit.reg_dram_sai_c = addr_in_c >> 2;

	IPE_SETREG(DMA_TO_IPE_REGISTER_2_OFS, local_reg0.reg);
	IPE_SETREG(DMA_TO_IPE_REGISTER_3_OFS, local_reg1.reg);
}

void ipe_set_dma_in_defog_addr(UINT32 sai)
{
	T_DMA_DEFOG_SUBIMG_INPUT_CHANNEL_REGISTER local_reg0;

	if ((sai == 0)) {
		DBG_ERR("IMG defog Input addresses cannot be 0x0!\r\n");
		return;
	}

	local_reg0.reg = IPE_GETREG(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_REGISTER_OFS);
	local_reg0.bit.reg_defog_subimg_dramsai = sai >> 2;


	IPE_SETREG(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_REGISTER_OFS, local_reg0.reg);
}

/**
    IPE set DMA address of Gamma LUT

    Configure DMA address of Gamma LUT

    @param sai DMA input channel (DMA to IPE) starting address for RGB Gamma LUT.

    @return None.
*/
void ipe_set_dma_in_gamma_lut_addr(UINT32 sai)
{
	T_DMA_TO_IPE_REGISTER_5 local_reg3;

	if (sai == 0) {
		DBG_ERR("LUT Input addresses cannot be 0x0!\r\n");
		return;
	}

	local_reg3.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_5_OFS);
	local_reg3.bit.reg_dram_sai_gamma = sai >> 2;

	IPE_SETREG(DMA_TO_IPE_REGISTER_5_OFS, local_reg3.reg);
}

/**
    IPE set DMA address of Y curve LUT

    Configure DMA address of Y cuvrve LUT

    @param sai DMA input channel (DMA to IPE) starting address for Y curve LUT.

    @return None.
*/
void ipe_set_dma_in_ycurve_lut_addr(UINT32 sai)
{
	T_DMA_TO_IPE_REGISTER_4 local_reg2;

	if (sai == 0) {
		DBG_ERR("LUT Input addresses cannot be 0x0!\r\n");
		return;
	}

	local_reg2.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_4_OFS);
	local_reg2.bit.reg_dram_sai_ycurve = sai >> 2;

	IPE_SETREG(DMA_TO_IPE_REGISTER_4_OFS, local_reg2.reg);
}

/**
    IPE set DMA input line offset

    Configure DMA input line offset.

    @param ofsi0 input line offset(word) 0 for Y channel.
    @param ofsi1 input line offset(word) 1 for C channel.

    @return None.
*/
void ipe_set_dma_in_offset(UINT32 ofsi0, UINT32 ofsi1)
{
	T_DMA_TO_IPE_REGISTER_6 local_reg;
	T_DMA_TO_IPE_REGISTER_7 local_reg2;

	local_reg.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_6_OFS);
	local_reg.bit.reg_dram_ofsi_y = ofsi0 >> 2;

	local_reg2.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_7_OFS);
	local_reg2.bit.reg_dram_ofsi_c = ofsi1 >> 2;

	IPE_SETREG(DMA_TO_IPE_REGISTER_6_OFS, local_reg.reg);
	IPE_SETREG(DMA_TO_IPE_REGISTER_7_OFS, local_reg2.reg);
}

void ipe_set_dma_in_defog_offset(UINT32 ofsi)
{
	T_DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER_OFS);
	local_reg.bit.reg_defog_subimg_lofsi = ofsi >> 2;

	IPE_SETREG(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER_OFS, local_reg.reg);
}

/**
    IPE set DMA input random LSB

    Configure input random LSB.

    @param ofsi0 input line offset(word) 0 for Y channel.
    @param ofsi1 input line offset(word) 1 for C channel.

    @return None.
*/
void ipe_set_dma_in_rand(IPE_ENABLE en, IPE_ENABLE reset)
{
	T_DMA_TO_IPE_REGISTER_6 local_reg;

	local_reg.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_6_OFS);
	local_reg.bit.reg_inrand_en = en;
	local_reg.bit.reg_inrand_rst = reset;

	IPE_SETREG(DMA_TO_IPE_REGISTER_6_OFS, local_reg.reg);
}

/**
    IPE set DMA output address

    Configure DMA output address.

    @param sao_y DMA output starting address of Y channel.

    @return None.
*/
void ipe_set_dma_out_addr_y(UINT32 sao_y)
{
	T_IPE_TO_DMA_YCC_CHANNEL_REGISTER_0 local_reg0;

	if (sao_y == 0) {
		DBG_ERR("IMG Output addresses cannot be 0x0!\r\n");
		return;
	}

	local_reg0.reg = IPE_GETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_0_OFS);
	local_reg0.bit.reg_dram_sao_y = sao_y >> 2;

	IPE_SETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_0_OFS, local_reg0.reg);
}

/**
    IPE set DMA output c address

    Configure DMA output c address.

    @param sao_c DMA output starting address of C channel.

    @return None.
*/
void ipe_set_dma_out_addr_c(UINT32 sao_c)
{
	T_IPE_TO_DMA_YCC_CHANNEL_REGISTER_1 local_reg1;

	if (sao_c == 0) {
		DBG_ERR("IMG Output addresses cannot be 0x0!\r\n");
		return;
	}

	local_reg1.reg = IPE_GETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_1_OFS);
	local_reg1.bit.reg_dram_sao_c = sao_c >> 2;

	IPE_SETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_1_OFS, local_reg1.reg);
}

/**
    IPE set DMA output y line offset

    Configure DMA output y line offset.

    @param ofso_y output line offset(word) of Y channel.


    @return None.
*/
void ipe_set_dma_out_offset_y(UINT32 ofso_y)
{
	T_IPE_TO_DMA_YCC_CHANNEL_REGISTER_2 local_reg;

	local_reg.reg = IPE_GETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_2_OFS);
	local_reg.bit.reg_dram_ofso_y = ofso_y >> 2;

	IPE_SETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_2_OFS, local_reg.reg);
}


/**
    IPE set DMA output c line offset

    Configure DMA output c line offset.

    @param ofso_c output line offset(word) of C channel.


    @return None.
*/
void ipe_set_dma_out_offset_c(UINT32 ofso_c)
{
	T_IPE_TO_DMA_YCC_CHANNEL_REGISTER_3 local_reg1;

	local_reg1.reg = IPE_GETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_3_OFS);
	local_reg1.bit.reg_dram_ofso_c = ofso_c >> 2;

	IPE_SETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_3_OFS, local_reg1.reg);
}

/**
    IPE interrupt enable status

    Get current IPE interrupt enable status

    @return IPE interrupt status enable readout.

    @return None
*/
UINT32 ipe_get_int_enable(void)
{
	UINT32 reg;

	reg = IPE_GETREG(IPE_INTERRUPT_ENABLE_REGISTER_OFS);
	return reg;
}

/**
    Enable IPE interrupt

    IPE interrupt enable selection.

    @param intr 1's in this word will activate corresponding interrupt.

    @return None.
*/
void ipe_enable_int(UINT32 intr)
{
	T_IPE_INTERRUPT_ENABLE_REGISTER local_reg;

	local_reg.reg = intr;

	IPE_SETREG(IPE_INTERRUPT_ENABLE_REGISTER_OFS, local_reg.reg);
}

/**
    IPE interrupt status

    Get current IPE interrupt status

    @return IPE interrupt status readout.

    @return None.
*/
UINT32 ipe_get_int_status(void)
{
	UINT32 reg;

	reg = IPE_GETREG(IPE_INTERRUPT_STATUS_REGISTER_OFS);
	return reg;
}

/**
    IPE interrupt status clear

    Clear IPE interrupt status.

    @param intr 1's in this word will clear corresponding interrupt status.

    @return None.
*/
void ipe_clear_int(UINT32 intr)
{
	IPE_SETREG(IPE_INTERRUPT_STATUS_REGISTER_OFS, intr);
}

/**
    IPE get debug status

    IPE get debug status.

    @param None

    @return debug status
*/
UINT32 ipe_get_debug_status(void)
{
	T_DEBUG_STATUS_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(DEBUG_STATUS_REGISTER_OFS);

	return local_reg.reg;
}

/**
    IPE check busy

    IPE check status is idle or busy now

    @param None

    @return idle:0/busy:1
*/
BOOL ipe_check_busy(void)
{
	T_DEBUG_STATUS_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(DEBUG_STATUS_REGISTER_OFS);
	if (local_reg.bit.reg_ipestatus) {
		return 1;
	} else {
		return 0;
	}
}

/**
    IPE set tone remap LUT

    Set IPE Tone remap LUT.

    @param *p_tonemap_lut pointer to tone remap LUT.

    @return None.
*/
void ipe_set_tone_remap(UINT32 *p_tonemap_lut)
{
	UINT32 i = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_EDGE_GAMMA0 local_reg0;
	T_EDGE_GAMMA1 local_reg1;
	T_EDGE_GAMMA2 local_reg2;
	T_EDGE_GAMMA3 local_reg3;
	T_EDGE_GAMMA4 local_reg4;
	T_EDGE_GAMMA5 local_reg5;
	T_EDGE_GAMMA6 local_reg6;
	T_EDGE_GAMMA7 local_reg7;
	T_EDGE_GAMMA8 local_reg8;
	T_EDGE_GAMMA9 local_reg9;
	T_EDGE_GAMMA10 local_reg10;
	T_EDGE_GAMMA11 local_reg11;
	T_EDGE_GAMMA12 local_reg12;
	T_EDGE_GAMMA13 local_reg13;
	T_EDGE_GAMMA14 local_reg14;
	T_EDGE_GAMMA15 local_reg15;
	T_EDGE_GAMMA16 local_reg16;
	T_EDGE_GAMMA17 local_reg17;
	T_EDGE_GAMMA18 local_reg18;
	T_EDGE_GAMMA19 local_reg19;
	T_EDGE_GAMMA20 local_reg20;
#endif

	T_EDGE_GAMMA21 local_reg21;

#if 0
	local_reg0.reg = IPE_GETREG(EDGE_GAMMA0_OFS);
	local_reg0.bit.reg_edge_lut_0 = p_tonemap_lut[0];
	local_reg0.bit.reg_edge_lut_1 = p_tonemap_lut[1];
	local_reg0.bit.reg_edge_lut_2 = p_tonemap_lut[2];
	local_reg1.reg = IPE_GETREG(EDGE_GAMMA1_OFS);
	local_reg1.bit.reg_edge_lut_3 = p_tonemap_lut[3];
	local_reg1.bit.reg_edge_lut_4 = p_tonemap_lut[4];
	local_reg1.bit.reg_edge_lut_5 = p_tonemap_lut[5];
	local_reg2.reg = IPE_GETREG(EDGE_GAMMA2_OFS);
	local_reg2.bit.reg_edge_lut_6 = p_tonemap_lut[6];
	local_reg2.bit.reg_edge_lut_7 = p_tonemap_lut[7];
	local_reg2.bit.reg_edge_lut_8 = p_tonemap_lut[8];
	local_reg3.reg = IPE_GETREG(EDGE_GAMMA3_OFS);
	local_reg3.bit.reg_edge_lut_9 = p_tonemap_lut[9];
	local_reg3.bit.reg_edge_lut_10 = p_tonemap_lut[10];
	local_reg3.bit.reg_edge_lut_11 = p_tonemap_lut[11];
	local_reg4.reg = IPE_GETREG(EDGE_GAMMA4_OFS);
	local_reg4.bit.reg_edge_lut_12 = p_tonemap_lut[12];
	local_reg4.bit.reg_edge_lut_13 = p_tonemap_lut[13];
	local_reg4.bit.reg_edge_lut_14 = p_tonemap_lut[14];
	local_reg5.reg = IPE_GETREG(EDGE_GAMMA5_OFS);
	local_reg5.bit.reg_edge_lut_15 = p_tonemap_lut[15];
	local_reg5.bit.reg_edge_lut_16 = p_tonemap_lut[16];
	local_reg5.bit.reg_edge_lut_17 = p_tonemap_lut[17];
	local_reg6.reg = IPE_GETREG(EDGE_GAMMA6_OFS);
	local_reg6.bit.reg_edge_lut_18 = p_tonemap_lut[18];
	local_reg6.bit.reg_edge_lut_19 = p_tonemap_lut[19];
	local_reg6.bit.reg_edge_lut_20 = p_tonemap_lut[20];
	local_reg7.reg = IPE_GETREG(EDGE_GAMMA7_OFS);
	local_reg7.bit.reg_edge_lut_21 = p_tonemap_lut[21];
	local_reg7.bit.reg_edge_lut_22 = p_tonemap_lut[22];
	local_reg7.bit.reg_edge_lut_23 = p_tonemap_lut[23];
	local_reg8.reg = IPE_GETREG(EDGE_GAMMA8_OFS);
	local_reg8.bit.reg_edge_lut_24 = p_tonemap_lut[24];
	local_reg8.bit.reg_edge_lut_25 = p_tonemap_lut[25];
	local_reg8.bit.reg_edge_lut_26 = p_tonemap_lut[26];
	local_reg9.reg = IPE_GETREG(EDGE_GAMMA9_OFS);
	local_reg9.bit.reg_edge_lut_27 = p_tonemap_lut[27];
	local_reg9.bit.reg_edge_lut_28 = p_tonemap_lut[28];
	local_reg9.bit.reg_edge_lut_29 = p_tonemap_lut[29];
	local_reg10.reg = IPE_GETREG(EDGE_GAMMA10_OFS);
	local_reg10.bit.reg_edge_lut_30 = p_tonemap_lut[30];
	local_reg10.bit.reg_edge_lut_31 = p_tonemap_lut[31];
	local_reg10.bit.reg_edge_lut_32 = p_tonemap_lut[32];
	local_reg11.reg = IPE_GETREG(EDGE_GAMMA11_OFS);
	local_reg11.bit.reg_edge_lut_33 = p_tonemap_lut[33];
	local_reg11.bit.reg_edge_lut_34 = p_tonemap_lut[34];
	local_reg11.bit.reg_edge_lut_35 = p_tonemap_lut[35];
	local_reg12.reg = IPE_GETREG(EDGE_GAMMA12_OFS);
	local_reg12.bit.reg_edge_lut_36 = p_tonemap_lut[36];
	local_reg12.bit.reg_edge_lut_37 = p_tonemap_lut[37];
	local_reg12.bit.reg_edge_lut_38 = p_tonemap_lut[38];
	local_reg13.reg = IPE_GETREG(EDGE_GAMMA13_OFS);
	local_reg13.bit.reg_edge_lut_39 = p_tonemap_lut[39];
	local_reg13.bit.reg_edge_lut_40 = p_tonemap_lut[40];
	local_reg13.bit.reg_edge_lut_41 = p_tonemap_lut[41];
	local_reg14.reg = IPE_GETREG(EDGE_GAMMA14_OFS);
	local_reg14.bit.reg_edge_lut_42 = p_tonemap_lut[42];
	local_reg14.bit.reg_edge_lut_43 = p_tonemap_lut[43];
	local_reg14.bit.reg_edge_lut_44 = p_tonemap_lut[44];
	local_reg15.reg = IPE_GETREG(EDGE_GAMMA15_OFS);
	local_reg15.bit.reg_edge_lut_45 = p_tonemap_lut[45];
	local_reg15.bit.reg_edge_lut_46 = p_tonemap_lut[46];
	local_reg15.bit.reg_edge_lut_47 = p_tonemap_lut[47];
	local_reg16.reg = IPE_GETREG(EDGE_GAMMA16_OFS);
	local_reg16.bit.reg_edge_lut_48 = p_tonemap_lut[48];
	local_reg16.bit.reg_edge_lut_49 = p_tonemap_lut[49];
	local_reg16.bit.reg_edge_lut_50 = p_tonemap_lut[50];
	local_reg17.reg = IPE_GETREG(EDGE_GAMMA17_OFS);
	local_reg17.bit.reg_edge_lut_51 = p_tonemap_lut[51];
	local_reg17.bit.reg_edge_lut_52 = p_tonemap_lut[52];
	local_reg17.bit.reg_edge_lut_53 = p_tonemap_lut[53];
	local_reg18.reg = IPE_GETREG(EDGE_GAMMA18_OFS);
	local_reg18.bit.reg_edge_lut_54 = p_tonemap_lut[54];
	local_reg18.bit.reg_edge_lut_55 = p_tonemap_lut[55];
	local_reg18.bit.reg_edge_lut_56 = p_tonemap_lut[56];
	local_reg19.reg = IPE_GETREG(EDGE_GAMMA19_OFS);
	local_reg19.bit.reg_edge_lut_57 = p_tonemap_lut[57];
	local_reg19.bit.reg_edge_lut_58 = p_tonemap_lut[58];
	local_reg19.bit.reg_edge_lut_59 = p_tonemap_lut[59];
	local_reg20.reg = IPE_GETREG(EDGE_GAMMA20_OFS);
	local_reg20.bit.reg_edge_lut_60 = p_tonemap_lut[60];
	local_reg20.bit.reg_edge_lut_61 = p_tonemap_lut[61];
	local_reg20.bit.reg_edge_lut_62 = p_tonemap_lut[62];
#endif


#if 0
	IPE_SETREG(EDGE_GAMMA0_OFS, local_reg0.reg);
	IPE_SETREG(EDGE_GAMMA1_OFS, local_reg1.reg);
	IPE_SETREG(EDGE_GAMMA2_OFS, local_reg2.reg);
	IPE_SETREG(EDGE_GAMMA3_OFS, local_reg3.reg);
	IPE_SETREG(EDGE_GAMMA4_OFS, local_reg4.reg);
	IPE_SETREG(EDGE_GAMMA5_OFS, local_reg5.reg);
	IPE_SETREG(EDGE_GAMMA6_OFS, local_reg6.reg);
	IPE_SETREG(EDGE_GAMMA7_OFS, local_reg7.reg);
	IPE_SETREG(EDGE_GAMMA8_OFS, local_reg8.reg);
	IPE_SETREG(EDGE_GAMMA9_OFS, local_reg9.reg);
	IPE_SETREG(EDGE_GAMMA10_OFS, local_reg10.reg);
	IPE_SETREG(EDGE_GAMMA11_OFS, local_reg11.reg);
	IPE_SETREG(EDGE_GAMMA12_OFS, local_reg12.reg);
	IPE_SETREG(EDGE_GAMMA13_OFS, local_reg13.reg);
	IPE_SETREG(EDGE_GAMMA14_OFS, local_reg14.reg);
	IPE_SETREG(EDGE_GAMMA15_OFS, local_reg15.reg);
	IPE_SETREG(EDGE_GAMMA16_OFS, local_reg16.reg);
	IPE_SETREG(EDGE_GAMMA17_OFS, local_reg17.reg);
	IPE_SETREG(EDGE_GAMMA18_OFS, local_reg18.reg);
	IPE_SETREG(EDGE_GAMMA19_OFS, local_reg19.reg);
	IPE_SETREG(EDGE_GAMMA20_OFS, local_reg20.reg);
#endif

	for (i = 0; i < 21; i++) {
		//reg_set_val = 0;

		reg_ofs = EDGE_GAMMA0_OFS + (i << 2);
		reg_set_val = (p_tonemap_lut[(i * 3)] & 0x000003FF) + ((p_tonemap_lut[(i * 3) + 1] & 0x000003FF) << 10) + ((p_tonemap_lut[(i * 3) + 2] & 0x000003FF) << 20);

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	local_reg21.reg = IPE_GETREG(EDGE_GAMMA21_OFS);
	local_reg21.bit.reg_edge_lut_63 = p_tonemap_lut[63];
	local_reg21.bit.reg_edge_lut_64 = p_tonemap_lut[64];
	IPE_SETREG(EDGE_GAMMA21_OFS, local_reg21.reg);
}

/**
    IPE set edge selection

    IPE edge extraction diagonal coefficients setting.

    @param gamsel
    @param chsel

    @return None.
*/
void ipe_set_edge_extract_sel(IPE_EEXT_GAM_ENUM gam_sel, IPE_EEXT_CH_ENUM ch_sel)
{
	T_EDGE_EXTRACTION_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_0_OFS);
	local_reg.bit.reg_eext_gamsel = gam_sel;
	local_reg.bit.reg_eext_chsel = ch_sel;

	IPE_SETREG(EDGE_EXTRACTION_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE set edge kernel B

    IPE edge extraction diagonal coefficients setting.

    @param eext integer in 2's complement representation, 0~1023 or -512~511.
    @param eextdiv edge extraction normalization factor, 0~15.

    @return None.
*/
void ipe_set_edge_extract_b(INT16 *p_eext_ker, UINT32 eext_div_b, UINT8 eext_enh_b)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	T_EDGE_EXTRACTION_REGISTER_0 local_reg0;
	//T_EDGE_EXTRACTION_REGISTER_1 local_reg1;
	//T_EDGE_EXTRACTION_REGISTER_2 local_reg2;
	//T_EDGE_EXTRACTION_REGISTER_3 local_reg3;

	local_reg0.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_0_OFS);
	local_reg0.bit.reg_eext0 = p_eext_ker[0];
	local_reg0.bit.reg_eextdiv_e7 = eext_div_b;
	local_reg0.bit.reg_eext_enh_e7 = eext_enh_b;
	IPE_SETREG(EDGE_EXTRACTION_REGISTER_0_OFS, local_reg0.reg);

#if 0
	local_reg1.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_1_OFS);
	local_reg1.bit.reg_eext1 = p_eext_ker[1];
	local_reg1.bit.reg_eext2 = p_eext_ker[2];
	local_reg1.bit.reg_eext3 = p_eext_ker[3];

	local_reg2.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_2_OFS);
	local_reg2.bit.reg_eext4 = p_eext_ker[4];
	local_reg2.bit.reg_eext5 = p_eext_ker[5];
	local_reg2.bit.reg_eext6 = p_eext_ker[6];

	local_reg3.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_3_OFS);
	local_reg3.bit.reg_eext7 = p_eext_ker[7];
	local_reg3.bit.reg_eext8 = p_eext_ker[8];
	local_reg3.bit.reg_eext9 = p_eext_ker[9];
	IPE_SETREG(EDGE_EXTRACTION_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(EDGE_EXTRACTION_REGISTER_2_OFS, local_reg2.reg);
	IPE_SETREG(EDGE_EXTRACTION_REGISTER_3_OFS, local_reg3.reg);
#endif

	for (i = 0; i < 3; i++) {
		//reg_set_val = 0;

		reg_ofs = EDGE_EXTRACTION_REGISTER_1_OFS + (i << 2);

		idx = (i * 3);
		reg_set_val = ((UINT32)(p_eext_ker[idx + 1] & 0x03FF)) + (((UINT32)(p_eext_ker[idx + 2] & 0x03FF)) << 10) + (((UINT32)(p_eext_ker[idx + 3] & 0x03FF)) << 20);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
    IPE set edge kernel strength
*/
void ipe_set_eext_kerstrength(IPE_EEXT_KER_STRENGTH *p_ker_str)
{
	T_EDGE_EXTRACTION_REGISTER_4 local_reg0;
	T_EDGE_EXTRACTION_REGISTER_5 local_reg1;

	local_reg0.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_4_OFS);
	local_reg0.bit.reg_eext_enh_e3 = p_ker_str->ker_a.eext_enh;
	local_reg0.bit.reg_eext_enh_e5a = p_ker_str->ker_c.eext_enh;
	local_reg0.bit.reg_eext_enh_e5b = p_ker_str->ker_d.eext_enh;
	local_reg1.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_5_OFS);
	local_reg1.bit.reg_eextdiv_e3 =  p_ker_str->ker_a.eext_div;
	local_reg1.bit.reg_eextdiv_e5a =  p_ker_str->ker_c.eext_div;
	local_reg1.bit.reg_eextdiv_e5b =  p_ker_str->ker_d.eext_div;

	IPE_SETREG(EDGE_EXTRACTION_REGISTER_4_OFS, local_reg0.reg);
	IPE_SETREG(EDGE_EXTRACTION_REGISTER_5_OFS, local_reg1.reg);
}

void ipe_set_eext_engcon(IPE_EEXT_ENG_CON *p_eext_engcon)
{
	T_EDGE_EXTRACTION_REGISTER_5 local_reg0;
	T_EDGE_REGION_EXTRACTION_REGISTER_0 local_reg1;

	local_reg0.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_5_OFS);
	local_reg0.bit.reg_eextdiv_eng =  p_eext_engcon->eext_div_eng;
	local_reg0.bit.reg_eextdiv_con =  p_eext_engcon->eext_div_con;

	local_reg1.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_0_OFS);
	local_reg1.bit.reg_w_con_eng = p_eext_engcon->wt_con_eng;

	IPE_SETREG(EDGE_EXTRACTION_REGISTER_5_OFS, local_reg0.reg);
	IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_0_OFS, local_reg1.reg);
}

void ipe_set_ker_thickness(IPE_KER_THICKNESS *p_ker_thickness)
{
	T_EDGE_REGION_EXTRACTION_REGISTER_1 local_reg0;

	local_reg0.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_1_OFS);
	local_reg0.bit.reg_w_ker_thin =  p_ker_thickness->wt_ker_thin;
	local_reg0.bit.reg_w_ker_robust =  p_ker_thickness->wt_ker_robust;
	local_reg0.bit.reg_iso_ker_thin =  p_ker_thickness->iso_ker_thin;
	local_reg0.bit.reg_iso_ker_robust =  p_ker_thickness->iso_ker_robust;

	IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_1_OFS, local_reg0.reg);
}

void ipe_set_ker_thickness_hld(IPE_KER_THICKNESS *p_ker_thickness_hld)
{
	T_EDGE_REGION_EXTRACTION_REGISTER_2 local_reg0;

	local_reg0.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_2_OFS);
	local_reg0.bit.reg_w_ker_thin_hld =  p_ker_thickness_hld->wt_ker_thin;
	local_reg0.bit.reg_w_ker_robust_hld =  p_ker_thickness_hld->wt_ker_robust;
	local_reg0.bit.reg_iso_ker_thin_hld =  p_ker_thickness_hld->iso_ker_thin;
	local_reg0.bit.reg_iso_ker_robust_hld =  p_ker_thickness_hld->iso_ker_robust;

	IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_2_OFS, local_reg0.reg);
}

void ipe_set_region(IPE_REGION_PARAM *p_region)
{
	T_EDGE_REGION_EXTRACTION_REGISTER_0 local_reg0;
	T_EDGE_REGION_EXTRACTION_REGISTER_2 local_reg1;
	T_EDGE_REGION_EXTRACTION_REGISTER_3 local_reg2;
	T_EDGE_REGION_EXTRACTION_REGISTER_4 local_reg3;

	local_reg0.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_0_OFS);
	local_reg0.bit.reg_w_low = p_region->wt_low;
	local_reg0.bit.reg_w_high = p_region->wt_high;
	local_reg1.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_2_OFS);
	local_reg1.bit.reg_w_hld_low = p_region->wt_low_hld;
	local_reg1.bit.reg_w_hld_high = p_region->wt_high_hld;
	local_reg2.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_3_OFS);
	local_reg2.bit.reg_th_flat = p_region->th_flat;
	local_reg2.bit.reg_th_edge = p_region->th_edge;
	local_reg3.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_4_OFS);
	local_reg3.bit.reg_th_flat_hld = p_region->th_flat_hld;
	local_reg3.bit.reg_th_edge_hld = p_region->th_edge_hld;
	local_reg3.bit.reg_th_lum_hld = p_region->th_lum_hld;

	IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_2_OFS, local_reg1.reg);
	IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_3_OFS, local_reg2.reg);
	IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_4_OFS, local_reg3.reg);

}

void ipe_set_region_slope(UINT16 slope, UINT16 slope_hld)
{
	T_EDGE_REGION_EXTRACTION_REGISTER_5 local_reg0;

	local_reg0.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_5_OFS);
	local_reg0.bit.reg_slope_con_eng = slope;
	local_reg0.bit.reg_slope_hld_con_eng = slope_hld;

	IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_5_OFS, local_reg0.reg);
}

void ipe_set_overshoot(IPE_OVERSHOOT_PARAM *p_overshoot)
{
	T_OVERSHOOTING_CONTROL_REGISTER_0 local_reg0;
	T_OVERSHOOTING_CONTROL_REGISTER_1 local_reg1;
	T_OVERSHOOTING_CONTROL_REGISTER_2 local_reg2;
	T_OVERSHOOTING_CONTROL_REGISTER_3 local_reg3;
	T_OVERSHOOTING_CONTROL_REGISTER_4 local_reg4;

	local_reg0.reg = IPE_GETREG(OVERSHOOTING_CONTROL_REGISTER_0_OFS);
	local_reg0.bit.reg_overshoot_en = p_overshoot->overshoot_en;
	local_reg0.bit.reg_wt_overshoot = p_overshoot->wt_overshoot;
	local_reg0.bit.reg_wt_undershoot = p_overshoot->wt_undershoot;

	local_reg1.reg = IPE_GETREG(OVERSHOOTING_CONTROL_REGISTER_1_OFS);
	local_reg1.bit.reg_th_overshoot = p_overshoot->th_overshoot;
	local_reg1.bit.reg_th_undershoot = p_overshoot->th_undershoot;
	local_reg1.bit.reg_th_undershoot_lum = p_overshoot->th_undershoot_lum;
	local_reg1.bit.reg_th_undershoot_eng = p_overshoot->th_undershoot_eng;

	local_reg2.reg = IPE_GETREG(OVERSHOOTING_CONTROL_REGISTER_2_OFS);
	local_reg2.bit.reg_clamp_wt_mod_lum = p_overshoot->clamp_wt_mod_lum;
	local_reg2.bit.reg_clamp_wt_mod_eng = p_overshoot->clamp_wt_mod_eng;
	local_reg2.bit.reg_strength_lum_eng = p_overshoot->strength_lum_eng;
	local_reg2.bit.reg_norm_lum_eng = p_overshoot->norm_lum_eng;

	local_reg3.reg = IPE_GETREG(OVERSHOOTING_CONTROL_REGISTER_3_OFS);
	local_reg3.bit.reg_slope_overshoot = p_overshoot->slope_overshoot;
	local_reg3.bit.reg_slope_undershoot = p_overshoot->slope_undershoot;

	local_reg4.reg = IPE_GETREG(OVERSHOOTING_CONTROL_REGISTER_3_OFS);
	local_reg4.bit.reg_slope_undershoot_lum = p_overshoot->slope_undershoot_lum;
	local_reg4.bit.reg_slope_undershoot_eng = p_overshoot->slope_undershoot_eng;

	IPE_SETREG(OVERSHOOTING_CONTROL_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(OVERSHOOTING_CONTROL_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(OVERSHOOTING_CONTROL_REGISTER_2_OFS, local_reg2.reg);
	IPE_SETREG(OVERSHOOTING_CONTROL_REGISTER_3_OFS, local_reg3.reg);
	IPE_SETREG(OVERSHOOTING_CONTROL_REGISTER_4_OFS, local_reg4.reg);

}

void ipe_get_edge_stcs(IPE_EDGE_STCS_RSLT *stcs_result)
{
	T_EDGE_STATISTIC_REGISTER local_reg0;

	local_reg0.reg = IPE_GETREG(EDGE_STATISTIC_REGISTER_OFS);
	stcs_result->localmax_max = local_reg0.bit.localmax_statistics_max;
	stcs_result->coneng_max = local_reg0.bit.coneng_statistics_max;
	stcs_result->coneng_avg = local_reg0.bit.coneng_statistics_avg;
}


/**
    IPE set luminance map thresholds

    Set IPE edge mapping: luminance thresholds.

    @param p_esmap edge luminance map infor

    @return None.
*/
void ipe_set_esmap_th(IPE_ESMAP_INFOR *p_esmap)
{
	T_EDGE_LUMINANCE_PROCESS_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(EDGE_LUMINANCE_PROCESS_REGISTER_0_OFS);
	local_reg.bit.reg_esthrl = p_esmap->ethr_low;
	local_reg.bit.reg_esthrh = p_esmap->ethr_high;
	local_reg.bit.reg_establ = p_esmap->etab_low;
	local_reg.bit.reg_estabh = p_esmap->etab_high;

	IPE_SETREG(EDGE_LUMINANCE_PROCESS_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE set Estab table

    IPE edge control luminance mapping.

    @param *estab16tbl pointer to edge profile mapping table.

    @return None.
*/
void ipe_set_estab(UINT8 *p_estab)
{
	UINT32 i = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_EDGE_LUMINANCE_PROCESS_REGISTER_1 local_reg0;
	T_EDGE_LUMINANCE_PROCESS_REGISTER_2 local_reg1;
	T_EDGE_LUMINANCE_PROCESS_REGISTER_3 local_reg2;
	T_EDGE_LUMINANCE_PROCESS_REGISTER_4 local_reg3;

	local_reg0.reg = 0;
	local_reg0.bit.reg_eslutl_0 = p_estab[0];
	local_reg0.bit.reg_eslutl_1 = p_estab[1];
	local_reg0.bit.reg_eslutl_2 = p_estab[2];
	local_reg0.bit.reg_eslutl_3 = p_estab[3];

	local_reg1.reg = 0;
	local_reg1.bit.reg_eslutl_4 = p_estab[4];
	local_reg1.bit.reg_eslutl_5 = p_estab[5];
	local_reg1.bit.reg_eslutl_6 = p_estab[6];
	local_reg1.bit.reg_eslutl_7 = p_estab[7];

	local_reg2.reg = 0;
	local_reg2.bit.reg_esluth_0 = p_estab[8];
	local_reg2.bit.reg_esluth_1 = p_estab[9];
	local_reg2.bit.reg_esluth_2 = p_estab[10];
	local_reg2.bit.reg_esluth_3 = p_estab[11];

	local_reg3.reg = 0;
	local_reg3.bit.reg_esluth_4 = p_estab[12];
	local_reg3.bit.reg_esluth_5 = p_estab[13];
	local_reg3.bit.reg_esluth_6 = p_estab[14];
	local_reg3.bit.reg_esluth_7 = p_estab[15];

	IPE_SETREG(EDGE_LUMINANCE_PROCESS_REGISTER_1_OFS, local_reg0.reg);
	IPE_SETREG(EDGE_LUMINANCE_PROCESS_REGISTER_2_OFS, local_reg1.reg);
	IPE_SETREG(EDGE_LUMINANCE_PROCESS_REGISTER_3_OFS, local_reg2.reg);
	IPE_SETREG(EDGE_LUMINANCE_PROCESS_REGISTER_4_OFS, local_reg3.reg);
#endif

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = EDGE_LUMINANCE_PROCESS_REGISTER_1_OFS + (i << 2);
		reg_set_val = (p_estab[(i << 2)] & 0xFF) + ((p_estab[(i << 2) + 1] & 0xFF) << 8) + ((p_estab[(i << 2) + 2] & 0xFF) << 16) + ((p_estab[(i << 2) + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
    IPE set edge map thresholds

    Set IPE edge mapping thresholds.

    @param p_edmap edge map infor

    @return None.
*/
void ipe_set_edgemap_th(IPE_EDGEMAP_INFOR *p_edmap)
{
	T_EDGE_DMAP_PROCESS_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(EDGE_DMAP_PROCESS_REGISTER_0_OFS);
	local_reg.bit.reg_edthrl = p_edmap->ethr_low;
	local_reg.bit.reg_edthrh = p_edmap->ethr_high;
	local_reg.bit.reg_edtabl = p_edmap->etab_low;
	local_reg.bit.reg_edtabh = p_edmap->etab_high;
	local_reg.bit.reg_edin_sel = p_edmap->ein_sel;

	IPE_SETREG(EDGE_DMAP_PROCESS_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE set edge map

    Set IPE Edge map 16 tap table.

    @param *emap16tbl pointer to edge map table.

    @return None.
*/
void ipe_set_edgemap_tab(UINT8 *p_edtab)
{
	UINT32 i = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_EDGE_DMAP_PROCESS_REGISTER_1 local_reg0;
	T_EDGE_DMAP_PROCESS_REGISTER_2 local_reg1;
	T_EDGE_DMAP_PROCESS_REGISTER_3 local_reg2;
	T_EDGE_DMAP_PROCESS_REGISTER_4 local_reg3;

	local_reg0.reg = 0;
	local_reg0.bit.reg_edlutl_0 = p_edtab[0];
	local_reg0.bit.reg_edlutl_1 = p_edtab[1];
	local_reg0.bit.reg_edlutl_2 = p_edtab[2];
	local_reg0.bit.reg_edlutl_3 = p_edtab[3];

	local_reg1.reg = 0;
	local_reg1.bit.reg_edlutl_4 = p_edtab[4];
	local_reg1.bit.reg_edlutl_5 = p_edtab[5];
	local_reg1.bit.reg_edlutl_6 = p_edtab[6];
	local_reg1.bit.reg_edlutl_7 = p_edtab[7];

	local_reg2.reg = 0;
	local_reg2.bit.reg_edluth_0 = p_edtab[8];
	local_reg2.bit.reg_edluth_1 = p_edtab[9];
	local_reg2.bit.reg_edluth_2 = p_edtab[10];
	local_reg2.bit.reg_edluth_3 = p_edtab[11];

	local_reg3.reg = 0;
	local_reg3.bit.reg_edluth_4 = p_edtab[12];
	local_reg3.bit.reg_edluth_5 = p_edtab[13];
	local_reg3.bit.reg_edluth_6 = p_edtab[14];
	local_reg3.bit.reg_edluth_7 = p_edtab[15];

	IPE_SETREG(EDGE_DMAP_PROCESS_REGISTER_1_OFS, local_reg0.reg);
	IPE_SETREG(EDGE_DMAP_PROCESS_REGISTER_2_OFS, local_reg1.reg);
	IPE_SETREG(EDGE_DMAP_PROCESS_REGISTER_3_OFS, local_reg2.reg);
	IPE_SETREG(EDGE_DMAP_PROCESS_REGISTER_4_OFS, local_reg3.reg);
#endif

	for (i = 0; i < 4; i += 1) {
		//reg_set_val = 0;

		reg_ofs = EDGE_DMAP_PROCESS_REGISTER_1_OFS + (i << 2);
		reg_set_val = (p_edtab[(i << 2)] & 0xFF) + ((p_edtab[(i << 2) + 1] & 0xFF) << 8) + ((p_edtab[(i << 2) + 2] & 0xFF) << 16) + ((p_edtab[(i << 2) + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

void ipe_set_rgblpf(IPE_RGBLPF_PARAM *p_lpf_info)
{
	T_RGB_LPF_REGISTER_0 local_reg0;
	T_RGB_LPF_REGISTER_1 local_reg1;
	T_RGB_LPF_REGISTER_2 local_reg2;

	local_reg0.reg = 0;
	local_reg0.bit.reg_glpfw = p_lpf_info->lpf_g_info.lpfw;
	local_reg0.bit.reg_gsonlyw = p_lpf_info->lpf_g_info.sonly_w;
	local_reg0.bit.reg_g_rangeth0 = p_lpf_info->lpf_g_info.range_th0;
	local_reg0.bit.reg_g_rangeth1 = p_lpf_info->lpf_g_info.range_th1;
	local_reg0.bit.reg_g_filtsize = p_lpf_info->lpf_g_info.filt_size;

	local_reg1.reg = 0;
	local_reg1.bit.reg_rlpfw = p_lpf_info->lpf_r_info.lpfw;
	local_reg1.bit.reg_rsonlyw = p_lpf_info->lpf_r_info.sonly_w;
	local_reg1.bit.reg_r_rangeth0 = p_lpf_info->lpf_r_info.range_th0;
	local_reg1.bit.reg_r_rangeth1 = p_lpf_info->lpf_r_info.range_th1;
	local_reg1.bit.reg_r_filtsize = p_lpf_info->lpf_r_info.filt_size;

	local_reg2.reg = 0;
	local_reg2.bit.reg_blpfw = p_lpf_info->lpf_b_info.lpfw;
	local_reg2.bit.reg_bsonlyw = p_lpf_info->lpf_b_info.sonly_w;
	local_reg2.bit.reg_b_rangeth0 = p_lpf_info->lpf_b_info.range_th0;
	local_reg2.bit.reg_b_rangeth1 = p_lpf_info->lpf_b_info.range_th1;
	local_reg2.bit.reg_b_filtsize = p_lpf_info->lpf_b_info.filt_size;

	IPE_SETREG(RGB_LPF_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(RGB_LPF_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(RGB_LPF_REGISTER_2_OFS, local_reg2.reg);
}

void ipe_set_pfr_general(IPE_PFR_PARAM *p_pfr_info)
{
	T_PURPLE_FRINGE_REDUCTION_REGISTER0 local_reg0;
	T_PURPLE_FRINGE_REDUCTION_REGISTER1 local_reg1;


	local_reg0.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER0_OFS);
	local_reg0.bit.reg_pfr_uv_filt_en = p_pfr_info->uv_filt_en;
	local_reg0.bit.reg_pfr_luma_level_en = p_pfr_info->luma_level_en;
	local_reg0.bit.reg_pfr_out_wet = p_pfr_info->wet_out;

	local_reg1.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER1_OFS);
	local_reg1.bit.reg_pfr_edge_th = p_pfr_info->edge_th;
	local_reg1.bit.reg_pfr_edge_str = p_pfr_info->edge_strength;
	local_reg1.bit.reg_pfr_g_wet = p_pfr_info->color_wet_g;

	IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER0_OFS, local_reg0.reg);
	IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER1_OFS, local_reg1.reg);
}

void ipe_set_pfr_color(UINT32 idx, IPE_PFR_COLOR_WET *p_pfr_color_info)
{
	UINT32 addr_ofs;
	UINT32 mask = 1;
	T_PURPLE_FRINGE_REDUCTION_REGISTER0 local_reg0;
	T_PURPLE_FRINGE_REDUCTION_REGISTER2 local_reg1;
	T_PURPLE_FRINGE_REDUCTION_REGISTER6 local_reg2;
	T_PURPLE_FRINGE_REDUCTION_REGISTER7 local_reg3;

	local_reg0.reg = IPE_GETREG((PURPLE_FRINGE_REDUCTION_REGISTER0_OFS));
	mask = mask << (2 + idx);
	if (idx < 4) {
		addr_ofs = idx * 8;
	} else {
		return;
	}
	local_reg1.reg = IPE_GETREG((PURPLE_FRINGE_REDUCTION_REGISTER2_OFS + idx * 4));
	local_reg2.reg = IPE_GETREG((PURPLE_FRINGE_REDUCTION_REGISTER6_OFS + addr_ofs));
	local_reg3.reg = IPE_GETREG((PURPLE_FRINGE_REDUCTION_REGISTER7_OFS + addr_ofs));

	if (p_pfr_color_info->enable) {
		local_reg0.reg  = local_reg0.reg | (mask & 0xffffffff);
	} else {
		local_reg0.reg  = local_reg0.reg & (~mask & 0xffffffff);
	}

	local_reg1.bit.reg_pfr_color_u0  = p_pfr_color_info->color_u;
	local_reg1.bit.reg_pfr_color_v0  = p_pfr_color_info->color_v;
	local_reg1.bit.reg_pfr_r_wet0    = p_pfr_color_info->color_wet_r;
	local_reg1.bit.reg_pfr_b_wet0    = p_pfr_color_info->color_wet_b;

	local_reg2.bit.reg_pfr_cdif_set0_lut0    = p_pfr_color_info->cdiff_lut[0];
	local_reg2.bit.reg_pfr_cdif_set0_lut1    = p_pfr_color_info->cdiff_lut[1];
	local_reg2.bit.reg_pfr_cdif_set0_lut2    = p_pfr_color_info->cdiff_lut[2];
	local_reg2.bit.reg_pfr_cdif_set0_lut3    = p_pfr_color_info->cdiff_lut[3];

	local_reg3.bit.reg_pfr_cdif_set0_lut4    = p_pfr_color_info->cdiff_lut[4];
	local_reg3.bit.reg_pfr_cdif_set0_th      = p_pfr_color_info->cdiff_th;
	local_reg3.bit.reg_pfr_cdif_set0_step    = p_pfr_color_info->cdiff_step;

	IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER0_OFS, local_reg0.reg);
	IPE_SETREG((PURPLE_FRINGE_REDUCTION_REGISTER2_OFS + idx * 4), local_reg1.reg);
	IPE_SETREG((PURPLE_FRINGE_REDUCTION_REGISTER6_OFS + addr_ofs), local_reg2.reg);
	IPE_SETREG((PURPLE_FRINGE_REDUCTION_REGISTER7_OFS + addr_ofs), local_reg3.reg);
}

void ipe_set_pfr_luma(UINT32 luma_th, UINT32 *luma_table)
{
	T_PURPLE_FRINGE_REDUCTION_REGISTER14 local_reg0;
	T_PURPLE_FRINGE_REDUCTION_REGISTER15 local_reg1;
	T_PURPLE_FRINGE_REDUCTION_REGISTER16 local_reg2;
	T_PURPLE_FRINGE_REDUCTION_REGISTER17 local_reg3;

	if (luma_table != NULL) {
		local_reg0.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER14_OFS);
		local_reg0.bit.reg_pfr_luma_level_0 = luma_table[0];
		local_reg0.bit.reg_pfr_luma_level_1 = luma_table[1];
		local_reg0.bit.reg_pfr_luma_level_2 = luma_table[2];
		local_reg0.bit.reg_pfr_luma_level_3 = luma_table[3];

		local_reg1.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER15_OFS);
		local_reg1.bit.reg_pfr_luma_level_4 = luma_table[4];
		local_reg1.bit.reg_pfr_luma_level_5 = luma_table[5];
		local_reg1.bit.reg_pfr_luma_level_6 = luma_table[6];
		local_reg1.bit.reg_pfr_luma_level_7 = luma_table[7];

		local_reg2.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER16_OFS);
		local_reg2.bit.reg_pfr_luma_level_8 = luma_table[8];
		local_reg2.bit.reg_pfr_luma_level_9 = luma_table[9];
		local_reg2.bit.reg_pfr_luma_level_10 = luma_table[10];
		local_reg2.bit.reg_pfr_luma_level_11 = luma_table[11];

		local_reg3.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER17_OFS);
		local_reg3.bit.reg_pfr_luma_level_12 = luma_table[12];
		local_reg3.bit.reg_pfr_luma_th = luma_th;
	} else {
		local_reg0.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER14_OFS);
		local_reg1.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER15_OFS);
		local_reg2.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER16_OFS);
		local_reg3.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER17_OFS);
		local_reg3.bit.reg_pfr_luma_th = luma_th;
	}

	IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER14_OFS, local_reg0.reg);
	IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER15_OFS, local_reg1.reg);
	IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER16_OFS, local_reg2.reg);
	IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER17_OFS, local_reg3.reg);
}


/**
    IPE CC matrix setting

    Configure color correction matrix

    @param p_cc_param CC parameters

    @return None.
*/
void ipe_set_color_correct(IPE_CC_PARAM *p_cc_param)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	T_COLOR_CORRECTION_REGISTER_0 local_reg0;
	//T_COLOR_CORRECTION_REGISTER_1 local_reg1;
	//T_COLOR_CORRECTION_REGISTER_2 local_reg2;
	//T_COLOR_CORRECTION_REGISTER_3 local_reg3;
	//T_COLOR_CORRECTION_REGISTER_4 local_reg4;

	local_reg0.reg = 0;
	local_reg0.bit.reg_ccrange = p_cc_param->cc_range;
	local_reg0.bit.reg_cc2_sel = p_cc_param->cc2_sel;
	local_reg0.bit.reg_cc_gamsel = p_cc_param->cc_gam_sel;
	local_reg0.bit.reg_ccstab_sel = p_cc_param->cc_stab_sel;
	local_reg0.bit.reg_ccofs_sel  = p_cc_param->cc_ofs_sel; //sc
	local_reg0.bit.reg_coef_rr = p_cc_param->p_cc_coeff[0];
	IPE_SETREG(COLOR_CORRECTION_REGISTER_0_OFS, local_reg0.reg);

#if 0
	local_reg1.reg = 0;
	local_reg1.bit.reg_coef_rg = p_cc_param->p_cc_coeff[1];
	local_reg1.bit.reg_coef_rb = p_cc_param->p_cc_coeff[2];

	local_reg2.reg = 0;
	local_reg2.bit.reg_coef_gr = p_cc_param->p_cc_coeff[3];
	local_reg2.bit.reg_coef_gg = p_cc_param->p_cc_coeff[4];

	local_reg3.reg = 0;
	local_reg3.bit.reg_coef_gb = p_cc_param->p_cc_coeff[5];
	local_reg3.bit.reg_coef_br = p_cc_param->p_cc_coeff[6];

	local_reg4.reg = 0;
	local_reg4.bit.reg_coef_bg = p_cc_param->p_cc_coeff[7];
	local_reg4.bit.reg_coef_bb = p_cc_param->p_cc_coeff[8];

	IPE_SETREG(COLOR_CORRECTION_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_CORRECTION_REGISTER_2_OFS, local_reg2.reg);
	IPE_SETREG(COLOR_CORRECTION_REGISTER_3_OFS, local_reg3.reg);
	IPE_SETREG(COLOR_CORRECTION_REGISTER_4_OFS, local_reg4.reg);
#endif

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CORRECTION_REGISTER_1_OFS) + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_cc_param->p_cc_coeff[idx + 1] & 0xFFF) + ((p_cc_param->p_cc_coeff[idx + 2] & 0xFFF) << 16);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
    IPE set Fstab table

    IPE color noise reduction luminance mapping.

    @param *fstab16tbl pointer to luminance profile mapping table.

    @return None.
*/
void ipe_set_fstab(UINT8 *p_fstab_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_0 local_reg0;
	T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_1 local_reg1;
	T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_2 local_reg2;
	T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_3 local_reg3;

	local_reg0.reg = 0;
	local_reg0.bit.reg_fstab0 = p_fstab_lut[0];
	local_reg0.bit.reg_fstab1 = p_fstab_lut[1];
	local_reg0.bit.reg_fstab2 = p_fstab_lut[2];
	local_reg0.bit.reg_fstab3 = p_fstab_lut[3];

	local_reg1.reg = 0;
	local_reg1.bit.reg_fstab4 = p_fstab_lut[4];
	local_reg1.bit.reg_fstab5 = p_fstab_lut[5];
	local_reg1.bit.reg_fstab6 = p_fstab_lut[6];
	local_reg1.bit.reg_fstab7 = p_fstab_lut[7];

	local_reg2.reg = 0;
	local_reg2.bit.reg_fstab8 = p_fstab_lut[8];
	local_reg2.bit.reg_fstab9 = p_fstab_lut[9];
	local_reg2.bit.reg_fstab10 = p_fstab_lut[10];
	local_reg2.bit.reg_fstab11 = p_fstab_lut[11];

	local_reg3.reg = 0;
	local_reg3.bit.reg_fstab12 = p_fstab_lut[12];
	local_reg3.bit.reg_fstab13 = p_fstab_lut[13];
	local_reg3.bit.reg_fstab14 = p_fstab_lut[14];
	local_reg3.bit.reg_fstab15 = p_fstab_lut[15];

	IPE_SETREG(COLOR_CORRECTION_STAB_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(COLOR_CORRECTION_STAB_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_CORRECTION_STAB_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	IPE_SETREG(COLOR_CORRECTION_STAB_MAPPING_REGISTER_3_OFS, local_reg3.reg);
#endif

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CORRECTION_STAB_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_fstab_lut[idx] & 0xFF) + ((p_fstab_lut[idx + 1] & 0xFF) << 8) + ((p_fstab_lut[idx + 2] & 0xFF) << 16) + ((p_fstab_lut[idx + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
    IPE set Fdtab table

    IPE color noise reduction saturatioin mapping.

    @param *fdtab16tbl pointer to saturation profile mapping table.

    @return None.
*/
void ipe_set_fdtab(UINT8 *p_fdtab_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0 local_reg0;
	T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1 local_reg1;
	T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2 local_reg2;
	T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3 local_reg3;

	local_reg0.reg = 0;
	local_reg0.bit.reg_fdtab0 = p_fdtab_lut[0];
	local_reg0.bit.reg_fdtab1 = p_fdtab_lut[1];
	local_reg0.bit.reg_fdtab2 = p_fdtab_lut[2];
	local_reg0.bit.reg_fdtab3 = p_fdtab_lut[3];

	local_reg1.reg = 0;
	local_reg1.bit.reg_fdtab4 = p_fdtab_lut[4];
	local_reg1.bit.reg_fdtab5 = p_fdtab_lut[5];
	local_reg1.bit.reg_fdtab6 = p_fdtab_lut[6];
	local_reg1.bit.reg_fdtab7 = p_fdtab_lut[7];

	local_reg2.reg = 0;
	local_reg2.bit.reg_fdtab8 = p_fdtab_lut[8];
	local_reg2.bit.reg_fdtab9 = p_fdtab_lut[9];
	local_reg2.bit.reg_fdtab10 = p_fdtab_lut[10];
	local_reg2.bit.reg_fdtab11 = p_fdtab_lut[11];

	local_reg3.reg = 0;
	local_reg3.bit.reg_fdtab12 = p_fdtab_lut[12];
	local_reg3.bit.reg_fdtab13 = p_fdtab_lut[13];
	local_reg3.bit.reg_fdtab14 = p_fdtab_lut[14];
	local_reg3.bit.reg_fdtab15 = p_fdtab_lut[15];

	IPE_SETREG(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	IPE_SETREG(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3_OFS, local_reg3.reg);
#endif

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_fdtab_lut[idx] & 0xFF) + ((p_fdtab_lut[idx + 1] & 0xFF) << 8) + ((p_fdtab_lut[idx + 2] & 0xFF) << 16) + ((p_fdtab_lut[idx + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
    IPE set CST coefficient

    IPE color space transform coefficient setting.

    @param cst0 CST coefficient 0.
    @param cst1 CST coefficient 1.
    @param cst2 CST coefficient 2.
    @param cst3 CST coefficient 3.

    @return None.
*/
void ipe_set_cst(IPE_CST_PARAM *p_cst)
{
	T_COLOR_SPACE_TRANSFORM_REGISTER0 local_reg0;
	T_COLOR_SPACE_TRANSFORM_REGISTER1 local_reg1;
	T_COLOR_SPACE_TRANSFORM_REGISTER2 local_reg2;

	local_reg0.reg = IPE_GETREG(COLOR_SPACE_TRANSFORM_REGISTER0_OFS);
	local_reg0.bit.reg_coef_yr = p_cst->p_cst_coeff[0];
	local_reg0.bit.reg_coef_yg = p_cst->p_cst_coeff[1];
	local_reg0.bit.reg_coef_yb = p_cst->p_cst_coeff[2];

	local_reg1.reg = IPE_GETREG(COLOR_SPACE_TRANSFORM_REGISTER1_OFS);
	local_reg1.bit.reg_coef_ur = p_cst->p_cst_coeff[3];
	local_reg1.bit.reg_coef_ug = p_cst->p_cst_coeff[4];
	local_reg1.bit.reg_coef_ub = p_cst->p_cst_coeff[5];

#if (defined(_NVT_EMULATION_) == ON)
	local_reg1.bit.reg_cstoff_sel = p_cst->cstoff_sel;
#else
	local_reg1.bit.reg_cstoff_sel = 0x1; // do not support 0x0
#endif

	local_reg2.reg = IPE_GETREG(COLOR_SPACE_TRANSFORM_REGISTER2_OFS);
	local_reg2.bit.reg_coef_vr = p_cst->p_cst_coeff[6];
	local_reg2.bit.reg_coef_vg = p_cst->p_cst_coeff[7];
	local_reg2.bit.reg_coef_vb = p_cst->p_cst_coeff[8];

	IPE_SETREG(COLOR_SPACE_TRANSFORM_REGISTER0_OFS, local_reg0.reg);
	IPE_SETREG(COLOR_SPACE_TRANSFORM_REGISTER1_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_SPACE_TRANSFORM_REGISTER2_OFS, local_reg2.reg);
}

/**
    IPE set Int Sat offset

    Set IPE color control intensity and saturation offset.

    @param int_ofs Intensity offset.
    @param sat_ofs Saturation offset.

    @return None.
*/
void ipe_set_int_sat_offset(INT16 int_ofs, INT16 sat_ofs)
{
	T_COLOR_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(COLOR_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_int_ofs = int_ofs;
	local_reg.bit.reg_sat_ofs = sat_ofs;

	IPE_SETREG(COLOR_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
    IPE set hue rotation enable

    Set IPE color control hue table rotation selection, if rot_en is 1, reg_chuem[n] LSB 2 bit = 0 : 0 degree rotation, 1 : 90 degree rotation, 2 bit = 2 : 180 degree rotation, 3 : 270 degree rotation

    @return None.
*/
void ipe_set_hue_rotate(BOOL hue_rot_en)
{
	T_COLOR_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(COLOR_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_chue_roten = hue_rot_en;

	IPE_SETREG(COLOR_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
    IPE set hue C to G enable

    Set IPE color control hue calculation
    \n 0: input is G channel, 1: input is C channel and convert it to G

    @return None.
*/
void ipe_set_hue_c2g_en(BOOL c2g_en)
{
	T_COLOR_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(COLOR_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_chue_c2gen = c2g_en;

	IPE_SETREG(COLOR_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
    IPE set color suppress

    Set IPE color adjustment Cb Cr color suppress level.

    @param weight color suppress level.

    @return None.
*/
void ipe_set_color_sup(IPE_CCTRL_SEL_ENUM cctrl_sel, UINT8 vdetdiv)
{
	T_COLOR_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(COLOR_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_cctrl_sel = cctrl_sel;
	local_reg.bit.reg_vdet_div = vdetdiv;

	IPE_SETREG(COLOR_CONTROL_REGISTER_OFS, local_reg.reg);

	ipe_enable_feature(IPE_CTRL_EN | IPE_CADJ_EN);
}

/**
    IPE set Hue table

    Set IPE color control Hue mapping.

    @param *hue24tbl pointer to hue mapping table.

    @return None.
*/
void ipe_set_cctrl_hue(UINT8 *p_hue_tbl)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_0 local_reg0;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_1 local_reg1;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_2 local_reg2;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_3 local_reg3;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_4 local_reg4;
	T_COLOR_CONTROL_HUE_MAPPING_REGISTER_5 local_reg5;

	local_reg0.reg = 0;
	local_reg0.bit.reg_chuem0 = p_hue_tbl[0];
	local_reg0.bit.reg_chuem1 = p_hue_tbl[1];
	local_reg0.bit.reg_chuem2 = p_hue_tbl[2];
	local_reg0.bit.reg_chuem3 = p_hue_tbl[3];

	local_reg1.reg = 0;
	local_reg1.bit.reg_chuem4 = p_hue_tbl[4];
	local_reg1.bit.reg_chuem5 = p_hue_tbl[5];
	local_reg1.bit.reg_chuem6 = p_hue_tbl[6];
	local_reg1.bit.reg_chuem7 = p_hue_tbl[7];

	local_reg2.reg = 0;
	local_reg2.bit.reg_chuem8 = p_hue_tbl[8];
	local_reg2.bit.reg_chuem9 = p_hue_tbl[9];
	local_reg2.bit.reg_chuem10 = p_hue_tbl[10];
	local_reg2.bit.reg_chuem11 = p_hue_tbl[11];

	local_reg3.reg = 0;
	local_reg3.bit.reg_chuem12 = p_hue_tbl[12];
	local_reg3.bit.reg_chuem13 = p_hue_tbl[13];
	local_reg3.bit.reg_chuem14 = p_hue_tbl[14];
	local_reg3.bit.reg_chuem15 = p_hue_tbl[15];

	local_reg4.reg = 0;
	local_reg4.bit.reg_chuem16 = p_hue_tbl[16];
	local_reg4.bit.reg_chuem17 = p_hue_tbl[17];
	local_reg4.bit.reg_chuem18 = p_hue_tbl[18];
	local_reg4.bit.reg_chuem19 = p_hue_tbl[19];

	local_reg5.reg = 0;
	local_reg5.bit.reg_chuem20 = p_hue_tbl[20];
	local_reg5.bit.reg_chuem21 = p_hue_tbl[21];
	local_reg5.bit.reg_chuem22 = p_hue_tbl[22];
	local_reg5.bit.reg_chuem23 = p_hue_tbl[23];

	IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_3_OFS, local_reg3.reg);
	IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_4_OFS, local_reg4.reg);
	IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_5_OFS, local_reg5.reg);
#endif

	for (i = 0; i < 6; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CONTROL_HUE_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_hue_tbl[idx] & 0xFF) + ((p_hue_tbl[idx + 1] & 0xFF) << 8) + ((p_hue_tbl[idx + 2] & 0xFF) << 16) + ((p_hue_tbl[idx + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
    IPE set Int table

    Set IPE color control Intensity mapping.

    @param *intensity24tbl pointer to intensity mapping table.

    @return None.
*/
void ipe_set_cctrl_int(INT32 *p_int_tbl)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0 local_reg0;
	T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_1 local_reg1;
	T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_2 local_reg2;
	T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_3 local_reg3;
	T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_4 local_reg4;
	T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_5 local_reg5;

	local_reg0.reg = 0;
	local_reg0.bit.reg_cintm0 = p_int_tbl[0];
	local_reg0.bit.reg_cintm1 = p_int_tbl[1];
	local_reg0.bit.reg_cintm2 = p_int_tbl[2];
	local_reg0.bit.reg_cintm3 = p_int_tbl[3];

	local_reg1.reg = 0;
	local_reg1.bit.reg_cintm4 = p_int_tbl[4];
	local_reg1.bit.reg_cintm5 = p_int_tbl[5];
	local_reg1.bit.reg_cintm6 = p_int_tbl[6];
	local_reg1.bit.reg_cintm7 = p_int_tbl[7];

	local_reg2.reg = 0;
	local_reg2.bit.reg_cintm8 = p_int_tbl[8];
	local_reg2.bit.reg_cintm9 = p_int_tbl[9];
	local_reg2.bit.reg_cintm10 = p_int_tbl[10];
	local_reg2.bit.reg_cintm11 = p_int_tbl[11];

	local_reg3.reg = 0;
	local_reg3.bit.reg_cintm12 = p_int_tbl[12];
	local_reg3.bit.reg_cintm13 = p_int_tbl[13];
	local_reg3.bit.reg_cintm14 = p_int_tbl[14];
	local_reg3.bit.reg_cintm15 = p_int_tbl[15];

	local_reg4.reg = 0;
	local_reg4.bit.reg_cintm16 = p_int_tbl[16];
	local_reg4.bit.reg_cintm17 = p_int_tbl[17];
	local_reg4.bit.reg_cintm18 = p_int_tbl[18];
	local_reg4.bit.reg_cintm19 = p_int_tbl[19];

	local_reg5.reg = 0;
	local_reg5.bit.reg_cintm20 = p_int_tbl[20];
	local_reg5.bit.reg_cintm21 = p_int_tbl[21];
	local_reg5.bit.reg_cintm22 = p_int_tbl[22];
	local_reg5.bit.reg_cintm23 = p_int_tbl[23];

	IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_3_OFS, local_reg3.reg);
	IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_4_OFS, local_reg4.reg);
	IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_5_OFS, local_reg5.reg);
#endif

	for (i = 0; i < 6; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_int_tbl[idx] & 0xFF) + ((p_int_tbl[idx + 1] & 0xFF) << 8) + ((p_int_tbl[idx + 2] & 0xFF) << 16) + ((p_int_tbl[idx + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
    IPE set Sat table

    Set IPE color control Saturation mapping.

    @param *sat24tbl pointer to saturation mapping table.

    @return None.
*/
void ipe_set_cctrl_sat(INT32 *p_sat_tbl)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0 local_reg0;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1 local_reg1;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2 local_reg2;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3 local_reg3;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4 local_reg4;
	T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5 local_reg5;

	local_reg0.reg = 0;
	local_reg0.bit.reg_csatm0 = p_sat_tbl[0];
	local_reg0.bit.reg_csatm1 = p_sat_tbl[1];
	local_reg0.bit.reg_csatm2 = p_sat_tbl[2];
	local_reg0.bit.reg_csatm3 = p_sat_tbl[3];

	local_reg1.reg = 0;
	local_reg1.bit.reg_csatm4 = p_sat_tbl[4];
	local_reg1.bit.reg_csatm5 = p_sat_tbl[5];
	local_reg1.bit.reg_csatm6 = p_sat_tbl[6];
	local_reg1.bit.reg_csatm7 = p_sat_tbl[7];

	local_reg2.reg = 0;
	local_reg2.bit.reg_csatm8 = p_sat_tbl[8];
	local_reg2.bit.reg_csatm9 = p_sat_tbl[9];
	local_reg2.bit.reg_csatm10 = p_sat_tbl[10];
	local_reg2.bit.reg_csatm11 = p_sat_tbl[11];

	local_reg3.reg = 0;
	local_reg3.bit.reg_csatm12 = p_sat_tbl[12];
	local_reg3.bit.reg_csatm13 = p_sat_tbl[13];
	local_reg3.bit.reg_csatm14 = p_sat_tbl[14];
	local_reg3.bit.reg_csatm15 = p_sat_tbl[15];

	local_reg4.reg = 0;
	local_reg4.bit.reg_csatm16 = p_sat_tbl[16];
	local_reg4.bit.reg_csatm17 = p_sat_tbl[17];
	local_reg4.bit.reg_csatm18 = p_sat_tbl[18];
	local_reg4.bit.reg_csatm19 = p_sat_tbl[19];

	local_reg5.reg = 0;
	local_reg5.bit.reg_csatm20 = p_sat_tbl[20];
	local_reg5.bit.reg_csatm21 = p_sat_tbl[21];
	local_reg5.bit.reg_csatm22 = p_sat_tbl[22];
	local_reg5.bit.reg_csatm23 = p_sat_tbl[23];

	IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3_OFS, local_reg3.reg);
	IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4_OFS, local_reg4.reg);
	IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5_OFS, local_reg5.reg);
#endif

	for (i = 0; i < 6; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_sat_tbl[idx] & 0xFF) + ((p_sat_tbl[idx + 1] & 0xFF) << 8) + ((p_sat_tbl[idx + 2] & 0xFF) << 16) + ((p_sat_tbl[(i << 2) + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
    IPE set Edge table

    Set IPE color control Edge mapping.

    @param *edg24tbl pointer to edge mapping table.

    @return None.
*/
void ipe_set_cctrl_edge(UINT8 *p_edge_tbl)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_0 local_reg0;
	T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_1 local_reg1;
	T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_2 local_reg2;
	T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_3 local_reg3;
	T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_4 local_reg4;
	T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_5 local_reg5;

	local_reg0.reg = 0;
	local_reg0.bit.reg_cedgem0 = p_edge_tbl[0];
	local_reg0.bit.reg_cedgem1 = p_edge_tbl[1];
	local_reg0.bit.reg_cedgem2 = p_edge_tbl[2];
	local_reg0.bit.reg_cedgem3 = p_edge_tbl[3];

	local_reg1.reg = 0;
	local_reg1.bit.reg_cedgem4 = p_edge_tbl[4];
	local_reg1.bit.reg_cedgem5 = p_edge_tbl[5];
	local_reg1.bit.reg_cedgem6 = p_edge_tbl[6];
	local_reg1.bit.reg_cedgem7 = p_edge_tbl[7];

	local_reg2.reg = 0;
	local_reg2.bit.reg_cedgem8 = p_edge_tbl[8];
	local_reg2.bit.reg_cedgem9 = p_edge_tbl[9];
	local_reg2.bit.reg_cedgem10 = p_edge_tbl[10];
	local_reg2.bit.reg_cedgem11 = p_edge_tbl[11];

	local_reg3.reg = 0;
	local_reg3.bit.reg_cedgem12 = p_edge_tbl[12];
	local_reg3.bit.reg_cedgem13 = p_edge_tbl[13];
	local_reg3.bit.reg_cedgem14 = p_edge_tbl[14];
	local_reg3.bit.reg_cedgem15 = p_edge_tbl[15];

	local_reg4.reg = 0;
	local_reg4.bit.reg_cedgem16 = p_edge_tbl[16];
	local_reg4.bit.reg_cedgem17 = p_edge_tbl[17];
	local_reg4.bit.reg_cedgem18 = p_edge_tbl[18];
	local_reg4.bit.reg_cedgem19 = p_edge_tbl[19];

	local_reg5.reg = 0;
	local_reg5.bit.reg_cedgem20 = p_edge_tbl[20];
	local_reg5.bit.reg_cedgem21 = p_edge_tbl[21];
	local_reg5.bit.reg_cedgem22 = p_edge_tbl[22];
	local_reg5.bit.reg_cedgem23 = p_edge_tbl[23];

	IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_3_OFS, local_reg3.reg);
	IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_4_OFS, local_reg4.reg);
	IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_5_OFS, local_reg5.reg);
#endif

	for (i = 0; i < 6; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CONTROL_EDGE_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_edge_tbl[idx] & 0xFF) + ((p_edge_tbl[idx + 1] & 0xFF) << 8) + ((p_edge_tbl[idx + 2] & 0xFF) << 16) + ((p_edge_tbl[idx + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
    IPE set DDS table

    Set IPE color control DDS table.

    @param *p_dds_tbl pointer to DDS table.

    @return None.
*/
void ipe_set_cctrl_dds(UINT8 *p_dds_tbl)
{
	T_COLOR_CONTROL_DDS_REGISTER_0 local_reg0;
	T_COLOR_CONTROL_DDS_REGISTER_1 local_reg1;

	local_reg0.reg = 0;
	local_reg0.bit.reg_dds0 = p_dds_tbl[0];
	local_reg0.bit.reg_dds1 = p_dds_tbl[1];
	local_reg0.bit.reg_dds2 = p_dds_tbl[2];
	local_reg0.bit.reg_dds3 = p_dds_tbl[3];

	local_reg1.reg = 0;
	local_reg1.bit.reg_dds4 = p_dds_tbl[4];
	local_reg1.bit.reg_dds5 = p_dds_tbl[5];
	local_reg1.bit.reg_dds6 = p_dds_tbl[6];
	local_reg1.bit.reg_dds7 = p_dds_tbl[7];

	IPE_SETREG(COLOR_CONTROL_DDS_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(COLOR_CONTROL_DDS_REGISTER_1_OFS, local_reg1.reg);
}

/**
    IPE set edge enhance

    Set IPE positive and negative edge enhance.

    @param yenhp positive edge enhance level.
    @param yenhn negative edge enhance level.

    @return None.
*/
void ipe_set_edge_enhance(UINT32 y_enh_p, UINT32 y_enh_n)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0_OFS);
	local_reg.bit.reg_y_enh_p = y_enh_p;
	local_reg.bit.reg_y_enh_n = y_enh_n;

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE set edge invert

    Set IPE positive and negative edge invert.

    @param einvp positive edge sign invert selection.
    @param einvn negative edge sign invert selection.

    @return None.
*/
void ipe_set_edge_inv(BOOL b_einv_p, BOOL b_einv_n)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0_OFS);
	local_reg.bit.reg_y_einv_p = b_einv_p;
	local_reg.bit.reg_y_einv_n = b_einv_n;

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0_OFS, local_reg.reg);
}

/**
    IPE set YCbCr contrast

    IPE color adjustment contrast setting.

    @param YCon Y contrast parameter 0~255.

    @return None.
*/
void ipe_set_ycon(UINT32 con_y)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_1 local_reg1;

	local_reg1.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS);
	local_reg1.bit.reg_y_con = con_y;

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS, local_reg1.reg);
}


/**
    IPE set YCbCr random

    IPE color adjustment YCbCr random noise level setting.

    @param p_ycrand YCbCr random infor.

    @return None.
*/
void ipe_set_yc_rand(IPE_CADJ_RAND_PARAM *p_ycrand)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_1 local_reg1;
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_5 local_reg2;

	local_reg1.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS);
	local_reg1.bit.reg_y_rand_en = p_ycrand->yrand_en;
	local_reg1.bit.reg_y_rand = p_ycrand->yrand_level;
	local_reg1.bit.reg_yc_randreset = p_ycrand->yc_rand_rst;

	local_reg2.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS);
	local_reg2.bit.reg_c_rand_en = p_ycrand->crand_en;
	local_reg2.bit.reg_c_rand = p_ycrand->crand_level;

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS, local_reg2.reg);
}

/**
    IPE set edge threshold for YCbCr fix replace

    Set IPE edge abs threshold for fixed YCbCr replacement.

    @param ethy Edge abs threshold for Y component replacement.
    @param ethc Edge abs threshold for CbCr component replacement.

    @return None.
*/
void ipe_set_yfix_th(STR_YTH1_INFOR *p_yth1, STR_YTH2_INFOR *p_yth2)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_1 local_reg1;
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_2 local_reg2;
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_3 local_reg3;

	local_reg1.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS);
	local_reg1.bit.reg_y_ethy = p_yth1->edgeth;

	local_reg2.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_2_OFS);
	local_reg2.bit.reg_y_yth1 = p_yth1->y_th;
	local_reg2.bit.reg_y_hit1sel = p_yth1->hit_sel;
	local_reg2.bit.reg_y_nhit1sel = p_yth1->nhit_sel;
	local_reg2.bit.reg_y_fix1_hit = p_yth1->hit_value;
	local_reg2.bit.reg_y_fix1_nhit = p_yth1->nhit_value;

	local_reg3.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_3_OFS);
	local_reg3.bit.reg_y_yth2 = p_yth2->y_th;
	local_reg3.bit.reg_y_hit2sel = p_yth2->hit_sel;
	local_reg3.bit.reg_y_nhit2sel = p_yth2->nhit_sel;
	local_reg3.bit.reg_y_fix2_hit = p_yth2->hit_value;
	local_reg3.bit.reg_y_fix2_nhit = p_yth2->nhit_value;

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_2_OFS, local_reg2.reg);
	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_3_OFS, local_reg3.reg);
}

void ipe_set_yc_mask(UINT8 mask_y, UINT8 mask_cb, UINT8 mask_cr)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_4 local_reg;

	local_reg.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_4_OFS);
	local_reg.bit.reg_ymask = mask_y;
	local_reg.bit.reg_cbmask = mask_cb;
	local_reg.bit.reg_crmask = mask_cr;

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_4_OFS, local_reg.reg);
}

/**
    IPE set CbCr offset

    IPE color adjustment CbCr offset setting.

    @param cbofs Cb component offset.
    @param crofs Cr component offset.

    @return None.
*/
void ipe_set_cbcr_offset(UINT32 cb_ofs, UINT32 cr_ofs)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_5 local_reg;

	local_reg.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS);
	local_reg.bit.reg_c_cbofs = cb_ofs;
	local_reg.bit.reg_c_crofs = cr_ofs;

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS, local_reg.reg);
}

void ipe_set_cbcr_con(UINT32 con_c)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_5 local_reg;

	local_reg.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS);
	local_reg.bit.reg_c_con = con_c;

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS, local_reg.reg);
}


void ipe_set_cbcr_contab_sel(IPE_CCONTAB_SEL ccontab_sel)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_14 local_reg5;

	local_reg5.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS);
	local_reg5.bit.reg_ccontab_sel = ccontab_sel;

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS, local_reg5.reg);
}

void ipe_set_cbcr_contab(UINT32 *p_ccontab)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_9  local_reg0;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_10 local_reg1;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_11 local_reg2;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_12 local_reg3;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_13 local_reg4;
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_14 local_reg5;

#if 0
	local_reg0.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_9_OFS);
	local_reg0.bit.reg_ccontab0 = p_ccontab[0];
	local_reg0.bit.reg_ccontab1 = p_ccontab[1];
	local_reg0.bit.reg_ccontab2 = p_ccontab[2];

	local_reg1.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_10_OFS);
	local_reg1.bit.reg_ccontab3 = p_ccontab[3];
	local_reg1.bit.reg_ccontab4 = p_ccontab[4];
	local_reg1.bit.reg_ccontab5 = p_ccontab[5];

	local_reg2.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_11_OFS);
	local_reg2.bit.reg_ccontab6 = p_ccontab[6];
	local_reg2.bit.reg_ccontab7 = p_ccontab[7];
	local_reg2.bit.reg_ccontab8 = p_ccontab[8];

	local_reg3.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_12_OFS);
	local_reg3.bit.reg_ccontab9 = p_ccontab[9];
	local_reg3.bit.reg_ccontab10 = p_ccontab[10];
	local_reg3.bit.reg_ccontab11 = p_ccontab[11];

	local_reg4.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_13_OFS);
	local_reg4.bit.reg_ccontab12 = p_ccontab[12];
	local_reg4.bit.reg_ccontab13 = p_ccontab[13];
	local_reg4.bit.reg_ccontab14 = p_ccontab[14];

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_9_OFS, local_reg0.reg);
	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_10_OFS, local_reg1.reg);
	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_11_OFS, local_reg2.reg);
	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_12_OFS, local_reg3.reg);
	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_13_OFS, local_reg4.reg);
#endif

	for (i = 0; i < 5; i++) {
		//reg_set_val = 0;

		reg_ofs = COLOR_COMPONENT_ADJUSTMENT_REGISTER_9_OFS + (i << 2);

		idx = (i * 3);
		reg_set_val = (p_ccontab[idx] & 0x3FF) + ((p_ccontab[idx + 1] & 0x3FF) << 10) + ((p_ccontab[idx + 2] & 0x3FF) << 20);

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	local_reg5.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS);
	local_reg5.bit.reg_ccontab15 = p_ccontab[15];
	local_reg5.bit.reg_ccontab16 = p_ccontab[16];
	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS, local_reg5.reg);
}

/**
    IPE set YCbCr threshold for CbCr fix replace

    Set IPE YCbCr threshold for fixed CbCr replacement.

    @param yth Y high low threshold for CbCr replacement.
    @param cbth Cb high low threshold for CbCr replacement.
    @param crth Cr high low threshold for CbCr replacement.

    @return None.
*/
void ipe_set_cbcr_fixth(STR_CTH_INFOR *p_cth)
{
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_6 local_reg6;
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_7 local_reg7;
	T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_8 local_reg8;

	local_reg6.reg = 0;
	local_reg6.bit.reg_cb_fix_hit = p_cth->cb_hit_value;
	local_reg6.bit.reg_cb_fix_nhit = p_cth->cb_nhit_value;
	local_reg6.bit.reg_cr_fix_hit = p_cth->cr_hit_value;
	local_reg6.bit.reg_cr_fix_nhit = p_cth->cr_nhit_value;

	local_reg7.reg = 0;
	local_reg7.bit.reg_c_eth = p_cth->edgeth;
	local_reg7.bit.reg_c_hitsel = p_cth->hit_sel;
	local_reg7.bit.reg_c_nhitsel = p_cth->nhit_sel;
	local_reg7.bit.reg_c_yth_h = p_cth->yth_high;
	local_reg7.bit.reg_c_yth_l = p_cth->yth_low;

	local_reg8.reg = 0;
	local_reg8.bit.reg_c_cbth_h = p_cth->cbth_high;
	local_reg8.bit.reg_c_cbth_l = p_cth->cbth_low;
	local_reg8.bit.reg_c_crth_h = p_cth->crth_high;
	local_reg8.bit.reg_c_crth_l = p_cth->crth_low;

	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_6_OFS, local_reg6.reg);
	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_7_OFS, local_reg7.reg);
	IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_8_OFS, local_reg8.reg);
}

void ipe_set_cstp(UINT32 ratio)
{
	T_COLOR_SPACE_TRANSFORM_REGISTER0 local_reg;

	local_reg.reg = IPE_GETREG(COLOR_SPACE_TRANSFORM_REGISTER0_OFS);
	local_reg.bit.reg_cstp_rat = ratio;

	IPE_SETREG(COLOR_SPACE_TRANSFORM_REGISTER0_OFS, local_reg.reg);
}

void ipe_set_ycurve_sel(IPE_YCURVE_SEL ycurve_sel)
{
	T_YCURVE_REFISTER local_reg;

	local_reg.reg = IPE_GETREG(YCURVE_REFISTER_OFS);
	local_reg.bit.reg_ycurve_sel = ycurve_sel;

	IPE_SETREG(YCURVE_REFISTER_OFS, local_reg.reg);
}

void ipe_set_gam_y_rand(IPE_GAMYRAND *p_gamy_rand)
{
	T_GAMMA_YCURVE_REGISTER local_reg0;

	local_reg0.reg = IPE_GETREG(GAMMA_YCURVE_REGISTER_OFS);
	local_reg0.bit.reg_gamyrand_en = p_gamy_rand->gam_y_rand_en;
	local_reg0.bit.reg_gamyrand_reset = p_gamy_rand->gam_y_rand_rst;
	local_reg0.bit.reg_gamyrand_shf = p_gamy_rand->gam_y_rand_shft;

	IPE_SETREG(GAMMA_YCURVE_REGISTER_OFS, local_reg0.reg);
}

void ipe_set_defog_rand(IPE_DEFOGROUND *p_defog_rnd)
{
	T_GAMMA_YCURVE_REGISTER local_reg0;

	local_reg0.reg = IPE_GETREG(GAMMA_YCURVE_REGISTER_OFS);
	local_reg0.bit.reg_defogrnd_opt = p_defog_rnd->rand_opt;
	local_reg0.bit.reg_defogrnd_rst = p_defog_rnd->rand_rst;

	IPE_SETREG(GAMMA_YCURVE_REGISTER_OFS, local_reg0.reg);
}

void ipe_set_edge_dbg_sel(IPE_EDGEDBG_SEL mode_sel)
{
	T_EDGE_DEBUG_REGISTER local_reg0;

	local_reg0.reg = IPE_GETREG(EDGE_DEBUG_REGISTER_OFS);
	local_reg0.bit.reg_edge_dbg_modesel = mode_sel;

	IPE_SETREG(EDGE_DEBUG_REGISTER_OFS, local_reg0.reg);
}

ER ipe_get_burst_length(IPE_BURST_LENGTH *p_burstlen)
{
	T_IPE_MODE_REGISTER_0 local_reg0;

	local_reg0.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	p_burstlen->burstlen_in_y = (IPE_BURST_SEL)local_reg0.bit.reg_iny_burst_mode;
	p_burstlen->burstlen_in_c = (IPE_BURST_SEL)local_reg0.bit.reg_inc_burst_mode;
	p_burstlen->burstlen_out_y = (IPE_BURST_SEL)local_reg0.bit.reg_outy_burst_mode;
	p_burstlen->burstlen_out_c = (IPE_BURST_SEL)local_reg0.bit.reg_outc_burst_mode;

	return E_OK;
}

void ipe_set_eth_size(IPE_ETH_SIZE *p_eth)
{
	T_EDGE_THRESHOLD_REGISTER_0 local_reg0;
	T_EDGE_THRESHOLD_REGISTER_1 local_reg1;

	local_reg0.reg = IPE_GETREG(EDGE_THRESHOLD_REGISTER_0_OFS);
	local_reg1.reg = IPE_GETREG(EDGE_THRESHOLD_REGISTER_1_OFS);

	local_reg1.bit.reg_eth_outsel = p_eth->eth_outsel;
	local_reg0.bit.reg_eth_outfmt = p_eth->eth_outfmt;

	IPE_SETREG(EDGE_THRESHOLD_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(EDGE_THRESHOLD_REGISTER_1_OFS, local_reg1.reg);
}

void ipe_set_eth_param(IPE_ETH_PARAM *p_eth)
{
	T_EDGE_THRESHOLD_REGISTER_0 local_reg0;
	T_EDGE_THRESHOLD_REGISTER_1 local_reg1;

	local_reg0.reg = IPE_GETREG(EDGE_THRESHOLD_REGISTER_0_OFS);
	local_reg1.reg = IPE_GETREG(EDGE_THRESHOLD_REGISTER_1_OFS);

	local_reg0.bit.reg_ethlow = p_eth->eth_low;
	local_reg0.bit.reg_ethmid = p_eth->eth_mid;
	local_reg0.bit.reg_ethhigh = p_eth->eth_high;

	local_reg1.bit.reg_hout_sel = p_eth->eth_hout;
	local_reg1.bit.reg_vout_sel = p_eth->eth_vout;

	IPE_SETREG(EDGE_THRESHOLD_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(EDGE_THRESHOLD_REGISTER_1_OFS, local_reg1.reg);
}

void ipe_set_dma_out_va_addr(UINT32 sao)
{
	T_IPE_TO_DMA_VA_CHANNEL_REGISTER_0 local_reg;

	if ((sao == 0)) {
		DBG_ERR("IMG Input addresses cannot be 0x0!\r\n");
		return;
	}

	local_reg.reg = IPE_GETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_0_OFS);
	local_reg.bit.reg_dram_sao_va = sao >> 2;

	IPE_SETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_0_OFS, local_reg.reg);
}

void ipe_set_dma_out_va_offset(UINT32 ofso)
{
	T_IPE_TO_DMA_VA_CHANNEL_REGISTER_1 local_reg;

	local_reg.reg = IPE_GETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_1_OFS);
	local_reg.bit.reg_dram_ofso_va = ofso >> 2;

	IPE_SETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_1_OFS, local_reg.reg);
}


void ipe_set_va_out_sel(BOOL en)
{
	T_VARIATION_ACCUMULATION_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS);
	local_reg.bit.reg_vacc_outsel = en;

	IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS, local_reg.reg);
}

void ipe_set_va_filter_g1(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g1)
{
	T_VARIATION_DETECTION_GROUP1_REGISTER_0 local_reg0;
	T_VARIATION_DETECTION_GROUP1_REGISTER_1 local_reg1;
	T_VARIATION_ACCUMULATION_REGISTER_1 local_reg2;

	local_reg0.reg = IPE_GETREG(VARIATION_DETECTION_GROUP1_REGISTER_0_OFS);
	local_reg1.reg = IPE_GETREG(VARIATION_DETECTION_GROUP1_REGISTER_1_OFS);
	local_reg2.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_1_OFS);

	local_reg0.bit.reg_vdet_gh1_a = p_va_fltr_g1->filt_h.tap_a;
	local_reg0.bit.reg_vdet_gh1_b = p_va_fltr_g1->filt_h.tap_b;
	local_reg0.bit.reg_vdet_gh1_c = p_va_fltr_g1->filt_h.tap_c;
	local_reg0.bit.reg_vdet_gh1_d = p_va_fltr_g1->filt_h.tap_d;
	local_reg0.bit.reg_vdet_gh1_bcd_op = p_va_fltr_g1->filt_h.filt_symm;
	local_reg0.bit.reg_vdet_gh1_fsize = p_va_fltr_g1->filt_h.fltr_size;
	local_reg0.bit.reg_vdet_gh1_div = p_va_fltr_g1->filt_h.div;

	local_reg1.bit.reg_vdet_gv1_a = p_va_fltr_g1->filt_v.tap_a;
	local_reg1.bit.reg_vdet_gv1_b = p_va_fltr_g1->filt_v.tap_b;
	local_reg1.bit.reg_vdet_gv1_c = p_va_fltr_g1->filt_v.tap_c;
	local_reg1.bit.reg_vdet_gv1_d = p_va_fltr_g1->filt_v.tap_d;
	local_reg1.bit.reg_vdet_gv1_bcd_op = p_va_fltr_g1->filt_v.filt_symm;
	local_reg1.bit.reg_vdet_gv1_fsize = p_va_fltr_g1->filt_v.fltr_size;
	local_reg1.bit.reg_vdet_gv1_div = p_va_fltr_g1->filt_v.div;

	local_reg2.bit.reg_va_g1h_thl = p_va_fltr_g1->filt_h.th_low;
	local_reg2.bit.reg_va_g1h_thh = p_va_fltr_g1->filt_h.th_high;
	local_reg2.bit.reg_va_g1v_thl = p_va_fltr_g1->filt_v.th_low;
	local_reg2.bit.reg_va_g1v_thh = p_va_fltr_g1->filt_v.th_high;

	IPE_SETREG(VARIATION_DETECTION_GROUP1_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(VARIATION_DETECTION_GROUP1_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_1_OFS, local_reg2.reg);
}

void ipe_set_va_filter_g2(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g2)
{
	T_VARIATION_DETECTION_GROUP2_REGISTER_0 local_reg0;
	T_VARIATION_DETECTION_GROUP2_REGISTER_1 local_reg1;
	T_VARIATION_ACCUMULATION_REGISTER_2 local_reg2;

	local_reg0.reg = IPE_GETREG(VARIATION_DETECTION_GROUP2_REGISTER_0_OFS);
	local_reg1.reg = IPE_GETREG(VARIATION_DETECTION_GROUP2_REGISTER_1_OFS);
	local_reg2.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_2_OFS);

	local_reg0.bit.reg_vdet_gh2_a = p_va_fltr_g2->filt_h.tap_a;
	local_reg0.bit.reg_vdet_gh2_b = p_va_fltr_g2->filt_h.tap_b;
	local_reg0.bit.reg_vdet_gh2_c = p_va_fltr_g2->filt_h.tap_c;
	local_reg0.bit.reg_vdet_gh2_d = p_va_fltr_g2->filt_h.tap_d;
	local_reg0.bit.reg_vdet_gh2_bcd_op = p_va_fltr_g2->filt_h.filt_symm;
	local_reg0.bit.reg_vdet_gh2_fsize = p_va_fltr_g2->filt_h.fltr_size;
	local_reg0.bit.reg_vdet_gh2_div = p_va_fltr_g2->filt_h.div;

	local_reg1.bit.reg_vdet_gv2_a = p_va_fltr_g2->filt_v.tap_a;
	local_reg1.bit.reg_vdet_gv2_b = p_va_fltr_g2->filt_v.tap_b;
	local_reg1.bit.reg_vdet_gv2_c = p_va_fltr_g2->filt_v.tap_c;
	local_reg1.bit.reg_vdet_gv2_d = p_va_fltr_g2->filt_v.tap_d;
	local_reg1.bit.reg_vdet_gv2_bcd_op = p_va_fltr_g2->filt_v.filt_symm;
	local_reg1.bit.reg_vdet_gv2_fsize = p_va_fltr_g2->filt_v.fltr_size;
	local_reg1.bit.reg_vdet_gv2_div = p_va_fltr_g2->filt_v.div;

	local_reg2.bit.reg_va_g2h_thl = p_va_fltr_g2->filt_h.th_low;
	local_reg2.bit.reg_va_g2h_thh = p_va_fltr_g2->filt_h.th_high;
	local_reg2.bit.reg_va_g2v_thl = p_va_fltr_g2->filt_v.th_low;
	local_reg2.bit.reg_va_g2v_thh = p_va_fltr_g2->filt_v.th_high;

	IPE_SETREG(VARIATION_DETECTION_GROUP2_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(VARIATION_DETECTION_GROUP2_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_2_OFS, local_reg2.reg);
}

void ipe_set_va_mode_enable(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g1, IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g2)
{
	T_VARIATION_ACCUMULATION_REGISTER_3 local_reg0;

	local_reg0.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_3_OFS);

	if (p_va_fltr_g1 != NULL) {
		local_reg0.bit.reg_va_g1_line_max_mode = p_va_fltr_g1->linemax_en;
		local_reg0.bit.reg_va_g1_cnt_en = p_va_fltr_g1->cnt_en;
	}
	if (p_va_fltr_g2 != NULL) {
		local_reg0.bit.reg_va_g2_line_max_mode = p_va_fltr_g2->linemax_en;
		local_reg0.bit.reg_va_g2_cnt_en = p_va_fltr_g2->cnt_en;
	}

	IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_3_OFS, local_reg0.reg);
}


void ipe_set_va_win_info(IPE_VA_WIN_PARAM *p_va_win)
{

	T_VARIATION_ACCUMULATION_REGISTER_0 local_reg0;
	T_VARIATION_ACCUMULATION_REGISTER_3 local_reg1;
	T_VARIATION_ACCUMULATION_REGISTER_4 local_reg2;

	va_ring_setting[0].win_num_x = p_va_win->win_numx;
	va_ring_setting[0].win_num_y = p_va_win->win_numy;

	local_reg0.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS);
	local_reg1.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_3_OFS);
	local_reg2.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_4_OFS);

	local_reg0.bit.reg_va_stx = p_va_win->win_stx;
	local_reg0.bit.reg_va_sty = p_va_win->win_sty;

	local_reg1.bit.reg_va_win_szx = p_va_win->win_szx;
	local_reg1.bit.reg_va_win_szy = p_va_win->win_szy;

	local_reg2.bit.reg_va_win_numx = p_va_win->win_numx - 1;
	local_reg2.bit.reg_va_win_numy = p_va_win->win_numy - 1;
	local_reg2.bit.reg_va_win_skipx = p_va_win->win_spx;
	local_reg2.bit.reg_va_win_skipy = p_va_win->win_spy;

	IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_3_OFS, local_reg1.reg);
	IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_4_OFS, local_reg2.reg);
}

void ipe_set_va_indep_win(IPE_INDEP_VA_PARAM  *p_indep_va_win_info, UINT32 win_idx)
{
	UINT32 addr_ofs;
	T_INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0 local_reg1;
	T_INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1 local_reg2;

	if (win_idx < 5) {
		addr_ofs = win_idx * 8;
	} else {
		return;
	}
	local_reg1.reg = IPE_GETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0_OFS + addr_ofs)); //no need//
	local_reg2.reg = IPE_GETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1_OFS + addr_ofs)); //no need//

	local_reg1.bit.reg_win0_stx    = p_indep_va_win_info->win_stx;
	local_reg1.bit.reg_win0_sty    = p_indep_va_win_info->win_sty;

	local_reg2.bit.reg_win0_g1_line_max_mode    = p_indep_va_win_info->linemax_g1_en;
	local_reg2.bit.reg_win0_g2_line_max_mode    = p_indep_va_win_info->linemax_g2_en;
	local_reg2.bit.reg_win0_hsz    = p_indep_va_win_info->win_szx;
	local_reg2.bit.reg_win0_vsz    = p_indep_va_win_info->win_szy;

	IPE_SETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0_OFS + addr_ofs), local_reg1.reg);
	IPE_SETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1_OFS + addr_ofs), local_reg2.reg);
}

UINT32 ipe_get_va_out_addr(void)
{
	T_IPE_TO_DMA_VA_CHANNEL_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_0_OFS);
	return local_reg.bit.reg_dram_sao_va << 2;
}

UINT32 ipe_get_va_out_lofs(void)
{
	T_IPE_TO_DMA_VA_CHANNEL_REGISTER_1 local_reg;

	local_reg.reg = IPE_GETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_1_OFS);
	return local_reg.bit.reg_dram_ofso_va << 2;
}

BOOL ipe_get_va_outsel(void)
{
	T_VARIATION_ACCUMULATION_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS);
	return local_reg.bit.reg_vacc_outsel;
}

void ipe_get_win_info(IPE_VA_WIN_PARAM *p_va_win)
{
	T_VARIATION_ACCUMULATION_REGISTER_0 local_reg0;
	T_VARIATION_ACCUMULATION_REGISTER_3 local_reg1;
	T_VARIATION_ACCUMULATION_REGISTER_4 local_reg2;

	local_reg0.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS);
	local_reg1.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_3_OFS);
	local_reg2.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_4_OFS);

	p_va_win->win_stx = local_reg0.bit.reg_va_stx;
	p_va_win->win_sty = local_reg0.bit.reg_va_sty;
	p_va_win->win_szx = local_reg1.bit.reg_va_win_szx;
	p_va_win->win_szy = local_reg1.bit.reg_va_win_szy;

	p_va_win->win_numx = local_reg2.bit.reg_va_win_numx + 1;
	p_va_win->win_numy = local_reg2.bit.reg_va_win_numy + 1;
	p_va_win->win_spx = local_reg2.bit.reg_va_win_skipx;
	p_va_win->win_spy = local_reg2.bit.reg_va_win_skipy;
}

void ipe_get_va_rslt(UINT32 *p_g1_h, UINT32 *p_g1_v, UINT32 *p_g1_hcnt, UINT32 *p_g1_vcnt, UINT32 *p_g2_h, UINT32 *p_g2_v, UINT32 *p_g2_hcnt, UINT32 *p_g2_vcnt)
{
	IPE_VA_WIN_PARAM p_va_win = {0};
	UINT32 buff_addr, buff_lofs;
#ifdef __KERNEL__
	buff_addr = g_ipe_dramio[IPE_OUT_VA].addr; //virtual addr
#else
	buff_addr = dma_getNonCacheAddr(ipe_get_va_out_addr());
#endif

	if (buff_addr == 0) {
		DBG_DUMP("addr should not be 0 \r\n");
		return;
	}

	buff_lofs = ipe_get_va_out_lofs();
	ipe_get_win_info(&p_va_win);
	ipe_get_va_rslt_manual(p_g1_h, p_g1_v, p_g1_hcnt, p_g1_vcnt, p_g2_h, p_g2_v, p_g2_hcnt, p_g2_vcnt, &p_va_win, buff_addr, buff_lofs);
}

void ipe_get_va_rslt_manual(UINT32 *p_g1_h, UINT32 *p_g1_v, UINT32 *p_g1_hcnt, UINT32 *p_g1_vcnt, UINT32 *p_g2_h, UINT32 *p_g2_v, UINT32 *p_g2_hcnt, UINT32 *p_g2_vcnt, IPE_VA_WIN_PARAM *p_va_win, UINT32 address, UINT32 lineoffset)
{
	UINT32 i, j, id, buf;
	//UINT32 uiWinNum;
	UINT32 va_h, va_v, va_cnt_h, va_cnt_v;

	//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "addr =  0x%08x, lineoffset =  0x%08x\r\n", (unsigned int)address, (unsigned int)lineoffset);
	//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "winNumx=%d,winNumy=%d,\r\n", (unsigned int)p_va_win->win_numx, (unsigned int)p_va_win->win_numy);
	//uiWinNum = p_va_win->win_numx * p_va_win->win_numy;
	if (ipe_get_va_outsel() == VA_OUT_GROUP1) {
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "VA_OUT_GROUP1 \r\n");
		for (j = 0; j < p_va_win->win_numy; j++) {
			for (i = 0; i < p_va_win->win_numx; i++) {
				// 2 word per window
				id = j * p_va_win->win_numx + i;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 3));
				va_h    = ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g1_h + id)     = va_h;
				va_cnt_h = ((buf >> 16) & 0xffff);
				*(p_g1_hcnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 3) + 4);
				va_v    = ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g1_v + id)     = va_v;
				va_cnt_v = ((buf >> 16) & 0xffff);
				*(p_g1_vcnt + id)  = va_cnt_v;
			}
		}
	} else { // ipe_get_va_outsel() == VA_OUT_BOTH
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "VA_OUT_BOTH \r\n");
		for (j = 0; j < p_va_win->win_numy; j++) {
			for (i = 0; i < p_va_win->win_numx; i++) {
				// 4 word per window
				id = j * p_va_win->win_numx + i;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4));
				va_h    = ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g1_h + id)     = va_h;
				va_cnt_h = ((buf >> 16) & 0xffff);
				*(p_g1_hcnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 4);
				va_v    = ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g1_v + id)     = va_v;
				va_cnt_v = ((buf >> 16) & 0xffff);
				*(p_g1_vcnt + id)  = va_cnt_v;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 8);
				va_h    = ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g2_h + id)     = va_h;
				va_cnt_h = ((buf >> 16) & 0xffff);
				*(p_g2_hcnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 12);
				va_v    = ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g2_v + id)     = va_v;
				va_cnt_v = ((buf >> 16) & 0xffff);
				*(p_g2_vcnt + id)  = va_cnt_v;
			}
		}
	}

	ipe_dbg_cmd &= ~(IPE_DBG_VA);
}

void ipe_get_va_result(IPE_VA_SETTING *p_va_info, IPE_VA_RSLT *p_va_rslt)
{
	UINT32 i, j, id, buf;
	//UINT32 uiWinNum;
	UINT32 va_h, va_v, va_cnt_h, va_cnt_v;
	UINT32 address, lineoffset;

	p_va_info->win_num_x = va_ring_setting[1].win_num_x;
	p_va_info->win_num_y = va_ring_setting[1].win_num_y;
	p_va_info->outsel = va_ring_setting[1].outsel;
	p_va_info->address = va_ring_setting[1].address;
	p_va_info->lineoffset = va_ring_setting[1].lineoffset;

	address = p_va_info->address;
	lineoffset = p_va_info->lineoffset;

	if (va_ring_setting[1].va_en == 0) {
		DBG_DUMP("get va fail, va not open \r\n");
		return;
	}

	if (p_va_info->win_num_x == 0 || p_va_info->win_num_y == 0) {
		DBG_DUMP("get va fail, va not ready \r\n");
		return;
	}

	if (address == 0) {
		DBG_DUMP("get va fail, addr should not be 0 \r\n");
		return;
	}

	//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "addr =  0x%08x, lineoffset =  0x%08x\r\n", (unsigned int)address, (unsigned int)lineoffset);
	//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "winNumx=%d,winNumy=%d,\r\n", (unsigned int)p_va_info->win_num_x, (unsigned int)p_va_info->win_num_y);

	if (p_va_info->outsel == VA_OUT_GROUP1) {
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "VA_OUT_GROUP1 \r\n");
		for (j = 0; j < p_va_info->win_num_y; j++) {
			for (i = 0; i < p_va_info->win_num_x; i++) {
				// 2 word per window
				id = j * p_va_info->win_num_x + i;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 3));
				va_h    = ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g1_h + id)     = va_h;
				va_cnt_h = ((buf >> 16) & 0xffff);
				*(p_va_rslt->p_g1_h_cnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 3) + 4);
				va_v    = ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g1_v + id)     = va_v;
				va_cnt_v = ((buf >> 16) & 0xffff);
				*(p_va_rslt->p_g1_v_cnt + id)  = va_cnt_v;
			}
		}
	} else { // ipe_get_va_outsel() == VA_OUT_BOTH
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "VA_OUT_BOTH \r\n");
		for (j = 0; j < p_va_info->win_num_y; j++) {
			for (i = 0; i < p_va_info->win_num_x; i++) {
				// 4 word per window
				id = j * p_va_info->win_num_x + i;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4));
				va_h    = ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g1_h + id)     = va_h;
				va_cnt_h = (((buf >> 16) & 0xffff) << 2);
				*(p_va_rslt->p_g1_h_cnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 4);
				va_v    = ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g1_v + id)     = va_v;
				va_cnt_v = (((buf >> 16) & 0xffff) << 2);
				*(p_va_rslt->p_g1_v_cnt + id)  = va_cnt_v;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 8);
				va_h    = ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g2_h + id)     = va_h;
				va_cnt_h = (((buf >> 16) & 0xffff) << 2);
				*(p_va_rslt->p_g2_h_cnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 12);
				va_v    = ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g2_v + id)     = va_v;
				va_cnt_v = (((buf >> 16) & 0xffff) << 2);
				*(p_va_rslt->p_g2_v_cnt + id)  = va_cnt_v;
			}
		}
	}

	ipe_dbg_cmd &= ~(IPE_DBG_VA);

}

IPE_INDEP_VA_PARAM ipe_get_indep_win_info(UINT32 win_idx)
{
	IPE_INDEP_VA_PARAM p_indep_va_win_info = {0};
	UINT32 addr_ofs;
	UINT32 mask = 1;
	T_IPE_MODE_REGISTER_1 local_reg0;

	T_INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0 local_reg1;
	T_INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1 local_reg2;

	local_reg0.reg = IPE_GETREG(IPE_MODE_REGISTER_1_OFS);

	mask = mask << (20 + win_idx);

	if (win_idx < 5) {
		addr_ofs = win_idx * 8;
	} else {
		return p_indep_va_win_info;
	}

	local_reg1.reg = IPE_GETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0_OFS + addr_ofs)); //no need//
	local_reg2.reg = IPE_GETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1_OFS + addr_ofs)); //no need//

	//p_indep_va_win_info.indep_va_en = local_reg0.reg.reg_win0_va_en;
	p_indep_va_win_info.indep_va_en = local_reg0.reg & (mask & 0xffffffff);
	p_indep_va_win_info.win_stx = local_reg1.bit.reg_win0_stx;
	p_indep_va_win_info.win_sty = local_reg1.bit.reg_win0_sty;

	p_indep_va_win_info.linemax_g1_en = local_reg2.bit.reg_win0_g1_line_max_mode;
	p_indep_va_win_info.linemax_g2_en = local_reg2.bit.reg_win0_g2_line_max_mode;
	p_indep_va_win_info.win_szx = local_reg2.bit.reg_win0_hsz;
	p_indep_va_win_info.win_szy = local_reg2.bit.reg_win0_vsz;

	return p_indep_va_win_info;
}

void ipe_get_indep_va_win_rslt(IPE_INDEP_VA_WIN_RSLT *p_indepva_rslt, UINT32 win_idx)
{
	UINT32 addr_ofs;
	T_INDEP_VARIATION_ACCUMULATION_REGISTER_0 local_reg0;
	T_INDEP_VARIATION_ACCUMULATION_REGISTER_1 local_reg1;
	T_INDEP_VARIATION_ACCUMULATION_REGISTER_2 local_reg2;
	T_INDEP_VARIATION_ACCUMULATION_REGISTER_3 local_reg3;

	if (win_idx < 5) {
		addr_ofs = win_idx * 16;
	} else {
		return;
	}

	local_reg0.reg = IPE_GETREG(INDEP_VARIATION_ACCUMULATION_REGISTER_0_OFS + addr_ofs);
	local_reg1.reg = IPE_GETREG(INDEP_VARIATION_ACCUMULATION_REGISTER_1_OFS + addr_ofs);
	local_reg2.reg = IPE_GETREG(INDEP_VARIATION_ACCUMULATION_REGISTER_2_OFS + addr_ofs);
	local_reg3.reg = IPE_GETREG(INDEP_VARIATION_ACCUMULATION_REGISTER_3_OFS + addr_ofs);

#if 1
	//va_g1_h : ([15:5] base, [4:0] exponent of 2)
	//uiVaG1HCnt : Vacnt (18bit->16bit MSB)
	p_indepva_rslt->va_g1_h    = ((local_reg0.bit.reg_va_win0g1h_vacc >> 5) & (0x7ff)) << ((local_reg0.bit.reg_va_win0g1h_vacc) & (0x1f));
	p_indepva_rslt->va_cnt_g1_h = (local_reg0.bit.reg_va_win0g1h_vacnt << 2);
	p_indepva_rslt->va_g1_v    = ((local_reg1.bit.reg_va_win0g1v_vacc >> 5) & (0x7ff)) << ((local_reg1.bit.reg_va_win0g1v_vacc) & (0x1f));
	p_indepva_rslt->va_cnt_g1_v = (local_reg1.bit.reg_va_win0g1v_vacnt << 2);
	p_indepva_rslt->va_g2_h    = ((local_reg2.bit.reg_va_win0g2h_vacc >> 5) & (0x7ff)) << ((local_reg2.bit.reg_va_win0g2h_vacc) & (0x1f));
	p_indepva_rslt->va_cnt_g2_h = (local_reg2.bit.reg_va_win0g2h_vacnt << 2);
	p_indepva_rslt->va_g2_v    = ((local_reg3.bit.reg_va_win0g2v_vacc >> 5) & (0x7ff)) << ((local_reg3.bit.reg_va_win0g2v_vacc) & (0x1f));
	p_indepva_rslt->va_cnt_g2_v = (local_reg3.bit.reg_va_win0g2v_vacnt << 2);

#else
	p_indepva_rslt->va_g1_h    = local_reg0.bit.reg_va_win0g1h_vacc;
	p_indepva_rslt->va_cnt_g1_h = local_reg0.bit.reg_va_win0g1h_vacnt;

	p_indepva_rslt->va_g1_v    = local_reg1.bit.reg_va_win0g1v_vacc;
	p_indepva_rslt->va_cnt_g1_v = local_reg1.bit.reg_va_win0g1v_vacnt;

	p_indepva_rslt->va_g2_h    = local_reg2.bit.reg_va_win0g2h_vacc;
	p_indepva_rslt->va_cnt_g2_h = local_reg2.bit.reg_va_win0g2h_vacnt;

	p_indepva_rslt->va_g2_v    = local_reg3.bit.reg_va_win0g2v_vacc;
	p_indepva_rslt->va_cnt_g2_v = local_reg3.bit.reg_va_win0g2v_vacnt;
#endif
}

ER ipe_check_param(IPE_MODEINFO *p_mode_info)
{
	ER er_return = E_OK;

	if (((p_mode_info->in_info.ipe_mode == IPE_OPMODE_IPP) || (p_mode_info->in_info.ipe_mode == IPE_OPMODE_SIE_IPP)) && (p_mode_info->out_info.yc_to_ime_en == 0)) {
		DBG_ERR("IPE: ipe mode is not D2D, but yc direct to ime is off !\r\n");
		return E_PAR;
	}

	if (p_mode_info->out_info.va_to_dram_en == 1) {
		er_return = ipe_check_va_lofs((p_mode_info->iq_info.p_va_win_info), &(p_mode_info->out_info));
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = ipe_check_va_win_info((p_mode_info->iq_info.p_va_win_info), p_mode_info->in_info.in_size_y);
		if (er_return != E_OK) {
			return er_return;
		}
	}
	return er_return;
}

ER ipe_check_va_lofs(IPE_VA_WIN_PARAM *p_va_win, IPE_OUTYCINFO *p_out_info)
{
	UINT32 lofs_min;
	ER er_return = E_OK;

	if (p_out_info->va_outsel == 1) {
		lofs_min = (0x4) * (p_va_win->win_numx);
	} else {
		lofs_min = (0x2) * (p_va_win->win_numx);
	}

	if (p_out_info->lofs_va < lofs_min) {
		DBG_ERR("IPE: VA lineoffset setting wrong !\r\n");
		return E_SYS;
	}
	return er_return;

}

ER ipe_check_va_win_info(IPE_VA_WIN_PARAM *p_va_win, IPE_IMG_SIZE size)
{
	UINT32 width_end, height_end;
	ER er_return = E_OK;

	width_end = (p_va_win->win_stx + (p_va_win->win_numx) * (p_va_win->win_szx + p_va_win->win_spx) - p_va_win->win_spx);
	height_end = (p_va_win->win_sty + (p_va_win->win_numy) * (p_va_win->win_szy + p_va_win->win_spy) - p_va_win->win_spy);

	if (width_end > size.h_size) {
		DBG_ERR("IPE: VA setting wrong ! ( x size  > img_width: %d)\r\n", (int)size.h_size);
		DBG_ERR("stx:%d, numx:%d, szx:%d, skipx:%d\r\n", (int)p_va_win->win_stx, (int)p_va_win->win_numx, (int)p_va_win->win_szx, (int)p_va_win->win_spx);
		return E_SYS;
	}
	if (height_end > size.v_size) {
		DBG_ERR("IPE: VA setting wrong ! ( y size > img_height: %d))\r\n", (int)size.v_size);
		DBG_ERR("sty:%d, numy:%d, szy:%d, skipy:%d\r\n", (int)p_va_win->win_sty, (int)p_va_win->win_numy, (int)p_va_win->win_szy, (int)p_va_win->win_spy);
		return E_SYS;
	}

	return er_return;
}


void ipe_set_dma_out_defog_addr(UINT32 sao)
{
	T_DMA_DEFOG_SUBIMG_OUTPUT_CHANNEL_REGISTER local_reg;

	if ((sao == 0)) {
		DBG_ERR("IMG Input addresses cannot be 0x0!\r\n");
		return;
	}

	local_reg.reg = IPE_GETREG(DMA_DEFOG_SUBIMG_OUTPUT_CHANNEL_REGISTER_OFS);
	local_reg.bit.reg_defog_subimg_dramsao = sao >> 2;

	IPE_SETREG(DMA_DEFOG_SUBIMG_OUTPUT_CHANNEL_REGISTER_OFS, local_reg.reg);
}

void ipe_set_dma_out_defog_offset(UINT32 ofso)
{
	T_DMA_DEFOG_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(DMA_DEFOG_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER_OFS);
	local_reg.bit.reg_defog_subimg_lofso = ofso >> 2;

	IPE_SETREG(DMA_DEFOG_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER_OFS, local_reg.reg);
}

void ipe_set_dfg_subimg_size(IPE_IMG_SIZE subimg_size)
{
	T_DEFOG_SUBIMG_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(DEFOG_SUBIMG_REGISTER_OFS);
	local_reg.bit.reg_defog_subimg_width = subimg_size.h_size - 1;
	local_reg.bit.reg_defog_subimg_height = subimg_size.v_size - 1;

	IPE_SETREG(DEFOG_SUBIMG_REGISTER_OFS, local_reg.reg);
}

void ipe_set_dfg_subout_param(UINT32 blk_sizeh, UINT32 blk_cent_h_fact, UINT32 blk_sizev, UINT32 blk_cent_v_fact)
{

	T_DEFOG_SUBOUT_REGISTER_0 local_reg0;
	T_DEFOG_SUBOUT_REGISTER_1 local_reg1;

	local_reg0.reg = IPE_GETREG(DEFOG_SUBOUT_REGISTER_0_OFS);
	local_reg0.bit.reg_defog_sub_blk_sizeh = blk_sizeh;
	local_reg0.bit.reg_defog_blk_cent_hfactor = blk_cent_h_fact;
	local_reg1.reg = IPE_GETREG(DEFOG_SUBOUT_REGISTER_1_OFS);
	local_reg1.bit.reg_defog_sub_blk_sizev = blk_sizev;
	local_reg1.bit.reg_defog_blk_cent_vfactor = blk_cent_v_fact;

	IPE_SETREG(DEFOG_SUBOUT_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(DEFOG_SUBOUT_REGISTER_1_OFS, local_reg1.reg);

}

void ipe_set_dfg_input_bld(UINT8 *input_bld_wt)
{
	T_DEFOG_INPUT_BLENDING_REGISTER_0 local_reg0;
	T_DEFOG_INPUT_BLENDING_REGISTER_1 local_reg1;
	T_DEFOG_INPUT_BLENDING_REGISTER_2 local_reg2;


	local_reg0.reg = IPE_GETREG(DEFOG_INPUT_BLENDING_REGISTER_0_OFS);
	local_reg0.bit.reg_defog_input_bldrto0 = input_bld_wt[0];
	local_reg0.bit.reg_defog_input_bldrto1 = input_bld_wt[1];
	local_reg0.bit.reg_defog_input_bldrto2 = input_bld_wt[2];
	local_reg0.bit.reg_defog_input_bldrto3 = input_bld_wt[3];
	local_reg1.reg = IPE_GETREG(DEFOG_INPUT_BLENDING_REGISTER_1_OFS);
	local_reg1.bit.reg_defog_input_bldrto4 = input_bld_wt[4];
	local_reg1.bit.reg_defog_input_bldrto5 = input_bld_wt[5];
	local_reg1.bit.reg_defog_input_bldrto6 = input_bld_wt[6];
	local_reg1.bit.reg_defog_input_bldrto7 = input_bld_wt[7];
	local_reg2.reg = IPE_GETREG(DEFOG_INPUT_BLENDING_REGISTER_2_OFS);
	local_reg2.bit.reg_defog_input_bldrto8 = input_bld_wt[8];

	IPE_SETREG(DEFOG_INPUT_BLENDING_REGISTER_0_OFS, local_reg0.reg);
	IPE_SETREG(DEFOG_INPUT_BLENDING_REGISTER_1_OFS, local_reg1.reg);
	IPE_SETREG(DEFOG_INPUT_BLENDING_REGISTER_2_OFS, local_reg2.reg);

}

void ipe_set_dfg_scal_factor(UINT32 scalfact_h, UINT32 scalfact_v)
{
	T_DEFOG_SUBIMG_SCALING_REGISTER_0 local_reg;

	local_reg.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER_0_OFS);
	local_reg.bit.reg_defog_subimg_hfactor = scalfact_h;
	local_reg.bit.reg_defog_subimg_vfactor = scalfact_v;

	IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER_0_OFS, local_reg.reg);
}

void ipe_set_dfg_scal_filtcoeff(UINT8 *p_filtcoef)
{
	T_DEFOG_SUBIMG_LPF_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(DEFOG_SUBIMG_LPF_REGISTER_OFS);
	local_reg.bit.reg_defog_lpf_c0 = p_filtcoef[0];
	local_reg.bit.reg_defog_lpf_c1 = p_filtcoef[1];
	local_reg.bit.reg_defog_lpf_c2 = p_filtcoef[2];

	IPE_SETREG(DEFOG_SUBIMG_LPF_REGISTER_OFS, local_reg.reg);
}

void ipe_set_dfg_scalg_edgeinterplut(UINT8 *p_interp_diff_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_DEFOG_SUBIMG_SCALING_REGISTER1 local_reg0;
	//T_DEFOG_SUBIMG_SCALING_REGISTER2 local_reg1;
	//T_DEFOG_SUBIMG_SCALING_REGISTER3 local_reg2;
	T_DEFOG_SUBIMG_SCALING_REGISTER4 local_reg3;

#if 0
	local_reg0.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER1_OFS);
	local_reg0.bit.reg_defog_interp_diff_lut0 = p_interp_diff_lut[0];
	local_reg0.bit.reg_defog_interp_diff_lut1 = p_interp_diff_lut[1];
	local_reg0.bit.reg_defog_interp_diff_lut2 = p_interp_diff_lut[2];
	local_reg0.bit.reg_defog_interp_diff_lut3 = p_interp_diff_lut[3];
	local_reg0.bit.reg_defog_interp_diff_lut4 = p_interp_diff_lut[4];
	local_reg1.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER2_OFS);
	local_reg1.bit.reg_defog_interp_diff_lut5 = p_interp_diff_lut[5];
	local_reg1.bit.reg_defog_interp_diff_lut6 = p_interp_diff_lut[6];
	local_reg1.bit.reg_defog_interp_diff_lut7 = p_interp_diff_lut[7];
	local_reg1.bit.reg_defog_interp_diff_lut8 = p_interp_diff_lut[8];
	local_reg1.bit.reg_defog_interp_diff_lut9 = p_interp_diff_lut[9];
	local_reg2.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER3_OFS);
	local_reg2.bit.reg_defog_interp_diff_lut10 = p_interp_diff_lut[10];
	local_reg2.bit.reg_defog_interp_diff_lut11 = p_interp_diff_lut[11];
	local_reg2.bit.reg_defog_interp_diff_lut12 = p_interp_diff_lut[12];
	local_reg2.bit.reg_defog_interp_diff_lut13 = p_interp_diff_lut[13];
	local_reg2.bit.reg_defog_interp_diff_lut14 = p_interp_diff_lut[14];

	IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER1_OFS, local_reg0.reg);
	IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER2_OFS, local_reg1.reg);
	IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER3_OFS, local_reg2.reg);
#endif

	for (i = 0; i < 3; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_SUBIMG_SCALING_REGISTER1_OFS + (i << 2);

		idx = (i * 5);
		reg_set_val = (p_interp_diff_lut[idx] & 0x3F)   + ((p_interp_diff_lut[idx + 1] & 0x3F) << 6) + ((p_interp_diff_lut[idx + 2] & 0x3F) << 12) +
					  ((p_interp_diff_lut[idx + 3] & 0x3F) << 18) + ((p_interp_diff_lut[idx + 4] & 0x3F) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	local_reg3.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER4_OFS);
	local_reg3.bit.reg_defog_interp_diff_lut15 = p_interp_diff_lut[15];
	local_reg3.bit.reg_defog_interp_diff_lut16 = p_interp_diff_lut[16];
	IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER4_OFS, local_reg3.reg);
}

void ipe_set_defog_scal_edgeinterp(UINT8 wdist, UINT8 wout, UINT8 wcenter, UINT8 wsrc)
{

	T_DEFOG_SUBIMG_SCALING_REGISTER5 local_reg;

	local_reg.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER5_OFS);
	local_reg.bit.reg_defog_interp_wdist = wdist;
	local_reg.bit.reg_defog_interp_wout = wout;
	local_reg.bit.reg_defog_interp_wcenter = wcenter;
	local_reg.bit.reg_defog_interp_wsrc = wsrc;

	IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER5_OFS, local_reg.reg);
}

void ipe_set_defog_atmospherelight(UINT16 val0, UINT16 val1, UINT16 val2)
{
	T_DEFOG_AIRLIGHT_REGISTER0 local_reg0;
	T_DEFOG_AIRLIGHT_REGISTER1 local_reg1;

	local_reg0.reg = IPE_GETREG(DEFOG_AIRLIGHT_REGISTER0_OFS);
	local_reg0.bit.reg_defog_air0 = val0;
	local_reg0.bit.reg_defog_air1 = val1;

	local_reg1.reg = IPE_GETREG(DEFOG_AIRLIGHT_REGISTER1_OFS);
	local_reg1.bit.reg_defog_air2 = val2;

	IPE_SETREG(DEFOG_AIRLIGHT_REGISTER0_OFS, local_reg0.reg);
	IPE_SETREG(DEFOG_AIRLIGHT_REGISTER1_OFS, local_reg1.reg);
}

void ipe_set_defog_fog_protect(BOOL self_cmp_en, UINT16 min_diff)
{
	T_DEFOG_STRENGTH_CONTROL_REGISTER5 local_reg;

	local_reg.reg = IPE_GETREG(DEFOG_STRENGTH_CONTROL_REGISTER5_OFS);
	local_reg.bit.reg_defog_min_diff = min_diff;
	local_reg.bit.reg_defog_selfcmp_en = self_cmp_en;

	IPE_SETREG(DEFOG_STRENGTH_CONTROL_REGISTER5_OFS, local_reg.reg);
}

void ipe_set_defog_fog_mod(UINT16 *p_fog_mod_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//UINT32 addr_ofs, i;
	//T_DEFOG_FOG_MODIFY_REGISTER0 local_reg0;
	T_DEFOG_FOG_MODIFY_REGISTER8 local_reg1;

	for (i = 0; i < 8; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_FOG_MODIFY_REGISTER0_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_fog_mod_lut[idx] & 0x3FF) + ((p_fog_mod_lut[idx + 1] & 0x3FF) << 16);

		//addr_ofs = i * 4;
		//local_reg0.reg = IPE_GETREG((DEFOG_FOG_MODIFY_REGISTER0_OFS + addr_ofs));

		//local_reg0.bit.reg_defog_mod_lut_0 = p_fog_mod_lut[2 * i];
		//local_reg0.bit.reg_defog_mod_lut_1 = p_fog_mod_lut[2 * i + 1];

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	local_reg1.reg = IPE_GETREG(DEFOG_FOG_MODIFY_REGISTER8_OFS);
	local_reg1.bit.reg_defog_mod_lut_16 = p_fog_mod_lut[16];
	IPE_SETREG(DEFOG_FOG_MODIFY_REGISTER8_OFS, local_reg1.reg);
}


void ipe_set_defog_fog_target_lut(UINT16 *p_fog_target_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//UINT32 addr_ofs, i;
	//T_DEFOG_STRENGTH_CONTROL_REGISTER0 local_reg0;
	T_DEFOG_STRENGTH_CONTROL_REGISTER4 local_reg1;

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_STRENGTH_CONTROL_REGISTER0_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_fog_target_lut[idx] & 0x3FF) + ((p_fog_target_lut[idx + 1] & 0x3FF) << 16);

		//addr_ofs = i * 4;
		//local_reg0.reg = IPE_GETREG((DEFOG_STRENGTH_CONTROL_REGISTER0_OFS + addr_ofs));

		//local_reg0.bit.reg_defog_target_lut_0 = p_fog_target_lut[2 * i];
		//local_reg0.bit.reg_defog_target_lut_1 = p_fog_target_lut[2 * i + 1];

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	local_reg1.reg = IPE_GETREG(DEFOG_STRENGTH_CONTROL_REGISTER4_OFS);
	local_reg1.bit.reg_defog_target_lut_8 = p_fog_target_lut[8];
	IPE_SETREG(DEFOG_STRENGTH_CONTROL_REGISTER4_OFS, local_reg1.reg);
}

void ipe_set_defog_strength(IPE_DEFOG_METHOD_SEL modesel, UINT8 fog_ratio, UINT8 dgain, UINT8 gain_th)
{

	T_DEFOG_STRENGTH_CONTROL_REGISTER5 local_reg0;
	T_DEFOG_STRENGTH_CONTROL_REGISTER6 local_reg1;

	local_reg0.reg = IPE_GETREG(DEFOG_STRENGTH_CONTROL_REGISTER5_OFS);
	local_reg0.bit.reg_defog_mode_sel = modesel;
	local_reg0.bit.reg_defog_fog_rto = fog_ratio;
	local_reg0.bit.reg_defog_dgain_ratio = dgain;

	local_reg1.reg = IPE_GETREG(DEFOG_STRENGTH_CONTROL_REGISTER6_OFS);
	local_reg1.bit.reg_defog_gain_th = gain_th;

	IPE_SETREG(DEFOG_STRENGTH_CONTROL_REGISTER5_OFS, local_reg0.reg);
	IPE_SETREG(DEFOG_STRENGTH_CONTROL_REGISTER6_OFS, local_reg1.reg);
}

void ipe_set_defog_outbld_lum_lut(BOOL lum_bld_ref, UINT8 *p_outbld_lum_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//UINT32 addr_ofs, i;
	//T_DEFOG_OUTPUT_BLENDING_REGISTER0 local_reg0;
	T_DEFOG_OUTPUT_BLENDING_REGISTER4 local_reg1;

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_OUTPUT_BLENDING_REGISTER0_OFS + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_outbld_lum_lut[idx] & 0xFF) + ((p_outbld_lum_lut[idx + 1] & 0xFF) << 8) + ((p_outbld_lum_lut[idx + 2] & 0xFF) << 16) + ((p_outbld_lum_lut[idx + 3] & 0xFF) << 24);


		//addr_ofs = i * 4;
		//local_reg0.reg = IPE_GETREG((DEFOG_OUTPUT_BLENDING_REGISTER0_OFS + addr_ofs));

		//local_reg0.bit.reg_defog_outbld_lumwt0 = p_outbld_lum_lut[4 * i];
		//local_reg0.bit.reg_defog_outbld_lumwt1 = p_outbld_lum_lut[4 * i + 1];
		//local_reg0.bit.reg_defog_outbld_lumwt2 = p_outbld_lum_lut[4 * i + 2];
		//local_reg0.bit.reg_defog_outbld_lumwt3 = p_outbld_lum_lut[4 * i + 3];

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	local_reg1.reg = IPE_GETREG(DEFOG_OUTPUT_BLENDING_REGISTER4_OFS);
	local_reg1.bit.reg_defog_outbld_lumwt16 = p_outbld_lum_lut[16];
	local_reg1.bit.reg_defog_wet_ref = lum_bld_ref;
	IPE_SETREG(DEFOG_OUTPUT_BLENDING_REGISTER4_OFS, local_reg1.reg);
}

void ipe_set_defog_outbld_diff_lut(UINT8 *p_outbld_diff_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//UINT32 addr_ofs, i;
	//T_DEFOG_OUTPUT_BLENDING_REGISTER5 local_reg0;
	T_DEFOG_OUTPUT_BLENDING_REGISTER8 local_reg1;

	for (i = 0; i < 3; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_OUTPUT_BLENDING_REGISTER5_OFS + (i << 2);

		idx = (i * 5);
		reg_set_val = (p_outbld_diff_lut[idx] & 0x3F) + ((p_outbld_diff_lut[idx + 1] & 0x3F) << 6) + ((p_outbld_diff_lut[idx + 2] & 0x3F) << 12) + ((p_outbld_diff_lut[idx + 3] & 0x3F) << 18) + ((p_outbld_diff_lut[idx + 4] & 0x3F) << 24);

		//addr_ofs = i * 4;
		//local_reg0.reg = IPE_GETREG((DEFOG_OUTPUT_BLENDING_REGISTER5_OFS + addr_ofs));

		//local_reg0.bit.reg_defog_outbld_diffwt0 = p_outbld_diff_lut[5 * i];
		//local_reg0.bit.reg_defog_outbld_diffwt1 = p_outbld_diff_lut[5 * i + 1];
		//local_reg0.bit.reg_defog_outbld_diffwt2 = p_outbld_diff_lut[5 * i + 2];
		//local_reg0.bit.reg_defog_outbld_diffwt3 = p_outbld_diff_lut[5 * i + 3];
		//local_reg0.bit.reg_defog_outbld_diffwt4 = p_outbld_diff_lut[5 * i + 4];

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	local_reg1.reg = IPE_GETREG(DEFOG_OUTPUT_BLENDING_REGISTER8_OFS);
	local_reg1.bit.reg_defog_outbld_diffwt15 = p_outbld_diff_lut[15];
	local_reg1.bit.reg_defog_outbld_diffwt16 = p_outbld_diff_lut[16];
	IPE_SETREG(DEFOG_OUTPUT_BLENDING_REGISTER8_OFS, local_reg1.reg);

}

void ipe_set_defog_outbld_local(UINT32 local_en, UINT32 air_nfactor)
{
	T_DEFOG_AIRLIGHT_REGISTER2 local_reg0;
	T_DEFOG_OUTPUT_BLENDING_REGISTER4 local_reg1;

	local_reg0.reg = IPE_GETREG(DEFOG_AIRLIGHT_REGISTER2_OFS);
	local_reg0.bit.reg_defog_air_normfactor = air_nfactor;

	local_reg1.reg = IPE_GETREG(DEFOG_OUTPUT_BLENDING_REGISTER4_OFS);
	local_reg1.bit.reg_defog_local_outbld_en = local_en;

	IPE_SETREG(DEFOG_AIRLIGHT_REGISTER2_OFS, local_reg0.reg);
	IPE_SETREG(DEFOG_OUTPUT_BLENDING_REGISTER4_OFS, local_reg1.reg);
}

void ipe_set_defog_stcs_pcnt(UINT32 pixelcnt)
{
	T_DEFOG_STATISTICS_REGISTER0 local_reg1;

	local_reg1.reg = IPE_GETREG(DEFOG_STATISTICS_REGISTER0_OFS);
	local_reg1.bit.reg_defog_statistics_pixcnt = pixelcnt;

	IPE_SETREG(DEFOG_STATISTICS_REGISTER0_OFS, local_reg1.reg);
}

void ipe_get_defog_stcs(DEFOG_STCS_RSLT *stcs)
{
	T_DEFOG_STATISTICS_REGISTER1 local_reg0;
	T_DEFOG_STATISTICS_REGISTER2 local_reg1;

	local_reg0.reg = IPE_GETREG(DEFOG_STATISTICS_REGISTER1_OFS);
	stcs->airlight[0] = local_reg0.bit.reg_defog_statistics_air0;
	stcs->airlight[1] = local_reg0.bit.reg_defog_statistics_air1;

	local_reg1.reg = IPE_GETREG(DEFOG_STATISTICS_REGISTER2_OFS);
	stcs->airlight[2] = local_reg1.bit.reg_defog_statistics_air2;
}

void ipe_get_defog_airlight_setting(UINT16 *p_val)
{
	T_DEFOG_AIRLIGHT_REGISTER0 local_reg0;
	T_DEFOG_AIRLIGHT_REGISTER1 local_reg1;

	local_reg0.reg = IPE_GETREG(DEFOG_AIRLIGHT_REGISTER0_OFS);
	p_val[0] = local_reg0.bit.reg_defog_air0;
	p_val[1] = local_reg0.bit.reg_defog_air1;

	local_reg1.reg = IPE_GETREG(DEFOG_AIRLIGHT_REGISTER1_OFS);
	p_val[2] = local_reg1.bit.reg_defog_air2;
}


void ipe_set_lce_diff_param(UINT8 wt_avg, UINT8 wt_pos, UINT8 wt_neg)
{
	T_LCE_REGISTER_0 local_reg1;

	local_reg1.reg = IPE_GETREG(LCE_REGISTER_0_OFS);
	local_reg1.bit.reg_lce_wt_diff_pos = wt_pos;
	local_reg1.bit.reg_lce_wt_diff_neg = wt_neg;
	local_reg1.bit.reg_lce_wt_diff_avg = wt_avg;

	IPE_SETREG(LCE_REGISTER_0_OFS, local_reg1.reg);
}

void ipe_set_lce_lum_lut(UINT8 *p_lce_lum_lut)
{
	T_LCE_REGISTER_1 local_reg0;
	T_LCE_REGISTER_2 local_reg1;
	T_LCE_REGISTER_3 local_reg2;

	local_reg0.reg = IPE_GETREG(LCE_REGISTER_1_OFS);
	local_reg0.bit.reg_lce_lum_adj_lut0 = p_lce_lum_lut[0];
	local_reg0.bit.reg_lce_lum_adj_lut1 = p_lce_lum_lut[1];
	local_reg0.bit.reg_lce_lum_adj_lut2 = p_lce_lum_lut[2];
	local_reg0.bit.reg_lce_lum_adj_lut3 = p_lce_lum_lut[3];
	local_reg1.reg = IPE_GETREG(LCE_REGISTER_2_OFS);
	local_reg1.bit.reg_lce_lum_adj_lut4 = p_lce_lum_lut[4];
	local_reg1.bit.reg_lce_lum_adj_lut5 = p_lce_lum_lut[5];
	local_reg1.bit.reg_lce_lum_adj_lut6 = p_lce_lum_lut[6];
	local_reg1.bit.reg_lce_lum_adj_lut7 = p_lce_lum_lut[7];
	local_reg2.reg = IPE_GETREG(LCE_REGISTER_3_OFS);
	local_reg2.bit.reg_lce_lum_adj_lut8 = p_lce_lum_lut[8];

	IPE_SETREG(LCE_REGISTER_1_OFS, local_reg0.reg);
	IPE_SETREG(LCE_REGISTER_2_OFS, local_reg1.reg);
	IPE_SETREG(LCE_REGISTER_3_OFS, local_reg2.reg);
}
#else
/**
    IPE reset on

    Set IPE reset bit to 1.

    @param None.

    @return None.
*/
void ipe_reset_on(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_swrst = ENABLE;

	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
	IPE reset off

	Set IPE reset bit to 0.

	@param None.

	@return None.
*/
void ipe_reset_off(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_swrst = DISABLE;

	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
	IPE set linked list fire

	Set IPE linked list fire bit to 1.

	@param None.

	@return None.
*/
void ipe_set_ll_fire(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ll_fire = ENABLE;

	local_reg.bit.reg_ipe_start = DISABLE;
	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

void ipe_set_ll_terminate(void)
{
	T_LINKED_LIST_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(LINKED_LIST_REGISTER_OFS);
	local_reg.bit.reg_ll_terminate = ENABLE;

	IPE_SETREG(LINKED_LIST_REGISTER_OFS, local_reg.reg);
}


/**
	IPE set start

	Set IPE start bit to 1.

	@param None.

	@return None.
*/
void ipe_set_start(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_start = ENABLE;

	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

//for FPGA verification only
void ipe_set_start_frmend_load(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_start = ENABLE;

	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_fd = ENABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;


	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}


/**
	IPE set stop

	Set IPE reset bit to 0.

	@param None.

	@return None.
*/
void ipe_set_stop(void)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_start = DISABLE;

	local_reg.bit.reg_ipe_rwgamma = DISABLE;
	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
	IPE Set load

	Set IPE one of the load bits to 1.

	@param type Load type.

	@return None.
*/
void ipe_set_load(IPE_LOAD_TYPE type)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	switch (type) {
	case IPE_START_LOAD:
		local_reg.bit.reg_ipe_load_start = ENABLE;
		//HW clear load, force write zero to prevent multiple loads
		local_reg.bit.reg_ipe_load_fd = DISABLE;
		local_reg.bit.reg_ipe_load_fs = DISABLE;
		break;
	case IPE_FRMEND_LOAD:
		if (g_ipe_status == IPE_STATUS_RUN) {
			local_reg.bit.reg_ipe_load_fd = ENABLE;
			//HW clear load, force write zero to prevent multiple loads
			local_reg.bit.reg_ipe_load_start = DISABLE;
			local_reg.bit.reg_ipe_load_fs = DISABLE;
		} else {
			local_reg.bit.reg_ipe_load_fd = DISABLE;
			local_reg.bit.reg_ipe_load_start = DISABLE;
			local_reg.bit.reg_ipe_load_fs = DISABLE;
		}
		break;
	case IPE_DIRECT_START_LOAD:
		local_reg.bit.reg_ipe_load_fs = ENABLE;
		//HW clear load, force write zero to prevent multiple loads
		local_reg.bit.reg_ipe_load_fd = DISABLE;
		local_reg.bit.reg_ipe_load_start = DISABLE;
		break;
	default:
		DBG_ERR("ipe_set_load() load type error!\r\n");
		//HW clear load, force write zero to prevent multiple loads
		local_reg.bit.reg_ipe_load_fd = DISABLE;
		local_reg.bit.reg_ipe_load_start = DISABLE;
		local_reg.bit.reg_ipe_load_fs = DISABLE;
		break;

	}
	local_reg.bit.reg_ipe_rwgamma = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
	IPE Gamma read write control

	Enable/disable IPE Gamma SRAM read/write by CPU

	@param rw_sel

	@return None.
*/
void ipe_set_gamma_rw_en(IPE_RW_SRAM_ENUM rw_sel, IPE_RWGAM_OPT opt)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_rwgamma = rw_sel;
	if (rw_sel == DMA_WRITE_LUT) {
		local_reg.bit.reg_ipe_rwgamma_opt = opt;
	}

	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

//for FPGA verify only
void ipe_set_rw_en(IPE_RW_SRAM_ENUM rw_sel)
{
	T_IPE_CONTROL_REGISTER local_reg;

	local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);
	local_reg.bit.reg_ipe_rwgamma = rw_sel;

	local_reg.bit.reg_ipe_load_fd = DISABLE;
	local_reg.bit.reg_ipe_load_fs = DISABLE;
	local_reg.bit.reg_ipe_load_start = DISABLE;

	IPE_SETREG(IPE_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
	IPE get version

	Get IPE HW version number.

	@param None.

	@return ipe version number.
*/
UINT32 ipe_get_version(void)
{
	//T_IPE_CONTROL_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(IPE_CONTROL_REGISTER_OFS);

	return 0;//local_reg.bit.VERSION;
}

/**
	IPE set operation mode

	Set IPE operation mode.

	@param mode 0: D2D, 1: DCE mode, 2: all direct

	@return None.
*/
void ipe_set_op_mode(IPE_OPMODE mode)
{
	//T_IPE_MODE_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	switch (mode) {
	case IPE_OPMODE_D2D:
		ipeg->reg_1.bit.ipe_mode = 0;
		break;
	case IPE_OPMODE_IPP:
		ipeg->reg_1.bit.ipe_mode = 1;
		break;
	case IPE_OPMODE_SIE_IPP:
		ipeg->reg_1.bit.ipe_mode = 2;
		break;
	default:
		DBG_ERR("ipe_set_op_mode() illegal mode %d!\r\n", mode);
		ipeg->reg_1.bit.ipe_mode = 0;
		break;
	}
	//IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE dram single output mode

	Dram single output mode selection.

	@return None.
*/
void ipe_set_dram_out_mode(IPE_DRAM_OUTPUT_MODE mode)
{
	//T_DMA_TO_IPE_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_0_OFS);
	ipeg->reg_3.bit.ipe_dram_out_mode = mode;

	//IPE_SETREG(DMA_TO_IPE_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE dram single output channel_enable

	Dram single output channel_enable selection.

	@return None.
*/
void ipe_set_dram_single_channel_en(IPE_DRAM_SINGLE_OUT_EN ch)
{
	//T_DMA_TO_IPE_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_0_OFS);
	ipeg->reg_3.bit.ipe_dram_out0_single_en = ch.single_out_y_en;
	ipeg->reg_3.bit.ipe_dram_out1_single_en = ch.single_out_c_en;
	ipeg->reg_3.bit.ipe_dram_out2_single_en = ch.single_out_va_en;
	ipeg->reg_3.bit.ipe_dram_out3_single_en = ch.single_out_defog_en;

	//IPE_SETREG(DMA_TO_IPE_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE output switch

	Output channel selection.

	@param to_ime_en 0: disable IPE to IME output,1: enable IPE to IME output.
	@param to_dram_en 0: disable IPE to DRAM output,1: enable IPE to DRAM output.

	@return None.
*/
void ipe_set_output_switch(BOOL to_ime_en, BOOL to_dram_en)
{
	//T_IPE_MODE_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	ipeg->reg_1.bit.ipe_imeon = to_ime_en;
	ipeg->reg_1.bit.ipe_dmaon = to_dram_en;

	//IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE set dram output data type

	Select IPE dram output data type.

	@param sel 0:original, 1:subout, 2:direction.

	@return None.
*/
void ipe_set_dma_outsel(IPE_DMAOSEL sel)
{
	//T_IPE_MODE_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	ipeg->reg_1.bit.dmao_sel = sel;

	//IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE set H overlap to fixed 8 pixels

	Select IPE H overlap format.

	@param opt 0:auto calculation, 1: fixed for 8 pixels (MST+SQUARE)

	@return None.
*/
void ipe_set_hovlp_opt(IPE_HOVLP_SEL_ENUM opt)
{
	//T_IPE_MODE_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	ipeg->reg_1.bit.ipe_mst_hovlp = opt;

	//IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE set input format

	Select IPE input YCbCr format.

	@param imat 0:444, 1:422, 2:420, 3: Y only.

	@return None.
*/
void ipe_set_in_format(IPE_INOUT_FORMAT imat)
{
	//T_IPE_MODE_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	ipeg->reg_1.bit.ipe_imat = imat;

	//IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE set output format

	Select IPE output YCbCr format.

	@param omat 0:444, 1:422, 2:420, 3: Y only.
	@param subhsel 422 and 420 subsample selection,0: drop right pixel,1: drop left pixel.

	@return None.
*/
void ipe_set_out_format(IPE_INOUT_FORMAT omat, IPE_HSUB_SEL_ENUM sub_h_sel)
{
	//T_IPE_MODE_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	ipeg->reg_1.bit.ipe_omat = omat;
	ipeg->reg_1.bit.ipe_subhsel = sub_h_sel;

	//IPE_SETREG(IPE_MODE_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE enable features

	Enable IPE function blocks.

	@param features 1's in this input word will turn on corresponding blocks.

	@return None.
*/
void ipe_enable_feature(UINT32 features)
{
	T_IPE_MODE_REGISTER_1 local_reg;

	local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_1_OFS);
	local_reg.reg |= features;

	IPE_SETREG(IPE_MODE_REGISTER_1_OFS, local_reg.reg);
}

/**
	IPe disable features

	Disable IPE function blocks.

	@param features 1's in this input word will turn off corresponding blocks.

	@return None.
*/
void ipe_disable_feature(UINT32 features)
{
	T_IPE_MODE_REGISTER_1 local_reg;

	local_reg.reg = IPE_GETREG(IPE_MODE_REGISTER_1_OFS);
	local_reg.reg &= ~features;

	IPE_SETREG(IPE_MODE_REGISTER_1_OFS, local_reg.reg);
}

/**
	IPE set function

	Set IPE function blocks.

	@param features 1's in this input word will turn on corresponding blocks, while 0's will turn off.

	@return None.
*/
void ipe_set_feature(UINT32 features)
{
	T_IPE_MODE_REGISTER_1 local_reg;

	local_reg.reg = features;

	IPE_SETREG(IPE_MODE_REGISTER_1_OFS, local_reg.reg);
}

/**
	Check IPE functions

	IPE check function enable/disable now

	@param ipe_function which function need to check

	@return disable/enable
*/
BOOL ipe_check_function_enable(UINT32 ipe_function)
{
	if (IPE_GETREG(IPE_MODE_REGISTER_1_OFS) & ipe_function) {
		return ENABLE;
	} else {
		return DISABLE;
	}
}

/**
	IPE set h stripe

	Set IPE horizontal stripe.

	@param hstripe_infor Data stucture that contains related horizontal striping information: n, l, m, olap.

	@return None.
*/
void ipe_set_h_stripe(STR_STRIPE_INFOR hstripe_infor)
{
	//T_IPE_STRIPE_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_STRIPE_REGISTER_0_OFS);
	ipeg->reg_5.bit.hn = hstripe_infor.hn;
	ipeg->reg_5.bit.hl = hstripe_infor.hl;
	ipeg->reg_5.bit.hm = hstripe_infor.hm;
	//local_reg.bit.HOLAP = hstripe_infor.Olap;

	//IPE_SETREG(IPE_STRIPE_REGISTER_0_OFS, local_reg.reg);
}

/**
	Get IPE h stripe

	IPE horizontal striping information readout.

	@param None.

	@return Data stucture that includes horizontal striping l, m, n.
*/
STR_STRIPE_INFOR ipe_get_h_stripe(void)
{
	//T_IPE_STRIPE_REGISTER_0 local_reg;
	STR_STRIPE_INFOR h_stripe;

	//local_reg.reg = IPE_GETREG(IPE_STRIPE_REGISTER_0_OFS);
	h_stripe.hn = ipeg->reg_5.bit.hn;
	h_stripe.hl = ipeg->reg_5.bit.hl;
	h_stripe.hm = ipeg->reg_5.bit.hm;
	//h_stripe.Olap = (IPE_OLAP_ENUM)local_reg.bit.HOLAP;

	return h_stripe;
}

/**
	IPE set v stripe

	Set IPE vertical stripe.

	@param vstripe_infor Data stucture that contains related vertical striping information: n, l, m, olap.

	@return None.
*/
void ipe_set_v_stripe(STR_STRIPE_INFOR hstripe_infor)
{
	//T_IPE_STRIPE_REGISTER_1 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_STRIPE_REGISTER_1_OFS);
	ipeg->reg_6.bit.vl = hstripe_infor.hl;

	//IPE_SETREG(IPE_STRIPE_REGISTER_1_OFS, local_reg.reg);
}

/**
	Get IPE vstripe

	IPE vertical striping information readout.

	@param None.

	@return Data stucture that includes vertical striping l, m, n.
*/
STR_STRIPE_INFOR ipe_get_v_stripe(void)
{
	T_IPE_STRIPE_REGISTER_1 local_reg;
	STR_STRIPE_INFOR v_stripe;

	local_reg.reg = IPE_GETREG(IPE_STRIPE_REGISTER_1_OFS);
	v_stripe.hn = 0;
	v_stripe.hl = local_reg.bit.reg_vl;
	v_stripe.hm = 0;
	//v_stripe.Olap = IPE_OLAP_OFF;

	return v_stripe;
}


void ipe_set_linked_list_addr(UINT32 sai)
{
	//T_DMA_TO_IPE_REGISTER_1 local_reg0;

	if ((sai == 0)) {
		DBG_ERR("IMG Input addresses cannot be 0x0!\r\n");
		return;
	}

	//local_reg0.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_1_OFS);
	ipeg->reg_7.bit.dram_sai_ll = sai >> 2;


	//IPE_SETREG(DMA_TO_IPE_REGISTER_1_OFS, local_reg0.reg);
}

/**
	IPE set DMA input address

	Configure DMA input ping-pong addresses.

	@param addr_in_y DMA input channel (DMA to IPE) starting addressing 0 for Y channel.
	@param addr_in_c DMA input channel (DMA to IPE) starting addressing 1 for C channel.

	@return None.
*/
void ipe_set_dma_in_addr(UINT32 addr_in_y, UINT32 addr_in_c)
{
	//T_DMA_TO_IPE_REGISTER_2 local_reg0;
	//T_DMA_TO_IPE_REGISTER_3 local_reg1;

	if ((addr_in_y == 0) || (addr_in_c == 0)) {
		DBG_ERR("IMG Input addresses cannot be 0x0!\r\n");
		return;
	}

	//local_reg0.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_2_OFS);
	ipeg->reg_8.bit.dram_sai_y = addr_in_y >> 2;

	//local_reg1.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_3_OFS);
	ipeg->reg_9.bit.dram_sai_c = addr_in_c >> 2;

	//IPE_SETREG(DMA_TO_IPE_REGISTER_2_OFS, local_reg0.reg);
	//IPE_SETREG(DMA_TO_IPE_REGISTER_3_OFS, local_reg1.reg);
}

void ipe_set_dma_in_defog_addr(UINT32 sai)
{
	//T_DMA_DEFOG_SUBIMG_INPUT_CHANNEL_REGISTER local_reg0;

	if ((sai == 0)) {
		DBG_ERR("IMG defog Input addresses cannot be 0x0!\r\n");
		return;
	}

	//local_reg0.reg = IPE_GETREG(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_REGISTER_OFS);
	ipeg->reg_177.bit.defog_subimg_dramsai = sai >> 2;


	//IPE_SETREG(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_REGISTER_OFS, local_reg0.reg);
}

/**
	IPE set DMA address of Gamma LUT

	Configure DMA address of Gamma LUT

	@param sai DMA input channel (DMA to IPE) starting address for RGB Gamma LUT.

	@return None.
*/
void ipe_set_dma_in_gamma_lut_addr(UINT32 sai)
{
	//T_DMA_TO_IPE_REGISTER_5 local_reg3;

	if (sai == 0) {
		DBG_ERR("LUT Input addresses cannot be 0x0!\r\n");
		return;
	}

	//local_reg3.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_5_OFS);
	ipeg->reg_11.bit.dram_sai_gamma = sai >> 2;

	//IPE_SETREG(DMA_TO_IPE_REGISTER_5_OFS, local_reg3.reg);
}

/**
	IPE set DMA address of Y curve LUT

	Configure DMA address of Y cuvrve LUT

	@param sai DMA input channel (DMA to IPE) starting address for Y curve LUT.

	@return None.
*/
void ipe_set_dma_in_ycurve_lut_addr(UINT32 sai)
{
	//T_DMA_TO_IPE_REGISTER_4 local_reg2;

	if (sai == 0) {
		DBG_ERR("LUT Input addresses cannot be 0x0!\r\n");
		return;
	}

	//local_reg2.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_4_OFS);
	ipeg->reg_10.bit.dram_sai_ycurve = sai >> 2;

	//IPE_SETREG(DMA_TO_IPE_REGISTER_4_OFS, local_reg2.reg);
}

/**
	IPE set DMA input line offset

	Configure DMA input line offset.

	@param ofsi0 input line offset(word) 0 for Y channel.
	@param ofsi1 input line offset(word) 1 for C channel.

	@return None.
*/
void ipe_set_dma_in_offset(UINT32 ofsi0, UINT32 ofsi1)
{
	//T_DMA_TO_IPE_REGISTER_6 local_reg;
	//T_DMA_TO_IPE_REGISTER_7 local_reg2;

	//local_reg.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_6_OFS);
	ipeg->reg_12.bit.dram_ofsi_y = ofsi0 >> 2;

	//local_reg2.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_7_OFS);
	ipeg->reg_13.bit.dram_ofsi_c = ofsi1 >> 2;

	//IPE_SETREG(DMA_TO_IPE_REGISTER_6_OFS, local_reg.reg);
	//IPE_SETREG(DMA_TO_IPE_REGISTER_7_OFS, local_reg2.reg);
}

void ipe_set_dma_in_defog_offset(UINT32 ofsi)
{
	//T_DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER_OFS);
	ipeg->reg_178.bit.defog_subimg_lofsi = ofsi >> 2;

	//IPE_SETREG(DMA_DEFOG_SUBIMG_INPUT_CHANNEL_LINEOFFSET_REGISTER_OFS, local_reg.reg);
}

/**
	IPE set DMA input random LSB

	Configure input random LSB.

	@param ofsi0 input line offset(word) 0 for Y channel.
	@param ofsi1 input line offset(word) 1 for C channel.

	@return None.
*/
void ipe_set_dma_in_rand(IPE_ENABLE en, IPE_ENABLE reset)
{
	//T_DMA_TO_IPE_REGISTER_6 local_reg;

	//local_reg.reg = IPE_GETREG(DMA_TO_IPE_REGISTER_6_OFS);
	ipeg->reg_12.bit.inrand_en = en;
	ipeg->reg_12.bit.inrand_rst = reset;

	//IPE_SETREG(DMA_TO_IPE_REGISTER_6_OFS, local_reg.reg);
}

/**
	IPE set DMA output address

	Configure DMA output address.

	@param sao_y DMA output starting address of Y channel.

	@return None.
*/
void ipe_set_dma_out_addr_y(UINT32 sao_y)
{
	//T_IPE_TO_DMA_YCC_CHANNEL_REGISTER_0 local_reg0;

	if (sao_y == 0) {
		DBG_ERR("IMG Output addresses cannot be 0x0!\r\n");
		return;
	}

	//local_reg0.reg = IPE_GETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_0_OFS);
	ipeg->reg_14.bit.dram_sao_y = sao_y >> 2;

	//IPE_SETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_0_OFS, local_reg0.reg);
}

/**
	IPE set DMA output c address

	Configure DMA output c address.

	@param sao_c DMA output starting address of C channel.

	@return None.
*/
void ipe_set_dma_out_addr_c(UINT32 sao_c)
{
	//T_IPE_TO_DMA_YCC_CHANNEL_REGISTER_1 local_reg1;

	if (sao_c == 0) {
		DBG_ERR("IMG Output addresses cannot be 0x0!\r\n");
		return;
	}

	//local_reg1.reg = IPE_GETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_1_OFS);
	ipeg->reg_15.bit.dram_sao_c = sao_c >> 2;

	//IPE_SETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_1_OFS, local_reg1.reg);
}

/**
	IPE set DMA output y line offset

	Configure DMA output y line offset.

	@param ofso_y output line offset(word) of Y channel.


	@return None.
*/
void ipe_set_dma_out_offset_y(UINT32 ofso_y)
{
	//T_IPE_TO_DMA_YCC_CHANNEL_REGISTER_2 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_2_OFS);
	ipeg->reg_16.bit.dram_ofso_y = ofso_y >> 2;

	//IPE_SETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_2_OFS, local_reg.reg);
}


/**
	IPE set DMA output c line offset

	Configure DMA output c line offset.

	@param ofso_c output line offset(word) of C channel.


	@return None.
*/
void ipe_set_dma_out_offset_c(UINT32 ofso_c)
{
	//T_IPE_TO_DMA_YCC_CHANNEL_REGISTER_3 local_reg1;

	//local_reg1.reg = IPE_GETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_3_OFS);
	ipeg->reg_17.bit.dram_ofso_c = ofso_c >> 2;

	//IPE_SETREG(IPE_TO_DMA_YCC_CHANNEL_REGISTER_3_OFS, local_reg1.reg);
}

/**
	IPE interrupt enable status

	Get current IPE interrupt enable status

	@return IPE interrupt status enable readout.

	@return None
*/
UINT32 ipe_get_int_enable(void)
{
	//UINT32 reg;

	//reg = IPE_GETREG(IPE_INTERRUPT_ENABLE_REGISTER_OFS);
	return ipeg->reg_20.word;
}

/**
	Enable IPE interrupt

	IPE interrupt enable selection.

	@param intr 1's in this word will activate corresponding interrupt.

	@return None.
*/
void ipe_enable_int(UINT32 intr)
{
	//T_IPE_INTERRUPT_ENABLE_REGISTER local_reg;

	ipeg->reg_20.word = intr;

	//IPE_SETREG(IPE_INTERRUPT_ENABLE_REGISTER_OFS, local_reg.reg);
}

/**
	IPE interrupt status

	Get current IPE interrupt status

	@return IPE interrupt status readout.

	@return None.
*/
UINT32 ipe_get_int_status(void)
{
	//UINT32 reg;

	//reg = IPE_GETREG(IPE_INTERRUPT_STATUS_REGISTER_OFS);
	return ipeg->reg_21.word;
}

/**
	IPE interrupt status clear

	Clear IPE interrupt status.

	@param intr 1's in this word will clear corresponding interrupt status.

	@return None.
*/
void ipe_clear_int(UINT32 intr)
{
	//IPE_SETREG(IPE_INTERRUPT_STATUS_REGISTER_OFS, intr);
	ipeg->reg_21.word = intr;
}

/**
	IPE get debug status

	IPE get debug status.

	@param None

	@return debug status
*/
UINT32 ipe_get_debug_status(void)
{
	//T_DEBUG_STATUS_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(DEBUG_STATUS_REGISTER_OFS);

	return ipeg->reg_22.word;
}

/**
	IPE check busy

	IPE check status is idle or busy now

	@param None

	@return idle:0/busy:1
*/
BOOL ipe_check_busy(void)
{
	//T_DEBUG_STATUS_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(DEBUG_STATUS_REGISTER_OFS);
	if (ipeg->reg_22.bit.ipestatus) {
		return 1;
	} else {
		return 0;
	}
}

/**
	IPE set tone remap LUT

	Set IPE Tone remap LUT.

	@param *p_tonemap_lut pointer to tone remap LUT.

	@return None.
*/
void ipe_set_tone_remap(UINT32 *p_tonemap_lut)
{
	UINT32 i = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	//T_EDGE_GAMMA0 local_reg0;
	//T_EDGE_GAMMA1 local_reg1;
	//T_EDGE_GAMMA2 local_reg2;
	//T_EDGE_GAMMA3 local_reg3;
	//T_EDGE_GAMMA4 local_reg4;
	//T_EDGE_GAMMA5 local_reg5;
	//T_EDGE_GAMMA6 local_reg6;
	//T_EDGE_GAMMA7 local_reg7;
	//T_EDGE_GAMMA8 local_reg8;
	//T_EDGE_GAMMA9 local_reg9;
	//T_EDGE_GAMMA10 local_reg10;
	//T_EDGE_GAMMA11 local_reg11;
	//T_EDGE_GAMMA12 local_reg12;
	//T_EDGE_GAMMA13 local_reg13;
	//T_EDGE_GAMMA14 local_reg14;
	//T_EDGE_GAMMA15 local_reg15;
	//T_EDGE_GAMMA16 local_reg16;
	//T_EDGE_GAMMA17 local_reg17;
	//T_EDGE_GAMMA18 local_reg18;
	//T_EDGE_GAMMA19 local_reg19;
	//T_EDGE_GAMMA20 local_reg20;
#endif

	//T_EDGE_GAMMA21 local_reg21;

#if 0
	//local_reg0.reg = IPE_GETREG(EDGE_GAMMA0_OFS);
	ipeg->reg_512.bit.edge_lut_0 = p_tonemap_lut[0];
	ipeg->reg_512.bit.edge_lut_1 = p_tonemap_lut[1];
	ipeg->reg_512.bit.edge_lut_2 = p_tonemap_lut[2];
	//local_reg1.reg = IPE_GETREG(EDGE_GAMMA1_OFS);
	ipeg->reg_513.bit.edge_lut_3 = p_tonemap_lut[3];
	ipeg->reg_513.bit.edge_lut_4 = p_tonemap_lut[4];
	ipeg->reg_513.bit.edge_lut_5 = p_tonemap_lut[5];
	//local_reg2.reg = IPE_GETREG(EDGE_GAMMA2_OFS);
	ipeg->reg_514.bit.edge_lut_6 = p_tonemap_lut[6];
	ipeg->reg_514.bit.edge_lut_7 = p_tonemap_lut[7];
	ipeg->reg_514.bit.edge_lut_8 = p_tonemap_lut[8];
	//local_reg3.reg = IPE_GETREG(EDGE_GAMMA3_OFS);
	ipeg->reg_515.bit.edge_lut_9 = p_tonemap_lut[9];
	ipeg->reg_515.bit.edge_lut_10 = p_tonemap_lut[10];
	ipeg->reg_515.bit.edge_lut_11 = p_tonemap_lut[11];
	//local_reg4.reg = IPE_GETREG(EDGE_GAMMA4_OFS);
	ipeg->reg_516.bit.edge_lut_12 = p_tonemap_lut[12];
	ipeg->reg_516.bit.edge_lut_13 = p_tonemap_lut[13];
	ipeg->reg_516.bit.edge_lut_14 = p_tonemap_lut[14];
	//local_reg5.reg = IPE_GETREG(EDGE_GAMMA5_OFS);
	ipeg->reg_517.bit.edge_lut_15 = p_tonemap_lut[15];
	ipeg->reg_517.bit.edge_lut_16 = p_tonemap_lut[16];
	ipeg->reg_517.bit.edge_lut_17 = p_tonemap_lut[17];
	//local_reg6.reg = IPE_GETREG(EDGE_GAMMA6_OFS);
	ipeg->reg_518.bit.edge_lut_18 = p_tonemap_lut[18];
	ipeg->reg_518.bit.edge_lut_19 = p_tonemap_lut[19];
	ipeg->reg_518.bit.edge_lut_20 = p_tonemap_lut[20];
	//local_reg7.reg = IPE_GETREG(EDGE_GAMMA7_OFS);
	ipeg->reg_519.bit.edge_lut_21 = p_tonemap_lut[21];
	ipeg->reg_519.bit.edge_lut_22 = p_tonemap_lut[22];
	ipeg->reg_519.bit.edge_lut_23 = p_tonemap_lut[23];
	//local_reg8.reg = IPE_GETREG(EDGE_GAMMA8_OFS);
	ipeg->reg_520.bit.edge_lut_24 = p_tonemap_lut[24];
	ipeg->reg_520.bit.edge_lut_25 = p_tonemap_lut[25];
	ipeg->reg_520.bit.edge_lut_26 = p_tonemap_lut[26];
	//local_reg9.reg = IPE_GETREG(EDGE_GAMMA9_OFS);
	ipeg->reg_521.bit.edge_lut_27 = p_tonemap_lut[27];
	ipeg->reg_521.bit.edge_lut_28 = p_tonemap_lut[28];
	ipeg->reg_521.bit.edge_lut_29 = p_tonemap_lut[29];
	//local_reg10.reg = IPE_GETREG(EDGE_GAMMA10_OFS);
	ipeg->reg_522.bit.edge_lut_30 = p_tonemap_lut[30];
	ipeg->reg_522.bit.edge_lut_31 = p_tonemap_lut[31];
	ipeg->reg_522.bit.edge_lut_32 = p_tonemap_lut[32];
	//local_reg11.reg = IPE_GETREG(EDGE_GAMMA11_OFS);
	ipeg->reg_523.bit.edge_lut_33 = p_tonemap_lut[33];
	ipeg->reg_523.bit.edge_lut_34 = p_tonemap_lut[34];
	ipeg->reg_523.bit.edge_lut_35 = p_tonemap_lut[35];
	//local_reg12.reg = IPE_GETREG(EDGE_GAMMA12_OFS);
	ipeg->reg_524.bit.edge_lut_36 = p_tonemap_lut[36];
	ipeg->reg_524.bit.edge_lut_37 = p_tonemap_lut[37];
	ipeg->reg_524.bit.edge_lut_38 = p_tonemap_lut[38];
	//local_reg13.reg = IPE_GETREG(EDGE_GAMMA13_OFS);
	ipeg->reg_525.bit.edge_lut_39 = p_tonemap_lut[39];
	ipeg->reg_525.bit.edge_lut_40 = p_tonemap_lut[40];
	ipeg->reg_525.bit.edge_lut_41 = p_tonemap_lut[41];
	//local_reg14.reg = IPE_GETREG(EDGE_GAMMA14_OFS);
	ipeg->reg_526.bit.edge_lut_42 = p_tonemap_lut[42];
	ipeg->reg_526.bit.edge_lut_43 = p_tonemap_lut[43];
	ipeg->reg_526.bit.edge_lut_44 = p_tonemap_lut[44];
	//local_reg15.reg = IPE_GETREG(EDGE_GAMMA15_OFS);
	ipeg->reg_527.bit.edge_lut_45 = p_tonemap_lut[45];
	ipeg->reg_527.bit.edge_lut_46 = p_tonemap_lut[46];
	ipeg->reg_527.bit.edge_lut_47 = p_tonemap_lut[47];
	//local_reg16.reg = IPE_GETREG(EDGE_GAMMA16_OFS);
	ipeg->reg_528.bit.edge_lut_48 = p_tonemap_lut[48];
	ipeg->reg_528.bit.edge_lut_49 = p_tonemap_lut[49];
	ipeg->reg_528.bit.edge_lut_50 = p_tonemap_lut[50];
	//local_reg17.reg = IPE_GETREG(EDGE_GAMMA17_OFS);
	ipeg->reg_529.bit.edge_lut_51 = p_tonemap_lut[51];
	ipeg->reg_529.bit.edge_lut_52 = p_tonemap_lut[52];
	ipeg->reg_529.bit.edge_lut_53 = p_tonemap_lut[53];
	//local_reg18.reg = IPE_GETREG(EDGE_GAMMA18_OFS);
	ipeg->reg_530.bit.edge_lut_54 = p_tonemap_lut[54];
	ipeg->reg_530.bit.edge_lut_55 = p_tonemap_lut[55];
	ipeg->reg_530.bit.edge_lut_56 = p_tonemap_lut[56];
	//local_reg19.reg = IPE_GETREG(EDGE_GAMMA19_OFS);
	ipeg->reg_531.bit.edge_lut_57 = p_tonemap_lut[57];
	ipeg->reg_531.bit.edge_lut_58 = p_tonemap_lut[58];
	ipeg->reg_531.bit.edge_lut_59 = p_tonemap_lut[59];
	//local_reg20.reg = IPE_GETREG(EDGE_GAMMA20_OFS);
	ipeg->reg_532.bit.edge_lut_60 = p_tonemap_lut[60];
	ipeg->reg_532.bit.edge_lut_61 = p_tonemap_lut[61];
	ipeg->reg_532.bit.edge_lut_62 = p_tonemap_lut[62];
#endif


#if 0
	//IPE_SETREG(EDGE_GAMMA0_OFS, local_reg0.reg);
	//IPE_SETREG(EDGE_GAMMA1_OFS, local_reg1.reg);
	//IPE_SETREG(EDGE_GAMMA2_OFS, local_reg2.reg);
	//IPE_SETREG(EDGE_GAMMA3_OFS, local_reg3.reg);
	//IPE_SETREG(EDGE_GAMMA4_OFS, local_reg4.reg);
	//IPE_SETREG(EDGE_GAMMA5_OFS, local_reg5.reg);
	//IPE_SETREG(EDGE_GAMMA6_OFS, local_reg6.reg);
	//IPE_SETREG(EDGE_GAMMA7_OFS, local_reg7.reg);
	//IPE_SETREG(EDGE_GAMMA8_OFS, local_reg8.reg);
	//IPE_SETREG(EDGE_GAMMA9_OFS, local_reg9.reg);
	//IPE_SETREG(EDGE_GAMMA10_OFS, local_reg10.reg);
	//IPE_SETREG(EDGE_GAMMA11_OFS, local_reg11.reg);
	//IPE_SETREG(EDGE_GAMMA12_OFS, local_reg12.reg);
	//IPE_SETREG(EDGE_GAMMA13_OFS, local_reg13.reg);
	//IPE_SETREG(EDGE_GAMMA14_OFS, local_reg14.reg);
	//IPE_SETREG(EDGE_GAMMA15_OFS, local_reg15.reg);
	//IPE_SETREG(EDGE_GAMMA16_OFS, local_reg16.reg);
	//IPE_SETREG(EDGE_GAMMA17_OFS, local_reg17.reg);
	//IPE_SETREG(EDGE_GAMMA18_OFS, local_reg18.reg);
	//IPE_SETREG(EDGE_GAMMA19_OFS, local_reg19.reg);
	//IPE_SETREG(EDGE_GAMMA20_OFS, local_reg20.reg);
#endif

	for (i = 0; i < 21; i++) {
		//reg_set_val = 0;

		reg_ofs = EDGE_GAMMA0_OFS + (i << 2);
		reg_set_val = (p_tonemap_lut[(i * 3)]) + ((p_tonemap_lut[(i * 3) + 1]) << 10) + ((p_tonemap_lut[(i * 3) + 2]) << 20);

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	//local_reg21.reg = IPE_GETREG(EDGE_GAMMA21_OFS);
	ipeg->reg_533.bit.edge_lut_63 = p_tonemap_lut[63];
	ipeg->reg_533.bit.edge_lut_64 = p_tonemap_lut[64];
	//IPE_SETREG(EDGE_GAMMA21_OFS, local_reg21.reg);
}

/**
	IPE set edge selection

	IPE edge extraction diagonal coefficients setting.

	@param gamsel
	@param chsel

	@return None.
*/
void ipe_set_edge_extract_sel(IPE_EEXT_GAM_ENUM gam_sel, IPE_EEXT_CH_ENUM ch_sel)
{
	//T_EDGE_EXTRACTION_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_0_OFS);
	ipeg->reg_28.bit.eext_gamsel = gam_sel;
	ipeg->reg_28.bit.eext_chsel = ch_sel;

	//IPE_SETREG(EDGE_EXTRACTION_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE set edge kernel B

	IPE edge extraction diagonal coefficients setting.

	@param eext integer in 2's complement representation, 0~1023 or -512~511.
	@param eextdiv edge extraction normalization factor, 0~15.

	@return None.
*/
void ipe_set_edge_extract_b(INT16 *p_eext_ker, UINT32 eext_div_b, UINT8 eext_enh_b)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_EDGE_EXTRACTION_REGISTER_0 local_reg0;
	//T_EDGE_EXTRACTION_REGISTER_1 local_reg1;
	//T_EDGE_EXTRACTION_REGISTER_2 local_reg2;
	//T_EDGE_EXTRACTION_REGISTER_3 local_reg3;

	//local_reg0.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_0_OFS);
	ipeg->reg_28.bit.eext0 = p_eext_ker[0];
	ipeg->reg_28.bit.eextdiv_e7 = eext_div_b;
	ipeg->reg_28.bit.eext_enh_e7 = eext_enh_b;
	//IPE_SETREG(EDGE_EXTRACTION_REGISTER_0_OFS, local_reg0.reg);

#if 0
	//local_reg1.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_1_OFS);
	ipeg->reg_29.bit.eext1 = p_eext_ker[1];
	ipeg->reg_29.bit.eext2 = p_eext_ker[2];
	ipeg->reg_29.bit.eext3 = p_eext_ker[3];

	//local_reg2.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_2_OFS);
	ipeg->reg_30.bit.eext4 = p_eext_ker[4];
	ipeg->reg_30.bit.eext5 = p_eext_ker[5];
	ipeg->reg_30.bit.eext6 = p_eext_ker[6];

	//local_reg3.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_3_OFS);
	ipeg->reg_31.bit.eext7 = p_eext_ker[7];
	ipeg->reg_31.bit.eext8 = p_eext_ker[8];
	ipeg->reg_31.bit.eext9 = p_eext_ker[9];
	//IPE_SETREG(EDGE_EXTRACTION_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(EDGE_EXTRACTION_REGISTER_2_OFS, local_reg2.reg);
	//IPE_SETREG(EDGE_EXTRACTION_REGISTER_3_OFS, local_reg3.reg);
#endif

	for (i = 0; i < 3; i++) {
		//reg_set_val = 0;

		reg_ofs = EDGE_EXTRACTION_REGISTER_1_OFS + (i << 2);

		idx = (i * 3);
		reg_set_val = ((UINT32)(p_eext_ker[idx + 1] & 0x03FF)) + (((UINT32)(p_eext_ker[idx + 2] & 0x03FF)) << 10) + (((UINT32)(p_eext_ker[idx + 3] & 0x03FF)) << 20);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
	IPE set edge kernel strength
*/
void ipe_set_eext_kerstrength(IPE_EEXT_KER_STRENGTH *p_ker_str)
{
	//T_EDGE_EXTRACTION_REGISTER_4 local_reg0;
	//T_EDGE_EXTRACTION_REGISTER_5 local_reg1;

	//local_reg0.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_4_OFS);
	ipeg->reg_32.bit.eext_enh_e3 = p_ker_str->ker_a.eext_enh;
	ipeg->reg_32.bit.eext_enh_e5a = p_ker_str->ker_c.eext_enh;
	ipeg->reg_32.bit.eext_enh_e5b = p_ker_str->ker_d.eext_enh;
	//local_reg1.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_5_OFS);
	ipeg->reg_33.bit.eextdiv_e3 =  p_ker_str->ker_a.eext_div;
	ipeg->reg_33.bit.eextdiv_e5a =	p_ker_str->ker_c.eext_div;
	ipeg->reg_33.bit.eextdiv_e5b =	p_ker_str->ker_d.eext_div;

	//IPE_SETREG(EDGE_EXTRACTION_REGISTER_4_OFS, local_reg0.reg);
	//IPE_SETREG(EDGE_EXTRACTION_REGISTER_5_OFS, local_reg1.reg);
}

void ipe_set_eext_engcon(IPE_EEXT_ENG_CON *p_eext_engcon)
{
	//T_EDGE_EXTRACTION_REGISTER_5 local_reg0;
	//T_EDGE_REGION_EXTRACTION_REGISTER_0 local_reg1;

	//local_reg0.reg = IPE_GETREG(EDGE_EXTRACTION_REGISTER_5_OFS);
	ipeg->reg_33.bit.eextdiv_eng =	p_eext_engcon->eext_div_eng;
	ipeg->reg_33.bit.eextdiv_con =	p_eext_engcon->eext_div_con;

	//local_reg1.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_0_OFS);
	ipeg->reg_34.bit.w_con_eng = p_eext_engcon->wt_con_eng;

	//IPE_SETREG(EDGE_EXTRACTION_REGISTER_5_OFS, local_reg0.reg);
	//IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_0_OFS, local_reg1.reg);
}

void ipe_set_ker_thickness(IPE_KER_THICKNESS *p_ker_thickness)
{
	//T_EDGE_REGION_EXTRACTION_REGISTER_1 local_reg0;

	//local_reg0.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_1_OFS);
	ipeg->reg_35.bit.w_ker_thin =  p_ker_thickness->wt_ker_thin;
	ipeg->reg_35.bit.w_ker_robust =  p_ker_thickness->wt_ker_robust;
	ipeg->reg_35.bit.iso_ker_thin =  p_ker_thickness->iso_ker_thin;
	ipeg->reg_35.bit.iso_ker_robust =  p_ker_thickness->iso_ker_robust;

	//IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_1_OFS, local_reg0.reg);
}

void ipe_set_ker_thickness_hld(IPE_KER_THICKNESS *p_ker_thickness_hld)
{
	//T_EDGE_REGION_EXTRACTION_REGISTER_2 local_reg0;

	//local_reg0.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_2_OFS);
	ipeg->reg_36.bit.w_ker_thin_hld =  p_ker_thickness_hld->wt_ker_thin;
	ipeg->reg_36.bit.w_ker_robust_hld =  p_ker_thickness_hld->wt_ker_robust;
	ipeg->reg_36.bit.iso_ker_thin_hld =  p_ker_thickness_hld->iso_ker_thin;
	ipeg->reg_36.bit.iso_ker_robust_hld =  p_ker_thickness_hld->iso_ker_robust;

	//IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_2_OFS, local_reg0.reg);
}

void ipe_set_region(IPE_REGION_PARAM *p_region)
{
	//T_EDGE_REGION_EXTRACTION_REGISTER_0 local_reg0;
	//T_EDGE_REGION_EXTRACTION_REGISTER_2 local_reg1;
	//T_EDGE_REGION_EXTRACTION_REGISTER_3 local_reg2;
	//T_EDGE_REGION_EXTRACTION_REGISTER_4 local_reg3;

	//local_reg0.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_0_OFS);
	ipeg->reg_34.bit.w_low = p_region->wt_low;
	ipeg->reg_34.bit.w_high = p_region->wt_high;
	//local_reg1.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_2_OFS);
	ipeg->reg_36.bit.w_hld_low = p_region->wt_low_hld;
	ipeg->reg_36.bit.w_hld_high = p_region->wt_high_hld;
	//local_reg2.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_3_OFS);
	ipeg->reg_37.bit.th_flat = p_region->th_flat;
	ipeg->reg_37.bit.th_edge = p_region->th_edge;
	//local_reg3.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_4_OFS);
	ipeg->reg_38.bit.th_flat_hld = p_region->th_flat_hld;
	ipeg->reg_38.bit.th_edge_hld = p_region->th_edge_hld;
	ipeg->reg_38.bit.th_lum_hld = p_region->th_lum_hld;

	//IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_2_OFS, local_reg1.reg);
	//IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_3_OFS, local_reg2.reg);
	//IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_4_OFS, local_reg3.reg);

}

void ipe_set_region_slope(UINT16 slope, UINT16 slope_hld)
{
	//T_EDGE_REGION_EXTRACTION_REGISTER_5 local_reg0;

	//local_reg0.reg = IPE_GETREG(EDGE_REGION_EXTRACTION_REGISTER_5_OFS);
	ipeg->reg_39.bit.slope_con_eng = slope;
	ipeg->reg_39.bit.slope_hld_con_eng = slope_hld;

	//IPE_SETREG(EDGE_REGION_EXTRACTION_REGISTER_5_OFS, local_reg0.reg);
}

void ipe_set_overshoot(IPE_OVERSHOOT_PARAM *p_overshoot)
{
	//T_OVERSHOOTING_CONTROL_REGISTER_0 local_reg0;
	//T_OVERSHOOTING_CONTROL_REGISTER_1 local_reg1;
	//T_OVERSHOOTING_CONTROL_REGISTER_2 local_reg2;
	//T_OVERSHOOTING_CONTROL_REGISTER_3 local_reg3;
	//T_OVERSHOOTING_CONTROL_REGISTER_4 local_reg4;

	//local_reg0.reg = IPE_GETREG(OVERSHOOTING_CONTROL_REGISTER_0_OFS);
	ipeg->reg_40.bit.overshoot_en = p_overshoot->overshoot_en;
	ipeg->reg_40.bit.wt_overshoot = p_overshoot->wt_overshoot;
	ipeg->reg_40.bit.wt_undershoot = p_overshoot->wt_undershoot;

	//local_reg1.reg = IPE_GETREG(OVERSHOOTING_CONTROL_REGISTER_1_OFS);
	ipeg->reg_41.bit.th_overshoot = p_overshoot->th_overshoot;
	ipeg->reg_41.bit.th_undershoot = p_overshoot->th_undershoot;
	ipeg->reg_41.bit.th_undershoot_lum = p_overshoot->th_undershoot_lum;
	ipeg->reg_41.bit.th_undershoot_eng = p_overshoot->th_undershoot_eng;

	//local_reg2.reg = IPE_GETREG(OVERSHOOTING_CONTROL_REGISTER_2_OFS);
	ipeg->reg_42.bit.clamp_wt_mod_lum = p_overshoot->clamp_wt_mod_lum;
	ipeg->reg_42.bit.clamp_wt_mod_eng = p_overshoot->clamp_wt_mod_eng;
	ipeg->reg_42.bit.strength_lum_eng = p_overshoot->strength_lum_eng;
	ipeg->reg_42.bit.norm_lum_eng = p_overshoot->norm_lum_eng;

	//local_reg3.reg = IPE_GETREG(OVERSHOOTING_CONTROL_REGISTER_3_OFS);
	ipeg->reg_43.bit.slope_overshoot = p_overshoot->slope_overshoot;
	ipeg->reg_43.bit.slope_undershoot = p_overshoot->slope_undershoot;

	//local_reg4.reg = IPE_GETREG(OVERSHOOTING_CONTROL_REGISTER_3_OFS);
	ipeg->reg_44.bit.slope_undershoot_lum = p_overshoot->slope_undershoot_lum;
	ipeg->reg_44.bit.slope_undershoot_eng = p_overshoot->slope_undershoot_eng;

	//IPE_SETREG(OVERSHOOTING_CONTROL_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(OVERSHOOTING_CONTROL_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(OVERSHOOTING_CONTROL_REGISTER_2_OFS, local_reg2.reg);
	//IPE_SETREG(OVERSHOOTING_CONTROL_REGISTER_3_OFS, local_reg3.reg);
	//IPE_SETREG(OVERSHOOTING_CONTROL_REGISTER_4_OFS, local_reg4.reg);

}

void ipe_get_edge_stcs(IPE_EDGE_STCS_RSLT *stcs_result)
{
	//T_EDGE_STATISTIC_REGISTER local_reg0;

	//local_reg0.reg = IPE_GETREG(EDGE_STATISTIC_REGISTER_OFS);
	stcs_result->localmax_max = ipeg->reg_55.bit.localmax_statistics_max;
	stcs_result->coneng_max = ipeg->reg_55.bit.coneng_statistics_max;
	stcs_result->coneng_avg = ipeg->reg_55.bit.coneng_statistics_avg;
}


/**
	IPE set luminance map thresholds

	Set IPE edge mapping: luminance thresholds.

	@param p_esmap edge luminance map infor

	@return None.
*/
void ipe_set_esmap_th(IPE_ESMAP_INFOR *p_esmap)
{
	//T_EDGE_LUMINANCE_PROCESS_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(EDGE_LUMINANCE_PROCESS_REGISTER_0_OFS);
	ipeg->reg_45.bit.esthrl = p_esmap->ethr_low;
	ipeg->reg_45.bit.esthrh = p_esmap->ethr_high;
	ipeg->reg_45.bit.establ = p_esmap->etab_low;
	ipeg->reg_45.bit.estabh = p_esmap->etab_high;

	//IPE_SETREG(EDGE_LUMINANCE_PROCESS_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE set Estab table

	IPE edge control luminance mapping.

	@param *estab16tbl pointer to edge profile mapping table.

	@return None.
*/
void ipe_set_estab(UINT8 *p_estab)
{
	UINT32 i = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	//T_EDGE_LUMINANCE_PROCESS_REGISTER_1 local_reg0;
	//T_EDGE_LUMINANCE_PROCESS_REGISTER_2 local_reg1;
	//T_EDGE_LUMINANCE_PROCESS_REGISTER_3 local_reg2;
	//T_EDGE_LUMINANCE_PROCESS_REGISTER_4 local_reg3;

	local_reg0.reg = 0;
	ipeg->reg_46.bit.eslutl_0 = p_estab[0];
	ipeg->reg_46.bit.eslutl_1 = p_estab[1];
	ipeg->reg_46.bit.eslutl_2 = p_estab[2];
	ipeg->reg_46.bit.eslutl_3 = p_estab[3];

	local_reg1.reg = 0;
	ipeg->reg_47.bit.eslutl_4 = p_estab[4];
	ipeg->reg_47.bit.eslutl_5 = p_estab[5];
	ipeg->reg_47.bit.eslutl_6 = p_estab[6];
	ipeg->reg_47.bit.eslutl_7 = p_estab[7];

	local_reg2.reg = 0;
	ipeg->reg_48.bit.esluth_0 = p_estab[8];
	ipeg->reg_48.bit.esluth_1 = p_estab[9];
	ipeg->reg_48.bit.esluth_2 = p_estab[10];
	ipeg->reg_48.bit.esluth_3 = p_estab[11];

	local_reg3.reg = 0;
	ipeg->reg_49.bit.esluth_4 = p_estab[12];
	ipeg->reg_49.bit.esluth_5 = p_estab[13];
	ipeg->reg_49.bit.esluth_6 = p_estab[14];
	ipeg->reg_49.bit.esluth_7 = p_estab[15];

	//IPE_SETREG(EDGE_LUMINANCE_PROCESS_REGISTER_1_OFS, local_reg0.reg);
	//IPE_SETREG(EDGE_LUMINANCE_PROCESS_REGISTER_2_OFS, local_reg1.reg);
	//IPE_SETREG(EDGE_LUMINANCE_PROCESS_REGISTER_3_OFS, local_reg2.reg);
	//IPE_SETREG(EDGE_LUMINANCE_PROCESS_REGISTER_4_OFS, local_reg3.reg);
#endif

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = EDGE_LUMINANCE_PROCESS_REGISTER_1_OFS + (i << 2);
		reg_set_val = (p_estab[(i << 2)] & 0xFF) + ((p_estab[(i << 2) + 1] & 0xFF) << 8) + ((p_estab[(i << 2) + 2] & 0xFF) << 16) + ((p_estab[(i << 2) + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
	IPE set edge map thresholds

	Set IPE edge mapping thresholds.

	@param p_edmap edge map infor

	@return None.
*/
void ipe_set_edgemap_th(IPE_EDGEMAP_INFOR *p_edmap)
{
	//T_EDGE_DMAP_PROCESS_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(EDGE_DMAP_PROCESS_REGISTER_0_OFS);
	ipeg->reg_50.bit.edthrl = p_edmap->ethr_low;
	ipeg->reg_50.bit.edthrh = p_edmap->ethr_high;
	ipeg->reg_50.bit.edtabl = p_edmap->etab_low;
	ipeg->reg_50.bit.edtabh = p_edmap->etab_high;
	ipeg->reg_50.bit.edin_sel = p_edmap->ein_sel;

	//IPE_SETREG(EDGE_DMAP_PROCESS_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE set edge map

	Set IPE Edge map 16 tap table.

	@param *emap16tbl pointer to edge map table.

	@return None.
*/
void ipe_set_edgemap_tab(UINT8 *p_edtab)
{
	UINT32 i = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	//T_EDGE_DMAP_PROCESS_REGISTER_1 local_reg0;
	//T_EDGE_DMAP_PROCESS_REGISTER_2 local_reg1;
	//T_EDGE_DMAP_PROCESS_REGISTER_3 local_reg2;
	//T_EDGE_DMAP_PROCESS_REGISTER_4 local_reg3;

	local_reg0.reg = 0;
	ipeg->reg_51.bit.edlutl_0 = p_edtab[0];
	ipeg->reg_51.bit.edlutl_1 = p_edtab[1];
	ipeg->reg_51.bit.edlutl_2 = p_edtab[2];
	ipeg->reg_51.bit.edlutl_3 = p_edtab[3];

	local_reg1.reg = 0;
	ipeg->reg_52.bit.edlutl_4 = p_edtab[4];
	ipeg->reg_52.bit.edlutl_5 = p_edtab[5];
	ipeg->reg_52.bit.edlutl_6 = p_edtab[6];
	ipeg->reg_52.bit.edlutl_7 = p_edtab[7];

	local_reg2.reg = 0;
	ipeg->reg_53.bit.edluth_0 = p_edtab[8];
	ipeg->reg_53.bit.edluth_1 = p_edtab[9];
	ipeg->reg_53.bit.edluth_2 = p_edtab[10];
	ipeg->reg_53.bit.edluth_3 = p_edtab[11];

	local_reg3.reg = 0;
	ipeg->reg_54.bit.edluth_4 = p_edtab[12];
	ipeg->reg_54.bit.edluth_5 = p_edtab[13];
	ipeg->reg_54.bit.edluth_6 = p_edtab[14];
	ipeg->reg_54.bit.edluth_7 = p_edtab[15];

	//IPE_SETREG(EDGE_DMAP_PROCESS_REGISTER_1_OFS, local_reg0.reg);
	//IPE_SETREG(EDGE_DMAP_PROCESS_REGISTER_2_OFS, local_reg1.reg);
	//IPE_SETREG(EDGE_DMAP_PROCESS_REGISTER_3_OFS, local_reg2.reg);
	//IPE_SETREG(EDGE_DMAP_PROCESS_REGISTER_4_OFS, local_reg3.reg);
#endif

	for (i = 0; i < 4; i += 1) {
		//reg_set_val = 0;

		reg_ofs = EDGE_DMAP_PROCESS_REGISTER_1_OFS + (i << 2);
		reg_set_val = (p_edtab[(i << 2)] & 0xFF) + ((p_edtab[(i << 2) + 1] & 0xFF) << 8) + ((p_edtab[(i << 2) + 2] & 0xFF) << 16) + ((p_edtab[(i << 2) + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

void ipe_set_rgblpf(IPE_RGBLPF_PARAM *p_lpf_info)
{
	//T_RGB_LPF_REGISTER_0 local_reg0;
	//T_RGB_LPF_REGISTER_1 local_reg1;
	//T_RGB_LPF_REGISTER_2 local_reg2;

	ipeg->reg_60.word = 0;
	ipeg->reg_60.bit.glpfw = p_lpf_info->lpf_g_info.lpfw;
	ipeg->reg_60.bit.gsonlyw = p_lpf_info->lpf_g_info.sonly_w;
	ipeg->reg_60.bit.g_rangeth0 = p_lpf_info->lpf_g_info.range_th0;
	ipeg->reg_60.bit.g_rangeth1 = p_lpf_info->lpf_g_info.range_th1;
	ipeg->reg_60.bit.g_filtsize = p_lpf_info->lpf_g_info.filt_size;

	ipeg->reg_61.word = 0;
	ipeg->reg_61.bit.rlpfw = p_lpf_info->lpf_r_info.lpfw;
	ipeg->reg_61.bit.rsonlyw = p_lpf_info->lpf_r_info.sonly_w;
	ipeg->reg_61.bit.r_rangeth0 = p_lpf_info->lpf_r_info.range_th0;
	ipeg->reg_61.bit.r_rangeth1 = p_lpf_info->lpf_r_info.range_th1;
	ipeg->reg_61.bit.r_filtsize = p_lpf_info->lpf_r_info.filt_size;

	ipeg->reg_62.word = 0;
	ipeg->reg_62.bit.blpfw = p_lpf_info->lpf_b_info.lpfw;
	ipeg->reg_62.bit.bsonlyw = p_lpf_info->lpf_b_info.sonly_w;
	ipeg->reg_62.bit.b_rangeth0 = p_lpf_info->lpf_b_info.range_th0;
	ipeg->reg_62.bit.b_rangeth1 = p_lpf_info->lpf_b_info.range_th1;
	ipeg->reg_62.bit.b_filtsize = p_lpf_info->lpf_b_info.filt_size;

	//IPE_SETREG(RGB_LPF_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(RGB_LPF_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(RGB_LPF_REGISTER_2_OFS, local_reg2.reg);
}

void ipe_set_pfr_general(IPE_PFR_PARAM *p_pfr_info)
{
	//T_PURPLE_FRINGE_REDUCTION_REGISTER0 local_reg0;
	//T_PURPLE_FRINGE_REDUCTION_REGISTER1 local_reg1;


	//local_reg0.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER0_OFS);
	ipeg->reg_232.bit.pfr_uv_filt_en = p_pfr_info->uv_filt_en;
	ipeg->reg_232.bit.pfr_luma_level_en = p_pfr_info->luma_level_en;
	ipeg->reg_232.bit.pfr_out_wet = p_pfr_info->wet_out;

	//local_reg1.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER1_OFS);
	ipeg->reg_233.bit.pfr_edge_th = p_pfr_info->edge_th;
	ipeg->reg_233.bit.pfr_edge_str = p_pfr_info->edge_strength;
	ipeg->reg_233.bit.pfr_g_wet = p_pfr_info->color_wet_g;

	//IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER0_OFS, local_reg0.reg);
	//IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER1_OFS, local_reg1.reg);
}

void ipe_set_pfr_color(UINT32 idx, IPE_PFR_COLOR_WET *p_pfr_color_info)
{
	UINT32 addr_ofs;
	UINT32 mask = 1;
	T_PURPLE_FRINGE_REDUCTION_REGISTER0 local_reg0;
	T_PURPLE_FRINGE_REDUCTION_REGISTER2 local_reg1;
	T_PURPLE_FRINGE_REDUCTION_REGISTER6 local_reg2;
	T_PURPLE_FRINGE_REDUCTION_REGISTER7 local_reg3;

	local_reg0.reg = IPE_GETREG((PURPLE_FRINGE_REDUCTION_REGISTER0_OFS));
	mask = mask << (2 + idx);
	if (idx < 4) {
		addr_ofs = idx * 8;
	} else {
		return;
	}
	local_reg1.reg = IPE_GETREG((PURPLE_FRINGE_REDUCTION_REGISTER2_OFS + idx * 4));
	local_reg2.reg = IPE_GETREG((PURPLE_FRINGE_REDUCTION_REGISTER6_OFS + addr_ofs));
	local_reg3.reg = IPE_GETREG((PURPLE_FRINGE_REDUCTION_REGISTER7_OFS + addr_ofs));

	if (p_pfr_color_info->enable) {
		local_reg0.reg	= local_reg0.reg | (mask & 0xffffffff);
	} else {
		local_reg0.reg	= local_reg0.reg & (~mask & 0xffffffff);
	}

	local_reg1.bit.reg_pfr_color_u0	= p_pfr_color_info->color_u;
	local_reg1.bit.reg_pfr_color_v0	= p_pfr_color_info->color_v;
	local_reg1.bit.reg_pfr_r_wet0	= p_pfr_color_info->color_wet_r;
	local_reg1.bit.reg_pfr_b_wet0	= p_pfr_color_info->color_wet_b;

	local_reg2.bit.reg_pfr_cdif_set0_lut0	= p_pfr_color_info->cdiff_lut[0];
	local_reg2.bit.reg_pfr_cdif_set0_lut1	= p_pfr_color_info->cdiff_lut[1];
	local_reg2.bit.reg_pfr_cdif_set0_lut2	= p_pfr_color_info->cdiff_lut[2];
	local_reg2.bit.reg_pfr_cdif_set0_lut3	= p_pfr_color_info->cdiff_lut[3];

	local_reg3.bit.reg_pfr_cdif_set0_lut4	= p_pfr_color_info->cdiff_lut[4];
	local_reg3.bit.reg_pfr_cdif_set0_th		= p_pfr_color_info->cdiff_th;
	local_reg3.bit.reg_pfr_cdif_set0_step	= p_pfr_color_info->cdiff_step;


	IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER0_OFS, local_reg0.reg);
	IPE_SETREG((PURPLE_FRINGE_REDUCTION_REGISTER2_OFS + idx * 4), local_reg1.reg);
	IPE_SETREG((PURPLE_FRINGE_REDUCTION_REGISTER6_OFS + addr_ofs), local_reg2.reg);
	IPE_SETREG((PURPLE_FRINGE_REDUCTION_REGISTER7_OFS + addr_ofs), local_reg3.reg);
}

void ipe_set_pfr_luma(UINT32 luma_th, UINT32 *luma_table)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_PURPLE_FRINGE_REDUCTION_REGISTER14 local_reg0;
	//T_PURPLE_FRINGE_REDUCTION_REGISTER15 local_reg1;
	//T_PURPLE_FRINGE_REDUCTION_REGISTER16 local_reg2;
	//T_PURPLE_FRINGE_REDUCTION_REGISTER17 local_reg3;

	if (luma_table != NULL) {
	//	local_reg0.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER14_OFS);
		//ipeg->reg_246.bit.pfr_luma_level_0 = luma_table[0];
		//ipeg->reg_246.bit.pfr_luma_level_1 = luma_table[1];
		//ipeg->reg_246.bit.pfr_luma_level_2 = luma_table[2];
		//ipeg->reg_246.bit.pfr_luma_level_3 = luma_table[3];

	//	local_reg1.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER15_OFS);
		//ipeg->reg_247.bit.pfr_luma_level_4 = luma_table[4];
		//ipeg->reg_247.bit.pfr_luma_level_5 = luma_table[5];
		//ipeg->reg_247.bit.pfr_luma_level_6 = luma_table[6];
		//ipeg->reg_247.bit.pfr_luma_level_7 = luma_table[7];

	//	local_reg2.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER16_OFS);
		//ipeg->reg_248.bit.pfr_luma_level_8 = luma_table[8];
		//ipeg->reg_248.bit.pfr_luma_level_9 = luma_table[9];
		//ipeg->reg_248.bit.pfr_luma_level_10 = luma_table[10];
		//ipeg->reg_248.bit.pfr_luma_level_11 = luma_table[11];

		for (i = 0; i < 3; i++) {
			//reg_set_val = 0;

			reg_ofs = PURPLE_FRINGE_REDUCTION_REGISTER14_OFS + (i << 2);

			idx = (i << 2);
			reg_set_val = (luma_table[idx]) + (luma_table[idx+1] << 8) + (luma_table[idx+2] << 16) + (luma_table[idx+3] << 24);

			IPE_SETREG(reg_ofs, reg_set_val);
		}

	//	local_reg3.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER17_OFS);
		ipeg->reg_249.bit.pfr_luma_level_12 = luma_table[12];
		ipeg->reg_249.bit.pfr_luma_th = luma_th;
	} else {
	//	local_reg0.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER14_OFS);
	//	local_reg1.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER15_OFS);
	//	local_reg2.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER16_OFS);
	//	local_reg3.reg = IPE_GETREG(PURPLE_FRINGE_REDUCTION_REGISTER17_OFS);
		ipeg->reg_249.bit.pfr_luma_th = luma_th;
	}

	//IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER14_OFS, local_reg0.reg);
	//IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER15_OFS, local_reg1.reg);
	//IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER16_OFS, local_reg2.reg);
	//IPE_SETREG(PURPLE_FRINGE_REDUCTION_REGISTER17_OFS, local_reg3.reg);
}


/**
	IPE CC matrix setting

	Configure color correction matrix

	@param p_cc_param CC parameters

	@return None.
*/
void ipe_set_color_correct(IPE_CC_PARAM *p_cc_param)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_COLOR_CORRECTION_REGISTER_0 local_reg0;
	//T_COLOR_CORRECTION_REGISTER_1 local_reg1;
	//T_COLOR_CORRECTION_REGISTER_2 local_reg2;
	//T_COLOR_CORRECTION_REGISTER_3 local_reg3;
	//T_COLOR_CORRECTION_REGISTER_4 local_reg4;

	ipeg->reg_64.word = 0;
	ipeg->reg_64.bit.ccrange = p_cc_param->cc_range;
	ipeg->reg_64.bit.cc2_sel = p_cc_param->cc2_sel;
	ipeg->reg_64.bit.cc_gamsel = p_cc_param->cc_gam_sel;
	ipeg->reg_64.bit.ccstab_sel = p_cc_param->cc_stab_sel;
	ipeg->reg_64.bit.ccofs_sel  = p_cc_param->cc_ofs_sel; //sc
	ipeg->reg_64.bit.coef_rr = p_cc_param->p_cc_coeff[0];
	//IPE_SETREG(COLOR_CORRECTION_REGISTER_0_OFS, local_reg0.reg);

#if 0
	local_reg1.reg = 0;
	ipeg->reg_65.bit.coef_rg = p_cc_param->p_cc_coeff[1];
	ipeg->reg_65.bit.coef_rb = p_cc_param->p_cc_coeff[2];

	local_reg2.reg = 0;
	ipeg->reg_66.bit.coef_gr = p_cc_param->p_cc_coeff[3];
	ipeg->reg_66.bit.coef_gg = p_cc_param->p_cc_coeff[4];

	local_reg3.reg = 0;
	ipeg->reg_67.bit.coef_gb = p_cc_param->p_cc_coeff[5];
	ipeg->reg_67.bit.coef_br = p_cc_param->p_cc_coeff[6];

	local_reg4.reg = 0;
	ipeg->reg_68.bit.coef_bg = p_cc_param->p_cc_coeff[7];
	ipeg->reg_68.bit.coef_bb = p_cc_param->p_cc_coeff[8];

	//IPE_SETREG(COLOR_CORRECTION_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_CORRECTION_REGISTER_2_OFS, local_reg2.reg);
	//IPE_SETREG(COLOR_CORRECTION_REGISTER_3_OFS, local_reg3.reg);
	//IPE_SETREG(COLOR_CORRECTION_REGISTER_4_OFS, local_reg4.reg);
#endif

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CORRECTION_REGISTER_1_OFS) + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_cc_param->p_cc_coeff[idx + 1] & 0xFFF) + ((p_cc_param->p_cc_coeff[idx + 2] & 0xFFF) << 16);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
	IPE set Fstab table

	IPE color noise reduction luminance mapping.

	@param *fstab16tbl pointer to luminance profile mapping table.

	@return None.
*/
void ipe_set_fstab(UINT8 *p_fstab_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	//T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_0 local_reg0;
	//T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_1 local_reg1;
	//T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_2 local_reg2;
	//T_COLOR_CORRECTION_STAB_MAPPING_REGISTER_3 local_reg3;

	local_reg0.reg = 0;
	ipeg->reg_69.bit.fstab0 = p_fstab_lut[0];
	ipeg->reg_69.bit.fstab1 = p_fstab_lut[1];
	ipeg->reg_69.bit.fstab2 = p_fstab_lut[2];
	ipeg->reg_69.bit.fstab3 = p_fstab_lut[3];

	local_reg1.reg = 0;
	ipeg->reg_70.bit.fstab4 = p_fstab_lut[4];
	ipeg->reg_70.bit.fstab5 = p_fstab_lut[5];
	ipeg->reg_70.bit.fstab6 = p_fstab_lut[6];
	ipeg->reg_70.bit.fstab7 = p_fstab_lut[7];

	local_reg2.reg = 0;
	ipeg->reg_71.bit.fstab8 = p_fstab_lut[8];
	ipeg->reg_71.bit.fstab9 = p_fstab_lut[9];
	ipeg->reg_71.bit.fstab10 = p_fstab_lut[10];
	ipeg->reg_71.bit.fstab11 = p_fstab_lut[11];

	local_reg3.reg = 0;
	ipeg->reg_72.bit.fstab12 = p_fstab_lut[12];
	ipeg->reg_72.bit.fstab13 = p_fstab_lut[13];
	ipeg->reg_72.bit.fstab14 = p_fstab_lut[14];
	ipeg->reg_72.bit.fstab15 = p_fstab_lut[15];

	//IPE_SETREG(COLOR_CORRECTION_STAB_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(COLOR_CORRECTION_STAB_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_CORRECTION_STAB_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	//IPE_SETREG(COLOR_CORRECTION_STAB_MAPPING_REGISTER_3_OFS, local_reg3.reg);
#endif

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CORRECTION_STAB_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_fstab_lut[idx] & 0xFF) + ((p_fstab_lut[idx + 1] & 0xFF) << 8) + ((p_fstab_lut[idx + 2] & 0xFF) << 16) + ((p_fstab_lut[idx + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
	IPE set Fdtab table

	IPE color noise reduction saturatioin mapping.

	@param *fdtab16tbl pointer to saturation profile mapping table.

	@return None.
*/
void ipe_set_fdtab(UINT8 *p_fdtab_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	//T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0 local_reg0;
	//T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1 local_reg1;
	//T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2 local_reg2;
	//T_COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3 local_reg3;

	local_reg0.reg = 0;
	ipeg->reg_73.bit.fdtab0 = p_fdtab_lut[0];
	ipeg->reg_73.bit.fdtab1 = p_fdtab_lut[1];
	ipeg->reg_73.bit.fdtab2 = p_fdtab_lut[2];
	ipeg->reg_73.bit.fdtab3 = p_fdtab_lut[3];

	local_reg1.reg = 0;
	ipeg->reg_74.bit.fdtab4 = p_fdtab_lut[4];
	ipeg->reg_74.bit.fdtab5 = p_fdtab_lut[5];
	ipeg->reg_74.bit.fdtab6 = p_fdtab_lut[6];
	ipeg->reg_74.bit.fdtab7 = p_fdtab_lut[7];

	local_reg2.reg = 0;
	ipeg->reg_75.bit.fdtab8 = p_fdtab_lut[8];
	ipeg->reg_75.bit.fdtab9 = p_fdtab_lut[9];
	ipeg->reg_75.bit.fdtab10 = p_fdtab_lut[10];
	ipeg->reg_75.bit.fdtab11 = p_fdtab_lut[11];

	local_reg3.reg = 0;
	ipeg->reg_76.bit.fdtab12 = p_fdtab_lut[12];
	ipeg->reg_76.bit.fdtab13 = p_fdtab_lut[13];
	ipeg->reg_76.bit.fdtab14 = p_fdtab_lut[14];
	ipeg->reg_76.bit.fdtab15 = p_fdtab_lut[15];

	//IPE_SETREG(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	//IPE_SETREG(COLOR_CORRECTION_DTAB_MAPPING_REGISTER_3_OFS, local_reg3.reg);
#endif

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CORRECTION_DTAB_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_fdtab_lut[idx] & 0xFF) + ((p_fdtab_lut[idx + 1] & 0xFF) << 8) + ((p_fdtab_lut[idx + 2] & 0xFF) << 16) + ((p_fdtab_lut[idx + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
	IPE set CST coefficient

	IPE color space transform coefficient setting.

	@param cst0 CST coefficient 0.
	@param cst1 CST coefficient 1.
	@param cst2 CST coefficient 2.
	@param cst3 CST coefficient 3.

	@return None.
*/
void ipe_set_cst(IPE_CST_PARAM *p_cst)
{
	//T_COLOR_SPACE_TRANSFORM_REGISTER0 local_reg0;
	//T_COLOR_SPACE_TRANSFORM_REGISTER1 local_reg1;
	//T_COLOR_SPACE_TRANSFORM_REGISTER2 local_reg2;

	//local_reg0.reg = IPE_GETREG(COLOR_SPACE_TRANSFORM_REGISTER0_OFS);
	ipeg->reg_78.bit.coef_yr = p_cst->p_cst_coeff[0];
	ipeg->reg_78.bit.coef_yg = p_cst->p_cst_coeff[1];
	ipeg->reg_78.bit.coef_yb = p_cst->p_cst_coeff[2];

	//local_reg1.reg = IPE_GETREG(COLOR_SPACE_TRANSFORM_REGISTER1_OFS);
	ipeg->reg_79.bit.coef_ur = p_cst->p_cst_coeff[3];
	ipeg->reg_79.bit.coef_ug = p_cst->p_cst_coeff[4];
	ipeg->reg_79.bit.coef_ub = p_cst->p_cst_coeff[5];
	ipeg->reg_79.bit.cstoff_sel = p_cst->cstoff_sel;

	//local_reg2.reg = IPE_GETREG(COLOR_SPACE_TRANSFORM_REGISTER2_OFS);
	ipeg->reg_80.bit.coef_vr = p_cst->p_cst_coeff[6];
	ipeg->reg_80.bit.coef_vg = p_cst->p_cst_coeff[7];
	ipeg->reg_80.bit.coef_vb = p_cst->p_cst_coeff[8];

	//IPE_SETREG(COLOR_SPACE_TRANSFORM_REGISTER0_OFS, local_reg0.reg);
	//IPE_SETREG(COLOR_SPACE_TRANSFORM_REGISTER1_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_SPACE_TRANSFORM_REGISTER2_OFS, local_reg2.reg);
}

/**
	IPE set Int Sat offset

	Set IPE color control intensity and saturation offset.

	@param int_ofs Intensity offset.
	@param sat_ofs Saturation offset.

	@return None.
*/
void ipe_set_int_sat_offset(INT16 int_ofs, INT16 sat_ofs)
{
	//T_COLOR_CONTROL_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(COLOR_CONTROL_REGISTER_OFS);
	ipeg->reg_82.bit.int_ofs = int_ofs;
	ipeg->reg_82.bit.sat_ofs = sat_ofs;

	//IPE_SETREG(COLOR_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
	IPE set hue rotation enable

	Set IPE color control hue table rotation selection, if rot_en is 1, reg_chuem[n] LSB 2 bit = 0 : 0 degree rotation, 1 : 90 degree rotation, 2 bit = 2 : 180 degree rotation, 3 : 270 degree rotation

	@return None.
*/
void ipe_set_hue_rotate(BOOL hue_rot_en)
{
	//T_COLOR_CONTROL_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(COLOR_CONTROL_REGISTER_OFS);
	ipeg->reg_82.bit.chue_roten = hue_rot_en;

	//IPE_SETREG(COLOR_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
	IPE set hue C to G enable

	Set IPE color control hue calculation
	\n 0: input is G channel, 1: input is C channel and convert it to G

	@return None.
*/
void ipe_set_hue_c2g_en(BOOL c2g_en)
{
	//T_COLOR_CONTROL_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(COLOR_CONTROL_REGISTER_OFS);
	ipeg->reg_82.bit.chue_c2gen = c2g_en;

	//IPE_SETREG(COLOR_CONTROL_REGISTER_OFS, local_reg.reg);
}

/**
	IPE set color suppress

	Set IPE color adjustment Cb Cr color suppress level.

	@param weight color suppress level.

	@return None.
*/
void ipe_set_color_sup(IPE_CCTRL_SEL_ENUM cctrl_sel, UINT8 vdetdiv)
{
	//T_COLOR_CONTROL_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(COLOR_CONTROL_REGISTER_OFS);
	ipeg->reg_82.bit.cctrl_sel = cctrl_sel;
	ipeg->reg_82.bit.vdet_div = vdetdiv;

	//IPE_SETREG(COLOR_CONTROL_REGISTER_OFS, local_reg.reg);

	ipe_enable_feature(IPE_CTRL_EN | IPE_CADJ_EN);
}

/**
	IPE set Hue table

	Set IPE color control Hue mapping.

	@param *hue24tbl pointer to hue mapping table.

	@return None.
*/
void ipe_set_cctrl_hue(UINT8 *p_hue_tbl)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	//T_COLOR_CONTROL_HUE_MAPPING_REGISTER_0 local_reg0;
	//T_COLOR_CONTROL_HUE_MAPPING_REGISTER_1 local_reg1;
	//T_COLOR_CONTROL_HUE_MAPPING_REGISTER_2 local_reg2;
	//T_COLOR_CONTROL_HUE_MAPPING_REGISTER_3 local_reg3;
	//T_COLOR_CONTROL_HUE_MAPPING_REGISTER_4 local_reg4;
	//T_COLOR_CONTROL_HUE_MAPPING_REGISTER_5 local_reg5;

	local_reg0.reg = 0;
	ipeg->reg_83.bit.chuem0 = p_hue_tbl[0];
	ipeg->reg_83.bit.chuem1 = p_hue_tbl[1];
	ipeg->reg_83.bit.chuem2 = p_hue_tbl[2];
	ipeg->reg_83.bit.chuem3 = p_hue_tbl[3];

	local_reg1.reg = 0;
	ipeg->reg_84.bit.chuem4 = p_hue_tbl[4];
	ipeg->reg_84.bit.chuem5 = p_hue_tbl[5];
	ipeg->reg_84.bit.chuem6 = p_hue_tbl[6];
	ipeg->reg_84.bit.chuem7 = p_hue_tbl[7];

	local_reg2.reg = 0;
	ipeg->reg_85.bit.chuem8 = p_hue_tbl[8];
	ipeg->reg_85.bit.chuem9 = p_hue_tbl[9];
	ipeg->reg_85.bit.chuem10 = p_hue_tbl[10];
	ipeg->reg_85.bit.chuem11 = p_hue_tbl[11];

	local_reg3.reg = 0;
	ipeg->reg_86.bit.chuem12 = p_hue_tbl[12];
	ipeg->reg_86.bit.chuem13 = p_hue_tbl[13];
	ipeg->reg_86.bit.chuem14 = p_hue_tbl[14];
	ipeg->reg_86.bit.chuem15 = p_hue_tbl[15];

	local_reg4.reg = 0;
	ipeg->reg_87.bit.chuem16 = p_hue_tbl[16];
	ipeg->reg_87.bit.chuem17 = p_hue_tbl[17];
	ipeg->reg_87.bit.chuem18 = p_hue_tbl[18];
	ipeg->reg_87.bit.chuem19 = p_hue_tbl[19];

	local_reg5.reg = 0;
	ipeg->reg_88.bit.chuem20 = p_hue_tbl[20];
	ipeg->reg_88.bit.chuem21 = p_hue_tbl[21];
	ipeg->reg_88.bit.chuem22 = p_hue_tbl[22];
	ipeg->reg_88.bit.chuem23 = p_hue_tbl[23];

	//IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	//IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_3_OFS, local_reg3.reg);
	//IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_4_OFS, local_reg4.reg);
	//IPE_SETREG(COLOR_CONTROL_HUE_MAPPING_REGISTER_5_OFS, local_reg5.reg);
#endif

	for (i = 0; i < 6; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CONTROL_HUE_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_hue_tbl[idx] & 0xFF) + ((p_hue_tbl[idx + 1] & 0xFF) << 8) + ((p_hue_tbl[idx + 2] & 0xFF) << 16) + ((p_hue_tbl[idx + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
	IPE set Int table

	Set IPE color control Intensity mapping.

	@param *intensity24tbl pointer to intensity mapping table.

	@return None.
*/
void ipe_set_cctrl_int(INT32 *p_int_tbl)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	//T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0 local_reg0;
	//T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_1 local_reg1;
	//T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_2 local_reg2;
	//T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_3 local_reg3;
	//T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_4 local_reg4;
	//T_COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_5 local_reg5;

	local_reg0.reg = 0;
	ipeg->reg_89.bit.cintm0 = p_int_tbl[0];
	ipeg->reg_89.bit.cintm1 = p_int_tbl[1];
	ipeg->reg_89.bit.cintm2 = p_int_tbl[2];
	ipeg->reg_89.bit.cintm3 = p_int_tbl[3];

	local_reg1.reg = 0;
	ipeg->reg_90.bit.cintm4 = p_int_tbl[4];
	ipeg->reg_90.bit.cintm5 = p_int_tbl[5];
	ipeg->reg_90.bit.cintm6 = p_int_tbl[6];
	ipeg->reg_90.bit.cintm7 = p_int_tbl[7];

	local_reg2.reg = 0;
	ipeg->reg_91.bit.cintm8 = p_int_tbl[8];
	ipeg->reg_91.bit.cintm9 = p_int_tbl[9];
	ipeg->reg_91.bit.cintm10 = p_int_tbl[10];
	ipeg->reg_91.bit.cintm11 = p_int_tbl[11];

	local_reg3.reg = 0;
	ipeg->reg_92.bit.cintm12 = p_int_tbl[12];
	ipeg->reg_92.bit.cintm13 = p_int_tbl[13];
	ipeg->reg_92.bit.cintm14 = p_int_tbl[14];
	ipeg->reg_92.bit.cintm15 = p_int_tbl[15];

	local_reg4.reg = 0;
	ipeg->reg_93.bit.cintm16 = p_int_tbl[16];
	ipeg->reg_93.bit.cintm17 = p_int_tbl[17];
	ipeg->reg_93.bit.cintm18 = p_int_tbl[18];
	ipeg->reg_93.bit.cintm19 = p_int_tbl[19];

	local_reg5.reg = 0;
	ipeg->reg_94.bit.cintm20 = p_int_tbl[20];
	ipeg->reg_94.bit.cintm21 = p_int_tbl[21];
	ipeg->reg_94.bit.cintm22 = p_int_tbl[22];
	ipeg->reg_94.bit.cintm23 = p_int_tbl[23];

	//IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	//IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_3_OFS, local_reg3.reg);
	//IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_4_OFS, local_reg4.reg);
	//IPE_SETREG(COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_5_OFS, local_reg5.reg);
#endif

	for (i = 0; i < 6; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CONTROL_INTENSITY_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_int_tbl[idx] & 0xFF) + ((p_int_tbl[idx + 1] & 0xFF) << 8) + ((p_int_tbl[idx + 2] & 0xFF) << 16) + ((p_int_tbl[idx + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
	IPE set Sat table

	Set IPE color control Saturation mapping.

	@param *sat24tbl pointer to saturation mapping table.

	@return None.
*/
void ipe_set_cctrl_sat(INT32 *p_sat_tbl)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	//T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0 local_reg0;
	//T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1 local_reg1;
	//T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2 local_reg2;
	//T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3 local_reg3;
	//T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4 local_reg4;
	//T_COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5 local_reg5;

	local_reg0.reg = 0;
	ipeg->reg_95.bit.csatm0 = p_sat_tbl[0];
	ipeg->reg_95.bit.csatm1 = p_sat_tbl[1];
	ipeg->reg_95.bit.csatm2 = p_sat_tbl[2];
	ipeg->reg_95.bit.csatm3 = p_sat_tbl[3];

	local_reg1.reg = 0;
	ipeg->reg_96.bit.csatm4 = p_sat_tbl[4];
	ipeg->reg_96.bit.csatm5 = p_sat_tbl[5];
	ipeg->reg_96.bit.csatm6 = p_sat_tbl[6];
	ipeg->reg_96.bit.csatm7 = p_sat_tbl[7];

	local_reg2.reg = 0;
	ipeg->reg_97.bit.csatm8 = p_sat_tbl[8];
	ipeg->reg_97.bit.csatm9 = p_sat_tbl[9];
	ipeg->reg_97.bit.csatm10 = p_sat_tbl[10];
	ipeg->reg_97.bit.csatm11 = p_sat_tbl[11];

	local_reg3.reg = 0;
	ipeg->reg_98.bit.csatm12 = p_sat_tbl[12];
	ipeg->reg_98.bit.csatm13 = p_sat_tbl[13];
	ipeg->reg_98.bit.csatm14 = p_sat_tbl[14];
	ipeg->reg_98.bit.csatm15 = p_sat_tbl[15];

	local_reg4.reg = 0;
	ipeg->reg_99.bit.csatm16 = p_sat_tbl[16];
	ipeg->reg_99.bit.csatm17 = p_sat_tbl[17];
	ipeg->reg_99.bit.csatm18 = p_sat_tbl[18];
	ipeg->reg_99.bit.csatm19 = p_sat_tbl[19];

	local_reg5.reg = 0;
	ipeg->reg_100.bit.csatm20 = p_sat_tbl[20];
	ipeg->reg_100.bit.csatm21 = p_sat_tbl[21];
	ipeg->reg_100.bit.csatm22 = p_sat_tbl[22];
	ipeg->reg_100.bit.csatm23 = p_sat_tbl[23];

	//IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	//IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_3_OFS, local_reg3.reg);
	//IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_4_OFS, local_reg4.reg);
	//IPE_SETREG(COLOR_CONTROL_SATURATION_MAPPING_REGISTER_5_OFS, local_reg5.reg);
#endif

	for (i = 0; i < 6; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CONTROL_SATURATION_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_sat_tbl[idx] & 0xFF) + ((p_sat_tbl[idx + 1] & 0xFF) << 8) + ((p_sat_tbl[idx + 2] & 0xFF) << 16) + ((p_sat_tbl[(i << 2) + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
	IPE set Edge table

	Set IPE color control Edge mapping.

	@param *edg24tbl pointer to edge mapping table.

	@return None.
*/
void ipe_set_cctrl_edge(UINT8 *p_edge_tbl)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

#if 0
	//T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_0 local_reg0;
	//T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_1 local_reg1;
	//T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_2 local_reg2;
	//T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_3 local_reg3;
	//T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_4 local_reg4;
	//T_COLOR_CONTROL_EDGE_MAPPING_REGISTER_5 local_reg5;

	local_reg0.reg = 0;
	ipeg->reg_101.bit.cedgem0 = p_edge_tbl[0];
	ipeg->reg_101.bit.cedgem1 = p_edge_tbl[1];
	ipeg->reg_101.bit.cedgem2 = p_edge_tbl[2];
	ipeg->reg_101.bit.cedgem3 = p_edge_tbl[3];

	local_reg1.reg = 0;
	ipeg->reg_102.bit.cedgem4 = p_edge_tbl[4];
	ipeg->reg_102.bit.cedgem5 = p_edge_tbl[5];
	ipeg->reg_102.bit.cedgem6 = p_edge_tbl[6];
	ipeg->reg_102.bit.cedgem7 = p_edge_tbl[7];

	local_reg2.reg = 0;
	ipeg->reg_103.bit.cedgem8 = p_edge_tbl[8];
	ipeg->reg_103.bit.cedgem9 = p_edge_tbl[9];
	ipeg->reg_103.bit.cedgem10 = p_edge_tbl[10];
	ipeg->reg_103.bit.cedgem11 = p_edge_tbl[11];

	local_reg3.reg = 0;
	ipeg->reg_104.bit.cedgem12 = p_edge_tbl[12];
	ipeg->reg_104.bit.cedgem13 = p_edge_tbl[13];
	ipeg->reg_104.bit.cedgem14 = p_edge_tbl[14];
	ipeg->reg_104.bit.cedgem15 = p_edge_tbl[15];

	local_reg4.reg = 0;
	ipeg->reg_105.bit.cedgem16 = p_edge_tbl[16];
	ipeg->reg_105.bit.cedgem17 = p_edge_tbl[17];
	ipeg->reg_105.bit.cedgem18 = p_edge_tbl[18];
	ipeg->reg_105.bit.cedgem19 = p_edge_tbl[19];

	local_reg5.reg = 0;
	ipeg->reg_106.bit.cedgem20 = p_edge_tbl[20];
	ipeg->reg_106.bit.cedgem21 = p_edge_tbl[21];
	ipeg->reg_106.bit.cedgem22 = p_edge_tbl[22];
	ipeg->reg_106.bit.cedgem23 = p_edge_tbl[23];

	//IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_2_OFS, local_reg2.reg);
	//IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_3_OFS, local_reg3.reg);
	//IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_4_OFS, local_reg4.reg);
	//IPE_SETREG(COLOR_CONTROL_EDGE_MAPPING_REGISTER_5_OFS, local_reg5.reg);
#endif

	for (i = 0; i < 6; i++) {
		//reg_set_val = 0;

		reg_ofs = (COLOR_CONTROL_EDGE_MAPPING_REGISTER_0_OFS) + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_edge_tbl[idx] & 0xFF) + ((p_edge_tbl[idx + 1] & 0xFF) << 8) + ((p_edge_tbl[idx + 2] & 0xFF) << 16) + ((p_edge_tbl[idx + 3] & 0xFF) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}
}

/**
	IPE set DDS table

	Set IPE color control DDS table.

	@param *p_dds_tbl pointer to DDS table.

	@return None.
*/
void ipe_set_cctrl_dds(UINT8 *p_dds_tbl)
{
	//T_COLOR_CONTROL_DDS_REGISTER_0 local_reg0;
	//T_COLOR_CONTROL_DDS_REGISTER_1 local_reg1;

	ipeg->reg_107.word = 0;
	ipeg->reg_107.bit.dds0 = p_dds_tbl[0];
	ipeg->reg_107.bit.dds1 = p_dds_tbl[1];
	ipeg->reg_107.bit.dds2 = p_dds_tbl[2];
	ipeg->reg_107.bit.dds3 = p_dds_tbl[3];

	ipeg->reg_108.word = 0;
	ipeg->reg_108.bit.dds4 = p_dds_tbl[4];
	ipeg->reg_108.bit.dds5 = p_dds_tbl[5];
	ipeg->reg_108.bit.dds6 = p_dds_tbl[6];
	ipeg->reg_108.bit.dds7 = p_dds_tbl[7];

	//IPE_SETREG(COLOR_CONTROL_DDS_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(COLOR_CONTROL_DDS_REGISTER_1_OFS, local_reg1.reg);
}

/**
	IPE set edge enhance

	Set IPE positive and negative edge enhance.

	@param yenhp positive edge enhance level.
	@param yenhn negative edge enhance level.

	@return None.
*/
void ipe_set_edge_enhance(UINT32 y_enh_p, UINT32 y_enh_n)
{
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0_OFS);
	ipeg->reg_110.bit.y_enh_p = y_enh_p;
	ipeg->reg_110.bit.y_enh_n = y_enh_n;

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0_OFS, local_reg.reg);
}





extern UINT16 ipe_int_to_2comp(INT16 val, UINT16 bits);
extern INT16 ipe_2comp_to_int(UINT16 val, UINT16 bits);


void ipe_set_edge_region_strength_enable(BOOL set_en)
{
    ipeg->reg_56.bit.region_str_en = set_en;
}

void ipe_set_edge_region_kernel_strength(UINT8 set_thin_str, UINT8 set_robust_str)
{
    ipeg->reg_56.bit.enh_thin = set_thin_str;
    ipeg->reg_56.bit.enh_robust = set_robust_str;
}

void ipe_et_edge_region_flat_strength(INT16 set_slop, UINT8 set_str)
{
    UINT16 get_slope_flat = ipe_int_to_2comp(set_slop, 16);
    
    ipeg->reg_57.bit.slope_flat = get_slope_flat;
    ipeg->reg_58.bit.str_flat = set_str;
}

void ipe_et_edge_region_edge_strength(INT16 set_slop, UINT8 set_str)
{
    UINT16 get_slope_edge = ipe_int_to_2comp(set_slop, 16);
    
    ipeg->reg_57.bit.slope_edge = get_slope_edge;
    ipeg->reg_58.bit.str_edge = set_str;
}


/**
	IPE set edge invert

	Set IPE positive and negative edge invert.

	@param einvp positive edge sign invert selection.
	@param einvn negative edge sign invert selection.

	@return None.
*/
void ipe_set_edge_inv(BOOL b_einv_p, BOOL b_einv_n)
{
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0_OFS);
	ipeg->reg_110.bit.y_einv_p = b_einv_p;
	ipeg->reg_110.bit.y_einv_n = b_einv_n;

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_0_OFS, local_reg.reg);
}

/**
	IPE set YCbCr contrast

	IPE color adjustment contrast setting.

	@param YCon Y contrast parameter 0~255.

	@return None.
*/
void ipe_set_ycon(UINT32 con_y)
{
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_1 local_reg1;

	//local_reg1.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS);
	ipeg->reg_111.bit.y_con = con_y;

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS, local_reg1.reg);
}


/**
	IPE set YCbCr random

	IPE color adjustment YCbCr random noise level setting.

	@param p_ycrand YCbCr random infor.

	@return None.
*/
void ipe_set_yc_rand(IPE_CADJ_RAND_PARAM *p_ycrand)
{
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_1 local_reg1;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_5 local_reg2;

	//local_reg1.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS);
	ipeg->reg_111.bit.y_rand_en = p_ycrand->yrand_en;
	ipeg->reg_111.bit.y_rand = p_ycrand->yrand_level;
	ipeg->reg_111.bit.yc_randreset = p_ycrand->yc_rand_rst;

	//local_reg2.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS);
	ipeg->reg_115.bit.c_rand_en = p_ycrand->crand_en;
	ipeg->reg_115.bit.c_rand = p_ycrand->crand_level;

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS, local_reg2.reg);
}

/**
	IPE set edge threshold for YCbCr fix replace

	Set IPE edge abs threshold for fixed YCbCr replacement.

	@param ethy Edge abs threshold for Y component replacement.
	@param ethc Edge abs threshold for CbCr component replacement.

	@return None.
*/
void ipe_set_yfix_th(STR_YTH1_INFOR *p_yth1, STR_YTH2_INFOR *p_yth2)
{
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_1 local_reg1;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_2 local_reg2;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_3 local_reg3;

	//local_reg1.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS);
	ipeg->reg_111.bit.y_ethy = p_yth1->edgeth;

	//local_reg2.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_2_OFS);
	ipeg->reg_112.bit.y_yth1 = p_yth1->y_th;
	ipeg->reg_112.bit.y_hit1sel = p_yth1->hit_sel;
	ipeg->reg_112.bit.y_nhit1sel = p_yth1->nhit_sel;
	ipeg->reg_112.bit.y_fix1_hit = p_yth1->hit_value;
	ipeg->reg_112.bit.y_fix1_nhit = p_yth1->nhit_value;

	//local_reg3.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_3_OFS);
	ipeg->reg_113.bit.y_yth2 = p_yth2->y_th;
	ipeg->reg_113.bit.y_hit2sel = p_yth2->hit_sel;
	ipeg->reg_113.bit.y_nhit2sel = p_yth2->nhit_sel;
	ipeg->reg_113.bit.y_fix2_hit = p_yth2->hit_value;
	ipeg->reg_113.bit.y_fix2_nhit = p_yth2->nhit_value;

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_2_OFS, local_reg2.reg);
	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_3_OFS, local_reg3.reg);
}

void ipe_set_yc_mask(UINT8 mask_y, UINT8 mask_cb, UINT8 mask_cr)
{
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_4 local_reg;

	//local_reg.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_4_OFS);
	ipeg->reg_114.bit.ymask = mask_y;
	ipeg->reg_114.bit.cbmask = mask_cb;
	ipeg->reg_114.bit.crmask = mask_cr;

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_4_OFS, local_reg.reg);
}

/**
	IPE set CbCr offset

	IPE color adjustment CbCr offset setting.

	@param cbofs Cb component offset.
	@param crofs Cr component offset.

	@return None.
*/
void ipe_set_cbcr_offset(UINT32 cb_ofs, UINT32 cr_ofs)
{
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_5 local_reg;

	//local_reg.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS);
	ipeg->reg_115.bit.c_cbofs = cb_ofs;
	ipeg->reg_115.bit.c_crofs = cr_ofs;

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS, local_reg.reg);
}

void ipe_set_cbcr_con(UINT32 con_c)
{
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_5 local_reg;

	//local_reg.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS);
	ipeg->reg_115.bit.c_con = con_c;

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_5_OFS, local_reg.reg);
}


void ipe_set_cbcr_contab_sel(IPE_CCONTAB_SEL ccontab_sel)
{
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_14 local_reg5;

	//local_reg5.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS);
	ipeg->reg_124.bit.ccontab_sel = ccontab_sel;

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS, local_reg5.reg);
}

void ipe_set_cbcr_contab(UINT32 *p_ccontab)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_9  local_reg0;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_10 local_reg1;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_11 local_reg2;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_12 local_reg3;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_13 local_reg4;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_14 local_reg5;

#if 0
	//local_reg0.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_9_OFS);
	ipeg->reg_119.bit.ccontab0 = p_ccontab[0];
	ipeg->reg_119.bit.ccontab1 = p_ccontab[1];
	ipeg->reg_119.bit.ccontab2 = p_ccontab[2];

	//local_reg1.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_10_OFS);
	ipeg->reg_120.bit.ccontab3 = p_ccontab[3];
	ipeg->reg_120.bit.ccontab4 = p_ccontab[4];
	ipeg->reg_120.bit.ccontab5 = p_ccontab[5];

	//local_reg2.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_11_OFS);
	ipeg->reg_121.bit.ccontab6 = p_ccontab[6];
	ipeg->reg_121.bit.ccontab7 = p_ccontab[7];
	ipeg->reg_121.bit.ccontab8 = p_ccontab[8];

	//local_reg3.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_12_OFS);
	ipeg->reg_122.bit.ccontab9 = p_ccontab[9];
	ipeg->reg_122.bit.ccontab10 = p_ccontab[10];
	ipeg->reg_122.bit.ccontab11 = p_ccontab[11];

	//local_reg4.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_13_OFS);
	ipeg->reg_123.bit.ccontab12 = p_ccontab[12];
	ipeg->reg_123.bit.ccontab13 = p_ccontab[13];
	ipeg->reg_123.bit.ccontab14 = p_ccontab[14];

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_9_OFS, local_reg0.reg);
	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_10_OFS, local_reg1.reg);
	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_11_OFS, local_reg2.reg);
	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_12_OFS, local_reg3.reg);
	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_13_OFS, local_reg4.reg);
#endif

	for (i = 0; i < 5; i++) {
		//reg_set_val = 0;

		reg_ofs = COLOR_COMPONENT_ADJUSTMENT_REGISTER_9_OFS + (i << 2);

		idx = (i * 3);
		reg_set_val = (p_ccontab[idx]) + ((p_ccontab[idx + 1]) << 10) + ((p_ccontab[idx + 2]) << 20);

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	//local_reg5.reg = IPE_GETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS);
	ipeg->reg_124.bit.ccontab15 = p_ccontab[15];
	ipeg->reg_124.bit.ccontab16 = p_ccontab[16];
	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_14_OFS, local_reg5.reg);
}

/**
	IPE set YCbCr threshold for CbCr fix replace

	Set IPE YCbCr threshold for fixed CbCr replacement.

	@param yth Y high low threshold for CbCr replacement.
	@param cbth Cb high low threshold for CbCr replacement.
	@param crth Cr high low threshold for CbCr replacement.

	@return None.
*/
void ipe_set_cbcr_fixth(STR_CTH_INFOR *p_cth)
{
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_6 local_reg6;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_7 local_reg7;
	//T_COLOR_COMPONENT_ADJUSTMENT_REGISTER_8 local_reg8;

	ipeg->reg_116.word = 0;
	ipeg->reg_116.bit.cb_fix_hit = p_cth->cb_hit_value;
	ipeg->reg_116.bit.cb_fix_nhit = p_cth->cb_nhit_value;
	ipeg->reg_116.bit.cr_fix_hit = p_cth->cr_hit_value;
	ipeg->reg_116.bit.cr_fix_nhit = p_cth->cr_nhit_value;

	ipeg->reg_117.word = 0;
	ipeg->reg_117.bit.c_eth = p_cth->edgeth;
	ipeg->reg_117.bit.c_hitsel = p_cth->hit_sel;
	ipeg->reg_117.bit.c_nhitsel = p_cth->nhit_sel;
	ipeg->reg_117.bit.c_yth_h = p_cth->yth_high;
	ipeg->reg_117.bit.c_yth_l = p_cth->yth_low;

	ipeg->reg_118.word = 0;
	ipeg->reg_118.bit.c_cbth_h = p_cth->cbth_high;
	ipeg->reg_118.bit.c_cbth_l = p_cth->cbth_low;
	ipeg->reg_118.bit.c_crth_h = p_cth->crth_high;
	ipeg->reg_118.bit.c_crth_l = p_cth->crth_low;

	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_6_OFS, local_reg6.reg);
	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_7_OFS, local_reg7.reg);
	//IPE_SETREG(COLOR_COMPONENT_ADJUSTMENT_REGISTER_8_OFS, local_reg8.reg);
}

void ipe_set_cstp(UINT32 ratio)
{
	//T_COLOR_SPACE_TRANSFORM_REGISTER0 local_reg;

	//local_reg.reg = IPE_GETREG(COLOR_SPACE_TRANSFORM_REGISTER0_OFS);
	ipeg->reg_78.bit.cstp_rat = ratio;

	//IPE_SETREG(COLOR_SPACE_TRANSFORM_REGISTER0_OFS, local_reg.reg);
}

void ipe_set_ycurve_sel(IPE_YCURVE_SEL ycurve_sel)
{
	//T_YCURVE_REFISTER local_reg;

	//local_reg.reg = IPE_GETREG(YCURVE_REFISTER_OFS);
	ipeg->reg_127.bit.ycurve_sel = ycurve_sel;

	//IPE_SETREG(YCURVE_REFISTER_OFS, local_reg.reg);
}

void ipe_set_gam_y_rand(IPE_GAMYRAND *p_gamy_rand)
{
	//T_GAMMA_YCURVE_REGISTER local_reg0;

	//local_reg0.reg = IPE_GETREG(GAMMA_YCURVE_REGISTER_OFS);
	ipeg->reg_130.bit.gamyrand_en = p_gamy_rand->gam_y_rand_en;
	ipeg->reg_130.bit.gamyrand_reset = p_gamy_rand->gam_y_rand_rst;
	ipeg->reg_130.bit.gamyrand_shf = p_gamy_rand->gam_y_rand_shft;

	//IPE_SETREG(GAMMA_YCURVE_REGISTER_OFS, local_reg0.reg);
}

void ipe_set_defog_rand(IPE_DEFOGROUND *p_defog_rnd)
{
	//T_GAMMA_YCURVE_REGISTER local_reg0;

	//local_reg0.reg = IPE_GETREG(GAMMA_YCURVE_REGISTER_OFS);
	ipeg->reg_130.bit.defogrnd_opt = p_defog_rnd->rand_opt;
	ipeg->reg_130.bit.defogrnd_rst = p_defog_rnd->rand_rst;

	//IPE_SETREG(GAMMA_YCURVE_REGISTER_OFS, local_reg0.reg);
}

void ipe_set_edge_dbg_sel(IPE_EDGEDBG_SEL mode_sel)
{
	//T_EDGE_DEBUG_REGISTER local_reg0;

	//local_reg0.reg = IPE_GETREG(EDGE_DEBUG_REGISTER_OFS);
	ipeg->reg_126.bit.edge_dbg_modesel = mode_sel;

	//IPE_SETREG(EDGE_DEBUG_REGISTER_OFS, local_reg0.reg);
}

ER ipe_get_burst_length(IPE_BURST_LENGTH *p_burstlen)
{
	//T_IPE_MODE_REGISTER_0 local_reg0;

	//local_reg0.reg = IPE_GETREG(IPE_MODE_REGISTER_0_OFS);
	p_burstlen->burstlen_in_y = (IPE_BURST_SEL)ipeg->reg_1.bit.iny_burst_mode;
	p_burstlen->burstlen_in_c = (IPE_BURST_SEL)ipeg->reg_1.bit.inc_burst_mode;
	p_burstlen->burstlen_out_y = (IPE_BURST_SEL)ipeg->reg_1.bit.outy_burst_mode;
	p_burstlen->burstlen_out_c = (IPE_BURST_SEL)ipeg->reg_1.bit.outc_burst_mode;

	return E_OK;
}

void ipe_set_eth_size(IPE_ETH_SIZE *p_eth)
{
	//T_EDGE_THRESHOLD_REGISTER_0 local_reg0;
	//T_EDGE_THRESHOLD_REGISTER_1 local_reg1;

	//local_reg0.reg = IPE_GETREG(EDGE_THRESHOLD_REGISTER_0_OFS);
	//local_reg1.reg = IPE_GETREG(EDGE_THRESHOLD_REGISTER_1_OFS);

	ipeg->reg_133.bit.eth_outsel = p_eth->eth_outsel;
	ipeg->reg_132.bit.eth_outfmt = p_eth->eth_outfmt;

	//IPE_SETREG(EDGE_THRESHOLD_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(EDGE_THRESHOLD_REGISTER_1_OFS, local_reg1.reg);
}

void ipe_set_eth_param(IPE_ETH_PARAM *p_eth)
{
	//T_EDGE_THRESHOLD_REGISTER_0 local_reg0;
	//T_EDGE_THRESHOLD_REGISTER_1 local_reg1;

	//local_reg0.reg = IPE_GETREG(EDGE_THRESHOLD_REGISTER_0_OFS);
	//local_reg1.reg = IPE_GETREG(EDGE_THRESHOLD_REGISTER_1_OFS);

	ipeg->reg_132.bit.ethlow = p_eth->eth_low;
	ipeg->reg_132.bit.ethmid = p_eth->eth_mid;
	ipeg->reg_132.bit.ethhigh = p_eth->eth_high;

	ipeg->reg_133.bit.hout_sel = p_eth->eth_hout;
	ipeg->reg_133.bit.vout_sel = p_eth->eth_vout;

	//IPE_SETREG(EDGE_THRESHOLD_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(EDGE_THRESHOLD_REGISTER_1_OFS, local_reg1.reg);
}

void ipe_set_dma_out_va_addr(UINT32 sao)
{
	//T_IPE_TO_DMA_VA_CHANNEL_REGISTER_0 local_reg;

	if ((sao == 0)) {
		DBG_ERR("IMG Input addresses cannot be 0x0!\r\n");
		return;
	}

	//local_reg.reg = IPE_GETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_0_OFS);
	ipeg->reg_18.bit.dram_sao_va = sao >> 2;

	//IPE_SETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_0_OFS, local_reg.reg);
}

void ipe_set_dma_out_va_offset(UINT32 ofso)
{
	//T_IPE_TO_DMA_VA_CHANNEL_REGISTER_1 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_1_OFS);
	ipeg->reg_19.bit.dram_ofso_va = ofso >> 2;

	//IPE_SETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_1_OFS, local_reg.reg);
}


void ipe_set_va_out_sel(BOOL en)
{
	//T_VARIATION_ACCUMULATION_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS);
	ipeg->reg_139.bit.vacc_outsel = en;

	//IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS, local_reg.reg);
}

void ipe_set_va_filter_g1(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g1)
{
	//T_VARIATION_DETECTION_GROUP1_REGISTER_0 local_reg0;
	//T_VARIATION_DETECTION_GROUP1_REGISTER_1 local_reg1;
	//T_VARIATION_ACCUMULATION_REGISTER_1 local_reg2;

	//local_reg0.reg = IPE_GETREG(VARIATION_DETECTION_GROUP1_REGISTER_0_OFS);
	//local_reg1.reg = IPE_GETREG(VARIATION_DETECTION_GROUP1_REGISTER_1_OFS);
	//local_reg2.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_1_OFS);

	ipeg->reg_135.bit.vdet_gh1_a = p_va_fltr_g1->filt_h.tap_a;
	ipeg->reg_135.bit.vdet_gh1_b = p_va_fltr_g1->filt_h.tap_b;
	ipeg->reg_135.bit.vdet_gh1_c = p_va_fltr_g1->filt_h.tap_c;
	ipeg->reg_135.bit.vdet_gh1_d = p_va_fltr_g1->filt_h.tap_d;
	ipeg->reg_135.bit.vdet_gh1_bcd_op = p_va_fltr_g1->filt_h.filt_symm;
	ipeg->reg_135.bit.vdet_gh1_fsize = p_va_fltr_g1->filt_h.fltr_size;
	ipeg->reg_135.bit.vdet_gh1_div = p_va_fltr_g1->filt_h.div;

	ipeg->reg_136.bit.vdet_gv1_a = p_va_fltr_g1->filt_v.tap_a;
	ipeg->reg_136.bit.vdet_gv1_b = p_va_fltr_g1->filt_v.tap_b;
	ipeg->reg_136.bit.vdet_gv1_c = p_va_fltr_g1->filt_v.tap_c;
	ipeg->reg_136.bit.vdet_gv1_d = p_va_fltr_g1->filt_v.tap_d;
	ipeg->reg_136.bit.vdet_gv1_bcd_op = p_va_fltr_g1->filt_v.filt_symm;
	ipeg->reg_136.bit.vdet_gv1_fsize = p_va_fltr_g1->filt_v.fltr_size;
	ipeg->reg_136.bit.vdet_gv1_div = p_va_fltr_g1->filt_v.div;

	ipeg->reg_140.bit.va_g1h_thl = p_va_fltr_g1->filt_h.th_low;
	ipeg->reg_140.bit.va_g1h_thh = p_va_fltr_g1->filt_h.th_high;
	ipeg->reg_140.bit.va_g1v_thl = p_va_fltr_g1->filt_v.th_low;
	ipeg->reg_140.bit.va_g1v_thh = p_va_fltr_g1->filt_v.th_high;

	//IPE_SETREG(VARIATION_DETECTION_GROUP1_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(VARIATION_DETECTION_GROUP1_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_1_OFS, local_reg2.reg);
}

void ipe_set_va_filter_g2(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g2)
{
	//T_VARIATION_DETECTION_GROUP2_REGISTER_0 local_reg0;
	//T_VARIATION_DETECTION_GROUP2_REGISTER_1 local_reg1;
	//T_VARIATION_ACCUMULATION_REGISTER_2 local_reg2;

	//local_reg0.reg = IPE_GETREG(VARIATION_DETECTION_GROUP2_REGISTER_0_OFS);
	//local_reg1.reg = IPE_GETREG(VARIATION_DETECTION_GROUP2_REGISTER_1_OFS);
	//local_reg2.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_2_OFS);

	ipeg->reg_137.bit.vdet_gh2_a = p_va_fltr_g2->filt_h.tap_a;
	ipeg->reg_137.bit.vdet_gh2_b = p_va_fltr_g2->filt_h.tap_b;
	ipeg->reg_137.bit.vdet_gh2_c = p_va_fltr_g2->filt_h.tap_c;
	ipeg->reg_137.bit.vdet_gh2_d = p_va_fltr_g2->filt_h.tap_d;
	ipeg->reg_137.bit.vdet_gh2_bcd_op = p_va_fltr_g2->filt_h.filt_symm;
	ipeg->reg_137.bit.vdet_gh2_fsize = p_va_fltr_g2->filt_h.fltr_size;
	ipeg->reg_137.bit.vdet_gh2_div = p_va_fltr_g2->filt_h.div;

	ipeg->reg_138.bit.vdet_gv2_a = p_va_fltr_g2->filt_v.tap_a;
	ipeg->reg_138.bit.vdet_gv2_b = p_va_fltr_g2->filt_v.tap_b;
	ipeg->reg_138.bit.vdet_gv2_c = p_va_fltr_g2->filt_v.tap_c;
	ipeg->reg_138.bit.vdet_gv2_d = p_va_fltr_g2->filt_v.tap_d;
	ipeg->reg_138.bit.vdet_gv2_bcd_op = p_va_fltr_g2->filt_v.filt_symm;
	ipeg->reg_138.bit.vdet_gv2_fsize = p_va_fltr_g2->filt_v.fltr_size;
	ipeg->reg_138.bit.vdet_gv2_div = p_va_fltr_g2->filt_v.div;

	ipeg->reg_141.bit.va_g2h_thl = p_va_fltr_g2->filt_h.th_low;
	ipeg->reg_141.bit.va_g2h_thh = p_va_fltr_g2->filt_h.th_high;
	ipeg->reg_141.bit.va_g2v_thl = p_va_fltr_g2->filt_v.th_low;
	ipeg->reg_141.bit.va_g2v_thh = p_va_fltr_g2->filt_v.th_high;

	//IPE_SETREG(VARIATION_DETECTION_GROUP2_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(VARIATION_DETECTION_GROUP2_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_2_OFS, local_reg2.reg);
}

void ipe_set_va_mode_enable(IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g1, IPE_VA_FLTR_GROUP_PARAM *p_va_fltr_g2)
{
	//T_VARIATION_ACCUMULATION_REGISTER_3 local_reg0;

	//local_reg0.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_3_OFS);

	if (p_va_fltr_g1 != NULL) {
		ipeg->reg_142.bit.va_g1_line_max_mode = p_va_fltr_g1->linemax_en;
		ipeg->reg_142.bit.va_g1_cnt_en = p_va_fltr_g1->cnt_en;
	}
	if (p_va_fltr_g2 != NULL) {
		ipeg->reg_142.bit.va_g2_line_max_mode = p_va_fltr_g2->linemax_en;
		ipeg->reg_142.bit.va_g2_cnt_en = p_va_fltr_g2->cnt_en;
	}

	//IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_3_OFS, local_reg0.reg);
}


void ipe_set_va_win_info(IPE_VA_WIN_PARAM *p_va_win)
{

	//T_VARIATION_ACCUMULATION_REGISTER_0 local_reg0;
	//T_VARIATION_ACCUMULATION_REGISTER_3 local_reg1;
	//T_VARIATION_ACCUMULATION_REGISTER_4 local_reg2;

	va_ring_setting[0].win_num_x = p_va_win->win_numx;
	va_ring_setting[0].win_num_y = p_va_win->win_numy;

	//local_reg0.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS);
	//local_reg1.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_3_OFS);
	//local_reg2.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_4_OFS);

	ipeg->reg_139.bit.va_stx = p_va_win->win_stx;
	ipeg->reg_139.bit.va_sty = p_va_win->win_sty;

	ipeg->reg_142.bit.va_win_szx = p_va_win->win_szx;
	ipeg->reg_142.bit.va_win_szy = p_va_win->win_szy;

	ipeg->reg_143.bit.va_win_numx = p_va_win->win_numx - 1;
	ipeg->reg_143.bit.va_win_numy = p_va_win->win_numy - 1;
	ipeg->reg_143.bit.va_win_skipx = p_va_win->win_spx;
	ipeg->reg_143.bit.va_win_skipy = p_va_win->win_spy;

	//IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_3_OFS, local_reg1.reg);
	//IPE_SETREG(VARIATION_ACCUMULATION_REGISTER_4_OFS, local_reg2.reg);
}

void ipe_set_va_indep_win(IPE_INDEP_VA_PARAM  *p_indep_va_win_info, UINT32 win_idx)
{
	UINT32 addr_ofs;
	T_INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0 local_reg1;
	T_INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1 local_reg2;

	if (win_idx < 5) {
		addr_ofs = win_idx * 8;
	} else {
		return;
	}
	local_reg1.reg = IPE_GETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0_OFS + addr_ofs)); //no need//
	local_reg2.reg = IPE_GETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1_OFS + addr_ofs)); //no need//

	local_reg1.bit.reg_win0_stx	  = p_indep_va_win_info->win_stx;
	local_reg1.bit.reg_win0_sty	  = p_indep_va_win_info->win_sty;

	local_reg2.bit.reg_win0_g1_line_max_mode    = p_indep_va_win_info->linemax_g1_en;
	local_reg2.bit.reg_win0_g2_line_max_mode    = p_indep_va_win_info->linemax_g2_en;
	local_reg2.bit.reg_win0_hsz	  = p_indep_va_win_info->win_szx;
	local_reg2.bit.reg_win0_vsz	  = p_indep_va_win_info->win_szy;

	IPE_SETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0_OFS + addr_ofs), local_reg1.reg);
	IPE_SETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1_OFS + addr_ofs), local_reg2.reg);
}

UINT32 ipe_get_va_out_addr(void)
{
	//T_IPE_TO_DMA_VA_CHANNEL_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_0_OFS);
	return ipeg->reg_18.bit.dram_sao_va << 2;
}

UINT32 ipe_get_va_out_lofs(void)
{
	//T_IPE_TO_DMA_VA_CHANNEL_REGISTER_1 local_reg;

	//local_reg.reg = IPE_GETREG(IPE_TO_DMA_VA_CHANNEL_REGISTER_1_OFS);
	return ipeg->reg_19.bit.dram_ofso_va << 2;
}

BOOL ipe_get_va_outsel(void)
{
	//T_VARIATION_ACCUMULATION_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS);
	return ipeg->reg_139.bit.vacc_outsel;
}

void ipe_get_win_info(IPE_VA_WIN_PARAM *p_va_win)
{
	//T_VARIATION_ACCUMULATION_REGISTER_0 local_reg0;
	//T_VARIATION_ACCUMULATION_REGISTER_3 local_reg1;
	//T_VARIATION_ACCUMULATION_REGISTER_4 local_reg2;

	//local_reg0.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_0_OFS);
	//local_reg1.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_3_OFS);
	//local_reg2.reg = IPE_GETREG(VARIATION_ACCUMULATION_REGISTER_4_OFS);

	p_va_win->win_stx = ipeg->reg_139.bit.va_stx;
	p_va_win->win_sty = ipeg->reg_139.bit.va_sty;
	p_va_win->win_szx = ipeg->reg_142.bit.va_win_szx;
	p_va_win->win_szy = ipeg->reg_142.bit.va_win_szy;

	p_va_win->win_numx = ipeg->reg_143.bit.va_win_numx + 1;
	p_va_win->win_numy = ipeg->reg_143.bit.va_win_numy + 1;
	p_va_win->win_spx = ipeg->reg_143.bit.va_win_skipx;
	p_va_win->win_spy = ipeg->reg_143.bit.va_win_skipy;
}

void ipe_get_va_rslt(UINT32 *p_g1_h, UINT32 *p_g1_v, UINT32 *p_g1_hcnt, UINT32 *p_g1_vcnt, UINT32 *p_g2_h, UINT32 *p_g2_v, UINT32 *p_g2_hcnt, UINT32 *p_g2_vcnt)
{
	IPE_VA_WIN_PARAM p_va_win = {0};
	UINT32 buff_addr, buff_lofs;
#ifdef __KERNEL__
	buff_addr = g_ipe_dramio[IPE_OUT_VA].addr; //virtual addr
#else
	buff_addr = dma_getNonCacheAddr(ipe_get_va_out_addr());
#endif

	if (buff_addr == 0) {
		DBG_DUMP("addr should not be 0 \r\n");
		return;
	}

	buff_lofs = ipe_get_va_out_lofs();
	ipe_get_win_info(&p_va_win);
	ipe_get_va_rslt_manual(p_g1_h, p_g1_v, p_g1_hcnt, p_g1_vcnt, p_g2_h, p_g2_v, p_g2_hcnt, p_g2_vcnt, &p_va_win, buff_addr, buff_lofs);
}

void ipe_get_va_rslt_manual(UINT32 *p_g1_h, UINT32 *p_g1_v, UINT32 *p_g1_hcnt, UINT32 *p_g1_vcnt, UINT32 *p_g2_h, UINT32 *p_g2_v, UINT32 *p_g2_hcnt, UINT32 *p_g2_vcnt, IPE_VA_WIN_PARAM *p_va_win, UINT32 address, UINT32 lineoffset)
{
	UINT32 i, j, id, buf;
	//UINT32 uiWinNum;
	UINT32 va_h, va_v, va_cnt_h, va_cnt_v;

	//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "addr =  0x%08x, lineoffset =  0x%08x\r\n", (unsigned int)address, (unsigned int)lineoffset);
	//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "winNumx=%d,winNumy=%d,\r\n", (unsigned int)p_va_win->win_numx, (unsigned int)p_va_win->win_numy);
	//uiWinNum = p_va_win->win_numx * p_va_win->win_numy;
	if (ipe_get_va_outsel() == VA_OUT_GROUP1) {
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "VA_OUT_GROUP1 \r\n");
		for (j = 0; j < p_va_win->win_numy; j++) {
			for (i = 0; i < p_va_win->win_numx; i++) {
				// 2 word per window
				id = j * p_va_win->win_numx + i;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 3));
				va_h	= ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g1_h + id)	   = va_h;
				va_cnt_h = ((buf >> 16) & 0xffff);
				*(p_g1_hcnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 3) + 4);
				va_v	= ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g1_v + id)	   = va_v;
				va_cnt_v = ((buf >> 16) & 0xffff);
				*(p_g1_vcnt + id)  = va_cnt_v;
			}
		}
	} else { // ipe_get_va_outsel() == VA_OUT_BOTH
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "VA_OUT_BOTH \r\n");
		for (j = 0; j < p_va_win->win_numy; j++) {
			for (i = 0; i < p_va_win->win_numx; i++) {
				// 4 word per window
				id = j * p_va_win->win_numx + i;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4));
				va_h	= ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g1_h + id)	   = va_h;
				va_cnt_h = ((buf >> 16) & 0xffff);
				*(p_g1_hcnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 4);
				va_v	= ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g1_v + id)	   = va_v;
				va_cnt_v = ((buf >> 16) & 0xffff);
				*(p_g1_vcnt + id)  = va_cnt_v;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 8);
				va_h	= ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g2_h + id)	   = va_h;
				va_cnt_h = ((buf >> 16) & 0xffff);
				*(p_g2_hcnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 12);
				va_v	= ((buf >> 4) & 0xfff) << ((buf) & 0xf);
				*(p_g2_v + id)	   = va_v;
				va_cnt_v = ((buf >> 16) & 0xffff);
				*(p_g2_vcnt + id)  = va_cnt_v;
			}
		}
	}

	ipe_dbg_cmd &= ~(IPE_DBG_VA);
}

void ipe_get_va_result(IPE_VA_SETTING *p_va_info, IPE_VA_RSLT *p_va_rslt)
{
	UINT32 i, j, id, buf;
	//UINT32 uiWinNum;
	UINT32 va_h, va_v, va_cnt_h, va_cnt_v;
	UINT32 address, lineoffset;

	p_va_info->win_num_x = va_ring_setting[1].win_num_x;
	p_va_info->win_num_y = va_ring_setting[1].win_num_y;
	p_va_info->outsel = va_ring_setting[1].outsel;
	p_va_info->address = va_ring_setting[1].address;
	p_va_info->lineoffset = va_ring_setting[1].lineoffset;

	address = p_va_info->address;
	lineoffset = p_va_info->lineoffset;

	if (va_ring_setting[1].va_en == 0) {
		DBG_DUMP("get va fail, va not open \r\n");
		return;
	}

	if (p_va_info->win_num_x == 0 || p_va_info->win_num_y == 0) {
		DBG_DUMP("get va fail, va not ready \r\n");
		return;
	}

	if (address == 0) {
		DBG_DUMP("get va fail, addr should not be 0 \r\n");
		return;
	}

	//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "addr =  0x%08x, lineoffset =  0x%08x\r\n", (unsigned int)address, (unsigned int)lineoffset);
	//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "winNumx=%d,winNumy=%d,\r\n", (unsigned int)p_va_info->win_num_x, (unsigned int)p_va_info->win_num_y);

	if (p_va_info->outsel == VA_OUT_GROUP1) {
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "VA_OUT_GROUP1 \r\n");
		for (j = 0; j < p_va_info->win_num_y; j++) {
			for (i = 0; i < p_va_info->win_num_x; i++) {
				// 2 word per window
				id = j * p_va_info->win_num_x + i;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 3));
				va_h	= ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g1_h + id)	  = va_h;
				va_cnt_h = ((buf >> 16) & 0xffff);
				*(p_va_rslt->p_g1_h_cnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 3) + 4);
				va_v	= ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g1_v + id)	  = va_v;
				va_cnt_v = ((buf >> 16) & 0xffff);
				*(p_va_rslt->p_g1_v_cnt + id)  = va_cnt_v;
			}
		}
	} else { // ipe_get_va_outsel() == VA_OUT_BOTH
		//PRINT_IPE((ipe_dbg_cmd & IPE_DBG_VA), "VA_OUT_BOTH \r\n");
		for (j = 0; j < p_va_info->win_num_y; j++) {
			for (i = 0; i < p_va_info->win_num_x; i++) {
				// 4 word per window
				id = j * p_va_info->win_num_x + i;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4));
				va_h	= ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g1_h + id)	  = va_h;
				va_cnt_h = (((buf >> 16) & 0xffff) << 2);
				*(p_va_rslt->p_g1_h_cnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 4);
				va_v	= ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g1_v + id)	  = va_v;
				va_cnt_v = (((buf >> 16) & 0xffff) << 2);
				*(p_va_rslt->p_g1_v_cnt + id)  = va_cnt_v;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 8);
				va_h	= ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g2_h + id)	  = va_h;
				va_cnt_h = (((buf >> 16) & 0xffff) << 2);
				*(p_va_rslt->p_g2_h_cnt + id)  = va_cnt_h;
				buf = *(volatile UINT32 *)((address + j * lineoffset) + (i << 4) + 12);
				va_v	= ((buf >> 5) & 0x7ff) << ((buf) & 0x1f);
				*(p_va_rslt->p_g2_v + id)	  = va_v;
				va_cnt_v = (((buf >> 16) & 0xffff) << 2);
				*(p_va_rslt->p_g2_v_cnt + id)  = va_cnt_v;
			}
		}
	}

	ipe_dbg_cmd &= ~(IPE_DBG_VA);

}

IPE_INDEP_VA_PARAM ipe_get_indep_win_info(UINT32 win_idx)
{
	IPE_INDEP_VA_PARAM p_indep_va_win_info = {0};
	UINT32 addr_ofs;
	UINT32 mask = 1;
	T_IPE_MODE_REGISTER_1 local_reg0;

	T_INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0 local_reg1;
	T_INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1 local_reg2;

	local_reg0.reg = IPE_GETREG(IPE_MODE_REGISTER_1_OFS);

	mask = mask << (20 + win_idx);

	if (win_idx < 5) {
		addr_ofs = win_idx * 8;
	} else {
		return p_indep_va_win_info;
	}

	local_reg1.reg = IPE_GETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_0_OFS + addr_ofs)); //no need//
	local_reg2.reg = IPE_GETREG((INDEP_VARIATION_ACCUMULATION_WINDOW_0_REGISTER_1_OFS + addr_ofs)); //no need//

	//p_indep_va_win_info.indep_va_en = local_reg0.reg.reg_win0_va_en;
	p_indep_va_win_info.indep_va_en = local_reg0.reg & (mask & 0xffffffff);
	p_indep_va_win_info.win_stx = local_reg1.bit.reg_win0_stx;
	p_indep_va_win_info.win_sty = local_reg1.bit.reg_win0_sty;

	p_indep_va_win_info.linemax_g1_en = local_reg2.bit.reg_win0_g1_line_max_mode;
	p_indep_va_win_info.linemax_g2_en = local_reg2.bit.reg_win0_g2_line_max_mode;
	p_indep_va_win_info.win_szx = local_reg2.bit.reg_win0_hsz;
	p_indep_va_win_info.win_szy = local_reg2.bit.reg_win0_vsz;

	return p_indep_va_win_info;
}


void ipe_get_indep_va_win_rslt(IPE_INDEP_VA_WIN_RSLT *p_indepva_rslt, UINT32 win_idx)
{
        UINT32 addr_ofs;
        T_INDEP_VARIATION_ACCUMULATION_REGISTER_0 local_reg0;
        T_INDEP_VARIATION_ACCUMULATION_REGISTER_1 local_reg1;
        T_INDEP_VARIATION_ACCUMULATION_REGISTER_2 local_reg2;
        T_INDEP_VARIATION_ACCUMULATION_REGISTER_3 local_reg3;
    
        if (win_idx < 5) {
            addr_ofs = win_idx * 16;
        } else {
            return;
        }
    
        local_reg0.reg = IPE_GETREG(INDEP_VARIATION_ACCUMULATION_REGISTER_0_OFS + addr_ofs);
        local_reg1.reg = IPE_GETREG(INDEP_VARIATION_ACCUMULATION_REGISTER_1_OFS + addr_ofs);
        local_reg2.reg = IPE_GETREG(INDEP_VARIATION_ACCUMULATION_REGISTER_2_OFS + addr_ofs);
        local_reg3.reg = IPE_GETREG(INDEP_VARIATION_ACCUMULATION_REGISTER_3_OFS + addr_ofs);
    
#if 1
        //va_g1_h : ([15:5] base, [4:0] exponent of 2)
        //uiVaG1HCnt : Vacnt (18bit->16bit MSB)
        p_indepva_rslt->va_g1_h    = ((local_reg0.bit.reg_va_win0g1h_vacc >> 5) & (0x7ff)) << ((local_reg0.bit.reg_va_win0g1h_vacc) & (0x1f));
        p_indepva_rslt->va_cnt_g1_h = (local_reg0.bit.reg_va_win0g1h_vacnt << 2);
        p_indepva_rslt->va_g1_v    = ((local_reg1.bit.reg_va_win0g1v_vacc >> 5) & (0x7ff)) << ((local_reg1.bit.reg_va_win0g1v_vacc) & (0x1f));
        p_indepva_rslt->va_cnt_g1_v = (local_reg1.bit.reg_va_win0g1v_vacnt << 2);
        p_indepva_rslt->va_g2_h    = ((local_reg2.bit.reg_va_win0g2h_vacc >> 5) & (0x7ff)) << ((local_reg2.bit.reg_va_win0g2h_vacc) & (0x1f));
        p_indepva_rslt->va_cnt_g2_h = (local_reg2.bit.reg_va_win0g2h_vacnt << 2);
        p_indepva_rslt->va_g2_v    = ((local_reg3.bit.reg_va_win0g2v_vacc >> 5) & (0x7ff)) << ((local_reg3.bit.reg_va_win0g2v_vacc) & (0x1f));
        p_indepva_rslt->va_cnt_g2_v = (local_reg3.bit.reg_va_win0g2v_vacnt << 2);
    
#else
        p_indepva_rslt->va_g1_h    = local_reg0.bit.reg_va_win0g1h_vacc;
        p_indepva_rslt->va_cnt_g1_h = local_reg0.bit.reg_va_win0g1h_vacnt;
    
        p_indepva_rslt->va_g1_v    = local_reg1.bit.reg_va_win0g1v_vacc;
        p_indepva_rslt->va_cnt_g1_v = local_reg1.bit.reg_va_win0g1v_vacnt;
    
        p_indepva_rslt->va_g2_h    = local_reg2.bit.reg_va_win0g2h_vacc;
        p_indepva_rslt->va_cnt_g2_h = local_reg2.bit.reg_va_win0g2h_vacnt;
    
        p_indepva_rslt->va_g2_v    = local_reg3.bit.reg_va_win0g2v_vacc;
        p_indepva_rslt->va_cnt_g2_v = local_reg3.bit.reg_va_win0g2v_vacnt;
#endif
}

ER ipe_check_param(IPE_MODEINFO *p_mode_info)
{
	ER er_return = E_OK;

	if (((p_mode_info->in_info.ipe_mode == IPE_OPMODE_IPP) || (p_mode_info->in_info.ipe_mode == IPE_OPMODE_SIE_IPP)) && (p_mode_info->out_info.yc_to_ime_en == 0)) {
		DBG_ERR("IPE: ipe mode is not D2D, but yc direct to ime is off !\r\n");
		return E_PAR;
	}

	if (p_mode_info->out_info.va_to_dram_en == 1) {
		er_return = ipe_check_va_lofs((p_mode_info->iq_info.p_va_win_info), &(p_mode_info->out_info));
		if (er_return != E_OK) {
			return er_return;
		}
		er_return = ipe_check_va_win_info((p_mode_info->iq_info.p_va_win_info), p_mode_info->in_info.in_size_y);
		if (er_return != E_OK) {
			return er_return;
		}
	}
	return er_return;
}

ER ipe_check_va_lofs(IPE_VA_WIN_PARAM *p_va_win, IPE_OUTYCINFO *p_out_info)
{
	UINT32 lofs_min;
	ER er_return = E_OK;

	if (p_out_info->va_outsel == 1) {
		lofs_min = (0x4) * (p_va_win->win_numx);
	} else {
		lofs_min = (0x2) * (p_va_win->win_numx);
	}

	if (p_out_info->lofs_va < lofs_min) {
		DBG_ERR("IPE: VA lineoffset setting wrong !\r\n");
		return E_SYS;
	}
	return er_return;

}

ER ipe_check_va_win_info(IPE_VA_WIN_PARAM *p_va_win, IPE_IMG_SIZE size)
{
	UINT32 width_end, height_end;
	ER er_return = E_OK;

	width_end = (p_va_win->win_stx + (p_va_win->win_numx) * (p_va_win->win_szx + p_va_win->win_spx) - p_va_win->win_spx);
	height_end = (p_va_win->win_sty + (p_va_win->win_numy) * (p_va_win->win_szy + p_va_win->win_spy) - p_va_win->win_spy);

	if (width_end > size.h_size) {
		DBG_ERR("IPE: VA setting wrong ! ( x size  > img_width: %d)\r\n", (int)size.h_size);
		DBG_ERR("stx:%d, numx:%d, szx:%d, skipx:%d\r\n", (int)p_va_win->win_stx, (int)p_va_win->win_numx, (int)p_va_win->win_szx, (int)p_va_win->win_spx);
		return E_SYS;
	}
	if (height_end > size.v_size) {
		DBG_ERR("IPE: VA setting wrong ! ( y size > img_height: %d))\r\n", (int)size.v_size);
		DBG_ERR("sty:%d, numy:%d, szy:%d, skipy:%d\r\n", (int)p_va_win->win_sty, (int)p_va_win->win_numy, (int)p_va_win->win_szy, (int)p_va_win->win_spy);
		return E_SYS;
	}

	return er_return;
}


void ipe_set_dma_out_defog_addr(UINT32 sao)
{
	//T_DMA_DEFOG_SUBIMG_OUTPUT_CHANNEL_REGISTER local_reg;

	if ((sao == 0)) {
		DBG_ERR("IMG Input addresses cannot be 0x0!\r\n");
		return;
	}

	//local_reg.reg = IPE_GETREG(DMA_DEFOG_SUBIMG_OUTPUT_CHANNEL_REGISTER_OFS);
	ipeg->reg_179.bit.defog_subimg_dramsao = sao >> 2;

	//IPE_SETREG(DMA_DEFOG_SUBIMG_OUTPUT_CHANNEL_REGISTER_OFS, local_reg.reg);
}

void ipe_set_dma_out_defog_offset(UINT32 ofso)
{
	//T_DMA_DEFOG_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(DMA_DEFOG_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER_OFS);
	ipeg->reg_180.bit.defog_subimg_lofso = ofso >> 2;

	//IPE_SETREG(DMA_DEFOG_SUBIMG_OUPTUPT_CHANNEL_LINEOFFSET_REGISTER_OFS, local_reg.reg);
}

void ipe_set_dfg_subimg_size(IPE_IMG_SIZE subimg_size)
{
	//T_DEFOG_SUBIMG_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(DEFOG_SUBIMG_REGISTER_OFS);
	ipeg->reg_176.bit.defog_subimg_width = subimg_size.h_size - 1;
	ipeg->reg_176.bit.defog_subimg_height = subimg_size.v_size - 1;

	//IPE_SETREG(DEFOG_SUBIMG_REGISTER_OFS, local_reg.reg);
}

void ipe_set_dfg_subout_param(UINT32 blk_sizeh, UINT32 blk_cent_h_fact, UINT32 blk_sizev, UINT32 blk_cent_v_fact)
{

	//T_DEFOG_SUBOUT_REGISTER_0 local_reg0;
	//T_DEFOG_SUBOUT_REGISTER_1 local_reg1;

	//local_reg0.reg = IPE_GETREG(DEFOG_SUBOUT_REGISTER_0_OFS);
	ipeg->reg_182.bit.defog_sub_blk_sizeh = blk_sizeh;
	ipeg->reg_182.bit.defog_blk_cent_hfactor = blk_cent_h_fact;
	//local_reg1.reg = IPE_GETREG(DEFOG_SUBOUT_REGISTER_1_OFS);
	ipeg->reg_183.bit.defog_sub_blk_sizev = blk_sizev;
	ipeg->reg_183.bit.defog_blk_cent_vfactor = blk_cent_v_fact;

	//IPE_SETREG(DEFOG_SUBOUT_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(DEFOG_SUBOUT_REGISTER_1_OFS, local_reg1.reg);

}

void ipe_set_dfg_input_bld(UINT8 *input_bld_wt)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_DEFOG_INPUT_BLENDING_REGISTER_0 local_reg0;
	//T_DEFOG_INPUT_BLENDING_REGISTER_1 local_reg1;
	//T_DEFOG_INPUT_BLENDING_REGISTER_2 local_reg2;


	//local_reg0.reg = IPE_GETREG(DEFOG_INPUT_BLENDING_REGISTER_0_OFS);
	//ipeg->reg_185.bit.defog_input_bldrto0 = input_bld_wt[0];
	//ipeg->reg_185.bit.defog_input_bldrto1 = input_bld_wt[1];
	//ipeg->reg_185.bit.defog_input_bldrto2 = input_bld_wt[2];
	//ipeg->reg_185.bit.defog_input_bldrto3 = input_bld_wt[3];
	//local_reg1.reg = IPE_GETREG(DEFOG_INPUT_BLENDING_REGISTER_1_OFS);
	//ipeg->reg_186.bit.defog_input_bldrto4 = input_bld_wt[4];
	//ipeg->reg_186.bit.defog_input_bldrto5 = input_bld_wt[5];
	//ipeg->reg_186.bit.defog_input_bldrto6 = input_bld_wt[6];
	//ipeg->reg_186.bit.defog_input_bldrto7 = input_bld_wt[7];

	for (i = 0; i < 2; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_INPUT_BLENDING_REGISTER_0_OFS + (i << 2);

		idx = (i << 2);
		reg_set_val = ((UINT32)input_bld_wt[idx]) + (((UINT32)input_bld_wt[idx+1]) << 8) + (((UINT32)input_bld_wt[idx+2]) << 16) + (((UINT32)input_bld_wt[idx+3]) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	//local_reg2.reg = IPE_GETREG(DEFOG_INPUT_BLENDING_REGISTER_2_OFS);
	ipeg->reg_187.bit.defog_input_bldrto8 = input_bld_wt[8];

	//IPE_SETREG(DEFOG_INPUT_BLENDING_REGISTER_0_OFS, local_reg0.reg);
	//IPE_SETREG(DEFOG_INPUT_BLENDING_REGISTER_1_OFS, local_reg1.reg);
	//IPE_SETREG(DEFOG_INPUT_BLENDING_REGISTER_2_OFS, local_reg2.reg);

}

void ipe_set_dfg_scal_factor(UINT32 scalfact_h, UINT32 scalfact_v)
{
	//T_DEFOG_SUBIMG_SCALING_REGISTER_0 local_reg;

	//local_reg.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER_0_OFS);
	ipeg->reg_181.bit.defog_subimg_hfactor = scalfact_h;
	ipeg->reg_181.bit.defog_subimg_vfactor = scalfact_v;

	//IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER_0_OFS, local_reg.reg);
}

void ipe_set_dfg_scal_filtcoeff(UINT8 *p_filtcoef)
{
	//T_DEFOG_SUBIMG_LPF_REGISTER local_reg;

	//local_reg.reg = IPE_GETREG(DEFOG_SUBIMG_LPF_REGISTER_OFS);
	ipeg->reg_188.bit.defog_lpf_c0 = p_filtcoef[0];
	ipeg->reg_188.bit.defog_lpf_c1 = p_filtcoef[1];
	ipeg->reg_188.bit.defog_lpf_c2 = p_filtcoef[2];

	//IPE_SETREG(DEFOG_SUBIMG_LPF_REGISTER_OFS, local_reg.reg);
}

void ipe_set_dfg_scalg_edgeinterplut(UINT8 *p_interp_diff_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_DEFOG_SUBIMG_SCALING_REGISTER1 local_reg0;
	//T_DEFOG_SUBIMG_SCALING_REGISTER2 local_reg1;
	//T_DEFOG_SUBIMG_SCALING_REGISTER3 local_reg2;
	//T_DEFOG_SUBIMG_SCALING_REGISTER4 local_reg3;

#if 0
	//local_reg0.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER1_OFS);
	ipeg->reg_203.bit.defog_interp_diff_lut0 = p_interp_diff_lut[0];
	ipeg->reg_203.bit.defog_interp_diff_lut1 = p_interp_diff_lut[1];
	ipeg->reg_203.bit.defog_interp_diff_lut2 = p_interp_diff_lut[2];
	ipeg->reg_203.bit.defog_interp_diff_lut3 = p_interp_diff_lut[3];
	ipeg->reg_203.bit.defog_interp_diff_lut4 = p_interp_diff_lut[4];
	//local_reg1.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER2_OFS);
	ipeg->reg_204.bit.defog_interp_diff_lut5 = p_interp_diff_lut[5];
	ipeg->reg_204.bit.defog_interp_diff_lut6 = p_interp_diff_lut[6];
	ipeg->reg_204.bit.defog_interp_diff_lut7 = p_interp_diff_lut[7];
	ipeg->reg_204.bit.defog_interp_diff_lut8 = p_interp_diff_lut[8];
	ipeg->reg_204.bit.defog_interp_diff_lut9 = p_interp_diff_lut[9];
	//local_reg2.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER3_OFS);
	ipeg->reg_205.bit.defog_interp_diff_lut10 = p_interp_diff_lut[10];
	ipeg->reg_205.bit.defog_interp_diff_lut11 = p_interp_diff_lut[11];
	ipeg->reg_205.bit.defog_interp_diff_lut12 = p_interp_diff_lut[12];
	ipeg->reg_205.bit.defog_interp_diff_lut13 = p_interp_diff_lut[13];
	ipeg->reg_205.bit.defog_interp_diff_lut14 = p_interp_diff_lut[14];

	//IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER1_OFS, local_reg0.reg);
	//IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER2_OFS, local_reg1.reg);
	//IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER3_OFS, local_reg2.reg);
#endif

	for (i = 0; i < 3; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_SUBIMG_SCALING_REGISTER1_OFS + (i << 2);

		idx = (i * 5);
		reg_set_val = (p_interp_diff_lut[idx] & 0x3F)	+ ((p_interp_diff_lut[idx + 1] & 0x3F) << 6) + ((p_interp_diff_lut[idx + 2] & 0x3F) << 12) +
					  ((p_interp_diff_lut[idx + 3] & 0x3F) << 18) + ((p_interp_diff_lut[idx + 4] & 0x3F) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	//local_reg3.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER4_OFS);
	ipeg->reg_206.bit.defog_interp_diff_lut15 = p_interp_diff_lut[15];
	ipeg->reg_206.bit.defog_interp_diff_lut16 = p_interp_diff_lut[16];
	//IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER4_OFS, local_reg3.reg);
}

void ipe_set_defog_scal_edgeinterp(UINT8 wdist, UINT8 wout, UINT8 wcenter, UINT8 wsrc)
{

	//T_DEFOG_SUBIMG_SCALING_REGISTER5 local_reg;

	//local_reg.reg = IPE_GETREG(DEFOG_SUBIMG_SCALING_REGISTER5_OFS);
	ipeg->reg_207.bit.defog_interp_wdist = wdist;
	ipeg->reg_207.bit.defog_interp_wout = wout;
	ipeg->reg_207.bit.defog_interp_wcenter = wcenter;
	ipeg->reg_207.bit.defog_interp_wsrc = wsrc;

	//IPE_SETREG(DEFOG_SUBIMG_SCALING_REGISTER5_OFS, local_reg.reg);
}

void ipe_set_defog_atmospherelight(UINT16 val0, UINT16 val1, UINT16 val2)
{
	//T_DEFOG_AIRLIGHT_REGISTER0 local_reg0;
	//T_DEFOG_AIRLIGHT_REGISTER1 local_reg1;

	//local_reg0.reg = IPE_GETREG(DEFOG_AIRLIGHT_REGISTER0_OFS);
	ipeg->reg_208.bit.defog_air0 = val0;
	ipeg->reg_208.bit.defog_air1 = val1;

	//local_reg1.reg = IPE_GETREG(DEFOG_AIRLIGHT_REGISTER1_OFS);
	ipeg->reg_209.bit.defog_air2 = val2;

	//IPE_SETREG(DEFOG_AIRLIGHT_REGISTER0_OFS, local_reg0.reg);
	//IPE_SETREG(DEFOG_AIRLIGHT_REGISTER1_OFS, local_reg1.reg);
}

void ipe_set_defog_fog_protect(BOOL self_cmp_en, UINT16 min_diff)
{
	//T_DEFOG_STRENGTH_CONTROL_REGISTER5 local_reg;

	//local_reg.reg = IPE_GETREG(DEFOG_STRENGTH_CONTROL_REGISTER5_OFS);
	ipeg->reg_214.bit.defog_min_diff = min_diff;
	ipeg->reg_214.bit.defog_selfcmp_en = self_cmp_en;

	//IPE_SETREG(DEFOG_STRENGTH_CONTROL_REGISTER5_OFS, local_reg.reg);
}

void ipe_set_defog_fog_mod(UINT16 *p_fog_mod_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//UINT32 addr_ofs, i;
	//T_DEFOG_FOG_MODIFY_REGISTER0 local_reg0;
	//T_DEFOG_FOG_MODIFY_REGISTER8 local_reg1;

	for (i = 0; i < 8; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_FOG_MODIFY_REGISTER0_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_fog_mod_lut[idx] & 0x3FF) + ((p_fog_mod_lut[idx + 1] & 0x3FF) << 16);

		//addr_ofs = i * 4;
		//local_reg0.reg = IPE_GETREG((DEFOG_FOG_MODIFY_REGISTER0_OFS + addr_ofs));

		//local_reg0.bit.defog_mod_lut_0 = p_fog_mod_lut[2 * i];
		//local_reg0.bit.defog_mod_lut_1 = p_fog_mod_lut[2 * i + 1];

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	//local_reg1.reg = IPE_GETREG(DEFOG_FOG_MODIFY_REGISTER8_OFS);
	ipeg->reg_202.bit.defog_mod_lut_16 = p_fog_mod_lut[16];
	//IPE_SETREG(DEFOG_FOG_MODIFY_REGISTER8_OFS, local_reg1.reg);
}


void ipe_set_defog_fog_target_lut(UINT16 *p_fog_target_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//UINT32 addr_ofs, i;
	//T_DEFOG_STRENGTH_CONTROL_REGISTER0 local_reg0;
	//T_DEFOG_STRENGTH_CONTROL_REGISTER4 local_reg1;

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_STRENGTH_CONTROL_REGISTER0_OFS + (i << 2);

		idx = (i << 1);
		reg_set_val = (p_fog_target_lut[idx] & 0x3FF) + ((p_fog_target_lut[idx + 1] & 0x3FF) << 16);

		//addr_ofs = i * 4;
		//local_reg0.reg = IPE_GETREG((DEFOG_STRENGTH_CONTROL_REGISTER0_OFS + addr_ofs));

		//local_reg0.bit.defog_target_lut_0 = p_fog_target_lut[2 * i];
		//local_reg0.bit.defog_target_lut_1 = p_fog_target_lut[2 * i + 1];

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	//local_reg1.reg = IPE_GETREG(DEFOG_STRENGTH_CONTROL_REGISTER4_OFS);
	ipeg->reg_193.bit.defog_target_lut_8 = p_fog_target_lut[8];
	//IPE_SETREG(DEFOG_STRENGTH_CONTROL_REGISTER4_OFS, local_reg1.reg);
}

void ipe_set_defog_strength(IPE_DEFOG_METHOD_SEL modesel, UINT8 fog_ratio, UINT8 dgain, UINT8 gain_th)
{

	//T_DEFOG_STRENGTH_CONTROL_REGISTER5 local_reg0;
	//T_DEFOG_STRENGTH_CONTROL_REGISTER6 local_reg1;

	//local_reg0.reg = IPE_GETREG(DEFOG_STRENGTH_CONTROL_REGISTER5_OFS);
	ipeg->reg_214.bit.defog_mode_sel = modesel;
	ipeg->reg_214.bit.defog_fog_rto = fog_ratio;
	ipeg->reg_214.bit.defog_dgain_ratio = dgain;

	//local_reg1.reg = IPE_GETREG(DEFOG_STRENGTH_CONTROL_REGISTER6_OFS);
	ipeg->reg_215.bit.defog_gain_th = gain_th;

	//IPE_SETREG(DEFOG_STRENGTH_CONTROL_REGISTER5_OFS, local_reg0.reg);
	//IPE_SETREG(DEFOG_STRENGTH_CONTROL_REGISTER6_OFS, local_reg1.reg);
}

void ipe_set_defog_outbld_lum_lut(BOOL lum_bld_ref, UINT8 *p_outbld_lum_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//UINT32 addr_ofs, i;
	//T_DEFOG_OUTPUT_BLENDING_REGISTER0 local_reg0;
	//T_DEFOG_OUTPUT_BLENDING_REGISTER4 local_reg1;

	for (i = 0; i < 4; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_OUTPUT_BLENDING_REGISTER0_OFS + (i << 2);

		idx = (i << 2);
		reg_set_val = (p_outbld_lum_lut[idx] & 0xFF) + ((p_outbld_lum_lut[idx + 1] & 0xFF) << 8) + ((p_outbld_lum_lut[idx + 2] & 0xFF) << 16) + ((p_outbld_lum_lut[idx + 3] & 0xFF) << 24);


		//addr_ofs = i * 4;
		//local_reg0.reg = IPE_GETREG((DEFOG_OUTPUT_BLENDING_REGISTER0_OFS + addr_ofs));

		//local_reg0.bit.defog_outbld_lumwt0 = p_outbld_lum_lut[4 * i];
		//local_reg0.bit.defog_outbld_lumwt1 = p_outbld_lum_lut[4 * i + 1];
		//local_reg0.bit.defog_outbld_lumwt2 = p_outbld_lum_lut[4 * i + 2];
		//local_reg0.bit.defog_outbld_lumwt3 = p_outbld_lum_lut[4 * i + 3];

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	//local_reg1.reg = IPE_GETREG(DEFOG_OUTPUT_BLENDING_REGISTER4_OFS);
	ipeg->reg_220.bit.defog_outbld_lumwt16 = p_outbld_lum_lut[16];
	ipeg->reg_220.bit.defog_wet_ref = lum_bld_ref;
	//IPE_SETREG(DEFOG_OUTPUT_BLENDING_REGISTER4_OFS, local_reg1.reg);
}

void ipe_set_defog_outbld_diff_lut(UINT8 *p_outbld_diff_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//UINT32 addr_ofs, i;
	//T_DEFOG_OUTPUT_BLENDING_REGISTER5 local_reg0;
	//T_DEFOG_OUTPUT_BLENDING_REGISTER8 local_reg1;

	for (i = 0; i < 3; i++) {
		//reg_set_val = 0;

		reg_ofs = DEFOG_OUTPUT_BLENDING_REGISTER5_OFS + (i << 2);

		idx = (i * 5);
		reg_set_val = (p_outbld_diff_lut[idx] & 0x3F) + ((p_outbld_diff_lut[idx + 1] & 0x3F) << 6) + ((p_outbld_diff_lut[idx + 2] & 0x3F) << 12) + ((p_outbld_diff_lut[idx + 3] & 0x3F) << 18) + ((p_outbld_diff_lut[idx + 4] & 0x3F) << 24);

		//addr_ofs = i * 4;
		//local_reg0.reg = IPE_GETREG((DEFOG_OUTPUT_BLENDING_REGISTER5_OFS + addr_ofs));

		//local_reg0.bit.defog_outbld_diffwt0 = p_outbld_diff_lut[5 * i];
		//local_reg0.bit.defog_outbld_diffwt1 = p_outbld_diff_lut[5 * i + 1];
		//local_reg0.bit.defog_outbld_diffwt2 = p_outbld_diff_lut[5 * i + 2];
		//local_reg0.bit.defog_outbld_diffwt3 = p_outbld_diff_lut[5 * i + 3];
		//local_reg0.bit.defog_outbld_diffwt4 = p_outbld_diff_lut[5 * i + 4];

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	//local_reg1.reg = IPE_GETREG(DEFOG_OUTPUT_BLENDING_REGISTER8_OFS);
	ipeg->reg_224.bit.defog_outbld_diffwt15 = p_outbld_diff_lut[15];
	ipeg->reg_224.bit.defog_outbld_diffwt16 = p_outbld_diff_lut[16];
	//IPE_SETREG(DEFOG_OUTPUT_BLENDING_REGISTER8_OFS, local_reg1.reg);

}

void ipe_set_defog_outbld_local(UINT32 local_en, UINT32 air_nfactor)
{
	//T_DEFOG_AIRLIGHT_REGISTER2 local_reg0;
	//T_DEFOG_OUTPUT_BLENDING_REGISTER4 local_reg1;

	//local_reg0.reg = IPE_GETREG(DEFOG_AIRLIGHT_REGISTER2_OFS);
	ipeg->reg_213.bit.defog_air_normfactor = air_nfactor;

	//local_reg1.reg = IPE_GETREG(DEFOG_OUTPUT_BLENDING_REGISTER4_OFS);
	ipeg->reg_220.bit.defog_local_outbld_en = local_en;

	//IPE_SETREG(DEFOG_AIRLIGHT_REGISTER2_OFS, local_reg0.reg);
	//IPE_SETREG(DEFOG_OUTPUT_BLENDING_REGISTER4_OFS, local_reg1.reg);
}

void ipe_set_defog_stcs_pcnt(UINT32 pixelcnt)
{
	//T_DEFOG_STATISTICS_REGISTER0 local_reg1;

	//local_reg1.reg = IPE_GETREG(DEFOG_STATISTICS_REGISTER0_OFS);
	ipeg->reg_210.bit.defog_statistics_pixcnt = pixelcnt;

	//IPE_SETREG(DEFOG_STATISTICS_REGISTER0_OFS, local_reg1.reg);
}

void ipe_get_defog_stcs(DEFOG_STCS_RSLT *stcs)
{
	//T_DEFOG_STATISTICS_REGISTER1 local_reg0;
	//T_DEFOG_STATISTICS_REGISTER2 local_reg1;

	//local_reg0.reg = IPE_GETREG(DEFOG_STATISTICS_REGISTER1_OFS);
	stcs->airlight[0] = ipeg->reg_211.bit.defog_statistics_air0;
	stcs->airlight[1] = ipeg->reg_211.bit.defog_statistics_air1;

	//local_reg1.reg = IPE_GETREG(DEFOG_STATISTICS_REGISTER2_OFS);
	stcs->airlight[2] = ipeg->reg_212.bit.defog_statistics_air2;
}

void ipe_get_defog_airlight_setting(UINT16 *p_val)
{
	//T_DEFOG_AIRLIGHT_REGISTER0 local_reg0;
	//T_DEFOG_AIRLIGHT_REGISTER1 local_reg1;

	//local_reg0.reg = IPE_GETREG(DEFOG_AIRLIGHT_REGISTER0_OFS);
	p_val[0] = ipeg->reg_208.bit.defog_air0;
	p_val[1] = ipeg->reg_208.bit.defog_air1;

	//local_reg1.reg = IPE_GETREG(DEFOG_AIRLIGHT_REGISTER1_OFS);
	p_val[2] = ipeg->reg_209.bit.defog_air2;
}


void ipe_set_lce_diff_param(UINT8 wt_avg, UINT8 wt_pos, UINT8 wt_neg)
{
	//T_LCE_REGISTER_0 local_reg1;

	//local_reg1.reg = IPE_GETREG(LCE_REGISTER_0_OFS);
	ipeg->reg_225.bit.lce_wt_diff_pos = wt_pos;
	ipeg->reg_225.bit.lce_wt_diff_neg = wt_neg;
	ipeg->reg_225.bit.lce_wt_diff_avg = wt_avg;

	//IPE_SETREG(LCE_REGISTER_0_OFS, local_reg1.reg);
}

void ipe_set_lce_lum_lut(UINT8 *p_lce_lum_lut)
{
	UINT32 i = 0, idx = 0, reg_ofs = 0, reg_set_val = 0;

	//T_LCE_REGISTER_1 local_reg0;
	//T_LCE_REGISTER_2 local_reg1;
	//T_LCE_REGISTER_3 local_reg2;

	//local_reg0.reg = IPE_GETREG(LCE_REGISTER_1_OFS);
	//ipeg->reg_226.bit.lce_lum_adj_lut0 = p_lce_lum_lut[0];
	//ipeg->reg_226.bit.lce_lum_adj_lut1 = p_lce_lum_lut[1];
	//ipeg->reg_226.bit.lce_lum_adj_lut2 = p_lce_lum_lut[2];
	//ipeg->reg_226.bit.lce_lum_adj_lut3 = p_lce_lum_lut[3];
	//local_reg1.reg = IPE_GETREG(LCE_REGISTER_2_OFS);
	//ipeg->reg_227.bit.lce_lum_adj_lut4 = p_lce_lum_lut[4];
	//ipeg->reg_227.bit.lce_lum_adj_lut5 = p_lce_lum_lut[5];
	//ipeg->reg_227.bit.lce_lum_adj_lut6 = p_lce_lum_lut[6];
	//ipeg->reg_227.bit.lce_lum_adj_lut7 = p_lce_lum_lut[7];

	for (i = 0; i < 2; i++) {
		//reg_set_val = 0;

		reg_ofs = LCE_REGISTER_1_OFS + (i << 2);

		idx = (i << 2);
		reg_set_val = ((UINT32)p_lce_lum_lut[idx]) + (((UINT32)p_lce_lum_lut[idx+1]) << 8) + (((UINT32)p_lce_lum_lut[idx+2]) << 16) + (((UINT32)p_lce_lum_lut[idx+3]) << 24);

		IPE_SETREG(reg_ofs, reg_set_val);
	}

	//local_reg2.reg = IPE_GETREG(LCE_REGISTER_3_OFS);
	ipeg->reg_228.bit.lce_lum_adj_lut8 = p_lce_lum_lut[8];

	//IPE_SETREG(LCE_REGISTER_1_OFS, local_reg0.reg);
	//IPE_SETREG(LCE_REGISTER_2_OFS, local_reg1.reg);
	//IPE_SETREG(LCE_REGISTER_3_OFS, local_reg2.reg);
}

#endif

#ifdef __KERNEL__
EXPORT_SYMBOL(ipe_clr_frame_end);
EXPORT_SYMBOL(ipe_wait_frame_end);
EXPORT_SYMBOL(ipe_clr_dma_end);
EXPORT_SYMBOL(ipe_wait_dma_end);
EXPORT_SYMBOL(ipe_clr_gamma_in_end);
EXPORT_SYMBOL(ipe_wait_gamma_in_end);
EXPORT_SYMBOL(ipe_clr_frame_start);
EXPORT_SYMBOL(ipe_wait_frame_start);
EXPORT_SYMBOL(ipe_clr_ll_done);
EXPORT_SYMBOL(ipe_wait_ll_done);
EXPORT_SYMBOL(ipe_clr_ll_job_end);
EXPORT_SYMBOL(ipe_wait_ll_job_end);
EXPORT_SYMBOL(ipe_open);
EXPORT_SYMBOL(ipe_close);
EXPORT_SYMBOL(ipe_change_param);
EXPORT_SYMBOL(ipe_change_interrupt);
EXPORT_SYMBOL(ipe_change_input);
EXPORT_SYMBOL(ipe_change_output);
EXPORT_SYMBOL(ipe_set_builtin_flow_disable);
EXPORT_SYMBOL(ipe_set_mode);
//EXPORT_SYMBOL(ipe_change_all);
EXPORT_SYMBOL(ipe_start);
EXPORT_SYMBOL(ipe_pause);
EXPORT_SYMBOL(ipe_reset);
EXPORT_SYMBOL(ipe_load_gamma);
EXPORT_SYMBOL(ipe_read_gamma);
EXPORT_SYMBOL(ipe_check_param);
EXPORT_SYMBOL(ipe_check_function_enable);
EXPORT_SYMBOL(ipe_check_va_win_info);
EXPORT_SYMBOL(ipe_get_h_stripe);
EXPORT_SYMBOL(ipe_get_v_stripe);
EXPORT_SYMBOL(ipe_get_int_enable);
EXPORT_SYMBOL(ipe_get_int_status);
EXPORT_SYMBOL(ipe_clear_int);
EXPORT_SYMBOL(ipe_get_debug_status);
EXPORT_SYMBOL(ipe_check_busy);
EXPORT_SYMBOL(ipe_get_defog_stcs);
EXPORT_SYMBOL(ipe_get_edge_stcs);
EXPORT_SYMBOL(ipe_get_va_rslt_manual);
EXPORT_SYMBOL(ipe_get_indep_va_win_rslt);
EXPORT_SYMBOL(ipe_int_to_2comp);
EXPORT_SYMBOL(ipe_2comp_to_int);
#endif

#if (defined(_NVT_EMULATION_) == ON)
void ipe_update_opmode(IPE_OPMODE ipe_opmode_set)
{
	ipe_opmode = ipe_opmode_set;
}

ER ipe_set_mode_fix_state(VOID)
{
	ER er_return;

	er_return = ipe_state_machine(IPE_OP_SETMODE, UPDATE);

	return er_return;
}
#endif

//@
