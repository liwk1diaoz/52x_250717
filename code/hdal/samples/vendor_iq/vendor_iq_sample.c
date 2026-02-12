#include <string.h>
#include <pthread.h>

#include <kwrap/file.h>

#include "vendor_isp.h"
#include "vendor_common.h"

#define USER_EXPAND_DPC_TEST 1
#define USER_EXPAND_BIN_0    "/mnt/app/isp/dpc_expand_table.bin"

#define USER_EXPAND_DPC_BUFFER_SIZE (160000 * 4)
#define USER_EXPAND_DPC_X 300
#define USER_EXPAND_DPC_Y 300
#define USER_EXPAND_DPC_W 400
#define USER_EXPAND_DPC_H 400

typedef struct _DDR_INFO {
	void *va;
	UINT32 pa;
	UINT32 size;
} DDR_INFO;

//============================================================================
// global
//============================================================================
pthread_t iq_tonelv_thread_id;
static UINT32 iq_tonelv_conti_run = 1;

BOOL tone_level_en = FALSE;
BOOL tone_level_dbg = FALSE;

static INT32 get_choose_int(void)
{
	CHAR buf[256];
	INT val, error;

	error = scanf("%d", &val);

	if (error != 1) {
		printf("Invalid option. Try again.\n");
		clearerr(stdin);
		fgets(buf, sizeof(buf), stdin);
		val = -1;
	}

	return val;
}

static INT32 iq_intpl(INT32 index, INT32 l_value, INT32 h_value, INT32 l_index, INT32 h_index)
{
	INT32 range = h_index - l_index;

	if (l_value == h_value) {
		return l_value;
	} else if (index <= l_index) {
		return l_value;
	} else if (index >= h_index) {
		return h_value;
	}
	if (h_value < l_value) {
		return l_value + ((h_value - l_value) * (index  - l_index) - (range >> 1)) / range;
	} else {
		return l_value + ((h_value - l_value) * (index  - l_index) + (range >> 1)) / range;
	}
}

static void *iq_tonelv_thread(void *arg)
{
	AET_STATUS_INFO ae_status = {0};
	IQT_DR_LEVEL dr_level = {0};
	IQT_SHDR_TONE_LV shdr_tone_lv = {0};
	UINT32 tonelv_max_th_l_iso = 400, tonelv_max_th_l_value = 100;
	UINT32 tonelv_max_th_h_iso = 1600, tonelv_max_th_h_value = 50;
	UINT32 curr_tonelv_max;
	UINT32 tonelv_th_l_dr = 20, tonelv_th_l_value = 0;
	UINT32 tonelv_th_m_dr = 50, tonelv_th_m_value = 50;
	UINT32 tonelv_th_h_dr = 100, tonelv_th_h_value = 100;
	UINT32 smooth_factor = 7;

	static BOOL first_run = TRUE;
	static UINT32 smooth_dr_level = 50;

	UINT32 *id = (UINT32 *)arg;

	ae_status.id = *id;
	dr_level.id = *id;
	shdr_tone_lv.id = *id;

	if (tone_level_en == TRUE) {
		while (iq_tonelv_conti_run) {
			// Get current dr level, and smooth it
			vendor_isp_get_iq(IQT_ITEM_DR_LEVEL, &dr_level);
			if (first_run) {
				smooth_dr_level = dr_level.dr_level;
				first_run = FALSE;
			}
			if (smooth_dr_level <= dr_level.dr_level) {
				smooth_dr_level = (smooth_dr_level * smooth_factor + dr_level.dr_level * 1 + smooth_factor) / (smooth_factor + 1); // Unconditional carry
			} else {
				smooth_dr_level = (smooth_dr_level * smooth_factor + dr_level.dr_level * 1) / (smooth_factor + 1); // Unconditional chop
			}

			// Get shdr tone level setting
			if (smooth_dr_level <= tonelv_th_m_dr) {
				shdr_tone_lv.lv = iq_intpl(smooth_dr_level, tonelv_th_l_value, tonelv_th_m_value, tonelv_th_l_dr, tonelv_th_m_dr);
			} else {
				shdr_tone_lv.lv = iq_intpl(smooth_dr_level, tonelv_th_m_value, tonelv_th_h_value, tonelv_th_m_dr, tonelv_th_h_dr);
			}

			// Get max boundary of tone level
			vendor_isp_get_ae(AET_ITEM_STATUS, &ae_status);
			curr_tonelv_max = iq_intpl(ae_status.status_info.iso_gain[1], tonelv_max_th_l_value, tonelv_max_th_h_value, tonelv_max_th_l_iso, tonelv_max_th_h_iso);
			if (shdr_tone_lv.lv > curr_tonelv_max) {
				shdr_tone_lv.lv = curr_tonelv_max;
			}

			vendor_isp_set_iq(IQT_ITEM_SHDR_TONE_LV, &shdr_tone_lv);
			if (tone_level_dbg == TRUE) {
				printf("dr_level smooth = %d (curr = %d) \n", smooth_dr_level, dr_level.dr_level);
				printf("shdr_tone level = %d (max = %d) \n", shdr_tone_lv.lv, curr_tonelv_max);
			}
			sleep(1);
		}
	} else {
		shdr_tone_lv.lv = 50;
		vendor_isp_set_iq(IQT_ITEM_SHDR_TONE_LV, &shdr_tone_lv);
		printf("shdr_tone id = %d \n", shdr_tone_lv.id);
		printf("shdr_tone level = %d \n", shdr_tone_lv.lv);
	}

	return 0;
}

static BOOL hd_mem_init = FALSE;
static IQT_DPC_PARAM dpc = {0};
static IQT_SHADING_PARAM shading = {0};
static IQT_SHADING_EXT_PARAM shading_ext = {0};
static IQT_LDC_PARAM ldc = {0};
int main(int argc, char *argv[])
{
	INT32 option;
	UINT32 trig = 1;
	UINT32 tmp = 0;
	UINT32 i = 0;
	UINT32 version = 0;
	IQT_CFG_INFO cfg_info = {0};
	IQT_DTSI_INFO dtsi_info = {0};
	IQT_NR_LV nr_lv = {0};
	IQT_3DNR_LV nr3d_lv = {0};
	IQT_SHARPNESS_LV sharpness_lv = {0};
	IQT_SATURATION_LV saturation_lv = {0};
	IQT_CONTRAST_LV contrast_lv = {0};
	IQT_BRIGHTNESS_LV brightness_lv = {0};
	IQT_NIGHT_MODE night_mode = {0};
	IQT_YCC_FORMAT ycc_format = {0};
	IQT_OPERATION operation = {0};
	IQT_IMAGEEFFECT imageeffect = {0};
	IQT_CCID ccid = {0};
	IQT_HUE_SHIFT hue_shift = {0};
	IQT_SHDR_TONE_LV shdr_tone_lv = {0};
	IQT_GAMMA_LV gamma_lv = {0};
	IQT_CCM_EXT2_PARAM ccm_ext2 = {0};
	IQT_GAMMA_EXT_PARAM gamma_ext = {0};
	IQT_CCM_EXT_PARAM ccm_ext = {0};
	IQT_SHADING_VIG_EXT_PARAM shading_vig_ext = {0};
	IQT_SHADING_INTER_PARAM shading_inter = {0};
	IQT_SHADING_DITH_PARAM shading_dith = {0};
	IQT_CST_PARAM cst = {0};
	IQT_STRIPE_PARAM stripe = {0};
	IQT_OB_PARAM ob = {0};
	IQT_NR_PARAM nr_2d = {0};
	IQT_CFA_PARAM cfa = {0};
	IQT_VA_PARAM va = {0};
	IQT_GAMMA_PARAM gamma = {0};
	IQT_CCM_PARAM ccm = {0};
	IQT_COLOR_PARAM color = {0};
	IQT_CONTRAST_PARAM contrast = {0};
	IQT_EDGE_PARAM edge = {0};
	IQT_EDGE_REGION_PARAM edge_region = {0};
	IQT_3DNR_PARAM nr_3d = {0};
	IQT_PFR_PARAM pfr = {0};
	IQT_WDR_PARAM wdr = {0};
	IQT_DEFOG_PARAM defog = {0};
	IQT_SHDR_PARAM shdr = {0};
	IQT_SHDR_EXT_PARAM shdr_ext = {0};
	IQT_RGBIR_PARAM rgbir = {0};
	IQT_COMPANDING_PARAM companding = {0};
	IQT_SHDR_MODE shdr_mode = {0};
	IQT_3DNR_MISC_PARAM nr_3d_misc = {0};
	IQT_POST_3DNR_PARAM post_3dnr = {0};
	IQT_DR_LEVEL dr_level = {0};
	IQT_RGBIR_ENH_PARAM rgbir_enh = {0};
	IQT_RGBIR_ENH_ISO rgbir_enh_iso = {0};
	IQT_LCA_SUB_RATIO_PARAM lca_sub_ratio = {0};
	IQT_EXPAND_DPC_PARAM expand_dpc = {0};
	IQT_DARK_ENH_RATIO dark_enh_ratio = {0};
	IQT_CONTRAST_ENH_RATIO contrast_enh_ratio = {0};
	IQT_GREEN_ENH_RATIO green_enh_ratio = {0};
	IQT_SKIN_ENH_RATIO skin_enh_ratio = {0};

	VENDOR_COMM_MAX_FREE_BLOCK max_free_block = {0};
	DDR_INFO dpc_buffer;
	HD_RESULT ret, file_ret;

	// open MCU device
	if (vendor_isp_init() == HD_ERR_NG) {
		return -1;
	}

	while (trig) {
		printf("----------------------------------------\r\n");
		printf("   1. Get version \n");
		printf("   2. Reload config file \n");
		printf("   3. Reload dtsi file \n");
		printf("   5. Get gamma_lv \n");
		printf("   6. Get ccm_ext2 \n");
		printf("   7. Get gamma_ext \n");
		printf("   8. Get expand_dpc \n");
		printf("   9. Get shading_dith \n");
		printf("  10. Get nr level \n");
		printf("  11. Get sharpness level \n");
		printf("  12. Get saturation level \n");
		printf("  13. Get contrast level \n");
		printf("  14. Get brightness level \n");
		printf("  15. Get nighe mode \n");
		printf("  16. Get ycc format \n");
		printf("  17. Get operation \n");
		printf("  18. Get imageeffect \n");
		printf("  19. Get ccid \n");
		printf("  20. Get hue_shift \n");
		printf("  21. Get shdr_tone_lv \n");
		printf("  22. Get nr3d level \n");
		printf("  23. Get ccm_ext \n");
		printf("  24. Get shading_vig_ext \n");
		printf("  25. Get shdr_ext \n");
		printf("  26. Get shading_inter \n");
		printf("  27. Get shading_ext \n");
		printf("  28. Get CST \n");
		printf("  29. Get STRIPE \n");
		printf("  30. Get lca sub ratio \n");
		printf("  31. Get ob \n");
		printf("  32. Get nr_2d \n");
		printf("  33. Get cfa \n");
		printf("  34. Get va \n");
		printf("  35. Get gamma \n");
		printf("  36. Get ccm \n");
		printf("  37. Get color \n");
		printf("  38. Get contrast \n");
		printf("  39. Get edge \n");
		printf("  40. Get nr_3d \n");
		printf("  41. Get dpc \n");
		printf("  42. Get shading \n");
		printf("  43. Get ldc \n");
		printf("  44. Get pfr \n");
		printf("  45. Get wdr \n");
		printf("  46. Get defog \n");
		printf("  47. Get shdr \n");
		printf("  48. Get rgbir \n");
		printf("  49. Get companding \n");
		printf("  52. Get shdr_mode \n");
		printf("  53. Get 3dnr misc param \n");
		printf("  54. Get post_3dnr \n");
		printf("  55. Get dr_level \n");
		printf("  56. Get rgbir_enh \n");
		printf("  57. Get rgbir_enh_iso \n");
		printf("  59. Get edge_region \n");
		printf(" 109. Set shading_dith \n");
		printf(" 108. Set expand_dpc \n");
		printf(" 110. Set nr level \n");
		printf(" 111. Set sharpness level \n");
		printf(" 112. Set saturation level \n");
		printf(" 113. Set contrast level \n");
		printf(" 114. Set brightness level \n");
		printf(" 115. Set nighe mode \n");
		printf(" 116. Set ycc format \n");
		printf(" 117. Set operation \n");
		printf(" 118. Set imageeffect \n");
		printf(" 119. Set ccid \n");
		printf(" 120. Set hue_shift \n");
		printf(" 121. Set shdr_tone_lv \n");
		printf(" 122. Set nr3d level \n");
		printf(" 123. Set ccm_ext \n");
		printf(" 124. Set shading_vig_ext \n");
		printf(" 125. Set shdr_ext \n");
		printf(" 128. Set CST \n");
		printf(" 129. Set STRIPE \n");
		printf(" 130. Set lca sub ratio \n");
		printf(" 153. Set 3dnr misc param \n");
		printf(" 159. Set edge_region \n");
		printf(" 200. Get NNSC drak_enh \n");
		printf(" 201. Get NNSC contrast_enh \n");
		printf(" 202. Get NNSC green_enh \n");
		printf(" 203. Get NNSC skin_enh \n");
		printf(" 300. Set NNSC drak_enh \n");
		printf(" 301. Set NNSC contrast_enh \n");
		printf(" 302. Set NNSC green_enh \n");
		printf(" 303. Set NNSC skin_enh \n");
		printf(" 500. Set auto tone level enable \n");
		printf("   0. Quit\n");
		printf("----------------------------------------\n");

		do {
			printf(">> ");
			option = get_choose_int();
		} while (option < 0);

		switch (option) {
		case 1:
			vendor_isp_get_iq(IQT_ITEM_VERSION, &version);
			printf("version = 0x%X \n", version);
			break;

		case 2:
			do {
				printf("Set isp id (0, 1)>> \n");
				cfg_info.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Select chg file>> \n");
				printf("  1: isp_os02k10_0.cfg \n");
				printf("  2: isp_os02k10_0_hdr.cfg \n");
				printf("  3: isp_os02k10_0_cap.cfg \n");
				printf("  4: isp_os05a10_0.cfg \n");
				printf("  5: isp_os05a10_0_hdr.cfg \n");
				printf("  6: isp_ov2718_0.cfg \n");
				printf("  7: isp_imx290_0.cfg \n");
				printf("  8: isp_imx290_0_hdr.cfg \n");
				tmp = (UINT32)get_choose_int();
			} while (0);

			switch (tmp) {
			case 1:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0.cfg", CFG_NAME_LENGTH);
				break;

			case 2:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0_hdr.cfg", CFG_NAME_LENGTH);
				break;

			case 3:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os02k10_0_cap.cfg", CFG_NAME_LENGTH);
				break;

			case 4:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0.cfg", CFG_NAME_LENGTH);
				break;

			case 5:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_os05a10_0_hdr.cfg", CFG_NAME_LENGTH);
				break;

			case 6:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_ov2718_0.cfg", CFG_NAME_LENGTH);
				break;

			case 7:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
				break;

			case 8:
				strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0_hdr.cfg", CFG_NAME_LENGTH);
				break;

			default:
				printf("Not support item (%d) \n", tmp);
				break;
			}
			vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
			break;

		case 3:
			do {
				printf("Set isp id (0, 1)>> \n");
				dtsi_info.id = (UINT32)get_choose_int();
			} while (0);
			do {
				printf("Select dtsi file>> \n");
				printf("1: imx290_iq_0.dtsi \n");
				printf("2: imx290_iq_0_cap.dtsi \n");
				printf("3: imx290_iq_dpc_0.dtsi \n");
				printf("4: imx290_iq_ldc_0.dtsi \n");
				printf("5: imx290_iq_shading_0.dtsi \n");
				tmp = (UINT32)get_choose_int();
			} while (0);

			switch (tmp) {
			case 1:
				strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_0", DTSI_NAME_LENGTH);
				break;

			case 2:
				strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_0_cap", DTSI_NAME_LENGTH);
				break;

			case 3:
				strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_dpc_0", DTSI_NAME_LENGTH);
				break;

			case 4:
				strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_ldc_0", DTSI_NAME_LENGTH);
				break;

			case 5:
				strncpy(dtsi_info.node_path, "/isp/iq/imx290_iq_shading_0", DTSI_NAME_LENGTH);
				break;

			default:
				printf("Not support item (%d) \n", tmp);
				break;
			}

			strncpy(dtsi_info.file_path, "/mnt/app/isp/isp.dtb", DTSI_NAME_LENGTH);
			dtsi_info.buf_addr = NULL;
			vendor_isp_set_iq(IQT_ITEM_RLD_DTSI, &dtsi_info);
			break;

		case 5:
			do {
				printf("Set isp id (0, 1)>> \n");
				gamma_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_GAMMA_LV, &gamma_lv);
			printf("gamma id = %d \n", gamma_lv.id);
			printf("gamma level = %d \n", gamma_lv.lv);
			break;

		case 6:
			do {
				printf("Set isp id (0, 1)>> \n");
				ccm_ext2.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CCM_EXT2_PARAM, &ccm_ext2);
			printf("ccm_ext2 id = %d \n", ccm_ext2.id);
			printf("ccm_ext2 enable = %d \n", ccm_ext2.ccm_ext2.enable);
			printf("ccm_ext2 auto_param[0].ccm = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext2.ccm_ext2.auto_param[0].coef[0], ccm_ext2.ccm_ext2.auto_param[0].coef[1], ccm_ext2.ccm_ext2.auto_param[0].coef[2]
				, ccm_ext2.ccm_ext2.auto_param[0].coef[3], ccm_ext2.ccm_ext2.auto_param[0].coef[4], ccm_ext2.ccm_ext2.auto_param[0].coef[5]
				, ccm_ext2.ccm_ext2.auto_param[0].coef[6], ccm_ext2.ccm_ext2.auto_param[0].coef[7], ccm_ext2.ccm_ext2.auto_param[0].coef[8]);
			printf("ccm_ext2 auto_param[1].ccm = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext2.ccm_ext2.auto_param[1].coef[0], ccm_ext2.ccm_ext2.auto_param[1].coef[1], ccm_ext2.ccm_ext2.auto_param[1].coef[2]
				, ccm_ext2.ccm_ext2.auto_param[1].coef[3], ccm_ext2.ccm_ext2.auto_param[1].coef[4], ccm_ext2.ccm_ext2.auto_param[1].coef[5]
				, ccm_ext2.ccm_ext2.auto_param[1].coef[6], ccm_ext2.ccm_ext2.auto_param[1].coef[7], ccm_ext2.ccm_ext2.auto_param[1].coef[8]);
			printf("ccm_ext2 auto_param[2].ccm = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext2.ccm_ext2.auto_param[2].coef[0], ccm_ext2.ccm_ext2.auto_param[2].coef[1], ccm_ext2.ccm_ext2.auto_param[2].coef[2]
				, ccm_ext2.ccm_ext2.auto_param[2].coef[3], ccm_ext2.ccm_ext2.auto_param[2].coef[4], ccm_ext2.ccm_ext2.auto_param[2].coef[5]
				, ccm_ext2.ccm_ext2.auto_param[2].coef[6], ccm_ext2.ccm_ext2.auto_param[2].coef[7], ccm_ext2.ccm_ext2.auto_param[2].coef[8]);
			printf("ccm_ext2 auto_param[3].ccm = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext2.ccm_ext2.auto_param[3].coef[0], ccm_ext2.ccm_ext2.auto_param[3].coef[1], ccm_ext2.ccm_ext2.auto_param[3].coef[2]
				, ccm_ext2.ccm_ext2.auto_param[3].coef[3], ccm_ext2.ccm_ext2.auto_param[3].coef[4], ccm_ext2.ccm_ext2.auto_param[3].coef[5]
				, ccm_ext2.ccm_ext2.auto_param[3].coef[6], ccm_ext2.ccm_ext2.auto_param[3].coef[7], ccm_ext2.ccm_ext2.auto_param[3].coef[8]);
			printf("ccm_ext2 auto_param[4].ccm = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext2.ccm_ext2.auto_param[4].coef[0], ccm_ext2.ccm_ext2.auto_param[4].coef[1], ccm_ext2.ccm_ext2.auto_param[4].coef[2]
				, ccm_ext2.ccm_ext2.auto_param[4].coef[3], ccm_ext2.ccm_ext2.auto_param[4].coef[4], ccm_ext2.ccm_ext2.auto_param[4].coef[5]
				, ccm_ext2.ccm_ext2.auto_param[4].coef[6], ccm_ext2.ccm_ext2.auto_param[4].coef[7], ccm_ext2.ccm_ext2.auto_param[4].coef[8]);
			break;

		case 7:
			do {
				printf("Set isp id (0, 1)>> \n");
				gamma_ext.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_GAMMA_EXT_PARAM, &gamma_ext);
			printf("gamma_ext id = %d \n", gamma_ext.id);
			printf("gamma_ext ext_sel = %d \n", gamma_ext.gamma_ext.ext_sel);
			printf("gamma_ext gamma_set0_lv = %d \n", gamma_ext.gamma_ext.gamma_set0_lv);
			printf("gamma_ext gamma_set0_lut = {%d, %d, %d, %d, %d, %d, %d, %d, %d, ...} \n"
				, gamma_ext.gamma_ext.gamma_set0_lut[0], gamma_ext.gamma_ext.gamma_set0_lut[1], gamma_ext.gamma_ext.gamma_set0_lut[2]
				, gamma_ext.gamma_ext.gamma_set0_lut[3], gamma_ext.gamma_ext.gamma_set0_lut[4], gamma_ext.gamma_ext.gamma_set0_lut[5]
				, gamma_ext.gamma_ext.gamma_set0_lut[6], gamma_ext.gamma_ext.gamma_set0_lut[7], gamma_ext.gamma_ext.gamma_set0_lut[8]);
			printf("gamma_ext gamma_set1_lv = %d \n", gamma_ext.gamma_ext.gamma_set1_lv);
			printf("gamma_ext gamma_set1_lut = {%d, %d, %d, %d, %d, %d, %d, %d, %d, ...} \n"
				, gamma_ext.gamma_ext.gamma_set1_lut[0], gamma_ext.gamma_ext.gamma_set1_lut[1], gamma_ext.gamma_ext.gamma_set1_lut[2]
				, gamma_ext.gamma_ext.gamma_set1_lut[3], gamma_ext.gamma_ext.gamma_set1_lut[4], gamma_ext.gamma_ext.gamma_set1_lut[5]
				, gamma_ext.gamma_ext.gamma_set1_lut[6], gamma_ext.gamma_ext.gamma_set1_lut[7], gamma_ext.gamma_ext.gamma_set1_lut[8]);
			printf("gamma_ext gamma_set2_lv = %d \n", gamma_ext.gamma_ext.gamma_set2_lv);
			printf("gamma_ext gamma_set2_lut = {%d, %d, %d, %d, %d, %d, %d, %d, %d, ...} \n"
				, gamma_ext.gamma_ext.gamma_set2_lut[0], gamma_ext.gamma_ext.gamma_set2_lut[1], gamma_ext.gamma_ext.gamma_set2_lut[2]
				, gamma_ext.gamma_ext.gamma_set2_lut[3], gamma_ext.gamma_ext.gamma_set2_lut[4], gamma_ext.gamma_ext.gamma_set2_lut[5]
				, gamma_ext.gamma_ext.gamma_set2_lut[6], gamma_ext.gamma_ext.gamma_set2_lut[7], gamma_ext.gamma_ext.gamma_set2_lut[8]);
			printf("gamma_ext gamma_set3_lv = %d \n", gamma_ext.gamma_ext.gamma_set3_lv);
			printf("gamma_ext gamma_set3_lut = {%d, %d, %d, %d, %d, %d, %d, %d, %d, ...} \n"
				, gamma_ext.gamma_ext.gamma_set3_lut[0], gamma_ext.gamma_ext.gamma_set3_lut[1], gamma_ext.gamma_ext.gamma_set3_lut[2]
				, gamma_ext.gamma_ext.gamma_set3_lut[3], gamma_ext.gamma_ext.gamma_set3_lut[4], gamma_ext.gamma_ext.gamma_set3_lut[5]
				, gamma_ext.gamma_ext.gamma_set3_lut[6], gamma_ext.gamma_ext.gamma_set3_lut[7], gamma_ext.gamma_ext.gamma_set3_lut[8]);
			printf("gamma_ext gamma_set4_lv = %d \n", gamma_ext.gamma_ext.gamma_set4_lv);
			printf("gamma_ext gamma_set4_lut = {%d, %d, %d, %d, %d, %d, %d, %d, %d, ...} \n"
				, gamma_ext.gamma_ext.gamma_set4_lut[0], gamma_ext.gamma_ext.gamma_set4_lut[1], gamma_ext.gamma_ext.gamma_set4_lut[2]
				, gamma_ext.gamma_ext.gamma_set4_lut[3], gamma_ext.gamma_ext.gamma_set4_lut[4], gamma_ext.gamma_ext.gamma_set4_lut[5]
				, gamma_ext.gamma_ext.gamma_set4_lut[6], gamma_ext.gamma_ext.gamma_set4_lut[7], gamma_ext.gamma_ext.gamma_set4_lut[8]);
			break;

		case 8:
			printf("Set isp id (0, 5)>> \n");
			expand_dpc.id = (UINT32)get_choose_int();

			vendor_isp_get_iq(IQT_ITEM_EXPAND_DPC_PARAM, &expand_dpc);
			printf("expand_dpc id = %d \n", expand_dpc.id);
			printf("expand_dpc enable = %d \n", expand_dpc.expand_dpc.enable);
			printf("expand_dpc size = %d \n", expand_dpc.expand_dpc.size);
			printf("expand_dpc table_phyaddr =0x%lx \n", expand_dpc.expand_dpc.table_phyaddr);
			break;

		case 9:
			printf("Set isp id (0, 5)>> \n");
			shading_dith.id = (UINT32)get_choose_int();

			vendor_isp_get_iq(IQT_ITEM_SHADING_DITH_PARAM, &shading_dith);
			printf("shading_dith id = %d \n", shading_dith.id);
			printf("shading_dith dithering_enable = %d \n", shading_dith.dithering_enable);
			break;

		case 10:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_NR_LV, &nr_lv);
			printf("nr id = %d \n", nr_lv.id);
			printf("nr level = %d \n", nr_lv.lv);
			break;

		case 11:
			do {
				printf("Set isp id (0, 1)>> \n");
				sharpness_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHARPNESS_LV, &sharpness_lv);
			printf("sharpness id = %d \n", sharpness_lv.id);
			printf("sharpness level = %d \n", sharpness_lv.lv);
			break;

		case 12:
			do {
				printf("Set isp id (0, 1)>> \n");
				saturation_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SATURATION_LV, &saturation_lv);
			printf("saturation id = %d \n", saturation_lv.id);
			printf("saturation level = %d \n", saturation_lv.lv);
			break;

		case 13:
			do {
				printf("Set isp id (0, 1)>> \n");
				contrast_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CONTRAST_LV, &contrast_lv);
			printf("contrast id = %d \n", contrast_lv.id);
			printf("contrast level = %d \n", contrast_lv.lv);
			break;

		case 14:
			do {
				printf("Set isp id (0, 1)>> \n");
				brightness_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_BRIGHTNESS_LV, &brightness_lv);
			printf("brightness id = %d \n", brightness_lv.id);
			printf("brightness level = %d \n", brightness_lv.lv);
			break;

		case 15:
			do {
				printf("Set isp id (0, 1)>> \n");
				night_mode.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_NIGHT_MODE, &night_mode);
			printf("night id = %d \n", night_mode.id);
			printf("night mode = %d \n", night_mode.mode);
			break;

		case 16:
			do {
				printf("Set isp id (0, 1)>> \n");
				ycc_format.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_YCC_FORMAT, &ycc_format);
			printf("ycc_format id = %d \n", ycc_format.id);
			printf("ycc_format format = %d \n", ycc_format.format);
			break;

		case 17:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_OPERATION, &operation);
			printf("operation id = %d \n", operation.id);
			printf("operation sel = %d \n", operation.operation);
			break;

		case 18:
			do {
				printf("Set isp id (0, 1)>> \n");
				imageeffect.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_IMAGEEFFECT, &imageeffect);
			printf("imageeffect id = %d \n", imageeffect.id);
			printf("imageeffect effect = %d \n", imageeffect.effect);
			break;

		case 19:
			do {
				printf("Set isp id (0, 1)>> \n");
				ccid.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CCID, &ccid);
			printf("color id = %d \n", ccid.id);
			printf("color ccid = %d \n", ccid.ccid);
			break;

		case 20:
			do {
				printf("Set isp id (0, 1)>> \n");
				hue_shift.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_HUE_SHIFT, &hue_shift);
			printf("hue id = %d \n", hue_shift.id);
			printf("hue shift = %d \n", hue_shift.hue_shift);
			break;

		case 21:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr_tone_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHDR_TONE_LV, &shdr_tone_lv);
			printf("shdr_tone id = %d \n", shdr_tone_lv.id);
			printf("shdr_tone level = %d \n", shdr_tone_lv.lv);
			break;

		case 22:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr3d_lv.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_3DNR_LV, &nr3d_lv);
			printf("3dnr id = %d \n", nr3d_lv.id);
			printf("3dnr level = %d \n", nr3d_lv.lv);
			break;

		case 23:
			do {
				printf("Set isp id (0, 1)>> \n");
				ccm_ext.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CCM_EXT_PARAM, &ccm_ext);
			printf("ccm_ext id = %d \n", ccm_ext.id);
			printf("ccm_ext manual_param.int_tab = {%d, %d, %d, %d, %d, %d, ..., %d, %d, %d} \n"
				, ccm_ext.ccm_ext.manual_param.int_tab[0], ccm_ext.ccm_ext.manual_param.int_tab[1], ccm_ext.ccm_ext.manual_param.int_tab[2]
				, ccm_ext.ccm_ext.manual_param.int_tab[3], ccm_ext.ccm_ext.manual_param.int_tab[4], ccm_ext.ccm_ext.manual_param.int_tab[5]
				, ccm_ext.ccm_ext.manual_param.int_tab[21], ccm_ext.ccm_ext.manual_param.int_tab[22], ccm_ext.ccm_ext.manual_param.int_tab[23]);
			printf("ccm_ext auto_param[0].int_tab = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext.ccm_ext.auto_param[0].int_tab[0], ccm_ext.ccm_ext.auto_param[0].int_tab[1], ccm_ext.ccm_ext.auto_param[0].int_tab[2]
				, ccm_ext.ccm_ext.auto_param[0].int_tab[3], ccm_ext.ccm_ext.auto_param[0].int_tab[4], ccm_ext.ccm_ext.auto_param[0].int_tab[5]
				, ccm_ext.ccm_ext.auto_param[0].int_tab[21], ccm_ext.ccm_ext.auto_param[0].int_tab[22], ccm_ext.ccm_ext.auto_param[0].int_tab[23]);
			break;

		case 24:
			do {
				printf("Set isp id (0, 1)>> \n");
				shading_vig_ext.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHADING_VIG_EXT_PARAM, &shading_vig_ext);
			printf("shading_vig_ext id = %d \n", shading_vig_ext.id);
			printf("shading_vig_ext vig_fisheye_enable = %d \n", shading_vig_ext.shading_vig_ext.vig_fisheye_enable);
			printf("shading_vig_ext vig_fisheye_radius = %d \n", shading_vig_ext.shading_vig_ext.vig_fisheye_radius);
			printf("shading_vig_ext vig_fisheye_slope = %d \n", shading_vig_ext.shading_vig_ext.vig_fisheye_slope);
			break;

		case 25:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr_ext.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHDR_EXT_PARAM, &shdr_ext);
			printf("shdr_ext id = %d \n", shdr_ext.id);
			printf("shdr_ext fcurve_y_w_lut = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d \n"
				, shdr_ext.shdr_ext.fcurve_y_w_lut[0], shdr_ext.shdr_ext.fcurve_y_w_lut[1], shdr_ext.shdr_ext.fcurve_y_w_lut[2]
				, shdr_ext.shdr_ext.fcurve_y_w_lut[3], shdr_ext.shdr_ext.fcurve_y_w_lut[4], shdr_ext.shdr_ext.fcurve_y_w_lut[5]
				, shdr_ext.shdr_ext.fcurve_y_w_lut[6], shdr_ext.shdr_ext.fcurve_y_w_lut[7], shdr_ext.shdr_ext.fcurve_y_w_lut[8]
				, shdr_ext.shdr_ext.fcurve_y_w_lut[9], shdr_ext.shdr_ext.fcurve_y_w_lut[10], shdr_ext.shdr_ext.fcurve_y_w_lut[11]
				, shdr_ext.shdr_ext.fcurve_y_w_lut[12], shdr_ext.shdr_ext.fcurve_y_w_lut[13], shdr_ext.shdr_ext.fcurve_y_w_lut[14]
				, shdr_ext.shdr_ext.fcurve_y_w_lut[15], shdr_ext.shdr_ext.fcurve_y_w_lut[16]);
			break;

		case 26:
			do {
				printf("Set isp id (0, 1)>> \n");
				shading_inter.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHADING_INTER_PARAM, &shading_inter);
			printf("shading_inter id = %d \n", shading_inter.id);
			printf("shading_inter mode = %d \n", shading_inter.shading_inter.mode);
			printf("shading_inter ecs_smooth_l_m_ct_lower = %d \n", shading_inter.shading_inter.ecs_smooth_l_m_ct_lower);
			printf("shading_inter ecs_smooth_l_m_ct_upper = %d \n", shading_inter.shading_inter.ecs_smooth_l_m_ct_upper);
			printf("shading_inter ecs_smooth_m_h_ct_lower = %d \n", shading_inter.shading_inter.ecs_smooth_m_h_ct_lower);
			printf("shading_inter ecs_smooth_m_h_ct_upper = %d \n", shading_inter.shading_inter.ecs_smooth_m_h_ct_upper);
			break;

		case 27:
			do {
				printf("Set isp id (0, 1)>> \n");
				shading_ext.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set ecs id (0, 1, 2)>> \n");
				shading_ext.shading_ext_if.ecs_map_idx = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHADING_EXT_PARAM, &shading_ext);
			printf("shading_ext id = %d \n", shading_ext.id);
			printf("shading_ext ecs_map_idx = %d \n", shading_ext.shading_ext_if.ecs_map_idx);
			printf("shading_ext ecs_map_tbl[0] = 0x%x \n", shading_ext.shading_ext_if.ecs_map_tbl[0]);
			break;

		case 28:
			do {
				printf("Set isp id (0, 1)>> \n");
				cst.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CST_PARAM, &cst);
			printf("cst id = %d \n", cst.id);
			printf("cst cst_coef = {%d, %d, %d, %d, %d, %d, %d, %d, %d}, \n"
				, cst.cst_coef[0], cst.cst_coef[1], cst.cst_coef[2], cst.cst_coef[3], cst.cst_coef[4]
				, cst.cst_coef[5], cst.cst_coef[6], cst.cst_coef[7], cst.cst_coef[8]);
			printf("cst cstp_ratio = %d \n", cst.cstp_ratio);
			break;

		case 29:
			do {
				printf("Set isp id (0, 1)>> \n");
				stripe.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_STRIPE_PARAM, &stripe);
			printf("stripe id = %d \n", stripe.id);
			printf("stripe stripe_type = %d, \n", stripe.stripe_type);
			break;

		case 30:
			do {
				printf("Set isp id (0, 1)>> \n");
				lca_sub_ratio.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_LCA_SUB_RATIO_PARAM, &lca_sub_ratio);
			printf("lca_sub_ratio id = %d \n", lca_sub_ratio.id);
			printf("lca_sub_ratio ratio = %d \n", lca_sub_ratio.lca_sub_ratio.ratio);
			break;

		case 31:
			do {
				printf("Set isp id (0, 1)>> \n");
				ob.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_OB_PARAM, &ob);
			printf("ob id = %d \n", ob.id);
			printf("ob mode = %d \n", ob.ob.mode);
			break;

		case 32:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_2d.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_NR_PARAM, &nr_2d);
			printf("nr_2d id = %d \n", nr_2d.id);
			printf("nr_2d mode = %d \n", nr_2d.nr.mode);
			break;

		case 33:
			do {
				printf("Set isp id (0, 1)>> \n");
				cfa.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CFA_PARAM, &cfa);
			printf("cfa id = %d \n", cfa.id);
			printf("cfa mode = %d \n", cfa.cfa.mode);
			break;

		case 34:
			do {
				printf("Set isp id (0, 1)>> \n");
				va.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_VA_PARAM, &va);
			printf("va id = %d \n", va.id);
			printf("va mode = %d \n", va.va.mode);
			break;

		case 35:
			do {
				printf("Set isp id (0, 1)>> \n");
				gamma.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_GAMMA_PARAM, &gamma);
			printf("gamma id = %d \n", gamma.id);
			printf("gamma mode = %d \n", gamma.gamma.mode);
			break;

		case 36:
			do {
				printf("Set isp id (0, 1)>> \n");
				ccm.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CCM_PARAM, &ccm);
			printf("ccm id = %d \n", ccm.id);
			printf("ccm mode = %d \n", ccm.ccm.mode);
			break;

		case 37:
			do {
				printf("Set isp id (0, 1)>> \n");
				color.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_COLOR_PARAM, &color);
			printf("color id = %d \n", color.id);
			printf("color mode = %d \n", color.color.mode);
			break;

		case 38:
			do {
				printf("Set isp id (0, 1)>> \n");
				contrast.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_CONTRAST_PARAM, &contrast);
			printf("contrast id = %d \n", contrast.id);
			printf("contrast mode = %d \n", contrast.contrast.mode);
			break;

		case 39:
			do {
				printf("Set isp id (0, 1)>> \n");
				edge.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_EDGE_PARAM, &edge);
			printf("edge id = %d \n", edge.id);
			printf("edge mode = %d \n", edge.edge.mode);
			break;

		case 40:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_3d.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_3DNR_PARAM, &nr_3d);
			printf("nr_3d id = %d \n", nr_3d.id);
			printf("nr_3d mode = %d \n", nr_3d._3dnr.mode);
			break;

		case 41:
			do {
				printf("Set isp id (0, 1)>> \n");
				dpc.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_DPC_PARAM, &dpc);
			printf("dpc id = %d \n", dpc.id);
			printf("dpc enable = %d \n", dpc.dpc.enable);
			break;

		case 42:
			do {
				printf("Set isp id (0, 1)>> \n");
				shading.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHADING_PARAM, &shading);
			printf("shading id = %d \n", shading.id);
			printf("shading ecs_enable = %d \n", shading.shading.ecs_enable);
			printf("shading vig_enable = %d \n", shading.shading.vig_enable);
			break;

		case 43:
			do {
				printf("Set isp id (0, 1)>> \n");
				ldc.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_LDC_PARAM, &ldc);
			printf("ldc id = %d \n", ldc.id);
			printf("ldc geo_enable = %d \n", ldc.ldc.geo_enable);
			printf("ldc lut_2d_enable = %d \n", ldc.ldc.lut_2d_enable);
			break;

		case 44:
			do {
				printf("Set isp id (0, 1)>> \n");
				pfr.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_PFR_PARAM, &pfr);
			printf("pfr id = %d \n", pfr.id);
			printf("pfr mode = %d \n", pfr.pfr.mode);
			break;

		case 45:
			do {
				printf("Set isp id (0, 1)>> \n");
				wdr.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_WDR_PARAM, &wdr);
			printf("wdr id = %d \n", wdr.id);
			printf("wdr mode = %d \n", wdr.wdr.mode);
			break;

		case 46:
			do {
				printf("Set isp id (0, 1)>> \n");
				defog.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_DEFOG_PARAM, &defog);
			printf("defog id = %d \n", defog.id);
			printf("defog mode = %d \n", defog.defog.mode);
			break;

		case 47:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHDR_PARAM, &shdr);
			printf("shdr id = %d \n", shdr.id);
			printf("shdr mode = %d \n", shdr.shdr.mode);
			break;

		case 48:
			do {
				printf("Set isp id (0, 1)>> \n");
				rgbir.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_RGBIR_PARAM, &rgbir);
			printf("rgbir id = %d \n", rgbir.id);
			printf("rgbir mode = %d \n", rgbir.rgbir.mode);
			break;

		case 49:
			do {
				printf("Set isp id (0, 1)>> \n");
				companding.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_COMPANDING_PARAM, &companding);
			printf("companding id = %d \n", companding.id);
			printf("companding decomp_knee_pts = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d \n"
				, companding.companding.decomp_knee_pts[0], companding.companding.decomp_knee_pts[1], companding.companding.decomp_knee_pts[2]
				, companding.companding.decomp_knee_pts[3], companding.companding.decomp_knee_pts[4], companding.companding.decomp_knee_pts[5]
				, companding.companding.decomp_knee_pts[6], companding.companding.decomp_knee_pts[7], companding.companding.decomp_knee_pts[8]
				, companding.companding.decomp_knee_pts[9], companding.companding.decomp_knee_pts[10]);
			break;

		case 52:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr_mode.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_SHDR_MODE, &shdr_mode);
			printf("shdr_mode id = %d \n", shdr_mode.id);
			printf("shdr_mode mode = %d \n", shdr_mode.shdr_mode);
			break;

		case 53:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_3d_misc.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_3DNR_MISC_PARAM, &nr_3d_misc);
			printf("nr_3d_misc id = %d \n", nr_3d_misc.id);
			printf("nr_3d_misc md_roi = %d, %d \n", nr_3d_misc._3dnr_misc.md_roi[0], nr_3d_misc._3dnr_misc.md_roi[1]);
			printf("nr_3d_misc mc_roi = %d, %d \n", nr_3d_misc._3dnr_misc.mc_roi[0], nr_3d_misc._3dnr_misc.mc_roi[1]);
			printf("nr_3d_misc roi_mv_th = %d \n", nr_3d_misc._3dnr_misc.roi_mv_th);
			printf("nr_3d_misc ds_th_roi = %d \n", nr_3d_misc._3dnr_misc.ds_th_roi);
			break;

		case 54:
			do {
				printf("Set isp id (0, 1)>> \n");
				post_3dnr.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_POST_3DNR_PARAM, &post_3dnr);
			printf("post_3dnr id = %d \n", post_3dnr.id);
			printf("post_3dnr enable = %d \n", post_3dnr.post_3dnr.enable);
			printf("post_3dnr mode = %d \n", post_3dnr.post_3dnr.mode);
			break;

		case 55:
			do {
				printf("Set isp id (0, 1)>> \n");
				dr_level.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_DR_LEVEL, &dr_level);
			printf("dr_level id = %d \n", dr_level.id);
			printf("dr_level dr_level = %d \n", dr_level.dr_level);
			break;

		case 56:
			do {
				printf("Set isp id (0, 1)>> \n");
				rgbir_enh.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_RGBIR_ENH_PARAM, &rgbir_enh);
			printf("rgbir_enh id = %d \n", rgbir_enh.id);
			printf("rgbir_enh enable = %d \n", rgbir_enh.rgbir_enh.enable);
			break;

		case 57:
			do {
				printf("Set isp id (0, 1)>> \n");
				rgbir_enh_iso.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_RGBIR_ENH_ISO, &rgbir_enh_iso);
			printf("rgbir_enh_iso id = %d \n", rgbir_enh_iso.id);
			printf("rgbir_enh_iso rgbir_enh_iso = %d \n", rgbir_enh_iso.rgbir_enh_iso);
			break;

		case 59:
			do {
				printf("Set isp id (0, 1)>> \n");
				edge_region.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_EDGE_REGION_PARAM, &edge_region);
			printf("edge_region id = %d \n", edge_region.id);
			printf("edge_region manual str_flat = %d \n", edge_region.edge_region.manual_param.str_flat);
			printf("edge_region manual str_edge = %d \n", edge_region.edge_region.manual_param.str_edge);
			printf("edge_region manual th_flat_low = %d \n", edge_region.edge_region.manual_param.th_flat_low);
			printf("edge_region manual th_edge_high = %d \n", edge_region.edge_region.manual_param.th_edge_high);
			break;

		case 105:
			do {
				printf("Set isp id (0, 1)>> \n");
				gamma_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set gamma_lv (0 ~ %d)>> \n", IQ_UI_GAMMA_LV_MAX_CNT - 1);
				gamma_lv.lv = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_GAMMA_LV, &gamma_lv);
			printf("gamma_lv id = %d \n", gamma_lv.id);
			printf("gamma_lv level = %d \n", gamma_lv.lv);
			break;

		case 106:
			do {
				printf("Set isp id (0, 1)>> \n");
				ccm_ext2.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set enable (0 ~ 1)>> \n");
				ccm_ext2.ccm_ext2.enable = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_CCM_EXT2_PARAM, &ccm_ext2);
			printf("ccm_ext2 id = %d \n", ccm_ext2.id);
			printf("ccm_ext2 enable = %d \n", ccm_ext2.ccm_ext2.enable);
			printf("ccm_ext2 auto_param[0].ccm = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext2.ccm_ext2.auto_param[0].coef[0], ccm_ext2.ccm_ext2.auto_param[0].coef[1], ccm_ext2.ccm_ext2.auto_param[0].coef[2]
				, ccm_ext2.ccm_ext2.auto_param[0].coef[3], ccm_ext2.ccm_ext2.auto_param[0].coef[4], ccm_ext2.ccm_ext2.auto_param[0].coef[5]
				, ccm_ext2.ccm_ext2.auto_param[0].coef[6], ccm_ext2.ccm_ext2.auto_param[0].coef[7], ccm_ext2.ccm_ext2.auto_param[0].coef[8]);
			printf("ccm_ext2 auto_param[1].ccm = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext2.ccm_ext2.auto_param[1].coef[0], ccm_ext2.ccm_ext2.auto_param[1].coef[1], ccm_ext2.ccm_ext2.auto_param[1].coef[2]
				, ccm_ext2.ccm_ext2.auto_param[1].coef[3], ccm_ext2.ccm_ext2.auto_param[1].coef[4], ccm_ext2.ccm_ext2.auto_param[1].coef[5]
				, ccm_ext2.ccm_ext2.auto_param[1].coef[6], ccm_ext2.ccm_ext2.auto_param[1].coef[7], ccm_ext2.ccm_ext2.auto_param[1].coef[8]);
			printf("ccm_ext2 auto_param[2].ccm = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext2.ccm_ext2.auto_param[2].coef[0], ccm_ext2.ccm_ext2.auto_param[2].coef[1], ccm_ext2.ccm_ext2.auto_param[2].coef[2]
				, ccm_ext2.ccm_ext2.auto_param[2].coef[3], ccm_ext2.ccm_ext2.auto_param[2].coef[4], ccm_ext2.ccm_ext2.auto_param[2].coef[5]
				, ccm_ext2.ccm_ext2.auto_param[2].coef[6], ccm_ext2.ccm_ext2.auto_param[2].coef[7], ccm_ext2.ccm_ext2.auto_param[2].coef[8]);
			printf("ccm_ext2 auto_param[3].ccm = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext2.ccm_ext2.auto_param[3].coef[0], ccm_ext2.ccm_ext2.auto_param[3].coef[1], ccm_ext2.ccm_ext2.auto_param[3].coef[2]
				, ccm_ext2.ccm_ext2.auto_param[3].coef[3], ccm_ext2.ccm_ext2.auto_param[3].coef[4], ccm_ext2.ccm_ext2.auto_param[3].coef[5]
				, ccm_ext2.ccm_ext2.auto_param[3].coef[6], ccm_ext2.ccm_ext2.auto_param[3].coef[7], ccm_ext2.ccm_ext2.auto_param[3].coef[8]);
			printf("ccm_ext2 auto_param[4].ccm = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext2.ccm_ext2.auto_param[4].coef[0], ccm_ext2.ccm_ext2.auto_param[4].coef[1], ccm_ext2.ccm_ext2.auto_param[4].coef[2]
				, ccm_ext2.ccm_ext2.auto_param[4].coef[3], ccm_ext2.ccm_ext2.auto_param[4].coef[4], ccm_ext2.ccm_ext2.auto_param[4].coef[5]
				, ccm_ext2.ccm_ext2.auto_param[4].coef[6], ccm_ext2.ccm_ext2.auto_param[4].coef[7], ccm_ext2.ccm_ext2.auto_param[4].coef[8]);
			break;

		case 107:
			do {
				printf("Set isp id (0, 1)>> \n");
				gamma_ext.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set ext_sel (0 ~ 2)>> \n");
				gamma_ext.gamma_ext.ext_sel = (UINT32)get_choose_int();
			} while (0);

			gamma_ext.gamma_ext.auto_param[0].lv = 10;
			gamma_ext.gamma_ext.auto_param[0].gamma_level = 100;
			gamma_ext.gamma_ext.auto_param[1].lv = 8;
			gamma_ext.gamma_ext.auto_param[1].gamma_level = 75;
			gamma_ext.gamma_ext.auto_param[2].lv = 6;
			gamma_ext.gamma_ext.auto_param[2].gamma_level = 50;
			gamma_ext.gamma_ext.auto_param[3].lv = 4;
			gamma_ext.gamma_ext.auto_param[3].gamma_level = 25;
			gamma_ext.gamma_ext.auto_param[4].lv = 2;
			gamma_ext.gamma_ext.auto_param[4].gamma_level = 0;

			vendor_isp_set_iq(IQT_ITEM_GAMMA_EXT_PARAM, &gamma_ext);
			printf("gamma_ext id = %d \n", gamma_ext.id);
			printf("gamma_ext ext_sel = %d \n", gamma_ext.gamma_ext.ext_sel);
			printf("gamma_ext gamma_set0_lv = %d \n", gamma_ext.gamma_ext.gamma_set0_lv);
			printf("gamma_ext gamma_set0_lut = {%d, %d, %d, %d, %d, %d, %d, %d, %d, ...} \n"
				, gamma_ext.gamma_ext.gamma_set0_lut[0], gamma_ext.gamma_ext.gamma_set0_lut[1], gamma_ext.gamma_ext.gamma_set0_lut[2]
				, gamma_ext.gamma_ext.gamma_set0_lut[3], gamma_ext.gamma_ext.gamma_set0_lut[4], gamma_ext.gamma_ext.gamma_set0_lut[5]
				, gamma_ext.gamma_ext.gamma_set0_lut[6], gamma_ext.gamma_ext.gamma_set0_lut[7], gamma_ext.gamma_ext.gamma_set0_lut[8]);
			printf("gamma_ext gamma_set1_lv = %d \n", gamma_ext.gamma_ext.gamma_set1_lv);
			printf("gamma_ext gamma_set1_lut = {%d, %d, %d, %d, %d, %d, %d, %d, %d, ...} \n"
				, gamma_ext.gamma_ext.gamma_set1_lut[0], gamma_ext.gamma_ext.gamma_set1_lut[1], gamma_ext.gamma_ext.gamma_set1_lut[2]
				, gamma_ext.gamma_ext.gamma_set1_lut[3], gamma_ext.gamma_ext.gamma_set1_lut[4], gamma_ext.gamma_ext.gamma_set1_lut[5]
				, gamma_ext.gamma_ext.gamma_set1_lut[6], gamma_ext.gamma_ext.gamma_set1_lut[7], gamma_ext.gamma_ext.gamma_set1_lut[8]);
			printf("gamma_ext gamma_set2_lv = %d \n", gamma_ext.gamma_ext.gamma_set2_lv);
			printf("gamma_ext gamma_set2_lut = {%d, %d, %d, %d, %d, %d, %d, %d, %d, ...} \n"
				, gamma_ext.gamma_ext.gamma_set2_lut[0], gamma_ext.gamma_ext.gamma_set2_lut[1], gamma_ext.gamma_ext.gamma_set2_lut[2]
				, gamma_ext.gamma_ext.gamma_set2_lut[3], gamma_ext.gamma_ext.gamma_set2_lut[4], gamma_ext.gamma_ext.gamma_set2_lut[5]
				, gamma_ext.gamma_ext.gamma_set2_lut[6], gamma_ext.gamma_ext.gamma_set2_lut[7], gamma_ext.gamma_ext.gamma_set2_lut[8]);
			printf("gamma_ext gamma_set3_lv = %d \n", gamma_ext.gamma_ext.gamma_set3_lv);
			printf("gamma_ext gamma_set3_lut = {%d, %d, %d, %d, %d, %d, %d, %d, %d, ...} \n"
				, gamma_ext.gamma_ext.gamma_set3_lut[0], gamma_ext.gamma_ext.gamma_set3_lut[1], gamma_ext.gamma_ext.gamma_set3_lut[2]
				, gamma_ext.gamma_ext.gamma_set3_lut[3], gamma_ext.gamma_ext.gamma_set3_lut[4], gamma_ext.gamma_ext.gamma_set3_lut[5]
				, gamma_ext.gamma_ext.gamma_set3_lut[6], gamma_ext.gamma_ext.gamma_set3_lut[7], gamma_ext.gamma_ext.gamma_set3_lut[8]);
			printf("gamma_ext gamma_set4_lv = %d \n", gamma_ext.gamma_ext.gamma_set4_lv);
			printf("gamma_ext gamma_set4_lut = {%d, %d, %d, %d, %d, %d, %d, %d, %d, ...} \n"
				, gamma_ext.gamma_ext.gamma_set4_lut[0], gamma_ext.gamma_ext.gamma_set4_lut[1], gamma_ext.gamma_ext.gamma_set4_lut[2]
				, gamma_ext.gamma_ext.gamma_set4_lut[3], gamma_ext.gamma_ext.gamma_set4_lut[4], gamma_ext.gamma_ext.gamma_set4_lut[5]
				, gamma_ext.gamma_ext.gamma_set4_lut[6], gamma_ext.gamma_ext.gamma_set4_lut[7], gamma_ext.gamma_ext.gamma_set4_lut[8]);
			break;

		case 108:
			{
			CHAR expand_dpc_bin[64] = {USER_EXPAND_BIN_0};
			VOS_FILE fp;
			struct vos_stat stat;

			UINT32 buffer_size;
			UINT16 *dpc_table_va;
			#if USER_EXPAND_DPC_TEST
			UINT32 i = 0, x, y;
			#endif

			if (hd_mem_init == FALSE) {
				hd_common_init(2);
				hd_common_mem_init(NULL);
				hd_mem_init = TRUE;
			}

			file_ret = vos_file_stat((CHAR *)&expand_dpc_bin, &stat);
			if (file_ret == 0) {
				buffer_size = stat.st_size;
				printf("get stat of %s OK, buffer_size = %d \n", &expand_dpc_bin, stat.st_size);
			} else {
				buffer_size = USER_EXPAND_DPC_BUFFER_SIZE;
				printf("get stat of %s NG, buffer_size = %d  \n", &expand_dpc_bin, USER_EXPAND_DPC_BUFFER_SIZE);
			}

			max_free_block.ddr = 0;
			vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_MAX_FREE_BLOCK_SIZE, &max_free_block);
			printf("DDR0 max_free_block size = %d \n", max_free_block.size);

			if (max_free_block.size < buffer_size) {
				printf("DDR0 max_free_block size is too small \n");

				max_free_block.ddr = 1;
				vendor_common_mem_get(VENDOR_COMMON_MEM_ITEM_MAX_FREE_BLOCK_SIZE, &max_free_block);
				printf("DDR1 max_free_block size = %d \n", max_free_block.size);
				if (max_free_block.size < buffer_size) {
					printf("DDR1 max_free_block size is too small \n");
					break;
				};
			};

			ret = hd_common_mem_alloc("DPC", &dpc_buffer.pa, (void **)&dpc_buffer.va, buffer_size, 0);
			if (ret != HD_OK) {
				printf("memory allocate size %d NG \n", buffer_size);
				break;
			};

			dpc_table_va = dpc_buffer.va;
			printf("memory allocate size %d OK, va(0x%lx) \n", buffer_size, dpc_buffer.va);

			printf("Set isp id (0, 5)>> \n");
			expand_dpc.id = (UINT32)get_choose_int();

			printf("Set enable (0, 1)>> \n");
			expand_dpc.expand_dpc.enable = (UINT32)get_choose_int();;

			expand_dpc.expand_dpc.size = buffer_size;
			expand_dpc.expand_dpc.table_phyaddr = dpc_buffer.pa;

			if (file_ret == HD_OK) {
				fp = vos_file_open((CHAR *)&expand_dpc_bin, O_RDONLY, 0);
				if (fp == VOS_FILE_INVALID) {
					printf("open %s fail \n", &expand_dpc_bin);
					break;
				}
				vos_file_read(fp, (UINT32 *)dpc_buffer.va, stat.st_size);
				vos_file_close(fp);
			} else {
				#if USER_EXPAND_DPC_TEST
				for (y = USER_EXPAND_DPC_Y; y < (USER_EXPAND_DPC_Y + USER_EXPAND_DPC_H); y++) {
					*(dpc_table_va + i) = 0x8000 | y;
					i++;
					for (x = USER_EXPAND_DPC_X; x < (USER_EXPAND_DPC_X + USER_EXPAND_DPC_W); x++) {
						*(dpc_table_va + i) = x;
						i++;
						if (i >= buffer_size / sizeof(UINT16)) {
							break;
						}
					}
					if (i >= buffer_size / sizeof(UINT16)) {
						break;
					}
				}
				if (i < buffer_size / sizeof(UINT16)) {
					*(dpc_table_va + i) = 0xFFFF;
				}
				*(dpc_table_va + buffer_size / sizeof(UINT16) - 1) = 0xFFFF;
				#else
				*(dpc_table_va) = 0xFFFF;
				#endif
			}

			vendor_isp_set_iq(IQT_ITEM_EXPAND_DPC_PARAM, &expand_dpc);
			printf("expand_dpc id = %d \n", expand_dpc.id);
			printf("expand_dpc enable = %d \n", expand_dpc.expand_dpc.enable);
			printf("expand_dpc size = %d \n", expand_dpc.expand_dpc.size);
			printf("expand_dpc table_phyaddr =0x%lx \n", expand_dpc.expand_dpc.table_phyaddr);
			printf("expand_dpc table = {0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, ..., 0x%x} \n"
				, *(dpc_table_va + 0), *(dpc_table_va + 1), *(dpc_table_va + 2)
				, *(dpc_table_va + 3), *(dpc_table_va + 4), *(dpc_table_va + 5)
				, *(dpc_table_va + 6), *(dpc_table_va + 7), *(dpc_table_va + 8)
				, *(dpc_table_va + buffer_size / sizeof(UINT16) - 1));

			}
		break;

		case 109:
			printf("Set isp id (0, 5)>> \n");
			shading_dith.id = (UINT32)get_choose_int();

			printf("Set ecs dithering_enable (0, 1)>> \n");
			shading_dith.dithering_enable = (UINT32)get_choose_int();

			vendor_isp_set_iq(IQT_ITEM_SHADING_DITH_PARAM, &shading_dith);
			printf("shading_dith id = %d \n", shading_dith.id);
			printf("shading_dith dithering_enable = %d \n", shading_dith.dithering_enable);
			break;

		case 110:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				nr_lv.lv = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_NR_LV, &nr_lv);
			printf("nr id = %d \n", nr_lv.id);
			printf("nr level = %d \n", nr_lv.lv);
			break;

		case 111:
			do {
				printf("Set isp id (0, 1)>> \n");
				sharpness_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				sharpness_lv.lv = get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_SHARPNESS_LV, &sharpness_lv);
			printf("sharpness id = %d \n", sharpness_lv.id);
			printf("sharpness level = %d \n", sharpness_lv.lv);
			break;

		case 112:
			do {
				printf("Set isp id (0, 1)>> \n");
				saturation_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				saturation_lv.lv = get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_SATURATION_LV, &saturation_lv);
			printf("saturation id = %d \n", saturation_lv.id);
			printf("saturation level = %d \n", saturation_lv.lv);
			break;

		case 113:
			do {
				printf("Set isp id (0, 1)>> \n");
				contrast_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				contrast_lv.lv = get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_CONTRAST_LV, &contrast_lv);
			printf("contrast id = %d \n", contrast_lv.id);
			printf("contrast level = %d \n", contrast_lv.lv);
			break;

		case 114:
			do {
				printf("Set isp id (0, 1)>> \n");
				brightness_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				brightness_lv.lv = get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_BRIGHTNESS_LV, &brightness_lv);
			printf("brightness id = %d \n", brightness_lv.id);
			printf("brightness level = %d \n", brightness_lv.lv);
			break;

		case 115:
			do {
				printf("Set isp id (0, 1)>> \n");
				night_mode.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set mode (0 = OFF ; 1 = ON)>> \n");
				night_mode.mode = get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_NIGHT_MODE, &night_mode);
			printf("night id = %d \n", night_mode.id);
			printf("night mode = %d \n", night_mode.mode);
			break;

		case 116:
			do {
				printf("Set isp id (0, 1)>> \n");
				ycc_format.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set format (0 ~ %d)>> \n", IQ_UI_YCC_OUT_MAX_CNT - 1);
				ycc_format.format = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_YCC_FORMAT, &ycc_format);
			printf("ycc_format id = %d \n", ycc_format.id);
			printf("ycc_format format = %d \n", ycc_format.format);
			break;

		case 117:
			do {
				printf("Set isp id (0, 1)>> \n");
				operation.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set operation (0 ~ %d)>> \n", IQ_UI_OPERATION_MAX_CNT - 1);
				operation.operation = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_OPERATION, &operation);
			printf("operation id = %d \n", operation.id);
			printf("operation sel = %d \n", operation.operation);
			break;

		case 118:
			do {
				printf("Set isp id (0, 1)>> \n");
				imageeffect.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set effect (0 ~ %d)>> \n", IQ_UI_IMAGEEFFECT_MAX_CNT - 1);
				imageeffect.effect = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_IMAGEEFFECT, &imageeffect);
			printf("imageeffect id = %d \n", imageeffect.id);
			printf("imageeffect effect = %d \n", imageeffect.effect);
			break;

		case 119:
			do {
				printf("Set isp id (0, 1)>> \n");
				ccid.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set ccid (0 ~ %d)>> \n", IQ_UI_CCID_MAX_CNT - 1);
				ccid.ccid = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_CCID, &ccid);
			printf("color id = %d \n", ccid.id);
			printf("color ccid = %d \n", ccid.ccid);
			break;

		case 120:
			do {
				printf("Set isp id (0, 1)>> \n");
				hue_shift.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set hue_shift (0 ~ %d)>> \n", IQ_UI_HUE_SHIFT_MAX_CNT - 1);
				hue_shift.hue_shift = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_HUE_SHIFT, &hue_shift);
			printf("hue id = %d \n", hue_shift.id);
			printf("hue shift = %d \n", hue_shift.hue_shift);
			break;

		case 121:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr_tone_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set shdr_tone_lv (0 ~ %d)>> \n", IQ_UI_SHDR_TONE_LV_MAX_CNT - 1);
				shdr_tone_lv.lv = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_SHDR_TONE_LV, &shdr_tone_lv);
			printf("shdr_tone id = %d \n", shdr_tone_lv.id);
			printf("shdr_tone level = %d \n", shdr_tone_lv.lv);
			break;

		case 122:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr3d_lv.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set lv (0 ~ 200)>> \n");
				nr3d_lv.lv = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_3DNR_LV, &nr3d_lv);
			printf("3dnr id = %d \n", nr3d_lv.id);
			printf("3dnr level = %d \n", nr3d_lv.lv);
			break;

		case 123:
			do {
				printf("Set isp id (0, 1)>> \n");
				ccm_ext.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set int_tab[0]>> \n");
				ccm_ext.ccm_ext.manual_param.int_tab[0] = (UINT32)get_choose_int();
			} while (0);

			printf("Set int_tab[1]>> \n");
			ccm_ext.ccm_ext.manual_param.int_tab[1] = (UINT32)get_choose_int();

			printf("Set int_tab[2]>> \n");
			ccm_ext.ccm_ext.manual_param.int_tab[2] = (UINT32)get_choose_int();

			printf("Set int_tab[3]>> \n");
			ccm_ext.ccm_ext.manual_param.int_tab[3] = (UINT32)get_choose_int();

			vendor_isp_set_iq(IQT_ITEM_CCM_EXT_PARAM, &ccm_ext);
			printf("ccm_ext id = %d \n", ccm_ext.id);
			printf("ccm_ext manual_param.int_tab = {%d, %d, %d, %d, %d, %d, ..., %d, %d, %d} \n"
				, ccm_ext.ccm_ext.manual_param.int_tab[0], ccm_ext.ccm_ext.manual_param.int_tab[1], ccm_ext.ccm_ext.manual_param.int_tab[2]
				, ccm_ext.ccm_ext.manual_param.int_tab[3], ccm_ext.ccm_ext.manual_param.int_tab[4], ccm_ext.ccm_ext.manual_param.int_tab[5]
				, ccm_ext.ccm_ext.manual_param.int_tab[21], ccm_ext.ccm_ext.manual_param.int_tab[22], ccm_ext.ccm_ext.manual_param.int_tab[23]);
			printf("ccm_ext auto_param[0].int_tab = {%d, %d, %d, %d, %d, %d, %d, %d, %d} \n"
				, ccm_ext.ccm_ext.auto_param[0].int_tab[0], ccm_ext.ccm_ext.auto_param[0].int_tab[1], ccm_ext.ccm_ext.auto_param[0].int_tab[2]
				, ccm_ext.ccm_ext.auto_param[0].int_tab[3], ccm_ext.ccm_ext.auto_param[0].int_tab[4], ccm_ext.ccm_ext.auto_param[0].int_tab[5]
				, ccm_ext.ccm_ext.auto_param[0].int_tab[21], ccm_ext.ccm_ext.auto_param[0].int_tab[22], ccm_ext.ccm_ext.auto_param[0].int_tab[23]);
			break;

		case 124:
			do {
				printf("Set isp id (0, 1)>> \n");
				shading_vig_ext.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set vig_fisheye_enable (0 ~ 1)>> \n");
				shading_vig_ext.shading_vig_ext.vig_fisheye_enable = (UINT32)get_choose_int();;
			} while (0);

			do {
				printf("Set vig_fisheye_radius (0 ~ 1023)>> \n");
				shading_vig_ext.shading_vig_ext.vig_fisheye_radius = (UINT32)get_choose_int();;
			} while (0);

			do {
				printf("Set vig_fisheye_slope (0 ~ 15)>> \n");
				shading_vig_ext.shading_vig_ext.vig_fisheye_slope = (UINT8)get_choose_int();;
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_SHADING_VIG_EXT_PARAM, &shading_vig_ext);
			printf("shading_vig_ext id = %d \n", shading_vig_ext.id);
			printf("shading_vig_ext vig_fisheye_enable = %d \n", shading_vig_ext.shading_vig_ext.vig_fisheye_enable);
			printf("shading_vig_ext vig_fisheye_radius = %d \n", shading_vig_ext.shading_vig_ext.vig_fisheye_radius);
			printf("shading_vig_ext vig_fisheye_slope = %d \n", shading_vig_ext.shading_vig_ext.vig_fisheye_slope);
			break;

		case 125:
			do {
				printf("Set isp id (0, 1)>> \n");
				shdr_ext.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set fcurve_y_w_lut (0 ~ 255)>> \n");
				tmp = (UINT8)get_choose_int();
				for (i = 0; i < IQ_SHDR_FCURVE_Y_W_NUM; i++) {
					shdr_ext.shdr_ext.fcurve_y_w_lut[i] = tmp;
				}
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_SHDR_EXT_PARAM, &shdr_ext);
			printf("shdr_ext id = %d \n", shdr_ext.id);
			printf("shdr_ext fcurve_y_w_lut = %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d \n"
				, shdr_ext.shdr_ext.fcurve_y_w_lut[0], shdr_ext.shdr_ext.fcurve_y_w_lut[1], shdr_ext.shdr_ext.fcurve_y_w_lut[2]
				, shdr_ext.shdr_ext.fcurve_y_w_lut[3], shdr_ext.shdr_ext.fcurve_y_w_lut[4], shdr_ext.shdr_ext.fcurve_y_w_lut[5]
				, shdr_ext.shdr_ext.fcurve_y_w_lut[6], shdr_ext.shdr_ext.fcurve_y_w_lut[7], shdr_ext.shdr_ext.fcurve_y_w_lut[8]
				, shdr_ext.shdr_ext.fcurve_y_w_lut[9], shdr_ext.shdr_ext.fcurve_y_w_lut[10], shdr_ext.shdr_ext.fcurve_y_w_lut[11]
				, shdr_ext.shdr_ext.fcurve_y_w_lut[12], shdr_ext.shdr_ext.fcurve_y_w_lut[13], shdr_ext.shdr_ext.fcurve_y_w_lut[14]
				, shdr_ext.shdr_ext.fcurve_y_w_lut[15], shdr_ext.shdr_ext.fcurve_y_w_lut[16]);
			break;

		case 128:
			do {
				printf("Set isp id (0, 1)>> \n");
				cst.id = (UINT32)get_choose_int();
			} while (0);

			cst.cst_coef[0] = 77;
			cst.cst_coef[1] = 150;
			cst.cst_coef[2] = 29;
			cst.cst_coef[3] = 0;
			cst.cst_coef[4] = 0;
			cst.cst_coef[5] = 0;
			cst.cst_coef[6] = 0;
			cst.cst_coef[7] = 0;
			cst.cst_coef[8] = 0;

			cst.cstp_ratio = 0;

			vendor_isp_set_iq(IQT_ITEM_CST_PARAM, &cst);
			printf("cst id = %d \n", cst.id);
			printf("cst cst_coef = {%d, %d, %d, %d, %d, %d, %d, %d, %d}, \n"
				, cst.cst_coef[0], cst.cst_coef[1], cst.cst_coef[2], cst.cst_coef[3], cst.cst_coef[4]
				, cst.cst_coef[5], cst.cst_coef[6], cst.cst_coef[7], cst.cst_coef[8]);
			printf("cst cstp_ratio = %d \n", cst.cstp_ratio);
			break;

		case 129:
			do {
				printf("Set isp id (0, 1)>> \n");
				stripe.id = (UINT32)get_choose_int();
			} while (0);

			stripe.stripe_type = IQT_STRIPE_MANUAL_4STRIPE;

			vendor_isp_set_iq(IQT_ITEM_STRIPE_PARAM, &stripe);
			printf("stripe id = %d \n", stripe.id);
			printf("stripe stripe_type = %d, \n", stripe.stripe_type);
			break;

		case 130:
			do {
				printf("Set isp id (0, 1)>> \n");
				lca_sub_ratio.id = (UINT32)get_choose_int();
				printf("Set lca sub ratio (64 ~ 250)>> \n");
				lca_sub_ratio.lca_sub_ratio.ratio = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_LCA_SUB_RATIO_PARAM, &lca_sub_ratio);
			printf("lca_sub_ratio id = %d \n", lca_sub_ratio.id);
			printf("lca_sub_ratio ratio = %d \n", lca_sub_ratio.lca_sub_ratio.ratio);
			break;

		case 153:
			do {
				printf("Set isp id (0, 1)>> \n");
				nr_3d_misc.id = (UINT32)get_choose_int();
			} while (0);

			nr_3d_misc._3dnr_misc.mc_roi[0] = 1;
			nr_3d_misc._3dnr_misc.mc_roi[1] = 2;
			nr_3d_misc._3dnr_misc.md_roi[0] = 3;
			nr_3d_misc._3dnr_misc.md_roi[1] = 4;
			nr_3d_misc._3dnr_misc.roi_mv_th = 11;
			nr_3d_misc._3dnr_misc.ds_th_roi = 22;

			vendor_isp_set_iq(IQT_ITEM_3DNR_MISC_PARAM, &nr_3d_misc);
			printf("nr_3d_misc id = %d \n", nr_3d_misc.id);
			printf("nr_3d_misc param = %d %d %d %d %d %d \n", nr_3d_misc._3dnr_misc.mc_roi[0], nr_3d_misc._3dnr_misc.mc_roi[1], 
												nr_3d_misc._3dnr_misc.md_roi[0], nr_3d_misc._3dnr_misc.md_roi[1], 
												nr_3d_misc._3dnr_misc.roi_mv_th, nr_3d_misc._3dnr_misc.ds_th_roi);
			break;

		case 159:
			do {
				printf("Set isp id (0, 1)>> \n");
				edge_region.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set str_flat (0 ~ 64)>> \n");
				edge_region.edge_region.manual_param.str_flat = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set str_edge (64 ~ 255)>> \n");
				edge_region.edge_region.manual_param.str_edge = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set th_flat_low (0 ~ 1023)>> \n");
				edge_region.edge_region.manual_param.th_flat_low = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set th_edge_high (0 ~ 1023)>> \n");
				edge_region.edge_region.manual_param.th_edge_high = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_EDGE_REGION_PARAM, &edge_region);
			printf("edge_region id = %d \n", edge_region.id);
			printf("edge_region manual str_flat = %d \n", edge_region.edge_region.manual_param.str_flat);
			printf("edge_region manual str_edge = %d \n", edge_region.edge_region.manual_param.str_edge);
			printf("edge_region manual th_flat_low = %d \n", edge_region.edge_region.manual_param.th_flat_low);
			printf("edge_region manual th_edge_high = %d \n", edge_region.edge_region.manual_param.th_edge_high);
			break;

		case 200:
			do {
				printf("Set isp id (0, 1)>> \n");
				dark_enh_ratio.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_NNSC_DARK_ENH_RATIO, &dark_enh_ratio);
			printf("dark_enh_ratio id = %d \n", dark_enh_ratio.id);
			printf("dark_enh_ratio ratio = %d \n", dark_enh_ratio.ratio);
			break;

		case 201:
			do {
				printf("Set isp id (0, 1)>> \n");
				contrast_enh_ratio.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_NNSC_CONTRAST_ENH_RATIO, &contrast_enh_ratio);
			printf("contrast_enh_ratio id = %d \n", contrast_enh_ratio.id);
			printf("contrast_enh_ratio ratio = %d \n", contrast_enh_ratio.ratio);
			break;

		case 202:
			do {
				printf("Set isp id (0, 1)>> \n");
				green_enh_ratio.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_NNSC_GREEN_ENH_RATIO, &green_enh_ratio);
			printf("green_enh_ratio id = %d \n", green_enh_ratio.id);
			printf("green_enh_ratio ratio = %d \n", green_enh_ratio.ratio);
			break;

		case 203:
			do {
				printf("Set isp id (0, 1)>> \n");
				skin_enh_ratio.id = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_get_iq(IQT_ITEM_NNSC_SKIN_ENH_RATIO, &skin_enh_ratio);
			printf("dark_enh_ratio id = %d \n", skin_enh_ratio.id);
			printf("dark_enh_ratio ratio = %d \n", skin_enh_ratio.ratio);
			break;

		case 300:
			do {
				printf("Set isp id (0, 1)>> \n");
				dark_enh_ratio.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set ratio (0 ~ 100)>> \n");
				dark_enh_ratio.ratio = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_NNSC_DARK_ENH_RATIO, &dark_enh_ratio);
			printf("dark_enh_ratio id = %d \n", dark_enh_ratio.id);
			printf("dark_enh_ratio ratio = %d \n", dark_enh_ratio.ratio);
			break;

		case 301:
			do {
				printf("Set isp id (0, 1)>> \n");
				contrast_enh_ratio.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set ratio (0 ~ 100)>> \n");
				contrast_enh_ratio.ratio = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_NNSC_CONTRAST_ENH_RATIO, &contrast_enh_ratio);
			printf("contrast_enh_ratio id = %d \n", contrast_enh_ratio.id);
			printf("contrast_enh_ratio ratio = %d \n", contrast_enh_ratio.ratio);
			break;

		case 302:
			do {
				printf("Set isp id (0, 1)>> \n");
				green_enh_ratio.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set ratio (0 ~ 100)>> \n");
				green_enh_ratio.ratio = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_NNSC_GREEN_ENH_RATIO, &green_enh_ratio);
			printf("green_enh_ratio id = %d \n", green_enh_ratio.id);
			printf("green_enh_ratio ratio = %d \n", green_enh_ratio.ratio);
			break;

		case 303:
			do {
				printf("Set isp id (0, 1)>> \n");
				skin_enh_ratio.id = (UINT32)get_choose_int();
			} while (0);

			do {
				printf("Set ratio (0 ~ 100)>> \n");
				skin_enh_ratio.ratio = (UINT32)get_choose_int();
			} while (0);

			vendor_isp_set_iq(IQT_ITEM_NNSC_SKIN_ENH_RATIO, &skin_enh_ratio);
			printf("skin_enh_ratio id = %d \n", skin_enh_ratio.id);
			printf("skin_enh_ratio ratio = %d \n", skin_enh_ratio.ratio);
			break;

		case 500:
			do {
				printf("Set isp id (0, 1)>> \n");
				tmp = (UINT32)get_choose_int();
				printf("Set auto tone level enable (0, 1)>> \n");
				tone_level_en = (BOOL)get_choose_int();
				printf("Set debug enable (0, 1)>> \n");
				tone_level_dbg = (BOOL)get_choose_int();
			} while (0);

			printf("dr_level id = %d \n", dr_level.id);

			iq_tonelv_conti_run = 1;
			if (pthread_create(&iq_tonelv_thread_id, NULL, iq_tonelv_thread, &tmp) < 0) {
				printf("create iq_tonelv thread failed");
				break;
			}

			do {
				printf("Enter 0 to exit >> \n");
				iq_tonelv_conti_run = (UINT32)get_choose_int();
			} while (0);

			// destroy encode thread
			pthread_join(iq_tonelv_thread_id, NULL);

			break;

		case 0:
		default:
			trig = 0;
			break;
		}
	}

	vendor_isp_uninit();

	return 0;
}
