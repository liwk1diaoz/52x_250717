#include <string.h>

#include "vendor_isp.h"

//============================================================================
// global
//============================================================================
static int get_choose_int(void)
{
	CHAR buf[256];
	int val = 0, error;

	do {
		error = scanf("%d", &val);
		if (error != 1) {
			printf("Invalid option. Try again.\n");
			clearerr(stdin);
			fgets(buf, sizeof(buf), stdin);
			printf(">> ");
		}
	} while (error != 1);

	return val;
}

int main(int argc, char *argv[])
{
	int option = 0;
	UINT32 i, j;
	UINT32 trig = 1;
	UINT32 tmp = 0;
	UINT32 version = 0;
	HD_RESULT result = HD_OK;
	AWBT_CFG_INFO cfg_info = {0};
	AWBT_DTSI_INFO dtsi_info = {0};
	AWBT_SCENE_MODE scene = {0};
	AWBT_WB_RATIO wb_ratio = {0};
	AWBT_OPERATION operation = {0};
	AWBT_STITCH_ID stitch = {0};
	AWBT_CA_TH ca_th = {0};
	AWBT_TH th = {0};
	AWBT_LV lv = {0};
	AWBT_CT_WEIGHT ct_weight = {0};
	AWBT_TARGET target = {0};
	AWBT_CT_INFO ct_info = {0};
	AWBT_MWB_GAIN mwb = {0};
	AWBT_CONVERGE converge = {0};
	AWBT_EXPAND_BLOCK expand_blcok = {0};
	AWBT_MANUAL manual = {0};
	AWBT_STATUS status = {0};

	static AWBT_CA ca = {0};
	AWBT_FLAG flag = {0};
	AWBT_LUMA_WEIGHT luma_weight = {0};
	AWBT_KGAIN_RATIO kgain = {0};
	AWBT_CT_TO_CGAIN ct_to_cgain = {0};
	AWBT_WIN win = {0};
	AWBT_EXPAND_CT_ENABLE expand_ct = {0};
	AWBT_AUTO_WEIGHT_ENABLE auto_weight = {0};
	AWBT_GRAY_WORLD_ENABLE gray_world = {0};
	int ret = 0;

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
			printf("----------------------------------------\n");
			printf("  1. Get version \n");
			printf("  2. Reload config file \n");
			printf("  3. Reload dtsi file \n");
			printf(" 10. Get scene mode \n");
			printf(" 11. Get wb ratio \n");
			printf(" 12. Get operation \n");
			printf(" 13. Get stitch id \n");
			printf(" 30. Get ca th \n");
			printf(" 31. Get th \n");
			printf(" 32. Get lv \n");
			printf(" 33. Get ct weight \n");
			printf(" 34. Get target \n");
			printf(" 35. Get ct info \n");
			printf(" 36. Get mwb \n");
			printf(" 37. Get converge \n");
			printf(" 38. Get expand_blcok \n");
			printf(" 39. Get manual \n");
			printf(" 40. Get status \n");
			printf(" 41. Get ca \n");
			printf(" 42. Get flag \n");
			printf(" 43. Get luma weight \n");
			printf(" 44. Get kgain ratio \n");
			printf(" 45. ct to color gain \n");
			printf(" 46. Get win \n");
			printf(" 47. Get expand ct enable \n");
			printf(" 48. Get auto weight enable \n");
			printf(" 49. Get gray world enable \n");
			printf(" 60. Set scene mode \n");
			printf(" 61. Set wb ratio \n");
			printf(" 62. Set operation \n");
			printf(" 63. Set stitch id \n");
			printf(" 74. Set target \n");
			printf(" 77. Set converge \n");
			printf(" 79. Set manual \n");
			printf(" 84. Set kgain ratio \n");
			printf(" 86. Set win \n");
			printf(" 87. Set expand ct enable \n");
			printf(" 88. Set auto weight enable \n");
			printf(" 89. Set gray world enable \n");
			printf("  0. Quit\n");
			printf("----------------------------------------\n");

		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {
		case 1:
			result = vendor_isp_get_awb(AWBT_ITEM_VERSION, &version);
			if (result == HD_OK) {
				printf("version = 0x%X \n", version);
			} else {
				printf("Get AWBT_ITEM_VERSION fail \n");
			}
			break;

		case 2:
			do {
				printf("Set isp id (0, 1)>> \n");
				cfg_info.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Select chg file>> \n");
				printf("1: isp_os02k10_0.cfg \n");
				printf("2: isp_ov2718_0.cfg \n");
				printf("3: isp_imx290_0.cfg \n");
				tmp = (UINT32)get_choose_int();
			} while (0);

			switch (tmp) {
			case 1:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0.cfg", CFG_NAME_LENGTH);
				break;

			case 2:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_ov2718_0.cfg", CFG_NAME_LENGTH);
				break;

			case 3:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
				break;

			default:
				printf("Not support item (%d) \n", tmp);
				break;
			}
			result = vendor_isp_set_awb(AWBT_ITEM_RLD_CONFIG, &cfg_info);
			if (result != HD_OK) {
				printf("Set AWBT_ITEM_RLD_CONFIG fail \n");
			}
			break;

		case 3:
			do {
				printf("Set isp id (0, 1)>> \n");
				dtsi_info.id = (UINT32)get_choose_int();
			} while (0);

			strncpy(dtsi_info.node_path, "/isp/awb/imx290_awb_0", DTSI_NAME_LENGTH);
			strncpy(dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
			dtsi_info.buf_addr = NULL;
			result = vendor_isp_set_awb(AWBT_ITEM_RLD_DTSI, &dtsi_info);
			if (result != HD_OK) {
				printf("Set AWBT_ITEM_RLD_DTSI fail \n");
			}
			break;

		case 10:
			do {
				printf("Set isp id (0, 1)>> \n");
				scene.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_SCENE, &scene);
			if (result == HD_OK) {
				printf("id = %d, scene mode = %d \n", scene.id, scene.mode);
			} else {
				printf("Get AWBT_ITEM_SCENE fail \n");
			}
			break;

		case 11:
			do {
				printf("Set isp id (0, 1)>> \n");
				wb_ratio.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_WB_RATIO, &wb_ratio);
			if (result == HD_OK) {
				printf("id = %d, wb R ratio = %d, wb B ratio = %d  \n", wb_ratio.id, wb_ratio.r, wb_ratio.b);
			} else {
				printf("Get AWBT_ITEM_WB_RATIO fail \n");
			}
			break;

		case 12:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_OPERATION, &operation);
			if (result == HD_OK) {
				printf("id = %d, operation = %d \n", operation.id, operation.operation);
			} else {
				printf("Get AWBT_ITEM_OPERATION fail \n");
			}
			break;

		case 13:
			do {
				printf("Set isp id (0, 1)>> \n");
				stitch.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_STITCH_ID, &stitch);
			if (result == HD_OK) {
				printf("id = %d, stitch id = 0x%x \n", stitch.id, stitch.stitch_id);
			} else {
				printf("Get AWBT_ITEM_STITCH_ID fail \n");
			}
			break;

		case 30:
			do {
				printf("Set isp id (0, 1)>> \n");
				ca_th.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CA_TH, &ca_th);
			if (result == HD_OK) {
				printf("id : %d, en : %d, mode : %d  , g(%d,%d), r(%d,%d), b(%d,%d), p(%d,%d) \n", ca_th.id, ca_th.ca_th.enable, ca_th.ca_th.mode,
			ca_th.ca_th.g_l, ca_th.ca_th.g_u, ca_th.ca_th.r_l, ca_th.ca_th.r_u, ca_th.ca_th.b_l, ca_th.ca_th.b_u, ca_th.ca_th.p_l, ca_th.ca_th.p_u);
			} else {
				printf("Get AWBT_ITEM_CA_TH fail \n");
			}
			break;

		case 31:
			do {
				printf("Set isp id (0, 1)>> \n");
				th.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_TH, &th);
			if (result == HD_OK) {
				printf("id : %d, y(%d,%d), rpb(%d,%d), rsb(%d,%d), r2g(%d,%d), b2g(%d,%d), rmb(%d,%d)\n", th.id,
					th.th.y_l, th.th.y_u, th.th.rpb_l, th.th.rpb_u, th.th.rsb_l, th.th.rsb_u,
					th.th.r2g_l, th.th.r2g_u, th.th.b2g_l, th.th.b2g_u, th.th.rmb_l, th.th.rmb_u);
			} else {
				printf("Get AWBT_ITEM_TH fail \n");
			}
			break;

		case 32:
			do {
				printf("Set isp id (0, 1)>> \n");
				lv.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_LV, &lv);
			if (result == HD_OK) {
				printf("id : %d, night(%d,%d), in(%d,%d), out(%d,%d)\n", lv.id,
					lv.lv.night_l, lv.lv.night_h, lv.lv.in_l, lv.lv.in_h, lv.lv.out_l, lv.lv.out_h);
			} else {
				printf("Get AWBT_ITEM_LV fail \n");
			}
			break;

		case 33:
			do {
				printf("Set isp id (0, 1)>> \n");
				ct_weight.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CT_WEIGHT, &ct_weight);
			if (result == HD_OK) {
				printf("id : %d \n", ct_weight.id);
				printf("ctmp    : %4d, %4d, %4d, %4d, %4d, %4d\n", ct_weight.ct_weight.ctmp[0], ct_weight.ct_weight.ctmp[1],
					 ct_weight.ct_weight.ctmp[2], ct_weight.ct_weight.ctmp[3], ct_weight.ct_weight.ctmp[4], ct_weight.ct_weight.ctmp[5]);
				printf("cx      : %4d, %4d, %4d, %4d, %4d, %4d\n", ct_weight.ct_weight.cx[0], ct_weight.ct_weight.cx[1],
					 ct_weight.ct_weight.cx[2], ct_weight.ct_weight.cx[3], ct_weight.ct_weight.cx[4], ct_weight.ct_weight.cx[5]);
				printf("out_w   : %4d, %4d, %4d, %4d, %4d, %4d\n", ct_weight.ct_weight.out_weight[0], ct_weight.ct_weight.out_weight[1],
					 ct_weight.ct_weight.out_weight[2], ct_weight.ct_weight.out_weight[3], ct_weight.ct_weight.out_weight[4], ct_weight.ct_weight.out_weight[5]);
				printf("in_w    : %4d, %4d, %4d, %4d, %4d, %4d\n", ct_weight.ct_weight.in_weight[0], ct_weight.ct_weight.in_weight[1],
					 ct_weight.ct_weight.in_weight[2], ct_weight.ct_weight.in_weight[3], ct_weight.ct_weight.in_weight[4], ct_weight.ct_weight.in_weight[5]);
				printf("night_w : %4d, %4d, %4d, %4d, %4d, %4d\n", ct_weight.ct_weight.night_weight[0], ct_weight.ct_weight.night_weight[1],
					 ct_weight.ct_weight.night_weight[2], ct_weight.ct_weight.night_weight[3], ct_weight.ct_weight.night_weight[4], ct_weight.ct_weight.night_weight[5]);
			} else {
				printf("Get AWBT_ITEM_CT_WEIGHT fail \n");
			}
			break;

		case 34:
			do {
				printf("Set isp id (0, 1)>> \n");
				target.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_TARGET, &target);
			if (result == HD_OK) {
				printf("id : %d \n", target.id);
				printf("CT : 2800, 4700, 6500 \n");
				printf("cx : %4d, %4d, %4d \n", target.target.cx[0], target.target.cx[1], target.target.cx[2]);
				printf("rg : %4d, %4d, %4d \n", target.target.rg_ratio[0], target.target.rg_ratio[1], target.target.rg_ratio[2]);
				printf("bg : %4d, %4d, %4d \n", target.target.bg_ratio[0], target.target.bg_ratio[1], target.target.bg_ratio[2]);
			} else {
				printf("Get AWBT_ITEM_TARGET fail \n");
			}
			break;

		case 35:
			do {
				printf("Set isp id (0, 1)>> \n");
				ct_info.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CT_INFO, &ct_info);
			if (result == HD_OK) {
				printf("id : %d \n", ct_info.id);
				printf("temperature : %4d, %4d, %4d, %4d, %4d, %4d \n", ct_info.ct_info.temperature[0], ct_info.ct_info.temperature[1],
					ct_info.ct_info.temperature[2], ct_info.ct_info.temperature[3], ct_info.ct_info.temperature[4], ct_info.ct_info.temperature[5]);
				printf("r_gain      : %4d, %4d, %4d, %4d, %4d, %4d \n", ct_info.ct_info.r_gain[0], ct_info.ct_info.r_gain[1],
					ct_info.ct_info.r_gain[2], ct_info.ct_info.r_gain[3], ct_info.ct_info.r_gain[4], ct_info.ct_info.r_gain[5]);
				printf("g_gain      : %4d, %4d, %4d, %4d, %4d, %4d \n", ct_info.ct_info.g_gain[0], ct_info.ct_info.g_gain[1],
					ct_info.ct_info.g_gain[2], ct_info.ct_info.g_gain[3], ct_info.ct_info.g_gain[4], ct_info.ct_info.g_gain[5]);
				printf("b_gain      : %4d, %4d, %4d, %4d, %4d, %4d \n", ct_info.ct_info.b_gain[0], ct_info.ct_info.b_gain[1],
					ct_info.ct_info.b_gain[2], ct_info.ct_info.b_gain[3], ct_info.ct_info.b_gain[4], ct_info.ct_info.b_gain[5]);
			} else {
				printf("AWBT_ITEM_CT_INFO fail \n");
			}

			break;

		case 36:
			do {
				printf("Set isp id (0, 1)>> \n");
				mwb.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_MWB_GAIN, &mwb);
			if (result == HD_OK) {
				printf("id : %d \n", mwb.id);
				printf("r_gain : %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \n",  mwb.mwb_gain.r_gain[0], mwb.mwb_gain.r_gain[1],
					 mwb.mwb_gain.r_gain[2], mwb.mwb_gain.r_gain[3], mwb.mwb_gain.r_gain[4], mwb.mwb_gain.r_gain[5], mwb.mwb_gain.r_gain[6],
					  mwb.mwb_gain.r_gain[7], mwb.mwb_gain.r_gain[8], mwb.mwb_gain.r_gain[9], mwb.mwb_gain.r_gain[10], mwb.mwb_gain.r_gain[11]);
				printf("g_gain : %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \n",  mwb.mwb_gain.g_gain[0], mwb.mwb_gain.g_gain[1],
					 mwb.mwb_gain.g_gain[2], mwb.mwb_gain.g_gain[3], mwb.mwb_gain.g_gain[4], mwb.mwb_gain.g_gain[5], mwb.mwb_gain.g_gain[6],
					  mwb.mwb_gain.g_gain[7], mwb.mwb_gain.g_gain[8], mwb.mwb_gain.g_gain[9], mwb.mwb_gain.g_gain[10], mwb.mwb_gain.g_gain[11]);
				printf("b_gain : %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d \n",  mwb.mwb_gain.b_gain[0], mwb.mwb_gain.b_gain[1],
					 mwb.mwb_gain.b_gain[2], mwb.mwb_gain.b_gain[3], mwb.mwb_gain.b_gain[4], mwb.mwb_gain.b_gain[5], mwb.mwb_gain.b_gain[6],
					  mwb.mwb_gain.b_gain[7], mwb.mwb_gain.b_gain[8], mwb.mwb_gain.b_gain[9], mwb.mwb_gain.b_gain[10], mwb.mwb_gain.b_gain[11]);
			} else {
				printf("Get AWBT_ITEM_MWB_GAIN fail \n");
			}
			break;

		case 37:
			do {
				printf("Set isp id (0, 1)>> \n");
				converge.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CONVERGE, &converge);
			if (result == HD_OK) {
				printf("id = %d, skip_frame = %d, speed = %d, tolerance = %d \n", converge.id,
					converge.converge.skip_frame, converge.converge.speed, converge.converge.tolerance);
			} else {
				printf("Get AWBT_ITEM_CONVERGE fail \n");
			}
			break;

		case 38:
			do {
				printf("Set isp id (0, 1)>> \n");
				expand_blcok.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_EXPAND_BLOCK, &expand_blcok);
			if (result == HD_OK) {
				printf("id = %d \n", expand_blcok.id);
				printf("expand_block \n");
				printf("mode  %4d %4d %4d %4d\n", expand_blcok.expand_block.mode[0], expand_blcok.expand_block.mode[1]
					, expand_blcok.expand_block.mode[2], expand_blcok.expand_block.mode[3]);
				printf("lv_l  %4d %4d %4d %4d\n", expand_blcok.expand_block.lv_l[0], expand_blcok.expand_block.lv_l[1]
					, expand_blcok.expand_block.lv_l[2], expand_blcok.expand_block.lv_l[3]);
				printf("lv_h  %4d %4d %4d %4d\n", expand_blcok.expand_block.lv_h[0], expand_blcok.expand_block.lv_h[1]
					, expand_blcok.expand_block.lv_h[2], expand_blcok.expand_block.lv_h[3]);
				printf("y_l   %4d %4d %4d %4d\n", expand_blcok.expand_block.y_l[0], expand_blcok.expand_block.y_l[1]
					, expand_blcok.expand_block.y_l[2], expand_blcok.expand_block.y_l[3]);
				printf("y_u   %4d %4d %4d %4d\n", expand_blcok.expand_block.y_u[0], expand_blcok.expand_block.y_u[1]
					, expand_blcok.expand_block.y_u[2], expand_blcok.expand_block.y_u[3]);
				printf("rpb_l %4d %4d %4d %4d\n", expand_blcok.expand_block.rpb_l[0], expand_blcok.expand_block.rpb_l[1]
					, expand_blcok.expand_block.rpb_l[2], expand_blcok.expand_block.rpb_l[3]);
				printf("rpb_u %4d %4d %4d %4d\n", expand_blcok.expand_block.rpb_u[0], expand_blcok.expand_block.rpb_u[1]
					, expand_blcok.expand_block.rpb_u[2], expand_blcok.expand_block.rpb_u[3]);
				printf("rsb_l %4d %4d %4d %4d\n", expand_blcok.expand_block.rsb_l[0], expand_blcok.expand_block.rsb_l[1]
					, expand_blcok.expand_block.rsb_l[2], expand_blcok.expand_block.rsb_l[3]);
				printf("rsb_u %4d %4d %4d %4d\n", expand_blcok.expand_block.rsb_u[0], expand_blcok.expand_block.rsb_u[1]
					, expand_blcok.expand_block.rsb_u[2], expand_blcok.expand_block.rsb_u[3]);
			} else {
				printf("Get AWBT_ITEM_EXPAND_BLOCK fail \n");
			}
			break;

		case 39:
			do {
				printf("Set isp id (0, 1)>> \n");
				manual.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_MANUAL, &manual);
			if (result == HD_OK) {
				printf("id = %d, en = %d, r_gain = %d, g_gain = %d, b_gain = %d \n", manual.id,
					manual.manual.en, manual.manual.r_gain, manual.manual.g_gain, manual.manual.b_gain);
			} else {
				printf("Get AWBT_ITEM_MANUAL fail \n");
			}
			break;

		case 40:
			do {
				printf("Set isp id (0, 1)>> \n");
				status.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_STATUS, &status);
			if (result == HD_OK) {
				printf("id = %d, cur_r_gain = %d, cur_g_gain = %d, cur_b_gain = %d, cur_ct = %d, cur_cx = %d, mode = %d\n",
					status.id, status.status.cur_r_gain, status.status.cur_g_gain, status.status.cur_b_gain,
					 status.status.cur_ct, status.status.cur_cx, status.status.mode);
			} else {
				printf("Get AWBT_ITEM_STATUS fail \n");
			}
			break;

		case 41:
			do {
				printf("Set isp id (0, 1)>> \n");
				ca.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CA, &ca);
			if (result == HD_OK) {
				printf("id = %d \n", ca.id);
				for (j = 0; j < ca.ca.win_num_y; j++) {
					printf("RGB  \t: ");
					for (i = 0; i < ca.ca.win_num_x; i++) {
						printf("%4d %4d %4d %4d\t", ca.ca.tab[AWBALG_CH_R][j][i], ca.ca.tab[AWBALG_CH_G][j][i], ca.ca.tab[AWBALG_CH_B][j][i], ca.ca.tab[AWBALG_ACC_CNT][j][i]);
					}
					printf("\r\n");
				}
			} else {
				printf("Get AWBT_ITEM_CA fail \n");
			}
			break;

		case 42:
			do {
				printf("Set isp id (0, 1)>> \n");
				flag.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_FLAG, &flag);
			if (result == HD_OK) {
				printf("id = %d \n", flag.id);
				for (j = 0; j < flag.flag.win_num_y; j++) {
					printf("flag \t: ");
					for (i = 0; i < flag.flag.win_num_x; i++) {
						printf("%4d\t", flag.flag.tab[j][i]);
					}
					printf("\r\n");
				}
			} else {
				printf("Get AWBT_ITEM_FLAG fail \n");
			}
			break;

		case 43:
			do {
				printf("Set isp id (0, 1)>> \n");
				luma_weight.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_LUMA_WEIGHT, &luma_weight);
			if (result == HD_OK) {
				printf("id = %d \n", luma_weight.id);
				printf("luma weight  \n");
				printf("en %4d \n", luma_weight.luma_weight.en);
				printf("y  %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d\n", luma_weight.luma_weight.y[0], luma_weight.luma_weight.y[1],
					luma_weight.luma_weight.y[2], luma_weight.luma_weight.y[3], luma_weight.luma_weight.y[4],
					luma_weight.luma_weight.y[5], luma_weight.luma_weight.y[6], luma_weight.luma_weight.y[7]);
				printf("w  %4d, %4d, %4d, %4d, %4d, %4d, %4d, %4d\n", luma_weight.luma_weight.w[0], luma_weight.luma_weight.w[1],
					luma_weight.luma_weight.w[2], luma_weight.luma_weight.w[3], luma_weight.luma_weight.w[4],
					luma_weight.luma_weight.w[5], luma_weight.luma_weight.w[6], luma_weight.luma_weight.w[7]);

			} else {
				printf("Get AWBT_ITEM_LUMA_WEIGHT fail \n");
			}
			break;

		case 44:
			do {
				printf("Set isp id (0, 1)>> \n");
				kgain.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_KGAIN_RATIO, &kgain);
			if (result == HD_OK) {
				printf("id = %d, Rratio = %d, Bratio = %d \n", kgain.id, kgain.k_gain.r_ratio, kgain.k_gain.b_ratio);
			} else {
				printf("Get AWBT_ITEM_LUMA_WEIGHT fail \n");
			}
			break;

		case 45:
			do {
				printf("Set isp id (0, 1)>> \n");
				kgain.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("0719Set color temperature (2300~11000)>> \n");
				ct_to_cgain.ct_to_cgain.ct = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CT_TO_CGAIN, &ct_to_cgain);
			if (result == HD_OK) {
				printf("id = %d, CT = %d, Rgain = %d, Ggain = %d, Bgain = %d \n", ct_to_cgain.id, ct_to_cgain.ct_to_cgain.ct,
					ct_to_cgain.ct_to_cgain.r_gain,	ct_to_cgain.ct_to_cgain.g_gain, ct_to_cgain.ct_to_cgain.b_gain);
			} else {
				printf("Get AWBT_ITEM_CT_TO_CGAIN fail \n");
			}
			break;

		case 46:
			do {
				printf("Set isp id (0, 1)>> \n");
				win.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_WIN, &win);
			if (result == HD_OK) {
				printf("id = %d \n", win.id);
				for (j = 0; j < 32; j++) {
					printf("win \t: ");
					for (i = 0; i < 32; i++) {
						printf("%4d\t", win.win.tab[j][i]);
					}
					printf("\r\n");
				}
			} else {
				printf("Get AWBT_ITEM_WIN fail \n");
			}
			break;

		case 47:
			do {
				printf("Set isp id (0, 1)>> \n");
				expand_ct.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_EXPAND_CT_ENABLE, &expand_ct);
			if (result == HD_OK) {
				printf("id = %d , enable = %d\n", expand_ct.id, expand_ct.enable);
			} else {
				printf("Get AWBT_ITEM_EXPAND_CT_ENABLE fail \n");
			}
			break;

		case 48:
			do {
				printf("Set isp id (0, 1)>> \n");
				auto_weight.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_AUTO_WEIGHT_ENABLE, &auto_weight);
			if (result == HD_OK) {
				printf("id = %d , enable = %d\n", auto_weight.id, auto_weight.enable);
			} else {
				printf("Get AWBT_ITEM_AUTO_WEIGHT_ENABLE fail \n");
			}
			break;

		case 49:
			do {
				printf("Set isp id (0, 1)>> \n");
				gray_world.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_GRAY_WORLD_ENABLE, &gray_world);
			if (result == HD_OK) {
				printf("id = %d , enable = %d\n", gray_world.id, gray_world.enable);
			} else {
				printf("Get AWBT_ITEM_GRAY_WORLD_ENABLE fail \n");
			}
			break;

		case 60:
			do {
				printf("Set isp id (0, 1)>> \n");
				scene.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set scene mode (0 ~ %d)>> \n", AWB_SCENE_MAX_CNT-1);
				scene.mode = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_SCENE, &scene);
			if (result == HD_OK) {
				printf("id = %d, scene mode = %d \n", scene.id, scene.mode);
			} else {
				printf("Set AWBT_ITEM_SCENE fail \n");
			}
			break;

		case 61:
			do {
				printf("Set isp id (0, 1)>> \n");
				wb_ratio.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set wb R ratio (1 ~ 400)>> \n");
				wb_ratio.r = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set wb B ratio (1 ~ 400)>> \n");
				wb_ratio.b = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_WB_RATIO, &wb_ratio);
			if (result == HD_OK) {
				printf("id = %d, wb R ratio = %d, wb B ratio = %d  n", wb_ratio.id, wb_ratio.r, wb_ratio.b);
			} else {
				printf("Set AWBT_ITEM_WB_RATIO fail \n");
			}
			break;

		case 62:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set operation (0 ~ %d)>> \n", AWB_OPERATION_MAX_CNT-1);
				operation.operation = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_OPERATION, &operation);
			if (result == HD_OK) {
				printf("id = %d, operation = %d \n", operation.id, operation.operation);
			} else {
				printf("Set AWBT_ITEM_OPERATION fail \n");
			}
			break;

		case 63:
			do {
				printf("Set isp id (0, 1)>> \n");
				stitch.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set left id >> \n");
				stitch.stitch_id = (UINT32)get_choose_int();
			} while (0);

			stitch.stitch_id = stitch.stitch_id << 16;

			do {
				printf("Set right id >> \n");
				stitch.stitch_id = stitch.stitch_id + (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_STITCH_ID, &stitch);
			if (result == HD_OK) {
				printf("id = %d, stitch id = 0x%x \n", stitch.id, stitch.stitch_id);
			} else {
				printf("Set AWBT_ITEM_STITCH_ID fail \n");
			}
			break;

		case 74:
			do {
				printf("Set isp id (0, 1)>> \n");
				target.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_TARGET, &target);

			if (result == HD_OK) {
				printf("id : %d \n", target.id);
				printf("CT : 2800, 4700, 6500 \n");
				printf("cx : %4d, %4d, %4d \n", target.target.cx[0], target.target.cx[1], target.target.cx[2]);
				printf("rg : %4d, %4d, %4d \n", target.target.rg_ratio[0], target.target.rg_ratio[1], target.target.rg_ratio[2]);
				printf("bg : %4d, %4d, %4d \n", target.target.bg_ratio[0], target.target.bg_ratio[1], target.target.bg_ratio[2]);
			} else {
				printf("Get AWBT_ITEM_TARGET fail \n");
			}

			do {
				printf("Set Hight CT2800 rg >> \n");
				target.target.rg_ratio[0] = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set Hight CT2800 bg >> \n");
				target.target.bg_ratio[0] = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set Hight CT4700 rg >> \n");
				target.target.rg_ratio[1] = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set Hight CT4700 bg >> \n");
				target.target.bg_ratio[1] = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set Hight CT6500 rg >> \n");
				target.target.rg_ratio[2] = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set Hight CT6500 bg >> \n");
				target.target.bg_ratio[2] = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_TARGET, &target);

			if (result == HD_OK) {
				printf("id : %d \n", target.id);
				printf("CT : 2800, 4700, 6500 \n");
				printf("cx : %4d, %4d, %4d \n", target.target.cx[0], target.target.cx[1], target.target.cx[2]);
				printf("rg : %4d, %4d, %4d \n", target.target.rg_ratio[0], target.target.rg_ratio[1], target.target.rg_ratio[2]);
				printf("bg : %4d, %4d, %4d \n", target.target.bg_ratio[0], target.target.bg_ratio[1], target.target.bg_ratio[2]);
			} else {
				printf("Set AWBT_ITEM_TARGET fail \n");
			}
			break;

		case 77:
			do {
				printf("Set isp id (0, 1)>> \n");
				converge.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_CONVERGE, &converge);
			if (result == HD_OK) {
				printf("id = %d, skip_frame = %d, speed = %d, tolerance = %d \n", converge.id,
					converge.converge.skip_frame, converge.converge.speed, converge.converge.tolerance);
			} else {
				printf("Get AWBT_ITEM_CONVERGE fail \n");
			}

			do {
				printf("Set skip_frame >> \n");
				converge.converge.skip_frame = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set speed >> \n");
				converge.converge.speed = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set tolerance >> \n");
				converge.converge.tolerance = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_CONVERGE, &converge);
			if (result == HD_OK) {
				printf("id = %d, skip_frame = %d, speed = %d, tolerance = %d \n", converge.id,
					converge.converge.skip_frame, converge.converge.speed, converge.converge.tolerance);
			} else {
				printf("Set AWBT_ITEM_CONVERGE fail \n");
			}

			break;

		case 79:
			do {
				printf("Set isp id (0, 1)>> \n");
				manual.id = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_get_awb(AWBT_ITEM_MANUAL, &manual);
			if (result == HD_OK) {
				printf("id = %d, en = %d, r_gain = %d, g_gain = %d, b_gain = %d \n", manual.id,
					manual.manual.en, manual.manual.r_gain, manual.manual.g_gain, manual.manual.b_gain);
			} else {
				printf("Get AWBT_ITEM_MANUAL fail \n");
			}

			do {
				printf("Set manual enable >> \n");
				manual.manual.en = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set manual Rgain >> \n");
				manual.manual.r_gain = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set manual Ggain >> \n");
				manual.manual.g_gain = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set manual Bgain >> \n");
				manual.manual.b_gain = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_MANUAL, &manual);
			if (result == HD_OK) {
				printf("id = %d, en = %d, r_gain = %d, g_gain = %d, b_gain = %d \n", manual.id,
					manual.manual.en, manual.manual.r_gain, manual.manual.g_gain, manual.manual.b_gain);
			} else {
				printf("Set AWBT_ITEM_MANUAL fail \n");
			}

			break;

		case 84:
			do {
				printf("Set isp id (0, 1)>> \n");
				kgain.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set Rratio (50~150) >> \n");
				kgain.k_gain.r_ratio = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set Bratio (50~150) >> \n");
				kgain.k_gain.b_ratio = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_KGAIN_RATIO, &kgain);
			if (result == HD_OK) {
				printf("id = %d, Rratio = %d, Bratio = %d \n", kgain.id, kgain.k_gain.r_ratio, kgain.k_gain.b_ratio);
			} else {
				printf("Get AWBT_ITEM_LUMA_WEIGHT fail \n");
			}
			break;

		case 86:
			do {
				printf("Set isp id (0, 1)>> \n");
				win.id = (UINT32)get_choose_int();
			} while (0);

			for (j = 0; j < 32; j++) {
				for (i = 0; i < 32; i++) {
					win.win.tab[j][i] = 0;
				}
			}
			for (j = 12; j < 20; j++) {
				for (i = 12; i < 20; i++) {
					win.win.tab[j][i] = 1;
				}
			}

			result = vendor_isp_set_awb(AWBT_ITEM_WIN, &win);
			if (result == HD_OK) {
				printf("id = %d \n", win.id);
				for (j = 0; j < 32; j++) {
					printf("win \t: ");
					for (i = 0; i < 32; i++) {
						printf("%4d\t", win.win.tab[j][i]);
					}
					printf("\r\n");
				}
			} else {
				printf("Get AWBT_ITEM_WIN fail \n");
			}
			break;

		case 87:
			do {
				printf("Set isp id (0, 1)>> \n");
				expand_ct.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set expand_ct enable >> \n");
				expand_ct.enable = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_EXPAND_CT_ENABLE, &expand_ct);
			if (result == HD_OK) {
				printf("id = %d , enable = %d\n", expand_ct.id, expand_ct.enable);
			} else {
				printf("Set AWBT_ITEM_EXPAND_CT_ENABLE fail \n");
			}
			break;

		case 88:
			do {
				printf("Set isp id (0, 1)>> \n");
				auto_weight.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set auto_weight enable >> \n");
				auto_weight.enable = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_AUTO_WEIGHT_ENABLE, &auto_weight);
			if (result == HD_OK) {
				printf("id = %d , enable = %d\n", auto_weight.id, auto_weight.enable);
			} else {
				printf("Set AWBT_ITEM_AUTO_WEIGHT_ENABLE fail \n");
			}
			break;

		case 89:
			do {
				printf("Set isp id (0, 1)>> \n");
				gray_world.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set gray_world enable >> \n");
				gray_world.enable = (UINT32)get_choose_int();
			} while (0);

			result = vendor_isp_set_awb(AWBT_ITEM_GRAY_WORLD_ENABLE, &gray_world);
			if (result == HD_OK) {
				printf("id = %d , enable = %d\n", gray_world.id, gray_world.enable);
			} else {
				printf("Set AWBT_ITEM_GRAY_WORLD_ENABLE fail \n");
			}
			break;

		case 0:
		default:
			trig = 0;
			break;
		}
	}

	ret = vendor_isp_uninit();
	if (ret != HD_OK) {
		printf("vendor_isp_uninit fail=%d\n", ret);
	}

	return 0;
}
