#ifdef __KERNEL__
#include <linux/slab.h>
#else
#include <stdio.h>
#endif

#include "kwrap/type.h"
#include "isp_dbg.h"
#include "isp_mtr.h"
#include "isp_api.h"
#include "ispt_api.h"
#include "aet_api.h"
#include "aft_api.h"
#include "awbt_api.h"
#include "iqt_api.h"

#define MAX_CNT 20
static UINT32 isp_dbg_msg[ISP_ID_MAX_NUM] = {0};
static UINT32 isp_bypass_eng[ISP_ID_MAX_NUM] = {0};
static UINT32 isp_ioc_control;
static UINT32 err_msg_cnt;
static UINT32 wrn_msg_cnt;
CHAR ioc_item_name[64];

UINT32 isp_dbg_get_dbg_mode(ISP_ID id)
{
	if (id >= ISP_ID_MAX_NUM) {
		return isp_dbg_msg[0];
	}

	return isp_dbg_msg[id];
}

void isp_dbg_set_dbg_mode(ISP_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= ISP_ID_MAX_NUM) {
		for (i = 0; i < ISP_ID_MAX_NUM; i++) {
			isp_dbg_msg[i] = cmd;
		}
	} else {
		isp_dbg_msg[id] = cmd;
	}

	if (cmd == ISP_DBG_NONE) {
		isp_dbg_clr_err_msg();
	}
}

UINT32 isp_dbg_get_bypass_eng(ISP_ID id)
{
	if (id >= ISP_ID_MAX_NUM) {
		return isp_bypass_eng[0];
	}

	return isp_bypass_eng[id];
}

void isp_dbg_set_bypass_eng(ISP_ID id, UINT32 cmd)
{
	UINT32 i;

	if (id >= ISP_ID_MAX_NUM) {
		for (i = 0; i < ISP_ID_MAX_NUM; i++) {
			isp_bypass_eng[i] = cmd;
		}
	} else {
		isp_bypass_eng[id] = cmd;
	}
}

UINT32 isp_dbg_get_ioc_control(void)
{
	return isp_ioc_control;
}

void isp_dbg_set_ioc_control(UINT32 cmd)
{
	isp_ioc_control = cmd;
}

BOOL isp_dbg_check_err_msg(BOOL show_dbg_msg)
{
	BOOL rt = TRUE;

	if ((err_msg_cnt < MAX_CNT) && (!show_dbg_msg)) {
		rt = TRUE;
	} else {
		rt = FALSE;
	}
	err_msg_cnt++;

	return rt;
}

void isp_dbg_clr_err_msg(void)
{
	err_msg_cnt = 0;
}

UINT32 isp_dbg_get_err_msg(void)
{
	return err_msg_cnt;
}

BOOL isp_dbg_check_wrn_msg(BOOL show_dbg_msg)
{
	BOOL rt = TRUE;

	if ((wrn_msg_cnt < MAX_CNT) && (!show_dbg_msg)) {
		rt = TRUE;
	} else {
		rt = FALSE;
	}
	wrn_msg_cnt++;

	return rt;
}

void isp_dbg_clr_wrn_msg(void)
{
	wrn_msg_cnt = 0;
}

UINT32 isp_dbg_get_wrn_msg(void)
{
	return wrn_msg_cnt;
}

CHAR *isp_dbg_get_ispt_item(UINT32 item)
{
	switch (item) {
		case ISPT_ITEM_VERSION: sprintf(ioc_item_name, "ISPT_ITEM_VERSION"); break;
		case ISPT_ITEM_SIZE_TAB: sprintf(ioc_item_name, "ISPT_ITEM_SIZE_TAB"); break;
		case ISPT_ITEM_FUNC: sprintf(ioc_item_name, "ISPT_ITEM_FUNC"); break;
		case ISPT_ITEM_YUV: sprintf(ioc_item_name, "ISPT_ITEM_YUV"); break;
		case ISPT_ITEM_RAW: sprintf(ioc_item_name, "ISPT_ITEM_RAW"); break;
		case ISPT_ITEM_FRAME: sprintf(ioc_item_name, "ISPT_ITEM_FRAME"); break;
		case ISPT_ITEM_MEMORY: sprintf(ioc_item_name, "ISPT_ITEM_MEMORY"); break;
		case ISPT_ITEM_SENSOR_INFO: sprintf(ioc_item_name, "ISPT_ITEM_SENSOR_INFO"); break;
		case ISPT_ITEM_SENSOR_REG: sprintf(ioc_item_name, "ISPT_ITEM_SENSOR_REG"); break;
		case ISPT_ITEM_SENSOR_MODE_INFO: sprintf(ioc_item_name, "ISPT_ITEM_SENSOR_MODE_INFO"); break;
		case ISPT_ITEM_3DNR_STA: sprintf(ioc_item_name, "ISPT_ITEM_3DNR_STA"); break;
		case ISPT_ITEM_EMU_NEW_BUF: sprintf(ioc_item_name, "ISPT_ITEM_EMU_NEW_BUF"); break;
		case ISPT_ITEM_EMU_RUN: sprintf(ioc_item_name, "ISPT_ITEM_EMU_RUN"); break;
		case ISPT_ITEM_CHIP_INFO: sprintf(ioc_item_name, "ISPT_ITEM_CHIP_INFO"); break;
		case ISPT_ITEM_CA_DATA: sprintf(ioc_item_name, "ISPT_ITEM_CA_DATA"); break;
		case ISPT_ITEM_LA_DATA: sprintf(ioc_item_name, "ISPT_ITEM_LA_DATA"); break;
		case ISPT_ITEM_VA_DATA: sprintf(ioc_item_name, "ISPT_ITEM_VA_DATA"); break;
		case ISPT_ITEM_VA_INDEP_DATA: sprintf(ioc_item_name, "ISPT_ITEM_VA_INDEP_DATA"); break;
		case ISPT_ITEM_WAIT_VD: sprintf(ioc_item_name, "ISPT_ITEM_WAIT_VD"); break;
		case ISPT_ITEM_SENSOR_EXPT: sprintf(ioc_item_name, "ISPT_ITEM_SENSOR_EXPT"); break;
		case ISPT_ITEM_SENSOR_GAIN: sprintf(ioc_item_name, "ISPT_ITEM_SENSOR_GAIN"); break;
		case ISPT_ITEM_D_GAIN: sprintf(ioc_item_name, "ISPT_ITEM_D_GAIN"); break;
		case ISPT_ITEM_C_GAIN: sprintf(ioc_item_name, "ISPT_ITEM_C_GAIN"); break;
		case ISPT_ITEM_TOTAL_GAIN: sprintf(ioc_item_name, "ISPT_ITEM_TOTAL_GAIN"); break;
		case ISPT_ITEM_LV: sprintf(ioc_item_name, "ISPT_ITEM_LV"); break;
		case ISPT_ITEM_CT: sprintf(ioc_item_name, "ISPT_ITEM_CT"); break;
		case ISPT_ITEM_MOTOR_IRIS: sprintf(ioc_item_name, "ISPT_ITEM_MOTOR_IRIS"); break;
		case ISPT_ITEM_MOTOR_FOCUS: sprintf(ioc_item_name, "ISPT_ITEM_MOTOR_FOCUS"); break;
		case ISPT_ITEM_MOTOR_ZOOM: sprintf(ioc_item_name, "ISPT_ITEM_MOTOR_ZOOM"); break;
		case ISPT_ITEM_MOTOR_MISC: sprintf(ioc_item_name, "ISPT_ITEM_MOTOR_MISC"); break;
		case ISPT_ITEM_SENSOR_DIRECTION: sprintf(ioc_item_name, "ISPT_ITEM_SENSOR_DIRECTION"); break;
		case ISPT_ITEM_SENSOR_SLEEP: sprintf(ioc_item_name, "ISPT_ITEM_SENSOR_SLEEP"); break;
		case ISPT_ITEM_SENSOR_WAKEUP: sprintf(ioc_item_name, "ISPT_ITEM_SENSOR_WAKEUP"); break;
		case ISPT_ITEM_HISTO_DATA: sprintf(ioc_item_name, "ISPT_ITEM_HISTO_DATA"); break;
		case ISPT_ITEM_IR_INFO: sprintf(ioc_item_name, "ISPT_ITEM_IR_INFO"); break;
		case ISPT_ITEM_MD_DATA: sprintf(ioc_item_name, "ISPT_ITEM_MD_DATA"); break;
		case ISPT_ITEM_MD_STA: sprintf(ioc_item_name, "ISPT_ITEM_MD_STA"); break;
		default: sprintf(ioc_item_name, "ispt item (%d)", (int)item); break;
	}

	return ioc_item_name;
}

CHAR *isp_dbg_get_aet_item(UINT32 item)
{
	switch (item) {
		case AET_ITEM_VERSION: sprintf(ioc_item_name, "AET_ITEM_VERSION"); break;
		case AET_ITEM_SIZE_TAB: sprintf(ioc_item_name, "AET_ITEM_SIZE_TAB"); break;
		case AET_ITEM_RLD_CONFIG: sprintf(ioc_item_name, "AET_ITEM_RLD_CONFIG"); break;
		case AET_ITEM_RLD_DTSI: sprintf(ioc_item_name, "AET_ITEM_RLD_DTSI"); break;
		case AET_ITEM_FREQUENCY: sprintf(ioc_item_name, "AET_ITEM_FREQUENCY"); break;
		case AET_ITEM_METER: sprintf(ioc_item_name, "AET_ITEM_METER"); break;
		case AET_ITEM_EV: sprintf(ioc_item_name, "AET_ITEM_EV"); break;
		case AET_ITEM_ISO: sprintf(ioc_item_name, "AET_ITEM_ISO"); break;
		case AET_ITEM_LONGEXP: sprintf(ioc_item_name, "AET_ITEM_LONGEXP"); break;
		case AET_ITEM_LONGEXP_EXPT: sprintf(ioc_item_name, "AET_ITEM_LONGEXP_EXPT"); break;
		case AET_ITEM_LONGEXP_ISO: sprintf(ioc_item_name, "AET_ITEM_LONGEXP_ISO"); break;
		case AET_ITEM_OPERATION: sprintf(ioc_item_name, "AET_ITEM_OPERATION"); break;
		case AET_ITEM_DCIRIS: sprintf(ioc_item_name, "AET_ITEM_DCIRIS"); break;
		case AET_ITEM_STITCH_ID: sprintf(ioc_item_name, "AET_ITEM_STITCH_ID"); break;
		case AET_ITEM_APERTURE_BOUND: sprintf(ioc_item_name, "AET_ITEM_APERTURE_BOUND"); break;
		case AET_ITEM_BASE_ISO: sprintf(ioc_item_name, "AET_ITEM_BASE_ISO"); break;
		case AET_ITEM_BASE_GAIN_RATIO: sprintf(ioc_item_name, "AET_ITEM_BASE_GAIN_RATIO"); break;
		case AET_ITEM_EXPECT_LUM: sprintf(ioc_item_name, "AET_ITEM_EXPECT_LUM"); break;
		case AET_ITEM_LA_CLAMP: sprintf(ioc_item_name, "AET_ITEM_LA_CLAMP"); break;
		case AET_ITEM_OVER_EXPOSURE: sprintf(ioc_item_name, "AET_ITEM_OVER_EXPOSURE"); break;
		case AET_ITEM_CONVERGENCE: sprintf(ioc_item_name, "AET_ITEM_CONVERGENCE"); break;
		case AET_ITEM_CURVE_GEN_MOVIE: sprintf(ioc_item_name, "AET_ITEM_CURVE_GEN_MOVIE"); break;
		case AET_ITEM_METER_WIN: sprintf(ioc_item_name, "AET_ITEM_METER_WIN"); break;
		case AET_ITEM_LUM_GAMMA: sprintf(ioc_item_name, "AET_ITEM_LUM_GAMMA"); break;
		case AET_ITEM_SHDR: sprintf(ioc_item_name, "AET_ITEM_SHDR"); break;
		case AET_ITEM_IRIS_CFG: sprintf(ioc_item_name, "AET_ITEM_IRIS_CFG"); break;
		case AET_ITEM_CURVE_GEN_PHOTO: sprintf(ioc_item_name, "AET_ITEM_CURVE_GEN_PHOTO"); break;
		case AET_ITEM_MANUAL: sprintf(ioc_item_name, "AET_ITEM_MANUAL"); break;
		case AET_ITEM_STATUS: sprintf(ioc_item_name, "AET_ITEM_STATUS"); break;
		case AET_ITEM_PRIORITY: sprintf(ioc_item_name, "AET_ITEM_PRIORITY"); break;
		case AET_ITEM_ROI_WIN: sprintf(ioc_item_name, "AET_ITEM_ROI_WIN"); break;
		case AET_ITEM_SMART_IR: sprintf(ioc_item_name, "AET_ITEM_SMART_IR"); break;
		case AET_ITEM_EXPT_BOUND: sprintf(ioc_item_name, "AET_ITEM_EXPT_BOUND"); break;
		case AET_ITEM_GAIN_BOUND: sprintf(ioc_item_name, "AET_ITEM_GAIN_BOUND"); break;
		case AET_ITEM_LA_WIN: sprintf(ioc_item_name, "AET_ITEM_LA_WIN"); break;
		default: sprintf(ioc_item_name, "aet item (%d)", (int)item); break;
	}

	return ioc_item_name;
}

CHAR *isp_dbg_get_aft_item(UINT32 item)
{
	switch (item) {
		case AFT_ITEM_VERSION: sprintf(ioc_item_name, "AFT_ITEM_VERSION"); break;
		case AFT_ITEM_SIZE_TAB: sprintf(ioc_item_name, "AFT_ITEM_SIZE_TAB"); break;
		case AFT_ITEM_RLD_CONFIG: sprintf(ioc_item_name, "AFT_ITEM_RLD_CONFIG"); break;
		case AFT_ITEM_RLD_DTSI: sprintf(ioc_item_name, "AFT_ITEM_RLD_DTSI"); break;
		case AFT_ITEM_OPERATION: sprintf(ioc_item_name, "AFT_ITEM_OPERATION"); break;
		case AFT_ITEM_ENABLE: sprintf(ioc_item_name, "AFT_ITEM_ENABLE"); break;
		case AFT_ITEM_ALG_METHOD: sprintf(ioc_item_name, "AFT_ITEM_ALG_METHOD"); break;
		case AFT_ITEM_SHOT_MODE: sprintf(ioc_item_name, "AFT_ITEM_SHOT_MODE"); break;
		case AFT_ITEM_SEARCH_DIR: sprintf(ioc_item_name, "AFT_ITEM_SEARCH_DIR"); break;
		case AFT_ITEM_SKIP_FRAME: sprintf(ioc_item_name, "AFT_ITEM_SKIP_FRAME"); break;
		case AFT_ITEM_THRES: sprintf(ioc_item_name, "AFT_ITEM_THRES"); break;
		case AFT_ITEM_STEP_SIZE: sprintf(ioc_item_name, "AFT_ITEM_STEP_SIZE"); break;
		case AFT_ITEM_MAX_COUNT: sprintf(ioc_item_name, "AFT_ITEM_MAX_COUNT"); break;
		case AFT_ITEM_WIN_WEIGHT: sprintf(ioc_item_name, "AFT_ITEM_WIN_WEIGHT"); break;
		case AFT_ITEM_VA_STA: sprintf(ioc_item_name, "AFT_ITEM_VA_STA"); break;
		case AFT_ITEM_RETRIGGER: sprintf(ioc_item_name, "AFT_ITEM_RETRIGGER"); break;
		case AFT_ITEM_DBG_MSG: sprintf(ioc_item_name, "AFT_ITEM_DBG_MSG"); break;
		case AFT_ITEM_EXEC_STATUS: sprintf(ioc_item_name, "AFT_ITEM_EXEC_STATUS"); break;
		case AFT_ITEM_SEN_RESOLUTION: sprintf(ioc_item_name, "AFT_ITEM_SEN_RESOLUTION"); break;
		case AFT_ITEM_NIGHT_MODE: sprintf(ioc_item_name, "AFT_ITEM_NIGHT_MODE"); break;
		case AFT_ITEM_ALG_CMD: sprintf(ioc_item_name, "AFT_ITEM_ALG_CMD"); break;
		case AFT_ITEM_VA_OPT: sprintf(ioc_item_name, "AFT_ITEM_VA_OPT"); break;
		case AFT_ITEM_BACKLASH_COMP: sprintf(ioc_item_name, "AFT_ITEM_BACKLASH_COMP"); break;
		default: sprintf(ioc_item_name, "aft item (%d)", (int)item); break;
	}

	return ioc_item_name;
}

CHAR *isp_dbg_get_awbt_item(UINT32 item)
{
	switch (item) {
		case AWBT_ITEM_VERSION: sprintf(ioc_item_name, "AWBT_ITEM_VERSION"); break;
		case AWBT_ITEM_SIZE_TAB: sprintf(ioc_item_name, "AWBT_ITEM_SIZE_TAB"); break;
		case AWBT_ITEM_RLD_CONFIG: sprintf(ioc_item_name, "AWBT_ITEM_RLD_CONFIG"); break;
		case AWBT_ITEM_RLD_DTSI: sprintf(ioc_item_name, "AWBT_ITEM_RLD_DTSI"); break;
		case AWBT_ITEM_SCENE: sprintf(ioc_item_name, "AWBT_ITEM_SCENE"); break;
		case AWBT_ITEM_WB_RATIO: sprintf(ioc_item_name, "AWBT_ITEM_WB_RATIO"); break;
		case AWBT_ITEM_OPERATION: sprintf(ioc_item_name, "AWBT_ITEM_OPERATION"); break;
		case AWBT_ITEM_STITCH_ID: sprintf(ioc_item_name, "AWBT_ITEM_STITCH_ID"); break;
		case AWBT_ITEM_CA_TH: sprintf(ioc_item_name, "AWBT_ITEM_CA_TH"); break;
		case AWBT_ITEM_TH: sprintf(ioc_item_name, "AWBT_ITEM_TH"); break;
		case AWBT_ITEM_LV: sprintf(ioc_item_name, "AWBT_ITEM_LV"); break;
		case AWBT_ITEM_CT_WEIGHT: sprintf(ioc_item_name, "AWBT_ITEM_CT_WEIGHT"); break;
		case AWBT_ITEM_TARGET: sprintf(ioc_item_name, "AWBT_ITEM_TARGET"); break;
		case AWBT_ITEM_CT_INFO: sprintf(ioc_item_name, "AWBT_ITEM_CT_INFO"); break;
		case AWBT_ITEM_MWB_GAIN: sprintf(ioc_item_name, "AWBT_ITEM_MWB_GAIN"); break;
		case AWBT_ITEM_CONVERGE: sprintf(ioc_item_name, "AWBT_ITEM_CONVERGE"); break;
		case AWBT_ITEM_EXPAND_BLOCK: sprintf(ioc_item_name, "AWBT_ITEM_EXPAND_BLOCK"); break;
		case AWBT_ITEM_MANUAL: sprintf(ioc_item_name, "AWBT_ITEM_MANUAL"); break;
		case AWBT_ITEM_STATUS: sprintf(ioc_item_name, "AWBT_ITEM_STATUS"); break;
		case AWBT_ITEM_CA: sprintf(ioc_item_name, "AWBT_ITEM_CA"); break;
		case AWBT_ITEM_FLAG: sprintf(ioc_item_name, "AWBT_ITEM_FLAG"); break;
		case AWBT_ITEM_LUMA_WEIGHT: sprintf(ioc_item_name, "AWBT_ITEM_LUMA_WEIGHT"); break;
		case AWBT_ITEM_KGAIN_RATIO: sprintf(ioc_item_name, "AWBT_ITEM_KGAIN_RATIO"); break;
		case AWBT_ITEM_CT_TO_CGAIN: sprintf(ioc_item_name, "AWBT_ITEM_CT_TO_CGAIN"); break;
		case AWBT_ITEM_WIN: sprintf(ioc_item_name, "AWBT_ITEM_WIN"); break;
		default: sprintf(ioc_item_name, "awbt item (%d)", (int)item); break;
	}

	return ioc_item_name;
}

CHAR *isp_dbg_get_iqt_item(UINT32 item)
{
	switch (item) {
		case IQT_ITEM_VERSION: sprintf(ioc_item_name, "IQT_ITEM_VERSION"); break;
		case IQT_ITEM_SIZE_TAB: sprintf(ioc_item_name, "IQT_ITEM_SIZE_TAB"); break;
		case IQT_ITEM_RLD_CONFIG: sprintf(ioc_item_name, "IQT_ITEM_RLD_CONFIG"); break;
		case IQT_ITEM_RLD_DTSI: sprintf(ioc_item_name, "IQT_ITEM_RLD_DTSI"); break;
		case IQT_ITEM_GAMMA_LV: sprintf(ioc_item_name, "IQT_ITEM_GAMMA_LV"); break;
		case IQT_ITEM_CCM_EXT2_PARAM: sprintf(ioc_item_name, "IQT_ITEM_CCM_EXT2_PARAM"); break;
		case IQT_ITEM_GAMMA_EXT_PARAM: sprintf(ioc_item_name, "IQT_ITEM_GAMMA_EXT_PARAM"); break;
		case IQT_ITEM_EXPAND_DPC_PARAM: sprintf(ioc_item_name, "IQT_ITEM_EXPAND_DPC_PARAM"); break;
		case IQT_ITEM_SHADING_DITH_PARAM: sprintf(ioc_item_name, "IQT_ITEM_SHADING_DITH_PARAM"); break;
		case IQT_ITEM_NR_LV: sprintf(ioc_item_name, "IQT_ITEM_NR_LV"); break;
		case IQT_ITEM_SHARPNESS_LV: sprintf(ioc_item_name, "IQT_ITEM_SHARPNESS_LV"); break;
		case IQT_ITEM_SATURATION_LV: sprintf(ioc_item_name, "IQT_ITEM_SATURATION_LV"); break;
		case IQT_ITEM_CONTRAST_LV: sprintf(ioc_item_name, "IQT_ITEM_CONTRAST_LV"); break;
		case IQT_ITEM_BRIGHTNESS_LV: sprintf(ioc_item_name, "IQT_ITEM_BRIGHTNESS_LV"); break;
		case IQT_ITEM_NIGHT_MODE: sprintf(ioc_item_name, "IQT_ITEM_NIGHT_MODE"); break;
		case IQT_ITEM_YCC_FORMAT: sprintf(ioc_item_name, "IQT_ITEM_YCC_FORMAT"); break;
		case IQT_ITEM_OPERATION: sprintf(ioc_item_name, "IQT_ITEM_OPERATION"); break;
		case IQT_ITEM_IMAGEEFFECT: sprintf(ioc_item_name, "IQT_ITEM_IMAGEEFFECT"); break;
		case IQT_ITEM_CCID: sprintf(ioc_item_name, "IQT_ITEM_CCID"); break;
		case IQT_ITEM_HUE_SHIFT: sprintf(ioc_item_name, "IQT_ITEM_HUE_SHIFT"); break;
		case IQT_ITEM_SHDR_TONE_LV: sprintf(ioc_item_name, "IQT_ITEM_SHDR_TONE_LV"); break;
		case IQT_ITEM_3DNR_LV: sprintf(ioc_item_name, "IQT_ITEM_3DNR_LV"); break;
		case IQT_ITEM_CCM_EXT_PARAM: sprintf(ioc_item_name, "IQT_ITEM_CCM_EXT_PARAM"); break;
		case IQT_ITEM_SHADING_VIG_EXT_PARAM: sprintf(ioc_item_name, "IQT_ITEM_SHADING_VIG_EXT_PARAM"); break;
		case IQT_ITEM_SHDR_EXT_PARAM: sprintf(ioc_item_name, "IQT_ITEM_SHDR_EXT_PARAM"); break;
		case IQT_ITEM_SHADING_INTER_PARAM: sprintf(ioc_item_name, "IQT_ITEM_SHADING_INTER_PARAM"); break;
		case IQT_ITEM_SHADING_EXT_PARAM: sprintf(ioc_item_name, "IQT_ITEM_SHADING_EXT_PARAM"); break;
		case IQT_ITEM_CST_PARAM: sprintf(ioc_item_name, "IQT_ITEM_CST_PARAM"); break;
		case IQT_ITEM_STRIPE_PARAM: sprintf(ioc_item_name, "IQT_ITEM_STRIPE_PARAM"); break;
		case IQT_ITEM_LCA_SUB_RATIO_PARAM: sprintf(ioc_item_name, "IQT_ITEM_LCA_SUB_RATIO_PARAM"); break;
		case IQT_ITEM_OB_PARAM: sprintf(ioc_item_name, "IQT_ITEM_OB_PARAM"); break;
		case IQT_ITEM_NR_PARAM: sprintf(ioc_item_name, "IQT_ITEM_NR_PARAM"); break;
		case IQT_ITEM_CFA_PARAM: sprintf(ioc_item_name, "IQT_ITEM_CFA_PARAM"); break;
		case IQT_ITEM_VA_PARAM: sprintf(ioc_item_name, "IQT_ITEM_VA_PARAM"); break;
		case IQT_ITEM_GAMMA_PARAM: sprintf(ioc_item_name, "IQT_ITEM_GAMMA_PARAM"); break;
		case IQT_ITEM_CCM_PARAM: sprintf(ioc_item_name, "IQT_ITEM_CCM_PARAM"); break;
		case IQT_ITEM_COLOR_PARAM: sprintf(ioc_item_name, "IQT_ITEM_COLOR_PARAM"); break;
		case IQT_ITEM_CONTRAST_PARAM: sprintf(ioc_item_name, "IQT_ITEM_CONTRAST_PARAM"); break;
		case IQT_ITEM_EDGE_PARAM: sprintf(ioc_item_name, "IQT_ITEM_EDGE_PARAM"); break;
		case IQT_ITEM_3DNR_PARAM: sprintf(ioc_item_name, "IQT_ITEM_3DNR_PARAM"); break;
		case IQT_ITEM_DPC_PARAM: sprintf(ioc_item_name, "IQT_ITEM_DPC_PARAM"); break;
		case IQT_ITEM_SHADING_PARAM: sprintf(ioc_item_name, "IQT_ITEM_SHADING_PARAM"); break;
		case IQT_ITEM_LDC_PARAM: sprintf(ioc_item_name, "IQT_ITEM_LDC_PARAM"); break;
		case IQT_ITEM_PFR_PARAM: sprintf(ioc_item_name, "IQT_ITEM_PFR_PARAM"); break;
		case IQT_ITEM_WDR_PARAM: sprintf(ioc_item_name, "IQT_ITEM_WDR_PARAM"); break;
		case IQT_ITEM_DEFOG_PARAM: sprintf(ioc_item_name, "IQT_ITEM_DEFOG_PARAM"); break;
		case IQT_ITEM_SHDR_PARAM: sprintf(ioc_item_name, "IQT_ITEM_SHDR_PARAM"); break;
		case IQT_ITEM_RGBIR_PARAM: sprintf(ioc_item_name, "IQT_ITEM_RGBIR_PARAM"); break;
		case IQT_ITEM_COMPANDING_PARAM: sprintf(ioc_item_name, "IQT_ITEM_COMPANDING_PARAM"); break;
		case IQT_ITEM_EDGE_DBG: sprintf(ioc_item_name, "IQT_ITEM_EDGE_DBG"); break;
		case IQT_ITEM_3DNR_DBG: sprintf(ioc_item_name, "IQT_ITEM_3DNR_DBG"); break;
		case IQT_ITEM_SHDR_MODE: sprintf(ioc_item_name, "IQT_ITEM_SHDR_MODE"); break;
		case IQT_ITEM_3DNR_MISC_PARAM: sprintf(ioc_item_name, "IQT_ITEM_3DNR_MISC_PARAM"); break;
		case IQT_ITEM_POST_3DNR_PARAM: sprintf(ioc_item_name, "IQT_ITEM_POST_3DNR_PARAM"); break;
		case IQT_ITEM_DR_LEVEL: sprintf(ioc_item_name, "IQT_ITEM_DR_LEVEL"); break;
		case IQT_ITEM_RGBIR_ENH_PARAM: sprintf(ioc_item_name, "IQT_ITEM_RGBIR_ENH_PARAM"); break;
		case IQT_ITEM_RGBIR_ENH_ISO: sprintf(ioc_item_name, "IQT_ITEM_RGBIR_ENH_ISO"); break;
		case IQT_ITEM_MD_PARAM: sprintf(ioc_item_name, "IQT_ITEM_MD_PARAM"); break;
		case IQT_ITEM_EDGE_REGION_PARAM: sprintf(ioc_item_name, "IQT_ITEM_EDGE_REGION_PARAM"); break;
		case IQT_ITEM_NNSC_DARK_ENH_RATIO: sprintf(ioc_item_name, "IQT_ITEM_NNSC_DARK_ENH_RATIO"); break;
		case IQT_ITEM_NNSC_CONTRAST_ENH_RATIO: sprintf(ioc_item_name, "IQT_ITEM_NNSC_CONTRAST_ENH_RATIO"); break;
		case IQT_ITEM_NNSC_GREEN_ENH_RATIO: sprintf(ioc_item_name, "IQT_ITEM_NNSC_GREEN_ENH_RATIO"); break;
		case IQT_ITEM_NNSC_SKIN_ENH_RATIO: sprintf(ioc_item_name, "IQT_ITEM_NNSC_SKIN_ENH_RATIO"); break;
		default: sprintf(ioc_item_name, "iqt item (%d)", (int)item); break;
	}

	return ioc_item_name;
}

