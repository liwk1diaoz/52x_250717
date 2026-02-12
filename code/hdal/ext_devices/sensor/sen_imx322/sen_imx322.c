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
VOS_MODULE_VERSION(nvt_sen_imx322, 1, 45, 000, 00);

//=============================================================================
// information
//=============================================================================
#define SEN_IMX322_MODULE_NAME     "sen_imx322"
#define SEN_MAX_MODE               1
#define MAX_VD_PERIOD              0xFFFF
#define MAX_EXPOSURE_LINE          0xFFFF

#define SEN_I2C_ADDR 0x34>>1
#define SEN_I2C_COMPATIBLE "nvt,sen_imx322"

#define TRANS_PROTOCOL_I2C 0
#define TRANS_PROTOCOL_SPI 1
#define TRANS_PROTOCOL TRANS_PROTOCOL_SPI

#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
#include "sen_i2c.c"
#else
#ifdef __KERNEL__
#include "sen_spi.c"
#endif
#endif

//=============================================================================
// function declaration
//=============================================================================
static CTL_SEN_DRV_TAB *sen_get_drv_tab_imx322(void);
static void sen_pwr_ctrl_imx322(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb);
static ER sen_open_imx322(CTL_SEN_ID id);
static ER sen_close_imx322(CTL_SEN_ID id);
static ER sen_sleep_imx322(CTL_SEN_ID id);
static ER sen_wakeup_imx322(CTL_SEN_ID id);
static ER sen_write_reg_imx322(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_read_reg_imx322(CTL_SEN_ID id, CTL_SEN_CMD *cmd);
static ER sen_chg_mode_imx322(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj);
static ER sen_chg_fps_imx322(CTL_SEN_ID id, UINT32 fps);
static ER sen_set_info_imx322(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static ER sen_get_info_imx322(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data);
static UINT32 sen_calc_chgmode_vd_imx322(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_calc_exp_vd_imx322(CTL_SEN_ID id, UINT32 fps);
static void sen_set_gain_imx322(CTL_SEN_ID id, void *param);
static void sen_set_expt_imx322(CTL_SEN_ID id, void *param);
static void sen_set_preset_imx322(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl);
static void sen_set_flip_imx322(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
static ER sen_get_flip_imx322(CTL_SEN_ID id, CTL_SEN_FLIP *flip);
#if defined(__FREERTOS)
void sen_get_gain_imx322(CTL_SEN_ID id, void *param);
void sen_get_expt_imx322(CTL_SEN_ID id, void *param);
#else
static void sen_get_gain_imx322(CTL_SEN_ID id, void *param);
static void sen_get_expt_imx322(CTL_SEN_ID id, void *param);
#endif
static void sen_get_min_expt_imx322(CTL_SEN_ID id, void *param);
static void sen_get_mode_basic_imx322(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic);
static void sen_get_attr_basic_imx322(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_BASIC_PARAM *data);
static void sen_get_attr_signal_imx322(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data);
static ER sen_get_attr_cmdif_imx322(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data);
static ER sen_get_attr_if_imx322(CTL_SENDRV_GET_ATTR_IF_PARAM *data);
static void sen_get_fps_imx322(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data);
static void sen_get_speed_imx322(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data);
static void sen_get_mode_para_imx322(CTL_SENDRV_GET_MODE_PARA_PARAM *data);
static void sen_get_modesel_imx322(CTL_SENDRV_GET_MODESEL_PARAM *data);
static UINT32 sen_calc_rowtime_imx322(CTL_SEN_ID id, CTL_SEN_MODE mode);
static UINT32 sen_calc_rowtime_step_imx322(CTL_SEN_ID id, CTL_SEN_MODE mode);
static void sen_get_rowtime_imx322(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data);
static void sen_set_cur_fps_imx322(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_cur_fps_imx322(CTL_SEN_ID id);
static void sen_set_chgmode_fps_imx322(CTL_SEN_ID id, UINT32 fps);
static UINT32 sen_get_chgmode_fps_imx322(CTL_SEN_ID id);
static ER sen_get_min_shr_imx322(CTL_SEN_MODE mode, UINT32 *min_shs1, UINT32 *min_exp);
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
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1},
	{CTL_SEN_CLK_SEL_SIEMCLK, CTL_SEN_IGNORE, 0x44, 1, 1}
};

#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
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
#else
#if defined(__KERNEL__)
static SEN_SPI sen_spi[CTL_SEN_ID_MAX] = {
	{SEN_SPI_ID_1, 10000000, SEN_SPI_FMT_24BIT, SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST},
	{SEN_SPI_ID_2, 10000000, SEN_SPI_FMT_24BIT, SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST},
	{SEN_SPI_ID_1, 10000000, SEN_SPI_FMT_24BIT, SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST},
	{SEN_SPI_ID_1, 10000000, SEN_SPI_FMT_24BIT, SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST},
	{SEN_SPI_ID_1, 10000000, SEN_SPI_FMT_24BIT, SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST},
	{SEN_SPI_ID_1, 10000000, SEN_SPI_FMT_24BIT, SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST},
	{SEN_SPI_ID_1, 10000000, SEN_SPI_FMT_24BIT, SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST},
	{SEN_SPI_ID_1, 10000000, SEN_SPI_FMT_24BIT, SPI_MODE_3 | SPI_CS_HIGH | SPI_LSB_FIRST}
};
#endif
#endif

static CTL_SENDRV_GET_ATTR_BASIC_PARAM basic_param = {
	SEN_IMX322_MODULE_NAME,
	CTL_SEN_VENDOR_SONY,
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
		37125000,
		74250000,
		74250000
	}
};

static CTL_SENDRV_GET_MODE_BASIC_PARAM mode_basic_param[SEN_MAX_MODE] = {
	{
		CTL_SEN_MODE_1,
		CTL_SEN_IF_TYPE_PARALLEL,
		CTL_SEN_DATA_FMT_RGB,
		CTL_SEN_MODE_LINEAR,
		3000,
		1,
		CTL_SEN_STPIX_R,
		CTL_SEN_PIXDEPTH_10BIT,
		CTL_SEN_FMT_POGRESSIVE,
		{2200, 1124},
		{{152, 37, 1920, 1080}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}},
		{1920, 1080},
		{0, 2200, 0, 1125},
		CTL_SEN_RATIO(16, 9),
		{1000, 125890},
		100
	}
};

static CTL_SENDRV_GET_MODE_PARA_PARAM mode_param[SEN_MAX_MODE] = {
	{CTL_SEN_MODE_1, {1}},
};

static CTL_SEN_CMD imx322_mode_1[] = {
	{0x0200, 1, {0x31, 0x0}},  // TESTEN + STANDBY //31
	{0x0211, 1, {0x00, 0x0}},  // OPORTSEL
	{0x022D, 1, {0x40, 0x0}},  // DCKDLY
	{0x0202, 1, {0x0F, 0x0}},  // HD1080p mode
	{0x0214, 1, {0x00, 0x0}},  // WINPH_L
	{0x0215, 1, {0x00, 0x0}},  // WINPH_H
	{0x0216, 1, {0x3c, 0x0}},  // WINPV_L
	{0x0217, 1, {0x00, 0x0}},  // WINPV_H
	{0x0218, 1, {0xC0, 0x0}},  // WINWH_L
	{0x0219, 1, {0x07, 0x0}},  // WINWH_H
	{0x021A, 1, {0x51, 0x0}},  // WINWV_L
	{0x021B, 1, {0x04, 0x0}},  // WINWV_H
	{0x029A, 1, {0x26, 0x0}},  // 12B1080P_L
	{0x029B, 1, {0x02, 0x0}},  // 12B1080P_H
	{0x02CE, 1, {0x16, 0x0}},  // PRES
	{0x02CF, 1, {0x82, 0x0}},  // DRES_L
	{0x02D0, 1, {0x00, 0x0}},  // DRES_H
	{0x0201, 1, {0x00, 0x0}},  // VREVERSE
	{0x0212, 1, {0x82, 0x0}},  // 0:SSBRK, 1: ADRES, 7:fix value
	{0x020f, 1, {0x00, 0x0}},  // SVS_L
	{0x0210, 1, {0x00, 0x0}},  // SVS_H
	{0x020d, 1, {0x00, 0x0}},  // SPL_L
	{0x020e, 1, {0x00, 0x0}},  // SPL_H
	{0x0208, 1, {0x00, 0x0}},  // SHS1_L
	{0x0209, 1, {0x00, 0x0}},  // SHS1_H
	{0x021E, 1, {0x00, 0x0}},  // GAIN
	{0x0220, 1, {0xF0, 0x0}},  // BLKLEVEL_L
	{0x0221, 1, {0x00, 0x0}},  // 0: BLKLEVEL_H, 4,5:XHSLNG, 7:10BITA
	{0x0222, 1, {0x41, 0x0}},  // 0~2:XVLNG 6:MUST=1  7:720PMODE
	{0x027a, 1, {0x00, 0x0}},
	{0x027b, 1, {0x00, 0x0}},
	{0x0298, 1, {0x26, 0x0}},
	{0x0299, 1, {0x02, 0x0}},
	{0x0203, 1, {0x4C, 0x0}},  // HMAX_L
	{0x0204, 1, {0x04, 0x0}},  // HMAX_H
	{0x0205, 1, {0x65, 0x0}},  // VMAX_L
	{0x0206, 1, {0x04, 0x0}},  // VMAX_H
	{0x022C, 1, {0x00, 0x0}},  // 0:XMSTA
	{0x0213, 1, {0x40, 0x0}},  // fix value
	{0x021C, 1, {0x50, 0x0}},  // fix value
	{0x021F, 1, {0x31, 0x0}},  // fix value
	{0x0317, 1, {0x0D, 0x0}},  // fix value
	{0x0227, 1, {0x20, 0x0}},  // fix value 2015/04/15
	{0x0204, 1, {0x04, 0x0}},  // HMAX_L
	{0x0203, 1, {0x4c, 0x0}},  // HMAX_H
	{0x0206, 1, {0x04, 0x0}},  // VMAX_L
	{0x0205, 1, {0x65, 0x0}},  // VMAX_H
	{0x0202, 1, {0x0f, 0x0}},  // HMAX_L
	{0x0211, 1, {0x00, 0x0}},  // HMAX_H
	{0x0212, 1, {0x82, 0x0}},  // VMAX_L
	{0x0216, 1, {0x3c, 0x0}},  // VMAX_H
	{0x0220, 1, {0xf0, 0x0}},  // 0:XMSTA
	{0x0221, 1, {0x00, 0x0}},  // fix value
	{0x0222, 1, {0x41, 0x0}},  // fix value
	{0x027a, 1, {0x00, 0x0}},  // fix value
	{0x027b, 1, {0x00, 0x0}},  // fix value
	{0x02ce, 1, {0x16, 0x0}},  // fix value 2015/04/15	
	{0x02cf, 1, {0x82, 0x0}},  // fix value 2015/04/15		
	{SEN_CMD_SETVD, 1, {0x0, 0x0}},
	{SEN_CMD_PRESET, 1, {0x0, 0x0}},
	{SEN_CMD_DIRECTION, 1, {0x0, 0x0}},	
	{0x0200, 1, {0x30, 0x0}},  // STANDBY off	
};

typedef struct gain_set {
	UINT32 gain_reg;
	UINT32 total_gain;
} gain_set_t;

static const gain_set_t gain_table[] = {  
    {0x00,	1000},  {0x01,  1031},  {0x02,	1078},   {0x03,  1109},   {0x04,  1140}, 
    {0x05,	1187},  {0x06,  1234},  {0x07,	1281},   {0x08,  1312},   {0x09,  1359}, 
    {0x0A,	1406},  {0x0B,  1468},  {0x0C,	1515},   {0x0D,  1562},   {0x0E,  1625},  
    {0x0F,	1671},  {0x10,	1734},  {0x11,  1796},   {0x12,  1859},   {0x13,  1921},  
    {0x14,  2000},  {0x15,	2062},  {0x16,  2140},   {0x17,  2218},   {0x18,  2296},  
    {0x19,	2375},  {0x1A,	2453},  {0x1B,  2546},   {0x1C,  2624},   {0x1D,  2718},  
    {0x1E,	2812},  {0x1F,	2921},  {0x20,  3015},   {0x21,  3125},   {0x22,  3234},  
    {0x23,	3343},  {0x24,	3468},  {0x25,  3593},   {0x26,  3718},   {0x27,  3843}, 
    {0x28,	3984},  {0x29,	4125},  {0x2A,  4265},   {0x2B,  4421},   {0x2C,  4578}, 
    {0x2D,	4734},  {0x2E,	4890},  {0x2F,  5062},   {0x30,  5250},   {0x31,  5437}, 
    {0x32,	5625},  {0x33,	5828},  {0x34,  6031},   {0x35,  6234},   {0x36,  6453}, 
    {0x37,	6687},  {0x38,	6921},  {0x39,  7156},   {0x3A,  7406},   {0x3B,  7671}, 
    {0x3C,	7937},  {0x3D,	8218},  {0x3E,  8515},   {0x3F,  8812},   {0x40,  9125}, 
    {0x41,	9437},  {0x42,	9765},  {0x43,  10109},  {0x44,  10468},  {0x45,  10843}, 
    {0x46,	11218}, {0x47,	11609}, {0x48,  12015},  {0x49,  12437},  {0x4A,  12875}, 
    {0x4B,	13328}, {0x4C,	13796}, {0x4D,  14281},  {0x4E,  14796},  {0x4F,  15312}, 
    {0x50,  15843}, {0x51,  16406}, {0x52,  16984},  {0x53,  17578},  {0x54,  18203}, 
    {0x55,  18843}, {0x56,  19500}, {0x57,  20187},  {0x58,  20890},  {0x59,  21625}, 
    {0x5A,  22390}, {0x5B,  23171}, {0x5C,  23984},  {0x5D,  24828},  {0x5E,  25703}, 
    {0x5F,  26609}, {0x60,  27546}, {0x61,  28515},  {0x62,  29515},  {0x63,  30546}, 
    {0x64,  31625}, {0x65,  32734}, {0x66,  33890},  {0x67,  35078},  {0x68,  36312}, 
    {0x69,  37578}, {0x6A,  38906}, {0x6B,  40265},  {0x6C,  41687},  {0x6D,  43156}, 
    {0x6E,  44671}, {0x6F,  46234}, {0x70,  47859},  {0x71,  49546},  {0x72,  51281}, 
    {0x73,  53093}, {0x74,  54953}, {0x75,  56890},  {0x76,  58890},  {0x77,  60953}, 
    {0x78,  63093}, {0x79,  65312}, {0x7A,  67609},  {0x7B,  69984},  {0x7C,  72437}, 
    {0x7D,  74984}, {0x7E,  77625}, {0x7F,  80359},  {0x80,  83171},  {0x81,  86093}, 
    {0x82,  89125}, {0x83,  92250}, {0x84,  95500},  {0x85,  98859},  {0x86,  102328}, 
    {0x87, 105921}, {0x88, 109640}, {0x89,  113500}, {0x8A,  117484}, {0x8B,  121625}, 
    {0x8C, 125890},           
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
#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
static BOOL i2c_valid[CTL_SEN_ID_MAX];
#else
#if defined(__KERNEL__)
static BOOL spi_valid[CTL_SEN_ID_MAX];
#endif
#endif

static CTL_SEN_DRV_TAB imx322_sen_drv_tab = {
	sen_open_imx322,
	sen_close_imx322,
	sen_sleep_imx322,
	sen_wakeup_imx322,
	sen_write_reg_imx322,
	sen_read_reg_imx322,
	sen_chg_mode_imx322,
	sen_chg_fps_imx322,
	sen_set_info_imx322,
	sen_get_info_imx322,
};

static CTL_SEN_DRV_TAB *sen_get_drv_tab_imx322(void)
{
	return &imx322_sen_drv_tab;
}

static void sen_pwr_ctrl_imx322(CTL_SEN_ID id, CTL_SEN_PWR_CTRL_FLAG flag, CTL_SEN_CLK_CB clk_cb)
{
	UINT32 i = 0;
	UINT32 reset_count = 0, pwdn_count = 0;
	DBG_IND("enter flag %d \r\n", flag);

	#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
	if ((flag == CTL_SEN_PWR_CTRL_TURN_ON) && ((!is_fastboot[id]) || (fastboot_i2c_id[id] != sen_i2c[id].id) || (fastboot_i2c_addr[id] != sen_i2c[id].addr))) {
	#else
	if (flag == CTL_SEN_PWR_CTRL_TURN_ON) {
	#endif
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

static CTL_SEN_CMD sen_set_cmd_info_imx322(UINT32 addr, UINT32 data_length, UINT32 data0, UINT32 data1)
{
	CTL_SEN_CMD cmd;

	cmd.addr = addr;
	cmd.data_len = data_length;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return cmd;
}

#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
#if defined(__KERNEL__)
static void sen_load_cfg_from_compatible_imx322(struct device_node *of_node)
{
	DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
	sen_common_load_cfg_preset_compatible(of_node, &sen_preset);
	sen_common_load_cfg_direction_compatible(of_node, &sen_direction);
	sen_common_load_cfg_power_compatible(of_node, &sen_power);
	sen_common_load_cfg_i2c_compatible(of_node, &sen_i2c);
}
#endif
#endif

static ER sen_open_imx322(CTL_SEN_ID id)
{
	ER rt = E_OK;

	#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
	#if defined(__KERNEL__)
	sen_i2c_reg_cb(sen_load_cfg_from_compatible_imx290);
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
	#else
	#if defined(__KERNEL__)
	spi_valid[id] = TRUE;
	rt = sen_spi_init_driver(id, &sen_spi[id]);
	if (rt != E_OK) {
		spi_valid[id] = FALSE;

		DBG_ERR("init. spi driver fail (%d) \r\n", id);
	}
	#else
	DBG_ERR("RTOS not support spi driver \r\n");
	#endif
	#endif

	return rt;
}

static ER sen_close_imx322(CTL_SEN_ID id)
{
	#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
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
	#else
	#if defined(__KERNEL__)
	if (spi_valid[id]) {
		sen_spi_remove_driver(id);
	}
	spi_valid[id] = FALSE;
	#else
	DBG_ERR("RTOS not support spi driver \r\n");
	#endif
	#endif

	return E_OK;
}

static ER sen_sleep_imx322(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_wakeup_imx322(CTL_SEN_ID id)
{
	DBG_IND("enter \r\n");
	return E_OK;
}

static ER sen_write_reg_imx322(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
	struct i2c_msg msgs;
	unsigned char buf[3];
	int i;

	if (!i2c_valid[id]) {
		return E_NOSPT;
	}

	buf[0]     = (((cmd->addr >> 8) & 0xFF) & (~0x80));
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

	#else
	#if defined(__KERNEL__)
	struct spi_msg msgs;
	unsigned char buf[3];

	if (!spi_valid[id]) {
		return E_NOSPT;
	}

	//DBG_ERR("1.addr1 0x%x, addr2 0x%x, reg3 0x%x, \r\n", (cmd->addr >> 8) & 0xFF, cmd->addr & 0xFF, cmd->data[0] & 0xFF);

	buf[0]     = (cmd->addr >> 8) & 0xFF;//((cmd->addr >> 8) & 0xFF) & (~0x80);
	buf[1]     = cmd->addr & 0xFF;
	buf[2]     = cmd->data[0] & 0xFF;
	msgs.len   = 3;
	msgs.buf   = buf;

	if (sen_spi_transfer(id, &msgs, 1) != 0) {
		return E_SYS;
	}

	return E_OK;
	#else
	return E_NOSPT;
	#endif
	#endif
}

static ER sen_read_reg_imx322(CTL_SEN_ID id, CTL_SEN_CMD *cmd)
{
	#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
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
	#else
	#if defined(__KERNEL__)
	struct spi_msg  msgs[2];
	unsigned char   tmp[2], tmp2[2];

	if (!spi_valid[id]) {
		return E_NOSPT;
	}

	tmp[0]        = (cmd->addr >> 8) & 0xFF;
	tmp[1]        = cmd->addr & 0xFF;
	msgs[0].len   = 2;
	msgs[0].buf   = tmp;

	tmp2[0]       = 0;
	msgs[1].len   = 1;
	msgs[1].buf   = tmp2;

	if (sen_spi_transfer(id, msgs, 2) != 0) {
		return E_SYS;
	}

	cmd->data[0] = tmp2[0];

	return E_OK;
	#else
	return E_NOSPT;
	#endif
	#endif
}

static UINT32 sen_get_cmd_tab_imx322(CTL_SEN_MODE mode, CTL_SEN_CMD **cmd_tab)
{
	switch (mode) {
	case CTL_SEN_MODE_1:
		*cmd_tab = imx322_mode_1;
		return sizeof(imx322_mode_1) / sizeof(CTL_SEN_CMD);

	default:
		DBG_ERR("sensor mode %d no cmd table\r\n", mode);
		*cmd_tab = NULL;
		return 0;
	}
}

static ER sen_chg_mode_imx322(CTL_SEN_ID id, CTL_SENDRV_CHGMODE_OBJ chgmode_obj)
{
	ISP_SENSOR_CTRL sensor_ctrl = {0};
	CTL_SEN_CMD *p_cmd_list = NULL, cmd;
	CTL_SEN_FLIP flip = CTL_SEN_FLIP_NONE;
	UINT32 sensor_vd;
	UINT32 idx, cmd_num = 0;
	ER rt = E_OK;
	
	if (cur_sen_mode[id] != chgmode_obj.mode) {
		gpio_direction_output((sen_power[id].rst_pin), 0);
		gpio_set_value((sen_power[id].rst_pin), 0);
		vos_util_delay_ms(sen_power[id].rst_time);
		gpio_set_value((sen_power[id].rst_pin), 1);
		vos_util_delay_ms(sen_power[id].stable_time);
	}

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
		sen_set_chgmode_fps_imx322(id, isp_builtin_get_chgmode_fps(id));
		sen_set_cur_fps_imx322(id, isp_builtin_get_chgmode_fps(id));
		sen_set_gain_imx322(id, &sensor_ctrl);
		sen_set_expt_imx322(id, &sensor_ctrl);
		#endif
		preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

		return E_OK;
	}

	// get & set sensor cmd table
	cmd_num = sen_get_cmd_tab_imx322(chgmode_obj.mode, &p_cmd_list);
	if (p_cmd_list == NULL) {
		DBG_ERR("%s: SenMode(%d) out of range!!! \r\n", __func__, chgmode_obj.mode);
		return E_SYS;
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_imx322(id, chgmode_obj.frame_rate);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_imx322(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_imx322(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	for (idx = 0; idx < cmd_num; idx++) {
		if (p_cmd_list[idx].addr == SEN_CMD_DELAY) {
			vos_util_delay_ms((p_cmd_list[idx].data[0] & 0xFF) | ((p_cmd_list[idx].data[1] & 0xFF) << 8));
		} else if (p_cmd_list[idx].addr == SEN_CMD_SETVD) {
			cmd = sen_set_cmd_info_imx322(0x0205, 1, sensor_vd & 0xFF, 0x00);
			rt |= sen_write_reg_imx322(id, &cmd);
			cmd = sen_set_cmd_info_imx322(0x0206, 1, (sensor_vd >> 8) & 0xFF, 0x00);
			rt |= sen_write_reg_imx322(id, &cmd);
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

			sen_set_gain_imx322(id, &sensor_ctrl);
			sen_set_expt_imx322(id, &sensor_ctrl);
		} else if (p_cmd_list[idx].addr == SEN_CMD_DIRECTION) {
			if (sen_direction[id].mirror) {
				flip |= CTL_SEN_FLIP_H;
			}
			if (sen_direction[id].flip) {
				flip |= CTL_SEN_FLIP_V;
			}
			sen_set_flip_imx322(id, &flip);
		} else {
			cmd = sen_set_cmd_info_imx322(p_cmd_list[idx].addr, p_cmd_list[idx].data_len, p_cmd_list[idx].data[0], p_cmd_list[idx].data[1]);
			rt |= sen_write_reg_imx322(id, &cmd);
		}
	}

	preset_ctrl[id].mode = ISP_SENSOR_PRESET_CHGMODE;

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
		return rt;
	}

	return E_OK;
}

static ER sen_chg_fps_imx322(CTL_SEN_ID id, UINT32 fps)
{
	CTL_SEN_CMD cmd;
	UINT32 sensor_vd;
	ER rt = E_OK;

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_CHGFPS) {
		sensor_vd = sen_calc_chgmode_vd_imx322(id, fps);
	} else {
		DBG_WRN(" not support fps adjust \r\n");
		sen_set_cur_fps_imx322(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_chgmode_fps_imx322(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
	}

	cmd = sen_set_cmd_info_imx322(0x0205, 1, sensor_vd & 0xFF, 0x00);
	rt |= sen_write_reg_imx322(id, &cmd);
	cmd = sen_set_cmd_info_imx322(0x0206, 1, (sensor_vd >> 8) & 0xFF, 0x00);
	rt |= sen_write_reg_imx322(id, &cmd);

	return rt;
}

static ER sen_set_info_imx322(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_SET_EXPT:
		sen_set_expt_imx322(id, data);
		break;
	case CTL_SENDRV_CFGID_SET_GAIN:
		sen_set_gain_imx322(id, data);
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		sen_set_flip_imx322(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE1:
		sen_set_preset_imx322(id, (ISP_SENSOR_PRESET_CTRL *)(data));
		break;
	default:
		return E_NOSPT;
	}
	return E_OK;
}

static ER sen_get_info_imx322(CTL_SEN_ID id, CTL_SENDRV_CFGID drv_cfg_id, void *data)
{
	ER rt = E_OK;

	switch (drv_cfg_id) {
	case CTL_SENDRV_CFGID_GET_EXPT:
		sen_get_expt_imx322(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_GAIN:
		sen_get_gain_imx322(id, data);
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_BASIC:
		sen_get_attr_basic_imx322(id, (CTL_SENDRV_GET_ATTR_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_SIGNAL:
		sen_get_attr_signal_imx322((CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_CMDIF:
		rt = sen_get_attr_cmdif_imx322(id, (CTL_SENDRV_GET_ATTR_CMDIF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_ATTR_IF:
		rt = sen_get_attr_if_imx322((CTL_SENDRV_GET_ATTR_IF_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_FPS:
		sen_get_fps_imx322(id, (CTL_SENDRV_GET_FPS_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_SPEED:
		sen_get_speed_imx322(id, (CTL_SENDRV_GET_SPEED_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_BASIC:
		sen_get_mode_basic_imx322((CTL_SENDRV_GET_MODE_BASIC_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_PARA:
		sen_get_mode_para_imx322((CTL_SENDRV_GET_MODE_PARA_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODESEL:
		sen_get_modesel_imx322((CTL_SENDRV_GET_MODESEL_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_GET_MODE_ROWTIME:
		sen_get_rowtime_imx322(id, (CTL_SENDRV_GET_MODE_ROWTIME_PARAM *)(data));
		break;
	case CTL_SENDRV_CFGID_FLIP_TYPE:
		rt = sen_get_flip_imx322(id, (CTL_SEN_FLIP *)(data));
		break;
	case CTL_SENDRV_CFGID_USER_DEFINE2:
		sen_get_min_expt_imx322(id, data);
		break;
	default:
		rt = E_NOSPT;
	}
	return rt;
}

static UINT32 sen_calc_chgmode_vd_imx322(CTL_SEN_ID id, UINT32 fps)
{
	UINT32 sensor_vd;

	if (1 > fps) {
		DBG_ERR("sensor fps can not small than (%d),change to dft sensor fps (%d) \r\n", fps, mode_basic_param[cur_sen_mode[id]].dft_fps);	
		fps = mode_basic_param[cur_sen_mode[id]].dft_fps;
	}
	sensor_vd = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / fps;

	sen_set_chgmode_fps_imx322(id, fps);
	sen_set_cur_fps_imx322(id, fps);

	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = MAX_VD_PERIOD;
		fps = (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period) * (mode_basic_param[cur_sen_mode[id]].dft_fps) / sensor_vd;
		sen_set_chgmode_fps_imx322(id, fps);
		sen_set_cur_fps_imx322(id, fps);
	}

	if(sensor_vd < (mode_basic_param[cur_sen_mode[id]].signal_info.vd_period)) {
		DBG_ERR("sensor vd out of sensor driver range (%d) \r\n", sensor_vd);
		sensor_vd = mode_basic_param[cur_sen_mode[id]].signal_info.vd_period;
		sen_set_chgmode_fps_imx322(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
		sen_set_cur_fps_imx322(id, mode_basic_param[cur_sen_mode[id]].dft_fps);
	}

	return sensor_vd;
}

static UINT32 sen_calc_exp_vd_imx322(CTL_SEN_ID id, UINT32 fps)
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

static void sen_set_gain_imx322(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 data[ISP_SEN_MFRAME_MAX_NUM] = {0};
	UINT32 i = 0, step = 0;	
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
		if (100 <= (compensation_ratio[id][frame_cnt])) {
			sensor_ctrl->gain_ratio[frame_cnt] = (sensor_ctrl->gain_ratio[frame_cnt]) * (compensation_ratio[id][frame_cnt]) / 100;
		}
		if (sensor_ctrl->gain_ratio[frame_cnt] < (mode_basic_param[cur_sen_mode[id]].gain.min)) {
			sensor_ctrl->gain_ratio[frame_cnt] = (mode_basic_param[cur_sen_mode[id]].gain.min);
		} else if (sensor_ctrl->gain_ratio[frame_cnt] > (mode_basic_param[cur_sen_mode[id]].gain.max)) {
			sensor_ctrl->gain_ratio[frame_cnt] = (mode_basic_param[cur_sen_mode[id]].gain.max);			
		}

    //Search most suitable gain into gain table
		for ( i = 0 ; i< NUM_OF_GAINSET ; i++ ) {
			step = i;
			if ((gain_table[i].total_gain) > (sensor_ctrl->gain_ratio[frame_cnt])) {
				break;
			}
		}

		if ( 1 > step ) {
			step = 0;
		} else {
			step -= 1;
		}
		data[frame_cnt] = gain_table[step].gain_reg;
	}	

	cmd = sen_set_cmd_info_imx322(0x021E, 1, data[0], 0x0);
	rt |= sen_write_reg_imx322(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_min_shr_imx322(CTL_SEN_MODE mode, UINT32 *min_shs1, UINT32 *min_exp)
{
    ER rt = E_OK;

    switch (mode) {
    case CTL_SEN_MODE_1:
        *min_shs1 = 1; 
        *min_exp = 1;
        break;   
    default:
        DBG_ERR("sensor mode %d no cmd table\r\n", mode);
        break;
    }
	return rt;
}

static void sen_set_expt_imx322(CTL_SEN_ID id, void *param)
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;
	UINT32 line[ISP_SEN_MFRAME_MAX_NUM];
	UINT32 frame_cnt, total_frame;
	CTL_SEN_CMD cmd;
	UINT32 expt_time = 0, sensor_vd = 0, chgmode_fps = 0, cur_fps = 0, clac_fps = 0, t_row = 0;
	UINT32 min_shs1 = 0, min_exp_line = 0, shs1 = 0;
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
		t_row = sen_calc_rowtime_imx322(id, cur_sen_mode[id]);
		if (0 == t_row) {
			DBG_WRN("t_row  = 0, must >= 1 \r\n");
			t_row = 1;
		}
		line[frame_cnt] = (sensor_ctrl->exp_time[frame_cnt]) * 10 / t_row;
		
		sen_get_min_shr_imx322(cur_sen_mode[id], &min_shs1, &min_exp_line);
		
		// Limit minimun exposure line
		if (line[frame_cnt] < min_exp_line) {
			line[frame_cnt] = min_exp_line;
		}
	}

	// Write exposure line
	// Get fps
	chgmode_fps = sen_get_chgmode_fps_imx322(id);

	// Calculate exposure time
	t_row = sen_calc_rowtime_imx322(id, cur_sen_mode[id]);
	expt_time = (line[0]) * t_row / 10;
	temp_line[0] = line[0];	 		

	// Calculate fps
	if (0 == expt_time) {
		DBG_WRN("expt_time  = 0, must >= 1 \r\n");		
		expt_time = 1;
	}
	clac_fps = 100000000 / expt_time;

	cur_fps = (clac_fps < chgmode_fps) ? clac_fps : chgmode_fps;
	sen_set_cur_fps_imx322(id, cur_fps);

	// Calculate new vd
	sensor_vd = sen_calc_exp_vd_imx322(id, cur_fps);

	//Check max vts
	if (sensor_vd > MAX_VD_PERIOD) {
		DBG_ERR("max vts overflow\r\n");
		sensor_vd = MAX_VD_PERIOD;
	}


	//Check max exp line reg
	if (line[0] > MAX_EXPOSURE_LINE) {
		DBG_ERR("max line overflow \r\n");
		line[0] = MAX_EXPOSURE_LINE;
	}

	//calculate shr
	if (line[0] > (sensor_vd - min_shs1)) {
		shs1 = min_shs1;
	} else {
		shs1 = sensor_vd - line[0];
	}
	compensation_ratio[id][0] = 100 * temp_line[0] / (sensor_vd - shs1);
	
	// set exposure line to sensor
	cmd = sen_set_cmd_info_imx322(0x0208, 1, shs1 & 0xFF , 0);
	rt |= sen_write_reg_imx322(id, &cmd);
	cmd = sen_set_cmd_info_imx322(0x0209, 1, (shs1 >> 8) & 0xFF, 0);
	rt |= sen_write_reg_imx322(id, &cmd);

	// Set Vmax to sensor 
	cmd = sen_set_cmd_info_imx322(0x0205, 1, sensor_vd & 0xFF, 0);
	rt |= sen_write_reg_imx322(id, &cmd);
	cmd = sen_set_cmd_info_imx322(0x0206, 1, (sensor_vd >> 8) & 0xFF, 0);
	rt |= sen_write_reg_imx322(id, &cmd);

	
	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static void sen_set_preset_imx322(CTL_SEN_ID id, ISP_SENSOR_PRESET_CTRL *ctrl)
{
	memcpy(&preset_ctrl[id], ctrl, sizeof(ISP_SENSOR_PRESET_CTRL));
}

static void sen_set_flip_imx322(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_imx322(0x0201, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx322(id, &cmd);

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
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
	
	rt |= sen_write_reg_imx322(id, &cmd);

	if (rt != E_OK) {
		DBG_ERR("write register error %d \r\n", (INT)rt);
	}
}

static ER sen_get_flip_imx322(CTL_SEN_ID id, CTL_SEN_FLIP *flip)
{
	CTL_SEN_CMD cmd;
	ER rt = E_OK;

	cmd = sen_set_cmd_info_imx322(0x0201, 1, 0x0, 0x0);
	rt |= sen_read_reg_imx322(id, &cmd);

	*flip = CTL_SEN_FLIP_NONE;
	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_MIRROR) {
	} else {
		DBG_WRN("no support mirror \r\n");
	}

	if (basic_param.property & CTL_SEN_SUPPORT_PROPERTY_FLIP) {
		if (cmd.data[0] & 0x01) {
			*flip |= CTL_SEN_FLIP_V;
		}
	} else {
		DBG_WRN("no support flip \r\n");
	}

	return rt;
}

#if defined(__FREERTOS)
void sen_get_gain_imx322(CTL_SEN_ID id, void *param)
#else
static void sen_get_gain_imx322(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->gain_ratio[0] = sensor_ctrl_last[id].gain_ratio[0];
	sensor_ctrl->gain_ratio[1] = sensor_ctrl_last[id].gain_ratio[1];
}

#if defined(__FREERTOS)
void sen_get_expt_imx322(CTL_SEN_ID id, void *param)
#else
static void sen_get_expt_imx322(CTL_SEN_ID id, void *param)
#endif
{
	ISP_SENSOR_CTRL *sensor_ctrl = (ISP_SENSOR_CTRL *)param;

	sensor_ctrl->exp_time[0] = sensor_ctrl_last[id].exp_time[0];
	sensor_ctrl->exp_time[1] = sensor_ctrl_last[id].exp_time[1];
}

static void sen_get_min_expt_imx322(CTL_SEN_ID id, void *param)
{
	UINT32 *min_exp_time = (UINT32 *)param;
	UINT32 t_row = 0, min_shr = 0, min_exp_line = 0;

	sen_get_min_shr_imx322(cur_sen_mode[id], &min_shr, &min_exp_line);

	t_row = sen_calc_rowtime_imx322(id, cur_sen_mode[id]);
	*min_exp_time = t_row * min_exp_line / 10 + 1;
}

static void sen_get_mode_basic_imx322(CTL_SENDRV_GET_MODE_BASIC_PARAM *mode_basic)
{
	UINT32 mode = mode_basic->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(mode_basic, &mode_basic_param[mode], sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
}

static void sen_get_attr_basic_imx322(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_BASIC_PARAM *data)
{
	memcpy(data, &basic_param, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
}

static void sen_get_attr_signal_imx322(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM *data)
{
	memcpy(data, &signal_param, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
}

static ER sen_get_attr_cmdif_imx322(CTL_SEN_ID id, CTL_SENDRV_GET_ATTR_CMDIF_PARAM *data)
{
	#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	data->info.i2c.ch = sen_i2c[id].id;
	data->info.i2c.w_addr_info[0].w_addr = sen_i2c[id].addr;
	data->info.i2c.cur_w_addr_info.w_addr_sel = data->info.i2c.w_addr_info[0].w_addr_sel;
	data->info.i2c.cur_w_addr_info.w_addr = data->info.i2c.w_addr_info[0].w_addr;
	#else
	data->type = CTL_SEN_CMDIF_TYPE_I2C;
	memcpy(&data->info, &i2c, sizeof(CTL_SENDRV_I2C));
	#endif
	return E_OK;	
}

static ER sen_get_attr_if_imx322(CTL_SENDRV_GET_ATTR_IF_PARAM *data)
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

static void sen_get_fps_imx322(CTL_SEN_ID id, CTL_SENDRV_GET_FPS_PARAM *data)
{
	data->cur_fps = sen_get_cur_fps_imx322(id);
	data->chg_fps = sen_get_chgmode_fps_imx322(id);
}

static void sen_get_speed_imx322(CTL_SEN_ID id, CTL_SENDRV_GET_SPEED_PARAM *data)
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

static void sen_get_mode_para_imx322(CTL_SENDRV_GET_MODE_PARA_PARAM *data)
{
	UINT32 mode = data->mode;
	
	if (mode >= SEN_MAX_MODE) {
		mode = 0;
	}
	memcpy(data, &mode_param[mode], sizeof(CTL_SENDRV_GET_MODE_PARA_PARAM));
}

static void sen_get_modesel_imx322(CTL_SENDRV_GET_MODESEL_PARAM *data)
{
	if (data->if_type != CTL_SEN_IF_TYPE_PARALLEL) {
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

static UINT32 sen_calc_rowtime_step_imx322(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 div_step = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	if (mode_basic_param[mode].mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		div_step = 2;
	} else {
		div_step = 1;
	}

	return div_step;
}

static UINT32 sen_calc_rowtime_imx322(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
	UINT32 row_time = 0;

	if (mode >= SEN_MAX_MODE) {
		mode = cur_sen_mode[id];
	}

	//Precision * 10
	row_time = 1000 * (mode_basic_param[mode].signal_info.hd_period) / ((speed_param[mode].pclk) / 10000);

	return row_time;
}

static void sen_get_rowtime_imx322(CTL_SEN_ID id, CTL_SENDRV_GET_MODE_ROWTIME_PARAM *data)
{
	data->row_time_step = sen_calc_rowtime_step_imx322(id, data->mode);	
	data->row_time = sen_calc_rowtime_imx322(id, data->mode) * (data->row_time_step);
}

static void sen_set_cur_fps_imx322(CTL_SEN_ID id, UINT32 fps)
{
	cur_fps[id] = fps;
}

static UINT32 sen_get_cur_fps_imx322(CTL_SEN_ID id)
{
	return cur_fps[id];
}

static void sen_set_chgmode_fps_imx322(CTL_SEN_ID id, UINT32 fps)
{
	chgmode_fps[id] = fps;
}

static UINT32 sen_get_chgmode_fps_imx322(CTL_SEN_ID id)
{
	return chgmode_fps[id];
}

#if defined(__FREERTOS)
void sen_get_i2c_id_imx322(CTL_SEN_ID id, UINT32 *i2c_id)
{
	#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
	*i2c_id = sen_i2c[id].id;
	#else
	*i2c_id = 0;
	#endif
}

void sen_get_i2c_addr_imx322(CTL_SEN_ID id, UINT32 *i2c_addr)
{
	#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
	*i2c_addr = sen_i2c[id].addr;
	#else
	*i2c_addr = 0;
	#endif
}
int sen_init_imx322(SENSOR_DTSI_INFO *info)
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

	sprintf(compatible, "nvt,sen_imx322");
	if (sen_common_check_compatible(compatible)) {
		DBG_DUMP("compatible valid, using peri-dev.dtsi \r\n");
		sen_common_load_cfg_preset_compatible(compatible, &sen_preset);
		sen_common_load_cfg_direction_compatible(compatible, &sen_direction);
		sen_common_load_cfg_power_compatible(compatible, &sen_power);
		#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
		sen_common_load_cfg_i2c_compatible(compatible, &sen_i2c);
		#endif
	} else if (info->addr != NULL) {
		DBG_DUMP("compatible not valid, using sensor.dtsi \r\n");
		sprintf(node_path, "/sensor/sen_cfg/sen_imx322");
		sen_common_load_cfg_map(info->addr, node_path, &sen_map);
		sen_common_load_cfg_preset(info->addr, node_path, &sen_preset);
		sen_common_load_cfg_direction(info->addr, node_path, &sen_direction);
		sen_common_load_cfg_power(info->addr, node_path, &sen_power);
		#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
		sen_common_load_cfg_i2c(info->addr, node_path, &sen_i2c);
		#endif
	} else {
		DBG_WRN("DTSI addr is NULL \r\n");
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_imx322;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_imx322();
	rt = ctl_sen_reg_sendrv("nvt_sen_imx322", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

void sen_exit_imx322(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_imx322");
}

#else
static int __init sen_init_imx322(void)
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
			#if (TRANS_PROTOCOL == TRANS_PROTOCOL_I2C)
			sen_common_load_cfg_i2c(pcfg_file, &sen_i2c);
			#else
			#if defined(__KERNEL__)
			sen_common_load_cfg_spi(pcfg_file, &sen_spi);
			#endif
			#endif
			sen_common_close_cfg(pcfg_file);
		} else {
			DBG_WRN("load cfg fail \r\n");
		}
	}

	memset((void *)(&reg_obj), 0, sizeof(CTL_SEN_REG_OBJ));
	reg_obj.pwr_ctrl = sen_pwr_ctrl_imx322;
	reg_obj.det_plug_in = NULL;
	reg_obj.drv_tab = sen_get_drv_tab_imx322();
	rt = ctl_sen_reg_sendrv("nvt_sen_imx322", &reg_obj);
	if (rt != E_OK) {
		DBG_WRN("register sensor driver fail \r\n");
	}

	return rt;
}

static void __exit sen_exit_imx322(void)
{
	ctl_sen_unreg_sendrv("nvt_sen_imx322");
}

module_init(sen_init_imx322);
module_exit(sen_exit_imx322);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION(SEN_IMX322_MODULE_NAME);
MODULE_LICENSE("GPL");
#endif

