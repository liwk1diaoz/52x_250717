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
#else
#endif

//=============================================================================
// version
//=============================================================================
VOS_MODULE_VERSION(nvt_sen_ar0144, 1, 42, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_AR0144_MODULE_NAME     "sen_ar0144"
#define SEN_MAX_MODE               1
#define MAX_VD_PERIOD              0x3FFFF
#define MAX_EXPOSURE_LINE          0xFFFF
#define MIN_EXPOSURE_LINE          1
#define NON_EXPOSURE_LINE          2

#define SEN_I2C_ADDR 0x20 >> 1
#define SEN_I2C_COMPATIBLE "nvt,sen_ar0144"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_ar0144(void);
static void sen_pwr_ctrl_ar0144(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_ar0144(CTL_SEN_ID id);
static ER sen_close_ar0144(CTL_SEN_ID id);
static ER sen_sleep_ar0144(CTL_SEN_ID id);
static ER sen_wakeup_ar0144(CTL_SEN_ID id);
static ER sen_write_reg_ar0144(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_ar0144(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_ar0144(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_ar0144(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_ar0144(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_ar0144(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_ar0144(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_ar0144(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_ar0144(CTL_SEN_ID id, void *param);
static void sen_set_expt_ar0144(CTL_SEN_ID id, void *param);
static void sen_set_preset_ar0144(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_ar0144(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static void sen_get_gain_ar0144(CTL_SEN_ID id, void *param);
static void sen_get_expt_ar0144(CTL_SEN_ID id, void *param);
static void sen_get_min_expt_ar0144(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_ar0144(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_ar0144(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_ar0144(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_ar0144(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_ar0144(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_ar0144(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_ar0144(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
//static void sen_get_mode_para_ar0144(CTL_SENDRV_GET_MODE_PARA_PARAM *data);
static void sen_get_mode_mipi_ar0144(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_modesel_ar0144(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_ar0144(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_ar0144(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_ar0144(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_ar0144(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_ar0144(CTL_SEN_ID id);
static void sen_set_chgmode_fps_ar0144(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_ar0144(CTL_SEN_ID id);
//static void sen_get_iadj_ar0144(CTL_SEN_ID id,  CTL_SENDRV_GET_MODE_MANUAL_IADJ *data);
static UINT32 sen_get_cmd_tab_gs_2p4(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab);

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
	SEN_AR0144_MODULE_NAME,
	CTL_SEN_VENDOR_ONSEMI,
	SEN_MAX_MODE,
	CTL_SEN_SUPPORT_PROPERTY_MIRROR|CTL_SEN_SUPPORT_PROPERTY_FLIP|CTL_SEN_SUPPORT_PROPERTY_CHGFPS,
	0
};

static CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_param = {
	CTL_SEN_SIGNAL_MASTER,
	{CTL_SEN_ACTIVE_HIGH, CTL_SEN_ACTIVE_LOW, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING, CTL_SEN_PHASE_RISING}
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
		24000000,//24000000,//12000000,
		74250000,//74250000,
		74250000
	},
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
		6000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_12BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1280, 800},
		{{0, 40, 1280, 720}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0} },
		{1280, 720},
		//{0, 1488, 0, 1428},///828
		//{0, 1488, 0, 829},///828
		{0, 1488, 0, 1139},///828
		CTL_SEN_RATIO(16, 9),
		{1000, 128000},//154180
		100
	},
};


static CTL_SEN_CMD ar0144_mode_1[] = {
	//reset
	{0x301A, 2, {0x00, 0xD9} },
	{SEN_CMD_DELAY, 2, {2, 0x0} },
	//1DDC parameters
	{0x3F4C, 2, {0x4B, 0x3F} },
	{0x3F4C, 2, {0x00, 0x3F} },
	{0x3F4E, 2, {0x57, 0x18} },
	{0x3F4E, 2, {0x00, 0x18} },
	{0x3F50, 2, {0x40, 0x1F} },
	{0x3F50, 2, {0x17, 0xDF} },
		//DELAY = 200
	{SEN_CMD_DELAY, 2, {0x00C8, 0x0} },
		//AR0144CS REV4 Recommended Settings
	{0x3ED6, 2, {0x3C, 0xB5} },
	{0x3ED8, 2, {0x87, 0x65} },
	{0x3EDA, 2, {0x88, 0x88} },
	{0x3EDC, 2, {0x97, 0xFF} },
	{0x3EF8, 2, {0x65, 0x22} },
	{0x3EFA, 2, {0x22, 0x22} },
	{0x3EFC, 2, {0x66, 0x66} },
	{0x3F00, 2, {0xAA, 0x05} },
	{0x3EE2, 2, {0x18, 0x0E} },
	{0x3EE4, 2, {0x08, 0x08} },
	{0x3EEA, 2, {0x2A, 0x09} },
	{0x3060, 2, {0x00, 0x0D} },
	{0x3092, 2, {0x00, 0xCF} },
	{0x3268, 2, {0x00, 0x30} },
	{0x3786, 2, {0x00, 0x06} },
	{0x3F4A, 2, {0x0F, 0x70} },
	{0x306E, 2, {0x48, 0x10} },
	{0x3064, 2, {0x18, 0x02} },
	{0x3EF6, 2, {0x80, 0x4D} },
	{0x3180, 2, {0xC0, 0x8F} },
	{0x30BA, 2, {0x76, 0x23} },
	{0x3176, 2, {0x04, 0x80} },
	{0x3178, 2, {0x04, 0x80} },
	{0x317A, 2, {0x04, 0x80} },
/*
	{0x317C, 2, {0x04,0x80}},    //PLL-2
	{0x302A, 2, {0x00,0x06}},  //VT_PIX_CLK_DIV
	{0x302C, 2, {0x00,0x01}},  //VT_SYS_CLK_DIV
	{0x302E, 2, {0x00,0x04}},  //PRE_PLL_CLK_DIV
	{0x3030, 2, {0x00,0x4A}},  //PLL_MULTIPLIER
	{0x3036, 2, {0x00,0x0C}},  //OP_WORD_CLK_DIV
	{0x3038, 2, {0x00,0x01}},  //OP_SYS_CLK_DIV
	{0x30B0, 2, {0x00,0x28}},  //DIGITAL_TEST
	{0x31AC, 2, {0x0C,0x0C}},  //DATA_FORMAT_BITS
	{0x31AE, 2, {0x02,0x02}},  //SERIAL_FORMAT
	//{0x31B0, 2, {0x00,0x2F}},  //frame_preamble
	{0x31B0, 2, {0x00,0x2E}},  //frame_preamble
	//{0x31B0, 2, {0x00,0x38}},  //frame_preamble
	{0x31B2, 2, {0x00,0x1C}},  //line_preamble
	{0x31B4, 2, {0x16,0x65}},  //mipi_timing_0
	{0x31B6, 2, {0x11,0x0D}},  //mipi_timing_1
	{0x31B8, 2, {0x20,0x47}},  //mipi_timing_2
	{0x31BA, 2, {0x01,0x05}},  //mipi_timing_3
	{0x31BC, 2, {0x80,0x04}},  //mipi_timing_4   //mode 1280x802 P60
*/
	{0x317C, 2, {0x04, 0x80} },    //PLL-2
	{0x302A, 2, {0x00, 0x06} },  //VT_PIX_CLK_DIV
	{0x302C, 2, {0x00, 0x01} },  //VT_SYS_CLK_DIV
	{0x302E, 2, {0x00, 0x04} },  //PRE_PLL_CLK_DIV
	{0x3030, 2, {0x00, 0x42} },  //PLL_MULTIPLIER
	{0x3036, 2, {0x00, 0x0C} },  //OP_WORD_CLK_DIV
	{0x3038, 2, {0x00, 0x01} },  //OP_SYS_CLK_DIV

	//{0x31B0, 2, {0x00, 0x2F} },  //frame_preamble
	{0x31B0, 2, {0x00, 0x5A} },  //frame_preamble
	//{0x31B0, 2, {0x00, 0x38} },  //frame_preamble
	{0x31B2, 2, {0x00, 0x2E} },  //line_preamble
	{0x31B4, 2, {0x26, 0x33} },  //mipi_timing_0
	{0x31B6, 2, {0x21, 0x0E} },  //mipi_timing_1
	{0x31B8, 2, {0x20, 0xC7} },  //mipi_timing_2
	{0x31BA, 2, {0x01, 0x05} },  //mipi_timing_3
	{0x31BC, 2, {0x00, 0x04} },  //mipi_timing_4   //mode 1280x802 P60

	{0x31AC, 2, {0x0C, 0x0C} },  //DATA_FORMAT_BITS
	{0x31AE, 2, {0x02, 0x02} },  //SERIAL_FORMAT

	{0x30B0, 2, {0x00, 0x4A} },  //DIGITAL_TEST

	{0x3004, 2, {0x00, 0x04} },  //x_add_start
	{0x3008, 2, {0x05, 0x03} },  //x_add_end
	{0x3002, 2, {0x00, 0x00} },  //y_add_start
	{0x3006, 2, {0x03, 0x21} },  //y_add_end
	{0x3040, 2, {0x00, 0x41} },  //read mode
	{0x30A2, 2, {0x00, 0x01} },
	{0x30A6, 2, {0x00, 0x01} },
	{0x30A8, 2, {0x00, 0x03} },
	{0x30AE, 2, {0x00, 0x03} },
	{0x300C, 2, {0x05, 0xD0} },  //LLPCK
	{0x300A, 2, {0x04, 0x73} },  //FLL
	//{0x300A, 2, {0x03,0x3D}},  //FLL
				//{CMD_SETVD, 2, {0x0000, 0x0000}},
	{0x3012, 2, {0x03, 0x30} },  //coarse_integration_time
				//disable Embedded Data and Stats
	{0x3064, 2, {0x18, 0x02} },
					//{CMD_SETAE, 2, {0x00, 0x00}},
	{0x3028, 2, {0x00, 0x10} },	 //row speed
	{0x301A, 2, {0x00, 0x5C} },  //streaming on
};
static CTL_SEN_CMD ar0144m_gs_2p4_seq[] = {
	{0x3088, 2, {0x80, 0x00} },
	{0x3086, 2, {0x32, 0x7F} },
	{0x3086, 2, {0x57, 0x80} },
	{0x3086, 2, {0x27, 0x30} },
	{0x3086, 2, {0x7E, 0x13} },
	{0x3086, 2, {0x80, 0x00} },
	{0x3086, 2, {0x15, 0x7E} },
	{0x3086, 2, {0x13, 0x80} },
	{0x3086, 2, {0x00, 0x0F} },
	{0x3086, 2, {0x81, 0x90} },
	{0x3086, 2, {0x16, 0x43} },
	{0x3086, 2, {0x16, 0x3E} },
	{0x3086, 2, {0x45, 0x22} },
	{0x3086, 2, {0x09, 0x37} },
	{0x3086, 2, {0x81, 0x90} },
	{0x3086, 2, {0x16, 0x43} },
	{0x3086, 2, {0x16, 0x7F} },
	{0x3086, 2, {0x90, 0x80} },
	{0x3086, 2, {0x00, 0x38} },
	{0x3086, 2, {0x7F, 0x13} },
	{0x3086, 2, {0x80, 0x23} },
	{0x3086, 2, {0x3B, 0x7F} },
	{0x3086, 2, {0x93, 0x45} },
	{0x3086, 2, {0x02, 0x80} },
	{0x3086, 2, {0x00, 0x7F} },
	{0x3086, 2, {0xB0, 0x8D} },
	{0x3086, 2, {0x66, 0x7F} },
	{0x3086, 2, {0x90, 0x81} },
	{0x3086, 2, {0x92, 0x3C} },
	{0x3086, 2, {0x16, 0x35} },
	{0x3086, 2, {0x7F, 0x93} },
	{0x3086, 2, {0x45, 0x02} },
	{0x3086, 2, {0x80, 0x00} },
	{0x3086, 2, {0x7F, 0xB0} },
	{0x3086, 2, {0x8D, 0x66} },
	{0x3086, 2, {0x7F, 0x90} },
	{0x3086, 2, {0x81, 0x82} },
	{0x3086, 2, {0x37, 0x45} },
	{0x3086, 2, {0x02, 0x36} },
	{0x3086, 2, {0x81, 0x80} },
	{0x3086, 2, {0x44, 0x16} },
	{0x3086, 2, {0x31, 0x43} },
	{0x3086, 2, {0x74, 0x16} },
	{0x3086, 2, {0x78, 0x7B} },
	{0x3086, 2, {0x7D, 0x45} },
	{0x3086, 2, {0x02, 0x3D} },
	{0x3086, 2, {0x64, 0x45} },
	{0x3086, 2, {0x0A, 0x3D} },
	{0x3086, 2, {0x64, 0x7E} },
	{0x3086, 2, {0x12, 0x81} },
	{0x3086, 2, {0x80, 0x37} },
	{0x3086, 2, {0x7F, 0x10} },
	{0x3086, 2, {0x45, 0x0A} },
	{0x3086, 2, {0x3F, 0x74} },
	{0x3086, 2, {0x7E, 0x10} },
	{0x3086, 2, {0x7E, 0x12} },
	{0x3086, 2, {0x0F, 0x3D} },
	{0x3086, 2, {0xD2, 0x7F} },
	{0x3086, 2, {0xD4, 0x80} },
	{0x3086, 2, {0x24, 0x82} },
	{0x3086, 2, {0x9C, 0x03} },
	{0x3086, 2, {0x43, 0x0D} },
	{0x3086, 2, {0x2D, 0x46} },
	{0x3086, 2, {0x43, 0x16} },
	{0x3086, 2, {0x5F, 0x16} },
	{0x3086, 2, {0x53, 0x2D} },
	{0x3086, 2, {0x16, 0x60} },
	{0x3086, 2, {0x40, 0x4C} },
	{0x3086, 2, {0x29, 0x04} },
	{0x3086, 2, {0x29, 0x84} },
	{0x3086, 2, {0x81, 0xE7} },
	{0x3086, 2, {0x81, 0x6F} },
	{0x3086, 2, {0x17, 0x0A} },
	{0x3086, 2, {0x81, 0xE7} },
	{0x3086, 2, {0x7F, 0x81} },
	{0x3086, 2, {0x5C, 0x0D} },
	{0x3086, 2, {0x57, 0x49} },
	{0x3086, 2, {0x5F, 0x53} },
	{0x3086, 2, {0x25, 0x53} },
	{0x3086, 2, {0x27, 0x4D} },
	{0x3086, 2, {0x2B, 0xF8} },
	{0x3086, 2, {0x10, 0x16} },
	{0x3086, 2, {0x4C, 0x09} },
	{0x3086, 2, {0x2B, 0xB8} },
	{0x3086, 2, {0x2B, 0x98} },
	{0x3086, 2, {0x4E, 0x11} },
	{0x3086, 2, {0x53, 0x67} },
	{0x3086, 2, {0x40, 0x01} },
	{0x3086, 2, {0x60, 0x5C} },
	{0x3086, 2, {0x09, 0x5C} },
	{0x3086, 2, {0x1B, 0x40} },
	{0x3086, 2, {0x02, 0x45} },
	{0x3086, 2, {0x00, 0x45} },
	{0x3086, 2, {0x80, 0x29} },
	{0x3086, 2, {0xB6, 0x7F} },
	{0x3086, 2, {0x80, 0x40} },
	{0x3086, 2, {0x04, 0x7F} },
	{0x3086, 2, {0x88, 0x41} },
	{0x3086, 2, {0x09, 0x5C} },
	{0x3086, 2, {0x0B, 0x29} },
	{0x3086, 2, {0xB2, 0x41} },
	{0x3086, 2, {0x0C, 0x40} },
	{0x3086, 2, {0x03, 0x40} },
	{0x3086, 2, {0x13, 0x5C} },
	{0x3086, 2, {0x03, 0x41} },
	{0x3086, 2, {0x11, 0x17} },
	{0x3086, 2, {0x12, 0x5F} },
	{0x3086, 2, {0x2B, 0x90} },
	{0x3086, 2, {0x2B, 0x80} },
	{0x3086, 2, {0x81, 0x6F} },
	{0x3086, 2, {0x40, 0x10} },
	{0x3086, 2, {0x41, 0x01} },
	{0x3086, 2, {0x53, 0x27} },
	{0x3086, 2, {0x40, 0x01} },
	{0x3086, 2, {0x60, 0x29} },
	{0x3086, 2, {0xA3, 0x5F} },
	{0x3086, 2, {0x4D, 0x1C} },
	{0x3086, 2, {0x17, 0x02} },
	{0x3086, 2, {0x81, 0xE7} },
	{0x3086, 2, {0x29, 0x83} },
	{0x3086, 2, {0x45, 0x88} },
	{0x3086, 2, {0x40, 0x21} },
	{0x3086, 2, {0x7F, 0x8A} },
	{0x3086, 2, {0x40, 0x39} },
	{0x3086, 2, {0x45, 0x80} },
	{0x3086, 2, {0x24, 0x40} },
	{0x3086, 2, {0x08, 0x7F} },
	{0x3086, 2, {0x88, 0x5D} },
	{0x3086, 2, {0x53, 0x67} },
	{0x3086, 2, {0x29, 0x92} },
	{0x3086, 2, {0x88, 0x10} },
	{0x3086, 2, {0x2B, 0x04} },
	{0x3086, 2, {0x89, 0x16} },
	{0x3086, 2, {0x5C, 0x43} },
	{0x3086, 2, {0x86, 0x17} },
	{0x3086, 2, {0x0B, 0x5C} },
	{0x3086, 2, {0x03, 0x8A} },
	{0x3086, 2, {0x48, 0x4D} },
	{0x3086, 2, {0x4E, 0x2B} },
	{0x3086, 2, {0x80, 0x4C} },
	{0x3086, 2, {0x0B, 0x41} },
	{0x3086, 2, {0x9F, 0x81} },
	{0x3086, 2, {0x6F, 0x41} },
	{0x3086, 2, {0x10, 0x40} },
	{0x3086, 2, {0x01, 0x53} },
	{0x3086, 2, {0x27, 0x40} },
	{0x3086, 2, {0x01, 0x60} },
	{0x3086, 2, {0x29, 0x83} },
	{0x3086, 2, {0x29, 0x43} },
	{0x3086, 2, {0x5C, 0x05} },
	{0x3086, 2, {0x5F, 0x4D} },
	{0x3086, 2, {0x1C, 0x81} },
	{0x3086, 2, {0xE7, 0x45} },
	{0x3086, 2, {0x02, 0x81} },
	{0x3086, 2, {0x80, 0x7F} },
	{0x3086, 2, {0x80, 0x41} },
	{0x3086, 2, {0x0A, 0x91} },
	{0x3086, 2, {0x44, 0x16} },
	{0x3086, 2, {0x09, 0x2F} },
	{0x3086, 2, {0x7E, 0x37} },
	{0x3086, 2, {0x80, 0x20} },
	{0x3086, 2, {0x30, 0x7E} },
	{0x3086, 2, {0x37, 0x80} },
	{0x3086, 2, {0x20, 0x15} },
	{0x3086, 2, {0x7E, 0x37} },
	{0x3086, 2, {0x80, 0x20} },
	{0x3086, 2, {0x03, 0x43} },
	{0x3086, 2, {0x16, 0x4A} },
	{0x3086, 2, {0x0A, 0x43} },
	{0x3086, 2, {0x16, 0x0B} },
	{0x3086, 2, {0x43, 0x16} },
	{0x3086, 2, {0x8F, 0x43} },
	{0x3086, 2, {0x16, 0x90} },
	{0x3086, 2, {0x43, 0x16} },
	{0x3086, 2, {0x7F, 0x81} },
	{0x3086, 2, {0x45, 0x0A} },
	{0x3086, 2, {0x41, 0x30} },
	{0x3086, 2, {0x7F, 0x83} },
	{0x3086, 2, {0x5D, 0x29} },
	{0x3086, 2, {0x44, 0x88} },
	{0x3086, 2, {0x10, 0x2B} },
	{0x3086, 2, {0x04, 0x53} },
	{0x3086, 2, {0x2D, 0x40} },
	{0x3086, 2, {0x30, 0x45} },
	{0x3086, 2, {0x02, 0x40} },
	{0x3086, 2, {0x08, 0x7F} },
	{0x3086, 2, {0x80, 0x53} },
	{0x3086, 2, {0x2D, 0x89} },
	{0x3086, 2, {0x16, 0x5C} },
	{0x3086, 2, {0x45, 0x86} },
	{0x3086, 2, {0x17, 0x0B} },
	{0x3086, 2, {0x5C, 0x05} },
	{0x3086, 2, {0x8A, 0x60} },
	{0x3086, 2, {0x4B, 0x91} },
	{0x3086, 2, {0x44, 0x16} },
	{0x3086, 2, {0x09, 0x15} },
	{0x3086, 2, {0x3D, 0xFF} },
	{0x3086, 2, {0x3D, 0x87} },
	{0x3086, 2, {0x7E, 0x3D} },
	{0x3086, 2, {0x7E, 0x19} },
	{0x3086, 2, {0x80, 0x00} },
	{0x3086, 2, {0x8B, 0x1F} },
	{0x3086, 2, {0x2A, 0x1F} },
	{0x3086, 2, {0x83, 0xA2} },
	{0x3086, 2, {0x7E, 0x11} },
	{0x3086, 2, {0x75, 0x16} },
	{0x3086, 2, {0x33, 0x45} },
	{0x3086, 2, {0x0A, 0x7F} },
	{0x3086, 2, {0x53, 0x80} },
	{0x3086, 2, {0x23, 0x8C} },
	{0x3086, 2, {0x66, 0x7F} },
	{0x3086, 2, {0x13, 0x81} },
	{0x3086, 2, {0x84, 0x14} },
	{0x3086, 2, {0x81, 0x80} },
	{0x3086, 2, {0x31, 0x3D} },
	{0x3086, 2, {0x64, 0x45} },
	{0x3086, 2, {0x2A, 0x3D} },
	{0x3086, 2, {0xD2, 0x7F} },
	{0x3086, 2, {0x44, 0x80} },
	{0x3086, 2, {0x24, 0x94} },
	{0x3086, 2, {0x3D, 0xFF} },
	{0x3086, 2, {0x3D, 0x4D} },
	{0x3086, 2, {0x45, 0x02} },
	{0x3086, 2, {0x7F, 0xD0} },
	{0x3086, 2, {0x80, 0x00} },
	{0x3086, 2, {0x8C, 0x66} },
	{0x3086, 2, {0x7F, 0x90} },
	{0x3086, 2, {0x81, 0x94} },
	{0x3086, 2, {0x3F, 0x44} },
	{0x3086, 2, {0x16, 0x81} },
	{0x3086, 2, {0x84, 0x16} },
	{0x3086, 2, {0x2C, 0x2C} },
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

static CTL_SEN_DRV_TAB ar0144_sen_drv_tab = {
	sen_open_ar0144,
	sen_close_ar0144,
	sen_sleep_ar0144,
	sen_wakeup_ar0144,
	sen_write_reg_ar0144,
	sen_read_reg_ar0144,
	sen_chg_mode_ar0144,
	sen_chg_fps_ar0144,
	sen_set_info_ar0144,
	sen_get_info_ar0144,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_ar0144(void)
{
	return &ar0144_sen_drv_tab;
}

static void sen_pwr_ctrl_ar0144(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
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
				if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK) {
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
				if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK) {
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

static CTL_SEN_CMD sen_set_cmd_info_ar0144(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_ar0144(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_ar0144(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_ar0144);
	#endif

	rt = sen_i2c_init_driver(id, &sen_i2c[id]);

	if (rt != E_OK) {
		i2c_valid[id] = FALSE;

		DBG_ERR("init. i2c driver fail (%d) \r\n", id);
	} else {
		i2c_valid[id] = TRUE;
	}
	return rt;
}

static ER sen_close_ar0144(CTL_SEN_ID id)
{
	if (i2c_valid[id]) {
		sen_i2c_remove_driver(id);
		i2c_valid[id] = FALSE;
	}

	return E_OK;
}

static ER sen_sleep_ar0144(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_ar0144(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_ar0144(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	struct i2c_msg msgs;
	unsigned char buf[4];
	int i;

	if (!i2c_valid[id]) {
		return E_NOSPT;
	}

	buf[0]     = (cmd->addr >> 8) & 0xFF;
	buf[1]     = cmd->addr & 0xFF;
//	buf[2]     = (cmd->data[0] >> 8) & 0xFF;
//	buf[3]     = cmd->data[0] & 0xFF;
	buf[2]     = cmd->data[0] & 0xFF;
	buf[3]     = cmd->data[1] & 0xFF;
	msgs.addr  = sen_i2c[id].addr;
	msgs.flags = 0;
	msgs.len   = 4;
	msgs.buf   = buf;

	i = 0;
	while (1) {
		if (sen_i2c_transfer(id, &msgs, 1) == 0)
			break;
		i++;
		if (i == 5) {
			//DBG_ERR("%x %x %x\r\n", msgs.addr, buf[2], buf[3]);
			return E_SYS;
		}
	}

	return E_OK;
}

static ER sen_read_reg_ar0144(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

	i = 0;
	while (1) {
		if (sen_i2c_transfer(id, msgs, 2) == 0)
			break;
		i++;
		if (i == 5) {
			//DBG_ERR("\r\n");
			return E_SYS;
			}
	}

	cmd->data[0] = tmp2[0];
	cmd->data[1] = tmp2[1];

	return E_OK;
}

static UINT32 sen_get_cmd_tab_ar0144(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = ar0144_mode_1;
		return sizeof(ar0144_mode_1) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}
static UINT32 sen_get_cmd_tab_gs_2p4(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{

		*cmd_tab = ar0144m_gs_2p4_seq;
		return sizeof(ar0144m_gs_2p4_seq) / sizeof(CTL_SEN_CMD);


}
static ER sen_chg_mode_ar0144(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{
	ISP_SENSOR_CTRL sensor_ctrl = {0};
	CTL_SEN_CMD *p_cmd_list = NULL, cmd;
	CTL_SEN_FLIP flip = CTL_SEN_FLIP_NONE;
	UINT32 sensor_vd;
	UINT32 idx, cmd_num = 0;
	ER rt = E_OK;

	cur_sen_mode[id] = chgmode_obj.mode;

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_ar0144(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_ar0144(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_ar0144(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_ar0144(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}
	//get 0x300E
	{
		UINT32 reg = 0x0, pre_cmd_num = 0;
		CTL_SEN_CMD *p_pre_cmd_list = NULL;

		cmd = sen_set_cmd_info_ar0144(0x300E, 2, 0x00, 0x00);
		rt |= sen_read_reg_ar0144(id, &cmd);
		reg = (cmd.data[0]<<8)|cmd.data[1];

		if ((reg != 0x3040) && (reg != 0x4040)) {
			pre_cmd_num = sen_get_cmd_tab_gs_2p4(chgmode_obj.mode, &p_pre_cmd_list);

			if (p_pre_cmd_list != NULL)	{
				for (idx = 0; idx < pre_cmd_num; idx++) {
					cmd = sen_set_cmd_info_ar0144(p_pre_cmd_list[idx].addr, p_pre_cmd_list[idx].data_len, p_pre_cmd_list[idx].data[0], p_pre_cmd_list[idx].data[1]);
					rt |= sen_write_reg_ar0144(id, &cmd);
				}
			}
		}
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_ar0144(0x300A, 2, ((sensor_vd>>8) & 0xFF), (sensor_vd& 0xFF));
			rt |= sen_write_reg_ar0144(id, &cmd);
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

			sen_set_gain_ar0144(id, &sensor_ctrl);
			sen_set_expt_ar0144(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_ar0144(id, &flip);
		} else {
			cmd = sen_set_cmd_info_ar0144(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_ar0144(id, &cmd);
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_ar0144(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_ar0144(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_ar0144(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_ar0144(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}


	cmd = sen_set_cmd_info_ar0144(0x300A, 2, ((sensor_vd>>8) & 0xFF), (sensor_vd& 0xFF));
	rt |= sen_write_reg_ar0144(id, &cmd);

	return rt;
}

static ER sen_set_info_ar0144(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_ar0144(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_ar0144(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_ar0144(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_ar0144(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_ar0144(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_ar0144(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_ar0144(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_ar0144((CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_ar0144((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_ar0144(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_ar0144((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_ar0144(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_ar0144(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_ar0144((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
/*	case CTL_SENDRV_CFGID_GET_MODE_PARA:
		sen_get_mode_para_ar0144((CTL_SENDRV_GET_MODE_PARA_PARAM *)(data));
		break;*/
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_ar0144((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_ar0144((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_ar0144(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_ar0144(id, data);
		break;
/*	case CTL_SENDRV_CFGID_GET_MODE_MIPI_MANUAL_IADJ:
		sen_get_iadj_ar0144(id, (CTL_SENDRV_GET_MODE_MANUAL_IADJ *)(data));
		break;*/
	default:
		rt = E_NOSPT;
	}
	return rt;
}
/*
static void sen_get_iadj_ar0144(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_MANUAL_IADJ *data)
{
	data->mode = cur_sen_mode[id];
	data->sel = CTL_SEN_MANUAL_IADJ_SEL_IADJ;
	data->val.iadj = 3;
}*/
static UINT32 sen_calc_chgmode_vd_ar0144(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	sen_set_chgmode_fps_ar0144(id, fps);
	sen_set_cur_fps_ar0144(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_ar0144(id, fps);
		sen_set_cur_fps_ar0144(id, fps);
	}

	if (sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_ar0144(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_ar0144(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_ar0144(CTL_SEN_ID id, UINT32 fps)
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

	if (sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	return sensor_vd;
}

typedef struct gain_set {
	UINT16 a_gain_reg;
	UINT16 a_gain;
	UINT32 total_gain;
} gain_set_t;

static gain_set_t gain_table[] = // gain*1000
{
	{	10,	1454,	1684},
	{	11,	1523,	1738},
	{	12,	1600,	1825},
	{	13,	1684,	1921},
	{	14,	1777,	2028},
	{	15,	1882,	2147},
	{	16,	2000,	2282},
	{	18,	2133,	2434},
	{	20,	2285,	2608},
	{	22,	2461,	2808},
	{	24,	2666,	3042},
	{	26,	2909,	3319},
	{	28,	3200,	3651},
	{	31,	3555,	4056},
	{	32,	4625,	4625},
	{	33,	4774,	4774},
	{	34,	4933,	4933},
	{	35,	5103,	5103},
	{	36,	5285,	5285},
	{	37,	5481,	5481},
	{	38,	5692,	5692},
	{	39,	5920,	5920},
	{	40,	6166,	6166},
	{	41,	6434,	6434},
	{	42,	6727,	6727},
	{	43,	7047,	7047},
	{	44,	7400,	7400},
	{	45,	7789,	7789},
	{	46,	8222,	8222},
	{	47,	8706,	8706},
	{	48,	9250,	9250},
	{	50,	9866,	9866},
	{	52,	10571,	10571},
	{	54,	11384,	11384},
	{	56,	12333,	12333},
	{	58,	13454,	13454},
	{	60,	14800,	14800},
	{	62,	16444,	16444},
	{	64,	18500,	18500},

};


#define NUM_OF_GAINSET    (sizeof(gain_table) / sizeof(gain_set_t))
static void sen_set_gain_ar0144(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 i, frame_cnt, total_frame, step, data1 = 0, data2 = 0, gain_ratio;
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	// Calculate sensor gain
	if (mode_basic_param[cur_sen_mode[id]].frame_num == 0) {
		DBG_WRN("total_frame = 0, force to 1 \r\n");
		total_frame = 1;
	} else {
		total_frame = mode_basic_param[cur_sen_mode[id]].frame_num;
	}

	for (frame_cnt = 0; frame_cnt < total_frame; frame_cnt++) {
		if (sensor_ctrl->gain_ratio[frame_cnt] < mode_basic_param[cur_sen_mode[id]].gain.min) {
			sensor_ctrl->gain_ratio[frame_cnt] = mode_basic_param[cur_sen_mode[id]].gain.min;
		} else if (sensor_ctrl->gain_ratio[frame_cnt] > mode_basic_param[cur_sen_mode[id]].gain.max) {
			sensor_ctrl->gain_ratio[frame_cnt] = mode_basic_param[cur_sen_mode[id]].gain.max;
		}

		if (sensor_ctrl->gain_ratio[frame_cnt] > 154213) {
			gain_ratio = 154213;
		} else if (sensor_ctrl->gain_ratio[frame_cnt] < 1000) {
			gain_ratio = 1000;
		} else{
			gain_ratio = sensor_ctrl->gain_ratio[frame_cnt];
		}

		gain_ratio = gain_ratio*gain_table[0].total_gain/1000;
		//Search most suitable gain into gain table
		if (gain_ratio >= gain_table[NUM_OF_GAINSET-1].total_gain){
			step = NUM_OF_GAINSET;
		}else{
			for (i = 0; i < NUM_OF_GAINSET; i++) {
				step = i;

				if (gain_table[i].total_gain >= gain_ratio) {
					break;
				}
			}
		}

		//Collect the gain setting
		if (step == 0) {
			data1 = gain_table[0].a_gain_reg;
			data2 = 0x92;
		} else {
			data1 = gain_table[step-1].a_gain_reg;
			//data2 = 0x92;
			data2 = gain_ratio*128/gain_table[step-1].a_gain;
		}
	}

	//Set analog gain
	cmd = sen_set_cmd_info_ar0144(0x3060, 2, ((data1>>8) & 0xFF), (data1& 0xFF));
	rt = sen_write_reg_ar0144(id, &cmd);

	//Set digital gain
	cmd = sen_set_cmd_info_ar0144(0x305E, 2, ((data2>>8) & 0xFF), (data2& 0xFF));
	rt |= sen_write_reg_ar0144(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_expt_ar0144(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps, cur_fps = 0, clac_fps = 0;
	UINT32 t_row = 0;
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
		t_row = sen_calc_rowtime_ar0144(id, cur_sen_mode[id]);
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
	chgmode_fps = sen_get_chgmode_fps_ar0144(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_ar0144(id, cur_sen_mode[id]);
	expt_time = (line[0]) * t_row / 10;

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_ar0144(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_ar0144(id, cur_fps);

	if (sensor_vd > (MAX_VD_PERIOD)) {
		DBG_ERR("max vts overflow\r\n");
		sensor_vd = MAX_VD_PERIOD;
	}

	//Set vts line

	cmd = sen_set_cmd_info_ar0144(0x300A, 2, ((sensor_vd>>8) & 0xFF), (sensor_vd& 0xFF));
	rt |= sen_write_reg_ar0144(id, &cmd);


	if (line[0] > MAX_EXPOSURE_LINE) {
		DBG_ERR("max line overflow \r\n");
		line[0] = MAX_EXPOSURE_LINE;
	}

	if (line[0] < MIN_EXPOSURE_LINE) {
		DBG_ERR("min line overflow\r\n");
		line[0] = MIN_EXPOSURE_LINE;
	}

	if (line[0] > sensor_vd - NON_EXPOSURE_LINE) {
		line[0] = sensor_vd - NON_EXPOSURE_LINE;
	}

	//Set exp line

	cmd = sen_set_cmd_info_ar0144(0x3012, 2, ((line[0]>>8) & 0xFF), (line[0]& 0xFF));
	rt |= sen_write_reg_ar0144(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_ar0144(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_ar0144(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	DBG_IND("enter \r\n");
}

static void sen_get_gain_ar0144(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

static void sen_get_expt_ar0144(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_ar0144(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row;

	t_row = sen_calc_rowtime_ar0144(id, cur_sen_mode[id]);
	*min_exp_time = t_row * MIN_EXPOSURE_LINE / 10 + 1;
}

static void sen_get_mode_basic_ar0144(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_ar0144(CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_ar0144(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_ar0144(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_ar0144(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
{
	#if 1
	if (data->type == CTL_SEN_IF_TYPE_PARALLEL) {
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

static void sen_get_fps_ar0144(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_ar0144(id);
	data->chg_fps = sen_get_chgmode_fps_ar0144(id);
}

static void sen_get_speed_ar0144(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &speed_param[mode], sizeof(CTL_SENDRV_GET_SPEED_PARAM));

	if (sen_power[id].mclk == CTL_SEN_CLK_SEL_SIEMCLK) {
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

static void sen_get_mode_mipi_ar0144(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;

	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_modesel_ar0144(CTL_SENDRV_GET_MODESEL_PARAM *data)
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
				data->mode = CTL_SEN_MODE_1;
				return;


	}

	DBG_ERR("fail (frame_rate%d,size%d*%d,if_type%d,data_fmt%d,frame_num%d) \r\n"
			, data->frame_rate, data->size.w, data->size.h, data->if_type, data->data_fmt, data->frame_num);
	data->mode = CTL_SEN_MODE_1;
}

static UINT32 sen_calc_rowtime_step_ar0144(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 div_step = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	if (mode_basic_param[mode].mode_type == CTL_SEN_MODE_LINEAR) {
		div_step = 1;
	} /* else {
		div_step = 1;
	}
*/
	return div_step;
}

static UINT32 sen_calc_rowtime_ar0144(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 100 * (mode_basic_param[mode].signal_info.hd_period) / ((speed_param[mode].pclk) / 100000);

	return row_time;
}

static void sen_get_rowtime_ar0144(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_ar0144(id, data->mode);
	data->row_time = sen_calc_rowtime_ar0144(id, data->mode) * (data->row_time_step);
}

static void sen_set_cur_fps_ar0144(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_ar0144(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_ar0144(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_ar0144(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
int sen_init_ar0144(SENSOR_DTSI_INFO *info)
{
	CTL_SEN_REG_OBJ reg_obj;
	CHAR node_path[64];
	CHAR compatible[64];
	ER rt = E_OK;

	sprintf(compatible, "nvt,sen_ar0144");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_ar0144");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);

	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_ar0144;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_ar0144();
	rt = ctl_sen_reg_sendrv("nvt_sen_ar0144", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_ar0144(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_ar0144");
}

#else
static int __init sen_init_ar0144(void)
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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_ar0144;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_ar0144();
	rt = ctl_sen_reg_sendrv("nvt_sen_ar0144", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_ar0144(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_ar0144");
}

module_init(sen_init_ar0144);
module_exit(sen_exit_ar0144);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_AR0144_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

