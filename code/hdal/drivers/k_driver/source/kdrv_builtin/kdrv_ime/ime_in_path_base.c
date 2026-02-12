
/*
    IME module driver

    NT96510 IME module driver.

    @file       ime_in_path_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/

/*
#ifdef __KERNEL__

#include "kwrap/type.h"
#include <plat-na51055/nvt-sramctl.h>
//#include <frammap/frammap_if.h>
#include <mach/fmem.h>

#include "ime_dbg.h"

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "Type.h"
#include "ErrorNo.h"

#endif  // end #ifdef __KERNEL__
*/


#include "ime_reg.h"
#include "ime_int_public.h"
#include "ime_in_path_base.h"


UINT32 ime_in_y_addr = 0, ime_in_u_addr = 0, ime_in_v_addr = 0;

#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

VOID ime_set_input_source_reg(UINT32 in_src_sel)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 mode;

	mode.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	mode.bit.ime_src = in_src_sel;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, mode.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_input_image_format_reg(UINT32 in_fmt_sel)
{
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 mode;

	mode.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS);

	mode.bit.ime_imat = in_fmt_sel;

	IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS, mode.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_stripe_size_mode_reg(UINT32 set_mode)
{
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 arg;

	arg.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS);

	arg.bit.ime_st_size_mode = set_mode;

	IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_horizontal_fixed_stripe_size_reg(UINT32 set_hn, UINT32 set_hl)
{
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1 arg;

	arg.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS);

	arg.bit.st_hn = set_hn;
	arg.bit.st_hl = set_hl;

	IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_horizontal_stripe_number_reg(UINT32 set_hm)
{
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1 arg;

	arg.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS);

	arg.bit.st_hm = set_hm;

	IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_horizontal_varied_stripe_size_reg(UINT32 *p_set_hns)
{
	T_IME_VARIED_STRIPE_SIZE_REGISTER0 arg0;
	T_IME_VARIED_STRIPE_SIZE_REGISTER1 arg1;
	T_IME_VARIED_STRIPE_SIZE_REGISTER2 arg2;

	arg0.reg = IME_GETREG(IME_VARIED_STRIPE_SIZE_REGISTER0_OFS);
	arg1.reg = IME_GETREG(IME_VARIED_STRIPE_SIZE_REGISTER1_OFS);
	arg2.reg = IME_GETREG(IME_VARIED_STRIPE_SIZE_REGISTER2_OFS);

	arg0.bit.ime_st_hn0 = ((p_set_hns[0] >> 2) & 0x3FF);
	arg0.bit.ime_st_hn1 = ((p_set_hns[1] >> 2) & 0x3FF);
	arg0.bit.ime_st_hn2 = ((p_set_hns[2] >> 2) & 0x3FF);

	arg1.bit.ime_st_hn3 = ((p_set_hns[3] >> 2) & 0x3FF);
	arg1.bit.ime_st_hn4 = ((p_set_hns[4] >> 2) & 0x3FF);
	arg1.bit.ime_st_hn5 = ((p_set_hns[5] >> 2) & 0x3FF);

	arg2.bit.ime_st_hn6 = ((p_set_hns[6] >> 2) & 0x3FF);
	arg2.bit.ime_st_hn7 = ((p_set_hns[7] >> 2) & 0x3FF);

	arg2.bit.ime_st_msb_hn0 = (((p_set_hns[0] >> 2) >> 10) & 0x1);
	arg2.bit.ime_st_msb_hn1 = (((p_set_hns[1] >> 2) >> 10) & 0x1);
	arg2.bit.ime_st_msb_hn2 = (((p_set_hns[2] >> 2) >> 10) & 0x1);
	arg2.bit.ime_st_msb_hn3 = (((p_set_hns[3] >> 2) >> 10) & 0x1);
	arg2.bit.ime_st_msb_hn4 = (((p_set_hns[4] >> 2) >> 10) & 0x1);
	arg2.bit.ime_st_msb_hn5 = (((p_set_hns[5] >> 2) >> 10) & 0x1);
	arg2.bit.ime_st_msb_hn6 = (((p_set_hns[6] >> 2) >> 10) & 0x1);
	arg2.bit.ime_st_msb_hn7 = (((p_set_hns[7] >> 2) >> 10) & 0x1);

	IME_SETREG(IME_VARIED_STRIPE_SIZE_REGISTER0_OFS, arg0.reg);
	IME_SETREG(IME_VARIED_STRIPE_SIZE_REGISTER1_OFS, arg1.reg);
	IME_SETREG(IME_VARIED_STRIPE_SIZE_REGISTER2_OFS, arg2.reg);

}
//-------------------------------------------------------------------------------


VOID ime_set_horizontal_overlap_reg(UINT32 h_overlap_sel, UINT32 h_overlap, UINT32 h_prt_sel, UINT32 h_prt_size)
{
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 arg0;
	T_IME_INPUT_STRIPE_VERTICAL_DIMENSION arg1;

	arg0.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS);
	arg1.reg = IME_GETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS);

	arg0.bit.st_hovlp_sel = h_overlap_sel;
	arg0.bit.st_prt_sel = h_prt_sel;

	arg1.bit.st_hovlp = h_overlap - 1;
	arg1.bit.st_prt = h_prt_size - 1;

	IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS, arg0.reg);
	IME_SETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_vertical_stripe_info_reg(UINT32 set_vn, UINT32 set_vl, UINT32 set_vm)
{
	T_IME_INPUT_STRIPE_VERTICAL_DIMENSION arg;

	arg.reg = IME_GETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS);

	//arg.bit.ST_VN = set_vn;
	arg.bit.st_vl = set_vl;
	//arg.bit.ST_VM = set_vm;

	IME_SETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_horizontal_stripe_info_reg(UINT32 *p_get_hn, UINT32 *p_get_hl, UINT32 *p_get_hm)
{
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1 arg;

	arg.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS);

	*p_get_hn = (arg.bit.st_hn << 2);
	*p_get_hl = (arg.bit.st_hl << 2);
	*p_get_hm = (arg.bit.st_hm + 1);
}
//-------------------------------------------------------------------------------

VOID ime_get_vertical_stripe_info_reg(UINT32 *p_get_vn, UINT32 *p_get_vl, UINT32 *p_get_vm)
{
	T_IME_INPUT_STRIPE_VERTICAL_DIMENSION arg;

	arg.reg = IME_GETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS);

	*p_get_vn = 0x0;
	*p_get_vl = arg.bit.st_vl;
	*p_get_vm = 0x1;
}
//-------------------------------------------------------------------------------

VOID ime_get_horizontal_overlap_reg(UINT32 *p_get_h_overlap_sel, UINT32 *p_get_h_overlap)
{
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 arg0;
	T_IME_INPUT_STRIPE_VERTICAL_DIMENSION arg1;

	arg0.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS);
	arg1.reg = IME_GETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS);

	*p_get_h_overlap_sel = arg0.bit.st_hovlp_sel;
	*p_get_h_overlap = (arg1.bit.st_hovlp + 1);
}
//-------------------------------------------------------------------------------


VOID ime_set_input_lineoffset_y_reg(UINT32 lofs)
{
	T_IME_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	arg.bit.dram_y_ofsi = (lofs >> 2);

	IME_SETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_input_lineoffset_c_reg(UINT32 lofs)
{
	T_IME_INPUT_DMA_LINEOFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER1_OFS);

	arg.bit.dram_uv_ofsi = (lofs >> 2);

	IME_SETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_input_channel_addr1_reg(UINT32 addr_y, UINT32 addr_u, UINT32 addr_v)
{
	T_IME_INPUT_DMA_REGISTER0 arg_y;
	T_IME_INPUT_DMA_REGISTER1 arg_u;
	T_IME_INPUT_DMA_REGISTER2 arg_v;

	ime_in_y_addr = addr_y;
	ime_in_u_addr = addr_u;
	ime_in_v_addr = addr_v;

	arg_y.reg  = IME_GETREG(IME_INPUT_DMA_REGISTER0_OFS);
	arg_u.reg = IME_GETREG(IME_INPUT_DMA_REGISTER1_OFS);
	arg_v.reg = IME_GETREG(IME_INPUT_DMA_REGISTER2_OFS);

	arg_y.bit.dram_y_sai0 = ime_platform_va2pa(ime_in_y_addr) >> 2;
	arg_u.bit.dram_u_sai0 = ime_platform_va2pa(ime_in_u_addr) >> 2;
	arg_v.bit.dram_v_sai0 = ime_platform_va2pa(ime_in_v_addr) >> 2;

	IME_SETREG(IME_INPUT_DMA_REGISTER0_OFS, arg_y.reg);
	IME_SETREG(IME_INPUT_DMA_REGISTER1_OFS, arg_u.reg);
	IME_SETREG(IME_INPUT_DMA_REGISTER2_OFS, arg_v.reg);
}
//-------------------------------------------------------------------------------


VOID ime_get_input_channel_addr1_reg(UINT32 *p_get_addr_y, UINT32 *p_get_addr_u, UINT32 *p_get_addr_v)
{
	//T_IME_INPUT_DMA_REGISTER0 addrY;
	//T_IME_INPUT_DMA_REGISTER1 addrCb;
	//T_IME_INPUT_DMA_REGISTER2 addrCr;

	//addrY.reg  = IME_GETREG(IME_INPUT_DMA_REGISTER0_OFS);
	//addrCb.reg = IME_GETREG(IME_INPUT_DMA_REGISTER1_OFS);
	//addrCr.reg = IME_GETREG(IME_INPUT_DMA_REGISTER2_OFS);

	*p_get_addr_y = ime_in_y_addr;//addrY.reg;
	*p_get_addr_u = ime_in_u_addr;//addrCb.reg;
	*p_get_addr_v = ime_in_u_addr;//addrCr.reg;
}
//-------------------------------------------------------------------------------

VOID ime_set_input_image_size_reg(UINT32 set_size_h, UINT32 set_size_v)
{
	T_IME_INPUT_IMAGE_SIZE_REGISTER arg;

	arg.reg = IME_GETREG(IME_INPUT_IMAGE_SIZE_REGISTER_OFS);

	arg.bit.ime_in_h_size = set_size_h >> 2;
	arg.bit.ime_in_v_size = set_size_v >> 2;

	IME_SETREG(IME_INPUT_IMAGE_SIZE_REGISTER_OFS, arg.reg);
}



//################################################################################


UINT32 ime_get_input_lineoffset_y_reg(VOID)
{
	T_IME_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_input_lineoffset_c_reg(VOID)
{
	T_IME_INPUT_DMA_LINEOFFSET_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER1_OFS);

	return arg.reg;
}
//-------------------------------------------------------------------------------

VOID ime_get_input_image_size_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	T_IME_INPUT_IMAGE_SIZE_REGISTER arg;

	arg.reg = IME_GETREG(IME_INPUT_IMAGE_SIZE_REGISTER_OFS);

	*p_get_size_h  = arg.bit.ime_in_h_size << 2;
	*p_get_size_v = arg.bit.ime_in_v_size << 2;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_stripe_mode_reg(VOID)
{
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1 arg;
	//T_IME_INPUT_STRIPE_VERTICAL_DIMEMSION VStripeMode;
	UINT32 stp_mode = 0;

	arg.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS);
	//VStripeMode.reg = IME_GETREG(IME_INPUT_STRIPE_VERTICAL_DIMEMSION_OFS);

	if ((arg.bit.st_hm != 0)) {
		stp_mode = 1;
	}

	return stp_mode;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_input_source_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 mode;

	mode.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return mode.bit.ime_src;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_input_image_format_reg(VOID)
{
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 format;

	format.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS);

	return format.bit.ime_imat;
}
//-------------------------------------------------------------------------------

#else

VOID ime_set_input_source_reg(UINT32 in_src_sel)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 mode;

	//mode.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_src = in_src_sel;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, mode.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_input_image_format_reg(UINT32 in_fmt_sel)
{
	//T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 mode;

	//mode.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS);

	imeg->reg_9.bit.ime_imat = in_fmt_sel;

	//IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS, mode.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_stripe_size_mode_reg(UINT32 set_mode)
{
	//T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 arg;

	//arg.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS);

	imeg->reg_9.bit.ime_st_size_mode = set_mode;

	//IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_horizontal_fixed_stripe_size_reg(UINT32 set_hn, UINT32 set_hl)
{
	//T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1 arg;

	//arg.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS);

	imeg->reg_10.bit.st_hn = set_hn;
	imeg->reg_10.bit.st_hl = set_hl;

	//IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_horizontal_stripe_number_reg(UINT32 set_hm)
{
	//T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1 arg;

	//arg.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS);

	imeg->reg_10.bit.st_hm = set_hm;

	//IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_horizontal_varied_stripe_size_reg(UINT32 *p_set_hns)
{
	//T_IME_VARIED_STRIPE_SIZE_REGISTER0 arg0;
	//T_IME_VARIED_STRIPE_SIZE_REGISTER1 arg1;
	//T_IME_VARIED_STRIPE_SIZE_REGISTER2 arg2;

	//arg0.reg = IME_GETREG(IME_VARIED_STRIPE_SIZE_REGISTER0_OFS);
	//arg1.reg = IME_GETREG(IME_VARIED_STRIPE_SIZE_REGISTER1_OFS);
	//arg2.reg = IME_GETREG(IME_VARIED_STRIPE_SIZE_REGISTER2_OFS);

	imeg->reg_17.bit.ime_st_hn0 = ((p_set_hns[0] >> 2) & 0x3FF);
	imeg->reg_17.bit.ime_st_hn1 = ((p_set_hns[1] >> 2) & 0x3FF);
	imeg->reg_17.bit.ime_st_hn2 = ((p_set_hns[2] >> 2) & 0x3FF);

	imeg->reg_18.bit.ime_st_hn3 = ((p_set_hns[3] >> 2) & 0x3FF);
	imeg->reg_18.bit.ime_st_hn4 = ((p_set_hns[4] >> 2) & 0x3FF);
	imeg->reg_18.bit.ime_st_hn5 = ((p_set_hns[5] >> 2) & 0x3FF);

	imeg->reg_19.bit.ime_st_hn6 = ((p_set_hns[6] >> 2) & 0x3FF);
	imeg->reg_19.bit.ime_st_hn7 = ((p_set_hns[7] >> 2) & 0x3FF);

	imeg->reg_19.bit.ime_st_msb_hn0 = (((p_set_hns[0] >> 2) >> 10) & 0x1);
	imeg->reg_19.bit.ime_st_msb_hn1 = (((p_set_hns[1] >> 2) >> 10) & 0x1);
	imeg->reg_19.bit.ime_st_msb_hn2 = (((p_set_hns[2] >> 2) >> 10) & 0x1);
	imeg->reg_19.bit.ime_st_msb_hn3 = (((p_set_hns[3] >> 2) >> 10) & 0x1);
	imeg->reg_19.bit.ime_st_msb_hn4 = (((p_set_hns[4] >> 2) >> 10) & 0x1);
	imeg->reg_19.bit.ime_st_msb_hn5 = (((p_set_hns[5] >> 2) >> 10) & 0x1);
	imeg->reg_19.bit.ime_st_msb_hn6 = (((p_set_hns[6] >> 2) >> 10) & 0x1);
	imeg->reg_19.bit.ime_st_msb_hn7 = (((p_set_hns[7] >> 2) >> 10) & 0x1);

	//IME_SETREG(IME_VARIED_STRIPE_SIZE_REGISTER0_OFS, arg0.reg);
	//IME_SETREG(IME_VARIED_STRIPE_SIZE_REGISTER1_OFS, arg1.reg);
	//IME_SETREG(IME_VARIED_STRIPE_SIZE_REGISTER2_OFS, arg2.reg);

}
//-------------------------------------------------------------------------------


VOID ime_set_horizontal_overlap_reg(UINT32 h_overlap_sel, UINT32 h_overlap, UINT32 h_prt_sel, UINT32 h_prt_size)
{
	//T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 arg0;
	//T_IME_INPUT_STRIPE_VERTICAL_DIMENSION arg1;

	//arg0.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS);
	//arg1.reg = IME_GETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS);

	imeg->reg_9.bit.st_hovlp_sel = h_overlap_sel;
	imeg->reg_9.bit.st_prt_sel = h_prt_sel;

	imeg->reg_11.bit.st_hovlp = h_overlap - 1;
	imeg->reg_11.bit.st_prt = h_prt_size - 1;

	//IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS, arg0.reg);
	//IME_SETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS, arg1.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_vertical_stripe_info_reg(UINT32 set_vn, UINT32 set_vl, UINT32 set_vm)
{
	//T_IME_INPUT_STRIPE_VERTICAL_DIMENSION arg;

	//arg.reg = IME_GETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS);

	//arg.bit.ST_VN = set_vn;
	imeg->reg_11.bit.st_vl = set_vl;
	//arg.bit.ST_VM = set_vm;

	//IME_SETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_horizontal_stripe_info_reg(UINT32 *p_get_hn, UINT32 *p_get_hl, UINT32 *p_get_hm)
{
	//T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1 arg;

	//arg.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS);

	*p_get_hn = (imeg->reg_10.bit.st_hn << 2);
	*p_get_hl = (imeg->reg_10.bit.st_hl << 2);
	*p_get_hm = (imeg->reg_10.bit.st_hm + 1);
}
//-------------------------------------------------------------------------------

VOID ime_get_vertical_stripe_info_reg(UINT32 *p_get_vn, UINT32 *p_get_vl, UINT32 *p_get_vm)
{
	//T_IME_INPUT_STRIPE_VERTICAL_DIMENSION arg;

	//arg.reg = IME_GETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS);

	*p_get_vn = 0x0;
	*p_get_vl = imeg->reg_11.bit.st_vl;
	*p_get_vm = 0x1;
}
//-------------------------------------------------------------------------------

VOID ime_get_horizontal_overlap_reg(UINT32 *p_get_h_overlap_sel, UINT32 *p_get_h_overlap)
{
	//T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 arg0;
	//T_IME_INPUT_STRIPE_VERTICAL_DIMENSION arg1;

	//arg0.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS);
	//arg1.reg = IME_GETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS);

	*p_get_h_overlap_sel = imeg->reg_9.bit.st_hovlp_sel;
	*p_get_h_overlap = (imeg->reg_11.bit.st_hovlp + 1);
}
//-------------------------------------------------------------------------------


VOID ime_set_input_lineoffset_y_reg(UINT32 lofs)
{
	//T_IME_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	imeg->reg_12.bit.dram_y_ofsi = (lofs >> 2);

	//IME_SETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_input_lineoffset_c_reg(UINT32 lofs)
{
	//T_IME_INPUT_DMA_LINEOFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER1_OFS);

	imeg->reg_13.bit.dram_uv_ofsi = (lofs >> 2);

	//IME_SETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_input_channel_addr1_reg(UINT32 addr_y, UINT32 addr_u, UINT32 addr_v)
{
	//T_IME_INPUT_DMA_REGISTER0 arg_y;
	//T_IME_INPUT_DMA_REGISTER1 arg_u;
	//T_IME_INPUT_DMA_REGISTER2 arg_v;

	ime_in_y_addr = addr_y;
	ime_in_u_addr = addr_u;
	ime_in_v_addr = addr_v;

	//arg_y.reg  = IME_GETREG(IME_INPUT_DMA_REGISTER0_OFS);
	//arg_u.reg = IME_GETREG(IME_INPUT_DMA_REGISTER1_OFS);
	//arg_v.reg = IME_GETREG(IME_INPUT_DMA_REGISTER2_OFS);

	imeg->reg_14.bit.dram_y_sai0 = ime_platform_va2pa(ime_in_y_addr) >> 2;
	imeg->reg_15.bit.dram_u_sai0 = ime_platform_va2pa(ime_in_u_addr) >> 2;
	imeg->reg_16.bit.dram_v_sai0 = ime_platform_va2pa(ime_in_v_addr) >> 2;

	//IME_SETREG(IME_INPUT_DMA_REGISTER0_OFS, arg_y.reg);
	//IME_SETREG(IME_INPUT_DMA_REGISTER1_OFS, arg_u.reg);
	//IME_SETREG(IME_INPUT_DMA_REGISTER2_OFS, arg_v.reg);
}
//-------------------------------------------------------------------------------


VOID ime_get_input_channel_addr1_reg(UINT32 *p_get_addr_y, UINT32 *p_get_addr_u, UINT32 *p_get_addr_v)
{
	//T_IME_INPUT_DMA_REGISTER0 addrY;
	//T_IME_INPUT_DMA_REGISTER1 addrCb;
	//T_IME_INPUT_DMA_REGISTER2 addrCr;

	//addrY.reg  = IME_GETREG(IME_INPUT_DMA_REGISTER0_OFS);
	//addrCb.reg = IME_GETREG(IME_INPUT_DMA_REGISTER1_OFS);
	//addrCr.reg = IME_GETREG(IME_INPUT_DMA_REGISTER2_OFS);

	*p_get_addr_y = ime_in_y_addr;//addrY.reg;
	*p_get_addr_u = ime_in_u_addr;//addrCb.reg;
	*p_get_addr_v = ime_in_u_addr;//addrCr.reg;
}
//-------------------------------------------------------------------------------

VOID ime_set_input_image_size_reg(UINT32 set_size_h, UINT32 set_size_v)
{
	//T_IME_INPUT_IMAGE_SIZE_REGISTER arg;

	//arg.reg = IME_GETREG(IME_INPUT_IMAGE_SIZE_REGISTER_OFS);

	imeg->reg_8.bit.ime_in_h_size = set_size_h >> 2;
	imeg->reg_8.bit.ime_in_v_size = set_size_v >> 2;

	//IME_SETREG(IME_INPUT_IMAGE_SIZE_REGISTER_OFS, arg.reg);
}



//################################################################################


UINT32 ime_get_input_lineoffset_y_reg(VOID)
{
	//T_IME_INPUT_DMA_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER0_OFS);

	return imeg->reg_12.word;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_input_lineoffset_c_reg(VOID)
{
	//T_IME_INPUT_DMA_LINEOFFSET_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_INPUT_DMA_LINEOFFSET_REGISTER1_OFS);

	return imeg->reg_13.word;
}
//-------------------------------------------------------------------------------

VOID ime_get_input_image_size_reg(UINT32 *p_get_size_h, UINT32 *p_get_size_v)
{
	//T_IME_INPUT_IMAGE_SIZE_REGISTER arg;

	//arg.reg = IME_GETREG(IME_INPUT_IMAGE_SIZE_REGISTER_OFS);

	*p_get_size_h = imeg->reg_8.bit.ime_in_h_size << 2;
	*p_get_size_v = imeg->reg_8.bit.ime_in_v_size << 2;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_stripe_mode_reg(VOID)
{
	//T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1 arg;
	//T_IME_INPUT_STRIPE_VERTICAL_DIMEMSION VStripeMode;
	UINT32 stp_mode = 0;

	//arg.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS);
	//VStripeMode.reg = IME_GETREG(IME_INPUT_STRIPE_VERTICAL_DIMEMSION_OFS);

	if ((imeg->reg_10.bit.st_hm != 0)) {
		stp_mode = 1;
	}

	return stp_mode;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_input_source_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 mode;

	//mode.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return imeg->reg_1.bit.ime_src;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_input_image_format_reg(VOID)
{
	//T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 format;

	//format.reg = IME_GETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS);

	return imeg->reg_9.bit.ime_imat;
}
//-------------------------------------------------------------------------------


#endif




