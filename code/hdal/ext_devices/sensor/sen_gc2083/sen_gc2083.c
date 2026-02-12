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
VOS_MODULE_VERSION(nvt_sen_gc2083, 1, 52, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_GC2083_MODULE_NAME     "sen_gc2083"
#define SEN_MAX_MODE                 1
#define MAX_VD_PERIOD                0x3FFF
#define MAX_EXPOSURE_LINE            0x3FFF
#define MIN_EXPOSURE_LINE            1
#define NON_EXPOSURE_LINE            8

#define SEN_I2C_ADDR 0x6E >> 1
#define SEN_I2C_COMPATIBLE "nvt,sen_gc2083"

#include "sen_i2c.c"

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_gc2083(void);
static void sen_pwr_ctrl_gc2083(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_gc2083(CTL_SEN_ID id);
static ER sen_close_gc2083(CTL_SEN_ID id);
static ER sen_sleep_gc2083(CTL_SEN_ID id);
static ER sen_wakeup_gc2083(CTL_SEN_ID id);
static ER sen_write_reg_gc2083(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_gc2083(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_gc2083(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_gc2083(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_gc2083(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_gc2083(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_gc2083(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_gc2083(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_gc2083(CTL_SEN_ID id, void *param);
static void sen_set_expt_gc2083(CTL_SEN_ID id, void *param);
static void sen_set_preset_gc2083(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_gc2083(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_gc2083(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
#if defined(__FREERTOS)
void sen_get_gain_gc2083(CTL_SEN_ID id, void *param);
void sen_get_expt_gc2083(CTL_SEN_ID id, void *param);
#else
static void sen_get_gain_gc2083(CTL_SEN_ID id, void *param);
static void sen_get_expt_gc2083(CTL_SEN_ID id, void *param);
#endif
static void sen_get_min_expt_gc2083(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_gc2083(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_gc2083(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_gc2083(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_gc2083(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_gc2083(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_gc2083(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_gc2083(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_mipi_gc2083(CTL_SENDRV_GET_MODE_MIPI_PARAM *data);
static void sen_get_modesel_gc2083(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_gc2083(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_gc2083(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_gc2083(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_gc2083(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_gc2083(CTL_SEN_ID id);
static void sen_set_chgmode_fps_gc2083(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_gc2083(CTL_SEN_ID id);
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
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR},
	{SEN_I2C_ID_1, SEN_I2C_ADDR}
};

static CTL_SENDRV_GET_ATTR_BASIC_PARAM basic_param = {
	SEN_GC2083_MODULE_NAME,
	CTL_SEN_VENDOR_OTHERS,
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
		{CTL_SEN_I2C_W_ADDR_DFT,     0x6E},
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
		59400000,
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
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{1928, 1087}, //if flip = height - 1 = 1088 - 1  
		{{4, 4, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{1920, 1080},	
		{0, 2198, 0, 1125},
		CTL_SEN_RATIO(16, 9),
		{1000, 2005032},
		100
	}
};

static CTL_SEN_CMD gc2083_mode_1[] = {
	//window size:1928x1088
	//mclk=27mhz,mipi_rate=594Mbps/lane
	//rowtime=29.61616us
	//frame rate:30fps,
	//hts=2198,vts=1125,
	//bayer mode:RGGB
	///****system****/
	{0x03fe, 1, {0xf0, 0x0}},
	{0x03fe, 1, {0xf0, 0x0}},
	{0x03fe, 1, {0xf0, 0x0}},
	{0x03fe, 1, {0x00, 0x0}},
	{0x03f2, 1, {0x00, 0x0}},
	{0x03f3, 1, {0x00, 0x0}},
	{0x03f4, 1, {0x36, 0x0}},
	{0x03f5, 1, {0xc0, 0x0}},
	{0x03f6, 1, {0x24, 0x0}},
	{0x03f7, 1, {0x01, 0x0}},
	{0x03f8, 1, {0x2c, 0x0}},
	{0x03f9, 1, {0x43, 0x0}},
	{0x03fc, 1, {0x8e, 0x0}},
	{0x0381, 1, {0x07, 0x0}},
	{0x00d7, 1, {0x29, 0x0}},
	/****CISCTL & ANALOG****/
	{0x0d6d, 1, {0x18, 0x0}},
	{0x00d5, 1, {0x03, 0x0}},
	{0x0082, 1, {0x01, 0x0}},
	{0x0db3, 1, {0xd4, 0x0}},
	{0x0db0, 1, {0x0d, 0x0}},
	{0x0db5, 1, {0x96, 0x0}},
	{0x0d03, 1, {0x02, 0x0}},
	{0x0d04, 1, {0x02, 0x0}},
	{0x0d05, 1, {0x05, 0x0}},
	{0x0d06, 1, {0xba, 0x0}},
	{0x0d07, 1, {0x00, 0x0}},
	{0x0d08, 1, {0x11, 0x0}},
	{0x0d09, 1, {0x00, 0x0}},
	{0x0d0a, 1, {0x02, 0x0}},
	{0x000b, 1, {0x00, 0x0}},
	{0x000c, 1, {0x02, 0x0}},
	{0x0d0d, 1, {0x04, 0x0}},
	{0x0d0e, 1, {0x40, 0x0}},
	{0x000f, 1, {0x07, 0x0}},
	{0x0010, 1, {0x90, 0x0}},
	{0x0017, 1, {0x0c, 0x0}},
	{0x0d73, 1, {0x92, 0x0}},
	{0x0d76, 1, {0x00, 0x0}},
	{0x0076, 1, {0x00, 0x0}},
	{0x0d41, 1, {0x04, 0x0}},
	{0x0d42, 1, {0x65, 0x0}},
	{0x0d7a, 1, {0x10, 0x0}},
	{0x0d19, 1, {0x31, 0x0}},
	{0x0d25, 1, {0x0b, 0x0}},
	{0x0d20, 1, {0x60, 0x0}},
	{0x0d27, 1, {0x03, 0x0}},
	{0x0d29, 1, {0x60, 0x0}},
	{0x0d43, 1, {0x10, 0x0}},
	{0x0d49, 1, {0x10, 0x0}},
	{0x0d55, 1, {0x18, 0x0}},
	{0x0dc2, 1, {0x44, 0x0}},
	{0x0058, 1, {0x3c, 0x0}},
	{0x00d8, 1, {0x68, 0x0}},
	{0x00d9, 1, {0x14, 0x0}},
	{0x00da, 1, {0xc1, 0x0}},
	{0x0050, 1, {0x18, 0x0}},
	{0x0db6, 1, {0x3d, 0x0}},
	{0x00d2, 1, {0xbc, 0x0}},
	{0x0d66, 1, {0x42, 0x0}},
	{0x008c, 1, {0x07, 0x0}},
	{0x008d, 1, {0xff, 0x0}},
	/*gain*/
	{0x007a, 1, {0x50, 0x0}},//global gain
	{0x00d0, 1, {0x00, 0x0}},
	{0x0dc1, 1, {0x00, 0x0}},
	/*isp*/
	{0x0102, 1, {0xa9, 0x0}},//89
	{0x0158, 1, {0x00, 0x0}},
	{0x0107, 1, {0xa6, 0x0}},
	{0x0108, 1, {0xa9, 0x0}},
	{0x0109, 1, {0xa8, 0x0}},
	{0x010a, 1, {0xa7, 0x0}},
	{0x010b, 1, {0xff, 0x0}},
	{0x010c, 1, {0xff, 0x0}},
	{0x0428, 1, {0x86, 0x0}},
	{0x0429, 1, {0x86, 0x0}},
	{0x042a, 1, {0x86, 0x0}},
	{0x042b, 1, {0x68, 0x0}},
	{0x042c, 1, {0x68, 0x0}},
	{0x042d, 1, {0x68, 0x0}},
	{0x042e, 1, {0x68, 0x0}},
	{0x042f, 1, {0x68, 0x0}},
	{0x0430, 1, {0x4f, 0x0}},
	{0x0431, 1, {0x68, 0x0}},
	{0x0432, 1, {0x67, 0x0}},
	{0x0433, 1, {0x66, 0x0}},
	{0x0434, 1, {0x66, 0x0}},
	{0x0435, 1, {0x66, 0x0}},
	{0x0436, 1, {0x66, 0x0}},
	{0x0437, 1, {0x66, 0x0}},
	{0x0438, 1, {0x62, 0x0}},
	{0x0439, 1, {0x62, 0x0}},
	{0x043a, 1, {0x62, 0x0}},
	{0x043b, 1, {0x62, 0x0}},
	{0x043c, 1, {0x62, 0x0}},
	{0x043d, 1, {0x62, 0x0}},
	{0x043e, 1, {0x62, 0x0}},
	{0x043f, 1, {0x62, 0x0}},
	///*dark sun*/
	{0x0077, 1, {0x01, 0x0}},
	{0x0078, 1, {0x65, 0x0}},
	{0x0079, 1, {0x04, 0x0}},
	{0x0067, 1, {0xa0, 0x0}},
	{0x0054, 1, {0xff, 0x0}},
	{0x0055, 1, {0x02, 0x0}},
	{0x0056, 1, {0x00, 0x0}},
	{0x0057, 1, {0x04, 0x0}},
	{0x005a, 1, {0xff, 0x0}},
	{0x005b, 1, {0x07, 0x0}},
	/*blk*/
	{0x0026, 1, {0x01, 0x0}},
	{0x0152, 1, {0x02, 0x0}},
	{0x0153, 1, {0x50, 0x0}},
	{0x0155, 1, {0x93, 0x0}},
	{0x0410, 1, {0x16, 0x0}},
	{0x0411, 1, {0x16, 0x0}},
	{0x0412, 1, {0x16, 0x0}},
	{0x0413, 1, {0x16, 0x0}},
	{0x0414, 1, {0x6f, 0x0}},
	{0x0415, 1, {0x6f, 0x0}},
	{0x0416, 1, {0x6f, 0x0}},
	{0x0417, 1, {0x6f, 0x0}},
	{0x04e0, 1, {0x18, 0x0}},
	/*window*/
	{0x0192, 1, {0x00, 0x0}},
	{0x0194, 1, {0x00, 0x0}},
	{0x0195, 1, {0x04, 0x0}},
	{0x0196, 1, {0x40, 0x0}},
	{0x0197, 1, {0x07, 0x0}},
	{0x0198, 1, {0x88, 0x0}},
	{SEN_CMD_SETVD, 1, {0x00, 0x0}},
	{SEN_CMD_PRESET, 1, {0x00, 0x0}},
	{SEN_CMD_DIRECTION, 1, {0x00, 0x0}},		
	/****DVP & MIPI****/
	{0x0201, 1, {0x27, 0x0}},
	{0x0202, 1, {0x53, 0x0}},//0x50 
	{0x0203, 1, {0xce, 0x0}},//0xb6//0x8e
	{0x0204, 1, {0x40, 0x0}},
	{0x0212, 1, {0x07, 0x0}},
	{0x0213, 1, {0x80, 0x0}},
	{0x0215, 1, {0x12, 0x0}},
	{0x0229, 1, {0x05, 0x0}},
	{0x0237, 1, {0x03, 0x0}},
	{0x023e, 1, {0x99, 0x0}},
};

typedef struct gain_set {
    UINT8  gain_reg0;
    UINT8  gain_reg1;
    UINT8  gain_reg2;
    UINT8  gain_reg3;
    UINT8  gain_reg4;
    UINT8  gain_reg5;
    UINT8  gain_reg6;
    UINT8  gain_reg7;
    UINT8  gain_reg8;
    UINT8  gain_reg9;
    UINT8  gain_reg10;
    UINT8  gain_reg11;
    UINT8  gain_reg12;
    UINT32  total_gain;
} gain_set_t;

static gain_set_t gain_table[] = 
{
	//0x00d0 0x0155 0x0410 0x0411 0x0412 0x0413 0x0414 0x0415 0x0416 0x0417 0x00b8 0x00b9 0x0dc1
	{0x00, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x01, 0x00, 0x00,   1000},
	{0x10, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x01, 0x0c, 0x00,   1187},
	{0x01, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x01, 0x1a, 0x00,   1406},
	{0x11, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x01, 0x2b, 0x00,   1671},
	{0x02, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x02, 0x00, 0x00,   2000},
	{0x12, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x02, 0x18, 0x00,   2375},
	{0x03, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x02, 0x33, 0x00,   2796},
	{0x13, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x03, 0x15, 0x00,   3328},
	{0x04, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x04, 0x00, 0x00,   4000},
	{0x14, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x04, 0xe0, 0x00,   4765},
	{0x05, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x05, 0x26, 0x00,   5593},
	{0x15, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x06, 0x2b, 0x00,   6671},
	{0x44, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x08, 0x00, 0x00,   8000},
	{0x54, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x09, 0x22, 0x00,   9531},
	{0x45, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x0b, 0x0d, 0x00,  11203},
	{0x55, 0x03, 0x11, 0x11, 0x11, 0x11, 0x6f, 0x6f, 0x6f, 0x6f, 0x0d, 0x16, 0x00,  13343},
	{0x04, 0x19, 0x16, 0x16, 0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f, 0x10, 0x00, 0x01,  16000},
	{0x14, 0x19, 0x16, 0x16, 0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f, 0x13, 0x04, 0x01,  19062},
	{0x24, 0x19, 0x16, 0x16, 0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f, 0x16, 0x1a, 0x01,  22406},
	{0x34, 0x19, 0x16, 0x16, 0x16, 0x16, 0x6f, 0x6f, 0x6f, 0x6f, 0x1a, 0x2b, 0x01,  26671},
	{0x44, 0x36, 0x18, 0x18, 0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f, 0x20, 0x00, 0x01,  32000},
	{0x54, 0x36, 0x18, 0x18, 0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f, 0x26, 0x07, 0x01,  38109},
	{0x64, 0x36, 0x18, 0x18, 0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f, 0x2c, 0x33, 0x01,  44796},
	{0x74, 0x36, 0x18, 0x18, 0x18, 0x18, 0x6f, 0x6f, 0x6f, 0x6f, 0x35, 0x17, 0x01,  53359},
	{0x84, 0x64, 0x16, 0x16, 0x16, 0x16, 0x72, 0x72, 0x72, 0x72, 0x35, 0x17, 0x01,  64000},
	{0x94, 0x64, 0x16, 0x16, 0x16, 0x16, 0x72, 0x72, 0x72, 0x72, 0x35, 0x17, 0x01,  76218},
	{0x85, 0x64, 0x16, 0x16, 0x16, 0x16, 0x72, 0x72, 0x72, 0x72, 0x35, 0x17, 0x01,  89593},
	{0x95, 0x64, 0x16, 0x16, 0x16, 0x16, 0x72, 0x72, 0x72, 0x72, 0x35, 0x17, 0x01, 106718},
	{0xa5, 0x64, 0x16, 0x16, 0x16, 0x16, 0x72, 0x72, 0x72, 0x72, 0x35, 0x17, 0x01, 125437},
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

static CTL_SEN_DRV_TAB gc2083_sen_drv_tab = {
	sen_open_gc2083,
	sen_close_gc2083,
	sen_sleep_gc2083,
	sen_wakeup_gc2083,
	sen_write_reg_gc2083,
	sen_read_reg_gc2083,
	sen_chg_mode_gc2083,
	sen_chg_fps_gc2083,
	sen_set_info_gc2083,
	sen_get_info_gc2083,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_gc2083(void)
{
	return &gc2083_sen_drv_tab;
}

static void sen_pwr_ctrl_gc2083(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
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

static CTL_SEN_CMD sen_set_cmd_info_gc2083(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_gc2083(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif

static ER sen_open_gc2083(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_gc2083);
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

static ER sen_close_gc2083(CTL_SEN_ID id)
{
	if ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr)) {
		if (i2c_valid[id]) {
			sen_i2c_remove_driver(id);
		}
	} else {
		is_fastboot[id]= 0;
		#if defined(__KERNEL__)
		isp_builtin_uninit_i2c(id);
		#endif
	}

	i2c_valid[id] = FALSE;

	return E_OK;
}

static ER sen_sleep_gc2083(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_gc2083(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_gc2083(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

static ER sen_read_reg_gc2083(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
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

static UINT32 sen_get_cmd_tab_gc2083(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = gc2083_mode_1;
		return sizeof(gc2083_mode_1) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_gc2083(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
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
		sen_set_chgmode_fps_gc2083(id, isp_builtin_get_chgmode_fps(id));
		sen_set_cur_fps_gc2083(id, isp_builtin_get_chgmode_fps(id));
		sen_set_gain_gc2083(id, &sensor_ctrl);
		sen_set_expt_gc2083(id, &sensor_ctrl);
		#endif
		preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

		return E_OK;
	}

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_gc2083(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_gc2083(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_gc2083(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_gc2083(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_gc2083(0x0D41, 1, (sensor_vd >> 8) & 0x3F, 0x00);
			rt |= sen_write_reg_gc2083(id, &cmd);
			cmd = sen_set_cmd_info_gc2083(0x0D42, 1, sensor_vd & 0xFF, 0x00);
			rt |= sen_write_reg_gc2083(id, &cmd);
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

			sen_set_gain_gc2083(id, &sensor_ctrl);
			sen_set_expt_gc2083(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_gc2083(id, &flip);
		} else {
			cmd = sen_set_cmd_info_gc2083(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_gc2083(id, &cmd);
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_gc2083(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_gc2083(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_gc2083(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_gc2083(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	cmd = sen_set_cmd_info_gc2083(0x0D41, 1, (sensor_vd >> 8) & 0x3F, 0x00);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0D42, 1, sensor_vd & 0xFF, 0x00);
	rt |= sen_write_reg_gc2083(id, &cmd);

	return rt;
}

static ER sen_set_info_gc2083(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_gc2083(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_gc2083(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_gc2083(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_gc2083(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_gc2083(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_gc2083(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_gc2083(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_gc2083(id, (CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_gc2083((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_gc2083(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_gc2083((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_gc2083(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_gc2083(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_gc2083((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_MIPI:
		sen_get_mode_mipi_gc2083((CTL_SENDRV_GET_MODE_MIPI_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_gc2083((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_gc2083(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_gc2083(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_gc2083(id, data);
		break;
	default:
		rt = E_NOSPT;
	}
	return rt;
}

static UINT32 sen_calc_chgmode_vd_gc2083(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);	
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	sen_set_chgmode_fps_gc2083(id, fps);
	sen_set_cur_fps_gc2083(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_gc2083(id, fps);
		sen_set_cur_fps_gc2083(id, fps);
	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_gc2083(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_gc2083(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_gc2083(CTL_SEN_ID id, UINT32 fps)
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

static void sen_set_gain_gc2083(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 data1[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data2[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data3[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 data4[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 i, tbl_idx = NUM_OF_GAINSET, digital_gain = 0, decimal = 0;
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
		
		for (i = 0; i < NUM_OF_GAINSET; i++) {
			if (gain_table[i].total_gain > sensor_ctrl->gain_ratio[frame_cnt]) {
				tbl_idx = i;
				break;
			}
		}
	
		// collect the gain setting
		if (tbl_idx < 1) {
			digital_gain = sensor_ctrl->gain_ratio[frame_cnt] * 1000 / gain_table[0].total_gain;
			if (digital_gain <= 1000) {
				decimal = 64;
			} else {
				decimal = digital_gain * 64 / 1000;
			}				
			data1[frame_cnt] = (gain_table[0].gain_reg0 << 24) | (gain_table[0].gain_reg1 << 16) | (gain_table[0].gain_reg2 << 8) | (gain_table[0].gain_reg3);
			data2[frame_cnt] = (gain_table[0].gain_reg4 << 24) | (gain_table[0].gain_reg5 << 16) | (gain_table[0].gain_reg6 << 8) | (gain_table[0].gain_reg7);
			data3[frame_cnt] = (gain_table[0].gain_reg8 << 24) | (gain_table[0].gain_reg9 << 16) | (gain_table[0].gain_reg10 << 8) | (gain_table[0].gain_reg11);
			data4[frame_cnt] = (gain_table[0].gain_reg12 << 16) | decimal;
		} else {
			digital_gain = sensor_ctrl->gain_ratio[frame_cnt] * 1000 / gain_table[tbl_idx-1].total_gain;				
			if (digital_gain <= 1000) {
				decimal = 64;
			} else {
				decimal = digital_gain * 64 / 1000;
			}				
			data1[frame_cnt] = (gain_table[tbl_idx-1].gain_reg0 << 24) | (gain_table[tbl_idx-1].gain_reg1 << 16) | (gain_table[tbl_idx-1].gain_reg2 << 8) | (gain_table[tbl_idx-1].gain_reg3);
			data2[frame_cnt] = (gain_table[tbl_idx-1].gain_reg4 << 24) | (gain_table[tbl_idx-1].gain_reg5 << 16) | (gain_table[tbl_idx-1].gain_reg6 << 8) | (gain_table[tbl_idx-1].gain_reg7);
			data3[frame_cnt] = (gain_table[tbl_idx-1].gain_reg8 << 24) | (gain_table[tbl_idx-1].gain_reg9 << 16) | (gain_table[tbl_idx-1].gain_reg10 << 8) | (gain_table[tbl_idx-1].gain_reg11);
			data4[frame_cnt] = (gain_table[tbl_idx-1].gain_reg12 << 16) | decimal;
		}
	}

	cmd = sen_set_cmd_info_gc2083(0x00D0, 1, (data1[0] >> 24) & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0155, 1, (data1[0] >> 16) & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0410, 1, (data1[0] >> 8) & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0411, 1, data1[0] & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0412, 1, (data2[0] >> 24) & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0413, 1, (data2[0] >> 16) & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0414, 1, (data2[0] >> 8) & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0415, 1, data2[0] & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0416, 1, (data3[0] >> 24) & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0417, 1, (data3[0] >> 16) & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x00B8, 1, (data3[0] >> 8) & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x00B9, 1, data3[0] & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	
	cmd = sen_set_cmd_info_gc2083(0x031D, 1, 0x2E, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0DC1, 1, (data4[0] >> 16) & 0xFF, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);	
	cmd = sen_set_cmd_info_gc2083(0x031D, 1, 0x28, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	
	cmd = sen_set_cmd_info_gc2083(0x00B1, 1, (data4[0] >> 6) & 0x0F, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x00B2, 1, (data4[0] & 0x3F) << 2, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);	
				
	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_expt_gc2083(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps = 0, cur_fps = 0, clac_fps = 0, t_row = 0;
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
		t_row = sen_calc_rowtime_gc2083(id, cur_sen_mode[id]);
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

	// Get fps
	chgmode_fps = sen_get_chgmode_fps_gc2083(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_gc2083(id, cur_sen_mode[id]);
	expt_time = (line[0]) * t_row / 10;		
	temp_line[0] = line[0];

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_gc2083(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_gc2083(id, cur_fps);

	//Check max vts
	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("max vts overflow\r\n");
		sensor_vd = MAX_VD_PERIOD;
	}

	cmd = sen_set_cmd_info_gc2083(0x0D41, 1, (sensor_vd >> 8) & 0x3F, 0);
	rt |= sen_write_reg_gc2083(id, &cmd);

	cmd = sen_set_cmd_info_gc2083(0x0D42, 1, sensor_vd & 0xFF, 0);
	rt |= sen_write_reg_gc2083(id, &cmd);

	//Check max exp line reg
	if (line[0] > (MAX_EXPOSURE_LINE)) {
		DBG_ERR("max line overflow \r\n");
		line[0] = MAX_EXPOSURE_LINE;
	}

	//Check max exp line
	if ((line[0]) > (sensor_vd - NON_EXPOSURE_LINE)) {
		line[0] = sensor_vd - NON_EXPOSURE_LINE;
	}
	compensation_ratio[id][0] = 100 * temp_line[0] / line[0];

	// set exposure line to sensor
	cmd = sen_set_cmd_info_gc2083(0x0D03, 1, (line[0] >> 8) & 0x3F , 0);
	rt |= sen_write_reg_gc2083(id, &cmd);
	cmd = sen_set_cmd_info_gc2083(0x0D04, 1, line[0] & 0xFF, 0);
	rt |= sen_write_reg_gc2083(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_gc2083(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_gc2083(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	UINT32 temp_reg1 = 0, temp_reg2 = 0;
	ER rt = E_OK;
	
	cmd = sen_set_cmd_info_gc2083(0x0015, 1, 0x0, 0x0);
	rt |= sen_read_reg_gc2083(id, &cmd);
	temp_reg1 = cmd.data[0];

	cmd = sen_set_cmd_info_gc2083(0x0D15, 1, 0x0, 0x0);
	rt |= sen_read_reg_gc2083(id, &cmd);
	temp_reg2 = cmd.data[0];

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
		if (*flip & CTL_SEN_FLIP_H) {
				temp_reg1 |= 0x1;
				temp_reg2 |= 0x1;
		} else {
				temp_reg1 &= (~0x01);
				temp_reg2 &= (~0x01);
		}
	} else {
		DBG_WRN("no support mirror \r\n");
	}
	
	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (*flip & CTL_SEN_FLIP_V) {
			temp_reg1 |= 0x02;
			temp_reg2 |= 0x02;
		} else {
			temp_reg1 &= (~0x02);
			temp_reg2 &= (~0x02);
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}

	cmd = sen_set_cmd_info_gc2083(0x0015, 1, temp_reg1, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);

	cmd = sen_set_cmd_info_gc2083(0x0D15, 1, temp_reg2, 0x0);
	rt |= sen_write_reg_gc2083(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_gc2083(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_gc2083(0x0015, 1, 0x0, 0x0);
	rt |= sen_read_reg_gc2083(id, &cmd);

	*flip = CTL_SEN_FLIP_NONE;
	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
		if (cmd.data[0] & 0x1) {
			*flip |= CTL_SEN_FLIP_H;
		}
	} else {
		DBG_WRN("no support mirror \r\n");
	}

	cmd = sen_set_cmd_info_gc2083(0x0D15, 1, 0x0, 0x0);
	rt |= sen_read_reg_gc2083(id, &cmd);

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (cmd.data[0] & 0x02) {
			*flip |= CTL_SEN_FLIP_V;
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}

	return rt;
}

#if defined(__FREERTOS)
void sen_get_gain_gc2083(CTL_SEN_ID id, void *param)
#else
static void sen_get_gain_gc2083(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

#if defined(__FREERTOS)
void sen_get_expt_gc2083(CTL_SEN_ID id, void *param)
#else
static void sen_get_expt_gc2083(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_gc2083(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row;

	t_row = sen_calc_rowtime_gc2083(id, cur_sen_mode[id]);
	*min_exp_time = t_row * MIN_EXPOSURE_LINE / 10 + 1; // if linear and shdr mode is different,the value need to separate.
}

static void sen_get_mode_basic_gc2083(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_gc2083(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_gc2083(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_gc2083(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	return E_OK;
}

static ER sen_get_attr_if_gc2083(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
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

static void sen_get_fps_gc2083(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_gc2083(id);
	data->chg_fps = sen_get_chgmode_fps_gc2083(id);
}

static void sen_get_speed_gc2083(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
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

static void sen_get_mode_mipi_gc2083(CTL_SENDRV_GET_MODE_MIPI_PARAM *data)
{
	UINT32 mode = data->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mipi_param[mode], sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
}

static void sen_get_modesel_gc2083(CTL_SENDRV_GET_MODESEL_PARAM *data)
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
		if ((data->size.w <= 1920) && (data->size.h <= 1080)) {
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

static UINT32 sen_calc_rowtime_step_gc2083(CTL_SEN_ID id, CTL_SEN_MODE mode)
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

static UINT32 sen_calc_rowtime_gc2083(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 1000 * (mode_basic_param[mode].signal_info.hd_period) / ((speed_param[mode].pclk) / 10000);

	return row_time;
}

static void sen_get_rowtime_gc2083(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_gc2083(id, data->mode);
	data->row_time = sen_calc_rowtime_gc2083(id, data->mode) * (data->row_time_step);
}

static void sen_set_cur_fps_gc2083(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_gc2083(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_gc2083(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_gc2083(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
void sen_get_i2c_id_gc2083(CTL_SEN_ID id, UINT32 *i2c_id)
{
	*i2c_id = sen_i2c[id].id;
}

void sen_get_i2c_addr_gc2083(CTL_SEN_ID id, UINT32 *i2c_addr)
{
	*i2c_addr = sen_i2c[id].addr;
}

int sen_init_gc2083(SENSOR_DTSI_INFO *info)
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

	sprintf(compatible, "nvt,sen_gc2083");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_gc2083");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_gc2083;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_gc2083();
	rt = ctl_sen_reg_sendrv("nvt_sen_gc2083", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_gc2083(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_gc2083");
}

#else
static int __init sen_init_gc2083(void)
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
	reg_obj.pwr_ctrl = sen_pwr_ctrl_gc2083;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_gc2083();
	rt = ctl_sen_reg_sendrv("nvt_sen_gc2083", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_gc2083(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_gc2083");
}

module_init(sen_init_gc2083);
module_exit(sen_exit_gc2083);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_GC2083_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

