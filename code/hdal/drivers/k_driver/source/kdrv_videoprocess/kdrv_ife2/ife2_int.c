
/*
    IFE2 module driver

    NT98520 IFE2 module driver.

    @file       ife2_int.c
    @ingroup    mIIPPIFE2
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include    "ife2_platform.h"


#include "ife2_base.h"
#include "ife2_int.h"

volatile NT98520_IFE2_REGISTER_STRUCT *ifeg2 = NULL;


static UINT32 ife2_set_avg_u = 128, ife2_set_avg_v = 128;

//static UINT32 ife2_get_engine_lock_status(VOID);
static UINT32 ife2_get_avg_u(VOID);
static UINT32 ife2_get_avg_v(VOID);


//-----------------------------------------------------------------------

//static UINT32 ife2_get_engine_lock_status(VOID)
//{
//	return g_ife2_lock_status;
//}
//-----------------------------------------------------------------------

static UINT32 ife2_get_avg_u(VOID)
{
	UINT32 sum = 0x0, cnt = 0x0, avg = 0x0;

	sum = ife2_get_statistical_information_sum_u_reg();
	cnt = ife2_get_statistical_information_cnt_reg();

	avg = ife2_set_avg_u;
	if (cnt != 0) {
		avg = (sum + (cnt >> 1)) / cnt;
	}
	ife2_set_avg_u = avg;

	return ife2_set_avg_u;
}
//-----------------------------------------------------------------------

static UINT32 ife2_get_avg_v(VOID)
{
	UINT32 sum = 0x0, cnt = 0x0, avg = 0x0;

	sum = ife2_get_statistical_information_sum_v_reg();
	cnt = ife2_get_statistical_information_cnt_reg();

	avg = ife2_set_avg_v;
	if (cnt != 0) {
		avg = (sum + (cnt >> 1)) / cnt;
	}
	ife2_set_avg_v = avg;

	return ife2_set_avg_v;
}
//-----------------------------------------------------------------------


VOID ife2_set_engine_control(IFE2_ENGINE_CTRL_SEL ctrl_sel, UINT32 op_val)
{
	if (ctrl_sel == IFE2_CTRL_RESET) {
		ife2_set_reset_reg(ENABLE);
		ife2_set_reset_reg(DISABLE);
	} else if (ctrl_sel == IFE2_CTRL_START) {
		ife2_set_start_reg(op_val);
	} else if (ctrl_sel == IFE2_CTRL_LOAD) {
		switch ((IFE2_LOAD_TYPE)op_val) {
		case IFE2_START_LOAD:
			ife2_set_load_reg(IFE2_LOADTYPE_START, ENABLE);
			break;

		case IFE2_FRMEND_LOAD:
			ife2_set_load_reg(IFE2_LOADTYPE_FMEND, ENABLE);
			break;

		case IFE2_DIRECT_START_LOAD:
			ife2_set_load_reg(IFE2_LOADTYPE_DIRECT_START, ENABLE);
			break;

		default:
			DBG_ERR("IFE2: load type error...\r\n");
			break;
		}
	} else if (ctrl_sel == IFE2_CTRL_IFMT) {
		ife2_set_input_format_reg(op_val);
	} else if (ctrl_sel == IFE2_CTRL_OFMT) {
		ife2_set_output_format_reg(op_val);
	} else if (ctrl_sel == IFE2_CTRL_OUTDEST) {
		ife2_set_output_dest_reg(op_val);
	} else if (ctrl_sel == IFE2_CTRL_YFTREN) {
		ife2_set_filter_ychannel_enable_reg(op_val);
	} else if (ctrl_sel == IFE2_CTRL_FTREN) {
		ife2_set_filter_enable_reg(op_val);
	} else if (ctrl_sel == IFE2_CTRL_INTPEN) {
		ife2_set_interrupt_enable_reg(IFE2_INTP_ALL, op_val);
	} else if (ctrl_sel == IFE2_CTRL_INTP) {
		ife2_set_interrupt_status_reg(IFE2_INTP_ALL, op_val);
	} else if (ctrl_sel == IFE2_CTRL_OUTDRAM) {
		ife2_set_output_dram_reg(op_val);
	}
}
//-----------------------------------------------------------------------

UINT32 ife2_get_engine_info(IFE2_ENGINE_GET_SEL get_sel)
{
	UINT32 get_val = 0x0;

	switch (get_sel) {
	case IFE2_GET_INTSTATUS:
		get_val = ife2_get_interrupt_status_reg();
		break;

	case IFE2_GET_INTESTATUS:
		get_val = ife2_get_interrupt_enable_reg();
		break;

	case IFE2_GET_IMG_HSIZE:
		get_val = ife2_get_img_size_h_reg();
		break;

	case IFE2_GET_IMG_VSIZE:
		get_val = ife2_get_img_size_v_reg();
		break;

	case IFE2_GET_IFMT:
		get_val = ife2_get_input_format_reg();
		break;

	case IFE2_GET_OFMT:
		get_val = ife2_get_output_format_reg();
		break;

	case IFE2_GET_AVG_U:
		get_val = ife2_get_avg_u();
		break;

	case IFE2_GET_AVG_V:
		get_val = ife2_get_avg_v();
		break;

	case IFE2_GET_STARTSTATUS:
		get_val = ife2_get_start_reg();
		break;

	//case IFE2_GET_LOCKSTATUS:
	//  get_val = ife2_get_engine_lock_status();
	//  break;

	case IFE2_GET_IN_DMAY:
		get_val = ife2_get_input_dmaaddress_y_reg();
		break;

	case IFE2_GET_OUT_DMAY0:
		get_val = ife2_get_output_dma_addr_y0_reg();
		break;

	case IFE2_GET_IN_LOFSY:
		get_val = ife2_get_input_lineoffset_y_reg();
		break;

	case IFE2_GET_OUT_LOFSY:
		get_val = ife2_get_output_lineoffset_y_reg();
		break;

	case IFE2_GET_BASE_ADDR:
		get_val = ife2_get_base_addr_reg();
		break;

	case IFE2_GET_DEST:
		get_val = ife2_get_output_dest_reg();
		break;

	default:
		// do nothing...
		break;
	}

	return get_val;
}
//-----------------------------------------------------------------------

//VOID ife2_set_image_size(UINT32 img_size_h, UINT32 img_size_v)
//{
//	ife2_set_image_size_reg(img_size_h, img_size_v);
//}
//-----------------------------------------------------------------------

#if 0
VOID ife2_chg_burst_length(IFE2_BURST_LENGHT *p_set_bst)
{
#ifdef __KERNEL__

#else

#if (defined(_NVT_EMULATION_) == ON)
	{
		if (p_set_bst != NULL)
		{
			ife2_set_burst_length_reg((UINT32)(p_set_bst->in_bst_len), (UINT32)(p_set_bst->out_bst_len));
		}
	}
#else
	{
		ife2_set_burst_length_reg((UINT32)(p_set_bst->in_bst_len), (UINT32)IFE2_BURST_32W);
	}
#endif
#endif
}
#endif
//-----------------------------------------------------------------------

#if 0
VOID ife2_set_lineoffset(IFE2_IOTYPE_SEL io_sel, UINT32 lofs_y, UINT32 lofs_c)
{
	switch (io_sel) {
	case IFE2_IOTYPE_IN:
		ife2_set_input_lineoffset_reg(lofs_y, lofs_c);
		break;

	case IFE2_IOTYPE_OUT:
		ife2_set_output_lineoffset_reg(lofs_y, lofs_c);
		break;

	default:
		// do nothing...
		break;
	}
}
#endif
//-----------------------------------------------------------------------

#if 0
VOID ife2_set_address(IFE2_DMA_SEL io_sel, UINT32 addr_y, UINT32 addr_c)
{
	switch (io_sel) {
	case IFE2_DMA_IN:
		ife2_set_input_dma_addr_reg(addr_y, addr_c);
		break;

	case IFE2_DMA_OUT0:
		ife2_set_output_dma_addr0_reg(addr_y, addr_c);
		break;

	default:
		// do nothing...
		break;
	}
}
#endif
//-----------------------------------------------------------------------
#if 0
VOID ife2_set_reference_center_info(IFE2_CHL_SEL chl_sel, IFE2_REFCENT_PARAM *p_ref_cent_info)
{
	if (p_ref_cent_info != NULL) {
		switch (chl_sel) {
		case IFE2_CHL_Y:
			ife2_set_reference_center_cal_y_reg(p_ref_cent_info->ref_cent_y.range_th, p_ref_cent_info->ref_cent_y.cnt_wet, p_ref_cent_info->ref_cent_y.range_wet, p_ref_cent_info->ref_cent_y.outlier_th);

			ife2_set_outlier_difference_threshold_reg(p_ref_cent_info->ref_cent_y.outlier_dth, p_ref_cent_info->ref_cent_y.outlier_dth, p_ref_cent_info->ref_cent_y.outlier_dth);
			break;

		case IFE2_CHL_UV:
			ife2_set_reference_center_cal_uv_reg(p_ref_cent_info->ref_cent_uv.range_th, p_ref_cent_info->ref_cent_uv.cnt_wet, p_ref_cent_info->ref_cent_uv.range_wet, p_ref_cent_info->ref_cent_uv.outlier_th);

			ife2_set_outlier_difference_threshold_reg(p_ref_cent_info->ref_cent_uv.outlier_dth, p_ref_cent_info->ref_cent_uv.outlier_dth, p_ref_cent_info->ref_cent_uv.outlier_dth);
			break;

		case IFE2_CHL_ALL:
			ife2_set_reference_center_cal_y_reg(p_ref_cent_info->ref_cent_y.range_th, p_ref_cent_info->ref_cent_y.cnt_wet, p_ref_cent_info->ref_cent_y.range_wet, p_ref_cent_info->ref_cent_y.outlier_th);
			ife2_set_reference_center_cal_uv_reg(p_ref_cent_info->ref_cent_uv.range_th, p_ref_cent_info->ref_cent_uv.cnt_wet, p_ref_cent_info->ref_cent_uv.range_wet, p_ref_cent_info->ref_cent_uv.outlier_th);

			ife2_set_outlier_difference_threshold_reg(p_ref_cent_info->ref_cent_y.outlier_dth, p_ref_cent_info->ref_cent_uv.outlier_dth, p_ref_cent_info->ref_cent_uv.outlier_dth);
			break;

		default:
			// do nothing...
			break;
		}
	}
}
#endif

#if 0
VOID ife2_set_reference_center_info(IFE2_CHL_SEL chl_sel, IFE2_REFCENT_SET *p_set_ref_cent_y, IFE2_REFCENT_SET *p_set_ref_cent_uv)
{
	if ((p_set_ref_cent_y != NULL) && (p_set_ref_cent_uv != NULL)) {
		switch (chl_sel) {
		case IFE2_CHL_Y:
			ife2_set_reference_center_cal_y_reg(p_set_ref_cent_y->range_th, p_set_ref_cent_y->cnt_wet, p_set_ref_cent_y->range_wet, p_set_ref_cent_y->outlier_th);

			ife2_set_outlier_difference_threshold_reg(p_set_ref_cent_y->outlier_dth, p_set_ref_cent_y->outlier_dth, p_set_ref_cent_y->outlier_dth);
			break;

		case IFE2_CHL_UV:
			ife2_set_reference_center_cal_uv_reg(p_set_ref_cent_uv->range_th, p_set_ref_cent_uv->cnt_wet, p_set_ref_cent_uv->range_wet, p_set_ref_cent_uv->outlier_th);

			ife2_set_outlier_difference_threshold_reg(p_set_ref_cent_uv->outlier_dth, p_set_ref_cent_uv->outlier_dth, p_set_ref_cent_uv->outlier_dth);
			break;

		case IFE2_CHL_ALL:
			ife2_set_reference_center_cal_y_reg(p_set_ref_cent_y->range_th, p_set_ref_cent_y->cnt_wet, p_set_ref_cent_y->range_wet, p_set_ref_cent_y->outlier_th);
			ife2_set_reference_center_cal_uv_reg(p_set_ref_cent_uv->range_th, p_set_ref_cent_uv->cnt_wet, p_set_ref_cent_uv->range_wet, p_set_ref_cent_uv->outlier_th);

			ife2_set_outlier_difference_threshold_reg(p_set_ref_cent_y->outlier_dth, p_set_ref_cent_uv->outlier_dth, p_set_ref_cent_uv->outlier_dth);
			break;

		default:
			// do nothing...
			break;
		}
	}
}
#endif
//-----------------------------------------------------------------------

//VOID ife2_set_filter_win_size(IFE2_FILT_SIZE filter_size, IFE2_EDGE_KERNEL_SIZE edge_ker_size)
//{
//	ife2_set_filter_size_reg((UINT32)filter_size, (UINT32)edge_ker_size);
//}
//-----------------------------------------------------------------------

#if 0
VOID ife2_ife2_set_filter_info(IFE2_CHL_SEL chl_sel, IFE2_FILTER_LUT_PARAM *p_filter_info)
{
	switch (chl_sel) {
	case IFE2_CHL_Y:
		ife2_set_filter_computation_param_y_reg(p_filter_info->filter_set_y.filter_th, p_filter_info->filter_set_y.filter_wet);
		break;

	case IFE2_CHL_U:
		ife2_set_filter_computation_param_u_reg(p_filter_info->filter_set_u.filter_th, p_filter_info->filter_set_u.filter_wet);
		break;

	case IFE2_CHL_V:
		ife2_set_filter_computation_param_v_reg(p_filter_info->filter_set_v.filter_th, p_filter_info->filter_set_v.filter_wet);
		break;

	case IFE2_CHL_ALL:
		ife2_set_filter_computation_param_y_reg(p_filter_info->filter_set_y.filter_th, p_filter_info->filter_set_y.filter_wet);
		ife2_set_filter_computation_param_u_reg(p_filter_info->filter_set_u.filter_th, p_filter_info->filter_set_u.filter_wet);
		ife2_set_filter_computation_param_v_reg(p_filter_info->filter_set_v.filter_th, p_filter_info->filter_set_v.filter_wet);
		break;

	default:
		// do nothing...
		break;
	}
}
#endif

#if 0
VOID ife2_ife2_set_filter_info(IFE2_CHL_SEL chl_sel, IFE2_FILTER_SET *p_filter_y, IFE2_FILTER_SET *p_filter_u, IFE2_FILTER_SET *p_filter_v)
{
	switch (chl_sel) {
	case IFE2_CHL_Y:
		ife2_set_filter_computation_param_y_reg(p_filter_y->filter_th, p_filter_y->filter_wet);
		break;

	case IFE2_CHL_U:
		ife2_set_filter_computation_param_u_reg(p_filter_u->filter_th, p_filter_u->filter_wet);
		break;

	case IFE2_CHL_V:
		ife2_set_filter_computation_param_v_reg(p_filter_v->filter_th, p_filter_v->filter_wet);
		break;

	case IFE2_CHL_ALL:
		ife2_set_filter_computation_param_y_reg(p_filter_y->filter_th, p_filter_y->filter_wet);
		ife2_set_filter_computation_param_u_reg(p_filter_u->filter_th, p_filter_u->filter_wet);
		ife2_set_filter_computation_param_v_reg(p_filter_v->filter_th, p_filter_v->filter_wet);
		break;

	default:
		// do nothing...
		break;
	}
}
#endif
//-----------------------------------------------------------------------

//VOID ife2_set_edge_direction_threshold(IFE2_EDRCT_TH *p_dth)
//{
//	if (p_dth != NULL) {
//		ife2_set_edge_direction_threshold_reg(p_dth->edth_pn, p_dth->edth_hv);
//	}
//}
//-----------------------------------------------------------------------

//VOID ife2_set_statistical_info(IFE2_GRAY_STATAL *p_sta_info)
//{
//	if (p_sta_info != NULL) {
//		ife2_set_statistical_information_threshold_reg(p_sta_info->u_th0, p_sta_info->u_th1, p_sta_info->v_th0, p_sta_info->v_th1);
//	}
//}
//-----------------------------------------------------------------------

ER ife2_check_state_machine(IFE2_ENGINE_STATUS current_status, IFE2_ENGINE_OPERATION action_op)
{
	ER er_return;

	switch (current_status) {
	case IFE2_ENGINE_IDLE:
		switch (action_op) {
		case IFE2_OP_OPEN:
			er_return = E_OK;
			break;

		default:
			er_return = E_SYS;
			break;
		}
		break;

	case IFE2_ENGINE_READY:
		switch (action_op) {
		case IFE2_OP_CLOSE:
		case IFE2_OP_SETPARAM:
		case IFE2_OP_DYNAMICPARAM:
			er_return = E_OK;
			break;

		default:
			er_return = E_SYS;
			break;
		}
		break;

	case IFE2_ENGINE_PAUSE:
		switch (action_op) {
		case IFE2_OP_CLOSE:
		case IFE2_OP_PAUSE:
		case IFE2_OP_STARTRUN:
		case IFE2_OP_SETPARAM:
		case IFE2_OP_DYNAMICPARAM:
			er_return = E_OK;
			break;

		default:
			er_return = E_SYS;
			break;
		}
		break;

	case IFE2_ENGINE_RUN:
		switch (action_op) {
		case IFE2_OP_DYNAMICPARAM:
		case IFE2_OP_PAUSE:
			er_return = E_OK;
			break;

		default:
			er_return = E_SYS;
			break;
		}
		break;

	default:
		er_return = E_SYS;
		break;
	}

	return er_return;
}
//-----------------------------------------------------------------------

VOID ife2_flush_dma_buf(IFE2_DMAFLUSH_TYPE flash_type, UINT32 addr, UINT32 buf_size, IFE2_OPMODE op_mode)
{
	//UINT32 uiTmpAddr = addr;
	UINT32 tmp_size = buf_size;

	if (tmp_size == 0x0) {
		//
		DBG_WRN("IFE2: buffer size zero...\r\n");
		tmp_size = 64;
	}

#if 1
	switch (flash_type) {
	case IFE2_IFLUSH:
		if (op_mode == IFE2_OPMODE_D2D) {
			//dma_flushWriteCache(addr, buf_size);
			ife2_platform_dma_flush_mem2dev(addr, tmp_size);
		} else {
			ife2_platform_dma_flush_mem2dev_for_video_mode(addr, tmp_size);
		}
		//uiTmpAddr = dma_getPhyAddr(addr);
		break;

	case IFE2_OFLUSH_EQ:
		if (op_mode == IFE2_OPMODE_D2D) {
			//dma_flushReadCache(addr, buf_size);
			ife2_platform_dma_flush_dev2mem(addr, tmp_size);
		}
		//uiTmpAddr = dma_getPhyAddr(addr);
		break;

	case IFE2_OFLUSH_NEQ:
		if (op_mode == IFE2_OPMODE_D2D) {
			//dma_flushReadCacheWidthNEQLineOffset(addr, buf_size);
			ife2_platform_dma_flush_dev2mem_width_neq_loff(addr, tmp_size);
		}
		//uiTmpAddr = dma_getPhyAddr(addr);
		break;

	default:
		// do nothing...
		break;
	}
#else
#ifdef __KERNEL__
	//fmem_dcache_sync((void *)addr, buf_size, DMA_BIDIRECTIONAL);
	fmem_dcache_sync_vb((void *)addr, buf_size, DMA_BIDIRECTIONAL);
#else
	switch (flash_type) {
	case IFE2_IFLUSH:
		dma_flushWriteCache(addr, buf_size);
		//uiTmpAddr = dma_getPhyAddr(addr);
		break;

	case IFE2_OFLUSH_EQ:
		dma_flushReadCache(addr, buf_size);
		//uiTmpAddr = dma_getPhyAddr(addr);
		break;

	case IFE2_OFLUSH_NEQ:
		dma_flushReadCacheWidthNEQLineOffset(addr, buf_size);
		//uiTmpAddr = dma_getPhyAddr(addr);
		break;

	default:
		// do nothing...
		break;
	}
#endif
#endif
}
//-----------------------------------------------------------------------

ER ife2_check_dma_addr(UINT32 addr)
{
	if ((addr == 0x0)) {
		DBG_ERR("IFE2: input address zero...\r\n");
		return E_PAR;
	}

	return E_OK;
}
//-----------------------------------------------------------------------

//VOID ife2_set_debug_function(UINT32 edge_en, UINT32 ref_cent_en, UINT32 eng_en)
//{
//	ife2_set_debug_mode_reg(edge_en, ref_cent_en, eng_en);
//}
//-----------------------------------------------------------------------

INT32 ife2_get_burst_length_info(UINT32 get_sel)
{
	return ife2_get_burst_length_reg(get_sel);
}





