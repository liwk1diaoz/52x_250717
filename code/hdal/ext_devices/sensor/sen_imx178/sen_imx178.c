#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#else
#include "plat/gpio.h"
#endif
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/util.h"
#include <kwrap/verinfo.h>
#include "kflow_videocapture/ctl_sen.h"
#include "isp_api.h"

#include "sen_cfg.h"
#include "sen_common.h"
#include "sen_inc.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#if defined(__KERNEL__)
char *sen_cfg_path = "null";
module_param_named(sen_cfg_path, sen_cfg_path, charp, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sen_cfg_path, "Path of cfg file");

#ifdef DEBUG
unsigned int sen_debug_level = THIS_DBGLVL;
module_param_named(sen_debug_level, sen_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(sen_debug_level, "Debug message level");
#endif
#endif

//=============================================================================
// version
//=============================================================================
VOS_MODULE_VERSION(nvt_sen_imx178, 1, 43, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_IMX178_MODULE_NAME     "sen_imx178"
#define SEN_MAX_MODE               1
#define MAX_VD_PERIOD              0x3FFFF
#define MIN_EXPOSURE_LINE          1
#define NON_EXP_LINE               8

#define SEN_I2C_ADDR 0x34>>1
#define SEN_I2C_COMPATIBLE "nvt,sen_imx178"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_imx178(void);
static void sen_pwr_ctrl_imx178(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_imx178(CTL_SEN_ID id);
static ER sen_close_imx178(CTL_SEN_ID id);
static ER sen_sleep_imx178(CTL_SEN_ID id);
static ER sen_wakeup_imx178(CTL_SEN_ID id);
static ER sen_write_reg_imx178(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_imx178(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_imx178(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_imx178(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_imx178(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_imx178(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_imx178(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_imx178(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_imx178(CTL_SEN_ID id, void *param);
static void sen_set_expt_imx178(CTL_SEN_ID id, void *param);
static void sen_set_preset_imx178(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_imx178(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_imx178(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
// TODO:
#if 0
static void sen_set_status_imx178(CTL_SEN_ID id, CTL_SEN_STATUS *status);
#endif
static void sen_get_gain_imx178(CTL_SEN_ID id, void *param);
static void sen_get_expt_imx178(CTL_SEN_ID id, void *param);
static void sen_get_min_expt_imx178(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_imx178(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_imx178(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_imx178(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_imx178(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_imx178(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_imx178(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_imx178(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_lvds_imx178(CTL_SENDRV_GET_MODE_LVDS_PARAM *data);
static void sen_get_modesel_imx178(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_imx178(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_imx178(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_imx178(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_imx178(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_imx178(CTL_SEN_ID id);
static void sen_set_chgmode_fps_imx178(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_imx178(CTL_SEN_ID id);

//=============================================================================
// global variable
//=============================================================================
static UINT32 sen_map = SEN_PATH_1;

static SEN_PRESET sen_preset[CTL_SEN_ID_MAX] = {
	{10000, 1000},
	{10000, 1000},
	{10000, 1000},
	{10000, 1000},
	{10000, 1000},
	{10000, 1000},
	{10000, 1000},
	{10000, 1000}
};

static SEN_DIRECTION sen_direction[CTL_SEN_ID_MAX] = {
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE},
	{FALSE, FALSE}
};

static SEN_POWER sen_power[CTL_SEN_ID_MAX] = {
	//C_GPIO:+0x0; P_GPIO:+0x20; S_GPIO:+0x40; L_GPIO:0x60
	#if defined(CONFIG_NVT_FPGA_EMULATION) || defined(_NVT_FPGA_)
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x23, 1, 1},
	#else
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	#endif
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1}
};

static SEN_I2C sen_i2c[CTL_SEN_ID_MAX] = {
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR},
	{SEN_I2C_ID_2, SEN_I2C_ADDR}
};

static CTL_SENDRV_GET_ATTR_BASIC_PARAM basic_param = {
	SEN_IMX178_MODULE_NAME,
	CTL_SEN_VENDOR_SONY,
	SEN_MAX_MODE,
	CTL_SEN_SUPPORT_PROPERTY_MIRROR|CTL_SEN_SUPPORT_PROPERTY_FLIP|CTL_SEN_SUPPORT_PROPERTY_CHGFPS,
	1
};

static CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_param = {
	CTL_SEN_SIGNAL_MASTER,
	{CTL_SEN_ACTIVE_HIGH, CTL_SEN_ACTIVE_HIGH, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING}
};

static CTL_SENDRV_I2C i2c = {
	{
		{CTL_SEN_I2C_W_ADDR_DFT,     0x34},
		{CTL_SEN_I2C_W_ADDR_OPTION1, 0xFF},
		{CTL_SEN_I2C_W_ADDR_OPTION2, 0xFF},
		{CTL_SEN_I2C_W_ADDR_OPTION3, 0xFF},
		{CTL_SEN_I2C_W_ADDR_OPTION4, 0xFF},
		{CTL_SEN_I2C_W_ADDR_OPTION5, 0xFF}
	}
};

static CTL_SENDRV_GET_SPEED_PARAM speed_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_SIEMCLK_SRC_DFT,
		27000000,
		74250000,
		99000000,
	}
};

static CTL_SENDRV_GET_MODE_LVDS_PARAM lvds_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ 1, 2, 0, 3},
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE,
		0,
		CTL_SEN_DATAIN_BIT_ORDER_LSB,
		0,
	}
};
static CTL_SENDRV_GET_MODE_BASIC_PARAM mode_basic_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_IF_TYPE_LVDS,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{3096, 2094},
		{{12, 31, 3072, 2048}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{3072, 2048},
		{0, 1001, 0, 2475},
		CTL_SEN_RATIO(3, 2),
		{1000, 251000},
		100
	}
};

static CTL_SEN_CMD imx178_mode_1[] = {
	// MCLK=27Mhz LVDS 4lane 3072x2048 p30 10bit
	{0x3000, 1, {0x01, 0x0}},// standby
	{SEN_CMD_DELAY, 1, { 20, 0x0}},
	{0x3008, 1, {0x01, 0x0}},
	{SEN_CMD_DELAY, 1, {20, 0x0}},
    {0x300E, 1, {0x00, 0x0}},
    {0x300F, 1, {0x00, 0x0}},
    {0x3010, 1, {0x00, 0x0}},
    {0x3066, 1, {0x06, 0x0}},
    {0x302C, 1, {0xAB, 0x0}},
    {0x302D, 1, {0x09, 0x0}},
    {0x302E, 1, {0x00, 0x0}},
    {0x302F, 1, {0xE9, 0x0}},
    {0x3030, 1, {0x03, 0x0}},
    {0x300D, 1, {0x00, 0x0}},
    {0x3059, 1, {0x30, 0x0}},
    {0x3004, 1, {0x03, 0x0}},
    {0x3101, 1, {0x30, 0x0}},
    //
    {0x310C, 1, {0x00, 0x0}},
    {0x33BE, 1, {0x21, 0x0}},
    {0x34BF, 1, {0x21, 0x0}},
    {0x33C0, 1, {0x2C, 0x0}},
    {0x33C1, 1, {0x2C, 0x0}},
    {0x33C2, 1, {0x21, 0x0}},
    {0x33C3, 1, {0x2C, 0x0}},
    {0x33C4, 1, {0x2C, 0x0}},
    {0x33C5, 1, {0x00, 0x0}},
    {0x311C, 1, {0x34, 0x0}},
    {0x311D, 1, {0x28, 0x0}},
    {0x311E, 1, {0xAB, 0x0}},
    {0x311F, 1, {0x00, 0x0}},
    {0x3120, 1, {0x95, 0x0}},
    {0x3121, 1, {0x00, 0x0}},
    {0x3122, 1, {0xB4, 0x0}},
    {0x3123, 1, {0x00, 0x0}},
    {0x3124, 1, {0x8C, 0x0}},
    {0x3125, 1, {0x02, 0x0}},
    {0x312D, 1, {0x03, 0x0}},
    {0x312E, 1, {0x0C, 0x0}},
    {0x312F, 1, {0x28, 0x0}},
    {0x3131, 1, {0x2D, 0x0}},
    {0x3132, 1, {0x00, 0x0}},
    {0x3133, 1, {0xB4, 0x0}},
    {0x3134, 1, {0x00, 0x0}},
    {0x3137, 1, {0x50, 0x0}},
    {0x3138, 1, {0x08, 0x0}},
    {0x3139, 1, {0x00, 0x0}},
    {0x313A, 1, {0x07, 0x0}},
    {0x313D, 1, {0x05, 0x0}},
    {0x3140, 1, {0x06, 0x0}},
    {0x3220, 1, {0x8B, 0x0}},
    {0x3221, 1, {0x00, 0x0}},
    {0x3222, 1, {0x74, 0x0}},
    {0x3223, 1, {0x00, 0x0}},
    {0x3226, 1, {0xC2, 0x0}},
    {0x3227, 1, {0x00, 0x0}},
    {0x32A9, 1, {0x1B, 0x0}},
    {0x32AA, 1, {0x00, 0x0}},
    {0x32B3, 1, {0x0E, 0x0}},
    {0x32B4, 1, {0x00, 0x0}},
    {0x33D6, 1, {0x16, 0x0}},
    {0x33D7, 1, {0x15, 0x0}},
    {0x33D8, 1, {0x14, 0x0}},
    {0x33D9, 1, {0x10, 0x0}},
    {0x33DA, 1, {0x08, 0x0}},
    //
    {0x3011, 1, {0x00, 0x0}},
    {0x3037, 1, {0x08, 0x0}},
    {0x3038, 1, {0x00, 0x0}},
    {0x3039, 1, {0x00, 0x0}},
    {0x30AD, 1, {0x49, 0x0}},
    {0x30AF, 1, {0x54, 0x0}},
    {0x30B0, 1, {0x33, 0x0}},
    {0x30B3, 1, {0x0A, 0x0}},
    {0x30C4, 1, {0x30, 0x0}},
    {0x3103, 1, {0x03, 0x0}},
    {0x3104, 1, {0x08, 0x0}},
    {0x3107, 1, {0x10, 0x0}},
    {0x310F, 1, {0x01, 0x0}},
    {0x32E5, 1, {0x06, 0x0}},
    {0x32E6, 1, {0x00, 0x0}},
    {0x32E7, 1, {0x1F, 0x0}},
    {0x32E8, 1, {0x00, 0x0}},
    {0x32E9, 1, {0x00, 0x0}},
    {0x32EA, 1, {0x00, 0x0}},
    {0x32EB, 1, {0x00, 0x0}},
    {0x32EC, 1, {0x00, 0x0}},
    {0x32EE, 1, {0x00, 0x0}},
    {0x32F2, 1, {0x00, 0x0}},
    {0x32F4, 1, {0x00, 0x0}},
    {0x32F5, 1, {0x00, 0x0}},
    {0x32F6, 1, {0x00, 0x0}},
    {0x32F7, 1, {0x00, 0x0}},
    {0x32F8, 1, {0x00, 0x0}},
    {0x32FC, 1, {0x02, 0x0}},
    {0x3310, 1, {0x11, 0x0}},
    {0x3338, 1, {0x81, 0x0}},
    {0x333D, 1, {0x00, 0x0}},
    {0x3362, 1, {0x00, 0x0}},
    {0x336B, 1, {0x02, 0x0}},
    {0x336E, 1, {0x11, 0x0}},
    {0x33B4, 1, {0xFE, 0x0}},
    {0x33B5, 1, {0x06, 0x0}},
    {0x33B9, 1, {0x00, 0x0}},
	////////test
	#if 0
	{0x301F, 1, {0xE0, 0x0}},
	{0x3020, 1, {0x01, 0x0}},
	{0x3034, 1, {0xFF, 0x0}},
    {0x3035, 1, {0x04, 0x0}},
    {0x3036, 1, {0x00, 0x0}},
    {0x302C, 1, {0xAB, 0x0}},
    {0x302D, 1, {0x09, 0x0}},
    {0x302E, 1, {0x00, 0x0}},
	{0x302F, 1, {0xE9, 0x0}},
    {0x3030, 1, {0x03, 0x0}},
	#endif
	//////
	{SEN_CMD_SETVD, 1, {0x00, 0x00}},
	{SEN_CMD_PRESET, 1, {0x00, 0x00} },
	{SEN_CMD_DIRECTION, 1, {0x00, 0x00} },
	//
    {0x3000, 1, {0x06, 0x0}},
    //
    {SEN_CMD_DELAY, 1, {20, 0x0}},
	{0x3000, 1, {0x00, 0x0}},
	{SEN_CMD_DELAY, 1, {20, 0x0}},
	//
    {SEN_CMD_DELAY, 1, { 20, 0x0}},
	{0x3008, 1, {0x00, 0x0}},
};

static UINT32 cur_sen_mode[CTL_SEN_ID_MAX] = {CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1};
static UINT32 cur_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 chgmode_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 power_ctrl_mclk[CTL_SEN_CLK_SEL_MAX] = {0};
static UINT32 reset_ctrl_count[CTL_SEN_ID_MAX] = {0};
static UINT32 pwdn_ctrl_count[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_CTRL sensor_ctrl_last[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_PRESET_CTRL preset_ctrl[CTL_SEN_ID_MAX] = {0};
static BOOL i2c_valid[CTL_SEN_ID_MAX];

static CTL_SEN_DRV_TAB imx178_sen_drv_tab = {
	sen_open_imx178,
	sen_close_imx178,
	sen_sleep_imx178,
	sen_wakeup_imx178,
	sen_write_reg_imx178,
	sen_read_reg_imx178,
	sen_chg_mode_imx178,
	sen_chg_fps_imx178,
	sen_set_info_imx178,
	sen_get_info_imx178,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_imx178(void)
{
	return &imx178_sen_drv_tab;
}

static void sen_pwr_ctrl_imx178(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
{
	UINT32 i = 0;
	UINT32 reset_count = 0, pwdn_count = 0;
	DBG_IND("enter flag %d \r\n", flag);

	if (flag == CTL_SEN_PWR_CTRL_TURN_ON) {
		if (sen_power[id].pwdn_pin != CTL_SEN_IGNORE) {
			for ( i = 0; i < CTL_SEN_ID_MAX ; i++ ) {
				if ( pwdn_ctrl_count[i] == (sen_power[id].pwdn_pin)) {
					pwdn_count++;
				}
			}
			pwdn_ctrl_count[id] = (sen_power[id].pwdn_pin);

			if (!pwdn_count) {
				gpio_direction_output((sen_power[id].pwdn_pin), 0);
				gpio_set_value((sen_power[id].pwdn_pin), 0);
				gpio_set_value((sen_power[id].pwdn_pin), 1);
			}
		}

		if (clk_cb != NULL) {
			if (sen_power[id].mclk != CTL_SEN_IGNORE) {
				if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK ) {
					power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK] += 1;
				} else if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK2) {
					power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK2] += 1;
				} else { //CTL_SEN_CLK_SEL_SIEMCLK3
					power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK3] += 1;
				}
				if (1 == (power_ctrl_mclk[sen_power[id].mclk])) {
					clk_cb(sen_power[id].mclk, TRUE);
				}
			}
		}

		if (sen_power[id].rst_pin != CTL_SEN_IGNORE) {
			vos_util_delay_ms(sen_power[id].stable_time);
			for ( i = 0; i < CTL_SEN_ID_MAX ; i++ ) {
				if ( reset_ctrl_count[i] == (sen_power[id].rst_pin)) {
					reset_count++;
				}
			}
			reset_ctrl_count[id] = (sen_power[id].rst_pin);

			if (!reset_count) {
				gpio_direction_output((sen_power[id].rst_pin), 0);
				gpio_set_value((sen_power[id].rst_pin), 0);
				vos_util_delay_ms(sen_power[id].rst_time);
				gpio_set_value((sen_power[id].rst_pin), 1);
				vos_util_delay_ms(sen_power[id].stable_time);
			}
		}
	}

	if (flag == CTL_SEN_PWR_CTRL_TURN_OFF) {

		if (sen_power[id].pwdn_pin != CTL_SEN_IGNORE) {
			pwdn_ctrl_count[id] = 0;

			for ( i = 0; i < CTL_SEN_ID_MAX ; i++ ) {
				if ( pwdn_ctrl_count[i] == (sen_power[id].pwdn_pin)) {
					pwdn_count++;
				}
			}

			if (!pwdn_count) {
				gpio_direction_output((sen_power[id].pwdn_pin), 0);
				gpio_set_value((sen_power[id].pwdn_pin), 0);
			}
		}

		if (sen_power[id].rst_pin != CTL_SEN_IGNORE) {
			reset_ctrl_count[id] = 0;

			for ( i = 0; i < CTL_SEN_ID_MAX ; i++ ) {
				if ( reset_ctrl_count[i] == (sen_power[id].rst_pin)) {
					reset_count++;
				}
			}

			if (!reset_count) {
				gpio_direction_output((sen_power[id].rst_pin), 0);
				gpio_set_value((sen_power[id].rst_pin), 0);
				vos_util_delay_ms(sen_power[id].stable_time);
			}
		}

		if (clk_cb != NULL) {
			if (sen_power[id].mclk != CTL_SEN_IGNORE) {			
				if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK ) {
					power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK] -= 1;
				} else if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK2) {
					power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK2] -= 1;
				} else { //CTL_SEN_CLK_SEL_SIEMCLK3
					power_ctrl_mclk[CTL_SEN_CLK_SEL_SIEMCLK3] -= 1;
				}
				if (!power_ctrl_mclk[sen_power[id].mclk]) {
					clk_cb(sen_power[id].mclk, FALSE);
				}
			}
		}
	}
}

static CTL_SEN_CMD sen_set_cmd_info_imx178(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_imx178(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_imx178(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_imx178);
	#endif

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_DEFAULT;

	rt = sen_i2c_init_driver(id, &sen_i2c[id]);

	if (rt != E_OK) {
		i2c_valid[id] = FALSE;

		DBG_ERR("init. i2c driver fail (%d) \r\n", id);
	} else {
		i2c_valid[id] = TRUE;
	}

	return rt;
}

static ER sen_close_imx178(CTL_SEN_ID id)
{
	if (i2c_valid[id]) {
		sen_i2c_remove_driver(id);
		i2c_valid[id] = FALSE;
	}

	return E_OK;
}

static ER sen_sleep_imx178(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_imx178(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_imx178(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	struct i2c_msg msgs;
	unsigned char buf[3];
	int i;

	if (!i2c_valid[id]) {
		return E_NOSPT;
	}

	buf[0]     = (cmd->addr >> 8) & 0xFF;
	buf[1]     = cmd->addr & 0xFF;
	buf[2]     = cmd->data[0] & 0xFF;
	msgs.addr  = sen_i2c[id].addr;
	msgs.flags = 0;
	msgs.len   = 3;
	msgs.buf   = buf;

	i = 0;
	while(1){
		if (sen_i2c_transfer(id, &msgs, 1) == 0)
			break;
		i++;
		if (i == 5)
			return E_SYS;
	}

	return E_OK;
}

static ER sen_read_reg_imx178(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	struct i2c_msg  msgs[2];
	unsigned char   tmp[2], tmp2[2];
	int i;

	if (!i2c_valid[id]) {
		return E_NOSPT;
	}

	tmp[0]        = (cmd->addr >> 8) & 0xFF;
	tmp[1]        = cmd->addr & 0xFF;
	msgs[0].addr  = sen_i2c[id].addr;
	msgs[0].flags = 0;
	msgs[0].len   = 2;
	msgs[0].buf   = tmp;

	tmp2[0]       = 0;
	msgs[1].addr  = sen_i2c[id].addr;
	msgs[1].flags = 1;
	msgs[1].len   = 1;
	msgs[1].buf   = tmp2;

	i = 0;
	while(1){
		if (sen_i2c_transfer(id, msgs, 2) == 0)
			break;
		i++;
		if (i == 5)
			return E_SYS;
	}

	cmd->data[0] = tmp2[0];

	return E_OK;
}

static UINT32 sen_get_cmd_tab_imx178(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = imx178_mode_1;
		return sizeof(imx178_mode_1) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_imx178(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{
	ISP_SENSOR_CTRL sensor_ctrl = {0};
	CTL_SEN_CMD *p_cmd_list = NULL, cmd;
	CTL_SEN_FLIP flip = CTL_SEN_FLIP_NONE;
	UINT32 sensor_vd;
	UINT32 idx, cmd_num = 0;
	ER rt = E_OK;

	cur_sen_mode[id] = chgmode_obj.mode;

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_imx178(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_imx178(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_imx178(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_imx178(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_imx178(0x302C, 1, sensor_vd & 0xFF, 0);
			rt |= sen_write_reg_imx178(id, &cmd);
			cmd = sen_set_cmd_info_imx178(0x302D, 1, (sensor_vd >> 8) & 0xFF, 0);
			rt |= sen_write_reg_imx178(id, &cmd);
			cmd = sen_set_cmd_info_imx178(0x302E, 1, (sensor_vd >> 16 ) & 0x01, 0);
			rt |= sen_write_reg_imx178(id, &cmd);
		} else if (p_cmd_list[idx].addr == SEN_CMD_PRESET) {
			switch (preset_ctrl[id].mode) {
				default:
				case ISP_SENSOR_PRESET_DEFAULT:
					sensor_ctrl.gain_ratio[0] = sen_preset[id].gain_ratio;
					sensor_ctrl.exp_time[0] = sen_preset[id].expt_time;
					if (mode_basic_param[cur_sen_mode[id]].frame_num == 2) {
						sensor_ctrl.exp_time[1] = sen_preset[id].expt_time >> 3;
					}
					break;

				case ISP_SENSOR_PRESET_CHGMODE:
					memcpy(&sensor_ctrl, &sensor_ctrl_last[id], sizeof(ISP_SENSOR_CTRL));
					break;

				case ISP_SENSOR_PRESET_AE:
					sensor_ctrl.exp_time[0] = preset_ctrl[id].exp_time[0];
					sensor_ctrl.exp_time[1] = preset_ctrl[id].exp_time[1];
					sensor_ctrl.gain_ratio[0] = preset_ctrl[id].gain_ratio[0];
					sensor_ctrl.gain_ratio[1] = preset_ctrl[id].gain_ratio[1];
				break;
			}

			sen_set_gain_imx178(id, &sensor_ctrl);
			sen_set_expt_imx178(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_imx178(id, &flip);
		} else {
			cmd = sen_set_cmd_info_imx178(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_imx178(id, &cmd);
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_imx178(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_imx178(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_imx178(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_imx178(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	cmd = sen_set_cmd_info_imx178(0x302C, 1, sensor_vd & 0xFF, 0);
	rt |= sen_write_reg_imx178(id, &cmd);
	cmd = sen_set_cmd_info_imx178(0x302D, 1, (sensor_vd >> 8) & 0xFF, 0);
	rt |= sen_write_reg_imx178(id, &cmd);
	cmd = sen_set_cmd_info_imx178(0x302E, 1, (sensor_vd >> 16 ) & 0x01, 0);
	rt |= sen_write_reg_imx178(id, &cmd);

	return rt;
}

static ER sen_set_info_imx178(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_imx178(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_imx178(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_imx178(id, (CTL_SEN_FLIP *)(data));
		break;
	// TODO:
	#if 0
	case CTL_SENDRV_CFGID_SET_STATUS:
		sen_set_status_imx178(id, (CTL_SEN_STATUS *)(data));
		break;
	#endif
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_imx178(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_imx178(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_imx178(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_imx178(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_imx178((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_imx178((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_imx178(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_imx178((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_imx178(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_imx178(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_imx178((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_LVDS:
		sen_get_mode_lvds_imx178((CTL_SENDRV_GET_MODE_LVDS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_imx178((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_imx178(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_imx178(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_imx178(id, data);
		break;
	default:
		rt = E_NOSPT;
	}
	return rt;
}

static UINT32 sen_calc_chgmode_vd_imx178(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	sen_set_chgmode_fps_imx178(id, fps);
	sen_set_cur_fps_imx178(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_imx178(id, fps);
		sen_set_cur_fps_imx178(id, fps);
	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_imx178(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_imx178(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_imx178(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;

	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	return sensor_vd;
}

static void sen_set_gain_imx178(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 data1[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	sensor_ctrl_last[id].gain_ratio[0] = sensor_ctrl->gain_ratio[0];
	sensor_ctrl_last[id].gain_ratio[1] = sensor_ctrl->gain_ratio[1];

	// Calculate sensor gain
	if (mode_basic_param[cur_sen_mode[id]].frame_num == 0) {
		DBG_WRN("total_frame = 0, force to 1 \r\n");
		total_frame = 1;
	} else {
		total_frame = mode_basic_param[cur_sen_mode[id]].frame_num;
	}

	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		if (sensor_ctrl->gain_ratio[frame_cnt] < (mode_basic_param[cur_sen_mode[id]].gain.min)) {
			sensor_ctrl->gain_ratio[frame_cnt] = (mode_basic_param[cur_sen_mode[id]].gain.min);
		} else if (sensor_ctrl->gain_ratio[frame_cnt] > (mode_basic_param[cur_sen_mode[id]].gain.max)) {
			sensor_ctrl->gain_ratio[frame_cnt] = (mode_basic_param[cur_sen_mode[id]].gain.max);
		}

		data1[frame_cnt] = (6 * sen_common_calc_log_2(sensor_ctrl->gain_ratio[frame_cnt], 1000)) / 10;
		if (data1[frame_cnt] > (0x1E0)) {
			DBG_ERR("gain overflow gain_ratio = %d data1[0] = 0x%.8x \r\n", sensor_ctrl->gain_ratio[frame_cnt], data1[frame_cnt]);
			data1[frame_cnt] = 0x1E0;
		}
	}

	//set low light performance mode
	cmd = sen_set_cmd_info_imx178(0x301B, 1, 0x1E, 0x0);
	rt |= sen_write_reg_imx178(id, &cmd);

	//Set analog gain
	cmd = sen_set_cmd_info_imx178(0x301F, 1, data1[0] & 0xFF, 0x0);
	rt |= sen_write_reg_imx178(id, &cmd);
	cmd = sen_set_cmd_info_imx178(0x3020, 1, (data1[0] >> 8 )& 0x01, 0x0);
	rt |= sen_write_reg_imx178(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_expt_imx178(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps, cur_fps = 0, clac_fps = 0;
	UINT32 shs1 = 0, t_row = 0;
	ER rt = E_OK;

	sensor_ctrl_last[id].exp_time[0] = sensor_ctrl->exp_time[0];
	sensor_ctrl_last[id].exp_time[1] = sensor_ctrl->exp_time[1];

	if (mode_basic_param[cur_sen_mode[id]].frame_num == 0) {
		DBG_WRN("total_frame = 0, force to 1 \r\n");
		total_frame = 1;
	} else {
		total_frame = mode_basic_param[cur_sen_mode[id]].frame_num;
	}

	// Calculate exposure line
	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		// Calculates the exposure setting
		t_row = sen_calc_rowtime_imx178(id, cur_sen_mode[id]);
		if (0 == t_row) {
			DBG_WRN("t_row  = 0, must >= 1 \r\n");
			t_row = 1;
		}
		line[frame_cnt] = (sensor_ctrl->exp_time[frame_cnt]) * 10 / t_row;

		// Limit minimun exposure line
		if (line[frame_cnt] < MIN_EXPOSURE_LINE) {
			line[frame_cnt] = MIN_EXPOSURE_LINE;
		}
	}

	// Write exposure line
	// Get fps
	chgmode_fps = sen_get_chgmode_fps_imx178(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_imx178(id, cur_sen_mode[id]);
	expt_time = (line[0]) * t_row / 10;

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_imx178(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_imx178(id, cur_fps);

	if (line[0] > (MAX_VD_PERIOD - NON_EXP_LINE)) {
		DBG_ERR("max line overflow \r\n");
		line[0] = (MAX_VD_PERIOD - NON_EXP_LINE);
	}

	if (line[0] < MIN_EXPOSURE_LINE) {
		DBG_ERR("min line overflow\r\n");
		line[0] = MIN_EXPOSURE_LINE;
	}

	// Calculate shs1
	if (line[0] > (sensor_vd - NON_EXP_LINE)) {
		shs1 = NON_EXP_LINE;
	} else {
		shs1 = sensor_vd - line[0];
	}

	//Set vmax to sensor
	cmd = sen_set_cmd_info_imx178(0x302C, 1, sensor_vd & 0xFF, 0);
	rt |= sen_write_reg_imx178(id, &cmd);
	cmd = sen_set_cmd_info_imx178(0x302D, 1, (sensor_vd >> 8) & 0xFF, 0);
	rt |= sen_write_reg_imx178(id, &cmd);
	cmd = sen_set_cmd_info_imx178(0x302E, 1, (sensor_vd >> 16 ) & 0x01, 0);
	rt |= sen_write_reg_imx178(id, &cmd);

	// Set exposure line to sensor (shs1)
	cmd = sen_set_cmd_info_imx178(0x3034, 1, shs1 & 0xFF, 0);
	rt |= sen_write_reg_imx178(id, &cmd);
	cmd = sen_set_cmd_info_imx178(0x3035, 1, (shs1 >> 8) & 0xFF, 0);
	rt |= sen_write_reg_imx178(id, &cmd);
	cmd = sen_set_cmd_info_imx178(0x3036, 1, (shs1 >> 16) & 0x01, 0);
	rt |= sen_write_reg_imx178(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_imx178(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_imx178(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_imx178(0x300F, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx178(id, &cmd);

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
		if (*flip & CTL_SEN_FLIP_H) {
				cmd.data[0] |= 0x2;
		} else {
				cmd.data[0] &= (~0x02);
		}
	} else {
		DBG_WRN("no support mirror \r\n");
	}
	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (*flip & CTL_SEN_FLIP_V) {
			cmd.data[0] |= 0x1;
		} else {
			cmd.data[0] &= (~0x01);
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}

	rt |= sen_write_reg_imx178(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_imx178(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_imx178(0x300F, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx178(id, &cmd);

	*flip = CTL_SEN_FLIP_NONE;
	if (cmd.data[0] & 0x2) {
		*flip |= CTL_SEN_FLIP_H;
	}

	if (cmd.data[0] & 0x1) {
		*flip |= CTL_SEN_FLIP_V;
	}

	return rt;
}

#if 0
static void sen_set_status_imx178(CTL_SEN_ID id, CTL_SEN_STATUS *status)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	switch (*status) {
		case CTL_SEN_STATUS_STANDBY:
			cmd = sen_set_cmd_info_imx178(0x3000, 1, 0x01, 0x0);  // STANDBY ON
			rt = sen_write_reg_imx178(id, &cmd);
			vos_util_delay_ms(20);
			cmd = sen_set_cmd_info_imx178(0x3002, 1, 0x01, 0x0);  // Master mode stop
			rt |= sen_write_reg_imx178(id, &cmd);
			vos_util_delay_ms(20);
			if (rt != E_OK) {
				DBG_ERR("write register error %d \r\n", (INT)rt);
			}
			break;

		default:
			DBG_ERR("set status error %d \r\n", *status);
	}
}
#endif

static void sen_get_gain_imx178(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

static void sen_get_expt_imx178(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_imx178(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row;

	t_row = sen_calc_rowtime_imx178(id, cur_sen_mode[id]);
	*min_exp_time = t_row * MIN_EXPOSURE_LINE / 10 + 1;
}

static void sen_get_mode_basic_imx178(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_imx178(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_imx178(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_imx178(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_imx178(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
{
	if (data->type == CTL_SEN_IF_TYPE_LVDS) {
		return E_OK;
	}
	return E_NOSPT;
}

static void sen_get_fps_imx178(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_imx178(id);
	data->chg_fps = sen_get_chgmode_fps_imx178(id);
}

static void sen_get_speed_imx178(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &speed_param[mode], sizeof(CTL_SENDRV_GET_SPEED_PARAM));

	if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK ) {
		data->mclk_src = CTL_SEN_SIEMCLK_SRC_MCLK;
	} else if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK2) {
		data->mclk_src = CTL_SEN_SIEMCLK_SRC_MCLK2;
	} else if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK3) {
		data->mclk_src = CTL_SEN_SIEMCLK_SRC_MCLK3;
	} else if (sen_power[id].mclk == CTL_SEN_IGNORE) {
		data->mclk_src = CTL_SEN_SIEMCLK_SRC_IGNORE;
	} else {
		DBG_ERR("mclk source is fail \r\n");
	}
}

static void sen_get_mode_lvds_imx178(CTL_SENDRV_GET_MODE_LVDS_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &lvds_param[mode], sizeof(CTL_SENDRV_GET_MODE_LVDS_PARAM));
}

static void sen_get_modesel_imx178(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->data_fmt != CTL_SEN_DATA_FMT_RGB) {
		DBG_ERR("data_fmt %d N.S. \r\n", data->data_fmt);
		return;
	}


	if (data->frame_num == 1) {
		if ((data->size.w <= 3072) && (data->size.h <= 2048)) {
			if (data->frame_rate <= 3000) {
				data->mode = CTL_SEN_MODE_1;
				return;
			}
		}
	}

	DBG_ERR("fail (frame_rate%d,size%d*%d,if_type%d,data_fmt%d,frame_num%d) \r\n"
			, data->frame_rate, data->size.w, data->size.h, data->if_type, data->data_fmt, data->frame_num);
	data->mode = CTL_SEN_MODE_1;
}

static UINT32 sen_calc_rowtime_step_imx178(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 div_step = 1;

	return div_step;
}

static UINT32 sen_calc_rowtime_imx178(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 1000 * (mode_basic_param[mode].signal_info.hd_period) / ((speed_param[mode].pclk) / 10000);

	return row_time;
}

static void sen_get_rowtime_imx178(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_imx178(id, data->mode);
	data->row_time = sen_calc_rowtime_imx178(id, data->mode) * (data->row_time_step);
}

static void sen_set_cur_fps_imx178(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_imx178(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_imx178(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_imx178(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
int sen_init_imx178(SENSOR_DTSI_INFO *info)
{
	CTL_SEN_REG_OBJ reg_obj;
	CHAR node_path[64];
	CHAR compatible[64];
	ER rt = E_OK;

	sprintf(compatible, "nvt,sen_imx178");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_imx178");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_imx178;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_imx178();
	rt = ctl_sen_reg_sendrv("nvt_sen_imx178", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_imx178(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_imx178");
}

#else
static int __init sen_init_imx178(void)
{
	INT8 cfg_path[MAX_PATH_NAME_LENGTH+1] = { '\0' };
	CFG_FILE_FMT *pcfg_file;
	CTL_SEN_REG_OBJ reg_obj;
	ER rt = E_OK;

	// Parsing cfc file if exist
	if ((strstr(sen_cfg_path, "null")) || (strstr(sen_cfg_path, "NULL"))) {
		DBG_WRN("cfg file no exist \r\n");
		cfg_path[0] = '\0';
	} else {
		if ((sen_cfg_path != NULL) && (strlen(sen_cfg_path) <= MAX_PATH_NAME_LENGTH)) {
			strncpy((char *)cfg_path, sen_cfg_path, MAX_PATH_NAME_LENGTH);
		}

		if ((pcfg_file = sen_common_open_cfg(cfg_path)) != NULL) {
			DBG_MSG("load %s success \r\n", sen_cfg_path);
			sen_common_load_cfg_map(pcfg_file, &sen_map);
			sen_common_load_cfg_preset(pcfg_file, &sen_preset);
			sen_common_load_cfg_direction(pcfg_file, &sen_direction);
			sen_common_load_cfg_power(pcfg_file, &sen_power);
			sen_common_load_cfg_i2c(pcfg_file, &sen_i2c);
			sen_common_close_cfg(pcfg_file);
		} else {
			DBG_WRN("load cfg fail \r\n");
		}
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_imx178;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_imx178();
	rt = ctl_sen_reg_sendrv("nvt_sen_imx178", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_imx178(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_imx178");
}

module_init(sen_init_imx178);
module_exit(sen_exit_imx178);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_IMX178_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

