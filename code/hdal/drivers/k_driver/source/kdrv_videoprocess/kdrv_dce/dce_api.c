#include "dce_platform.h"
#include "dce_api.h"
#include "dce_kit.h"
#include "dce_lib.h"
#include "kdrv_videoprocess/kdrv_ipp.h"


int nvt_dce_api_write_reg(PDCE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		DBG_ERR("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		DBG_ERR("invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul(pargv[1], 0, &reg_value)) {
		DBG_ERR("invalid rag value:%s\n", pargv[1]);
		return -EINVAL;

	}

	DBG_MSG("W REG 0x%lx to 0x%lx\n", reg_value, reg_addr);

	nvt_dce_drv_write_reg(pmodule_info, reg_addr, reg_value);
	return 0;
}

int nvt_dce_api_write_pattern(PDCE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	return 0;
}

int nvt_dce_api_read_reg(PDCE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0;
	unsigned long value = 0;

	if (argc != 1) {
		DBG_ERR("wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		DBG_ERR("invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	DBG_MSG("R REG 0x%lx\n", reg_addr);
	value = nvt_dce_drv_read_reg(pmodule_info, reg_addr);

	DBG_ERR("REG 0x%lx = 0x%lx\n", reg_addr, value);
	return 0;
}

int nvt_dce_api_dump(PDCE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	DBG_DUMP("Dump kdriver\r\n");
	kdrv_api_dce_dump_info(argc, pargv);
	return 0;
}

int nvt_dce_show_stripe_settings(PDCE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	KDRV_DCE_STRIPE_INFO tmp_stripe = {0};
	INT32 i;
	unsigned int channel;

	if (kstrtoint(pargv[0], 0, &channel)) {
		DBG_ERR("Fail to transform string %s to unsigned int from parameter!\r\n", pargv[0]);
		return 1;
	}

	kdrv_dce_get(channel, KDRV_DCE_PARAM_IPL_STRIPE_INFO, (void *)&tmp_stripe);

	for (i = 0; i < (INT32)tmp_stripe.hstp_num; i ++) {
		DBG_DUMP("dce out stripe %u, ime out stripe %u\r\n", (unsigned int)tmp_stripe.hstp[i], (unsigned int)tmp_stripe.ime_out_hstp[i]);
	}

	return 0;
}

#if !defined(CONFIG_NVT_SMALL_HDAL)
int nvt_dce_api_control_api_log(PDCE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	INT32 i;

	for (i = 0; i < KDRV_DCE_LOG_NUM; i++) {
		DBG_DUMP("[%03d] %s\r\n", i, g_kdrv_dce_ctrl_api_log[i]);
	}

	return 0;
}

int nvt_dce_api_test_kdrv_run_d2d(PDCE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	return kdrv_api_dce_test(argc, pargv);
}

int nvt_dce_api_hw_power_saving(PDCE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int enable = 1;

	if (argc > 0) {
		if (kstrtoint(pargv[0], 0, &enable)) {
			DBG_ERR("Fail to transform string %s to unsigned int from parameter!\r\n", pargv[0]);
			return 1;
		}
	} else {
		DBG_WRN("User should pass 1 parameter!\r\n");
		return 1;
	}

	if (enable == 1) {
		clk_set_phase(pmodule_info->pclk[0], 1);
		clk_set_phase(pmodule_info->p_pclk[0], 1);
	} else {
		clk_set_phase(pmodule_info->pclk[0], 0);
		clk_set_phase(pmodule_info->p_pclk[0], 0);
	}

	DBG_IND("end\r\n");

	return 0;
}

int nvt_dce_api_fw_power_saving(PDCE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned int enable = 1;

	if (argc > 0) {
		if (kstrtoint(pargv[0], 0, &enable)) {
			DBG_ERR("Fail to transform string %s to unsigned int from parameter!\r\n", pargv[0]);
			return 1;
		}
	} else {
		DBG_WRN("User should pass 1 parameter!\r\n");
		return 1;
	}

	if (enable == 1) {
		fw_power_saving_en = TRUE;
	} else {
		fw_power_saving_en = FALSE;
	}

	DBG_IND("end\r\n");

	return 0;
}
#endif
