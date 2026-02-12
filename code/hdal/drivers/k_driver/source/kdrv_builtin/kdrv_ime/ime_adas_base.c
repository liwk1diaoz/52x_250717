/*
    IME module driver

    NT96520 IME module driver.

    @file       ime_adas_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/


#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_adas_base.h"


static UINT32 ime_stl_out_edge_addr = 0x0;
static UINT32 ime_stl_out_hist_addr = 0x0;



static VOID ime_cal_threshold_sep(UINT32 th, UINT32 *p_msb, UINT32 *p_lsb);


static VOID ime_cal_threshold_sep(UINT32 th, UINT32 *p_msb, UINT32 *p_lsb)
{
	UINT32 rate = 0, factor = 0;

	rate = th >> 10;
	factor = th - (rate << 10);

	*p_msb = rate;
	*p_lsb = factor;
}
//-------------------------------------------------------------------------------

#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

VOID ime_set_adas_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_stl_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_flip_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_stl_flip_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


UINT32 ime_get_adas_enable_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return (UINT32)arg.bit.ime_stl_en;
}
//------------------------------------------------------------------------------


VOID ime_set_adas_median_filter_enable_reg(UINT32 set_en)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_stl_ftr_en = set_en;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_after_filter_out_sel_reg(UINT32 sel)
{
	T_IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS);

	arg.bit.ime_stl_out_sel = sel;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_row_position_reg(UINT32 row0, UINT32 row1)
{
	T_IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS);

	arg.bit.ime_stl_row0 = row0;
	arg.bit.ime_stl_row1 = row1;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_edge_kernel_enable_reg(UINT32 set_en0, UINT32 set_en1)
{
	T_IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS);

	arg.bit.ime_stl_emken0 = set_en0;
	arg.bit.ime_stl_emken1 = set_en1;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_adas_edge_kernel_sel_reg(UINT32 ker_sel0, UINT32 ker_sel1)
{
	T_IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS);

	arg.bit.ime_stl_emk0 = ker_sel0;
	arg.bit.ime_stl_emk1 = ker_sel1;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_edge_kernel_shift_reg(UINT32 ker_sft0, UINT32 ker_sft1)
{
	T_IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0_OFS);

	arg.bit.ime_stl_msft0 = ker_sft0;
	arg.bit.ime_stl_msft1 = ker_sft1;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_histogram_set0_reg(UINT32 pos_h, UINT32 pos_v, UINT32 size_h, UINT32 size_v)
{
	T_IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER0 arg0;
	T_IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0 arg1;

	arg0.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER0_OFS);
	arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0_OFS);

	arg0.bit.ime_stl_posh0 = pos_h;
	arg0.bit.ime_stl_posv0 = pos_v;

	arg1.bit.ime_stl_sizeh0 = (size_h & 0x3ff);
	arg1.bit.ime_stl_sizev0 = (size_v & 0x3ff);

	arg1.bit.ime_stl_sizeh0_msb = (size_h >> 10);
	arg1.bit.ime_stl_sizev0_msb = (size_v >> 10);

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER0_OFS, arg0.reg);
	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_histogram_set1_reg(UINT32 pos_h, UINT32 pos_v, UINT32 size_h, UINT32 size_v)
{
	T_IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER1 arg0;
	T_IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER1 arg1;

	arg0.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER1_OFS);
	arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER1_OFS);

	arg0.bit.ime_stl_posh1 = pos_h;
	arg0.bit.ime_stl_posv1 = pos_v;

	arg1.bit.ime_stl_sizeh1 = (size_h & 0x3ff);
	arg1.bit.ime_stl_sizev1 = (size_v & 0x3ff);

	arg1.bit.ime_stl_sizeh1_msb = (size_h >> 10);
	arg1.bit.ime_stl_sizev1_msb = (size_v >> 10);

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER1_OFS, arg0.reg);
	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER1_OFS, arg1.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_adas_histogram_acc_target_set0_reg(UINT32 acc_tag)
{
	T_IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0 arg0;

	arg0.reg = IME_GETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0_OFS);

	arg0.bit.ime_stl_acct0 = acc_tag;

	IME_SETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0_OFS, arg0.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_histogram_acc_target_set1_reg(UINT32 acc_tag)
{
	T_IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1 arg1;

	arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1_OFS);

	arg1.bit.ime_stl_acct1 = acc_tag;

	IME_SETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1_OFS, arg1.reg);
}
//------------------------------------------------------------------------------



VOID ime_set_adas_roi_threshold0_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI0_THRESHOLD_REGISTER0 arg;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;


	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI0_THRESHOLD_REGISTER0_OFS);
	msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	msb.bit.ime_roi0_msb_th0 = rate;
	arg.bit.ime_roi0_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	msb.bit.ime_roi0_msb_th1 = rate;
	arg.bit.ime_roi0_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	msb.bit.ime_roi0_msb_th2 = rate;
	arg.bit.ime_roi0_th2 = factor;

	msb.bit.ime_roi0_src = src;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI0_THRESHOLD_REGISTER0_OFS, arg.reg);
	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold1_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI1_THRESHOLD_REGISTER0 arg;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI1_THRESHOLD_REGISTER0_OFS);
	msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	msb.bit.ime_roi1_msb_th0 = rate;
	arg.bit.ime_roi1_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	msb.bit.ime_roi1_msb_th1 = rate;
	arg.bit.ime_roi1_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	msb.bit.ime_roi1_msb_th2 = rate;
	arg.bit.ime_roi1_th2 = factor;

	msb.bit.ime_roi1_src = src;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI1_THRESHOLD_REGISTER0_OFS, arg.reg);
	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold2_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI2_THRESHOLD_REGISTER0 arg;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI2_THRESHOLD_REGISTER0_OFS);
	msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	msb.bit.ime_roi2_msb_th0 = rate;
	arg.bit.ime_roi2_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	msb.bit.ime_roi2_msb_th1 = rate;
	arg.bit.ime_roi2_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	msb.bit.ime_roi2_msb_th2 = rate;
	arg.bit.ime_roi2_th2 = factor;

	msb.bit.ime_roi2_src = src;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI2_THRESHOLD_REGISTER0_OFS, arg.reg);
	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold3_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI3_THRESHOLD_REGISTER0 arg;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI3_THRESHOLD_REGISTER0_OFS);
	msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	msb.bit.ime_roi3_msb_th0 = rate;
	arg.bit.ime_roi3_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	msb.bit.ime_roi3_msb_th1 = rate;
	arg.bit.ime_roi3_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	msb.bit.ime_roi3_msb_th2 = rate;
	arg.bit.ime_roi3_th2 = factor;

	msb.bit.ime_roi3_src = src;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI3_THRESHOLD_REGISTER0_OFS, arg.reg);
	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold4_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI4_THRESHOLD_REGISTER0 arg;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI4_THRESHOLD_REGISTER0_OFS);
	msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	msb.bit.ime_roi4_msb_th0 = rate;
	arg.bit.ime_roi4_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	msb.bit.ime_roi4_msb_th1 = rate;
	arg.bit.ime_roi4_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	msb.bit.ime_roi4_msb_th2 = rate;
	arg.bit.ime_roi4_th2 = factor;

	msb.bit.ime_roi4_src = src;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI4_THRESHOLD_REGISTER0_OFS, arg.reg);
	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold5_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI5_THRESHOLD_REGISTER0 arg;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI5_THRESHOLD_REGISTER0_OFS);
	msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	msb.bit.ime_roi5_msb_th0 = rate;
	arg.bit.ime_roi5_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	msb.bit.ime_roi5_msb_th1 = rate;
	arg.bit.ime_roi5_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	msb.bit.ime_roi5_msb_th2 = rate;
	arg.bit.ime_roi5_th2 = factor;

	msb.bit.ime_roi5_src = src;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI5_THRESHOLD_REGISTER0_OFS, arg.reg);
	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold6_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI6_THRESHOLD_REGISTER0 arg;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI6_THRESHOLD_REGISTER0_OFS);
	msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	msb.bit.ime_roi6_msb_th0 = rate;
	arg.bit.ime_roi6_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	msb.bit.ime_roi6_msb_th1 = rate;
	arg.bit.ime_roi6_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	msb.bit.ime_roi6_msb_th2 = rate;
	arg.bit.ime_roi6_th2 = factor;

	msb.bit.ime_roi6_src = src;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI6_THRESHOLD_REGISTER0_OFS, arg.reg);
	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold7_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI7_THRESHOLD_REGISTER0 arg;
	T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI7_THRESHOLD_REGISTER0_OFS);
	msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	msb.bit.ime_roi7_msb_th0 = rate;
	arg.bit.ime_roi7_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	msb.bit.ime_roi7_msb_th1 = rate;
	arg.bit.ime_roi7_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	msb.bit.ime_roi7_msb_th2 = rate;
	arg.bit.ime_roi7_th2 = factor;

	msb.bit.ime_roi7_src = src;

	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI7_THRESHOLD_REGISTER0_OFS, arg.reg);
	IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_adas_edge_map_dam_addr_reg(UINT32 addr)
{
	T_IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0 arg;

	ime_stl_out_edge_addr = addr;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0_OFS);

	arg.bit.ime_em_dram_sao_p4 = ime_platform_va2pa(ime_stl_out_edge_addr) >> 2;//(uiaddr >> 2);

	IME_SETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_edge_map_dam_lineoffset_reg(UINT32 lofs)
{
	T_IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0_OFS);

	arg.bit.ime_em_ofso_p4 = (lofs >> 2);

	IME_SETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_adas_edge_map_dam_lineoffset_reg(VOID)
{
	T_IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0_OFS);

	return (arg.bit.ime_em_ofso_p4 << 2);
}
//------------------------------------------------------------------------------


VOID ime_set_adas_histogram_dam_addr_reg(UINT32 addr)
{
	T_IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0 arg;

	ime_stl_out_hist_addr = addr;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0_OFS);

	arg.bit.ime_hist_dram_sao = ime_platform_va2pa(ime_stl_out_hist_addr) >> 2;//(uiaddr >> 2);

	IME_SETREG(IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_adas_burst_length_reg(VOID)
{
#if 0  // HW removed
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.IME_IN_STL_BST = 1;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
#endif
}
//------------------------------------------------------------------------------

VOID ime_get_adas_max_edge_reg(UINT32 *p_get_max0, UINT32 *p_get_max1)
{
	T_IME_OUTPUT_PATH4_MAX_INFORMATION_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_OUTPUT_PATH4_MAX_INFORMATION_REGISTER0_OFS);

	*p_get_max0 = arg.bit.ime_emax0;
	*p_get_max1 = arg.bit.ime_emax1;
}
//------------------------------------------------------------------------------

VOID ime_get_adas_histogram_bin_reg(UINT32 *p_get_hist_bin0, UINT32 *p_get_hist_bin1)
{
	T_IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0 arg0;
	T_IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1 arg1;

	arg0.reg = IME_GETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0_OFS);
	arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1_OFS);

	*p_get_hist_bin0 = arg0.bit.ime_stl_hitbin0;
	*p_get_hist_bin1 = arg1.bit.ime_stl_hitbin1;
}
//------------------------------------------------------------------------------

VOID ime_get_adas_edge_map_addr_reg(UINT32 *p_get_addr)
{
	//T_IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0_OFS);

	*p_get_addr = ime_stl_out_edge_addr;//arg.reg;
}
//------------------------------------------------------------------------------

VOID ime_get_adas_histogram_addr_reg(UINT32 *p_get_addr)
{
	//T_IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0_OFS);

	*p_get_addr = ime_stl_out_hist_addr;//arg.reg;
}
//------------------------------------------------------------------------------

#else

VOID ime_set_adas_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_stl_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_flip_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	imeg->reg_2.bit.ime_stl_flip_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


UINT32 ime_get_adas_enable_reg(VOID)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return (UINT32)imeg->reg_1.bit.ime_stl_en;
}
//------------------------------------------------------------------------------


VOID ime_set_adas_median_filter_enable_reg(UINT32 set_en)
{
	//T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	imeg->reg_1.bit.ime_stl_ftr_en = set_en;

	//IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_after_filter_out_sel_reg(UINT32 sel)
{
	//T_IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS);

	imeg->reg_220.bit.ime_stl_out_sel = sel;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_row_position_reg(UINT32 row0, UINT32 row1)
{
	//T_IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS);

	imeg->reg_220.bit.ime_stl_row0 = row0;
	imeg->reg_220.bit.ime_stl_row1 = row1;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_edge_kernel_enable_reg(UINT32 set_en0, UINT32 set_en1)
{
	//T_IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS);

	imeg->reg_220.bit.ime_stl_emken0 = set_en0;
	imeg->reg_220.bit.ime_stl_emken1 = set_en1;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_adas_edge_kernel_sel_reg(UINT32 ker_sel0, UINT32 ker_sel1)
{
	//T_IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS);

	imeg->reg_220.bit.ime_stl_emk0 = ker_sel0;
	imeg->reg_220.bit.ime_stl_emk1 = ker_sel1;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_INFORMATION_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_edge_kernel_shift_reg(UINT32 ker_sft0, UINT32 ker_sft1)
{
	//T_IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0_OFS);

	imeg->reg_222.bit.ime_stl_msft0 = ker_sft0;
	imeg->reg_222.bit.ime_stl_msft1 = ker_sft1;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_histogram_set0_reg(UINT32 pos_h, UINT32 pos_v, UINT32 size_h, UINT32 size_v)
{
	//T_IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER0 arg0;
	//T_IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0 arg1;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER0_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0_OFS);

	imeg->reg_221.bit.ime_stl_posh0 = pos_h;
	imeg->reg_221.bit.ime_stl_posv0 = pos_v;

	imeg->reg_222.bit.ime_stl_sizeh0 = (size_h & 0x3ff);
	imeg->reg_222.bit.ime_stl_sizev0 = (size_v & 0x3ff);

	imeg->reg_222.bit.ime_stl_sizeh0_msb = (size_h >> 10);
	imeg->reg_222.bit.ime_stl_sizev0_msb = (size_v >> 10);

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER0_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER0_OFS, arg1.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_histogram_set1_reg(UINT32 pos_h, UINT32 pos_v, UINT32 size_h, UINT32 size_v)
{
	//T_IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER1 arg0;
	//T_IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER1 arg1;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER1_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER1_OFS);

	imeg->reg_228.bit.ime_stl_posh1 = pos_h;
	imeg->reg_228.bit.ime_stl_posv1 = pos_v;

	imeg->reg_229.bit.ime_stl_sizeh1 = (size_h & 0x3ff);
	imeg->reg_229.bit.ime_stl_sizev1 = (size_v & 0x3ff);

	imeg->reg_229.bit.ime_stl_sizeh1_msb = (size_h >> 10);
	imeg->reg_229.bit.ime_stl_sizev1_msb = (size_v >> 10);

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_POSITION_REGISTER1_OFS, arg0.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_SIZE_REGISTER1_OFS, arg1.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_adas_histogram_acc_target_set0_reg(UINT32 acc_tag)
{
	//T_IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0 arg0;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0_OFS);

	imeg->reg_235.bit.ime_stl_acct0 = acc_tag;

	//IME_SETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0_OFS, arg0.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_histogram_acc_target_set1_reg(UINT32 acc_tag)
{
	//T_IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1 arg1;

	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1_OFS);

	imeg->reg_236.bit.ime_stl_acct1 = acc_tag;

	//IME_SETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1_OFS, arg1.reg);
}
//------------------------------------------------------------------------------



VOID ime_set_adas_roi_threshold0_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI0_THRESHOLD_REGISTER0 arg;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;


	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI0_THRESHOLD_REGISTER0_OFS);
	//msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	imeg->reg_233.bit.ime_roi0_msb_th0 = rate;
	imeg->reg_223.bit.ime_roi0_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	imeg->reg_233.bit.ime_roi0_msb_th1 = rate;
	imeg->reg_223.bit.ime_roi0_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	imeg->reg_233.bit.ime_roi0_msb_th2 = rate;
	imeg->reg_223.bit.ime_roi0_th2 = factor;

	imeg->reg_233.bit.ime_roi0_src = src;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI0_THRESHOLD_REGISTER0_OFS, arg.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold1_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI1_THRESHOLD_REGISTER0 arg;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI1_THRESHOLD_REGISTER0_OFS);
	//msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	imeg->reg_233.bit.ime_roi1_msb_th0 = rate;
	imeg->reg_224.bit.ime_roi1_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	imeg->reg_233.bit.ime_roi1_msb_th1 = rate;
	imeg->reg_224.bit.ime_roi1_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	imeg->reg_233.bit.ime_roi1_msb_th2 = rate;
	imeg->reg_224.bit.ime_roi1_th2 = factor;

	imeg->reg_233.bit.ime_roi1_src = src;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI1_THRESHOLD_REGISTER0_OFS, arg.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold2_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI2_THRESHOLD_REGISTER0 arg;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI2_THRESHOLD_REGISTER0_OFS);
	//msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	imeg->reg_233.bit.ime_roi2_msb_th0 = rate;
	imeg->reg_225.bit.ime_roi2_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	imeg->reg_233.bit.ime_roi2_msb_th1 = rate;
	imeg->reg_225.bit.ime_roi2_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	imeg->reg_233.bit.ime_roi2_msb_th2 = rate;
	imeg->reg_225.bit.ime_roi2_th2 = factor;

	imeg->reg_233.bit.ime_roi2_src = src;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI2_THRESHOLD_REGISTER0_OFS, arg.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold3_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI3_THRESHOLD_REGISTER0 arg;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI3_THRESHOLD_REGISTER0_OFS);
	//msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	imeg->reg_233.bit.ime_roi3_msb_th0 = rate;
	imeg->reg_226.bit.ime_roi3_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	imeg->reg_233.bit.ime_roi3_msb_th1 = rate;
	imeg->reg_226.bit.ime_roi3_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	imeg->reg_233.bit.ime_roi3_msb_th2 = rate;
	imeg->reg_226.bit.ime_roi3_th2 = factor;

	imeg->reg_233.bit.ime_roi3_src = src;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI3_THRESHOLD_REGISTER0_OFS, arg.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold4_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI4_THRESHOLD_REGISTER0 arg;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI4_THRESHOLD_REGISTER0_OFS);
	//msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	imeg->reg_233.bit.ime_roi4_msb_th0 = rate;
	imeg->reg_227.bit.ime_roi4_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	imeg->reg_233.bit.ime_roi4_msb_th1 = rate;
	imeg->reg_227.bit.ime_roi4_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	imeg->reg_233.bit.ime_roi4_msb_th2 = rate;
	imeg->reg_227.bit.ime_roi4_th2 = factor;

	imeg->reg_233.bit.ime_roi4_src = src;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI4_THRESHOLD_REGISTER0_OFS, arg.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold5_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI5_THRESHOLD_REGISTER0 arg;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI5_THRESHOLD_REGISTER0_OFS);
	//msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	imeg->reg_233.bit.ime_roi5_msb_th0 = rate;
	imeg->reg_230.bit.ime_roi5_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	imeg->reg_233.bit.ime_roi5_msb_th1 = rate;
	imeg->reg_230.bit.ime_roi5_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	imeg->reg_233.bit.ime_roi5_msb_th2 = rate;
	imeg->reg_230.bit.ime_roi5_th2 = factor;

	imeg->reg_233.bit.ime_roi5_src = src;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI5_THRESHOLD_REGISTER0_OFS, arg.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold6_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI6_THRESHOLD_REGISTER0 arg;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI6_THRESHOLD_REGISTER0_OFS);
	//msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	imeg->reg_233.bit.ime_roi6_msb_th0 = rate;
	imeg->reg_231.bit.ime_roi6_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	imeg->reg_233.bit.ime_roi6_msb_th1 = rate;
	imeg->reg_231.bit.ime_roi6_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	imeg->reg_233.bit.ime_roi6_msb_th2 = rate;
	imeg->reg_231.bit.ime_roi6_th2 = factor;

	imeg->reg_233.bit.ime_roi6_src = src;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI6_THRESHOLD_REGISTER0_OFS, arg.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_roi_threshold7_reg(UINT32 th0, UINT32 th1, UINT32 th2, UINT32 src)
{
	UINT32 rate = 0, factor = 0;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI7_THRESHOLD_REGISTER0 arg;
	//T_IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0 msb;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI7_THRESHOLD_REGISTER0_OFS);
	//msb.reg = IME_GETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS);

	rate = th0 >> 10;
	factor = th0 - (rate << 10);
	ime_cal_threshold_sep(th0, &rate, &factor);
	imeg->reg_233.bit.ime_roi7_msb_th0 = rate;
	imeg->reg_232.bit.ime_roi7_th0 = factor;

	rate = th1 >> 10;
	factor = th1 - (rate << 10);
	ime_cal_threshold_sep(th1, &rate, &factor);
	imeg->reg_233.bit.ime_roi7_msb_th1 = rate;
	imeg->reg_232.bit.ime_roi7_th1 = factor;

	rate = th2 >> 10;
	factor = th2 - (rate << 10);
	ime_cal_threshold_sep(th2, &rate, &factor);
	imeg->reg_233.bit.ime_roi7_msb_th2 = rate;
	imeg->reg_232.bit.ime_roi7_th2 = factor;

	imeg->reg_233.bit.ime_roi7_src = src;

	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI7_THRESHOLD_REGISTER0_OFS, arg.reg);
	//IME_SETREG(IME_OUTPUT_PATH4_STATISTICAL_ROI_THRESHOLD_MSB_REGISTER0_OFS, msb.reg);
}
//------------------------------------------------------------------------------


VOID ime_set_adas_edge_map_dam_addr_reg(UINT32 addr)
{
	//T_IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0 arg;

	ime_stl_out_edge_addr = addr;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0_OFS);

	imeg->reg_237.bit.ime_em_dram_sao_p4 = ime_platform_va2pa(ime_stl_out_edge_addr) >> 2;//(uiaddr >> 2);

	//IME_SETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_set_adas_edge_map_dam_lineoffset_reg(UINT32 lofs)
{
	//T_IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0_OFS);

	imeg->reg_239.bit.ime_em_ofso_p4 = (lofs >> 2);

	//IME_SETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

UINT32 ime_get_adas_edge_map_dam_lineoffset_reg(VOID)
{
	//T_IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_LINEOFFSET_REGISTER0_OFS);

	return (imeg->reg_239.bit.ime_em_ofso_p4 << 2);
}
//------------------------------------------------------------------------------


VOID ime_set_adas_histogram_dam_addr_reg(UINT32 addr)
{
	//T_IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0 arg;

	ime_stl_out_hist_addr = addr;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0_OFS);

	imeg->reg_238.bit.ime_hist_dram_sao = ime_platform_va2pa(ime_stl_out_hist_addr) >> 2;//(uiaddr >> 2);

	//IME_SETREG(IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0_OFS, arg.reg);
}
//------------------------------------------------------------------------------

VOID ime_get_adas_burst_length_reg(VOID)
{
#if 0  // HW removed
	//T_IME_BURST_LENGTH_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_in_stl_bst = 1;

	//IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
#endif
}
//------------------------------------------------------------------------------

VOID ime_get_adas_max_edge_reg(UINT32 *p_get_max0, UINT32 *p_get_max1)
{
	//T_IME_OUTPUT_PATH4_MAX_INFORMATION_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_MAX_INFORMATION_REGISTER0_OFS);

	*p_get_max0 = imeg->reg_234.bit.ime_emax0;
	*p_get_max1 = imeg->reg_234.bit.ime_emax1;
}
//------------------------------------------------------------------------------

VOID ime_get_adas_histogram_bin_reg(UINT32 *p_get_hist_bin0, UINT32 *p_get_hist_bin1)
{
	//T_IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0 arg0;
	//T_IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1 arg1;

	//arg0.reg = IME_GETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER0_OFS);
	//arg1.reg = IME_GETREG(IME_OUTPUT_PATH4_HISTOGRAM_ACCUMULATION_TARGET_REGISTER1_OFS);

	*p_get_hist_bin0 = imeg->reg_235.bit.ime_stl_hitbin0;
	*p_get_hist_bin1 = imeg->reg_236.bit.ime_stl_hitbin1;
}
//------------------------------------------------------------------------------

VOID ime_get_adas_edge_map_addr_reg(UINT32 *p_get_addr)
{
	//T_IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_MAP_OUTPUT_DMA_ADDRESS_REGISTER0_OFS);

	*p_get_addr = ime_stl_out_edge_addr;//arg.reg;
}
//------------------------------------------------------------------------------

VOID ime_get_adas_histogram_addr_reg(UINT32 *p_get_addr)
{
	//T_IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_OUTPUT_PATH4_EDGE_HISTOGRAM_OUTPUT_DRAM_ADDRESS_REGISTER0_OFS);

	*p_get_addr = ime_stl_out_hist_addr;//arg.reg;
}
//------------------------------------------------------------------------------


#endif
