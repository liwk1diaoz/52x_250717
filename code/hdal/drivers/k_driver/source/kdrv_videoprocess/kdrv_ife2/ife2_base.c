
/*
    IFE2 module driver

    NT98520 IFE2 module driver.

    @file       ife2_base.c
    @ingroup    mIIPPIFE2
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include    "ife2_platform.h"


#if 0
#ifdef __KERNEL__

#include <plat-na51055/nvt-sramctl.h>
//#include <frammap/frammap_if.h>
#include <mach/fmem.h>

#include "kwrap/type.h"
#else


#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "Type.h"

#endif // end #ifdef __KERNEL__
#endif

#include "ife2_int.h"
#include "ife2_base.h"


#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

//-----------------------------------------------------------------------
VOID ife2_set_reset_reg(UINT32 op)
{
	T_IFE2_ENGINE_CONTROL_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS);

	arg.bit.ife2_rst = op;
	arg.bit.ife2_start = DISABLE;

	IFE2_SETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_start_reg(UINT32 op)
{
	T_IFE2_ENGINE_CONTROL_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS);

	arg.bit.ife2_start_aload = 0;
	arg.bit.ife2_frameend_aload = 0;
	arg.bit.ife2_drt_start_load = 0;

	arg.bit.ife2_start = op;

	IFE2_SETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_start_reg(VOID)
{
	T_IFE2_ENGINE_CONTROL_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS);

	return arg.bit.ife2_start;
}
//-----------------------------------------------------------------------

VOID ife2_set_load_reg(UINT32 load_type, UINT32 op)
{
	T_IFE2_ENGINE_CONTROL_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS);

	arg.bit.ife2_rst = 0x0;
	//arg.bit.ife2_start = 0x0;

	switch (load_type) {
	case IFE2_LOADTYPE_START:
		arg.bit.ife2_start_aload = op;
		arg.bit.ife2_frameend_aload = 0x0;
		arg.bit.ife2_drt_start_load = 0x0;
		break;

	case IFE2_LOADTYPE_FMEND:
		arg.bit.ife2_start_aload = 0x0;
		arg.bit.ife2_frameend_aload = op;
		arg.bit.ife2_drt_start_load = 0x0;
		break;

	case IFE2_LOADTYPE_DIRECT_START:
		arg.bit.ife2_start_aload = 0x0;
		arg.bit.ife2_frameend_aload = 0x0;
		arg.bit.ife2_drt_start_load = op;
		break;
	}

	IFE2_SETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_linked_list_fire_reg(UINT32 set_fire)
{
	T_IFE2_ENGINE_CONTROL_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS);

	arg.bit.ife2_ll_fire = set_fire;

	IFE2_SETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------


VOID ife2_set_linked_list_terminate_reg(UINT32 set_terminate)
{
	T_IFE2_LINKED_LIST_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_LINKED_LIST_REGISTER_OFS);

	arg.bit.ife2_ll_terminate = set_terminate;

	IFE2_SETREG(IFE2_LINKED_LIST_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------
UINT32 ife2_linked_list_dma_addr = 0;
VOID ife2_set_linked_list_dma_addr_reg(UINT32 set_addr)
{
	T_IFE2LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER3 arg;

	ife2_linked_list_dma_addr = set_addr;

	arg.reg = IFE2_GETREG(IFE2LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER3_OFS);

	//debug_msg("ll-addr: %08x\r\n", set_addr);

	arg.bit.ife2_dram_ll_sai = ife2_platform_va2pa(ife2_linked_list_dma_addr) >> 2;

	IFE2_SETREG(IFE2LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER3_OFS, arg.reg);
}
//-----------------------------------------------------------------------


VOID ife2_set_burst_length_reg(UINT32 in_len, UINT32 out_len)
{
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	arg.bit.ife2_in_brt_lenth = in_len;
	arg.bit.ife2_out_brt_lenth = out_len;

	IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

INT32 ife2_get_burst_length_reg(UINT32 get_sel)
{
	INT32 burst_len;
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	if (get_sel == 0) {
		switch (arg.bit.ife2_in_brt_lenth) {
		case 0:
			burst_len = 64;
			break;

		case 1:
			burst_len = 48;
			break;

		//case 2:
		//  burst_len = 96;
		//  break;

		//case 3:
		//  burst_len = 128;
		//  break;

		default:
			burst_len = 64;
			break;
		}
	} else {
		switch (arg.bit.ife2_out_brt_lenth) {
		case 0:
			burst_len = 64;
			break;

		case 1:
			burst_len = 32;
			break;

		//case 2:
		//  burst_len = 96;
		//  break;

		//case 3:
		//  burst_len = 128;
		//  break;

		default:
			burst_len = 64;
			break;
		}
	}

	return burst_len;
}
//-----------------------------------------------------------------------


VOID ife2_set_input_format_reg(UINT32 in_fmt)
{
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	arg.bit.ife2_ifmt = in_fmt;

	IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_output_format_reg(UINT32 out_fmt)
{
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	arg.bit.ife2_ofmt = out_fmt;

	IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_input_format_reg(VOID)
{
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	return arg.bit.ife2_ifmt;
}
//-----------------------------------------------------------------------

UINT32 ife2_get_output_format_reg(VOID)
{
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	return arg.bit.ife2_ofmt;
}
//-----------------------------------------------------------------------

VOID ife2_set_output_dest_reg(UINT32 out_dest)
{
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	arg.bit.ife2_dest = out_dest;

	IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_output_dram_reg(UINT32 set_en)
{
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	arg.bit.ife2_dram_out_en = set_en;

	IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------


UINT32 ife2_get_output_dest_reg(VOID)
{
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	return arg.bit.ife2_dest;
}
//-----------------------------------------------------------------------

VOID ife2_set_filter_ychannel_enable_reg(UINT32 op)
{
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	arg.bit.ife2_yftr_en = op;

	IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_filter_size_reg(UINT32 filter_size, UINT32 edge_size)
{
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	arg.bit.ife2_fsize_sel = filter_size;
	arg.bit.ife2_eksize_sel = edge_size;

	IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_interrupt_enable_reg(UINT32 int_sel, UINT32 int_val)
{
	T_IFE2_INTERRUPT_ENABLE_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_INTERRUPT_ENABLE_REGISTER_OFS);

	switch (int_sel) {

	case IFE2_INTP_LL_END:
		arg.bit.ife2_intpe_ll_end = int_val;
		break;

	case IFE2_INTP_LL_ERR:
		arg.bit.ife2_intpe_ll_err = int_val;
		break;

	case IFE2_INTP_LL_LATE:
		arg.bit.ife2_intpe_ll_red_late = int_val;
		break;

	case IFE2_INTP_LL_JEND:
		arg.bit.ife2_intpe_ll_job_end = int_val;
		break;

	case IFE2_INTP_OUT_OVF:
		arg.bit.ife2_intpe_out_ovf = int_val;
		break;

	case IFE2_INTP_FRM_END:
		arg.bit.ife2_intpe_frm_end = int_val;
		break;

	case IFE2_INTP_ALL:
		arg.reg = int_val;
		break;

	default:
		arg.reg = int_val;
		break;
	}

	IFE2_SETREG(IFE2_INTERRUPT_ENABLE_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_interrupt_status_reg(UINT32 int_sel, UINT32 int_val)
{
	T_IFE2_INTERRUPT_STATUS_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_INTERRUPT_STATUS_REGISTER_OFS);

	switch (int_sel) {
	case IFE2_INTP_LL_END:
		arg.bit.ife2_intps_ll_end = int_val;
		break;

	case IFE2_INTP_LL_ERR:
		arg.bit.ife2_intps_ll_err = int_val;
		break;

	case IFE2_INTP_LL_LATE:
		arg.bit.ife2_intps_ll_red_late = int_val;
		break;

	case IFE2_INTP_LL_JEND:
		arg.bit.ife2_intps_ll_job_end = int_val;
		break;

	case IFE2_INTP_OUT_OVF:
		arg.bit.ife2_intps_out_ovf = int_val;
		break;

	case IFE2_INTP_FRM_END:
		arg.bit.ife2_intps_frm_end = int_val;
		break;

	case IFE2_INTP_ALL:
		arg.reg = int_val;
		break;

	default:
		arg.reg = int_val;
		break;
	}

	IFE2_SETREG(IFE2_INTERRUPT_STATUS_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_interrupt_status_reg(VOID)
{
	T_IFE2_INTERRUPT_STATUS_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_INTERRUPT_STATUS_REGISTER_OFS);

	return arg.reg;
}
//-----------------------------------------------------------------------

UINT32 ife2_get_interrupt_enable_reg(VOID)
{
	T_IFE2_INTERRUPT_ENABLE_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_INTERRUPT_ENABLE_REGISTER_OFS);

	return arg.reg;
}
//-----------------------------------------------------------------------


VOID ife2_set_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	T_IFE2_INPUT_IMAGE_SIZE_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_IMAGE_SIZE_REGISTER0_OFS);

	arg.bit.ife2_h_size = size_h;
	arg.bit.ife2_v_size = size_v;

	IFE2_SETREG(IFE2_INPUT_IMAGE_SIZE_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_img_size_h_reg(VOID)
{
	T_IFE2_INPUT_IMAGE_SIZE_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_IMAGE_SIZE_REGISTER0_OFS);

	return arg.bit.ife2_h_size;
}
//-----------------------------------------------------------------------

UINT32 ife2_get_img_size_v_reg(VOID)
{
	T_IFE2_INPUT_IMAGE_SIZE_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_IMAGE_SIZE_REGISTER0_OFS);

	return arg.bit.ife2_v_size;
}
//-----------------------------------------------------------------------


VOID ife2_set_filter_enable_reg(UINT32 op)
{
	T_DEBUG_REGISTER0 arg;

	arg.reg = IFE2_GETREG(DEBUG_REGISTER0_OFS);

	arg.bit.ife2_filter_en = op;

	IFE2_SETREG(DEBUG_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_input_lineoffset_reg(UINT32 lofs_y, UINT32 lofs_uv)
{
	T_IFE2_DMA_INPUT_LINEOFFSET_REGISTER0 arg_y;
	//T_IFE2_INPUT_DMA_INPUT_LINEOFFSET_REGISTER1 argUV;

	arg_y.reg = IFE2_GETREG(IFE2_DMA_INPUT_LINEOFFSET_REGISTER0_OFS);
	//argUV.reg = IFE2_GETREG(IFE2_INPUT_DMA_INPUT_LINEOFFSET_REGISTER1_OFS);

	arg_y.bit.ife2_dram_y_ofsi = (lofs_y >> 2);
	//argUV.bit.IFE2_DRAM_UV_OFSI = (lofs_uv >> 2);

	IFE2_SETREG(IFE2_DMA_INPUT_LINEOFFSET_REGISTER0_OFS, arg_y.reg);
	//IFE2_SETREG(IFE2_INPUT_DMA_INPUT_LINEOFFSET_REGISTER1_OFS, argUV.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_input_lineoffset_y_reg(VOID)
{
	T_IFE2_DMA_INPUT_LINEOFFSET_REGISTER0 arg_y;

	arg_y.reg = IFE2_GETREG(IFE2_DMA_INPUT_LINEOFFSET_REGISTER0_OFS);

	return arg_y.reg;
}
//-----------------------------------------------------------------------

UINT32 ife2_input_dma_addr_y = 0x0;
VOID ife2_set_input_dma_addr_reg(UINT32 addr_y, UINT32 addr_uv)
{
	T_IFE2_INPUT_DMA_ADDRESS_REGISTER0 arg_y;
	//T_IFE2_INPUT_DMA_CHANNEL_REGISTER1 argUV;

	arg_y.reg = IFE2_GETREG(IFE2_INPUT_DMA_ADDRESS_REGISTER0_OFS);

	ife2_input_dma_addr_y = addr_y;

#if 1
	arg_y.bit.ife2_dram_y_sai = ife2_platform_va2pa(addr_y);
#else
#ifdef __KERNEL__

	arg_y.bit.ife2_dram_y_sai = fmem_lookup_pa(addr_y);

#else

	//argUV.reg = IFE2_GETREG(IFE2_INPUT_DMA_CHANNEL_REGISTER1_OFS);

	arg_y.bit.ife2_dram_y_sai = dma_getPhyAddr(addr_y);
#endif
#endif
	//argUV.bit.IFE2_DRAM_UV_SAI = addr_uv;

	IFE2_SETREG(IFE2_INPUT_DMA_ADDRESS_REGISTER0_OFS, arg_y.reg);
	//IFE2_SETREG(IFE2_INPUT_DMA_CHANNEL_REGISTER1_OFS, argUV.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_input_dmaaddress_y_reg(VOID)
{
	return ife2_input_dma_addr_y;
}
//-----------------------------------------------------------------------

VOID ife2_set_output_lineoffset_reg(UINT32 lofs_y, UINT32 lofs_uv)
{
	T_IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0 arg_y;
	//T_IFE2_OUTPUT_DMA_OUTPUT_LINEOFFSET_REGISTER1 argUV;

	arg_y.reg = IFE2_GETREG(IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0_OFS);
	//argUV.reg = IFE2_GETREG(IFE2_OUTPUT_DMA_OUTPUT_LINEOFFSET_REGISTER1_OFS);

	arg_y.bit.ife2_dram_y_ofso = (lofs_y >> 2);
	//argUV.bit.IFE2_DRAM_UV_OFSO = (lofs_uv >> 2);

	IFE2_SETREG(IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0_OFS, arg_y.reg);
	//IFE2_SETREG(IFE2_OUTPUT_DMA_OUTPUT_LINEOFFSET_REGISTER1_OFS, argUV.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_output_lineoffset_y_reg(VOID)
{
	T_IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0 arg_y;

	arg_y.reg = IFE2_GETREG(IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0_OFS);

	return arg_y.reg;
}
//-----------------------------------------------------------------------

UINT32 ife2_get_output_lineoffset_uv_reg(VOID)
{
#if 0
	T_IFE2_OUTPUT_DMA_OUTPUT_LINEOFFSET_REGISTER1 argUV;

	argUV.reg = IFE2_GETREG(IFE2_OUTPUT_DMA_OUTPUT_LINEOFFSET_REGISTER1_OFS);

	return argUV.reg;
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------

UINT32 ife2_output_dma_addr_y = 0x0;
VOID ife2_set_output_dma_addr0_reg(UINT32 addr_y, UINT32 addr_uv)
{
	T_IFE2_OUTPUT_DMA_ADDRESS_REGISTER0 arg_y;
	//T_IFE2_OUTPUT_DMA_CHANNEL_REGISTER1 argUV;

	arg_y.reg = IFE2_GETREG(IFE2_OUTPUT_DMA_ADDRESS_REGISTER0_OFS);
	//argUV.reg = IFE2_GETREG(IFE2_OUTPUT_DMA_CHANNEL_REGISTER1_OFS);

	ife2_output_dma_addr_y = addr_y;

#if 1
	arg_y.bit.ife2_dram_y_sao = ife2_platform_va2pa(addr_y);
#else
#ifdef __KERNEL__

	arg_y.bit.ife2_dram_y_sao = fmem_lookup_pa(addr_y);

#else
	arg_y.bit.ife2_dram_y_sao = dma_getPhyAddr(addr_y);
	//argUV.bit.IFE2_DRAM_UV_SAO0 = addr_uv;
#endif
#endif

	IFE2_SETREG(IFE2_OUTPUT_DMA_ADDRESS_REGISTER0_OFS, arg_y.reg);
	//IFE2_SETREG(IFE2_OUTPUT_DMA_CHANNEL_REGISTER1_OFS, argUV.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_output_dma_addr_y0_reg(VOID)
{
	return ife2_output_dma_addr_y;
}
//-----------------------------------------------------------------------


VOID ife2_set_reference_center_cal_y_reg(UINT32 *p_range_th_y, UINT32 wet_y, UINT32 *p_range_wet_y, UINT32 outlier_th_y)
{
	T_IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER0 arg0;
	T_IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER1 arg1;

	arg0.reg = IFE2_GETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER0_OFS);
	arg1.reg = IFE2_GETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER1_OFS);

	arg0.bit.ife2_y_rcth0 = p_range_th_y[0];
	arg0.bit.ife2_y_rcth1 = p_range_th_y[1];
	arg0.bit.ife2_y_rcth2 = p_range_th_y[2];
	arg0.bit.ife2_y_cwt = wet_y;

	arg1.bit.ife2_y_rcwt0 = p_range_wet_y[0];
	arg1.bit.ife2_y_rcwt1 = p_range_wet_y[1];
	arg1.bit.ife2_y_rcwt2 = p_range_wet_y[2];
	arg1.bit.ife2_y_rcwt3 = p_range_wet_y[3];
	arg1.bit.ife2_y_outl_th = outlier_th_y - 1;

	IFE2_SETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER0_OFS, arg0.reg);
	IFE2_SETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER1_OFS, arg1.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_reference_center_cal_uv_reg(UINT32 *p_range_th_uv, UINT32 wet_uv, UINT32 *p_range_wet_uv, UINT32 outlier_th_uv)
{
	T_IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER2 arg0;
	T_IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER3 arg1;

	arg0.reg = IFE2_GETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER2_OFS);
	arg1.reg = IFE2_GETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER3_OFS);

	arg0.bit.ife2_uv_rcth0 = p_range_th_uv[0];
	arg0.bit.ife2_uv_rcth1 = p_range_th_uv[1];
	arg0.bit.ife2_uv_rcth2 = p_range_th_uv[2];
	arg0.bit.ife2_uv_cwt = wet_uv;

	arg1.bit.ife2_uv_rcwt0 = p_range_wet_uv[0];
	arg1.bit.ife2_uv_rcwt1 = p_range_wet_uv[1];
	arg1.bit.ife2_uv_rcwt2 = p_range_wet_uv[2];
	arg1.bit.ife2_uv_rcwt3 = p_range_wet_uv[3];
	arg1.bit.ife2_uv_outl_th = outlier_th_uv - 1;

	IFE2_SETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER2_OFS, arg0.reg);
	IFE2_SETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER3_OFS, arg1.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_outlier_difference_threshold_reg(UINT32 dif_th_y, UINT32 dif_th_u, UINT32 dif_th_v)
{
	T_IFE2_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0_OFS);

	arg.bit.ife2_y_outl_dth = dif_th_y;
	arg.bit.ife2_u_outl_dth = dif_th_u;
	arg.bit.ife2_v_outl_dth = dif_th_v;

	IFE2_SETREG(IFE2_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_edge_direction_threshold_reg(UINT32 pn_th, UINT32 hv_th)
{
	T_IFE2_EDGE_DIRECTION_THRESHOLD_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_EDGE_DIRECTION_THRESHOLD_REGISTER0_OFS);

	arg.bit.ife2_ed_pn_th = pn_th;
	arg.bit.ife2_ed_hv_th = hv_th;

	IFE2_SETREG(IFE2_EDGE_DIRECTION_THRESHOLD_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_filter_computation_param_y_reg(UINT32 *p_th_y, UINT32 *p_wet_y)
{
	T_IFE2_FILTER_COMPUTATION_REGISTER0 arg0;
	T_IFE2_FILTER_COMPUTATION_REGISTER1 arg1;
	T_IFE2_FILTER_COMPUTATION_REGISTER2 arg2;

	arg0.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER0_OFS);
	arg1.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER1_OFS);
	arg2.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER2_OFS);

	arg0.bit.ife2_y_fth0 = p_th_y[0];
	arg0.bit.ife2_y_fth1 = p_th_y[1];
	arg0.bit.ife2_y_fth2 = p_th_y[2];
	arg0.bit.ife2_y_fth3 = p_th_y[3];
	arg1.bit.ife2_y_fth4 = p_th_y[4];

	arg2.bit.ife2_y_fwt0 = p_wet_y[0];
	arg2.bit.ife2_y_fwt1 = p_wet_y[1];
	arg2.bit.ife2_y_fwt2 = p_wet_y[2];
	arg2.bit.ife2_y_fwt3 = p_wet_y[3];
	arg2.bit.ife2_y_fwt4 = p_wet_y[4];
	arg2.bit.ife2_y_fwt5 = p_wet_y[5];

	IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER0_OFS, arg0.reg);
	IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER1_OFS, arg1.reg);
	IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER2_OFS, arg2.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_filter_computation_param_u_reg(UINT32 *p_th_u, UINT32 *p_wet_u)
{
	T_IFE2_FILTER_COMPUTATION_REGISTER3 arg0;
	T_IFE2_FILTER_COMPUTATION_REGISTER4 arg1;
	T_IFE2_FILTER_COMPUTATION_REGISTER5 arg2;

	arg0.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER3_OFS);
	arg1.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER4_OFS);
	arg2.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER5_OFS);

	arg0.bit.ife2_u_fth0 = p_th_u[0];
	arg0.bit.ife2_u_fth1 = p_th_u[1];
	arg0.bit.ife2_u_fth2 = p_th_u[2];
	arg0.bit.ife2_u_fth3 = p_th_u[3];
	arg1.bit.ife2_u_fth4 = p_th_u[4];

	arg2.bit.ife2_u_fwt0 = p_wet_u[0];
	arg2.bit.ife2_u_fwt1 = p_wet_u[1];
	arg2.bit.ife2_u_fwt2 = p_wet_u[2];
	arg2.bit.ife2_u_fwt3 = p_wet_u[3];
	arg2.bit.ife2_u_fwt4 = p_wet_u[4];
	arg2.bit.ife2_u_fwt5 = p_wet_u[5];

	IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER3_OFS, arg0.reg);
	IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER4_OFS, arg1.reg);
	IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER5_OFS, arg2.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_filter_computation_param_v_reg(UINT32 *p_th_v, UINT32 *p_wet_v)
{
	T_IFE2_FILTER_COMPUTATION_REGISTER6 arg0;
	T_IFE2_FILTER_COMPUTATION_REGISTER7 arg1;
	T_IFE2_FILTER_COMPUTATION_REGISTER8 arg2;

	arg0.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER6_OFS);
	arg1.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER7_OFS);
	arg2.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER8_OFS);

	arg0.bit.ife2_v_fth0 = p_th_v[0];
	arg0.bit.ife2_v_fth1 = p_th_v[1];
	arg0.bit.ife2_v_fth2 = p_th_v[2];
	arg0.bit.ife2_v_fth3 = p_th_v[3];
	arg1.bit.ife2_v_fth4 = p_th_v[4];

	arg2.bit.ife2_v_fwt0 = p_wet_v[0];
	arg2.bit.ife2_v_fwt1 = p_wet_v[1];
	arg2.bit.ife2_v_fwt2 = p_wet_v[2];
	arg2.bit.ife2_v_fwt3 = p_wet_v[3];
	arg2.bit.ife2_v_fwt4 = p_wet_v[4];
	arg2.bit.ife2_v_fwt5 = p_wet_v[5];

	IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER6_OFS, arg0.reg);
	IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER7_OFS, arg1.reg);
	IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER8_OFS, arg2.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_statistical_information_threshold_reg(UINT32 u_th0, UINT32 u_th1, UINT32 v_th0, UINT32 v_th1)
{
	T_IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0 arg_u;
	T_IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0 arg_v;

	arg_u.reg = IFE2_GETREG(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS);
	arg_v.reg = IFE2_GETREG(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS);

	arg_u.bit.ife2_stl_u_th0 = u_th0;
	arg_u.bit.ife2_stl_u_th1 = u_th1;

	arg_v.bit.ife2_stl_v_th0 = v_th0;
	arg_v.bit.ife2_stl_v_th1 = v_th1;

	IFE2_SETREG(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS, arg_u.reg);
	IFE2_SETREG(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS, arg_v.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_statistical_information_sum_u_reg(VOID)
{
	T_IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1 arg0;
	T_IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2 arg1;

	arg0.reg = IFE2_GETREG(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1_OFS);
	arg1.reg = IFE2_GETREG(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2_OFS);

	return ((arg1.bit.ife2_stl_u_sum1 << 19) + arg0.bit.ife2_stl_u_sum0);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_statistical_information_sum_v_reg(VOID)
{
	T_IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1 arg0;
	T_IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2 arg1;

	arg0.reg = IFE2_GETREG(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1_OFS);
	arg1.reg = IFE2_GETREG(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2_OFS);

	return ((arg1.bit.ife2_stl_v_sum1 << 19) + arg0.bit.ife2_stl_v_sum0);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_statistical_information_cnt_reg(VOID)
{
	T_IFE2_GRAY_STATISTICAL_INFORMATION_COUNT_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_GRAY_STATISTICAL_INFORMATION_COUNT_REGISTER_OFS);

	return arg.reg;
}
//-----------------------------------------------------------------------

UINT32 ife2_get_base_addr_reg(VOID)
{
	UINT32 addr = 0x0;

#if (defined(_NVT_EMULATION_) == ON)
	{
		addr = _IFE2_REG_BASE_ADDR;
	}
#else
	{
		addr = 0x0;
	}
#endif


	return addr;
}
//-----------------------------------------------------------------------

VOID ife2_set_debug_mode_reg(UINT32 edge_en, UINT32 rc_en, UINT32 eng_en)
{
	T_DEBUG_REGISTER0 arg;

	arg.reg = IFE2_GETREG(DEBUG_REGISTER0_OFS);

	arg.bit.ife2_egd_en = edge_en;
	arg.bit.ife2_rc_en = rc_en;
	arg.bit.ife2_filter_en = eng_en;

	IFE2_SETREG(DEBUG_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

#else

//-----------------------------------------------------------------------
VOID ife2_set_reset_reg(UINT32 op)
{
	T_IFE2_ENGINE_CONTROL_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS);

	arg.bit.ife2_rst = op;
	arg.bit.ife2_start = DISABLE;

	IFE2_SETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_start_reg(UINT32 op)
{
	T_IFE2_ENGINE_CONTROL_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS);

	arg.bit.ife2_start_aload = 0;
	arg.bit.ife2_frameend_aload = 0;
	arg.bit.ife2_drt_start_load = 0;

	arg.bit.ife2_start = op;

	IFE2_SETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_start_reg(VOID)
{
	T_IFE2_ENGINE_CONTROL_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS);

	return arg.bit.ife2_start;
}
//-----------------------------------------------------------------------

VOID ife2_set_load_reg(UINT32 load_type, UINT32 op)
{
	T_IFE2_ENGINE_CONTROL_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS);

	arg.bit.ife2_rst = 0x0;
	//arg.bit.ife2_start = 0x0;

	switch (load_type) {
	case IFE2_LOADTYPE_START:
		arg.bit.ife2_start_aload = op;
		arg.bit.ife2_frameend_aload = 0x0;
		arg.bit.ife2_drt_start_load = 0x0;
		break;

	case IFE2_LOADTYPE_FMEND:
		arg.bit.ife2_start_aload = 0x0;
		arg.bit.ife2_frameend_aload = op;
		arg.bit.ife2_drt_start_load = 0x0;
		break;

	case IFE2_LOADTYPE_DIRECT_START:
		arg.bit.ife2_start_aload = 0x0;
		arg.bit.ife2_frameend_aload = 0x0;
		arg.bit.ife2_drt_start_load = op;
		break;
	}

	IFE2_SETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_linked_list_fire_reg(UINT32 set_fire)
{
	T_IFE2_ENGINE_CONTROL_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS);

	arg.bit.ife2_ll_fire = set_fire;

	IFE2_SETREG(IFE2_ENGINE_CONTROL_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------


VOID ife2_set_linked_list_terminate_reg(UINT32 set_terminate)
{
	T_IFE2_LINKED_LIST_REGISTER arg;

	arg.reg = IFE2_GETREG(IFE2_LINKED_LIST_REGISTER_OFS);

	arg.bit.ife2_ll_terminate = set_terminate;

	IFE2_SETREG(IFE2_LINKED_LIST_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------
UINT32 ife2_linked_list_dma_addr = 0;
VOID ife2_set_linked_list_dma_addr_reg(UINT32 set_addr)
{
	//T_IFE2LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER3 arg;

	ife2_linked_list_dma_addr = set_addr;

	//arg.reg = IFE2_GETREG(IFE2LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER3_OFS);

	//debug_msg("ll-addr: %08x\r\n", set_addr);

	ifeg2->reg_11.bit.ife2_dram_ll_sai = ife2_platform_va2pa(ife2_linked_list_dma_addr) >> 2;

	//IFE2_SETREG(IFE2LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER3_OFS, arg.reg);
}
//-----------------------------------------------------------------------


VOID ife2_set_burst_length_reg(UINT32 in_len, UINT32 out_len)
{
	//T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	ifeg2->reg_1.bit.ife2_in_brt_lenth = in_len;
	ifeg2->reg_1.bit.ife2_out_brt_lenth = out_len;

	//IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

INT32 ife2_get_burst_length_reg(UINT32 get_sel)
{
	INT32 burst_len;
	T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	if (get_sel == 0) {
		switch (arg.bit.ife2_in_brt_lenth) {
		case 0:
			burst_len = 64;
			break;

		case 1:
			burst_len = 48;
			break;

		//case 2:
		//  burst_len = 96;
		//  break;

		//case 3:
		//  burst_len = 128;
		//  break;

		default:
			burst_len = 64;
			break;
		}
	} else {
		switch (arg.bit.ife2_out_brt_lenth) {
		case 0:
			burst_len = 64;
			break;

		case 1:
			burst_len = 32;
			break;

		//case 2:
		//  burst_len = 96;
		//  break;

		//case 3:
		//  burst_len = 128;
		//  break;

		default:
			burst_len = 64;
			break;
		}
	}

	return burst_len;
}
//-----------------------------------------------------------------------


VOID ife2_set_input_format_reg(UINT32 in_fmt)
{
	//T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	ifeg2->reg_1.bit.ife2_ifmt = in_fmt;

	//IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_output_format_reg(UINT32 out_fmt)
{
	//T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	ifeg2->reg_1.bit.ife2_ofmt = out_fmt;

	//IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_input_format_reg(VOID)
{
	//T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	return ifeg2->reg_1.bit.ife2_ifmt;
}
//-----------------------------------------------------------------------

UINT32 ife2_get_output_format_reg(VOID)
{
	//T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	return ifeg2->reg_1.bit.ife2_ofmt;
}
//-----------------------------------------------------------------------

VOID ife2_set_output_dest_reg(UINT32 out_dest)
{
	//T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	ifeg2->reg_1.bit.ife2_dest = out_dest;

	//IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_output_dram_reg(UINT32 set_en)
{
	//T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	ifeg2->reg_1.bit.ife2_dram_out_en = set_en;

	//IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------


UINT32 ife2_get_output_dest_reg(VOID)
{
	//T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	return ifeg2->reg_1.bit.ife2_dest;
}
//-----------------------------------------------------------------------

VOID ife2_set_filter_ychannel_enable_reg(UINT32 op)
{
	//T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	ifeg2->reg_1.bit.ife2_yftr_en = op;

	//IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_filter_size_reg(UINT32 filter_size, UINT32 edge_size)
{
	//T_IFE2_INPUT_CONTROL_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS);

	ifeg2->reg_1.bit.ife2_fsize_sel = filter_size;
	ifeg2->reg_1.bit.ife2_eksize_sel = edge_size;

	//IFE2_SETREG(IFE2_INPUT_CONTROL_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_interrupt_enable_reg(UINT32 int_sel, UINT32 int_val)
{
	//T_IFE2_INTERRUPT_ENABLE_REGISTER arg;

	//arg.reg = IFE2_GETREG(IFE2_INTERRUPT_ENABLE_REGISTER_OFS);

	switch (int_sel) {

	case IFE2_INTP_LL_END:
		ifeg2->reg_2.bit.ife2_intpe_ll_end = int_val;
		break;

	case IFE2_INTP_LL_ERR:
		ifeg2->reg_2.bit.ife2_intpe_ll_err = int_val;
		break;

	case IFE2_INTP_LL_LATE:
		ifeg2->reg_2.bit.ife2_intpe_ll_red_late = int_val;
		break;

	case IFE2_INTP_LL_JEND:
		ifeg2->reg_2.bit.ife2_intpe_ll_job_end = int_val;
		break;

	case IFE2_INTP_OUT_OVF:
		ifeg2->reg_2.bit.ife2_intpe_out_ovf = int_val;
		break;

	case IFE2_INTP_FRM_END:
		ifeg2->reg_2.bit.ife2_intpe_frm_end = int_val;
		break;

	case IFE2_INTP_ALL:
		ifeg2->reg_2.word = int_val;
		break;

	default:
		ifeg2->reg_2.word = int_val;
		break;
	}

	//IFE2_SETREG(IFE2_INTERRUPT_ENABLE_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_interrupt_status_reg(UINT32 int_sel, UINT32 int_val)
{
	//T_IFE2_INTERRUPT_STATUS_REGISTER arg;

	//arg.reg = IFE2_GETREG(IFE2_INTERRUPT_STATUS_REGISTER_OFS);

	switch (int_sel) {
	case IFE2_INTP_LL_END:
		ifeg2->reg_3.bit.ife2_intps_ll_end = int_val;
		break;

	case IFE2_INTP_LL_ERR:
		ifeg2->reg_3.bit.ife2_intps_ll_err = int_val;
		break;

	case IFE2_INTP_LL_LATE:
		ifeg2->reg_3.bit.ife2_intps_ll_red_late = int_val;
		break;

	case IFE2_INTP_LL_JEND:
		ifeg2->reg_3.bit.ife2_intps_ll_job_end = int_val;
		break;

	case IFE2_INTP_OUT_OVF:
		ifeg2->reg_3.bit.ife2_intps_out_ovf = int_val;
		break;

	case IFE2_INTP_FRM_END:
		ifeg2->reg_3.bit.ife2_intps_frm_end = int_val;
		break;

	case IFE2_INTP_ALL:
		ifeg2->reg_3.word = int_val;
		break;

	default:
		ifeg2->reg_3.word = int_val;
		break;
	}

	//IFE2_SETREG(IFE2_INTERRUPT_STATUS_REGISTER_OFS, arg.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_interrupt_status_reg(VOID)
{
	//T_IFE2_INTERRUPT_STATUS_REGISTER arg;

	//arg.reg = IFE2_GETREG(IFE2_INTERRUPT_STATUS_REGISTER_OFS);

	return ifeg2->reg_3.word;
}
//-----------------------------------------------------------------------

UINT32 ife2_get_interrupt_enable_reg(VOID)
{
	//T_IFE2_INTERRUPT_ENABLE_REGISTER arg;

	//arg.reg = IFE2_GETREG(IFE2_INTERRUPT_ENABLE_REGISTER_OFS);

	return ifeg2->reg_2.word;
}
//-----------------------------------------------------------------------


VOID ife2_set_image_size_reg(UINT32 size_h, UINT32 size_v)
{
	//T_IFE2_INPUT_IMAGE_SIZE_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_IMAGE_SIZE_REGISTER0_OFS);

	ifeg2->reg_5.bit.ife2_h_size = size_h;
	ifeg2->reg_5.bit.ife2_v_size = size_v;

	//IFE2_SETREG(IFE2_INPUT_IMAGE_SIZE_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_img_size_h_reg(VOID)
{
	//T_IFE2_INPUT_IMAGE_SIZE_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_IMAGE_SIZE_REGISTER0_OFS);

	return ifeg2->reg_5.bit.ife2_h_size;
}
//-----------------------------------------------------------------------

UINT32 ife2_get_img_size_v_reg(VOID)
{
	//T_IFE2_INPUT_IMAGE_SIZE_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_INPUT_IMAGE_SIZE_REGISTER0_OFS);

	return ifeg2->reg_5.bit.ife2_v_size;
}
//-----------------------------------------------------------------------


VOID ife2_set_filter_enable_reg(UINT32 op)
{
	//T_DEBUG_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(DEBUG_REGISTER0_OFS);

	ifeg2->reg_6.bit.ife2_filter_en = op;

	//IFE2_SETREG(DEBUG_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_input_lineoffset_reg(UINT32 lofs_y, UINT32 lofs_uv)
{
	//T_IFE2_DMA_INPUT_LINEOFFSET_REGISTER0 arg_y;
	//T_IFE2_INPUT_DMA_INPUT_LINEOFFSET_REGISTER1 argUV;

	//arg_y.reg = IFE2_GETREG(IFE2_DMA_INPUT_LINEOFFSET_REGISTER0_OFS);
	//argUV.reg = IFE2_GETREG(IFE2_INPUT_DMA_INPUT_LINEOFFSET_REGISTER1_OFS);

	ifeg2->reg_7.bit.ife2_dram_y_ofsi = (lofs_y >> 2);
	//argUV.bit.IFE2_DRAM_UV_OFSI = (lofs_uv >> 2);

	//IFE2_SETREG(IFE2_DMA_INPUT_LINEOFFSET_REGISTER0_OFS, arg_y.reg);
	//IFE2_SETREG(IFE2_INPUT_DMA_INPUT_LINEOFFSET_REGISTER1_OFS, argUV.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_input_lineoffset_y_reg(VOID)
{
	//T_IFE2_DMA_INPUT_LINEOFFSET_REGISTER0 arg_y;

	//arg_y.reg = IFE2_GETREG(IFE2_DMA_INPUT_LINEOFFSET_REGISTER0_OFS);

	return ifeg2->reg_7.word;
}
//-----------------------------------------------------------------------

UINT32 ife2_input_dma_addr_y = 0x0;
VOID ife2_set_input_dma_addr_reg(UINT32 addr_y, UINT32 addr_uv)
{
	//T_IFE2_INPUT_DMA_ADDRESS_REGISTER0 arg_y;
	//T_IFE2_INPUT_DMA_CHANNEL_REGISTER1 argUV;

	//arg_y.reg = IFE2_GETREG(IFE2_INPUT_DMA_ADDRESS_REGISTER0_OFS);

	ife2_input_dma_addr_y = addr_y;

#if 1
	ifeg2->reg_8.bit.ife2_dram_y_sai = ife2_platform_va2pa(addr_y);
#else
#ifdef __KERNEL__

	ifeg2->reg_8.bit.ife2_dram_y_sai = fmem_lookup_pa(addr_y);

#else

	//argUV.reg = IFE2_GETREG(IFE2_INPUT_DMA_CHANNEL_REGISTER1_OFS);

	ifeg2->reg_8.bit.ife2_dram_y_sai = dma_getPhyAddr(addr_y);
#endif
#endif
	//argUV.bit.IFE2_DRAM_UV_SAI = addr_uv;

	//IFE2_SETREG(IFE2_INPUT_DMA_ADDRESS_REGISTER0_OFS, arg_y.reg);
	//IFE2_SETREG(IFE2_INPUT_DMA_CHANNEL_REGISTER1_OFS, argUV.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_input_dmaaddress_y_reg(VOID)
{
	return ife2_input_dma_addr_y;
}
//-----------------------------------------------------------------------

VOID ife2_set_output_lineoffset_reg(UINT32 lofs_y, UINT32 lofs_uv)
{
	//T_IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0 arg_y;
	//T_IFE2_OUTPUT_DMA_OUTPUT_LINEOFFSET_REGISTER1 argUV;

	//arg_y.reg = IFE2_GETREG(IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0_OFS);
	//argUV.reg = IFE2_GETREG(IFE2_OUTPUT_DMA_OUTPUT_LINEOFFSET_REGISTER1_OFS);

	ifeg2->reg_12.bit.ife2_dram_y_ofso = (lofs_y >> 2);
	//argUV.bit.IFE2_DRAM_UV_OFSO = (lofs_uv >> 2);

	//IFE2_SETREG(IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0_OFS, arg_y.reg);
	//IFE2_SETREG(IFE2_OUTPUT_DMA_OUTPUT_LINEOFFSET_REGISTER1_OFS, argUV.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_output_lineoffset_y_reg(VOID)
{
	//T_IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0 arg_y;

	//arg_y.reg = IFE2_GETREG(IFE2_DMA_OUTPUT_LINEOFFSET_REGISTER0_OFS);

	return ifeg2->reg_12.word;
}
//-----------------------------------------------------------------------

UINT32 ife2_get_output_lineoffset_uv_reg(VOID)
{
#if 0
	//T_IFE2_OUTPUT_DMA_OUTPUT_LINEOFFSET_REGISTER1 argUV;

	//argUV.reg = IFE2_GETREG(IFE2_OUTPUT_DMA_OUTPUT_LINEOFFSET_REGISTER1_OFS);

	return argUV.reg;
#else
	return 0;
#endif
}
//-----------------------------------------------------------------------

UINT32 ife2_output_dma_addr_y = 0x0;
VOID ife2_set_output_dma_addr0_reg(UINT32 addr_y, UINT32 addr_uv)
{
	//T_IFE2_OUTPUT_DMA_ADDRESS_REGISTER0 arg_y;
	//T_IFE2_OUTPUT_DMA_CHANNEL_REGISTER1 argUV;

	//arg_y.reg = IFE2_GETREG(IFE2_OUTPUT_DMA_ADDRESS_REGISTER0_OFS);
	//argUV.reg = IFE2_GETREG(IFE2_OUTPUT_DMA_CHANNEL_REGISTER1_OFS);

	ife2_output_dma_addr_y = addr_y;

#if 1
	ifeg2->reg_13.bit.ife2_dram_y_sao = ife2_platform_va2pa(addr_y);
#else
#ifdef __KERNEL__

	ifeg2->reg_13.bit.ife2_dram_y_sao = fmem_lookup_pa(addr_y);

#else
	ifeg2->reg_13.bit.ife2_dram_y_sao = dma_getPhyAddr(addr_y);
	//argUV.bit.IFE2_DRAM_UV_SAO0 = addr_uv;
#endif
#endif

	//IFE2_SETREG(IFE2_OUTPUT_DMA_ADDRESS_REGISTER0_OFS, arg_y.reg);
	//IFE2_SETREG(IFE2_OUTPUT_DMA_CHANNEL_REGISTER1_OFS, argUV.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_output_dma_addr_y0_reg(VOID)
{
	return ife2_output_dma_addr_y;
}
//-----------------------------------------------------------------------


VOID ife2_set_reference_center_cal_y_reg(UINT32 *p_range_th_y, UINT32 wet_y, UINT32 *p_range_wet_y, UINT32 outlier_th_y)
{
	//T_IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER0 arg0;
	//T_IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER1 arg1;

	//arg0.reg = IFE2_GETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER0_OFS);
	//arg1.reg = IFE2_GETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER1_OFS);

	ifeg2->reg_17.bit.ife2_y_rcth0 = p_range_th_y[0];
	ifeg2->reg_17.bit.ife2_y_rcth1 = p_range_th_y[1];
	ifeg2->reg_17.bit.ife2_y_rcth2 = p_range_th_y[2];
	ifeg2->reg_17.bit.ife2_y_cwt = wet_y;

	ifeg2->reg_18.bit.ife2_y_rcwt0 = p_range_wet_y[0];
	ifeg2->reg_18.bit.ife2_y_rcwt1 = p_range_wet_y[1];
	ifeg2->reg_18.bit.ife2_y_rcwt2 = p_range_wet_y[2];
	ifeg2->reg_18.bit.ife2_y_rcwt3 = p_range_wet_y[3];
	ifeg2->reg_18.bit.ife2_y_outl_th = outlier_th_y - 1;

	//IFE2_SETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER0_OFS, arg0.reg);
	//IFE2_SETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER1_OFS, arg1.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_reference_center_cal_uv_reg(UINT32 *p_range_th_uv, UINT32 wet_uv, UINT32 *p_range_wet_uv, UINT32 outlier_th_uv)
{
	//T_IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER2 arg0;
	//T_IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER3 arg1;

	//arg0.reg = IFE2_GETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER2_OFS);
	//arg1.reg = IFE2_GETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER3_OFS);

	ifeg2->reg_19.bit.ife2_uv_rcth0 = p_range_th_uv[0];
	ifeg2->reg_19.bit.ife2_uv_rcth1 = p_range_th_uv[1];
	ifeg2->reg_19.bit.ife2_uv_rcth2 = p_range_th_uv[2];
	ifeg2->reg_19.bit.ife2_uv_cwt = wet_uv;

	ifeg2->reg_20.bit.ife2_uv_rcwt0 = p_range_wet_uv[0];
	ifeg2->reg_20.bit.ife2_uv_rcwt1 = p_range_wet_uv[1];
	ifeg2->reg_20.bit.ife2_uv_rcwt2 = p_range_wet_uv[2];
	ifeg2->reg_20.bit.ife2_uv_rcwt3 = p_range_wet_uv[3];
	ifeg2->reg_20.bit.ife2_uv_outl_th = outlier_th_uv - 1;

	//IFE2_SETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER2_OFS, arg0.reg);
	//IFE2_SETREG(IFE2_REFERENCE_CENTER_COMPUTATION_REGISTER3_OFS, arg1.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_outlier_difference_threshold_reg(UINT32 dif_th_y, UINT32 dif_th_u, UINT32 dif_th_v)
{
	//T_IFE2_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0_OFS);

	ifeg2->reg_21.bit.ife2_y_outl_dth = dif_th_y;
	ifeg2->reg_21.bit.ife2_u_outl_dth = dif_th_u;
	ifeg2->reg_21.bit.ife2_v_outl_dth = dif_th_v;

	//IFE2_SETREG(IFE2_REFERENCE_CENTER_OUTLIER_DIFFERENCE_THRESHOLD_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_edge_direction_threshold_reg(UINT32 pn_th, UINT32 hv_th)
{
	//T_IFE2_EDGE_DIRECTION_THRESHOLD_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(IFE2_EDGE_DIRECTION_THRESHOLD_REGISTER0_OFS);

	ifeg2->reg_22.bit.ife2_ed_pn_th = pn_th;
	ifeg2->reg_22.bit.ife2_ed_hv_th = hv_th;

	//IFE2_SETREG(IFE2_EDGE_DIRECTION_THRESHOLD_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_filter_computation_param_y_reg(UINT32 *p_th_y, UINT32 *p_wet_y)
{
	//T_IFE2_FILTER_COMPUTATION_REGISTER0 arg0;
	//T_IFE2_FILTER_COMPUTATION_REGISTER1 arg1;
	//T_IFE2_FILTER_COMPUTATION_REGISTER2 arg2;

	//arg0.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER0_OFS);
	//arg1.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER1_OFS);
	//arg2.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER2_OFS);

	ifeg2->reg_23.bit.ife2_y_fth0 = p_th_y[0];
	ifeg2->reg_23.bit.ife2_y_fth1 = p_th_y[1];
	ifeg2->reg_23.bit.ife2_y_fth2 = p_th_y[2];
	ifeg2->reg_23.bit.ife2_y_fth3 = p_th_y[3];
	ifeg2->reg_24.bit.ife2_y_fth4 = p_th_y[4];

	ifeg2->reg_25.bit.ife2_y_fwt0 = p_wet_y[0];
	ifeg2->reg_25.bit.ife2_y_fwt1 = p_wet_y[1];
	ifeg2->reg_25.bit.ife2_y_fwt2 = p_wet_y[2];
	ifeg2->reg_25.bit.ife2_y_fwt3 = p_wet_y[3];
	ifeg2->reg_25.bit.ife2_y_fwt4 = p_wet_y[4];
	ifeg2->reg_25.bit.ife2_y_fwt5 = p_wet_y[5];

	//IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER0_OFS, arg0.reg);
	//IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER1_OFS, arg1.reg);
	//IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER2_OFS, arg2.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_filter_computation_param_u_reg(UINT32 *p_th_u, UINT32 *p_wet_u)
{
	//T_IFE2_FILTER_COMPUTATION_REGISTER3 arg0;
	//T_IFE2_FILTER_COMPUTATION_REGISTER4 arg1;
	//T_IFE2_FILTER_COMPUTATION_REGISTER5 arg2;

	//arg0.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER3_OFS);
	//arg1.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER4_OFS);
	//arg2.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER5_OFS);

	ifeg2->reg_26.bit.ife2_u_fth0 = p_th_u[0];
	ifeg2->reg_26.bit.ife2_u_fth1 = p_th_u[1];
	ifeg2->reg_26.bit.ife2_u_fth2 = p_th_u[2];
	ifeg2->reg_26.bit.ife2_u_fth3 = p_th_u[3];
	ifeg2->reg_27.bit.ife2_u_fth4 = p_th_u[4];

	ifeg2->reg_28.bit.ife2_u_fwt0 = p_wet_u[0];
	ifeg2->reg_28.bit.ife2_u_fwt1 = p_wet_u[1];
	ifeg2->reg_28.bit.ife2_u_fwt2 = p_wet_u[2];
	ifeg2->reg_28.bit.ife2_u_fwt3 = p_wet_u[3];
	ifeg2->reg_28.bit.ife2_u_fwt4 = p_wet_u[4];
	ifeg2->reg_28.bit.ife2_u_fwt5 = p_wet_u[5];

	//IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER3_OFS, arg0.reg);
	//IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER4_OFS, arg1.reg);
	//IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER5_OFS, arg2.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_filter_computation_param_v_reg(UINT32 *p_th_v, UINT32 *p_wet_v)
{
	//T_IFE2_FILTER_COMPUTATION_REGISTER6 arg0;
	//T_IFE2_FILTER_COMPUTATION_REGISTER7 arg1;
	//T_IFE2_FILTER_COMPUTATION_REGISTER8 arg2;

	//arg0.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER6_OFS);
	//arg1.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER7_OFS);
	//arg2.reg = IFE2_GETREG(IFE2_FILTER_COMPUTATION_REGISTER8_OFS);

	ifeg2->reg_29.bit.ife2_v_fth0 = p_th_v[0];
	ifeg2->reg_29.bit.ife2_v_fth1 = p_th_v[1];
	ifeg2->reg_29.bit.ife2_v_fth2 = p_th_v[2];
	ifeg2->reg_29.bit.ife2_v_fth3 = p_th_v[3];
	ifeg2->reg_30.bit.ife2_v_fth4 = p_th_v[4];

	ifeg2->reg_31.bit.ife2_v_fwt0 = p_wet_v[0];
	ifeg2->reg_31.bit.ife2_v_fwt1 = p_wet_v[1];
	ifeg2->reg_31.bit.ife2_v_fwt2 = p_wet_v[2];
	ifeg2->reg_31.bit.ife2_v_fwt3 = p_wet_v[3];
	ifeg2->reg_31.bit.ife2_v_fwt4 = p_wet_v[4];
	ifeg2->reg_31.bit.ife2_v_fwt5 = p_wet_v[5];

	//IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER6_OFS, arg0.reg);
	//IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER7_OFS, arg1.reg);
	//IFE2_SETREG(IFE2_FILTER_COMPUTATION_REGISTER8_OFS, arg2.reg);
}
//-----------------------------------------------------------------------

VOID ife2_set_statistical_information_threshold_reg(UINT32 u_th0, UINT32 u_th1, UINT32 v_th0, UINT32 v_th1)
{
	//T_IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0 arg_u;
	//T_IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0 arg_v;

	//arg_u.reg = IFE2_GETREG(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS);
	//arg_v.reg = IFE2_GETREG(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS);

	ifeg2->reg_32.bit.ife2_stl_u_th0 = u_th0;
	ifeg2->reg_32.bit.ife2_stl_u_th1 = u_th1;

	ifeg2->reg_35.bit.ife2_stl_v_th0 = v_th0;
	ifeg2->reg_35.bit.ife2_stl_v_th1 = v_th1;

	//IFE2_SETREG(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS, arg_u.reg);
	//IFE2_SETREG(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER0_OFS, arg_v.reg);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_statistical_information_sum_u_reg(VOID)
{
	//T_IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1 arg0;
	//T_IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2 arg1;

	//arg0.reg = IFE2_GETREG(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1_OFS);
	//arg1.reg = IFE2_GETREG(IFE2_U_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2_OFS);

	return ((ifeg2->reg_34.bit.ife2_stl_u_sum1 << 19) + ifeg2->reg_33.bit.ife2_stl_u_sum0);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_statistical_information_sum_v_reg(VOID)
{
	//T_IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1 arg0;
	//T_IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2 arg1;

	//arg0.reg = IFE2_GETREG(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER1_OFS);
	//arg1.reg = IFE2_GETREG(IFE2_V_CHANNEL_GRAY_STATISTICAL_INFORMATION_REGISTER2_OFS);

	return ((ifeg2->reg_37.bit.ife2_stl_v_sum1 << 19) + ifeg2->reg_36.bit.ife2_stl_v_sum0);
}
//-----------------------------------------------------------------------

UINT32 ife2_get_statistical_information_cnt_reg(VOID)
{
	//T_IFE2_GRAY_STATISTICAL_INFORMATION_COUNT_REGISTER arg;

	//arg.reg = IFE2_GETREG(IFE2_GRAY_STATISTICAL_INFORMATION_COUNT_REGISTER_OFS);

	return ifeg2->reg_38.bit.ife2_stl_cnt;
}
//-----------------------------------------------------------------------

UINT32 ife2_get_base_addr_reg(VOID)
{
	UINT32 addr = 0x0;

#if (defined(_NVT_EMULATION_) == ON)
	{
		addr = _IFE2_REG_BASE_ADDR;
	}
#else
	{
		addr = 0x0;
	}
#endif


	return addr;
}
//-----------------------------------------------------------------------

VOID ife2_set_debug_mode_reg(UINT32 edge_en, UINT32 rc_en, UINT32 eng_en)
{
	//T_DEBUG_REGISTER0 arg;

	//arg.reg = IFE2_GETREG(DEBUG_REGISTER0_OFS);

	ifeg2->reg_6.bit.ife2_egd_en = edge_en;
	ifeg2->reg_6.bit.ife2_rc_en = rc_en;
	ifeg2->reg_6.bit.ife2_filter_en = eng_en;

	//IFE2_SETREG(DEBUG_REGISTER0_OFS, arg.reg);
}
//-----------------------------------------------------------------------


#endif


