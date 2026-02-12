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
VOS_MODULE_VERSION(nvt_sen_imx385, 1, 41, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_IMX385_MODULE_NAME     "sen_imx385"
#define SEN_MAX_MODE               6
#define MAX_VD_PERIOD              0xFFFF
#define MAX_EXPOSURE_LINE          0xFFFF
#define MIN_EXPOSURE_LINE          1
#define NON_EXPOSURE_LINE          5
#define BRL                        1097

#define SEN_I2C_ADDR 0x34 >> 1
#define SEN_I2C_COMPATIBLE "nvt,sen_imx385"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_imx385(void);
static void sen_pwr_ctrl_imx385(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_imx385(CTL_SEN_ID id);
static ER sen_close_imx385(CTL_SEN_ID id);
static ER sen_sleep_imx385(CTL_SEN_ID id);
static ER sen_wakeup_imx385(CTL_SEN_ID id);
static ER sen_write_reg_imx385(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_imx385(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_imx385(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_imx385(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_imx385(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_imx385(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_imx385(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_imx385(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_imx385(CTL_SEN_ID id, void *param);
static void sen_set_expt_imx385(CTL_SEN_ID id, void *param);
static void sen_set_preset_imx385(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_imx385(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_imx385(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
#if defined(__FREERTOS)
void sen_get_gain_imx385(CTL_SEN_ID id, void *param);
void sen_get_expt_imx385(CTL_SEN_ID id, void *param);
#else
static void sen_get_gain_imx385(CTL_SEN_ID id, void *param);
static void sen_get_expt_imx385(CTL_SEN_ID id, void *param);
#endif
static void sen_get_min_expt_imx385(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_imx385(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_imx385(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_imx385(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_imx385(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_imx385(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_imx385(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_imx385(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_mipi_imx385(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_mode_lvds_imx385(CTL_SENDRV_GET_MODE_LVDS_PARAM *data);
static void sen_get_modesel_imx385(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_imx385(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_imx385(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_imx385(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_imx385(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_imx385(CTL_SEN_ID id);
static void sen_set_chgmode_fps_imx385(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_imx385(CTL_SEN_ID id);

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
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},//MCLK, pwdn, reset, reset_time,stable_time
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
	SEN_IMX385_MODULE_NAME,
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
		37125000,//MCLK
		148500000,//PCLK
		74250000,//DATA RATE
	},
	{
		CTL_SEN_MODE_2,
		CTL_SEN_SIEMCLK_SRC_DFT,
		37125000,//MCLK
		148500000,//PCLK
		148500000,//DATA RATE
	},
	{
		CTL_SEN_MODE_3,
		CTL_SEN_SIEMCLK_SRC_DFT,
		37125000,//MCLK
		148500000,//PCLK
		178200000,//DATA RATE
	},
	{
		CTL_SEN_MODE_4,
		CTL_SEN_SIEMCLK_SRC_DFT,
		37125000,//MCLK
		148500000,//PCLK
		74250000,//DATA RATE
	},
	{
		CTL_SEN_MODE_5,
		CTL_SEN_SIEMCLK_SRC_DFT,
		37125000,//MCLK
		148500000,//PCLK
		148500000,//DATA RATE
	},
	{
		CTL_SEN_MODE_6,
		CTL_SEN_SIEMCLK_SRC_DFT,
		37125000,//MCLK
		148500000,//PCLK
		178200000,//DATA RATE
	}
};

static CTL_SENDRV_GET_MODE_MIPI_PARAM mipi_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	},
	{
		CTL_SEN_MODE_2,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	},
	{
		CTL_SEN_MODE_3,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{2, 1, 4, 0},
		SEN_BIT_OFS_0|SEN_BIT_OFS_1|SEN_BIT_OFS_2|SEN_BIT_OFS_3
	},
	{
		CTL_SEN_MODE_4, //dummy mode
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	},
	{
		CTL_SEN_MODE_5, //dummy mode
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE
	},
	{
		CTL_SEN_MODE_6, //dummy mode
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0}, {CTL_SEN_MIPI_MANUAL_NONE, 0} },
		0,
		{2, 1, 4, 0},
		SEN_BIT_OFS_0|SEN_BIT_OFS_1|SEN_BIT_OFS_2|SEN_BIT_OFS_3
	}
};

static CTL_SENDRV_GET_MODE_LVDS_PARAM lvds_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1, //dummy mode
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ 0, 0, 0, 0},
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE,
		0,
		CTL_SEN_DATAIN_BIT_ORDER_LSB,
		0,
	},
	{
		CTL_SEN_MODE_2, //dummy mode
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ 0, 0, 0, 0},
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE,
		0,
		CTL_SEN_DATAIN_BIT_ORDER_LSB,
		0,
	},
	{
		CTL_SEN_MODE_3, //dummy mode
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ 0, 0, 0, 0},
		{2, 1, 4, 0},
		SEN_BIT_OFS_NONE,
		0,
		CTL_SEN_DATAIN_BIT_ORDER_LSB,
		0,
	},
	{
		CTL_SEN_MODE_4,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ 1, 2, 0, 3},
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE,
		0,
		CTL_SEN_DATAIN_BIT_ORDER_LSB,
		0,
	},
	{
		CTL_SEN_MODE_5,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ 1, 2, 0, 3},
		{0, 0, 0, 0},
		SEN_BIT_OFS_NONE,
		0,
		CTL_SEN_DATAIN_BIT_ORDER_LSB,
		0,
	},
	{
		CTL_SEN_MODE_6,
		CTL_SEN_CLKLANE_1,
		CTL_SEN_DATALANE_4,
		{ 1, 2, 0, 3},
		{2, 1, 4, 0},
		SEN_BIT_OFS_NONE,
		0,
		CTL_SEN_DATAIN_BIT_ORDER_LSB,
		0,
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
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1937, 1097},//sensor output
		{{8, 8, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},//ISP start crop (x,y)
		{1920, 1080},//valid_size
		{0, 4400, 0, 1125},//HMX,VMX 
		CTL_SEN_RATIO(16, 9),
		{1000, 3981072},//GAIN range 0~3981X
		100
	},
	{
		CTL_SEN_MODE_2,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		6000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1937, 1097},//sensor output
		{{8, 8, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},//ISP start crop (x,y)
		{1920, 1080},//valid_size
		{0, 2200, 0, 1125},//HMX,VMX 
		CTL_SEN_RATIO(16, 9),
		{1000, 3981072},//GAIN range 0~3981X
		100
	},
	{
		CTL_SEN_MODE_3,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_STAGGER_HDR,
		3000,
		2,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1937, 1097},//sensor output
		{{8, 8, 1920, 1080}, {8, 8, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}},//ISP start crop (x,y)(long, short)
		{1920, 1080},//valid_size
		{0, 2200, 0, 1125},//HMX,VMX
		CTL_SEN_RATIO(16, 9),
		{1000, 3981072},//GAIN range 0~3981X
		100
	},
	{
		CTL_SEN_MODE_4,
		CTL_SEN_IF_TYPE_LVDS,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1937, 1097},//sensor output
		{{8, 9, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},//ISP start crop (x,y)
		{1920, 1080},//valid_size
		{0, 4400, 0, 1125},//HMX,VMX
		CTL_SEN_RATIO(16, 9),
		{1000, 3981072},//GAIN range 0~3981X
		100
	},
	{
		CTL_SEN_MODE_5,
		CTL_SEN_IF_TYPE_LVDS,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		6000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1937, 1097},//sensor output
		{{8, 9, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},//ISP start crop (x,y)
		{1920, 1080},//valid_size
		{0, 2200, 0, 1125},//HMX,VMX
		CTL_SEN_RATIO(16, 9),
		{1000, 3981072},//GAIN range 0~3981X
		100
	},
	{
		CTL_SEN_MODE_6,
		CTL_SEN_IF_TYPE_LVDS,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_STAGGER_HDR,
		3000,
		2,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1937, 1097},//sensor output
		{{8, 9, 1920, 1080}, {8, 9, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}},//ISP start crop (x,y)(long, short)
		{1920, 1080},//valid_size
		{0, 2200, 0, 1125},//HMX,VMX 
		CTL_SEN_RATIO(16, 9),
		{1000, 3981072},//GAIN range 0~3981X
		100
	}
};

static CTL_SEN_CMD imx385_mode_1[] = {      
    {0x3003, 1, {0x01, 0x0}}, // software reset
		{SEN_CMD_DELAY, 1, { 1, 0x0}},	
		{0x3000, 1, {0x01, 0x0}},/* standby */
		{SEN_CMD_DELAY, 1, { 20, 0x0}},		
		{0x3002, 1, {0x01, 0x0}},
		{SEN_CMD_DELAY, 1, { 20, 0x0}},
    {0x3009, 1, {0x02, 0x0}},
    {0x3012, 1, {0x2c, 0x0}},
    {0x3013, 1, {0x01, 0x0}},
    {0x3014, 1, {0x07, 0x0}}, //Gain
    {0x3018, 1, {0x65, 0x0}},//VMAX
    {0x3019, 1, {0x04, 0x0}},//VMAX
    {0x301b, 1, {0x30, 0x0}},//HMAX
    {0x301c, 1, {0x11, 0x0}},//HMAX 
    {0x3020, 1, {0x02, 0x0}}, //SHS1
    {0x3021, 1, {0x00, 0x0}}, //SHS1
    {0x3049, 1, {0x0a, 0x0}},
    {0x3054, 1, {0x66, 0x0}},
    {0x305d, 1, {0x00, 0x0}},
    {0x305f, 1, {0x00, 0x0}},
    {0x310b, 1, {0x07, 0x0}},
    {0x3110, 1, {0x12, 0x0}},
    {0x31ed, 1, {0x38, 0x0}},
    {0x3338, 1, {0xd4, 0x0}},
    {0x3339, 1, {0x40, 0x0}},
    {0x333a, 1, {0x10, 0x0}},
    {0x333b, 1, {0x00, 0x0}},
    {0x333c, 1, {0xd4, 0x0}},
    {0x333d, 1, {0x40, 0x0}},
    {0x333e, 1, {0x10, 0x0}},
    {0x333f, 1, {0x00, 0x0}},
    {0x3344, 1, {0x10, 0x0}},
    {0x336b, 1, {0x2f, 0x0}},
    {0x3380, 1, {0x20, 0x0}},
    {0x3381, 1, {0x25, 0x0}},
    {0x3382, 1, {0x5f, 0x0}},
    {0x3383, 1, {0x17, 0x0}},
    {0x3384, 1, {0x2f, 0x0}},
    {0x3385, 1, {0x17, 0x0}},
    {0x3386, 1, {0x17, 0x0}},
    {0x3387, 1, {0x0f, 0x0}},
    {0x3388, 1, {0x4f, 0x0}},
    {0x338d, 1, {0xb4, 0x0}},
    {0x338e, 1, {0x01, 0x0}},    
    {SEN_CMD_DIRECTION, 1, {0x00, 0x0}},
    {SEN_CMD_SETVD, 1, {0x00, 0x0}},
    {SEN_CMD_PRESET, 1, {0x00, 0x0}},     
    {0x3000, 1, {0x00, 0x0}}, /* standby cancel */
    {SEN_CMD_DELAY, 1, { 20, 0x0}},   
    {0x3002, 1, {0x00, 0x0}},
 
};

static CTL_SEN_CMD imx385_mode_2[] = {        
    {0x3003, 1, {0x01, 0x0}}, // software reset
		{SEN_CMD_DELAY, 1, { 1, 0x0}},	
		{0x3000, 1, {0x01, 0x0}},/* standby */
		{SEN_CMD_DELAY, 1, { 20, 0x0}},		
		{0x3002, 1, {0x01, 0x0}},
		{SEN_CMD_DELAY, 1, { 20, 0x0}},    
    {0x3009, 1, {0x01, 0x0}},//60/50frame
    {0x3012, 1, {0x2c, 0x0}},
    {0x3013, 1, {0x01, 0x0}},
    {0x3014, 1, {0x07, 0x0}}, //Gain
    {0x3018, 1, {0x65, 0x0}},
    {0x3019, 1, {0x04, 0x0}},
    {0x301b, 1, {0x98, 0x0}},
    {0x301c, 1, {0x08, 0x0}},
    {0x3020, 1, {0x00, 0x0}}, //SHS1
    {0x3021, 1, {0x00, 0x0}}, //SHS1
    {0x3049, 1, {0x0a, 0x0}},
    {0x3054, 1, {0x66, 0x0}},
    {0x305d, 1, {0x00, 0x0}},
    {0x305f, 1, {0x00, 0x0}},
    {0x310b, 1, {0x07, 0x0}},
    {0x3110, 1, {0x12, 0x0}},
    {0x31ed, 1, {0x38, 0x0}},
    {0x3338, 1, {0xd4, 0x0}},
    {0x3339, 1, {0x40, 0x0}},
    {0x333a, 1, {0x10, 0x0}},
    {0x333b, 1, {0x00, 0x0}},
    {0x333c, 1, {0xd4, 0x0}},
    {0x333d, 1, {0x40, 0x0}},
    {0x333e, 1, {0x10, 0x0}},
    {0x333f, 1, {0x00, 0x0}},
    {0x3344, 1, {0x00, 0x0}},//data_rate=445.5
    {0x336b, 1, {0x3f, 0x0}},
    {0x3380, 1, {0x20, 0x0}},
    {0x3381, 1, {0x25, 0x0}},
    {0x3382, 1, {0x67, 0x0}},
    {0x3383, 1, {0x1F, 0x0}},
    {0x3384, 1, {0x3f, 0x0}},
    {0x3385, 1, {0x27, 0x0}},
    {0x3386, 1, {0x1F, 0x0}},
    {0x3387, 1, {0x17, 0x0}},
    {0x3388, 1, {0x77, 0x0}},
    {0x338d, 1, {0xb4, 0x0}},
    {0x338e, 1, {0x01, 0x0}},    
    {SEN_CMD_DIRECTION, 1, {0x00, 0x0}},
    {SEN_CMD_SETVD, 1, {0x00, 0x0}},
    {SEN_CMD_PRESET, 1, {0x00, 0x0}},    
    {0x3000, 1, {0x00, 0x0}}, /* standby cancel */
    {SEN_CMD_DELAY, 1, { 20, 0x0}},    
    {0x3002, 1, {0x00, 0x0}},
 
};

static CTL_SEN_CMD imx385_mode_3[] = {
	{0x3003, 1, {0x01, 0x0}}, // software reset
    {SEN_CMD_DELAY, 1, { 1, 0x0}},
    {0x3000, 1, {0x01, 0x0}},	//STANDBY
    {0x3002, 1, {0x01, 0x0}},
    {0x3005, 1, {0x00, 0x0}},	//ADBIT
    {0x3009, 1, {0x01, 0x0}},	//HCG&FRSEL
    {0x300A, 1, {0x3C, 0x0}},
    {0x3012, 1, {0x2c, 0x0}},
    {0x3013, 1, {0x01, 0x0}},    
    {0x3044, 1, {0x01, 0x0}},	//OPORTSE&ODBIT
    {0x3054, 1, {0x66, 0x0}},	
    {0x305c, 1, {0x28, 0x0}},	//INCKSEL1,37.125MHz;
    {0x305d, 1, {0x00, 0x0}},	//INCKSEL2
    {0x305e, 1, {0x20, 0x0}}, //INCKSEL3
    {0x305f, 1, {0x00, 0x0}}, //INCKSEL4    
    {0x3109, 1, {0x01, 0x0}},
    {0x310b, 1, {0x07, 0x0}},
    {0x3110, 1, {0x12, 0x0}},
    {0x31ed, 1, {0x38, 0x0}},    
    //chid 05
    {0x3338, 1, {0xd4, 0x0}},   
    {0x3339, 1, {0x40, 0x0}},
    {0x333a, 1, {0x10, 0x0}},
    {0x333b, 1, {0x00, 0x0}},
    {0x333c, 1, {0xd4, 0x0}},
    {0x333d, 1, {0x40, 0x0}},
    {0x333e, 1, {0x10, 0x0}},
    {0x333f, 1, {0x00, 0x0}},    
    {0x3344, 1, {0x10, 0x0}},   
    {0x3346, 1, {0x03, 0x0}},
    {0x3353, 1, {0x00, 0x0}},//fix start pixel 0x0e -->0x0    
    {0x336b, 1, {0x37, 0x0}},
    {0x336c, 1, {0x1f, 0x0}},
    {0x337d, 1, {0x0A, 0x0}},
    {0x337e, 1, {0x0A, 0x0}},   
    {0x337f, 1, {0x03, 0x0}},
    {0x3380, 1, {0x20, 0x0}},	//INCK_FREQ1
    {0x3381, 1, {0x25, 0x0}},
    {0x3382, 1, {0x5f, 0x0}},
    {0x3383, 1, {0x1F, 0x0}},
    {0x3384, 1, {0x37, 0x0}},
    {0x3385, 1, {0x1F, 0x0}},
    {0x3386, 1, {0x1F, 0x0}},
    {0x3387, 1, {0x17, 0x0}},
    {0x3388, 1, {0x67, 0x0}},
    {0x3389, 1, {0x27, 0x0}},
    {0x338d, 1, {0xb4, 0x0}},
    {0x338e, 1, {0x01, 0x0}},    
    //DOL2F RAW10,25fps
    //BRL:1097
    {0x3007, 1, {0x10, 0x0}},	//WINMODE 
    {0x300C, 1, {0x11, 0x0}}, //DOL 2 mode 
    {0x3018, 1, {0x46, 0x0}},	//VMAX
    {0x3019, 1, {0x05, 0x0}},	//VMAX
    {0x301a, 1, {0x00, 0x0}}, //VMAX
    {0x301b, 1, {0x98, 0x0}},	//HMAX
    {0x301c, 1, {0x08, 0x0}},	//HMAX     
    {0x3020, 1, {0x03, 0x0}},	//SHS1	3-(RHS1-2)
    {0x3021, 1, {0x00, 0x0}}, //SHS1
    {0x3022, 1, {0x00, 0x0}}, //SHS1    
    {0x302c, 1, {0x21, 0x0}},	//RHS1	(2n+5)-(VMAX*2-BRL*2-13) shutter time:30H; RHS1 = 1125*2-1097*2-13=43
    {0x302d, 1, {0x00, 0x0}}, //RHS1	(2n+5)-493
    {0x302e, 1, {0x00, 0x0}}, //RHS1    
    {0x3023, 1, {0xe9, 0x0}}, //SHS2	 (RHS1+3)-(FSC-2), Shutter time:480H
    {0x3024, 1, {0x06, 0x0}}, //SHS2
    {0x302C, 1, {0x00, 0x0}}, //SHS2    
    {0x3043, 1, {0x05, 0x0}},
    {0x3108, 1, {0x11, 0x0}},	//XVS&XHS subsampling
    {0x3109, 1, {0x01, 0x0}},	//XVS&XHS subsampling
    {0x310a, 1, {0x00, 0x0}},    
    {0x3354, 1, {0x00, 0x0}},
    {0x3357, 1, {0xd2, 0x0}},	//PIC_SIZE_V = (1113 + (RHS1-1)/2) X 2 
    {0x3358, 1, {0x08, 0x0}},
    
    {SEN_CMD_SETVD, 1, {0x00, 0x0}},    
    {0x3000, 1, {0x00, 0x0}},	//standby cancel
    {SEN_CMD_DELAY, 1, { 20, 0x0}},
    {0x3002, 1, {0x00, 0x0}},	//Master start
    {0x3049, 1, {0x0a, 0x0}},	//XVS/XHS output
    {SEN_CMD_DELAY, 1, { 20, 0x0}},

};

static CTL_SEN_CMD imx385_mode_4[] = {
	{0x3003, 1, {0x01, 0x0}}, // software reset
    {SEN_CMD_DELAY, 1, { 1, 0x0}},
    {0x3000, 1, {0x01, 0x0}}, /* standby */
    {SEN_CMD_DELAY, 1, { 20, 0x0}},
    {0x3002, 1, {0x01, 0x0}},
    {0x3009, 1, {0x02, 0x0}},
    {0x3012, 1, {0x2C, 0x0}},
    {0x3013, 1, {0x01, 0x0}},
    {0x301b, 1, {0x30, 0x0}},
    {0x301c, 1, {0x11, 0x0}},
    {0x3044, 1, {0xE1, 0x0}},
    {0x3049, 1, {0x0A, 0x0}},
    {0x305d, 1, {0x00, 0x0}},
    {0x305f, 1, {0x00, 0x0}},
    {0x310b, 1, {0x07, 0x0}},
    {0x3110, 1, {0x12, 0x0}},
    {0x31ed, 1, {0x38, 0x0}},
    {0x3338, 1, {0xD4, 0x0}},
    {0x3339, 1, {0x40, 0x0}},
    {0x333a, 1, {0x10, 0x0}},
    {0x333b, 1, {0x00, 0x0}},
    {0x333c, 1, {0xD4, 0x0}},
    {0x333d, 1, {0x40, 0x0}},
    {0x333e, 1, {0x10, 0x0}},
    {0x333f, 1, {0x00, 0x0}},
    {0x3000, 1, {0x00, 0x0}}, /* standby cancel */
    {SEN_CMD_DELAY, 1, { 20, 0x0}},
    {0x3002, 1, {0x00, 0x0}},
};

static CTL_SEN_CMD imx385_mode_5[] = {
	{0x3003, 1, {0x01, 0x0}}, // software reset
    {SEN_CMD_DELAY, 1, { 1, 0x0}},
    {0x3000, 1, {0x01, 0x0}},  //STANDBY
    {SEN_CMD_DELAY, 1, { 20, 0x0}},
    {0x3002, 1, {0x01, 0x0}},
    {0x3012, 1, {0x2C, 0x0}},
    {0x3013, 1, {0x01, 0x0}},
    {0x3044, 1, {0xE1, 0x0}},
    {0x3049, 1, {0x0A, 0x0}},
    {0x305D, 1, {0x00, 0x0}},
    {0x305F, 1, {0x00, 0x0}},
    {0x310B, 1, {0x07, 0x0}},
    {0x3110, 1, {0x12, 0x0}},
    {0x31ED, 1, {0x38, 0x0}},
    {0x3338, 1, {0xD4, 0x0}},
    {0x3339, 1, {0x40, 0x0}},
    {0x333A, 1, {0x10, 0x0}},
    {0x333B, 1, {0x00, 0x0}},
    {0x333C, 1, {0xD4, 0x0}},
    {0x333D, 1, {0x40, 0x0}},
    {0x333E, 1, {0x10, 0x0}},
    {0x333F, 1, {0x00, 0x0}},
    {0x3000, 1, {0x00, 0x0}}, /* standby cancel */
    {SEN_CMD_DELAY, 1, { 20, 0x0}},
    {0x3002, 1, {0x00, 0x0}},
};

static CTL_SEN_CMD imx385_mode_6[] = {
	{0x3003, 1, {0x01, 0x0}}, // software reset
    {SEN_CMD_DELAY, 1, { 1, 0x0}},
    {0x3000, 1, {0x01, 0x0}},  //STANDBY
    {SEN_CMD_DELAY, 1, { 20, 0x0}},
    {0x3002, 1, {0x01, 0x0}},  //XMSTA
    {0x3007, 1, {0x10, 0x0}},
    {0x300C, 1, {0x11, 0x0}},
    {0x3012, 1, {0x2C, 0x0}},
    {0x3013, 1, {0x01, 0x0}},
    {0x3020, 1, {0x03, 0x0}}, //SHS1
    {0x3021, 1, {0x00, 0x0}}, //SHS1
    {0x3022, 1, {0x00, 0x0}}, //SHS1
    {0x3023, 1, {0xe9, 0x0}}, //SHS2
    {0x3024, 1, {0x03, 0x0}}, //SHS2
    {0x3025, 1, {0x00, 0x0}}, //SHS2
    {0x302c, 1, {0x2b, 0x0}}, //RHS1
    {0x302d, 1, {0x00, 0x0}}, //RHS1
    {0x302e, 1, {0x00, 0x0}}, //RHS1
    {0x3043, 1, {0x07, 0x0}},
    {0x3044, 1, {0xE1, 0x0}},
    {0x3049, 1, {0x0A, 0x0}},
    {0x305D, 1, {0x00, 0x0}},
    {0x305F, 1, {0x00, 0x0}},
    {0x3109, 1, {0x01, 0x0}},
    {0x310B, 1, {0x07, 0x0}},
    {0x3110, 1, {0x12, 0x0}},
    {0x31ED, 1, {0x38, 0x0}},
    {0x3338, 1, {0xD4, 0x0}},
    {0x3339, 1, {0x40, 0x0}},
    {0x333A, 1, {0x10, 0x0}},
    {0x333B, 1, {0x00, 0x0}},
    {0x333C, 1, {0xD4, 0x0}},
    {0x333D, 1, {0x40, 0x0}},
    {0x333E, 1, {0x10, 0x0}},
    {0x333F, 1, {0x00, 0x0}},
    {0x3000, 1, {0x00, 0x0}}, /* standby cancel */
    {SEN_CMD_DELAY, 1, { 20, 0x0}},
    {0x3002, 1, {0x00, 0x0}},
};

typedef struct gain_wdr_set {
	UINT32 a_gain;
	UINT32 total_gain;
} gain_wdr_set_t;

// A Gain 0~3981X
static const gain_wdr_set_t gain_wdr_table[] = {	
    {0x0  , 1000   }, {0x1  , 1012   }, {0x2  , 1023   }, {0x3  , 1035   }, {0x4  , 1047   }, {0x5  , 1059   },
    {0x6  , 1072   }, {0x7  , 1084   }, {0x8  , 1096   }, {0x9  , 1109   }, {0xA  , 1122   }, {0xB  , 1135   },
    {0xC  , 1148   }, {0xD  , 1161   }, {0xE  , 1175   }, {0xF  , 1189   }, {0x10 , 1202   }, {0x11 , 1216   },
    {0x12 , 1230   }, {0x13 , 1245   }, {0x14 , 1259   }, {0x15 , 1274   }, {0x16 , 1288   }, {0x17 , 1303   },
    {0x18 , 1318   }, {0x19 , 1334   }, {0x1A , 1349   }, {0x1B , 1365   }, {0x1C , 1380   }, {0x1D , 1396   },
    {0x1E , 1413   }, {0x1F , 1429   }, {0x20 , 1445   }, {0x21 , 1462   }, {0x22 , 1479   }, {0x23 , 1496   },
    {0x24 , 1514   }, {0x25 , 1531   }, {0x26 , 1549   }, {0x27 , 1567   }, {0x28 , 1585   }, {0x29 , 1603   },
    {0x2A , 1622   }, {0x2B , 1641   }, {0x2C , 1660   }, {0x2D , 1679   }, {0x2E , 1698   }, {0x2F , 1718   },
    {0x30 , 1738   }, {0x31 , 1758   }, {0x32 , 1778   }, {0x33 , 1799   }, {0x34 , 1820   }, {0x35 , 1841   },
    {0x36 , 1862   }, {0x37 , 1884   }, {0x38 , 1905   }, {0x39 , 1928   }, {0x3A , 1950   }, {0x3B , 1972   },
    {0x3C , 1995   }, {0x3D , 2018   }, {0x3E , 2042   }, {0x3F , 2065   }, {0x40 , 2089   }, {0x41 , 2113   },
    {0x42 , 2138   }, {0x43 , 2163   }, {0x44 , 2188   }, {0x45 , 2213   }, {0x46 , 2239   }, {0x47 , 2265   },
    {0x48 , 2291   }, {0x49 , 2317   }, {0x4A , 2344   }, {0x4B , 2371   }, {0x4C , 2399   }, {0x4D , 2427   },
    {0x4E , 2455   }, {0x4F , 2483   }, {0x50 , 2512   }, {0x51 , 2541   }, {0x52 , 2570   }, {0x53 , 2600   },
    {0x54 , 2630   }, {0x55 , 2661   }, {0x56 , 2692   }, {0x57 , 2723   }, {0x58 , 2754   }, {0x59 , 2786   },
    {0x5A , 2818   }, {0x5B , 2851   }, {0x5C , 2884   }, {0x5D , 2917   }, {0x5E , 2951   }, {0x5F , 2985   },
    {0x60 , 3020   }, {0x61 , 3055   }, {0x62 , 3090   }, {0x63 , 3126   }, {0x64 , 3162   }, {0x65 , 3199   },
    {0x66 , 3236   }, {0x67 , 3273   }, {0x68 , 3311   }, {0x69 , 3350   }, {0x6A , 3388   }, {0x6B , 3428   },
    {0x6C , 3467   }, {0x6D , 3508   }, {0x6E , 3548   }, {0x6F , 3589   }, {0x70 , 3631   }, {0x71 , 3673   },
    {0x72 , 3715   }, {0x73 , 3758   }, {0x74 , 3802   }, {0x75 , 3846   }, {0x76 , 3890   }, {0x77 , 3936   },
    {0x78 , 3981   }, {0x79 , 4027   }, {0x7A , 4074   }, {0x7B , 4121   }, {0x7C , 4169   }, {0x7D , 4217   },
    {0x7E , 4266   }, {0x7F , 4315   }, {0x80 , 4365   }, {0x81 , 4416   }, {0x82 , 4467   }, {0x83 , 4519   },
    {0x84 , 4571   }, {0x85 , 4624   }, {0x86 , 4677   }, {0x87 , 4732   }, {0x88 , 4786   }, {0x89 , 4842   },
    {0x8A , 4898   }, {0x8B , 4955   }, {0x8C , 5012   }, {0x8D , 5070   }, {0x8E , 5129   }, {0x8F , 5188   },
    {0x90 , 5248   }, {0x91 , 5309   }, {0x92 , 5370   }, {0x93 , 5433   }, {0x94 , 5495   }, {0x95 , 5559   },
    {0x96 , 5623   }, {0x97 , 5689   }, {0x98 , 5754   }, {0x99 , 5821   }, {0x9A , 5888   }, {0x9B , 5957   },
    {0x9C , 6026   }, {0x9D , 6095   }, {0x9E , 6166   }, {0x9F , 6237   }, {0xA0 , 6310   }, {0xA1 , 6383   },
    {0xA2 , 6457   }, {0xA3 , 6531   }, {0xA4 , 6607   }, {0xA5 , 6683   }, {0xA6 , 6761   }, {0xA7 , 6839   },
    {0xA8 , 6918   }, {0xA9 , 6998   }, {0xAA , 7079   }, {0xAB , 7161   }, {0xAC , 7244   }, {0xAD , 7328   },
    {0xAE , 7413   }, {0xAF , 7499   }, {0xB0 , 7586   }, {0xB1 , 7674   }, {0xB2 , 7762   }, {0xB3 , 7852   },
    {0xB4 , 7943   }, {0xB5 , 8035   }, {0xB6 , 8128   }, {0xB7 , 8222   }, {0xB8 , 8318   }, {0xB9 , 8414   },
    {0xBA , 8511   }, {0xBB , 8610   }, {0xBC , 8710   }, {0xBD , 8810   }, {0xBE , 8913   }, {0xBF , 9016   },
    {0xC0 , 9120   }, {0xC1 , 9226   }, {0xC2 , 9333   }, {0xC3 , 9441   }, {0xC4 , 9550   }, {0xC5 , 9661   },
    {0xC6 , 9772   }, {0xC7 , 9886   }, {0xC8 , 10000  }, {0xC9 , 10116  }, {0xCA , 10233  }, {0xCB , 10351  },
    {0xCC , 10471  }, {0xCD , 10593  }, {0xCE , 10715  }, {0xCF , 10839  }, {0xD0 , 10965  }, {0xD1 , 11092  },
    {0xD2 , 11220  }, {0xD3 , 11350  }, {0xD4 , 11482  }, {0xD5 , 11614  }, {0xD6 , 11749  }, {0xD7 , 11885  },
    {0xD8 , 12023  }, {0xD9 , 12162  }, {0xDA , 12303  }, {0xDB , 12445  }, {0xDC , 12589  }, {0xDD , 12735  },
    {0xDE , 12882  }, {0xDF , 13032  }, {0xE0 , 13183  }, {0xE1 , 13335  }, {0xE2 , 13490  }, {0xE3 , 13646  },
    {0xE4 , 13804  }, {0xE5 , 13964  }, {0xE6 , 14125  }, {0xE7 , 14289  }, {0xE8 , 14454  }, {0xE9 , 14622  },
    {0xEA , 14791  }, {0xEB , 14962  }, {0xEC , 15136  }, {0xED , 15311  }, {0xEE , 15488  }, {0xEF , 15668  },
    {0xF0 , 15849  }, {0xF1 , 16032  }, {0xF2 , 16218  }, {0xF3 , 16406  }, {0xF4 , 16596  }, {0xF5 , 16788  },
    {0xF6 , 16982  }, {0xF7 , 17179  }, {0xF8 , 17378  }, {0xF9 , 17579  }, {0xFA , 17783  }, {0xFB , 17989  },
    {0xFC , 18197  }, {0xFD , 18408  }, {0xFE , 18621  }, {0xFF , 18836  }, {0x100, 19055  }, {0x101, 19275  },
    {0x102, 19498  }, {0x103, 19724  }, {0x104, 19953  }, {0x105, 20184  }, {0x106, 20417  }, {0x107, 20654  },
    {0x108, 20893  }, {0x109, 21135  }, {0x10A, 21380  }, {0x10B, 21627  }, {0x10C, 21878  }, {0x10D, 22131  },
    {0x10E, 22387  }, {0x10F, 22646  }, {0x110, 22909  }, {0x111, 23174  }, {0x112, 23442  }, {0x113, 23714  },
    {0x114, 23988  }, {0x115, 24266  }, {0x116, 24547  }, {0x117, 24831  }, {0x118, 25119  }, {0x119, 25410  },
    {0x11A, 25704  }, {0x11B, 26002  }, {0x11C, 26303  }, {0x11D, 26607  }, {0x11E, 26915  }, {0x11F, 27227  },
    {0x120, 27542  }, {0x121, 27861  }, {0x122, 28184  }, {0x123, 28510  }, {0x124, 28840  }, {0x125, 29174  },
    {0x126, 29512  }, {0x127, 29854  }, {0x128, 30200  }, {0x129, 30549  }, {0x12A, 30903  }, {0x12B, 31261  },
    {0x12C, 31623  }, {0x12D, 31989  }, {0x12E, 32359  }, {0x12F, 32734  }, {0x130, 33113  }, {0x131, 33497  },
    {0x132, 33884  }, {0x133, 34277  }, {0x134, 34674  }, {0x135, 35075  }, {0x136, 35481  }, {0x137, 35892  },
    {0x138, 36308  }, {0x139, 36728  }, {0x13A, 37154  }, {0x13B, 37584  }, {0x13C, 38019  }, {0x13D, 38459  },
    {0x13E, 38905  }, {0x13F, 39355  }, {0x140, 39811  }, {0x141, 40272  }, {0x142, 40738  }, {0x143, 41210  },
    {0x144, 41687  }, {0x145, 42170  }, {0x146, 42658  }, {0x147, 43152  }, {0x148, 43652  }, {0x149, 44157  },
    {0x14A, 44668  }, {0x14B, 45186  }, {0x14C, 45709  }, {0x14D, 46238  }, {0x14E, 46774  }, {0x14F, 47315  },
    {0x150, 47863  }, {0x151, 48417  }, {0x152, 48978  }, {0x153, 49545  }, {0x154, 50119  }, {0x155, 50699  },
    {0x156, 51286  }, {0x157, 51880  }, {0x158, 52481  }, {0x159, 53088  }, {0x15A, 53703  }, {0x15B, 54325  },
    {0x15C, 54954  }, {0x15D, 55590  }, {0x15E, 56234  }, {0x15F, 56885  }, {0x160, 57544  }, {0x161, 58210  },
    {0x162, 58884  }, {0x163, 59566  }, {0x164, 60256  }, {0x165, 60954  }, {0x166, 61660  }, {0x167, 62373  },
    {0x168, 63096  }, {0x169, 63826  }, {0x16A, 64565  }, {0x16B, 65313  }, {0x16C, 66069  }, {0x16D, 66834  },
    {0x16E, 67608  }, {0x16F, 68391  }, {0x170, 69183  }, {0x171, 69984  }, {0x172, 70795  }, {0x173, 71614  },
    {0x174, 72444  }, {0x175, 73282  }, {0x176, 74131  }, {0x177, 74989  }, {0x178, 75858  }, {0x179, 76736  },
    {0x17A, 77625  }, {0x17B, 78524  }, {0x17C, 79433  }, {0x17D, 80353  }, {0x17E, 81283  }, {0x17F, 82224  },
    {0x180, 83176  }, {0x181, 84140  }, {0x182, 85114  }, {0x183, 86099  }, {0x184, 87096  }, {0x185, 88105  },
    {0x186, 89125  }, {0x187, 90157  }, {0x188, 91201  }, {0x189, 92257  }, {0x18A, 93325  }, {0x18B, 94406  },
    {0x18C, 95499  }, {0x18D, 96605  }, {0x18E, 97724  }, {0x18F, 98855  }, {0x190, 100000 }, {0x191, 101158 },
    {0x192, 102329 }, {0x193, 103514 }, {0x194, 104713 }, {0x195, 105925 }, {0x196, 107152 }, {0x197, 108393 },
    {0x198, 109648 }, {0x199, 110917 }, {0x19A, 112202 }, {0x19B, 113501 }, {0x19C, 114815 }, {0x19D, 116145 },
    {0x19E, 117490 }, {0x19F, 118850 }, {0x1A0, 120226 }, {0x1A1, 121619 }, {0x1A2, 123027 }, {0x1A3, 124451 },
    {0x1A4, 125893 }, {0x1A5, 127350 }, {0x1A6, 128825 }, {0x1A7, 130317 }, {0x1A8, 131826 }, {0x1A9, 133352 },
    {0x1AA, 134896 }, {0x1AB, 136458 }, {0x1AC, 138038 }, {0x1AD, 139637 }, {0x1AE, 141254 }, {0x1AF, 142889 },
    {0x1B0, 144544 }, {0x1B1, 146218 }, {0x1B2, 147911 }, {0x1B3, 149624 }, {0x1B4, 151356 }, {0x1B5, 153109 },
    {0x1B6, 154882 }, {0x1B7, 156675 }, {0x1B8, 158489 }, {0x1B9, 160325 }, {0x1BA, 162181 }, {0x1BB, 164059 },
    {0x1BC, 165959 }, {0x1BD, 167880 }, {0x1BE, 169824 }, {0x1BF, 171791 }, {0x1C0, 173780 }, {0x1C1, 175792 },
    {0x1C2, 177828 }, {0x1C3, 179887 }, {0x1C4, 181970 }, {0x1C5, 184077 }, {0x1C6, 186209 }, {0x1C7, 188365 },
    {0x1C8, 190546 }, {0x1C9, 192752 }, {0x1CA, 194984 }, {0x1CB, 197242 }, {0x1CC, 199526 }, {0x1CD, 201837 },
    {0x1CE, 204174 }, {0x1CF, 206538 }, {0x1D0, 208930 }, {0x1D1, 211349 }, {0x1D2, 213796 }, {0x1D3, 216272 },
    {0x1D4, 218776 }, {0x1D5, 221309 }, {0x1D6, 223872 }, {0x1D7, 226464 }, {0x1D8, 229087 }, {0x1D9, 231739 },
    {0x1DA, 234423 }, {0x1DB, 237137 }, {0x1DC, 239883 }, {0x1DD, 242661 }, {0x1DE, 245471 }, {0x1DF, 248313 },
    {0x1E0, 251189 }, {0x1E1, 254097 }, {0x1E2, 257040 }, {0x1E3, 260016 }, {0x1E4, 263027 }, {0x1E5, 266073 },
    {0x1E6, 269153 }, {0x1E7, 272270 }, {0x1E8, 275423 }, {0x1E9, 278612 }, {0x1EA, 281838 }, {0x1EB, 285102 },
    {0x1EC, 288403 }, {0x1ED, 291743 }, {0x1EE, 295121 }, {0x1EF, 298538 }, {0x1F0, 301995 }, {0x1F1, 305492 },
    {0x1F2, 309030 }, {0x1F3, 312608 }, {0x1F4, 316228 }, {0x1F5, 319890 }, {0x1F6, 323594 }, {0x1F7, 327341 },
    {0x1F8, 331131 }, {0x1F9, 334965 }, {0x1FA, 338844 }, {0x1FB, 342768 }, {0x1FC, 346737 }, {0x1FD, 350752 },
    {0x1FE, 354813 }, {0x1FF, 358922 }, {0x200, 363078 }, {0x201, 367282 }, {0x202, 371535 }, {0x203, 375837 },
    {0x204, 380189 }, {0x205, 384592 }, {0x206, 389045 }, {0x207, 393550 }, {0x208, 398107 }, {0x209, 402717 },
    {0x20A, 407380 }, {0x20B, 412098 }, {0x20C, 416869 }, {0x20D, 421697 }, {0x20E, 426580 }, {0x20F, 431519 },
    {0x210, 436516 }, {0x211, 441570 }, {0x212, 446684 }, {0x213, 451856 }, {0x214, 457088 }, {0x215, 462381 },
    {0x216, 467735 }, {0x217, 473151 }, {0x218, 478630 }, {0x219, 484172 }, {0x21A, 489779 }, {0x21B, 495450 },
    {0x21C, 501187 }, {0x21D, 506991 }, {0x21E, 512861 }, {0x21F, 518800 }, {0x220, 524807 }, {0x221, 530884 },
    {0x222, 537032 }, {0x223, 543250 }, {0x224, 549541 }, {0x225, 555904 }, {0x226, 562341 }, {0x227, 568853 },
    {0x228, 575440 }, {0x229, 582103 }, {0x22A, 588844 }, {0x22B, 595662 }, {0x22C, 602560 }, {0x22D, 609537 },
    {0x22E, 616595 }, {0x22F, 623735 }, {0x230, 630957 }, {0x231, 638263 }, {0x232, 645654 }, {0x233, 653131 },
    {0x234, 660693 }, {0x235, 668344 }, {0x236, 676083 }, {0x237, 683912 }, {0x238, 691831 }, {0x239, 699842 },
    {0x23A, 707946 }, {0x23B, 716143 }, {0x23C, 724436 }, {0x23D, 732825 }, {0x23E, 741310 }, {0x23F, 749894 },
    {0x240, 758578 }, {0x241, 767361 }, {0x242, 776247 }, {0x243, 785236 }, {0x244, 794328 }, {0x245, 803526 },
    {0x246, 812831 }, {0x247, 822243 }, {0x248, 831764 }, {0x249, 841395 }, {0x24A, 851138 }, {0x24B, 860994 },
    {0x24C, 870964 }, {0x24D, 881049 }, {0x24E, 891251 }, {0x24F, 901571 }, {0x250, 912011 }, {0x251, 922571 },
    {0x252, 933254 }, {0x253, 944061 }, {0x254, 954993 }, {0x255, 966051 }, {0x256, 977237 }, {0x257, 988553 },
    {0x258, 1000000}, {0x259, 1011579}, {0x25A, 1023293}, {0x25B, 1035142}, {0x25C, 1047129}, {0x25D, 1059254},
    {0x25E, 1071519}, {0x25F, 1083927}, {0x260, 1096478}, {0x261, 1109175}, {0x262, 1122018}, {0x263, 1135011},
    {0x264, 1148154}, {0x265, 1161449}, {0x266, 1174898}, {0x267, 1188502}, {0x268, 1202264}, {0x269, 1216186},
    {0x26A, 1230269}, {0x26B, 1244515}, {0x26C, 1258925}, {0x26D, 1273503}, {0x26E, 1288250}, {0x26F, 1303167},
    {0x270, 1318257}, {0x271, 1333521}, {0x272, 1348963}, {0x273, 1364583}, {0x274, 1380384}, {0x275, 1396368},
    {0x276, 1412538}, {0x277, 1428894}, {0x278, 1445440}, {0x279, 1462177}, {0x27A, 1479108}, {0x27B, 1496236},
    {0x27C, 1513561}, {0x27D, 1531087}, {0x27E, 1548817}, {0x27F, 1566751}, {0x280, 1584893}, {0x281, 1603245},
    {0x282, 1621810}, {0x283, 1640590}, {0x284, 1659587}, {0x285, 1678804}, {0x286, 1698244}, {0x287, 1717908},
    {0x288, 1737801}, {0x289, 1757924}, {0x28A, 1778279}, {0x28B, 1798871}, {0x28C, 1819701}, {0x28D, 1840772},
    {0x28E, 1862087}, {0x28F, 1883649}, {0x290, 1905461}, {0x291, 1927525}, {0x292, 1949845}, {0x293, 1972423},
    {0x294, 1995262}, {0x295, 2018366}, {0x296, 2041738}, {0x297, 2065380}, {0x298, 2089296}, {0x299, 2113489},
    {0x29A, 2137962}, {0x29B, 2162719}, {0x29C, 2187762}, {0x29D, 2213095}, {0x29E, 2238721}, {0x29F, 2264644},
    {0x2A0, 2290868}, {0x2A1, 2317395}, {0x2A2, 2344229}, {0x2A3, 2371374}, {0x2A4, 2398833}, {0x2A5, 2426610},
    {0x2A6, 2454709}, {0x2A7, 2483133}, {0x2A8, 2511886}, {0x2A9, 2540973}, {0x2AA, 2570396}, {0x2AB, 2600160},
    {0x2AC, 2630268}, {0x2AD, 2660725}, {0x2AE, 2691535}, {0x2AF, 2722701}, {0x2B0, 2754229}, {0x2B1, 2786121},
    {0x2B2, 2818383}, {0x2B3, 2851018}, {0x2B4, 2884032}, {0x2B5, 2917427}, {0x2B6, 2951209}, {0x2B7, 2985383},
    {0x2B8, 3019952}, {0x2B9, 3054921}, {0x2BA, 3090295}, {0x2BB, 3126079}, {0x2BC, 3162278}, {0x2BD, 3198895},
    {0x2BE, 3235937}, {0x2BF, 3273407}, {0x2C0, 3311311}, {0x2C1, 3349654}, {0x2C2, 3388442}, {0x2C3, 3427678},
    {0x2C4, 3467369}, {0x2C5, 3507519}, {0x2C6, 3548134}, {0x2C7, 3589219}, {0x2C8, 3630781}, {0x2C9, 3672823},
    {0x2CA, 3715352}, {0x2CB, 3758374}, {0x2CC, 3801894}, {0x2CD, 3845918}, {0x2CE, 3890451}, {0x2CF, 3935501},
    {0x2D0, 3981072},
};
#define NUM_OF_WDRGAINSET (sizeof(gain_wdr_table) / sizeof(gain_wdr_set_t))

static UINT32 cur_sen_mode[CTL_SEN_ID_MAX] = {CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1};
static UINT32 cur_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 chgmode_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 power_ctrl_mclk[CTL_SEN_CLK_SEL_MAX] = {0};
static UINT32 reset_ctrl_count[CTL_SEN_ID_MAX] = {0};
static UINT32 pwdn_ctrl_count[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_CTRL sensor_ctrl_last[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_PRESET_CTRL preset_ctrl[CTL_SEN_ID_MAX] = {0};
//static UINT32 compensation_ratio[CTL_SEN_ID_MAX][ISP_SEN_MFRAME_MAX_NUM] = {0};
static INT32 is_fastboot[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_id[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_addr[CTL_SEN_ID_MAX];
static BOOL i2c_valid[CTL_SEN_ID_MAX];

static CTL_SEN_DRV_TAB imx385_sen_drv_tab = {
	sen_open_imx385,
	sen_close_imx385,
	sen_sleep_imx385,
	sen_wakeup_imx385,
	sen_write_reg_imx385,
	sen_read_reg_imx385,
	sen_chg_mode_imx385,
	sen_chg_fps_imx385,
	sen_set_info_imx385,
	sen_get_info_imx385,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_imx385(void)
{
	return &imx385_sen_drv_tab;
}

static void sen_pwr_ctrl_imx385(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
{
	UINT32 i = 0;
	UINT32 reset_count = 0, pwdn_count = 0;	
	DBG_IND("enter flag %d \r\n", flag);

	if ((flag == CTL_SEN_PWR_CTRL_TURN_ON) && ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr))) {
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
			for ( i = 0; i < CTL_SEN_ID_MAX ; i++ ) {
				if ( reset_ctrl_count[i] == (sen_power[id].rst_pin)) {
					reset_count++;
				}
			}
			reset_ctrl_count[id] = (sen_power[id].rst_pin);		

			if (!reset_count) {						
				gpio_direction_output(sen_power[id].rst_pin, 0);
				gpio_set_value(sen_power[id].rst_pin, 0);
				gpio_set_value(sen_power[id].rst_pin, 1);
				gpio_set_value(sen_power[id].rst_pin, 0);
				vos_util_delay_ms(sen_power[id].rst_time);
				gpio_set_value(sen_power[id].rst_pin, 1);
				vos_util_delay_ms(sen_power[id].stable_time);
			}
		}

		if (sen_power[id].pwdn_pin != CTL_SEN_IGNORE) {
			for ( i = 0; i < CTL_SEN_ID_MAX ; i++ ) {
				if ( pwdn_ctrl_count[i] == (sen_power[id].pwdn_pin)) {
					pwdn_count++;
				}
			}
			pwdn_ctrl_count[id] = (sen_power[id].pwdn_pin);			

			if (!pwdn_count) {
				gpio_direction_output((sen_power[id].pwdn_pin), 0);
				gpio_set_value((sen_power[id].pwdn_pin), 1);
				gpio_set_value((sen_power[id].pwdn_pin), 0);

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
				gpio_set_value((sen_power[id].pwdn_pin), 1);
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

static CTL_SEN_CMD sen_set_cmd_info_imx385(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;
	
	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_imx385(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_imx385(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_imx385);
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

static ER sen_close_imx385(CTL_SEN_ID id)
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

static ER sen_sleep_imx385(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");

	return E_OK;
}

static ER sen_wakeup_imx385(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");

	return E_OK;
}

static ER sen_write_reg_imx385(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

	if (!i2c_valid[id]) {
		return E_NOSPT;
	}

	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		i = 0;
		while(1){
			if (sen_i2c_transfer(id, &msgs, 1) == 0)
				break;
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

static ER sen_read_reg_imx385(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

static UINT32 sen_get_cmd_tab_imx385(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = imx385_mode_1;
		return sizeof(imx385_mode_1) / sizeof(CTL_SEN_CMD);
	case CTL_SEN_MODE_2:
		*cmd_tab = imx385_mode_2;
		return sizeof(imx385_mode_2) / sizeof(CTL_SEN_CMD);
	case CTL_SEN_MODE_3:
		*cmd_tab = imx385_mode_3;
		return sizeof(imx385_mode_3) / sizeof(CTL_SEN_CMD);
	case CTL_SEN_MODE_4:
		*cmd_tab = imx385_mode_4;
		return sizeof(imx385_mode_4) / sizeof(CTL_SEN_CMD);
	case CTL_SEN_MODE_5:
		*cmd_tab = imx385_mode_5;
		return sizeof(imx385_mode_5) / sizeof(CTL_SEN_CMD);
	case CTL_SEN_MODE_6:
		*cmd_tab = imx385_mode_6;
		return sizeof(imx385_mode_6) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_imx385(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
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
		sen_set_chgmode_fps_imx385(id, isp_builtin_get_chgmode_fps(id));
		sen_set_cur_fps_imx385(id, isp_builtin_get_chgmode_fps(id));
		sen_set_gain_imx385(id, &sensor_ctrl);
		sen_set_expt_imx385(id, &sensor_ctrl);
		#endif
		preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

		return E_OK;
	}

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_imx385(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_imx385(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_imx385(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_imx385(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_imx385(0x3018, 1, sensor_vd & 0xFF, 0x00);
			rt |= sen_write_reg_imx385(id, &cmd);
			cmd = sen_set_cmd_info_imx385(0x3019, 1, (sensor_vd >> 8) & 0xFF, 0x00);
			rt |= sen_write_reg_imx385(id, &cmd);
			cmd = sen_set_cmd_info_imx385(0x301A, 1, (sensor_vd >> 16) & 0x03, 0x00);
			rt |= sen_write_reg_imx385(id, &cmd);
			DBG_ERR("SEN_CMD_SETVD=%d \r\n", sensor_vd);
		} else if (p_cmd_list[idx].addr == SEN_CMD_PRESET) {
			switch (preset_ctrl[id].mode) {
				default:
				case ISP_SENSOR_PRESET_DEFAULT:
						DBG_MSG("using default expt/gain \r\n");
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
			sen_set_gain_imx385(id, &sensor_ctrl);
			sen_set_expt_imx385(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_imx385(id, &flip);
			//DBG_ERR("mirror_flip\r\n");
		} else {
			cmd = sen_set_cmd_info_imx385(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_imx385(id, &cmd);//init table  
			//DBG_ERR("write addr=0x%x, data0=0x%x, data1=0x%x\r\n", p_cmd_list[idx].addr, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);		 
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_imx385(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_imx385(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_imx385(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_imx385(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}
	
	cmd = sen_set_cmd_info_imx385(0x3018, 1, sensor_vd & 0xFF, 0x00);
	rt |= sen_write_reg_imx385(id, &cmd);
	cmd = sen_set_cmd_info_imx385(0x3019, 1, (sensor_vd >> 8) & 0xFF, 0x00);
	rt |= sen_write_reg_imx385(id, &cmd);
	cmd = sen_set_cmd_info_imx385(0x301A, 1, (sensor_vd >> 16) & 0x03, 0x00);
	rt |= sen_write_reg_imx385(id, &cmd);
	
	return rt;
}

static ER sen_set_info_imx385(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_imx385(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_imx385(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_imx385(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_imx385(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_imx385(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_imx385(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_imx385(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_imx385((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_imx385((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_imx385(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_imx385((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_imx385(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_imx385(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_imx385((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_imx385((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_LVDS:
		sen_get_mode_lvds_imx385((CTL_SENDRV_GET_MODE_LVDS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_imx385((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_imx385(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_imx385(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_imx385(id, data);
		break;
	default:
		  rt = E_NOSPT;
	}
	return rt;
}

static UINT32 sen_calc_chgmode_vd_imx385(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;
	//DBG_ERR("sensor vd (%d) \r\n", sensor_vd);

	sen_set_chgmode_fps_imx385(id, fps);
	sen_set_cur_fps_imx385(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_imx385(id, fps);
		sen_set_cur_fps_imx385(id, fps);
	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_imx385(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_imx385(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_imx385(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);	
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;
	//DBG_ERR("sensor vd =%d \r\n", sensor_vd);

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

static void sen_set_gain_imx385(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	ER rt = E_OK;
	UINT32 a_gain = 0, i = 0;

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
		}

	// search most suitable gain into gain table
	for (i=0; i<NUM_OF_WDRGAINSET; i++) {
		if (gain_wdr_table[i].total_gain > sensor_ctrl->gain_ratio[0])
			break;
	}
	
	if(i != 0){
		a_gain = gain_wdr_table[i-1].a_gain;
		//Write Sensor reg
		cmd = sen_set_cmd_info_imx385(0x3014, 1, a_gain, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		cmd = sen_set_cmd_info_imx385(0x3015, 1, ((a_gain & 0x300) >> 8), 0);
		rt |= sen_write_reg_imx385(id, &cmd);		
	}

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_expt_imx385(CTL_SEN_ID id, void *param)
{   
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps = 0, cur_fps = 0, clac_fps = 0, t_row = 0, shs1 = 0, fsc = 0, rhs1 = 0, temp_max_short_line = 0, shs2 = 0;
	UINT32  real_short_exp = 0, y_out_size = 0;
	//UINT32 temp_line[ISP_SEN_MFRAME_MAX_NUM] = {0};	
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
		t_row = sen_calc_rowtime_imx385(id, cur_sen_mode[id]);
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
	chgmode_fps = sen_get_chgmode_fps_imx385(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_imx385(id, cur_sen_mode[id]);
	//DBG_ERR("t_row=%d\r\n", t_row);
	
	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		expt_time = (line[0] + line[1]) * t_row / 10;
	} else {
		expt_time = (line[0]) * t_row / 10;		
	}

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");		
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_imx385(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_imx385(id, cur_fps);

	//Check max vts
	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("max vts overflow\r\n");
		sensor_vd = MAX_VD_PERIOD;
	}
	
	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {		
		fsc = sensor_vd * 2;
		real_short_exp = line[1];
		rhs1 = (fsc - (BRL * 2)) - 13;
		temp_max_short_line = (100000000 / cur_fps ) * 10 / 17 / t_row;//1:16

		if (rhs1 >= fsc - (BRL * 2) - 13) {
			rhs1 = fsc - (BRL * 2) - 13;//43
		}
		
		if (0 != ((rhs1 - 5) % 2)){
			rhs1 = (rhs1 - 5) / 2 * 2 + 5;
		}
		
		if (real_short_exp > temp_max_short_line) {
			real_short_exp = temp_max_short_line;
		}
		
		if (rhs1 >= (real_short_exp + 1)) {
			shs1 = rhs1 - real_short_exp - 1;
		} else {
			shs1 = 2;
		}
		
		y_out_size = (1113 + (rhs1 - 1) / 2) * 2;
		
		if ((line[0]) > (((sensor_vd * 2) - (rhs1 + 2)) - 1)) {
		    shs2 = rhs1 + 2 ;//short
		} else {
		    shs2 = sensor_vd * 2 - line[0] - 1;
		}
		
		if (shs2 < (rhs1 + 2)) {
		    shs2 = rhs1 + 2;
		}

		if (shs2 > ((sensor_vd * 2) - 2)) {//long exp
		    shs2 = (sensor_vd * 2) - 2;
		}														
	}		

	//Write change mode VD	
	//Set vmax to sensor
	cmd = sen_set_cmd_info_imx385(0x3018, 1, sensor_vd & 0xFF, 0);
	rt |= sen_write_reg_imx385(id, &cmd);
	cmd = sen_set_cmd_info_imx385(0x3019, 1, (sensor_vd >> 8) & 0xFF, 0);
	rt |= sen_write_reg_imx385(id, &cmd);
	cmd = sen_set_cmd_info_imx385(0x301A, 1, (sensor_vd >> 16 ) & 0x03, 0);
	rt |= sen_write_reg_imx385(id, &cmd);

	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_LINEAR) {
		//Check max exp line reg
		if (line[0] > MAX_EXPOSURE_LINE) {
			DBG_ERR("max line overflow \r\n");
			line[0] = MAX_EXPOSURE_LINE;
		}

		//Check min exp line
		if (line[0] < MIN_EXPOSURE_LINE) {
			DBG_ERR("min line overflow\r\n");
			line[0] = MIN_EXPOSURE_LINE;
		}

		//Check max exp line
		if (line[0] > (sensor_vd - NON_EXPOSURE_LINE)) {
			line[0] = sensor_vd - NON_EXPOSURE_LINE;
		}
	
		shs1 = sensor_vd - line[0];

		// Set exposure long shutter (shs1)
		cmd = sen_set_cmd_info_imx385(0x3020, 1, shs1 & 0xFF, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		cmd = sen_set_cmd_info_imx385(0x3021, 1, (shs1 >> 8) & 0xFF, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		cmd = sen_set_cmd_info_imx385(0x3022, 1, (shs1 >> 16) & 0x01, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		//DBG_ERR("shs1=%d\r\n", shs1);
	}else if(mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR){
		// Set short shutter (shs1) = RHS1 - 2
		cmd = sen_set_cmd_info_imx385(0x3020, 1, shs1 & 0xFF, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		cmd = sen_set_cmd_info_imx385(0x3021, 1, (shs1 >> 8) & 0xFF, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		cmd = sen_set_cmd_info_imx385(0x3022, 1, (shs1 >> 16) & 0x0F, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		//DBG_ERR("shs1=%d\r\n", shs1);
		
		// Set long shutter (shs2) = FCS - 2
		cmd = sen_set_cmd_info_imx385(0x3023, 1, shs2 & 0xFF, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		cmd = sen_set_cmd_info_imx385(0x3024, 1, (shs2 >> 8) & 0xFF, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		cmd = sen_set_cmd_info_imx385(0x3025, 1, (shs2 >> 16) & 0x0F, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		//DBG_ERR("shs2=%d\r\n", shs2);
		
		// Readout timing (rhs1) = <= FSC - BRL X 2 - 13
		cmd = sen_set_cmd_info_imx385(0x302C, 1, rhs1 & 0xFF, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		cmd = sen_set_cmd_info_imx385(0x302D, 1, (rhs1 >> 8) & 0xFF, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		cmd = sen_set_cmd_info_imx385(0x302E, 1, (rhs1 >> 16) & 0x0F, 0);
		rt |= sen_write_reg_imx385(id, &cmd);
		//DBG_ERR("rhs1=%d\r\n", rhs1);
		
		//Y_OUT_SIZE
		cmd = sen_set_cmd_info_imx385(0x3357, 1, y_out_size & 0xFF, 0x0);
		rt |= sen_write_reg_imx385(id, &cmd);
		cmd = sen_set_cmd_info_imx385(0x3358, 1, (y_out_size >> 8)& 0x1F, 0x0);
		rt |= sen_write_reg_imx385(id, &cmd);
		//DBG_ERR("y_out_size=%d\r\n", y_out_size);
	}

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
	
}

static void sen_set_preset_imx385(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_imx385(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;
	
	cmd = sen_set_cmd_info_imx385(0x3007, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx385(id, &cmd);

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
	
	rt |= sen_write_reg_imx385(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_imx385(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_imx385(0x3007, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx385(id, &cmd);

	*flip = CTL_SEN_FLIP_NONE;
	if (cmd.data[0] & 0x2) {
		*flip |= CTL_SEN_FLIP_H;
	}

	cmd = sen_set_cmd_info_imx385(0x3007, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx385(id, &cmd);

	if (cmd.data[0] & 0x1) {
		*flip |= CTL_SEN_FLIP_V;
	}

	return rt;
}

#if defined(__FREERTOS)
void sen_get_gain_imx385(CTL_SEN_ID id, void *param)
#else
static void sen_get_gain_imx385(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

#if defined(__FREERTOS)
void sen_get_expt_imx385(CTL_SEN_ID id, void *param)
#else
static void sen_get_expt_imx385(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_imx385(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row;

	t_row = sen_calc_rowtime_imx385(id, cur_sen_mode[id]);
	*min_exp_time = t_row * MIN_EXPOSURE_LINE / 10;
}

static void sen_get_mode_basic_imx385(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_imx385(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_imx385(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_imx385(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_imx385(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
{
#if 1
	if (data->type == CTL_SEN_IF_TYPE_MIPI) {
		return E_OK;
	} else if (data->type == CTL_SEN_IF_TYPE_LVDS) {
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

static void sen_get_fps_imx385(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_imx385(id);
	data->chg_fps = sen_get_chgmode_fps_imx385(id);
}

static void sen_get_speed_imx385(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
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

static void sen_get_mode_mipi_imx385(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_mode_lvds_imx385(CTL_SENDRV_GET_MODE_LVDS_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &lvds_param[mode], sizeof(CTL_SENDRV_GET_MODE_LVDS_PARAM));
}

static void sen_get_modesel_imx385(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->data_fmt != CTL_SEN_DATA_FMT_RGB) {
		DBG_ERR("data_fmt %d N.S. \r\n", data->data_fmt);
		return;
	}

	if (data->if_type == CTL_SEN_IF_TYPE_MIPI) {
		if (data->frame_num == 1) {
			if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
				if (data->frame_rate <= 3000) {
					data->mode = CTL_SEN_MODE_1;
					DBG_ERR("mode = %d\r\n", CTL_SEN_MODE_1);
					return;
				}
				if (data->frame_rate <= 6000) {
					data->mode = CTL_SEN_MODE_2;
					DBG_ERR("mode = %d\r\n", CTL_SEN_MODE_2);
					return;
				}
			}
		}else if (data->frame_num == 2) {
				if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
					if (data->frame_rate <= 3000) {
						data->mode = CTL_SEN_MODE_3;
						DBG_ERR("mode = %d\r\n", CTL_SEN_MODE_3);
						return;
					}
				}
		}
	} else if (data->if_type == CTL_SEN_IF_TYPE_LVDS) {
		if (data->frame_num == 1) {
			if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
				if (data->frame_rate <= 3000) {
					data->mode = CTL_SEN_MODE_4;
					DBG_ERR("mode = %d\r\n", CTL_SEN_MODE_4);
					return;
				}
				if (data->frame_rate <= 6000) {
					data->mode = CTL_SEN_MODE_5;
					DBG_ERR("mode = %d\r\n", CTL_SEN_MODE_5);
					return;
				}
			}
		}else if (data->frame_num == 2) {
				if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
					if (data->frame_rate <= 3000) {
						data->mode = CTL_SEN_MODE_6;
						DBG_ERR("mode = %d\r\n", CTL_SEN_MODE_6);
						return;
					}
				}
		}
	}

	DBG_ERR("fail (frame_rate%d,size%d*%d,if_type%d,data_fmt%d,frame_num%d) \r\n"
			, data->frame_rate, data->size.w, data->size.h, data->if_type, data->data_fmt, data->frame_num);
	data->mode = CTL_SEN_MODE_1;
}

static UINT32 sen_calc_rowtime_step_imx385(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 div_step = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	if (mode_basic_param[mode].mode_type == CTL_SEN_MODE_LINEAR) {
		div_step = 1;
	} else {
		div_step = 2;
	}

	return div_step;
}

static UINT32 sen_calc_rowtime_imx385(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 100 * (mode_basic_param[mode].signal_info.hd_period) / ((speed_param[mode].pclk) / 100000);

	//DBG_ERR("hd_period=%d, row_time=%d\r\n",mode_basic_param[mode].signal_info.hd_period, row_time);

	return row_time;
}

static void sen_get_rowtime_imx385(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_imx385(id, data->mode);	
	data->row_time = sen_calc_rowtime_imx385(id, data->mode) * (data->row_time_step);	
}

static void sen_set_cur_fps_imx385(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_imx385(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_imx385(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_imx385(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
void sen_get_i2c_id_imx385(CTL_SEN_ID id, UINT32 *i2c_id)
{
	*i2c_id = sen_i2c[id].id;
}

void sen_get_i2c_addr_imx385(CTL_SEN_ID id, UINT32 *i2c_addr)
{
	*i2c_addr = sen_i2c[id].addr;
}

int sen_init_imx385(SENSOR_DTSI_INFO *info)
{
	CTL_SEN_REG_OBJ reg_obj;
	CHAR node_path[64];
	CHAR compatible[64];
	UINT32 id;
	ER rt = E_OK;

	for (id = 0; id < CTL_SEN_ID_MAX ; id++ ) {
		is_fastboot[id] = 0;
		fastboot_i2c_id[id] = 0xFFFFFFFF;
		fastboot_i2c_addr[id] = 0x0;
	}

	sprintf(compatible, "nvt,sen_imx385");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_imx385");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_imx385;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_imx385();
	rt = ctl_sen_reg_sendrv("nvt_sen_imx385", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_imx385(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_imx385");
}

#else
static int __init sen_init_imx385(void)
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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_imx385;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_imx385();
	rt = ctl_sen_reg_sendrv("nvt_sen_imx385", &reg_obj);
	
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_imx385(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_imx385");
}

module_init(sen_init_imx385);
module_exit(sen_exit_imx385);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_IMX385_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

