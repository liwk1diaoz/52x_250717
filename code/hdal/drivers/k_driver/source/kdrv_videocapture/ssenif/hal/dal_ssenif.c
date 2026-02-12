/*
    Serial Sensor Interface DAL library

    Exported Serial Sensor Interface DAL library.

    @file       dal_ssenif.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "ssenif_int.h"

#ifndef __KERNEL__
#define __MODULE__    rtos_ssenif
#include <kwrap/debug.h>
unsigned int rtos_ssenif_debug_level = NVT_DBG_WRN;
#endif

BOOL            ssenif_list_name_dump = 1;
DAL_SSENIF_ID   ssenif_lastest_start  = DAL_SSENIF_ID_MAX;

static DAL_SSENIF_ID    ssenif_lastest_get    = DAL_SSENIF_ID_MAX;

#if 1

static void ssenif_set_config_csi_noimplement(DAL_SSENIFCSI_CFGID config_id, UINT32 value)
{
	DBG_WRN("Current Object no implement this context\r\n");
}
static UINT32 ssenif_get_config_csi_noimplement(DAL_SSENIFCSI_CFGID config_id)
{
	DBG_WRN("Current Object no implement this context\r\n");
	return 0;
}
#if SSENIF_CSI_EN
static void ssenif_set_config_lvds_noimplement(DAL_SSENIFLVDS_CFGID config_id, UINT32 value)
{
	DBG_WRN("Current Object no implement this context\r\n");
}
static UINT32 ssenif_get_config_lvds_noimplement(DAL_SSENIFLVDS_CFGID config_id)
{
	DBG_WRN("Current Object no implement this context\r\n");
	return 0;
}
static void ssenif_set_laneconfig_lvds_noimplement(DAL_SSENIFLVDS_LANECFGID config_id, DAL_SSENIF_LANESEL lane_select, UINT32 value)
{
	DBG_WRN("Current Object no implement this context\r\n");
}
#endif
static void ssenif_set_config_slvsec_noimplement(DAL_SSENIFSLVSEC_CFGID config_id, UINT32 value)
{
	DBG_WRN("Current Object no implement this context\r\n");
}
static UINT32 ssenif_get_config_slvsec_noimplement(DAL_SSENIFSLVSEC_CFGID config_id)
{
	DBG_WRN("Current Object no implement this context\r\n");
	return 0;
}
static void ssenif_set_config_vx1_noimplement(DAL_SSENIFVX1_CFGID config_id, UINT32 value)
{
	DBG_WRN("Current Object no implement this context\r\n");
}
static UINT32 ssenif_get_config_vx1_noimplement(DAL_SSENIFVX1_CFGID config_id)
{
	DBG_WRN("Current Object no implement this context\r\n");
	return 0;
}
static DAL_SSENIFVX1_I2CSTS ssenif_sensor_i2c_write_noimplement(UINT32 reg_address, UINT32 reg_data)
{
	DBG_WRN("Current Object no implement this context\r\n");
	return DAL_SSENIFVX1_I2CSTS_ERROR;
}
static DAL_SSENIFVX1_I2CSTS ssenif_sensor_i2c_read_noimplement(UINT32 reg_address, UINT32 *reg_data)
{
	DBG_WRN("Current Object no implement this context\r\n");
	return DAL_SSENIFVX1_I2CSTS_ERROR;
}
static DAL_SSENIFVX1_I2CSTS ssenif_sensor_i2c_seq_write_noimplement(UINT32 start_address, UINT32 total_data_length, UINT32 data_buffer_address)
{
	DBG_WRN("Current Object no implement this context\r\n");
	return DAL_SSENIFVX1_I2CSTS_ERROR;
}
static void ssenif_set_vx1_gpio_noimplement(DAL_SSENIFVX1_GPIO pin, BOOL level)
{
	DBG_WRN("Current Object no implement this context\r\n");
}
static BOOL ssenif_get_vx1_gpio_noimplement(DAL_SSENIFVX1_GPIO pin)
{
	DBG_WRN("Current Object no implement this context\r\n");
	return 0;
}

#endif

static DAL_SSENIFOBJ dal_ssenif_object[DAL_SSENIF_ID_MAX] = {
#if SSENIF_CSI_EN
	{ DAL_SSENIF_ID_CSI,   "SSENIF_ID_CSI",   ssenif_init_csi,   ssenif_open_csi,   ssenif_is_opened_csi,   ssenif_close_csi,   ssenif_start_csi,   ssenif_stop_csi,   ssenif_wait_interrupt_csi,   ssenif_dump_debug_information_csi,  ssenif_set_config_csi,              ssenif_get_config_csi,                  ssenif_set_config_lvds_noimplement, ssenif_get_config_lvds_noimplement, ssenif_set_laneconfig_lvds_noimplement, ssenif_set_config_slvsec_noimplement, ssenif_get_config_slvsec_noimplement, ssenif_set_config_vx1_noimplement, ssenif_get_config_vx1_noimplement, ssenif_sensor_i2c_write_noimplement, ssenif_sensor_i2c_read_noimplement, ssenif_sensor_i2c_seq_write_noimplement, ssenif_set_vx1_gpio_noimplement, ssenif_get_vx1_gpio_noimplement},
	{ DAL_SSENIF_ID_CSI2,  "SSENIF_ID_CSI2",  ssenif_init_csi2,  ssenif_open_csi2,  ssenif_is_opened_csi2,  ssenif_close_csi2,  ssenif_start_csi2,  ssenif_stop_csi2,  ssenif_wait_interrupt_csi2,  ssenif_dump_debug_information_csi2, ssenif_set_config_csi2,             ssenif_get_config_csi2,                 ssenif_set_config_lvds_noimplement, ssenif_get_config_lvds_noimplement, ssenif_set_laneconfig_lvds_noimplement, ssenif_set_config_slvsec_noimplement, ssenif_get_config_slvsec_noimplement, ssenif_set_config_vx1_noimplement, ssenif_get_config_vx1_noimplement, ssenif_sensor_i2c_write_noimplement, ssenif_sensor_i2c_read_noimplement, ssenif_sensor_i2c_seq_write_noimplement, ssenif_set_vx1_gpio_noimplement, ssenif_get_vx1_gpio_noimplement},
#endif
	{ DAL_SSENIF_ID_LVDS,  "SSENIF_ID_LVDS",  ssenif_init_lvds,  ssenif_open_lvds,  ssenif_is_opened_lvds,  ssenif_close_lvds,  ssenif_start_lvds,  ssenif_stop_lvds,  ssenif_wait_interrupt_lvds,  ssenif_dump_debug_information_lvds,  ssenif_set_config_csi_noimplement, ssenif_get_config_csi_noimplement,  ssenif_set_config_lvds,             ssenif_get_config_lvds,             ssenif_set_laneconfig_lvds,             ssenif_set_config_slvsec_noimplement, ssenif_get_config_slvsec_noimplement, ssenif_set_config_vx1_noimplement, ssenif_get_config_vx1_noimplement, ssenif_sensor_i2c_write_noimplement, ssenif_sensor_i2c_read_noimplement, ssenif_sensor_i2c_seq_write_noimplement, ssenif_set_vx1_gpio_noimplement, ssenif_get_vx1_gpio_noimplement},
	{ DAL_SSENIF_ID_LVDS2, "SSENIF_ID_LVDS2", ssenif_init_lvds2, ssenif_open_lvds2, ssenif_is_opened_lvds2, ssenif_close_lvds2, ssenif_start_lvds2, ssenif_stop_lvds2, ssenif_wait_interrupt_lvds2, ssenif_dump_debug_information_lvds2, ssenif_set_config_csi_noimplement, ssenif_get_config_csi_noimplement,  ssenif_set_config_lvds2,            ssenif_get_config_lvds2,            ssenif_set_laneconfig_lvds2,            ssenif_set_config_slvsec_noimplement, ssenif_get_config_slvsec_noimplement, ssenif_set_config_vx1_noimplement, ssenif_get_config_vx1_noimplement, ssenif_sensor_i2c_write_noimplement, ssenif_sensor_i2c_read_noimplement, ssenif_sensor_i2c_seq_write_noimplement, ssenif_set_vx1_gpio_noimplement, ssenif_get_vx1_gpio_noimplement}
};

/**
    Get the Serial Sensor Interface DAL control object
*/
PDAL_SSENIFOBJ dal_ssenif_get_object(DAL_SSENIF_ID serial_senif_id)
{
	if (serial_senif_id >= DAL_SSENIF_ID_MAX) {
		DBG_ERR("No such SSENIF module.%d\r\n", serial_senif_id);
		return NULL;
	}
	ssenif_lastest_get = serial_senif_id;

	return &dal_ssenif_object[serial_senif_id];
}
#ifdef __KERNEL__
EXPORT_SYMBOL(dal_ssenif_get_object);
#endif

#if 1
#endif

#if 0 //ndef __KERNEL__
static BOOL ssenif_debug_dump(CHAR *str_cmd)
{
	loc_cpu();
#if SSENIF_CSI_EN
	ssenif_dump_debug_information_csi();
	ssenif_list_name_dump = 0;
	ssenif_dump_debug_information_csi2();
	ssenif_list_name_dump = 1;
#endif
	ssenif_dump_debug_information_lvds();
	ssenif_list_name_dump = 0;
	ssenif_dump_debug_information_lvds2();

	ssenif_list_name_dump = 1;
	unl_cpu();
	return TRUE;
}

static BOOL ssenif_dump_get(CHAR *str_cmd)
{
	if (ssenif_lastest_get != DAL_SSENIF_ID_MAX) {
		dal_ssenif_object[ssenif_lastest_get].dump_debug_information();
	}
	return TRUE;
}

static BOOL ssenif_dump_start(CHAR *str_cmd)
{
	if (ssenif_lastest_start != DAL_SSENIF_ID_MAX) {
		dal_ssenif_object[ssenif_lastest_start].dump_debug_information();
	}
	return TRUE;
}

void ssenif_install_cmd(void)
{
	static BOOL install = FALSE;

	static SXCMD_BEGIN(ssenif, "dump ssenif debug info")

	SXCMD_ITEM("get",   ssenif_dump_get,    "lastest get obj dump")
	SXCMD_ITEM("start", ssenif_dump_start,  "lastest start obj dump")
	SXCMD_ITEM("all",   ssenif_debug_dump,  "dump all")
	SXCMD_END()

	if (install == FALSE) {
		SX_CMD_ADDTABLE(ssenif);
		install = TRUE;
	}
}
#else
BOOL ssenif_debug_dump(CHAR *str_cmd)
{
	DAL_SSENIF_ID   ssenif_id;
	PDAL_SSENIFOBJ  PSEN1DALOBJ;
#if SSENIF_CSI_EN
	for (ssenif_id = DAL_SSENIF_ID_CSI; ssenif_id < DAL_SSENIF_ID_MAX; ssenif_id++) {
#else
	for (ssenif_id = DAL_SSENIF_ID_LVDS; ssenif_id < DAL_SSENIF_ID_MAX; ssenif_id++) {
#endif
		PSEN1DALOBJ = dal_ssenif_get_object(ssenif_id);
		PSEN1DALOBJ->init();
	}

	ssenif_dump_debug_information_csi();
	ssenif_list_name_dump = 0;
	ssenif_dump_debug_information_csi2();
	ssenif_list_name_dump = 1;
	ssenif_dump_debug_information_lvds();
	ssenif_list_name_dump = 0;
	ssenif_dump_debug_information_lvds2();
	ssenif_list_name_dump = 1;

	return TRUE;
}
#ifdef __KERNEL__
EXPORT_SYMBOL(ssenif_debug_dump);
#endif

void ssenif_install_cmd(void)
{}
#endif


