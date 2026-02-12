
/*
    VPE module driver

    NT98528 VPE module driver.

    @file       vpe_3dnr_base.c
    @ingroup    mIIPPVPE
    @note       None

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include "vpe_int.h"
#include "vpe_compression_base.h"

#if 1
UINT32 const vpe_yuvcomp_dct_dec_qtbl[28][3] = {
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


UINT32 const vpe_yuvcomp_dct_enc_qtbl[28][3] = {
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

UINT32 const vpe_yuvcomp_dct_max[28] = {
	314, 292, 255, 227, 204, 186, 157, 146, 128, 114, 102, 93, 79, 73, 64, 57, 51, 47, 39, 37, 32, 29, 26, 23, 20, 18, 16, 14
};

UINT32 vpe_yuvcomp_q_tbl_idx[16] = {0};




UINT32 vpe_yuv_dct_qtab_encp[16][3] = {0};
UINT32 vpe_yuv_dct_qtab_decp[16][3] = {0};
UINT32 vpe_yuv_dct_qtab_dc[16] = {0};
#endif



static VOID vpe_comps_encoder_parameters_reg(UINT32(*p_enc_tab)[3])
{
	UINT32 arg_val, i;

	if (p_enc_tab != NULL) {
		for (i = 0; i < 16; i++) {
			arg_val = 0;
			arg_val = (p_enc_tab[i][1] << 16) + (p_enc_tab[i][0]);

			VPE_SETREG(ENCODER_Q_TABLE_REGISTER0_OFS + (i << 3), arg_val);
		}

		for (i = 0; i < 16; i++) {
			arg_val = 0;
			arg_val = p_enc_tab[i][2];

			VPE_SETREG(ENCODER_Q_TABLE_REGISTER1_OFS + (i << 3), arg_val);
		}
	}

}
//--------------------------------------------------------------

static VOID vpe_comps_dc_max_reg(UINT32 *p_dc_max)
{
	UINT32 arg_val, i;

	if (p_dc_max != NULL) {
		for (i = 0; i < 16; i += 2) {
			arg_val = 0;
			arg_val = (p_dc_max[i + 1] << 16) + (p_dc_max[i]);

			VPE_SETREG(ENCODER_QUALITY_DC_REGISTER0_OFS + (i << 1), arg_val);
		}
	}

}
//--------------------------------------------------------------

static VOID vpe_comps_decoder_parameters_reg(UINT32(*p_dec_tab)[3])
{
	UINT32 arg_val, i;

	if (p_dec_tab != NULL) {
		for (i = 0; i < 16; i++) {
			arg_val = 0;
			arg_val = (p_dec_tab[i][1] << 16) + (p_dec_tab[i][0]);

			VPE_SETREG(DECODER_QUALITY_TABLE_0_REGISTER0_OFS + (i << 3), arg_val);
		}

		for (i = 0; i < 16; i++) {
			arg_val = 0;
			arg_val = p_dec_tab[i][2];

			VPE_SETREG(DECODER_QUALITY_TABLE_0_REGISTER1_OFS + (i << 3), arg_val);
		}
	}

}
//--------------------------------------------------------------

VOID vpe_set_compression_coefs_reg(VOID)
{
	vpeg->reg_960.bit.res0_enc_smode_en = 0;
	vpeg->reg_960.bit.res1_enc_smode_en = 0;

	vpe_yuvcomp_q_tbl_idx[0] = 1;
	vpe_yuvcomp_q_tbl_idx[1] = 3;
	vpe_yuvcomp_q_tbl_idx[2] = 4;
	vpe_yuvcomp_q_tbl_idx[3] = 5;
	vpe_yuvcomp_q_tbl_idx[4] = 6;
	vpe_yuvcomp_q_tbl_idx[5] = 7;
	vpe_yuvcomp_q_tbl_idx[6] = 8;
	vpe_yuvcomp_q_tbl_idx[7] = 9;
	vpe_yuvcomp_q_tbl_idx[8] = 10;
	vpe_yuvcomp_q_tbl_idx[9] = 11;
	vpe_yuvcomp_q_tbl_idx[10] = 12;
	vpe_yuvcomp_q_tbl_idx[11] = 13;
	vpe_yuvcomp_q_tbl_idx[12] = 14;
	vpe_yuvcomp_q_tbl_idx[13] = 15;
	vpe_yuvcomp_q_tbl_idx[14] = 16;
	vpe_yuvcomp_q_tbl_idx[15] = 19;

	vpe_yuv_dct_qtab_encp[0][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[0]][0];
	vpe_yuv_dct_qtab_encp[0][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[0]][1];
	vpe_yuv_dct_qtab_encp[0][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[0]][2];
	vpe_yuv_dct_qtab_encp[1][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[1]][0];
	vpe_yuv_dct_qtab_encp[1][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[1]][1];
	vpe_yuv_dct_qtab_encp[1][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[1]][2];
	vpe_yuv_dct_qtab_encp[2][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[2]][0];
	vpe_yuv_dct_qtab_encp[2][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[2]][1];
	vpe_yuv_dct_qtab_encp[2][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[2]][2];
	vpe_yuv_dct_qtab_encp[3][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[3]][0];
	vpe_yuv_dct_qtab_encp[3][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[3]][1];
	vpe_yuv_dct_qtab_encp[3][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[3]][2];
	vpe_yuv_dct_qtab_encp[4][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[4]][0];
	vpe_yuv_dct_qtab_encp[4][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[4]][1];
	vpe_yuv_dct_qtab_encp[4][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[4]][2];
	vpe_yuv_dct_qtab_encp[5][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[5]][0];
	vpe_yuv_dct_qtab_encp[5][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[5]][1];
	vpe_yuv_dct_qtab_encp[5][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[5]][2];
	vpe_yuv_dct_qtab_encp[6][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[6]][0];
	vpe_yuv_dct_qtab_encp[6][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[6]][1];
	vpe_yuv_dct_qtab_encp[6][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[6]][2];
	vpe_yuv_dct_qtab_encp[7][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[7]][0];
	vpe_yuv_dct_qtab_encp[7][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[7]][1];
	vpe_yuv_dct_qtab_encp[7][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[7]][2];
	vpe_yuv_dct_qtab_encp[8][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[8]][0];
	vpe_yuv_dct_qtab_encp[8][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[8]][1];
	vpe_yuv_dct_qtab_encp[8][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[8]][2];
	vpe_yuv_dct_qtab_encp[9][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[9]][0];
	vpe_yuv_dct_qtab_encp[9][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[9]][1];
	vpe_yuv_dct_qtab_encp[9][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[9]][2];
	vpe_yuv_dct_qtab_encp[10][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[10]][0];
	vpe_yuv_dct_qtab_encp[10][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[10]][1];
	vpe_yuv_dct_qtab_encp[10][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[10]][2];
	vpe_yuv_dct_qtab_encp[11][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[11]][0];
	vpe_yuv_dct_qtab_encp[11][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[11]][1];
	vpe_yuv_dct_qtab_encp[11][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[11]][2];
	vpe_yuv_dct_qtab_encp[12][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[12]][0];
	vpe_yuv_dct_qtab_encp[12][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[12]][1];
	vpe_yuv_dct_qtab_encp[12][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[12]][2];
	vpe_yuv_dct_qtab_encp[13][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[13]][0];
	vpe_yuv_dct_qtab_encp[13][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[13]][1];
	vpe_yuv_dct_qtab_encp[13][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[13]][2];
	vpe_yuv_dct_qtab_encp[14][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[14]][0];
	vpe_yuv_dct_qtab_encp[14][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[14]][1];
	vpe_yuv_dct_qtab_encp[14][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[14]][2];
	vpe_yuv_dct_qtab_encp[15][0] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[15]][0];
	vpe_yuv_dct_qtab_encp[15][1] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[15]][1];
	vpe_yuv_dct_qtab_encp[15][2] = vpe_yuvcomp_dct_enc_qtbl[vpe_yuvcomp_q_tbl_idx[15]][2];


	vpe_yuv_dct_qtab_decp[0][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[0]][0];
	vpe_yuv_dct_qtab_decp[0][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[0]][1];
	vpe_yuv_dct_qtab_decp[0][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[0]][2];
	vpe_yuv_dct_qtab_decp[1][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[1]][0];
	vpe_yuv_dct_qtab_decp[1][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[1]][1];
	vpe_yuv_dct_qtab_decp[1][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[1]][2];
	vpe_yuv_dct_qtab_decp[2][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[2]][0];
	vpe_yuv_dct_qtab_decp[2][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[2]][1];
	vpe_yuv_dct_qtab_decp[2][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[2]][2];
	vpe_yuv_dct_qtab_decp[3][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[3]][0];
	vpe_yuv_dct_qtab_decp[3][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[3]][1];
	vpe_yuv_dct_qtab_decp[3][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[3]][2];
	vpe_yuv_dct_qtab_decp[4][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[4]][0];
	vpe_yuv_dct_qtab_decp[4][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[4]][1];
	vpe_yuv_dct_qtab_decp[4][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[4]][2];
	vpe_yuv_dct_qtab_decp[5][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[5]][0];
	vpe_yuv_dct_qtab_decp[5][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[5]][1];
	vpe_yuv_dct_qtab_decp[5][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[5]][2];
	vpe_yuv_dct_qtab_decp[6][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[6]][0];
	vpe_yuv_dct_qtab_decp[6][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[6]][1];
	vpe_yuv_dct_qtab_decp[6][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[6]][2];
	vpe_yuv_dct_qtab_decp[7][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[7]][0];
	vpe_yuv_dct_qtab_decp[7][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[7]][1];
	vpe_yuv_dct_qtab_decp[7][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[7]][2];
	vpe_yuv_dct_qtab_decp[8][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[8]][0];
	vpe_yuv_dct_qtab_decp[8][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[8]][1];
	vpe_yuv_dct_qtab_decp[8][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[8]][2];
	vpe_yuv_dct_qtab_decp[9][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[9]][0];
	vpe_yuv_dct_qtab_decp[9][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[9]][1];
	vpe_yuv_dct_qtab_decp[9][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[9]][2];
	vpe_yuv_dct_qtab_decp[10][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[10]][0];
	vpe_yuv_dct_qtab_decp[10][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[10]][1];
	vpe_yuv_dct_qtab_decp[10][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[10]][2];
	vpe_yuv_dct_qtab_decp[11][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[11]][0];
	vpe_yuv_dct_qtab_decp[11][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[11]][1];
	vpe_yuv_dct_qtab_decp[11][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[11]][2];
	vpe_yuv_dct_qtab_decp[12][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[12]][0];
	vpe_yuv_dct_qtab_decp[12][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[12]][1];
	vpe_yuv_dct_qtab_decp[12][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[12]][2];
	vpe_yuv_dct_qtab_decp[13][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[13]][0];
	vpe_yuv_dct_qtab_decp[13][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[13]][1];
	vpe_yuv_dct_qtab_decp[13][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[13]][2];
	vpe_yuv_dct_qtab_decp[14][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[14]][0];
	vpe_yuv_dct_qtab_decp[14][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[14]][1];
	vpe_yuv_dct_qtab_decp[14][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[14]][2];
	vpe_yuv_dct_qtab_decp[15][0] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[15]][0];
	vpe_yuv_dct_qtab_decp[15][1] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[15]][1];
	vpe_yuv_dct_qtab_decp[15][2] = vpe_yuvcomp_dct_dec_qtbl[vpe_yuvcomp_q_tbl_idx[15]][2];

	vpe_yuv_dct_qtab_dc[0] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[0]];
	vpe_yuv_dct_qtab_dc[1] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[1]];
	vpe_yuv_dct_qtab_dc[2] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[2]];
	vpe_yuv_dct_qtab_dc[3] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[3]];
	vpe_yuv_dct_qtab_dc[4] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[4]];
	vpe_yuv_dct_qtab_dc[5] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[5]];
	vpe_yuv_dct_qtab_dc[6] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[6]];
	vpe_yuv_dct_qtab_dc[7] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[7]];
	vpe_yuv_dct_qtab_dc[8] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[8]];
	vpe_yuv_dct_qtab_dc[9] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[9]];
	vpe_yuv_dct_qtab_dc[10] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[10]];
	vpe_yuv_dct_qtab_dc[11] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[11]];
	vpe_yuv_dct_qtab_dc[12] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[12]];
	vpe_yuv_dct_qtab_dc[13] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[13]];
	vpe_yuv_dct_qtab_dc[14] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[14]];
	vpe_yuv_dct_qtab_dc[15] = vpe_yuvcomp_dct_max[vpe_yuvcomp_q_tbl_idx[15]];

	vpe_comps_encoder_parameters_reg(vpe_yuv_dct_qtab_encp);
	vpe_comps_decoder_parameters_reg(vpe_yuv_dct_qtab_decp);
	vpe_comps_dc_max_reg(vpe_yuv_dct_qtab_dc);
}

