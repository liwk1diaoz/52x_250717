
/*
    IME module driver

    NT96520 IME module driver.

    @file       ime_3dnr_base.c
    @ingroup    mIIPPIME
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/



#include "ime_reg.h"
#include "ime_int_public.h"
#include "ime_compression_base.h"

#if 1
UINT32 const ime_yuvcomp_dct_dec_qtbl[28][3] = {
	{26, 33, 42},
	{28, 35, 45},
	{32, 40, 51},
	{36, 46, 58},
	{40, 51, 64},
	{44, 56, 70},
	{52, 66, 83},
	{56, 71, 90},
	{64, 81, 102},
	{72, 91, 115},
	{80, 101, 128},
	{88, 111, 141},
	{104, 132, 166},
	{112, 142, 179},
	{128, 162, 205},
	{144, 182, 230},
	{160, 202, 256},
	{176, 223, 282},
	{208, 263, 333},
	{224, 283, 358},
	{256, 324, 410},
	{288, 364, 461},
	{320, 405, 512},
	{352, 445, 563},
	{416, 526, 666},
	{448, 567, 717},
	{512, 648, 819},
	{576, 729, 922},
};


UINT32 const ime_yuvcomp_dct_enc_qtbl[28][3] = {
	{5041, 3178, 1997},
	{4681, 2996, 1864},
	{4096, 2621, 1645},
	{3641, 2280, 1446},
	{3277, 2056, 1311},
	{2979, 1872, 1198},
	{2521, 1589, 1011},
	{2341, 1477, 932},
	{2048, 1295, 822},
	{1820, 1152, 729},
	{1638, 1038, 655},
	{1489, 945, 595},
	{1260, 794, 505},
	{1170, 738, 469},
	{1024, 647, 409},
	{910, 576, 365},
	{819, 519, 328},
	{745, 470, 297},
	{630, 399, 252},
	{585, 371, 234},
	{512, 324, 205},
	{455, 288, 182},
	{410, 259, 164},
	{372, 236, 149},
	{315, 199, 126},
	{293, 185, 117},
	{256, 162, 102},
	{228, 144, 91},
};

UINT32 const ime_yuvcomp_dct_max[28] = {
	314, 292, 255, 227, 204, 186, 157, 146, 128, 114, 102, 93, 79, 73, 64, 57, 51, 47, 39, 37, 32, 29, 26, 23, 20, 18, 16, 14
};

UINT32 ime_yuvcomp_q_tbl_idx[16] = {0};




UINT32 yuv_dct_qtab_encp[16][3] = {0};
UINT32 yuv_dct_qtab_decp[16][3] = {0};
UINT32 yuv_dct_qtab_dc[16] = {0};
#endif

#if (IPP_DRIVER_NEW_REG_HANDLE == DISABLE)

VOID ime_comps_p1_encoder_shift_mode_enable_reg(UINT32 set_en)
{
	T_IME_COMPRESSION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_p1_enc_smode_en = set_en;

	IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS, arg.reg);
}
//--------------------------------------------------------------

VOID ime_comps_tmnr_out_ref_encoder_shift_mode_enable_reg(UINT32 set_en)
{
	T_IME_COMPRESSION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_refout_enc_smode_en = set_en;

	IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS, arg.reg);
}
//--------------------------------------------------------------

VOID ime_comps_tmnr_in_ref_decoder_shift_mode_enable_reg(UINT32 set_en)
{
	T_IME_COMPRESSION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_refin_dec_smode_en = set_en;

	IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS, arg.reg);
}
//--------------------------------------------------------------

VOID ime_comps_tmnr_in_ref_encoder_dithering_enable_reg(UINT32 set_en)
{
	T_IME_COMPRESSION_CONTROL_REGISTER0 arg;

	arg.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS);

	arg.bit.ime_3dnr_refin_dec_dither_en = set_en;

	IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS, arg.reg);
}
//--------------------------------------------------------------

VOID ime_comps_tmnr_in_ref_encoder_dithering_initial_seed_reg(UINT32 set_seed0, UINT32 set_seed1)
{
	T_IME_COMPRESSION_CONTROL_REGISTER0 arg1;
	T_IME_COMPRESSION_CONTROL_REGISTER1 arg2;

	arg1.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS);
	arg2.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER1_OFS);

	arg1.bit.ime_3dnr_refin_dec_dither_seed0 = set_seed0;
	arg2.bit.ime_3dnr_refin_dec_dither_seed1 = set_seed1;

	IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS, arg1.reg);
	IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER1_OFS, arg2.reg);
}
//--------------------------------------------------------------

VOID ime_comps_encoder_parameters_reg(UINT32(*p_enc_tab)[3])
{
	UINT32 arg_val, i;


	for (i = 0; i < 16; i++) {
		arg_val = 0;
		arg_val = (p_enc_tab[i][1] << 16) + (p_enc_tab[i][0]);

		IME_SETREG(IME_YCC_ENCODER_Q_TABLE_REGISTER0_OFS + (i << 3), arg_val);
	}

	for (i = 0; i < 16; i++) {
		arg_val = 0;
		arg_val = p_enc_tab[i][2];

		IME_SETREG(IME_YCC_ENCODER_Q_TABLE_REGISTER1_OFS + (i << 3), arg_val);
	}

#if 0
	ime_int_reg->ime_register_580.bit.dct_qtbl0_enc_p0 = p_enc_tab[0][0];
	ime_int_reg->ime_register_580.bit.dct_qtbl0_enc_p1 = p_enc_tab[0][1];
	ime_int_reg->ime_register_581.bit.dct_qtbl0_enc_p2 = p_enc_tab[0][2];

	ime_int_reg->ime_register_582.bit.dct_qtbl1_enc_p0 = p_enc_tab[1][0];
	ime_int_reg->ime_register_582.bit.dct_qtbl1_enc_p1 = p_enc_tab[1][1];
	ime_int_reg->ime_register_583.bit.dct_qtbl1_enc_p2 = p_enc_tab[1][2];

	ime_int_reg->ime_register_584.bit.dct_qtbl2_enc_p0 = p_enc_tab[2][0];
	ime_int_reg->ime_register_584.bit.dct_qtbl2_enc_p1 = p_enc_tab[2][1];
	ime_int_reg->ime_register_585.bit.dct_qtbl2_enc_p2 = p_enc_tab[2][2];

	ime_int_reg->ime_register_586.bit.dct_qtbl3_enc_p0 = p_enc_tab[3][0];
	ime_int_reg->ime_register_586.bit.dct_qtbl3_enc_p1 = p_enc_tab[3][1];
	ime_int_reg->ime_register_587.bit.dct_qtbl3_enc_p2 = p_enc_tab[3][2];

	ime_int_reg->ime_register_588.bit.dct_qtbl4_enc_p0 = p_enc_tab[4][0];
	ime_int_reg->ime_register_588.bit.dct_qtbl4_enc_p1 = p_enc_tab[4][1];
	ime_int_reg->ime_register_589.bit.dct_qtbl4_enc_p2 = p_enc_tab[4][2];

	ime_int_reg->ime_register_590.bit.dct_qtbl5_enc_p0 = p_enc_tab[5][0];
	ime_int_reg->ime_register_590.bit.dct_qtbl5_enc_p1 = p_enc_tab[5][1];
	ime_int_reg->ime_register_591.bit.dct_qtbl5_enc_p2 = p_enc_tab[5][2];

	ime_int_reg->ime_register_592.bit.dct_qtbl6_enc_p0 = p_enc_tab[6][0];
	ime_int_reg->ime_register_592.bit.dct_qtbl6_enc_p1 = p_enc_tab[6][1];
	ime_int_reg->ime_register_593.bit.dct_qtbl6_enc_p2 = p_enc_tab[6][2];

	ime_int_reg->ime_register_594.bit.dct_qtbl7_enc_p0 = p_enc_tab[7][0];
	ime_int_reg->ime_register_594.bit.dct_qtbl7_enc_p1 = p_enc_tab[7][1];
	ime_int_reg->ime_register_595.bit.dct_qtbl7_enc_p2 = p_enc_tab[7][2];

	ime_int_reg->ime_register_596.bit.dct_qtbl8_enc_p0 = p_enc_tab[8][0];
	ime_int_reg->ime_register_596.bit.dct_qtbl8_enc_p1 = p_enc_tab[8][1];
	ime_int_reg->ime_register_597.bit.dct_qtbl8_enc_p2 = p_enc_tab[8][2];

	ime_int_reg->ime_register_598.bit.dct_qtbl9_enc_p0 = p_enc_tab[9][0];
	ime_int_reg->ime_register_598.bit.dct_qtbl9_enc_p1 = p_enc_tab[9][1];
	ime_int_reg->ime_register_599.bit.dct_qtbl9_enc_p2 = p_enc_tab[9][2];

	ime_int_reg->ime_register_600.bit.dct_qtbl10_enc_p0 = p_enc_tab[10][0];
	ime_int_reg->ime_register_600.bit.dct_qtbl10_enc_p1 = p_enc_tab[10][1];
	ime_int_reg->ime_register_601.bit.dct_qtbl10_enc_p2 = p_enc_tab[10][2];

	ime_int_reg->ime_register_602.bit.dct_qtbl11_enc_p0 = p_enc_tab[11][0];
	ime_int_reg->ime_register_602.bit.dct_qtbl11_enc_p1 = p_enc_tab[11][1];
	ime_int_reg->ime_register_603.bit.dct_qtbl11_enc_p2 = p_enc_tab[11][2];

	ime_int_reg->ime_register_604.bit.dct_qtbl12_enc_p0 = p_enc_tab[12][0];
	ime_int_reg->ime_register_604.bit.dct_qtbl12_enc_p1 = p_enc_tab[12][1];
	ime_int_reg->ime_register_605.bit.dct_qtbl12_enc_p2 = p_enc_tab[12][2];

	ime_int_reg->ime_register_606.bit.dct_qtbl13_enc_p0 = p_enc_tab[13][0];
	ime_int_reg->ime_register_606.bit.dct_qtbl13_enc_p1 = p_enc_tab[13][1];
	ime_int_reg->ime_register_607.bit.dct_qtbl13_enc_p2 = p_enc_tab[13][2];

	ime_int_reg->ime_register_608.bit.dct_qtbl14_enc_p0 = p_enc_tab[14][0];
	ime_int_reg->ime_register_608.bit.dct_qtbl14_enc_p1 = p_enc_tab[14][1];
	ime_int_reg->ime_register_609.bit.dct_qtbl14_enc_p2 = p_enc_tab[14][2];

	ime_int_reg->ime_register_610.bit.dct_qtbl15_enc_p0 = p_enc_tab[15][0];
	ime_int_reg->ime_register_610.bit.dct_qtbl15_enc_p1 = p_enc_tab[15][1];
	ime_int_reg->ime_register_611.bit.dct_qtbl15_enc_p2 = p_enc_tab[15][2];
#endif
}
//--------------------------------------------------------------


VOID ime_comps_decoder_parameters_reg(UINT32(*p_dec_tab)[3])
{
	UINT32 arg_val, i;


	for (i = 0; i < 16; i++) {
		arg_val = 0;
		arg_val = (p_dec_tab[i][1] << 16) + (p_dec_tab[i][0]);

		IME_SETREG(IME_DECODER_QUALITY_TABLE_0_REGISTER0_OFS + (i << 3), arg_val);
	}

	for (i = 0; i < 16; i++) {
		arg_val = 0;
		arg_val = p_dec_tab[i][2];

		IME_SETREG(IME_DECODER_QUALITY_TABLE_0_REGISTER1_OFS + (i << 3), arg_val);
	}

#if 0
	ime_int_reg->ime_register_624.bit.dct_qtbl0_dec_p0 = p_dec_tab[0][0];
	ime_int_reg->ime_register_624.bit.dct_qtbl0_dec_p1 = p_dec_tab[0][1];

	ime_int_reg->ime_register_625.bit.dct_qtbl0_dec_p2 = p_dec_tab[0][2];

	ime_int_reg->ime_register_626.bit.dct_qtbl1_dec_p0 = p_dec_tab[1][0];
	ime_int_reg->ime_register_626.bit.dct_qtbl1_dec_p1 = p_dec_tab[1][1];
	ime_int_reg->ime_register_627.bit.dct_qtbl1_dec_p2 = p_dec_tab[1][2];

	ime_int_reg->ime_register_628.bit.dct_qtbl2_dec_p0 = p_dec_tab[2][0];
	ime_int_reg->ime_register_628.bit.dct_qtbl2_dec_p1 = p_dec_tab[2][1];
	ime_int_reg->ime_register_629.bit.dct_qtbl2_dec_p2 = p_dec_tab[2][2];

	ime_int_reg->ime_register_630.bit.dct_qtbl3_dec_p0 = p_dec_tab[3][0];
	ime_int_reg->ime_register_630.bit.dct_qtbl3_dec_p1 = p_dec_tab[3][1];
	ime_int_reg->ime_register_631.bit.dct_qtbl3_dec_p2 = p_dec_tab[3][2];

	ime_int_reg->ime_register_632.bit.dct_qtbl4_dec_p0 = p_dec_tab[4][0];
	ime_int_reg->ime_register_632.bit.dct_qtbl4_dec_p1 = p_dec_tab[4][1];
	ime_int_reg->ime_register_633.bit.dct_qtbl4_dec_p2 = p_dec_tab[4][2];

	ime_int_reg->ime_register_634.bit.dct_qtbl5_dec_p0 = p_dec_tab[5][0];
	ime_int_reg->ime_register_634.bit.dct_qtbl5_dec_p1 = p_dec_tab[5][1];
	ime_int_reg->ime_register_635.bit.dct_qtbl5_dec_p2 = p_dec_tab[5][2];

	ime_int_reg->ime_register_636.bit.dct_qtbl6_dec_p0 = p_dec_tab[6][0];
	ime_int_reg->ime_register_636.bit.dct_qtbl6_dec_p1 = p_dec_tab[6][1];
	ime_int_reg->ime_register_637.bit.dct_qtbl6_dec_p2 = p_dec_tab[6][2];

	ime_int_reg->ime_register_638.bit.dct_qtbl7_dec_p0 = p_dec_tab[7][0];
	ime_int_reg->ime_register_638.bit.dct_qtbl7_dec_p1 = p_dec_tab[7][1];
	ime_int_reg->ime_register_639.bit.dct_qtbl7_dec_p2 = p_dec_tab[7][2];

	ime_int_reg->ime_register_640.bit.dct_qtbl8_dec_p0 = p_dec_tab[8][0];
	ime_int_reg->ime_register_640.bit.dct_qtbl8_dec_p1 = p_dec_tab[8][1];
	ime_int_reg->ime_register_641.bit.dct_qtbl8_dec_p2 = p_dec_tab[8][2];

	ime_int_reg->ime_register_642.bit.dct_qtbl9_dec_p0 = p_dec_tab[9][0];
	ime_int_reg->ime_register_642.bit.dct_qtbl9_dec_p1 = p_dec_tab[9][1];
	ime_int_reg->ime_register_643.bit.dct_qtbl9_dec_p2 = p_dec_tab[9][2];

	ime_int_reg->ime_register_644.bit.dct_qtbl10_dec_p0 = p_dec_tab[10][0];
	ime_int_reg->ime_register_644.bit.dct_qtbl10_dec_p1 = p_dec_tab[10][1];
	ime_int_reg->ime_register_645.bit.dct_qtbl10_dec_p2 = p_dec_tab[10][2];

	ime_int_reg->ime_register_646.bit.dct_qtbl11_dec_p0 = p_dec_tab[11][0];
	ime_int_reg->ime_register_646.bit.dct_qtbl11_dec_p1 = p_dec_tab[11][1];
	ime_int_reg->ime_register_647.bit.dct_qtbl11_dec_p2 = p_dec_tab[11][2];

	ime_int_reg->ime_register_648.bit.dct_qtbl12_dec_p0 = p_dec_tab[12][0];
	ime_int_reg->ime_register_648.bit.dct_qtbl12_dec_p1 = p_dec_tab[12][1];
	ime_int_reg->ime_register_649.bit.dct_qtbl12_dec_p2 = p_dec_tab[12][2];

	ime_int_reg->ime_register_650.bit.dct_qtbl13_dec_p0 = p_dec_tab[13][0];
	ime_int_reg->ime_register_650.bit.dct_qtbl13_dec_p1 = p_dec_tab[13][1];
	ime_int_reg->ime_register_651.bit.dct_qtbl13_dec_p2 = p_dec_tab[13][2];

	ime_int_reg->ime_register_652.bit.dct_qtbl14_dec_p0 = p_dec_tab[14][0];
	ime_int_reg->ime_register_652.bit.dct_qtbl14_dec_p1 = p_dec_tab[14][1];
	ime_int_reg->ime_register_653.bit.dct_qtbl14_dec_p2 = p_dec_tab[14][2];

	ime_int_reg->ime_register_654.bit.dct_qtbl15_dec_p0 = p_dec_tab[15][0];
	ime_int_reg->ime_register_654.bit.dct_qtbl15_dec_p1 = p_dec_tab[15][1];
	ime_int_reg->ime_register_655.bit.dct_qtbl15_dec_p2 = p_dec_tab[15][2];
#endif
}
//--------------------------------------------------------------


VOID ime_comps_dc_max_reg(UINT32 *p_dc_max)
{
	UINT32 arg_val, i;

	for (i = 0; i < 16; i += 2) {
		arg_val = 0;
		arg_val = (p_dc_max[i + 1] << 16) + (p_dc_max[i]);

		IME_SETREG(IME_YCC_ENCODER_QUALITY_DC_REGISTER0_OFS + (i << 1), arg_val);
	}

#if 0
	ime_int_reg->ime_register_612.bit.dct_qtbl0_dcmax = p_dc_max[0];
	ime_int_reg->ime_register_612.bit.dct_qtbl1_dcmax = p_dc_max[1];

	ime_int_reg->ime_register_613.bit.dct_qtbl2_dcmax = p_dc_max[2];
	ime_int_reg->ime_register_613.bit.dct_qtbl3_dcmax = p_dc_max[3];

	ime_int_reg->ime_register_614.bit.dct_qtbl4_dcmax = p_dc_max[4];
	ime_int_reg->ime_register_614.bit.dct_qtbl5_dcmax = p_dc_max[5];

	ime_int_reg->ime_register_615.bit.dct_qtbl6_dcmax = p_dc_max[6];
	ime_int_reg->ime_register_615.bit.dct_qtbl7_dcmax = p_dc_max[7];

	ime_int_reg->ime_register_616.bit.dct_qtbl8_dcmax = p_dc_max[8];
	ime_int_reg->ime_register_616.bit.dct_qtbl9_dcmax = p_dc_max[9];

	ime_int_reg->ime_register_617.bit.dct_qtbl10_dcmax = p_dc_max[10];
	ime_int_reg->ime_register_617.bit.dct_qtbl11_dcmax = p_dc_max[11];

	ime_int_reg->ime_register_618.bit.dct_qtbl12_dcmax = p_dc_max[12];
	ime_int_reg->ime_register_618.bit.dct_qtbl13_dcmax = p_dc_max[13];

	ime_int_reg->ime_register_619.bit.dct_qtbl14_dcmax = p_dc_max[14];
	ime_int_reg->ime_register_619.bit.dct_qtbl15_dcmax = p_dc_max[15];
#endif
}

#else

VOID ime_comps_p1_encoder_shift_mode_enable_reg(UINT32 set_en)
{
	//T_IME_COMPRESSION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS);

	imeg->reg_576.bit.ime_p1_enc_smode_en = set_en;

	//IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS, arg.reg);
}
//--------------------------------------------------------------

VOID ime_comps_tmnr_out_ref_encoder_shift_mode_enable_reg(UINT32 set_en)
{
	//T_IME_COMPRESSION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS);

	imeg->reg_576.bit.ime_3dnr_refout_enc_smode_en = set_en;

	//IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS, arg.reg);
}
//--------------------------------------------------------------

VOID ime_comps_tmnr_in_ref_decoder_shift_mode_enable_reg(UINT32 set_en)
{
	//T_IME_COMPRESSION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS);

	imeg->reg_576.bit.ime_3dnr_refin_dec_smode_en = set_en;

	//IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS, arg.reg);
}
//--------------------------------------------------------------

VOID ime_comps_tmnr_in_ref_encoder_dithering_enable_reg(UINT32 set_en)
{
	//T_IME_COMPRESSION_CONTROL_REGISTER0 arg;

	//arg.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS);

	imeg->reg_576.bit.ime_3dnr_refin_dec_dither_en = set_en;

	//IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS, arg.reg);
}
//--------------------------------------------------------------

VOID ime_comps_tmnr_in_ref_encoder_dithering_initial_seed_reg(UINT32 set_seed0, UINT32 set_seed1)
{
	//T_IME_COMPRESSION_CONTROL_REGISTER0 arg1;
	//T_IME_COMPRESSION_CONTROL_REGISTER1 arg2;

	//arg1.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS);
	//arg2.reg = IME_GETREG(IME_COMPRESSION_CONTROL_REGISTER1_OFS);

	imeg->reg_576.bit.ime_3dnr_refin_dec_dither_seed0 = set_seed0;
	imeg->reg_577.bit.ime_3dnr_refin_dec_dither_seed1 = set_seed1;

	//IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER0_OFS, arg1.reg);
	//IME_SETREG(IME_COMPRESSION_CONTROL_REGISTER1_OFS, arg2.reg);
}
//--------------------------------------------------------------

VOID ime_comps_encoder_parameters_reg(UINT32(*p_enc_tab)[3])
{
	UINT32 arg_val, i;


	for (i = 0; i < 16; i++) {
		arg_val = 0;
		arg_val = (p_enc_tab[i][1] << 16) + (p_enc_tab[i][0]);

		IME_SETREG(IME_YCC_ENCODER_Q_TABLE_REGISTER0_OFS + (i << 3), arg_val);
	}

	for (i = 0; i < 16; i++) {
		arg_val = 0;
		arg_val = p_enc_tab[i][2];

		IME_SETREG(IME_YCC_ENCODER_Q_TABLE_REGISTER1_OFS + (i << 3), arg_val);
	}

#if 0
	ime_int_reg->ime_register_580.bit.dct_qtbl0_enc_p0 = p_enc_tab[0][0];
	ime_int_reg->ime_register_580.bit.dct_qtbl0_enc_p1 = p_enc_tab[0][1];
	ime_int_reg->ime_register_581.bit.dct_qtbl0_enc_p2 = p_enc_tab[0][2];

	ime_int_reg->ime_register_582.bit.dct_qtbl1_enc_p0 = p_enc_tab[1][0];
	ime_int_reg->ime_register_582.bit.dct_qtbl1_enc_p1 = p_enc_tab[1][1];
	ime_int_reg->ime_register_583.bit.dct_qtbl1_enc_p2 = p_enc_tab[1][2];

	ime_int_reg->ime_register_584.bit.dct_qtbl2_enc_p0 = p_enc_tab[2][0];
	ime_int_reg->ime_register_584.bit.dct_qtbl2_enc_p1 = p_enc_tab[2][1];
	ime_int_reg->ime_register_585.bit.dct_qtbl2_enc_p2 = p_enc_tab[2][2];

	ime_int_reg->ime_register_586.bit.dct_qtbl3_enc_p0 = p_enc_tab[3][0];
	ime_int_reg->ime_register_586.bit.dct_qtbl3_enc_p1 = p_enc_tab[3][1];
	ime_int_reg->ime_register_587.bit.dct_qtbl3_enc_p2 = p_enc_tab[3][2];

	ime_int_reg->ime_register_588.bit.dct_qtbl4_enc_p0 = p_enc_tab[4][0];
	ime_int_reg->ime_register_588.bit.dct_qtbl4_enc_p1 = p_enc_tab[4][1];
	ime_int_reg->ime_register_589.bit.dct_qtbl4_enc_p2 = p_enc_tab[4][2];

	ime_int_reg->ime_register_590.bit.dct_qtbl5_enc_p0 = p_enc_tab[5][0];
	ime_int_reg->ime_register_590.bit.dct_qtbl5_enc_p1 = p_enc_tab[5][1];
	ime_int_reg->ime_register_591.bit.dct_qtbl5_enc_p2 = p_enc_tab[5][2];

	ime_int_reg->ime_register_592.bit.dct_qtbl6_enc_p0 = p_enc_tab[6][0];
	ime_int_reg->ime_register_592.bit.dct_qtbl6_enc_p1 = p_enc_tab[6][1];
	ime_int_reg->ime_register_593.bit.dct_qtbl6_enc_p2 = p_enc_tab[6][2];

	ime_int_reg->ime_register_594.bit.dct_qtbl7_enc_p0 = p_enc_tab[7][0];
	ime_int_reg->ime_register_594.bit.dct_qtbl7_enc_p1 = p_enc_tab[7][1];
	ime_int_reg->ime_register_595.bit.dct_qtbl7_enc_p2 = p_enc_tab[7][2];

	ime_int_reg->ime_register_596.bit.dct_qtbl8_enc_p0 = p_enc_tab[8][0];
	ime_int_reg->ime_register_596.bit.dct_qtbl8_enc_p1 = p_enc_tab[8][1];
	ime_int_reg->ime_register_597.bit.dct_qtbl8_enc_p2 = p_enc_tab[8][2];

	ime_int_reg->ime_register_598.bit.dct_qtbl9_enc_p0 = p_enc_tab[9][0];
	ime_int_reg->ime_register_598.bit.dct_qtbl9_enc_p1 = p_enc_tab[9][1];
	ime_int_reg->ime_register_599.bit.dct_qtbl9_enc_p2 = p_enc_tab[9][2];

	ime_int_reg->ime_register_600.bit.dct_qtbl10_enc_p0 = p_enc_tab[10][0];
	ime_int_reg->ime_register_600.bit.dct_qtbl10_enc_p1 = p_enc_tab[10][1];
	ime_int_reg->ime_register_601.bit.dct_qtbl10_enc_p2 = p_enc_tab[10][2];

	ime_int_reg->ime_register_602.bit.dct_qtbl11_enc_p0 = p_enc_tab[11][0];
	ime_int_reg->ime_register_602.bit.dct_qtbl11_enc_p1 = p_enc_tab[11][1];
	ime_int_reg->ime_register_603.bit.dct_qtbl11_enc_p2 = p_enc_tab[11][2];

	ime_int_reg->ime_register_604.bit.dct_qtbl12_enc_p0 = p_enc_tab[12][0];
	ime_int_reg->ime_register_604.bit.dct_qtbl12_enc_p1 = p_enc_tab[12][1];
	ime_int_reg->ime_register_605.bit.dct_qtbl12_enc_p2 = p_enc_tab[12][2];

	ime_int_reg->ime_register_606.bit.dct_qtbl13_enc_p0 = p_enc_tab[13][0];
	ime_int_reg->ime_register_606.bit.dct_qtbl13_enc_p1 = p_enc_tab[13][1];
	ime_int_reg->ime_register_607.bit.dct_qtbl13_enc_p2 = p_enc_tab[13][2];

	ime_int_reg->ime_register_608.bit.dct_qtbl14_enc_p0 = p_enc_tab[14][0];
	ime_int_reg->ime_register_608.bit.dct_qtbl14_enc_p1 = p_enc_tab[14][1];
	ime_int_reg->ime_register_609.bit.dct_qtbl14_enc_p2 = p_enc_tab[14][2];

	ime_int_reg->ime_register_610.bit.dct_qtbl15_enc_p0 = p_enc_tab[15][0];
	ime_int_reg->ime_register_610.bit.dct_qtbl15_enc_p1 = p_enc_tab[15][1];
	ime_int_reg->ime_register_611.bit.dct_qtbl15_enc_p2 = p_enc_tab[15][2];
#endif
}
//--------------------------------------------------------------


VOID ime_comps_decoder_parameters_reg(UINT32(*p_dec_tab)[3])
{
	UINT32 arg_val, i;


	for (i = 0; i < 16; i++) {
		arg_val = 0;
		arg_val = (p_dec_tab[i][1] << 16) + (p_dec_tab[i][0]);

		IME_SETREG(IME_DECODER_QUALITY_TABLE_0_REGISTER0_OFS + (i << 3), arg_val);
	}

	for (i = 0; i < 16; i++) {
		arg_val = 0;
		arg_val = p_dec_tab[i][2];

		IME_SETREG(IME_DECODER_QUALITY_TABLE_0_REGISTER1_OFS + (i << 3), arg_val);
	}

#if 0
	ime_int_reg->ime_register_624.bit.dct_qtbl0_dec_p0 = p_dec_tab[0][0];
	ime_int_reg->ime_register_624.bit.dct_qtbl0_dec_p1 = p_dec_tab[0][1];

	ime_int_reg->ime_register_625.bit.dct_qtbl0_dec_p2 = p_dec_tab[0][2];

	ime_int_reg->ime_register_626.bit.dct_qtbl1_dec_p0 = p_dec_tab[1][0];
	ime_int_reg->ime_register_626.bit.dct_qtbl1_dec_p1 = p_dec_tab[1][1];
	ime_int_reg->ime_register_627.bit.dct_qtbl1_dec_p2 = p_dec_tab[1][2];

	ime_int_reg->ime_register_628.bit.dct_qtbl2_dec_p0 = p_dec_tab[2][0];
	ime_int_reg->ime_register_628.bit.dct_qtbl2_dec_p1 = p_dec_tab[2][1];
	ime_int_reg->ime_register_629.bit.dct_qtbl2_dec_p2 = p_dec_tab[2][2];

	ime_int_reg->ime_register_630.bit.dct_qtbl3_dec_p0 = p_dec_tab[3][0];
	ime_int_reg->ime_register_630.bit.dct_qtbl3_dec_p1 = p_dec_tab[3][1];
	ime_int_reg->ime_register_631.bit.dct_qtbl3_dec_p2 = p_dec_tab[3][2];

	ime_int_reg->ime_register_632.bit.dct_qtbl4_dec_p0 = p_dec_tab[4][0];
	ime_int_reg->ime_register_632.bit.dct_qtbl4_dec_p1 = p_dec_tab[4][1];
	ime_int_reg->ime_register_633.bit.dct_qtbl4_dec_p2 = p_dec_tab[4][2];

	ime_int_reg->ime_register_634.bit.dct_qtbl5_dec_p0 = p_dec_tab[5][0];
	ime_int_reg->ime_register_634.bit.dct_qtbl5_dec_p1 = p_dec_tab[5][1];
	ime_int_reg->ime_register_635.bit.dct_qtbl5_dec_p2 = p_dec_tab[5][2];

	ime_int_reg->ime_register_636.bit.dct_qtbl6_dec_p0 = p_dec_tab[6][0];
	ime_int_reg->ime_register_636.bit.dct_qtbl6_dec_p1 = p_dec_tab[6][1];
	ime_int_reg->ime_register_637.bit.dct_qtbl6_dec_p2 = p_dec_tab[6][2];

	ime_int_reg->ime_register_638.bit.dct_qtbl7_dec_p0 = p_dec_tab[7][0];
	ime_int_reg->ime_register_638.bit.dct_qtbl7_dec_p1 = p_dec_tab[7][1];
	ime_int_reg->ime_register_639.bit.dct_qtbl7_dec_p2 = p_dec_tab[7][2];

	ime_int_reg->ime_register_640.bit.dct_qtbl8_dec_p0 = p_dec_tab[8][0];
	ime_int_reg->ime_register_640.bit.dct_qtbl8_dec_p1 = p_dec_tab[8][1];
	ime_int_reg->ime_register_641.bit.dct_qtbl8_dec_p2 = p_dec_tab[8][2];

	ime_int_reg->ime_register_642.bit.dct_qtbl9_dec_p0 = p_dec_tab[9][0];
	ime_int_reg->ime_register_642.bit.dct_qtbl9_dec_p1 = p_dec_tab[9][1];
	ime_int_reg->ime_register_643.bit.dct_qtbl9_dec_p2 = p_dec_tab[9][2];

	ime_int_reg->ime_register_644.bit.dct_qtbl10_dec_p0 = p_dec_tab[10][0];
	ime_int_reg->ime_register_644.bit.dct_qtbl10_dec_p1 = p_dec_tab[10][1];
	ime_int_reg->ime_register_645.bit.dct_qtbl10_dec_p2 = p_dec_tab[10][2];

	ime_int_reg->ime_register_646.bit.dct_qtbl11_dec_p0 = p_dec_tab[11][0];
	ime_int_reg->ime_register_646.bit.dct_qtbl11_dec_p1 = p_dec_tab[11][1];
	ime_int_reg->ime_register_647.bit.dct_qtbl11_dec_p2 = p_dec_tab[11][2];

	ime_int_reg->ime_register_648.bit.dct_qtbl12_dec_p0 = p_dec_tab[12][0];
	ime_int_reg->ime_register_648.bit.dct_qtbl12_dec_p1 = p_dec_tab[12][1];
	ime_int_reg->ime_register_649.bit.dct_qtbl12_dec_p2 = p_dec_tab[12][2];

	ime_int_reg->ime_register_650.bit.dct_qtbl13_dec_p0 = p_dec_tab[13][0];
	ime_int_reg->ime_register_650.bit.dct_qtbl13_dec_p1 = p_dec_tab[13][1];
	ime_int_reg->ime_register_651.bit.dct_qtbl13_dec_p2 = p_dec_tab[13][2];

	ime_int_reg->ime_register_652.bit.dct_qtbl14_dec_p0 = p_dec_tab[14][0];
	ime_int_reg->ime_register_652.bit.dct_qtbl14_dec_p1 = p_dec_tab[14][1];
	ime_int_reg->ime_register_653.bit.dct_qtbl14_dec_p2 = p_dec_tab[14][2];

	ime_int_reg->ime_register_654.bit.dct_qtbl15_dec_p0 = p_dec_tab[15][0];
	ime_int_reg->ime_register_654.bit.dct_qtbl15_dec_p1 = p_dec_tab[15][1];
	ime_int_reg->ime_register_655.bit.dct_qtbl15_dec_p2 = p_dec_tab[15][2];
#endif
}
//--------------------------------------------------------------


VOID ime_comps_dc_max_reg(UINT32 *p_dc_max)
{
	UINT32 arg_val, i;

	for (i = 0; i < 16; i += 2) {
		arg_val = 0;
		arg_val = (p_dc_max[i + 1] << 16) + (p_dc_max[i]);

		IME_SETREG(IME_YCC_ENCODER_QUALITY_DC_REGISTER0_OFS + (i << 1), arg_val);
	}

#if 0
	ime_int_reg->ime_register_612.bit.dct_qtbl0_dcmax = p_dc_max[0];
	ime_int_reg->ime_register_612.bit.dct_qtbl1_dcmax = p_dc_max[1];

	ime_int_reg->ime_register_613.bit.dct_qtbl2_dcmax = p_dc_max[2];
	ime_int_reg->ime_register_613.bit.dct_qtbl3_dcmax = p_dc_max[3];

	ime_int_reg->ime_register_614.bit.dct_qtbl4_dcmax = p_dc_max[4];
	ime_int_reg->ime_register_614.bit.dct_qtbl5_dcmax = p_dc_max[5];

	ime_int_reg->ime_register_615.bit.dct_qtbl6_dcmax = p_dc_max[6];
	ime_int_reg->ime_register_615.bit.dct_qtbl7_dcmax = p_dc_max[7];

	ime_int_reg->ime_register_616.bit.dct_qtbl8_dcmax = p_dc_max[8];
	ime_int_reg->ime_register_616.bit.dct_qtbl9_dcmax = p_dc_max[9];

	ime_int_reg->ime_register_617.bit.dct_qtbl10_dcmax = p_dc_max[10];
	ime_int_reg->ime_register_617.bit.dct_qtbl11_dcmax = p_dc_max[11];

	ime_int_reg->ime_register_618.bit.dct_qtbl12_dcmax = p_dc_max[12];
	ime_int_reg->ime_register_618.bit.dct_qtbl13_dcmax = p_dc_max[13];

	ime_int_reg->ime_register_619.bit.dct_qtbl14_dcmax = p_dc_max[14];
	ime_int_reg->ime_register_619.bit.dct_qtbl15_dcmax = p_dc_max[15];
#endif
}


#endif

