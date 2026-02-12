#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <kdrv_builtin/kdrv_builtin.h>
#include "isp_builtin.h"
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
VOS_MODULE_VERSION(nvt_sen_os04b10, 1, 42, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_OS04B10_MODULE_NAME     "sen_os04b10"
#define SEN_MAX_MODE               1
#define MAX_VD_PERIOD              0xFFFF
#define MAX_EXPOSURE_LINE          0xFFFF
#define MIN_EXPOSURE_LINE          4
#define NON_EXPOSURE_LINE          1
#define DEFAULT_VD_PERIOD          1493

#define SEN_I2C_ADDR 0x78 >> 1
#define SEN_I2C_COMPATIBLE "nvt,sen_os04b10"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_os04b10(void);
static void sen_pwr_ctrl_os04b10(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_os04b10(CTL_SEN_ID id);
static ER sen_close_os04b10(CTL_SEN_ID id);
static ER sen_sleep_os04b10(CTL_SEN_ID id);
static ER sen_wakeup_os04b10(CTL_SEN_ID id);
static ER sen_write_reg_os04b10(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_os04b10(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_os04b10(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_os04b10(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_os04b10(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_os04b10(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_os04b10(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_os04b10(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_os04b10(CTL_SEN_ID id, void *param);
static void sen_set_expt_os04b10(CTL_SEN_ID id, void *param);
static void sen_set_preset_os04b10(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_os04b10(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_os04b10(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
#if defined(__FREERTOS)
void sen_get_gain_os04b10(CTL_SEN_ID id, void *param);
void sen_get_expt_os04b10(CTL_SEN_ID id, void *param);
#else
static void sen_get_gain_os04b10(CTL_SEN_ID id, void *param);
static void sen_get_expt_os04b10(CTL_SEN_ID id, void *param);
#endif
static void sen_get_min_expt_os04b10(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_os04b10(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_os04b10(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_os04b10(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_os04b10(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_os04b10(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_os04b10(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_os04b10(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_mipi_os04b10(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_modesel_os04b10(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_os04b10(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_os04b10(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_os04b10(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_os04b10(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_os04b10(CTL_SEN_ID id);
static void sen_set_chgmode_fps_os04b10(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_os04b10(CTL_SEN_ID id);
//static void sen_get_iadj_os04b10(CTL_SEN_ID id,  CTL_SENDRV_GET_MODE_MANUAL_IADJ *data);
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
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
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
	SEN_OS04B10_MODULE_NAME,
	CTL_SEN_VENDOR_OMNIVISION,
	SEN_MAX_MODE,
	CTL_SEN_SUPPORT_PROPERTY_CHGFPS,
	0
};

static CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_param = {
	CTL_SEN_SIGNAL_MASTER,
	{CTL_SEN_ACTIVE_HIGH, CTL_SEN_ACTIVE_HIGH, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING}
};

static CTL_SENDRV_I2C i2c = {
	{
		{CTL_SEN_I2C_W_ADDR_DFT,     0x78},
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
		24000000,
		72000000,
		48000000
	}
};

static CTL_SENDRV_GET_MODE_MIPI_PARAM mipi_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_2,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	}
};

static CTL_SENDRV_GET_MODE_BASIC_PARAM mode_basic_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_B,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{2560, 1444},
		{{0, 2, 2560, 1440}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{2560, 1440},
		{0, 1584, 0, 1515},//1493 + vts // 0x16 ==> 22 blanking
		CTL_SEN_RATIO(16, 9),
		{1000, 248000},
		100
	}
};

static CTL_SEN_CMD os04b10_mode_1[] = {
	//{0xfd, 1, {0x00, 0x0}},
	//{0x20, 1, {0x00, 0x0}},
	//;delay 5ms
	//{SEN_CMD_DELAY, 1, { 5, 0x0}},       
	{0xfd, 1, {0x00, 0x0}},
	{0x34, 1, {0x71, 0x0}},//;71 for 240M,dac_clk divider
	{0x32, 1, {0x01, 0x0}},
	{0x33, 1, {0x01, 0x0}},//;dac 9bit
	{0x2e, 1, {0x0c, 0x0}},//;pll clk 720M
	{0xfd, 1, {0x01, 0x0}},
	{0x03, 1, {0x01, 0x0}},
	{0x04, 1, {0xc6, 0x0}},
	{0x06, 1, {0x0b, 0x0}},
	{0x0a, 1, {0x50, 0x0}},//;to fix the h_length
	{0x38, 1, {0x20, 0x0}},
	{0x39, 1, {0x08, 0x0}},
	{0x31, 1, {0x01, 0x0}},//;enable ana crop
	{0x24, 1, {0xff, 0x0}},
	{0x01, 1, {0x01, 0x0}},
	{0x11, 1, {0x59, 0x0}},//; 48 for 240M
	{0x13, 1, {0xf4, 0x0}},
	{0x14, 1, {0xff, 0x0}},
	{0x19, 1, {0xf2, 0x0}},//;col_comparator current 6.3uA
	{0x16, 1, {0x68, 0x0}},
	{0x1a, 1, {0x5e, 0x0}},
	{0x1c, 1, {0x1a, 0x0}},
	{0x1d, 1, {0xd6, 0x0}},
	{0x1f, 1, {0x17, 0x0}},//;enable col gain
	{0x20, 1, {0x99, 0x0}},//;enable local buffer,NVDD=-1.4V
	{0x26, 1, {0x76, 0x0}},//;HVDD=3.3V
	{0x27, 1, {0x0c, 0x0}},//;disable odd_even mode
	{0x29, 1, {0x3b, 0x0}},
	{0x2a, 1, {0x00, 0x0}},
	{0x2b, 1, {0x8e, 0x0}},//;disable pixel ldo
	{0x2c, 1, {0x0b, 0x0}},
	{0x2e, 1, {0x02, 0x0}},//;for sa settling
	{0x3f, 1, {0x00, 0x0}},	
	{0x44, 1, {0x03, 0x0}},
	{0x45, 1, {0xbe, 0x0}},
	{0x50, 1, {0x06, 0x0}},
	{0x51, 1, {0x10, 0x0}},//0x18 abby
	{0x52, 1, {0x0d, 0x0}},//0x10 abby
	{0x53, 1, {0x08, 0x0}},
	{0x55, 1, {0x15, 0x0}},//;for 240M,10
	{0x56, 1, {0x00, 0x0}},
	{0x57, 1, {0x09, 0x0}},//;for 240M,20//0x0d abby
	{0x59, 1, {0x00, 0x0}},//;decrease the h_length
	{0x5a, 1, {0x04, 0x0}},//;decrease the h_length
	{0x5b, 1, {0x00, 0x0}},//;put reset address at PTX
	//{0x5c, 1, {0xc0, 0x0}},//;put reset address at PTX, it can effect row noise
	{0x5c, 1, {0xe0, 0x0}},//;put reset address at PTX, it can effect row noise//0xb8 abby
	{0x5d, 1, {0x00, 0x0}},
	{0x65, 1, {0x00, 0x0}},
	{0x67, 1, {0x00, 0x0}},
	{0x66, 1, {0x2a, 0x0}},//;50 for 240M
	{0x68, 1, {0x2c, 0x0}},//;52 for 240M
	{0x69, 1, {0x0c, 0x0}},//0x16 abby
	{0x6a, 1, {0x0a, 0x0}},//0x10 abby
	{0x6b, 1, {0x03, 0x0}},
	{0x6c, 1, {0x18, 0x0}},
	{0x71, 1, {0x42, 0x0}},//;60 for 240M
	{0x72, 1, {0x04, 0x0}},
	{0x73, 1, {0x30, 0x0}},//;vertical shading can be improved;80 for 240M
	{0x74, 1, {0x03, 0x0}},
	{0x77, 1, {0x28, 0x0}},
	{0x7b, 1, {0x00, 0x0}},
	{0x7f, 1, {0x18, 0x0}},
	{0x83, 1, {0xf0, 0x0}},
	{0x85, 1, {0x10, 0x0}},
	{0x86, 1, {0xf0, 0x0}},
	{0x8a, 1, {0x33, 0x0}}, //;bitline current
	{0x8b, 1, {0x33, 0x0}},
	{0x28, 1, {0x04, 0x0}}, //;analog_dummy_size
	{0x34, 1, {0x00, 0x0}},
	{0x35, 1, {0x08, 0x0}}, //;h_start
	{0x36, 1, {0x0a, 0x0}}, //;h_size
	{0x37, 1, {0x00, 0x0}},
	{0x4a, 1, {0x00, 0x0}},
	{0x4b, 1, {0x02, 0x0}}, //;v_start
	{0x4c, 1, {0x05, 0x0}}, //;v_size
	{0x4d, 1, {0xa4, 0x0}},
	{0x01, 1, {0x01, 0x0}},
	{0x8e, 1, {0x0a, 0x0}},
	{0x8f, 1, {0x00, 0x0}},
	{0x90, 1, {0x05, 0x0}},
	{0x91, 1, {0xa4, 0x0}},
	{0xa1, 1, {0x04, 0x0}},
	{0xc4, 1, {0x80, 0x0}},
	{0xc5, 1, {0x80, 0x0}},
	{0xc6, 1, {0x80, 0x0}},
	{0xc7, 1, {0x80, 0x0}},
	{0xfb, 1, {0x03, 0x0}},
	{0xfa, 1, {0x16, 0x0}},	//;manual trigger blc when initial sensor
	{0xfa, 1, {0x14, 0x0}},	//;enable RPC trigger and offset trigger
	{0xf5, 1, {0x02, 0x0}},	//;offset trigger threshold
	{0xf0, 1, {0x40, 0x0}},
	{0xf1, 1, {0x40, 0x0}},
	{0xf2, 1, {0x40, 0x0}},
	{0xf3, 1, {0x40, 0x0}},
	{0xb1, 1, {0x01, 0x0}},
	{0xb6, 1, {0x80, 0x0}},
	//;78 b1 03 ;shutdowna  
	{0xfd, 1, {0x00, 0x0}},
	{0x36, 1, {0x01, 0x0}},
	{0x34, 1, {0x72, 0x0}},
	{0x34, 1, {0x71, 0x0}},
	{0x36, 1, {0x00, 0x0}},
	{0xfd, 1, {0x01, 0x0}},
	{0xfb, 1, {0x03, 0x0}},	//;enable blc  
	{0xfd, 1, {0x03, 0x0}},
	{0xc0, 1, {0x01, 0x0}},	//;enable OTP information transferring
	//;For OTP_BPC
	{0xfd, 1, {0x02, 0x0}},
	{0xa8, 1, {0x01, 0x0}},
	{0xa9, 1, {0x00, 0x0}},
	{0xaa, 1, {0x06, 0x0}},	//;Row start
	{0xab, 1, {0x00, 0x0}},
	{0xac, 1, {0x08, 0x0}},	//;column start
	{0xad, 1, {0x05, 0x0}},
	{0xae, 1, {0xa0, 0x0}},	//;row size 1440
	{0xaf, 1, {0x0a, 0x0}},
	{0xb0, 1, {0x00, 0x0}},	//;width, 2560
	{0x62, 1, {0x07, 0x0}},	//;Row start 8LSB
	{0x63, 1, {0x00, 0x0}},	//;row start 4msb
	{SEN_CMD_SETVD, 1, {0x00, 0x0}},    
	{SEN_CMD_PRESET, 1, {0x00, 0x0}},   
	{SEN_CMD_DIRECTION, 1, {0x00, 0x0}},
	{0xfd, 1, {0x01, 0x0}},
	{0xb1, 1, {0x03, 0x0}},
};

static UINT32 cur_sen_mode[CTL_SEN_ID_MAX] = {CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1};
static UINT32 cur_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 chgmode_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 power_ctrl_mclk[CTL_SEN_CLK_SEL_MAX] = {0};
static UINT32 reset_ctrl_count[CTL_SEN_ID_MAX] = {0};
static UINT32 pwdn_ctrl_count[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_CTRL sensor_ctrl_last[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_PRESET_CTRL preset_ctrl[CTL_SEN_ID_MAX] = {0};
static INT32 is_fastboot[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_id[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_addr[CTL_SEN_ID_MAX];
static BOOL i2c_valid[CTL_SEN_ID_MAX];

static CTL_SEN_DRV_TAB os04b10_sen_drv_tab = {
	sen_open_os04b10,
	sen_close_os04b10,
	sen_sleep_os04b10,
	sen_wakeup_os04b10,
	sen_write_reg_os04b10,
	sen_read_reg_os04b10,
	sen_chg_mode_os04b10,
	sen_chg_fps_os04b10,
	sen_set_info_os04b10,
	sen_get_info_os04b10,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_os04b10(void)
{
	return &os04b10_sen_drv_tab;
}

static void sen_pwr_ctrl_os04b10(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
{
	UINT32 i = 0;
	UINT32 reset_count = 0, pwdn_count = 0;
	DBG_IND("enter flag %d \r\n", flag);

	if ((flag == CTL_SEN_PWR_CTRL_TURN_ON) && ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr))) {

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
			vos_util_delay_ms(sen_power[id].stable_time);
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

static CTL_SEN_CMD sen_set_cmd_info_os04b10(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_os04b10(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_os04b10(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_os04b10);
	#endif

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_DEFAULT;
	i2c_valid[id] = TRUE;
	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		rt = sen_i2c_init_driver(id, &sen_i2c[id]);

		if (rt != E_OK) {
			i2c_valid[id] = FALSE;

			DBG_ERR("init. i2c driver fail (%d) \r\n", id);
		}
	}

	return rt;
}

static ER sen_close_os04b10(CTL_SEN_ID id)
{
	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		if (i2c_valid[id]) {
			sen_i2c_remove_driver(id);
		}
	} else {
		is_fastboot[id] = 0;
		#if defined(__KERNEL__)
		isp_builtin_uninit_i2c(id);
		#endif
	}

	i2c_valid[id] = FALSE;

	return E_OK;
}

static ER sen_sleep_os04b10(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_os04b10(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_os04b10(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	struct i2c_msg msgs;
	unsigned char buf[2];
	int i;

	if (!i2c_valid[id]) {
		return E_NOSPT;
	}

	buf[0]     = cmd->addr & 0xFF;
	buf[1]     = cmd->data[0] & 0xFF;
	msgs.addr  = sen_i2c[id].addr;
	msgs.flags = 0;
	msgs.len   = 2;
	msgs.buf   = buf;

	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		i = 0;
		while(1){
			if (sen_i2c_transfer(id, &msgs, 1) == 0) {
				break;
			}

			i++;
			if (i == 5)
				return E_SYS;
		}
	} else {
		#if defined(__KERNEL__)
		isp_builtin_set_transfer_i2c(id, &msgs, 1);
		#endif
	}

	return E_OK;
}

static ER sen_read_reg_os04b10(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	struct i2c_msg  msgs[2];
	unsigned char   tmp[1], tmp2[2];
	int i;

	if (!i2c_valid[id]) {
		return E_NOSPT;
	}

	tmp[0]        = cmd->addr & 0xFF;
	msgs[0].addr  = sen_i2c[id].addr;
	msgs[0].flags = 0;
	msgs[0].len   = 1;
	msgs[0].buf   = tmp;

	tmp2[0]       = 0;
	msgs[1].addr  = sen_i2c[id].addr;
	msgs[1].flags = 1;
	msgs[1].len   = 1;
	msgs[1].buf   = tmp2;

	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		i = 0;
		while(1){
			if (sen_i2c_transfer(id, msgs, 2) == 0)
				break;
			i++;
			if (i == 5)
				return E_SYS;
		}
	} else {
		#if defined(__KERNEL__)
		isp_builtin_set_transfer_i2c(id, msgs, 2);
		#endif
	}

	cmd->data[0] = tmp2[0];

	return E_OK;
}

static UINT32 sen_get_cmd_tab_os04b10(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = os04b10_mode_1;
		return sizeof(os04b10_mode_1) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_os04b10(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{
	ISP_SENSOR_CTRL sensor_ctrl = {0};
	CTL_SEN_CMD *p_cmd_list = NULL, cmd;
	CTL_SEN_FLIP flip = CTL_SEN_FLIP_NONE;
	UINT32 sensor_vd;
	UINT32 idx, cmd_num = 0;
	ER rt = E_OK;

	cur_sen_mode[id] = chgmode_obj.mode;

	if (is_fastboot[id]) {
		#if defined(__KERNEL__)
		ISP_BUILTIN_SENSOR_CTRL *p_sensor_ctrl_temp;

		p_sensor_ctrl_temp = isp_builtin_get_sensor_gain(id);
		sensor_ctrl.gain_ratio[0] = p_sensor_ctrl_temp->gain_ratio[0];
		sensor_ctrl.gain_ratio[1] = p_sensor_ctrl_temp->gain_ratio[1];
		p_sensor_ctrl_temp = isp_builtin_get_sensor_expt(id);
		sensor_ctrl.exp_time[0] = p_sensor_ctrl_temp->exp_time[0];
		sensor_ctrl.exp_time[1] = p_sensor_ctrl_temp->exp_time[1];
		sen_set_chgmode_fps_os04b10(id, isp_builtin_get_chgmode_fps(id));
		sen_set_cur_fps_os04b10(id, isp_builtin_get_chgmode_fps(id));
		sen_set_gain_os04b10(id, &sensor_ctrl);
		sen_set_expt_os04b10(id, &sensor_ctrl);
		#endif
		preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

		return E_OK;
	}

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_os04b10(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_os04b10(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_os04b10(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_os04b10(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_os04b10(0xFD, 1, 0x01, 0x00);
			rt |= sen_write_reg_os04b10(id, &cmd);
			cmd = sen_set_cmd_info_os04b10(0x05, 1, ((sensor_vd - DEFAULT_VD_PERIOD) >> 8) & 0xFF, 0x00);
			rt |= sen_write_reg_os04b10(id, &cmd);
			cmd = sen_set_cmd_info_os04b10(0x06, 1, (sensor_vd - DEFAULT_VD_PERIOD) & 0xFF, 0x00);
			rt |= sen_write_reg_os04b10(id, &cmd);
			cmd = sen_set_cmd_info_os04b10(0x01, 1, 0x01, 0x00);
			rt |= sen_write_reg_os04b10(id, &cmd);						
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

			sen_set_gain_os04b10(id, &sensor_ctrl);
			sen_set_expt_os04b10(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_os04b10(id, &flip);
		} else {
			cmd = sen_set_cmd_info_os04b10(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_os04b10(id, &cmd);
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_os04b10(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_os04b10(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_os04b10(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_os04b10(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	cmd = sen_set_cmd_info_os04b10(0xFD, 1, 0x01, 0x00);
	rt |= sen_write_reg_os04b10(id, &cmd);
	cmd = sen_set_cmd_info_os04b10(0x05, 1, ((sensor_vd - DEFAULT_VD_PERIOD) >> 8) & 0xFF, 0x00);
	rt |= sen_write_reg_os04b10(id, &cmd);
	cmd = sen_set_cmd_info_os04b10(0x06, 1, (sensor_vd - DEFAULT_VD_PERIOD) & 0xFF, 0x00);
	rt |= sen_write_reg_os04b10(id, &cmd);
	cmd = sen_set_cmd_info_os04b10(0x01, 1, 0x01, 0x00);
	rt |= sen_write_reg_os04b10(id, &cmd);

	return rt;
}

static ER sen_set_info_os04b10(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_os04b10(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_os04b10(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_os04b10(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_os04b10(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_os04b10(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_os04b10(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_os04b10(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_os04b10((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_os04b10((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_os04b10(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_os04b10((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_os04b10(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_os04b10(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_os04b10((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_os04b10((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_os04b10((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_os04b10(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_os04b10(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_os04b10(id, data);
		break;
/*
	case CTL_SENDRV_CFGID_GET_MODE_MIPI_MANUAL_IADJ:
		sen_get_iadj_os04b10(id, (CTL_SENDRV_GET_MODE_MANUAL_IADJ *)(data));
		break;
*/
	default:
		rt = E_NOSPT;
	}
	return rt;
}

static UINT32 sen_calc_chgmode_vd_os04b10(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	sen_set_chgmode_fps_os04b10(id, fps);
	sen_set_cur_fps_os04b10(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_os04b10(id, fps);
		sen_set_cur_fps_os04b10(id, fps);
	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_os04b10(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_os04b10(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_os04b10(CTL_SEN_ID id, UINT32 fps)
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

static void sen_set_gain_os04b10(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 data1[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data2[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 a_gain = 0, d_gain = 0;
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

		if((sensor_ctrl->gain_ratio[frame_cnt]) <= 15500) {
			a_gain = (sensor_ctrl->gain_ratio[frame_cnt]) * 16 / 1000;
			d_gain = 0x08;
		} else {
			d_gain = (sensor_ctrl->gain_ratio[frame_cnt]) * 100 * 8 / 15500 / 100;
			a_gain = 0xF8;
		}

		data1[frame_cnt] = a_gain;
		data2[frame_cnt] = d_gain;
	}

	cmd = sen_set_cmd_info_os04b10(0xFD, 1, 0x01, 0x0);
	rt |= sen_write_reg_os04b10(id, &cmd);
	cmd = sen_set_cmd_info_os04b10(0x24, 1, data1[0] & 0xFF, 0x0);
	rt |= sen_write_reg_os04b10(id, &cmd);
	cmd = sen_set_cmd_info_os04b10(0x39, 1, data2[0] & 0xFF, 0x0);
	rt |= sen_write_reg_os04b10(id, &cmd);
	cmd = sen_set_cmd_info_os04b10(0x01, 1, 0x01, 0x0);
	rt |= sen_write_reg_os04b10(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_expt_os04b10(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps = 0, cur_fps = 0, clac_fps = 0, t_row = 0;
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
		t_row = sen_calc_rowtime_os04b10(id, cur_sen_mode[id]);
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
	chgmode_fps = sen_get_chgmode_fps_os04b10(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_os04b10(id, cur_sen_mode[id]);
	expt_time = (line[0]) * t_row / 10;

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");		
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_os04b10(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_os04b10(id, cur_fps);

	//Check max vts
	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("max vts overflow\r\n");
		sensor_vd = MAX_VD_PERIOD;
	}

	cmd = sen_set_cmd_info_os04b10(0xFD, 1, 0x01, 0);
	rt |= sen_write_reg_os04b10(id, &cmd);

	cmd = sen_set_cmd_info_os04b10(0x05, 1, ((sensor_vd - DEFAULT_VD_PERIOD) >> 8) & 0xFF, 0);
	rt |= sen_write_reg_os04b10(id, &cmd);

	cmd = sen_set_cmd_info_os04b10(0x06, 1, (sensor_vd - DEFAULT_VD_PERIOD) & 0xFF, 0);
	rt |= sen_write_reg_os04b10(id, &cmd);

	//Check max exp line reg
	if (line[0] > MAX_EXPOSURE_LINE) {
		line[0] = MAX_EXPOSURE_LINE;
	}

	//Check max exp line
	if (line[0] > (sensor_vd - NON_EXPOSURE_LINE)) {
		line[0] = sensor_vd - NON_EXPOSURE_LINE;
	}

	cmd = sen_set_cmd_info_os04b10(0x03, 1, (line[0] >> 8) & 0xFF, 0);
	rt |= sen_write_reg_os04b10(id, &cmd);
	cmd = sen_set_cmd_info_os04b10(0x04, 1, line[0] & 0xFF, 0);
	rt |= sen_write_reg_os04b10(id, &cmd);
	cmd = sen_set_cmd_info_os04b10(0x01, 1, 0x01 , 0);
	rt |= sen_write_reg_os04b10(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_os04b10(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_os04b10(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
	} else {
		DBG_WRN("no support mirror \r\n");
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
	} else {
		DBG_WRN("no support flip \r\n");
	}

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_os04b10(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	*flip = CTL_SEN_FLIP_NONE;

	cmd = sen_set_cmd_info_os04b10(0xFD, 1, 0x01, 0x0);
	rt |= sen_write_reg_os04b10(id, &cmd);

	cmd = sen_set_cmd_info_os04b10(0x3F, 1, 0x0, 0x0);
	rt |= sen_read_reg_os04b10(id, &cmd);

	if (cmd.data[0] & (0x1)) {
		*flip |= CTL_SEN_FLIP_H;
	}

	if (cmd.data[0] & (0x02)) {
		*flip |= CTL_SEN_FLIP_V;
	}

	return rt;
}

#if defined(__FREERTOS)
void sen_get_gain_os04b10(CTL_SEN_ID id, void *param)
#else
static void sen_get_gain_os04b10(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

#if defined(__FREERTOS)
void sen_get_expt_os04b10(CTL_SEN_ID id, void *param)
#else
static void sen_get_expt_os04b10(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_os04b10(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row;

	t_row = sen_calc_rowtime_os04b10(id, cur_sen_mode[id]);
	*min_exp_time = t_row * MIN_EXPOSURE_LINE / 10 + 1;
}

static void sen_get_mode_basic_os04b10(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_os04b10(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_os04b10(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_os04b10(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_os04b10(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
{
	#if 1
	if (data->type == CTL_SEN_IF_TYPE_MIPI) {
		return E_OK;
	}
	return E_NOSPT;
	#else
	if (data->type == CTL_SEN_IF_TYPE_MIPI) {
		memcpy(&data->info.mipi, &mipi, sizeof(CTL_SENDRV_MIPI));
		return E_OK;
	}
	return E_NOSPT;
	#endif
}

static void sen_get_fps_os04b10(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_os04b10(id);
	data->chg_fps = sen_get_chgmode_fps_os04b10(id);
}

static void sen_get_speed_os04b10(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
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

static void sen_get_mode_mipi_os04b10(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_modesel_os04b10(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->if_type != CTL_SEN_IF_TYPE_MIPI) {
		DBG_ERR("if_type %d N.S. \r\n", data->if_type);
		return;
	}

	if (data->data_fmt != CTL_SEN_DATA_FMT_RGB) {
		DBG_ERR("data_fmt %d N.S. \r\n", data->data_fmt);
		return;
	}

	if (data->frame_num == 1) {
		if ((data->size.w <= 2560) && (data->size.h <= 1440)) {
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

static UINT32 sen_calc_rowtime_step_os04b10(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 div_step = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	if (mode_basic_param[mode].mode_type == CTL_SEN_MODE_LINEAR) {
		div_step = 1;
	}

	return div_step;
}

static UINT32 sen_calc_rowtime_os04b10(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 10 * (mode_basic_param[mode].signal_info.hd_period) / ((speed_param[mode].pclk) / 1000000);

	return row_time;
}

static void sen_get_rowtime_os04b10(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_os04b10(id, data->mode);
	data->row_time = sen_calc_rowtime_os04b10(id, data->mode) * (data->row_time_step);
}

static void sen_set_cur_fps_os04b10(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_os04b10(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_os04b10(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_os04b10(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}
/*
static void sen_get_iadj_os04b10(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_MANUAL_IADJ *data)
{
	data->sel = CTL_SEN_MANUAL_IADJ_SEL_IADJ;
	data->val.iadj = 3;
}
*/
#if defined(__FREERTOS)
void sen_get_i2c_id_os04b10(CTL_SEN_ID id, UINT32 *i2c_id)
{
	*i2c_id = sen_i2c[id].id;
}

void sen_get_i2c_addr_os04b10(CTL_SEN_ID id, UINT32 *i2c_addr)
{
	*i2c_addr = sen_i2c[id].addr;
}

int sen_init_os04b10(SENSOR_DTSI_INFO *info)
{
	CTL_SEN_REG_OBJ reg_obj;
	CHAR node_path[64];
	CHAR compatible[64];
	UINT32 id;
	ER rt = E_OK;

	for (id = 0; id < CTL_SEN_ID_MAX; id++ ) {
		is_fastboot[id] = 0;
		fastboot_i2c_id[id] = 0xFFFFFFFF;
		fastboot_i2c_addr[id] = 0x0;
	}

	sprintf(compatible, "nvt,sen_os04b10");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_os04b10");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_os04b10;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_os04b10();
	rt = ctl_sen_reg_sendrv("nvt_sen_os04b10", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_os04b10(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_os04b10");
}

#else
static int __init sen_init_os04b10(void)
{
	INT8 cfg_path[MAX_PATH_NAME_LENGTH+1] = { '\0' };
	CFG_FILE_FMT *pcfg_file;
	CTL_SEN_REG_OBJ reg_obj;
	UINT32 id;
	ER rt = E_OK;

	for (id = 0; id < ISP_BUILTIN_ID_MAX_NUM; id++ ) {
		is_fastboot[id] = kdrv_builtin_is_fastboot();
		fastboot_i2c_id[id] = isp_builtin_get_i2c_id(id);
		fastboot_i2c_addr[id] = isp_builtin_get_i2c_addr(id);
	}

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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_os04b10;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_os04b10();
	rt = ctl_sen_reg_sendrv("nvt_sen_os04b10", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_os04b10(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_os04b10");
}

module_init(sen_init_os04b10);
module_exit(sen_exit_os04b10);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_OS04B10_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

