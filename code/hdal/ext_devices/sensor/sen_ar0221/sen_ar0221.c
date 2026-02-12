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
VOS_MODULE_VERSION(nvt_sen_ar0221, 1, 49, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_AR0221_MODULE_NAME     "sen_ar0221"
#define SEN_MAX_MODE                  3
#define MAX_VD_PERIOD                 0xFFFF
#define MAX_EXPOSURE_LINE             0xFFFF
#define MIN_EXPOSURE_LINE             1
#define MIN_HDR_EXPOSURE_LINE         2
#define NON_EXPOSURE_LINE             2
#define NON_HDR_EXPOSURE_LINE         16

#define SEN_I2C_ADDR 0x20>>1
#define SEN_I2C_COMPATIBLE "nvt,sen_ar0221"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_ar0221(void);
static void sen_pwr_ctrl_ar0221(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_ar0221(CTL_SEN_ID id);
static ER sen_close_ar0221(CTL_SEN_ID id);
static ER sen_sleep_ar0221(CTL_SEN_ID id);
static ER sen_wakeup_ar0221(CTL_SEN_ID id);
static ER sen_write_reg_ar0221(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_ar0221(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_ar0221(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_ar0221(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_ar0221(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_ar0221(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_ar0221(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_ar0221(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_ar0221(CTL_SEN_ID id, void *param);
static void sen_set_expt_ar0221(CTL_SEN_ID id, void *param);
static void sen_set_preset_ar0221(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_ar0221(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_ar0221(CTL_SEN_ID id, CTL_SEN_FLIP *flip);

#if defined(__FREERTOS)
void sen_get_gain_ar0221(CTL_SEN_ID id, void *param);
void sen_get_expt_ar0221(CTL_SEN_ID id, void *param);
#else
static void sen_get_gain_ar0221(CTL_SEN_ID id, void *param);
static void sen_get_expt_ar0221(CTL_SEN_ID id, void *param);
#endif
static void sen_get_min_expt_ar0221(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_ar0221(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_ar0221(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_ar0221(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_ar0221(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_ar0221(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_ar0221(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_ar0221(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_mipi_ar0221(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_modesel_ar0221(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_ar0221(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_ar0221(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_ar0221(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_ar0221(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_ar0221(CTL_SEN_ID id);
static void sen_set_chgmode_fps_ar0221(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_ar0221(CTL_SEN_ID id);
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
	SEN_AR0221_MODULE_NAME,
	CTL_SEN_VENDOR_ONSEMI,
	SEN_MAX_MODE,
	CTL_SEN_SUPPORT_PROPERTY_MIRROR|CTL_SEN_SUPPORT_PROPERTY_FLIP|CTL_SEN_SUPPORT_PROPERTY_CHGFPS,
	0
};

static CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_param = {
	CTL_SEN_SIGNAL_MASTER,
	{CTL_SEN_ACTIVE_HIGH, CTL_SEN_ACTIVE_HIGH, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING}
};

static CTL_SENDRV_I2C i2c = {
	{
		{CTL_SEN_I2C_W_ADDR_DFT,     0x20},
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
		144000000,
	},
	{
		CTL_SEN_MODE_2,
		CTL_SEN_SIEMCLK_SRC_DFT,
		24000000,
		72000000,
		144000000,
	},
	{
		CTL_SEN_MODE_3,
		CTL_SEN_SIEMCLK_SRC_DFT,
		24000000,
		36000000,
		72000000,
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
		{1, 0, 0, 0},
		SEN_BIT_OFS_0|SEN_BIT_OFS_1
	},
	{
		CTL_SEN_MODE_3,
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
		6000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1924, 1084},
		{{3, 2, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{1920, 1080},
		{0, 1048, 0, 1145},	
		CTL_SEN_RATIO(16, 9),
		{1000, 2659769},
		100
	},
	{
		CTL_SEN_MODE_2,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_STAGGER_HDR,
		3000,
		2,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1924, 1084},
		{{3, 1, 1920, 1080}, {3, 2, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{1920, 1080},
		{0, 1818, 0, 1321},	
		CTL_SEN_RATIO(16, 9),
		{1000, 2659769},
		100
	},
	{
		CTL_SEN_MODE_3,
		CTL_SEN_IF_TYPE_MIPI,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1924, 1084},
		{{3, 2, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{1920, 1080},
		{0, 606, 0, 1981},	
		CTL_SEN_RATIO(16, 9),
		{1000, 2659769},
		100
	}
};

/*
[Linear MIPI 1080p-60fps_MCLK_24MHz]
// 12-bit, MIPI 4 lane
// Pixel clock = 72MHz
// Serial bit rate = 864Mbps
*/
static CTL_SEN_CMD ar0221_mode_1[] = {
	{0x301A, 2, {0x0001, 0x0}},
	{SEN_CMD_DELAY, 2, {10, 0x0}},
	{0x2512, 2, {0x8036, 0x0}}, // SEQ_CTRL_PORT
	{0x2510, 2, {0x090A, 0x0}}, // SEQ_DATA_PORT
	{0x2510, 2, {0x090A, 0x0}}, // SEQ_DATA_PORT
	{0x2512, 2, {0x808A, 0x0}}, // SEQ_CTRL_PORT
	{0x2510, 2, {0x0941, 0x0}}, // SEQ_DATA_PORT
	{0x2512, 2, {0x80D6, 0x0}}, // SEQ_CTRL_PORT
	{0x2510, 2, {0x0945, 0x0}}, // SEQ_DATA_PORT
	// PLL settings
	{0x3030, 2, {0x0036, 0x0}}, //PLL_MULTIPLIER = 54
	{0x302E, 2, {0x0003, 0x0}}, //PRE_PLL_CLK_DIV = 3
	{0x302C, 2, {0x0001, 0x0}}, //VT_SYS_CLK_DIV = 1
	{0x302A, 2, {0x0006, 0x0}}, //VT_PIX_CLK_DIV = 6
	{0x3038, 2, {0x0001, 0x0}}, //OP_SYS_CLK_DIV = 1
	{0x3036, 2, {0x000C, 0x0}}, //OP_PIX_CLK_DIV = 12
	// Timing
	{0x3004, 2, {0x0006, 0x0}}, //X_ADDR_START = 6
	{0x3008, 2, {0x0789, 0x0}}, //X_ADDR_END = 1929
	{0x3002, 2, {0x0006, 0x0}}, //Y_ADDR_START = 6
	{0x3006, 2, {0x0441, 0x0}}, //Y_ADDR_END = 1089
	{0x300A, 2, {0x0479, 0x0}}, //FRAME_LENGTH_LINES = 1981;0x7BD //new para = 1145;0x479
	{0x300C, 2, {0x0418, 0x0}}, //LINE_LENGTH_PCK = 606;0x25E     //new para = 1048;0x418
	{0x3012, 2, {0x0409, 0x0}}, //COARSE_INTEGRATION_TIME = 1033
	{0x3014, 2, {0x0000, 0x0}}, //FINE_INTEGRATION_TIME = 0
	{0x30A2, 2, {0x0001, 0x0}}, //X_ODD_INC = 1
	{0x30A6, 2, {0x0001, 0x0}}, //Y_ODD_INC = 1
	{0x3040, 2, {0x0000, 0x0}}, //READ_MODE
	{0x31AC, 2, {0x0C0C, 0x0}}, //DATA_FORMAT_BITS
	// MIPI setting
	{0x31AE, 2, {0x0204, 0x0}}, //SERIAL_FORMAT - MIPI-4
	{0x31B0, 2, {0x0078, 0x0}}, //FRAME_PREAMBLE = 120
	{0x31B2, 2, {0x004D, 0x0}}, //LINE_PREAMBLE = 77
	{0x31B4, 2, {0x1207, 0x0}}, //MIPI_TIMING_0
	{0x31B6, 2, {0x2216, 0x0}}, //MIPI_TIMING_1
	{0x31B8, 2, {0x7049, 0x0}}, //MIPI_TIMING_2
	{0x31BA, 2, {0x0209, 0x0}}, //MIPI_TIMING_3
	{0x31BC, 2, {0x8C08, 0x0}}, //MIPI_TIMING_4
	// Operation Mode
	{0x3082, 2, {0x0001, 0x0}}, //OPERATION_MODE_CTRL
	{0x30BA, 2, {0x1100, 0x0}}, //DIGITAL_CTRL
	//Recommended_Settings
	{0x3366, 2, {0x2020, 0x0}},
	{0x3370, 2, {0x0331, 0x0}},
	{0x3092, 2, {0x0C24, 0x0}},
	{0x351C, 2, {0x0045, 0x0}},
	{0x336A, 2, {0xA020, 0x0}},
	{0x3522, 2, {0x8840, 0x0}},
	{0x3524, 2, {0x4046, 0x0}},
	{0x3540, 2, {0xC63C, 0x0}},
	{0x3542, 2, {0x4640, 0x0}},
	{0x3544, 2, {0x464B, 0x0}},
	{0x3546, 2, {0x5653, 0x0}},
	{0x3548, 2, {0x5600, 0x0}},
	{0x337A, 2, {0x0B53, 0x0}},
	{0x30FE, 2, {0x00A8, 0x0}},
	{0x31DE, 2, {0x0410, 0x0}},
	{0x31E0, 2, {0x001B, 0x0}},
	{0x3372, 2, {0xF10F, 0x0}},
	{0x3566, 2, {0xB538, 0x0}},
	{0x30B4, 2, {0x0083, 0x0}},
	{0x3520, 2, {0x4600, 0x0}},
	{SEN_CMD_SETVD, 2, {0x00, 0x0}},
	{SEN_CMD_PRESET, 2, {0x00, 0x0}},
	{SEN_CMD_DIRECTION, 2, {0x00, 0x0}},
	{SEN_CMD_DELAY, 2, {20, 0x0}},
	//Start Streaming
	{0x301A, 2, {0x001E, 0x0}},  //RESET_REGISTER
};

/*
[LI-HDR 2-Exp MIPI 1080p-30fps_MCLK_24MH]
// 12-bit, MIPI 4 lane
// Pixel clock = 72MHz
// Serial bit rate = 864Mbps
*/
static CTL_SEN_CMD ar0221_mode_2[] = {
	{0x301A, 2, {0x0001, 0x0}},
	{SEN_CMD_DELAY, 2, {10, 0x0}},
	{0x2512, 2, {0x8036, 0x0}}, // SEQ_CTRL_PORT
	{0x2510, 2, {0x090A, 0x0}}, // SEQ_DATA_PORT
	{0x2510, 2, {0x090A, 0x0}}, // SEQ_DATA_PORT
	{0x2512, 2, {0x808A, 0x0}}, // SEQ_CTRL_PORT
	{0x2510, 2, {0x0941, 0x0}}, // SEQ_DATA_PORT
	{0x2512, 2, {0x80D6, 0x0}}, // SEQ_CTRL_PORT
	{0x2510, 2, {0x0945, 0x0}}, // SEQ_DATA_PORT 
	// PLL settings
	{0x3030, 2, {0x0036, 0x0}},   //PLL_MULTIPLIER = 54
	{0x302E, 2, {0x0003, 0x0}},   //PRE_PLL_CLK_DIV = 3
	{0x302C, 2, {0x0001, 0x0}},   //VT_SYS_CLK_DIV = 1
	{0x302A, 2, {0x0006, 0x0}},   //VT_PIX_CLK_DIV = 6
	{0x3038, 2, {0x0001, 0x0}},   //OP_SYS_CLK_DIV = 1
	{0x3036, 2, {0x000C, 0x0}},   //OP_PIX_CLK_DIV = 12
	// Timing
	{0x3004, 2, {0x0006, 0x0}},   //X_ADDR_START = 6
	{0x3008, 2, {0x0789, 0x0}},   //X_ADDR_END = 1929
	{0x3002, 2, {0x0006, 0x0}},   //Y_ADDR_START = 6
	{0x3006, 2, {0x0441, 0x0}},   //Y_ADDR_END = 1089
	{0x300A, 2, {0x0529, 0x0}},   //FRAME_LENGTH_LINES = 1321
	{0x300C, 2, {0x071A, 0x0}},   //LINE_LENGTH_PCK = 1818
	{0x3012, 2, {0x0343, 0x0}},   //COARSE_INTEGRATION_TIME = 835
	{0x3014, 2, {0x07B4, 0x0}},   //FINE_INTEGRATION_TIME = 1972
	{0x321E, 2, {0x07B4, 0x0}},   //FINE_INTEGRATION_TIME2 = 1972
	{0x3222, 2, {0x05DC, 0x0}},   //FINE_INTEGRATION_TIME3 = 1500
	{0x30A2, 2, {0x0001, 0x0}},   //X_ODD_INC = 1
	{0x30A6, 2, {0x0001, 0x0}},   //Y_ODD_INC = 1
	{0x3040, 2, {0x0000, 0x0}},   //READ_MODE
	{0x31AC, 2, {0x0C0C, 0x0}},   //DATA_FORMAT_BITS
	// MIPI setting
	{0x31AE, 2, {0x0204, 0x0}},   //SERIAL_FORMAT - MIPI-4
	{0x31B0, 2, {0x0078, 0x0}},   //FRAME_PREAMBLE = 120
	{0x31B2, 2, {0x004D, 0x0}},   //LINE_PREAMBLE = 77
	{0x31B4, 2, {0x1207, 0x0}},   //MIPI_TIMING_0
	{0x31B6, 2, {0x2216, 0x0}},   //MIPI_TIMING_1
	{0x31B8, 2, {0x7049, 0x0}},   //MIPI_TIMING_2
	{0x31BA, 2, {0x0209, 0x0}},   //MIPI_TIMING_3
	{0x31BC, 2, {0x8C08, 0x0}},   //MIPI_TIMING_4
	// Operation Mode
	{0x3082, 2, {0x0014, 0x0}},   //OPERATION_MODE_CTRL
	{0x30BA, 2, {0x1102, 0x0}},   //DIGITAL_CTRL
	{0x3238, 2, {0x8033, 0x0}},   //EXPOSURE_RATIO //ori 0x0033; new use flexible exposure
	//Recommended_Settings
	{0x3366, 2, {0x2020, 0x0}},
	{0x3370, 2, {0x0331, 0x0}},
	{0x3092, 2, {0x0C24, 0x0}},
	{0x351C, 2, {0x0045, 0x0}},
	{0x336A, 2, {0xA020, 0x0}},
	{0x3522, 2, {0x8840, 0x0}},
	{0x3524, 2, {0x4046, 0x0}},
	{0x3540, 2, {0xC63C, 0x0}},
	{0x3542, 2, {0x4640, 0x0}},
	{0x3544, 2, {0x464B, 0x0}},
	{0x3546, 2, {0x5653, 0x0}},
	{0x3548, 2, {0x5600, 0x0}},
	{0x337A, 2, {0x0B53, 0x0}},
	{0x30FE, 2, {0x00A8, 0x0}},
	{0x31DE, 2, {0x0410, 0x0}},
	{0x31E0, 2, {0x001B, 0x0}},
	{0x3372, 2, {0xF10F, 0x0}},
	{0x3566, 2, {0xB538, 0x0}},
	{0x30B4, 2, {0x0083, 0x0}},
	{0x3520, 2, {0x4600, 0x0}},
	{0x30B0, 2, {0x0820, 0x0}},	//test
	{SEN_CMD_SETVD, 2, {0x00, 0x0}},
	{SEN_CMD_PRESET, 2, {0x00, 0x0}},
	{SEN_CMD_DIRECTION, 2, {0x00, 0x0}},
	{SEN_CMD_DELAY, 2, {20, 0x0}},
	//Start Streaming
	{0x301A, 2, {0x001E, 0x0}},  //RESET_REGISTER
};

/*
[Linear MIPI 2-lane 1080p-30fps_MCLK_24MHz]
// 12-bit, MIPI 2 lane
// Pixel clock = 36MHz
// Serial bit rate = 864Mbps
*/
static CTL_SEN_CMD ar0221_mode_3[] = {
	{0x301A, 2, {0x0001, 0x0}},
	{SEN_CMD_DELAY, 2, {10, 0x0}},
	// load only the difference from OTPM sequencer
	{0x2512, 1, {0x8036, 0x0}},// SEQ_CTRL_PORT
	{0x2510, 1, {0x090A, 0x0}},
	{0x2510, 1, {0x090A, 0x0}},
	{0x2512, 1, {0x808A, 0x0}},// SEQ_CTRL_PORT
	{0x2510, 1, {0x0941, 0x0}},
	{0x2512, 1, {0x80D6, 0x0}},// SEQ_CTRL_PORT
	{0x2510, 1, {0x0945, 0x0}},
	
	// PLL settings
	{0x3030, 1, {0x0036, 0x0}},//PLL_MULTIPLIER = 54
	{0x302E, 1, {0x0003, 0x0}},//PRE_PLL_CLK_DIV = 3
	{0x302C, 1, {0x0001, 0x0}},//VT_SYS_CLK_DIV = 1
	{0x302A, 1, {0x000C, 0x0}},//VT_PIX_CLK_DIV = 6
	{0x3038, 1, {0x0001, 0x0}},//OP_SYS_CLK_DIV = 1
	{0x3036, 1, {0x000C, 0x0}},//OP_PIX_CLK_DIV = 12
	// Timing
	{0x3004, 1, {0x0006, 0x0}},//X_ADDR_START = 6
	{0x3008, 1, {0x0789, 0x0}},//X_ADDR_END = 1929
	{0x3002, 1, {0x0006, 0x0}},//Y_ADDR_START = 6
	{0x3006, 1, {0x0441, 0x0}},//Y_ADDR_END = 1089
	{0x300A, 1, {0x07BD, 0x0}},//FRAME_LENGTH_LINES = 1981
	{0x300C, 1, {0x025E, 0x0}},//LINE_LENGTH_PCK = 606
	{0x3012, 1, {0x0409, 0x0}},//COARSE_INTEGRATION_TIME = 1033
	{0x3014, 1, {0x0000, 0x0}},//FINE_INTEGRATION_TIME = 0
	{0x30A2, 1, {0x0001, 0x0}},//X_ODD_INC = 1
	{0x30A6, 1, {0x0001, 0x0}},//Y_ODD_INC = 1
	{0x3040, 1, {0x0000, 0x0}},//READ_MODE
	{0x31AC, 1, {0x0C0C, 0x0}},//DATA_FORMAT_BITS
	// MIPI setting
	{0x31AE, 1, {0x0202, 0x0}},//SERIAL_FORMAT - MIPI-2
	{0x31B0, 1, {0x0078, 0x0}},// FRAME_PREAMBLE
	{0x31B2, 1, {0x004D, 0x0}},// LINE_PREAMBLE
	{0x31B4, 1, {0x1207, 0x0}},// MIPI_TIMING_0
	{0x31B6, 1, {0x2216, 0x0}},// MIPI_TIMING_1
	{0x31B8, 1, {0x7049, 0x0}},// MIPI_TIMING_2
	{0x31BA, 1, {0x0209, 0x0}},// MIPI_TIMING_3
	{0x31BC, 1, {0x8C08, 0x0}},// MIPI_TIMING_4
	// Operation Mode
	{0x3082, 1, {0x0001, 0x0}},//OPERATION_MODE_CTRL
	{0x30BA, 1, {0x1100, 0x0}},//DIGITAL_CTRL
	
	//Recommended_Settings
	{0x3366, 1, {0x2020, 0x0}},
	{0x3370, 1, {0x0331, 0x0}},
	{0x3092, 1, {0x0C24, 0x0}},
	{0x351C, 1, {0x0045, 0x0}},
	{0x336A, 1, {0xA020, 0x0}},
	{0x3522, 1, {0x8840, 0x0}},
	{0x3524, 1, {0x4046, 0x0}},
	{0x3540, 1, {0xC63C, 0x0}},
	{0x3542, 1, {0x4640, 0x0}},
	{0x3544, 1, {0x464B, 0x0}},
	{0x3546, 1, {0x5653, 0x0}},
	{0x3548, 1, {0x5600, 0x0}},
	{0x337A, 1, {0x0B53, 0x0}},
	{0x30FE, 1, {0x00A8, 0x0}},
	{0x31DE, 1, {0x0410, 0x0}},
	{0x31E0, 1, {0x001B, 0x0}},
	{0x3372, 1, {0xF10F, 0x0}},
	{0x3566, 1, {0xB538, 0x0}},
	{0x30B4, 1, {0x0083, 0x0}},
	{0x3520, 1, {0x4600, 0x0}},
	{SEN_CMD_SETVD, 2, {0x00, 0x0}},
	{SEN_CMD_PRESET, 2, {0x00, 0x0}},
	{SEN_CMD_DIRECTION, 2, {0x00, 0x0}},
	{SEN_CMD_DELAY, 2, {20, 0x0}},
	//Start Streaming
	{0x301A, 2, {0x001E, 0x0}},  //RESET_REGISTER
};

typedef struct gain_set {
    UINT32  a_gain;    
    UINT32  total_gain;
} gain_set_t;

static gain_set_t gain_table[] = // gain*1000
{
	{0x0020, 1000},
	{0x0021, 1040},
	{0x0022, 1080},
	{0x0023, 1170},
	{0x0024, 1250},
	{0x0025, 1300},
	{0x0026, 1380},
	{0x0027, 1440},	
	{0x0029, 1560},		
	{0x002B, 1670},
	{0x002D, 1800},	
	{0x002E, 2000},
	{0x002F, 2330},
	{0x0030, 2500},
	{0x0031, 2700},
	{0x0032, 3000},
	{0x0033, 3240},
	{0x0034, 3510},
	{0x0035, 4000},
	{0x0036, 4320},
	{0x0037, 4680},
	{0x0038, 5000},
	{0x0039, 5400},
	{0x003A, 6000},
	{0x003B, 6480},
	{0x003C, 7000},
	{0x003D, 7560},
	{0x003E, 8000},
};

#define NUM_OF_GAINSET (sizeof(gain_table) / sizeof(gain_set_t))

static UINT32 cur_sen_mode[CTL_SEN_ID_MAX] = {CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1, CTL_SEN_MODE_1};
static UINT32 cur_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 chgmode_fps[CTL_SEN_ID_MAX] = {0};
static UINT32 power_ctrl_mclk[CTL_SEN_CLK_SEL_MAX] = {0};
static UINT32 reset_ctrl_count[CTL_SEN_ID_MAX] = {0};
static UINT32 pwdn_ctrl_count[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_CTRL sensor_ctrl_last[CTL_SEN_ID_MAX] = {0};
static ISP_SENSOR_PRESET_CTRL preset_ctrl[CTL_SEN_ID_MAX] = {0};
static UINT32 compensation_ratio[CTL_SEN_ID_MAX][ISP_SEN_MFRAME_MAX_NUM] = {0};
static INT32 is_fastboot[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_id[CTL_SEN_ID_MAX];
static UINT32 fastboot_i2c_addr[CTL_SEN_ID_MAX];
static BOOL i2c_valid[CTL_SEN_ID_MAX];

static CTL_SEN_DRV_TAB ar0221_sen_drv_tab = {
	sen_open_ar0221,
	sen_close_ar0221,
	sen_sleep_ar0221,
	sen_wakeup_ar0221,
	sen_write_reg_ar0221,
	sen_read_reg_ar0221,
	sen_chg_mode_ar0221,
	sen_chg_fps_ar0221,
	sen_set_info_ar0221,
	sen_get_info_ar0221,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_ar0221(void)
{
	return &ar0221_sen_drv_tab;
}

static void sen_pwr_ctrl_ar0221(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
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

static CTL_SEN_CMD sen_set_cmd_info_ar0221(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_ar0221(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_ar0221(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_ar0221);
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

static ER sen_close_ar0221(CTL_SEN_ID id)
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

static ER sen_sleep_ar0221(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_ar0221(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_ar0221(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	struct i2c_msg msgs;
	unsigned char buf[4];
	int i;

	if (!i2c_valid[id]) {
		return E_NOSPT;
	}

	buf[0]     = (cmd->addr >> 8) & 0xFF;
	buf[1]     = cmd->addr & 0xFF;
	buf[2]     = (cmd->data[0] >> 8) & 0xFF;
	buf[3]     = cmd->data[0] & 0xFF;
	msgs.addr  = sen_i2c[id].addr;
	msgs.flags = 0;
	msgs.len   = 4;
	msgs.buf   = buf;

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

static ER sen_read_reg_ar0221(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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
	tmp2[1]       = 0;
	msgs[1].addr  = sen_i2c[id].addr;
	msgs[1].flags = 1;
	msgs[1].len   = 2;
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

	cmd->data[0] = (tmp2[0] << 8) | tmp2[1];

	return E_OK;
}

static UINT32 sen_get_cmd_tab_ar0221(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = ar0221_mode_1;
		return sizeof(ar0221_mode_1) / sizeof(CTL_SEN_CMD);

	case CTL_SEN_MODE_2:
		*cmd_tab = ar0221_mode_2;
		return sizeof(ar0221_mode_2) / sizeof(CTL_SEN_CMD);
		
	case CTL_SEN_MODE_3:
		*cmd_tab = ar0221_mode_3;
		return sizeof(ar0221_mode_3) / sizeof(CTL_SEN_CMD);		

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_ar0221(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
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
		sen_set_chgmode_fps_ar0221(id, isp_builtin_get_chgmode_fps(id));
		sen_set_cur_fps_ar0221(id, isp_builtin_get_chgmode_fps(id));
		sen_set_gain_ar0221(id, &sensor_ctrl);
		sen_set_expt_ar0221(id, &sensor_ctrl);
		#endif
		preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

		return E_OK;
	}

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_ar0221(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_ar0221(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_ar0221(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_ar0221(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_ar0221(0x300A, 2, (sensor_vd & 0xFFFF), 0x00);
			rt |= sen_write_reg_ar0221(id, &cmd);
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

			sen_set_gain_ar0221(id, &sensor_ctrl);
			sen_set_expt_ar0221(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_ar0221(id, &flip);
		} else {
			cmd = sen_set_cmd_info_ar0221(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_ar0221(id, &cmd);
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_ar0221(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_ar0221(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_ar0221(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_ar0221(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	cmd = sen_set_cmd_info_ar0221(0x300A, 2, sensor_vd & 0xFFFF, 0x00);
	rt |= sen_write_reg_ar0221(id, &cmd);

	return rt;
}

static ER sen_set_info_ar0221(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_ar0221(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_ar0221(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_ar0221(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_ar0221(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_ar0221(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_ar0221(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_ar0221(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_ar0221((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_ar0221((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_ar0221(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_ar0221((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_ar0221(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_ar0221(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_ar0221((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_ar0221((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_ar0221((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_ar0221(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_ar0221(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_ar0221(id, data);
		break;
	default:
		rt = E_NOSPT;
	}
	return rt;
}

static UINT32 sen_calc_chgmode_vd_ar0221(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	sen_set_chgmode_fps_ar0221(id, fps);
	sen_set_cur_fps_ar0221(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_ar0221(id, fps);
		sen_set_cur_fps_ar0221(id, fps);
	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_ar0221(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_ar0221(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_ar0221(CTL_SEN_ID id, UINT32 fps)
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

static void sen_set_gain_ar0221(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 data1[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data2[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data3[ISP_SEN_MFRAME_MAX_NUM] = {0};	
	UINT32 data4[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 hcg_gain[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 frame_cnt, total_frame;
	UINT32 step = 0, i = 0;
	UINT32 d_gain1 = 0, d_gain1_reg = 0, d_gain2 = 0, d_gain2_reg = 0, dpc_para1 = 0, dpc_para2 = 0, dpc_para3 = 0;	
	UINT32 diff_ratio = 0;
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
		if (100 <= (compensation_ratio[id][frame_cnt])) {
			sensor_ctrl->gain_ratio[frame_cnt] = (sensor_ctrl->gain_ratio[frame_cnt]) * (compensation_ratio[id][frame_cnt]) / 100;
		}
		if (sensor_ctrl->gain_ratio[frame_cnt] < (mode_basic_param[cur_sen_mode[id]].gain.min)) {
			sensor_ctrl->gain_ratio[frame_cnt] = (mode_basic_param[cur_sen_mode[id]].gain.min);
		} else if (sensor_ctrl->gain_ratio[frame_cnt] > (mode_basic_param[cur_sen_mode[id]].gain.max)) {
			sensor_ctrl->gain_ratio[frame_cnt] = (mode_basic_param[cur_sen_mode[id]].gain.max);
		}
		if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {	
			if(41600 >= (sensor_ctrl->gain_ratio[frame_cnt])) {//41600 = 8x again x 5.2x HCG
				if (5200 <= (sensor_ctrl->gain_ratio[frame_cnt])) {
					hcg_gain[frame_cnt] = 1;
    			//Search most suitable gain into gain table
					for ( i = 0 ; i < NUM_OF_GAINSET ; i++ ) {
						step = i;
						if ((gain_table[i].total_gain) > ((sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / 5200)) {
							break;
						}
					}
    	
					if ( 1 > step ) {
						step = 0;		
					} else if ((gain_table[NUM_OF_GAINSET - 1].total_gain) > ((sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / 5200)){
						step -= 1;	
					}
					d_gain1 = 1000;
					d_gain1_reg = 0x80;
					d_gain2 =  ((sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / 5200) * 1000 / (gain_table[step].total_gain);
					d_gain2_reg = d_gain2 * 512 / 1000;												
				} else {
					hcg_gain[frame_cnt] = 0;
    			//Search most suitable gain into gain table
					for ( i = 0 ; i < NUM_OF_GAINSET ; i++ ) {
						step = i;
						if ((gain_table[i].total_gain) > (sensor_ctrl->gain_ratio[frame_cnt])) {
							break;
						}
					}
    	
					if ( 1 > step ) {
						step = 0;		
					} else if ((gain_table[NUM_OF_GAINSET - 1].total_gain) > (sensor_ctrl->gain_ratio[frame_cnt])){
						step -= 1;	
					}
					d_gain1 = 1000;
					d_gain1_reg = 0x80;							
					d_gain2 =  (sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / (gain_table[step].total_gain);
					d_gain2_reg = d_gain2 * 512 / 1000;							
				}
			} else if (166319 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
					hcg_gain[frame_cnt] = 1;
					step = NUM_OF_GAINSET - 1;
					d_gain1 = 1000;					
					d_gain1_reg = 0x80;
					d_gain2 = ((sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / 5200) * 1000 / (gain_table[step].total_gain);
					d_gain2_reg = d_gain2 * 512 / 1000;											
			} else {
					hcg_gain[frame_cnt] = 1;
					step = NUM_OF_GAINSET - 1;
					d_gain2 = 3998;
					d_gain2_reg = 0x7FF;
					d_gain1 = ((sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / 5200) * 1000 / (gain_table[step].total_gain) * 1000 / d_gain2 ;
					d_gain1_reg = d_gain1 * 128 / 1000;												
			}
	
			if (4742 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0x4B3F;
				dpc_para2 = 0x8030;
				dpc_para3 = 0x0804;						
			} else if (9360 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0x4B3F;
				dpc_para2 = 0x7830;
				dpc_para3 = 0x0804;				
			} else if (19393 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0x4B3F;
				dpc_para2 = 0x6818;
				dpc_para3 = 0x0A84;
			} else if (39312 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0x783F;
				dpc_para2 = 0x4818;
				dpc_para3 = 0x0F84;
			} else if (76700 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0xFFFF;
				dpc_para2 = 0x4072;
				dpc_para3 = 0x0F84;
			} else if (166320 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0xFFFF;
				dpc_para2 = 0x2818;
				dpc_para3 = 0x0F84;
			} else {
				dpc_para1 = 0xFFFF;
				dpc_para2 = 0x2018;
				dpc_para3 = 0x1604;
			}
			data1[frame_cnt] = (gain_table[step].a_gain << 16) | hcg_gain[0];
			data2[frame_cnt] = (d_gain1_reg << 16) | (dpc_para1);
			data3[frame_cnt] = (dpc_para2 << 16) | (dpc_para3);
			data4[frame_cnt] = d_gain2_reg;
			
		} else if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_LINEAR) {		
			if(41600 >= (sensor_ctrl->gain_ratio[frame_cnt])) {//41600 = 8x again x 5.2x HCG
				if (5200 <= (sensor_ctrl->gain_ratio[frame_cnt])) {
					hcg_gain[frame_cnt] = 1;
    			//Search most suitable gain into gain table
					for ( i = 0 ; i < NUM_OF_GAINSET ; i++ ) {
						step = i;
						if ((gain_table[i].total_gain) > ((sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / 5200)) {
							break;
						}
					}
    	
					if ( 1 > step ) {
						step = 0;		
					} else if ((gain_table[NUM_OF_GAINSET - 1].total_gain) > ((sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / 5200)){
						step -= 1;	
					}
					d_gain1 = 1000;
					d_gain1_reg = 0x80;
					d_gain2 =  ((sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / 5200) * 1000 / (gain_table[step].total_gain);
					d_gain2_reg = d_gain2 * 512 / 1000;												
				} else {
					hcg_gain[frame_cnt] = 0;
    			//Search most suitable gain into gain table
					for ( i = 0 ; i < NUM_OF_GAINSET ; i++ ) {
						step = i;
						if ((gain_table[i].total_gain) > (sensor_ctrl->gain_ratio[frame_cnt])) {
							break;
						}
					}
    	
					if ( 1 > step ) {
						step = 0;		
					} else if ((gain_table[NUM_OF_GAINSET - 1].total_gain) > (sensor_ctrl->gain_ratio[frame_cnt])){
						step -= 1;	
					}
					d_gain1 = 1000;
					d_gain1_reg = 0x80;							
					d_gain2 =  (sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / (gain_table[step].total_gain);
					d_gain2_reg = d_gain2 * 512 / 1000;							
				}
			} else if (166319 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
					hcg_gain[frame_cnt] = 1;
					step = NUM_OF_GAINSET - 1;
					d_gain1 = 1000;					
					d_gain1_reg = 0x80;
					d_gain2 = ((sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / 5200) * 1000 / (gain_table[step].total_gain);
					d_gain2_reg = d_gain2 * 512 / 1000;											
			} else {
					hcg_gain[frame_cnt] = 1;
					step = NUM_OF_GAINSET - 1;
					d_gain2 = 3998;
					d_gain2_reg = 0x7FF;
					d_gain1 = ((sensor_ctrl->gain_ratio[frame_cnt]) * 1000 / 5200) * 1000 / (gain_table[step].total_gain) * 1000 / d_gain2 ;
					d_gain1_reg = d_gain1 * 128 / 1000;												
			}
    	
			if (4742 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0x4B3F;
				dpc_para2 = 0x8030;
				dpc_para3 = 0x0804;						
			} else if (9360 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0x4B3F;
				dpc_para2 = 0x7830;
				dpc_para3 = 0x0804;				
			} else if (19393 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0x4B3F;
				dpc_para2 = 0x6818;
				dpc_para3 = 0x0A84;
			} else if (39312 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0x783F;
				dpc_para2 = 0x4818;
				dpc_para3 = 0x0F84;
			} else if (76700 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0xFFFF;
				dpc_para2 = 0x4072;
				dpc_para3 = 0x0F84;
			} else if (166320 >= (sensor_ctrl->gain_ratio[frame_cnt])) {
				dpc_para1 = 0xFFFF;
				dpc_para2 = 0x2818;
				dpc_para3 = 0x0F84;
			} else {
				dpc_para1 = 0xFFFF;
				dpc_para2 = 0x2018;
				dpc_para3 = 0x1604;
			}
			data1[frame_cnt] = (gain_table[step].a_gain << 16) | hcg_gain[frame_cnt];
			data2[frame_cnt] = (d_gain1_reg << 16) | (dpc_para1);
			data3[frame_cnt] = (dpc_para2 << 16) | (dpc_para3);
			data4[frame_cnt] = d_gain2_reg;			
		}
	}
	
	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {	   
		if ((sensor_ctrl->gain_ratio[0]) > (sensor_ctrl->gain_ratio[1])) {
			diff_ratio = (sensor_ctrl->gain_ratio[0]) * 1000 / (sensor_ctrl->gain_ratio[1]);
			data1[0] = data1[1];
			data4[0] = data4[1];
			d_gain1 = ((data2[1] >> 16) & 0x7FF) * 1000 / 128 * diff_ratio / 1000;
			d_gain1_reg = d_gain1 * 128 / 1000;
			if (0x7FF < d_gain1_reg) {
				d_gain1_reg =0x7FF;
			}
			data2[0] = (d_gain1_reg << 16) | (dpc_para1);
		} else if ((sensor_ctrl->gain_ratio[0]) < (sensor_ctrl->gain_ratio[1])) {
			diff_ratio = (sensor_ctrl->gain_ratio[1]) * 1000 / (sensor_ctrl->gain_ratio[0]);
			data1[1] = data1[0];
			data4[1] = data4[0];
			d_gain1 = ((data2[0] >> 16) & 0x7FF) * 1000 / 128 * diff_ratio / 1000;
			d_gain1_reg = d_gain1 * 128 / 1000;
			if (0x7FF < d_gain1_reg) {
				d_gain1_reg =0x7FF;
			}			
			data2[1] = (d_gain1_reg << 16) | (dpc_para1);
		}
	}

	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {	 
		//Set analog gain
		cmd = sen_set_cmd_info_ar0221(0x3366, 2,  ((((data1[1] >> 16) & 0x3F) << 8 ) | ((data1[0] >> 16) & 0x3F)), 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
		//set DCG gain
		cmd = sen_set_cmd_info_ar0221(0x3362, 2, (((data1[1] & 0x1) << 1) | (data1[0] & 0x1)), 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
		//Set digital gain
		cmd = sen_set_cmd_info_ar0221(0x305E, 2, (data2[0] >> 16) & 0x7FF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);		
		cmd = sen_set_cmd_info_ar0221(0x3158, 2, (data2[1] >> 16) & 0x7FF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);	
		cmd = sen_set_cmd_info_ar0221(0x3308, 2, data4[0]& 0x7FF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);		

		//Set 1DCC
		cmd = sen_set_cmd_info_ar0221(0x3F4C, 2, data2[0] & 0xFFFF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
		cmd = sen_set_cmd_info_ar0221(0x3F4E, 2, (data3[0] >> 16) & 0xFFFF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
		cmd = sen_set_cmd_info_ar0221(0x3F50, 2, data3[0] & 0xFFFF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);

	} else if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_LINEAR) {
		//Set analog gain
		cmd = sen_set_cmd_info_ar0221(0x3366, 2, (data1[0] >> 16) & 0x3F, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
		//Set DCG gain
		cmd = sen_set_cmd_info_ar0221(0x3362, 2, data1[0] & 0x1, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
		//Set digital gain
		cmd = sen_set_cmd_info_ar0221(0x305E, 2, (data2[0] >> 16) & 0x7FF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);		
		cmd = sen_set_cmd_info_ar0221(0x3308, 2, data4[0]& 0x7FF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
   	
		//Set 1DCC
		cmd = sen_set_cmd_info_ar0221(0x3F4C, 2, data2[0] & 0xFFFF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
		cmd = sen_set_cmd_info_ar0221(0x3F4E, 2, (data3[0] >> 16) & 0xFFFF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
		cmd = sen_set_cmd_info_ar0221(0x3F50, 2, data3[0] & 0xFFFF, 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);	
	}

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_expt_ar0221(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps = 0, cur_fps = 0, clac_fps = 0, t_row = 0, temp_max_short_line = 0;
	UINT32 temp_line[ISP_SEN_MFRAME_MAX_NUM] = {0};	
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
		t_row = sen_calc_rowtime_ar0221(id, cur_sen_mode[id]);
		if (0 == t_row) {
			DBG_WRN("t_row  = 0, must >= 1 \r\n");
			t_row = 1;
		}
		line[frame_cnt] = (sensor_ctrl->exp_time[frame_cnt]) * 10 / t_row;

		if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {		
			if (line[frame_cnt] < MIN_HDR_EXPOSURE_LINE) {
				line[frame_cnt] = MIN_HDR_EXPOSURE_LINE;
			}
		} else {
			if (line[frame_cnt] < MIN_EXPOSURE_LINE) {
				line[frame_cnt] = MIN_EXPOSURE_LINE;
			}
		}
	}

	// Write exposure line
	// Get fps
	chgmode_fps = sen_get_chgmode_fps_ar0221(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_ar0221(id, cur_sen_mode[id]);
	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		expt_time = (line[0] + line[1])  * t_row / 10;
		temp_line[0] = line[0];
		temp_line[1] = line[1];
	} else {
		expt_time = (line[0]) * t_row / 10;		
		temp_line[0] = line[0];
	}

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_ar0221(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_ar0221(id, cur_fps);

	//Check max vts
	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("max vts overflow\r\n");
		sensor_vd = MAX_VD_PERIOD;
	}

	cmd = sen_set_cmd_info_ar0221(0x300A, 1, sensor_vd & 0xFFFF, 0);
	rt |= sen_write_reg_ar0221(id, &cmd);

	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {		
		temp_max_short_line = (100000000 / cur_fps ) * 10 / 17 / t_row; //max support 1:16

		if (line[0] > (MAX_EXPOSURE_LINE)) {
			DBG_ERR("max line overflow \r\n");
			line[0] = MAX_EXPOSURE_LINE;
		}
		if (line[1] > (MAX_EXPOSURE_LINE)) {
			DBG_ERR("max line overflow \r\n");
			line[1] = MAX_EXPOSURE_LINE;
		}
		
		if ((line[1]) > (temp_max_short_line)) {
			line[1] = temp_max_short_line;
		}				
		
		if ((line[0] + line[1]) > (sensor_vd - NON_HDR_EXPOSURE_LINE)) {
			line[0] = sensor_vd - line[1] - NON_HDR_EXPOSURE_LINE;
		}		
		compensation_ratio[id][0] = 100 * temp_line[0] / line[0];
		compensation_ratio[id][1] = 100 * temp_line[1] / line[1];		

		// set exposure line to sensor
		cmd = sen_set_cmd_info_ar0221(0x3012, 2, line[0], 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
		cmd = sen_set_cmd_info_ar0221(0x3212, 2, line[1], 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);				
				
	} else {
		if (line[0] > (MAX_EXPOSURE_LINE)) {
			DBG_ERR("max line overflow \r\n");
			line[0] = MAX_EXPOSURE_LINE;
		}
  	
		if (line[0] > (sensor_vd - NON_EXPOSURE_LINE)) {
			line[0] = sensor_vd - NON_EXPOSURE_LINE;
		}
		compensation_ratio[id][0] = 100 * temp_line[0] / line[0];
		// set exposure line to sensor
		cmd = sen_set_cmd_info_ar0221(0x3012, 2, line[0], 0x0);
		rt |= sen_write_reg_ar0221(id, &cmd);
	}

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_ar0221(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_ar0221(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_ar0221(0x3040, 2, 0x0, 0x0);
	rt |= sen_read_reg_ar0221(id, &cmd);
	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
		if (*flip & CTL_SEN_FLIP_H) {
				cmd.data[0] |= 0x4000;
		} else {
				cmd.data[0] &= (~0x4000);
		}
	} else {
		DBG_WRN("no support mirror \r\n");
	}
	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (*flip & CTL_SEN_FLIP_V) {
			cmd.data[0] |= 0x8000;
		} else {
			cmd.data[0] &= (~0x8000);
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}

	rt |= sen_write_reg_ar0221(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_ar0221(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	*flip = CTL_SEN_FLIP_NONE;
	cmd = sen_set_cmd_info_ar0221(0x3040, 2, 0x0, 0x0);
	rt |= sen_read_reg_ar0221(id, &cmd);
		
	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
		if (cmd.data[0] & 0x4000) {
			*flip |= CTL_SEN_FLIP_H;
		}
	} else {
		DBG_WRN("no support mirror \r\n");
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (cmd.data[0] & 0x8000) {
			*flip |= CTL_SEN_FLIP_V;
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}

	return rt;
}

#if defined(__FREERTOS)
void sen_get_gain_ar0221(CTL_SEN_ID id, void *param)
#else
static void sen_get_gain_ar0221(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

#if defined(__FREERTOS)
void sen_get_expt_ar0221(CTL_SEN_ID id, void *param)
#else
static void sen_get_expt_ar0221(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_ar0221(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row;

	t_row = sen_calc_rowtime_ar0221(id, cur_sen_mode[id]);
	if (mode_basic_param[cur_sen_mode[id]].mode_type == CTL_SEN_MODE_STAGGER_HDR) {		
		*min_exp_time = t_row * MIN_HDR_EXPOSURE_LINE / 10 + 1; // if linear and shdr mode is different,the value need to separate.
	} else {
		*min_exp_time = t_row * MIN_EXPOSURE_LINE / 10 + 1; // if linear and shdr mode is different,the value need to separate.
	}
}

static void sen_get_mode_basic_ar0221(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_ar0221(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_ar0221(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_ar0221(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_ar0221(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
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

static void sen_get_fps_ar0221(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_ar0221(id);
	data->chg_fps = sen_get_chgmode_fps_ar0221(id);
}

static void sen_get_speed_ar0221(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
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

static void sen_get_mode_mipi_ar0221(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_modesel_ar0221(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->if_type != CTL_SEN_IF_TYPE_MIPI) {
		DBG_ERR("if_type %d N.S. \r\n", data->if_type);
		return;
	}

	if (data->data_fmt != CTL_SEN_DATA_FMT_RGB) {
		DBG_ERR("data_fmt %d N.S. \r\n", data->data_fmt);
		return;
	}

	if (data->data_lane == CTL_SEN_DATALANE_2) {
		if (data->frame_num == 1) {
			if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
				if (data->frame_rate <= 3000) {
					data->mode = CTL_SEN_MODE_3;
					return;
				}
			}
		}
	} else {
		if (data->frame_num == 1) {
			if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
				if (data->frame_rate <= 6000) {
					data->mode = CTL_SEN_MODE_1;
					return;
				}
			}
		} else if (data->frame_num == 2) {
			if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
				if (data->frame_rate <= 3000) {
					data->mode = CTL_SEN_MODE_2;
					return;
				}
			}		
		}
	}

	DBG_ERR("fail (frame_rate%d,size%d*%d,if_type%d,data_fmt%d,frame_num%d) \r\n"
			, data->frame_rate, data->size.w, data->size.h, data->if_type, data->data_fmt, data->frame_num);
	data->mode = CTL_SEN_MODE_1;
}

static UINT32 sen_calc_rowtime_step_ar0221(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 div_step = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	if (mode_basic_param[mode].mode_type == CTL_SEN_MODE_LINEAR) {
		div_step = 1;
	} else if (mode_basic_param[mode].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		div_step = 1;		
	}

	return div_step;
}

static UINT32 sen_calc_rowtime_ar0221(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 10 * (mode_basic_param[mode].signal_info.hd_period) / ((speed_param[mode].pclk) / 1000000);

	return row_time;
}

static void sen_get_rowtime_ar0221(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_ar0221(id, data->mode);
	data->row_time = sen_calc_rowtime_ar0221(id, data->mode) * (data->row_time_step);
}

static void sen_set_cur_fps_ar0221(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_ar0221(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_ar0221(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_ar0221(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
void sen_get_i2c_id_ar0221(CTL_SEN_ID id, UINT32 *i2c_id)
{
	*i2c_id = sen_i2c[id].id;
}

void sen_get_i2c_addr_ar0221(CTL_SEN_ID id, UINT32 *i2c_addr)
{
	*i2c_addr = sen_i2c[id].addr;
}

int sen_init_ar0221(SENSOR_DTSI_INFO *info)
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

	sprintf(compatible, "nvt,sen_ar0221");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_ar0221");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_ar0221;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_ar0221();
	rt = ctl_sen_reg_sendrv("nvt_sen_ar0221", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_ar0221(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_ar0221");
}

#else
static int __init sen_init_ar0221(void)
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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_ar0221;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_ar0221();
	rt = ctl_sen_reg_sendrv("nvt_sen_ar0221", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_ar0221(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_ar0221");
}

module_init(sen_init_ar0221);
module_exit(sen_exit_ar0221);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_AR0221_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

