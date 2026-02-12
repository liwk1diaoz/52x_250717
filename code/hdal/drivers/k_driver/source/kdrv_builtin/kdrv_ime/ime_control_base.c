
/*
    IME module driver

    NT96510 IME module driver.

    @file       ime_control_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2017.  All rights reserved.
*/



#include "ime_reg.h"
#include "ime_int_public.h"

#include "ime_control_base.h"


//-------------------------------------------------------------------------------
// private static function definition - bottom layer
VOID ime_load_reg(UINT32 load_type, UINT32 load_op)
{
	T_IME_ENGINE_CONTROL_REGISTE ctrl;

	ctrl.reg = IME_GETREG(IME_ENGINE_CONTROL_REGISTE_OFS);

	//ctrl.bit.ime_start = 0x0;

	switch (load_type) {
	case IME_LOADTYPE_START:
		ctrl.bit.ime_start_tload = load_op;
		ctrl.bit.ime_frameend_tload = 0x0;
		ctrl.bit.ime_drt_start_load = 0x0;
		break;

	case IME_LOADTYPE_FMEND:
		ctrl.bit.ime_start_tload = 0x0;
		ctrl.bit.ime_frameend_tload = load_op;
		ctrl.bit.ime_drt_start_load = 0x0;
		break;

	case IME_LOADTYPE_DIRECT_START:
		ctrl.bit.ime_start_tload = 0x0;
		ctrl.bit.ime_frameend_tload = 0x0;
		ctrl.bit.ime_drt_start_load = load_op;
		break;
	}

	IME_SETREG(IME_ENGINE_CONTROL_REGISTE_OFS, ctrl.reg);
}
//-------------------------------------------------------------------------------

VOID ime_start_reg(UINT32 op)
{
	T_IME_ENGINE_CONTROL_REGISTE ctrl;

	ctrl.reg = IME_GETREG(IME_ENGINE_CONTROL_REGISTE_OFS);

	ctrl.bit.ime_start = op;

	ctrl.bit.ime_start_tload = 0x0;
	ctrl.bit.ime_frameend_tload = 0x0;
	ctrl.bit.ime_drt_start_load = 0x0;

	IME_SETREG(IME_ENGINE_CONTROL_REGISTE_OFS, ctrl.reg);
}
//-------------------------------------------------------------------------------

VOID ime_reset_reg(UINT32 op)
{
	T_IME_ENGINE_CONTROL_REGISTE ctrl;

	ctrl.reg = IME_GETREG(IME_ENGINE_CONTROL_REGISTE_OFS);

	ctrl.bit.ime_rst = op;
	ctrl.bit.ime_start_tload = 0x0;
	ctrl.bit.ime_frameend_tload = 0x0;
	ctrl.bit.ime_drt_start_load = 0x0;

	IME_SETREG(IME_ENGINE_CONTROL_REGISTE_OFS, ctrl.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_direct_path_control_reg(UINT32 src_ctrl)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_dir_ctrl = src_ctrl;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_direct_path_control_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return arg.bit.ime_dir_ctrl;
}
//-------------------------------------------------------------------------------


VOID ime_set_interrupt_status_reg(UINT32 set_val)
{
	T_IME_STATUS_REGISTER int_status;

	int_status.reg = set_val;

	IME_SETREG(IME_STATUS_REGISTER_OFS, int_status.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_interrupt_enable_reg(UINT32 set_val)
{
	T_IME_INTERRUPT_ENABLE_REGISTER int_enable;

	int_enable.reg = set_val;

	IME_SETREG(IME_INTERRUPT_ENABLE_REGISTER_OFS, int_enable.reg);
}
//-------------------------------------------------------------------------------


UINT32 ime_get_interrupt_enable_reg(VOID)
{
	T_IME_INTERRUPT_ENABLE_REGISTER int_enable;

	int_enable.reg = IME_GETREG(IME_INTERRUPT_ENABLE_REGISTER_OFS);

	return int_enable.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_interrupt_status_reg(VOID)
{
	T_IME_STATUS_REGISTER int_status;

	int_status.reg = IME_GETREG(IME_STATUS_REGISTER_OFS);

	return int_status.reg;
}
//-------------------------------------------------------------------------------


UINT32 ime_get_function_enable_status_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 path_en;

	path_en.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS);

	return path_en.reg;
}
//-------------------------------------------------------------------------------

UINT32 ime_get_start_status_reg(VOID)
{
	T_IME_ENGINE_CONTROL_REGISTE ctrl;

	ctrl.reg = IME_GETREG(IME_ENGINE_CONTROL_REGISTE_OFS);

	return ctrl.bit.ime_start;
}
//-------------------------------------------------------------------------------


VOID ime_set_input_path_burst_size_reg(UINT32 bst_y, UINT32 bst_u, UINT32 bst_v)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_in_bst_y = bst_y;
	arg.bit.ime_in_bst_u = bst_u;
	arg.bit.ime_in_bst_v = bst_v;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_input_path_lca_burst_size_reg(UINT32 bst)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_in_lca_bst = bst;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_path_lca_burst_size_reg(UINT32 bst)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_out_lca_bst = bst;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_input_path_osd_set0_burst_size_reg(UINT32 bst)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_in_stp_bst = bst;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_input_path_pixelation_burst_size_reg(UINT32 bst)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_in_pix_bst = bst;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_input_path_tmnr_burst_size_reg(UINT32 bst0, UINT32 bst1, UINT32 bst2, UINT32 bst3)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_in_tmnr_bst_y = bst0;
	arg.bit.ime_in_tmnr_bst_c = bst1;
	arg.bit.ime_in_3dnr_bst_mv = bst2;
	arg.bit.ime_in_3dnr_bst_mo = bst3;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_path_tmnr_burst_size_reg(UINT32 bst0, UINT32 bst1, UINT32 bst2, UINT32 bst3, UINT32 bst4, UINT32 bst5)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_out_tmnr_bst_y      = bst0;
	arg.bit.ime_out_tmnr_bst_c      = bst1;
	arg.bit.ime_out_3dnr_bst_mv     = bst2;
	arg.bit.ime_out_3dnr_bst_mo     = bst3;
	arg.bit.ime_out_3dnr_bst_mo_roi = bst4;
	arg.bit.ime_out_3dnr_bst_sta    = bst5;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_output_path_p1_burst_size_reg(UINT32 bst_y, UINT32 bst_u, UINT32 bst_v)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_out_p1_bst_y = bst_y;
	arg.bit.ime_out_p1_bst_u = bst_u;
	arg.bit.ime_out_p1_bst_v = bst_v;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_path_p2_burst_size_reg(UINT32 bst_y, UINT32 bst_u)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_out_p2_bst_y = bst_y;
	arg.bit.ime_out_p2_bst_uv = bst_u;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_path_p3_burst_size_reg(UINT32 bst_y, UINT32 bst_u)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_out_p3_bst_y = bst_y;
	arg.bit.ime_out_p3_bst_uv = bst_u;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_output_path_p4_burst_size_reg(UINT32 bst_y)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.ime_out_p4_bst_y = bst_y;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);
}
//-------------------------------------------------------------------------------
#if 0
VOID ime_set_output_path_p5_burst_size_reg(UINT32 bst_y, UINT32 bst_u)
{

	T_IME_BURST_LENGTH_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	arg.bit.IME_OUT_P5_BST_Y = bst_y;
	arg.bit.IME_OUT_P5_BST_UV = bst_u;

	IME_SETREG(IME_BURST_LENGTH_REGISTER0_OFS, arg.reg);

}
//-------------------------------------------------------------------------------
#endif

#if 0
VOID ime_show_debug_message_reg(VOID)
{
#if 0

#if (_EMULATION_ == ENABLE)
	T_IME_DEBUG_REGISTER0 dbg_arg0;
	T_IME_DEBUG_REGISTER1 dbg_arg1;
	T_IME_DEBUG_REGISTER2 dbg_arg2;
	T_IME_DEBUG_REGISTER3 dbg_arg3;
	T_IME_DEBUG_REGISTER4 dbg_arg4;
	T_IME_DEBUG_REGISTER5 dbg_arg5;
	T_IME_DEBUG_REGISTER6 dbg_arg6;
	T_IME_DEBUG_REGISTER7 dbg_arg7;
	T_IME_DEBUG_REGISTER8 dbg_arg8;
	T_IME_DEBUG_REGISTER9 dbg_arg9;
	T_IME_DEBUG_REGISTER10 dbg_arg10;

	dbg_arg0.reg = IME_GETREG(IME_DEBUG_REGISTER0_OFS);
	dbg_arg1.reg = IME_GETREG(IME_DEBUG_REGISTER1_OFS);
	dbg_arg2.reg = IME_GETREG(IME_DEBUG_REGISTER2_OFS);
	dbg_arg3.reg = IME_GETREG(IME_DEBUG_REGISTER3_OFS);
	dbg_arg4.reg = IME_GETREG(IME_DEBUG_REGISTER4_OFS);
	dbg_arg5.reg = IME_GETREG(IME_DEBUG_REGISTER5_OFS);
	dbg_arg6.reg = IME_GETREG(IME_DEBUG_REGISTER6_OFS);
	dbg_arg7.reg = IME_GETREG(IME_DEBUG_REGISTER7_OFS);
	dbg_arg8.reg = IME_GETREG(IME_DEBUG_REGISTER8_OFS);
	dbg_arg9.reg = IME_GETREG(IME_DEBUG_REGISTER9_OFS);
	dbg_arg10.reg = IME_GETREG(IME_DEBUG_REGISTER10_OFS);


	DBG_DUMP("IME: P1 output pixel counter - %08x\r\n", dbg_arg0.bit.IME_OUTPIXEL_CNT_P1);
	DBG_DUMP("IME: P1 output line counter - %08x\r\n", dbg_arg0.bit.IME_OUTLINE_CNT_P1);
	DBG_DUMP("IME: P2 output pixel counter - %08x\r\n", dbg_arg1.bit.IME_OUTPIXEL_CNT_P2);
	DBG_DUMP("IME: P2 output line counter - %08x\r\n", dbg_arg1.bit.IME_OUTLINE_CNT_P2);
	DBG_DUMP("IME: P3 output pixel counter - %08x\r\n", dbg_arg2.bit.IME_OUTPIXEL_CNT_P3);
	DBG_DUMP("IME: P3 output line counter - %08x\r\n", dbg_arg2.bit.IME_OUTLINE_CNT_P3);
	DBG_DUMP("IME: P4 output pixel counter - %08x\r\n", dbg_arg3.bit.IME_OUTPIXEL_CNT_P4);
	DBG_DUMP("IME: P4 output line counter - %08x\r\n", dbg_arg3.bit.IME_OUTLINE_CNT_P4);

	DBG_DUMP("IME: P1 input pixel counter - %08x\r\n", dbg_arg4.bit.IME_INPIXEL_CNT_P1);
	DBG_DUMP("IME: P1 input line counter - %08x\r\n", dbg_arg4.bit.IME_INLINE_CNT_P1);
	DBG_DUMP("IME: P2 input pixel counter - %08x\r\n", dbg_arg5.bit.IME_INPIXEL_CNT_P2);
	DBG_DUMP("IME: P2 input line counter - %08x\r\n", dbg_arg5.bit.IME_INLINE_CNT_P2);
	DBG_DUMP("IME: P3 input pixel counter - %08x\r\n", dbg_arg6.bit.IME_INPIXEL_CNT_P3);
	DBG_DUMP("IME: P3 input line counter - %08x\r\n", dbg_arg6.bit.IME_INLINE_CNT_P3);
	DBG_DUMP("IME: P4 input pixel counter - %08x\r\n", dbg_arg7.bit.IME_INPIXEL_CNT_P4);
	DBG_DUMP("IME: P4 input line counter - %08x\r\n", dbg_arg7.bit.IME_INLINE_CNT_P4);

	DBG_DUMP("IME: check_sum for IME write H.264 Y - %08x\r\n", dbg_arg8.bit.IME_CHKSUM_Y_H264);
	DBG_DUMP("IME: check_sum for IME write H.264 C - %08x\r\n", dbg_arg9.bit.IME_CHKSUM_C_H264);
	DBG_DUMP("IME: write H.264 horizontal buffer counter - %08x\r\n", dbg_arg10.bit.IME_WHBC_H264);
	DBG_DUMP("IME: write H.264 vertical buffer counter - %08x\r\n", dbg_arg10.bit.IME_WVBC_H264);
	DBG_DUMP("IME: write H.264 buffer counter - %08x\r\n", dbg_arg10.bit.IME_WBC_H264);
#else
	DBG_DUMP("IME: sorry, this function is closed!!!\r\n");
#endif

#endif
}
#endif
//-------------------------------------------------------------------------------


INT32 ime_get_burst_size_reg(UINT32 chl_num)
{
	T_IME_BURST_LENGTH_REGISTER0 arg;
	INT32 rt_word = -1;

	arg.reg = IME_GETREG(IME_BURST_LENGTH_REGISTER0_OFS);

	//DBG_DUMP("IME: input and output burst setting information\r\n");
	if (chl_num == 0) {
		switch (arg.bit.ime_in_bst_y) {
		case 0:
			//DBG_DUMP("IME: input Y - 16W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: input Y - 16W.\r\n");
			rt_word = 16;
			break;

		default:
			//DBG_DUMP("IME: output path1 Y - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 1) {
		switch (arg.bit.ime_in_bst_u) {
		case 0:
			//DBG_DUMP("IME: input U - 16W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: input U - 16W.\r\n");
			rt_word = 16;
			break;

		default:
			//DBG_DUMP("IME: output path1 Y - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 2) {
		switch (arg.bit.ime_in_bst_v) {
		case 0:
			//DBG_DUMP("IME: input V - 16W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: input V - 16W.\r\n");
			rt_word = 16;
			break;

		default:
			//DBG_DUMP("IME: output path1 Y - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 3) {
		switch (arg.bit.ime_out_p1_bst_y) {
		case 0:
			//DBG_DUMP("IME: output path1 Y - 32W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: output path1 Y - 16W.\r\n");
			rt_word = 16;
			break;

		case 2:
			//DBG_DUMP("IME: output path1 Y - 48W.\r\n");
			rt_word = 48;
			break;

		case 3:
			//DBG_DUMP("IME: output path1 Y - 64W.\r\n");
			rt_word = 64;
			break;

		default:
			//DBG_DUMP("IME: output path1 Y - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 4) {
		switch (arg.bit.ime_out_p1_bst_u) {
		case 0:
			//DBG_DUMP("IME: output path1 U - 32W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: output path1 U - 16W.\r\n");
			rt_word = 16;
			break;

		case 2:
			//DBG_DUMP("IME: output path1 U - 48W.\r\n");
			rt_word = 48;
			break;

		case 3:
			//DBG_DUMP("IME: output path1 U - 64W.\r\n");
			rt_word = 64;
			break;

		default:
			//DBG_DUMP("IME: output path1 U - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 5) {
		switch (arg.bit.ime_out_p1_bst_v) {
		case 0:
			//DBG_DUMP("IME: output path1 V - 32W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: output path1 V - 16W.\r\n");
			rt_word = 16;
			break;

		default:
			//DBG_DUMP("IME: output path1 V - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 6) {
		switch (arg.bit.ime_out_p2_bst_y) {
		case 0:
			//DBG_DUMP("IME: output path2 Y - 32W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: output path2 Y - 16W.\r\n");
			rt_word = 16;
			break;

		case 2:
			//DBG_DUMP("IME: output path2 Y - 32W.\r\n");
			rt_word = 48;
			break;

		case 3:
			//DBG_DUMP("IME: output path2 Y - 16W.\r\n");
			rt_word = 64;
			break;

		default:
			//DBG_DUMP("IME: output path2 Y - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 7) {
		switch (arg.bit.ime_out_p2_bst_uv) {
		case 0:
			//DBG_DUMP("IME: output path2 UV - 32W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: output path2 UV - 16W.\r\n");
			rt_word = 16;
			break;

		case 2:
			//DBG_DUMP("IME: output path2 UV - 32W.\r\n");
			rt_word = 48;
			break;

		case 3:
			//DBG_DUMP("IME: output path2 UV - 16W.\r\n");
			rt_word = 64;
			break;

		default:
			//DBG_DUMP("IME: output path2 UV - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 8) {
		switch (arg.bit.ime_out_p3_bst_y) {
		case 0:
			//DBG_DUMP("IME: output path3 Y - 32W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: output path3 Y - 16W.\r\n");
			rt_word = 16;
			break;

		default:
			//DBG_DUMP("IME: output path3 Y - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 9) {
		switch (arg.bit.ime_out_p3_bst_uv) {
		case 0:
			//DBG_DUMP("IME: output path3 UV - 32W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: output path3 UV - 16W.\r\n");
			rt_word = 16;
			break;

		default:
			//DBG_DUMP("IME: output path3 UV - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 10) {
		switch (arg.bit.ime_out_p4_bst_y) {
		case 0:
			//DBG_DUMP("IME: output path4 Y - 32W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: output path4 Y - 16W.\r\n");
			rt_word = 16;
			break;

		default:
			//DBG_DUMP("IME: output path4 Y - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 11) {
		switch (arg.bit.ime_in_lca_bst) {
		case 0:
			//DBG_DUMP("IME: input LCA - 32W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: input LCA - 16W.\r\n");
			rt_word = 16;
			break;

		default:
			//DBG_DUMP("IME: input LCA - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 12) {
		switch (arg.bit.ime_out_lca_bst) {
		case 0:
			//DBG_DUMP("IME: input LCA - 32W.\r\n");
			rt_word = 32;
			break;

		case 1:
			//DBG_DUMP("IME: input LCA - 16W.\r\n");
			rt_word = 16;
			break;

		default:
			//DBG_DUMP("IME: input LCA - unknow.\r\n");
			rt_word = -1;
			break;
		}
	} else if (chl_num == 13) {
		switch (arg.bit.ime_in_stp_bst) {
		case 0:
			rt_word = 32;
			break;

		case 1:
			rt_word = 16;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 14) {
		switch (arg.bit.ime_in_pix_bst) {
		case 0:
			rt_word = 32;
			break;

		case 1:
			rt_word = 16;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 15) {
		switch (arg.bit.ime_in_tmnr_bst_y) {
		case 0:
			rt_word = 64;
			break;

		case 1:
			rt_word = 32;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 16) {
		switch (arg.bit.ime_in_tmnr_bst_c) {
		case 0:
			rt_word = 64;
			break;

		case 1:
			rt_word = 32;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 17) {
		switch (arg.bit.ime_out_tmnr_bst_y) {
		case 0:
			rt_word = 32;
			break;

		case 1:
			rt_word = 16;
			break;

		case 2:
			rt_word = 48;
			break;

		case 3:
			rt_word = 64;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 18) {
		switch (arg.bit.ime_out_tmnr_bst_c) {
		case 0:
			rt_word = 32;
			break;

		case 1:
			rt_word = 16;
			break;

		case 2:
			rt_word = 48;
			break;

		case 3:
			rt_word = 64;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 19) {
		switch (arg.bit.ime_in_3dnr_bst_mv) {
		case 0:
			rt_word = 32;
			break;

		case 1:
			rt_word = 16;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 20) {
		switch (arg.bit.ime_out_3dnr_bst_mv) {
		case 0:
			rt_word = 32;
			break;

		case 1:
			rt_word = 16;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 21) {
		switch (arg.bit.ime_in_3dnr_bst_mo) {
		case 0:
			rt_word = 32;
			break;

		case 1:
			rt_word = 16;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 22) {
		switch (arg.bit.ime_out_3dnr_bst_mo) {
		case 0:
			rt_word = 32;
			break;

		case 1:
			rt_word = 16;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 23) {
		switch (arg.bit.ime_out_3dnr_bst_mo_roi) {
		case 0:
			rt_word = 32;
			break;

		case 1:
			rt_word = 16;
			break;

		default:
			rt_word = -1;
			break;
		}
	} else if (chl_num == 24) {
		switch (arg.bit.ime_out_3dnr_bst_sta) {
		case 0:
			rt_word = 32;
			break;

		case 1:
			rt_word = 16;
			break;

		default:
			rt_word = -1;
			break;
		}
	}

	return rt_word;
}
//-------------------------------------------------------------------------------

VOID ime_set_init_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER0 arg0;
	T_IME_FUNCTION_CONTROL_REGISTER1 arg1;
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0 arg2;
	T_IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1 arg3;
	T_IME_INPUT_STRIPE_VERTICAL_DIMENSION arg4;

	arg0.reg = 0x0;
	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER0_OFS, arg0.reg);

	arg1.reg = 0x0;
	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg1.reg);

	arg2.reg = 0x0;
	IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION0_OFS, arg2.reg);

	arg3.reg = 0x0;
	IME_SETREG(IME_INPUT_STRIPE_HORIZONTAL_DIMENSION1_OFS, arg3.reg);

	arg4.reg = 0x0;
	IME_SETREG(IME_INPUT_STRIPE_VERTICAL_DIMENSION_OFS, arg4.reg);
}
//-------------------------------------------------------------------------------


VOID ime_set_linked_list_fire_reg(UINT32 set_fire)
{
	T_IME_ENGINE_CONTROL_REGISTE arg;

	arg.reg = IME_GETREG(IME_ENGINE_CONTROL_REGISTE_OFS);

	arg.bit.ime_ll_fire = set_fire;

	IME_SETREG(IME_ENGINE_CONTROL_REGISTE_OFS, arg.reg);
}
//-------------------------------------------------------------------------------



VOID ime_set_linked_list_terminate_reg(UINT32 set_terminate)
{
	T_IME_LINKED_LIST_CONTROL_REGISTER arg;

	arg.reg = IME_GETREG(IME_LINKED_LIST_CONTROL_REGISTER_OFS);

	arg.bit.ime_ll_terminate = set_terminate;

	IME_SETREG(IME_LINKED_LIST_CONTROL_REGISTER_OFS, arg.reg);

	//debug_msg("ime_ll_terminate: %08x\r\n", arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_linked_list_addr = 0;
VOID ime_set_linked_list_addr_reg(UINT32 set_addr)
{
	T_IME_LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER arg;

	ime_linked_list_addr = set_addr;

	arg.reg = IME_GETREG(IME_LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER_OFS);

	//debug_msg("*ll-addr: %08x\r\n", set_addr);

	arg.bit.ime_dram_ll_sai = ime_platform_va2pa(ime_linked_list_addr) >> 2;

	IME_SETREG(IME_LINKED_LIST_INPUT_DMA_CHANNEL_REGISTER_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_line_based_break_point_reg(UINT32 set_bp1, UINT32 set_bp2, UINT32 set_bp3)
{
	T_IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER0 arg1;
	T_IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER1 arg2;

	arg1.reg = IME_GETREG(IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER0_OFS);
	arg2.reg = IME_GETREG(IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER1_OFS);

	arg1.bit.ime_in_line_bp1 = set_bp1;
	arg1.bit.ime_in_line_bp2 = set_bp2;
	arg2.bit.ime_in_line_bp3 = set_bp3;

	IME_SETREG(IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER0_OFS, arg1.reg);
	IME_SETREG(IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER1_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_pixel_based_break_point_reg(UINT32 set_bp1, UINT32 set_bp2, UINT32 set_bp3)
{
	T_IME_INPUT_PIXEL_PATH_BREAK_POINT_REGISTER0 arg0;
	T_IME_INPUT_PIXEL_PATH_BREAK_POINT_REGISTER1 arg1;
	T_IME_INPUT_PIXEL_PATH_BREAK_POINT_REGISTER2 arg2;

	arg0.reg = IME_GETREG(IME_INPUT_PATH_PIXEL_BREAK_POINT_REGISTER0_OFS);
	arg1.reg = IME_GETREG(IME_INPUT_PATH_PIXEL_BREAK_POINT_REGISTER1_OFS);
	arg2.reg = IME_GETREG(IME_INPUT_PATH_PIXEL_BREAK_POINT_REGISTER2_OFS);

	arg0.bit.ime_in_pxl_bp1 = set_bp1;
	arg1.bit.ime_in_pxl_bp2 = set_bp2;
	arg2.bit.ime_in_pxl_bp3 = set_bp3;

	IME_SETREG(IME_INPUT_PATH_PIXEL_BREAK_POINT_REGISTER0_OFS, arg0.reg);
	IME_SETREG(IME_INPUT_PATH_PIXEL_BREAK_POINT_REGISTER1_OFS, arg1.reg);
	IME_SETREG(IME_INPUT_PATH_PIXEL_BREAK_POINT_REGISTER2_OFS, arg2.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_break_point_mode_reg(UINT32 set_bp_mode)
{
	T_IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER1_OFS);

	arg.bit.ime_bp_mode= set_bp_mode;

	IME_SETREG(IME_INPUT_PATH_LINE_BREAK_POINT_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_set_single_output_reg(UINT32 set_en, UINT32 set_ch)
{
	T_IME_DRAM_SINGLE_OUTPUT_CONTROL_REGISTER arg;

	//arg.reg = IME_GETREG(IME_DRAM_SINGLE_OUTPUT_CONTROL_REGISTER_OFS);

	arg.reg = (set_ch & 0x000003FF);
	arg.bit.ime_dram_out_mode = set_en;

	IME_SETREG(IME_DRAM_SINGLE_OUTPUT_CONTROL_REGISTER_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

VOID ime_get_single_output_reg(UINT32 *p_get_en, UINT32 *p_get_ch)
{
	T_IME_DRAM_SINGLE_OUTPUT_CONTROL_REGISTER arg;

	arg.reg = IME_GETREG(IME_DRAM_SINGLE_OUTPUT_CONTROL_REGISTER_OFS);

	*p_get_ch = (arg.reg & 0x000003FF);
	*p_get_en = arg.bit.ime_dram_out_mode;
}
//-------------------------------------------------------------------------------


VOID ime_set_low_delay_mode_reg(UINT32 set_en, UINT32 set_sel)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	arg.bit.ime_low_dly_en = set_en;
	arg.bit.ime_low_dly_sel = set_sel;

	IME_SETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS, arg.reg);
}
//-------------------------------------------------------------------------------

UINT32 ime_get_low_delay_mode_reg(VOID)
{
	T_IME_FUNCTION_CONTROL_REGISTER1 arg;

	arg.reg = IME_GETREG(IME_FUNCTION_CONTROL_REGISTER1_OFS);

	return arg.bit.ime_low_dly_en;
}



