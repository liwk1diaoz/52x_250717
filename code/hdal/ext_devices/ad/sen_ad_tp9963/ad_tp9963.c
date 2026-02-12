#include "ad_tp9963.h"
#include "nvt_ad_tp9963_reg.h"

#include "tp2802.h"

// AD driver version
AD_DRV_MODULE_VERSION(0, 01, 000, 00);

// mapping table: vin raw data -> meaningful vin info
static AD_TP9963_DET_MAP g_ad_tp9963_det_map[] = {
	{.raw_mode = {.mode = TP2802_SD, 		.eq = 0}, .vin_mode = {.width =  960, .height =  576, .fps = 5000, .prog = FALSE, .std = STD_TVI}}, ///< CVBS  PAL    960x576  25(50i)FPS
	{.raw_mode = {.mode = TP2802_SD, 		.eq = 1}, .vin_mode = {.width =  960, .height =  480, .fps = 6000, .prog = FALSE, .std = STD_TVI}}, ///< CVBS NTSC    960x480  30(60i)FPS
	{.raw_mode = {.mode = TP2802_720P30, 	.eq = 0}, .vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_HDA}}, ///< AHD  720P   1280x720  30FPS
	{.raw_mode = {.mode = TP2802_720P30, 	.eq = 1}, .vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_TVI}}, ///< TVI  720P   1280x720  30FPS
	{.raw_mode = {.mode = TP2802_720P25, 	.eq = 0}, .vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_HDA}}, ///< AHD  720P   1280x720  25FPS
	{.raw_mode = {.mode = TP2802_720P25, 	.eq = 1}, .vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_TVI}}, ///< TVI  720P   1280x720  25FPS
	{.raw_mode = {.mode = TP2802_720P60, 	.eq = 0}, .vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE, .std = STD_HDA}}, ///< AHD  720P   1280x720  60FPS
	{.raw_mode = {.mode = TP2802_720P60, 	.eq = 1}, .vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE, .std = STD_TVI}}, ///< TVI  720P   1280x720  60FPS
	{.raw_mode = {.mode = TP2802_720P50, 	.eq = 0}, .vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE, .std = STD_HDA}}, ///< AHD  720P   1280x720  50FPS
	{.raw_mode = {.mode = TP2802_720P50, 	.eq = 1}, .vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE, .std = STD_TVI}}, ///< TVI  720P   1280x720  50FPS
	{.raw_mode = {.mode = TP2802_1080P30, 	.eq = 0}, .vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE, .std = STD_HDA}}, ///< AHD 1080P  1920x1080  30FPS
	{.raw_mode = {.mode = TP2802_1080P30, 	.eq = 1}, .vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE, .std = STD_TVI}}, ///< TVI 1080P  1920x1080  30FPS
	{.raw_mode = {.mode = TP2802_1080P25, 	.eq = 0}, .vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE, .std = STD_HDA}}, ///< AHD 1080P  1920x1080  25FPS
	{.raw_mode = {.mode = TP2802_1080P25, 	.eq = 1}, .vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE, .std = STD_TVI}}, ///< TVI 1080P  1920x1080  25FPS
};

// mapping table: meaningful vin info (+ extra info) -> external decoder's input channel mode
static AD_TP9963_VIN_MAP g_ad_tp9963_vin_map[] = {
	{.vin_mode = {.width =  960, .height =  576, .fps = 5000, .prog = FALSE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_PAL, 		.std = STD_TVI}},
	{.vin_mode = {.width =  960, .height =  480, .fps = 6000, .prog = FALSE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_NTSC, 		.std = STD_TVI}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_720P25, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 2500, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_720P25, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_720P30, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 3000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_720P30, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_720P50, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 5000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_720P50, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_720P60, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1280, .height =  720, .fps = 6000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_720P60, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_1080P25, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 2500, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_1080P25, 	.std = STD_HDA}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE, .std = STD_TVI}, .ch_mode = {.mode = TP2802_1080P30, 	.std = STD_TVI}},
	{.vin_mode = {.width = 1920, .height = 1080, .fps = 3000, .prog =  TRUE, .std = STD_HDA}, .ch_mode = {.mode = TP2802_1080P30, 	.std = STD_HDA}},
};

// mapping table: meaningful vout info -> external decoder's output port mode
/*static AD_TP9963_VOUT_MAP g_ad_tp9963_vout_map[] = {
};*/

// slave addresses supported by decoder
static UINT32 g_ad_tp9963_slv_addr[] = {0x88, 0x8A, 0x8C, 0x8E};

// decoder dump all register information
#define AD_TP9963_BANK_ADDR 0x40
static AD_DRV_DBG_REG_BANK g_ad_tp9963_bank[] = {
	{.id = 0x00, .len = 0x100},
	{.id = 0x01, .len = 0x100},
	{.id = 0x08, .len = 0x040},
};
#define AD_TP9963_REG_DUMP_PER_ROW 16

// default value of AD_VIN.dft_width/dft_height/dft_fps/dft_prog
#define AD_TP9963_DFT_WIDTH 1920
#define AD_TP9963_DFT_HEIGHT 1080
#define AD_TP9963_DFT_FPS 2500
#define AD_TP9963_DFT_PROG TRUE
#define AD_TP9963_DFT_STD STD_TVI

// i2c retry number when operation fail occur
#define AD_TP9963_I2C_RETRY_NUM 5

// global variable
#define AD_TP9963_MODULE_NAME	AD_DRV_TOKEN_STR(_MODEL_NAME)
static AD_VIN g_ad_tp9963_vin[AD_TP9963_CHIP_MAX][AD_TP9963_VIN_MAX];
static AD_VOUT g_ad_tp9963_vout[AD_TP9963_CHIP_MAX][AD_TP9963_VOUT_MAX];
static AD_INFO g_ad_tp9963_info[AD_TP9963_CHIP_MAX] = {
	{
		.name = "TP9963_0",
		.vin_max = AD_TP9963_VIN_MAX,
		.vin = &g_ad_tp9963_vin[0][0],
		.vout_max = AD_TP9963_VOUT_MAX,
		.vout = &g_ad_tp9963_vout[0][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TP9963_1",
		.vin_max = AD_TP9963_VIN_MAX,
		.vin = &g_ad_tp9963_vin[1][0],
		.vout_max = AD_TP9963_VOUT_MAX,
		.vout = &g_ad_tp9963_vout[1][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TP9963_2",
		.vin_max = AD_TP9963_VIN_MAX,
		.vin = &g_ad_tp9963_vin[2][0],
		.vout_max = AD_TP9963_VOUT_MAX,
		.vout = &g_ad_tp9963_vout[2][0],
		.rev = {0, 0, 0, 0, 0},
	},

	{
		.name = "TP9963_3",
		.vin_max = AD_TP9963_VIN_MAX,
		.vin = &g_ad_tp9963_vin[3][0],
		.vout_max = AD_TP9963_VOUT_MAX,
		.vout = &g_ad_tp9963_vout[3][0],
		.rev = {0, 0, 0, 0, 0},
	},
};

static AD_TP9963_INFO g_ad_tp9963_info_private[AD_TP9963_CHIP_MAX] = {
	[0 ... (AD_TP9963_CHIP_MAX-1)] = {
		.is_inited = FALSE,
		.raw_mode = {
			[0 ... (AD_TP9963_VIN_MAX-1)] = {
				.mode = 0,
				.eq = 1,
			},
		},
		.det_vin_mode = {
			[0 ... (AD_TP9963_VIN_MAX-1)] = {0}
		},
		.mipi_mode = {
			[0 ... (AD_TP9963_VOUT_MAX-1)] = AD_TP9963_MIPI_MODE_UNKNOWN
		},
		.virtual_ch_id = {0, 1},
		.pat_gen_mode = {
			[0 ... (AD_TP9963_VIN_MAX-1)] = AD_TP9963_PATTERN_GEN_MODE_AUTO
		},
		.wdi = {
			[0 ... (AD_TP9963_VIN_MAX-1)] = {
				.count = 0,
				.mode = INVALID_FORMAT,
				.scan = SCAN_AUTO,
				.gain = {0, 0, 0, 0},
				.std = AD_TP9963_DFT_STD,
				.state = VIDEO_UNPLUG,
				.force = 0,
			}
		},
		.det_type = {
			[0 ... (AD_TP9963_VIN_MAX-1)] = AD_TYPE_AUTO
		},
		.ahd_det_idx = {
			[0 ... (AD_TP9963_VIN_MAX-1)] = 0
		},
	}
};

static ER ad_tp9963_open(UINT32 chip_id, void *ext_data);
static ER ad_tp9963_close(UINT32 chip_id, void *ext_data);
static ER ad_tp9963_init(UINT32 chip_id, void *ext_data);
static ER ad_tp9963_uninit(UINT32 chip_id, void *ext_data);
static ER ad_tp9963_get_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_tp9963_set_cfg(UINT32 chip_id, AD_CFGID item, void *data);
static ER ad_tp9963_chgmode(UINT32 chip_id, void *ext_data);
static ER ad_tp9963_watchdog_cb(UINT32 chip_id, void *ext_data);
static ER ad_tp9963_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len);
static ER ad_tp9963_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len);
static ER ad_tp9963_dbg(char *str_cmd);

static AD_DEV g_ad_tp9963_obj = {
	AD_TP9963_CHIP_MAX,
	g_ad_tp9963_info,

	sizeof(g_ad_tp9963_slv_addr) / sizeof(typeof(g_ad_tp9963_slv_addr[0])),
	g_ad_tp9963_slv_addr,

	{
		.open = ad_tp9963_open,
		.close = ad_tp9963_close,
		.init = ad_tp9963_init,
		.uninit = ad_tp9963_uninit,
		.get_cfg = ad_tp9963_get_cfg,
		.set_cfg = ad_tp9963_set_cfg,
		.chgmode = ad_tp9963_chgmode,
		.det_plug_in = ad_tp9963_watchdog_cb,
		.i2c_write = ad_tp9963_i2c_write,
		.i2c_read = ad_tp9963_i2c_read,
#if !defined(__UITRON) && !defined(__ECOS)
		.pwr_ctl = ad_drv_pwr_ctrl,
#endif
		.dbg_info = ad_tp9963_dbg,
	}
};

// internal function
static AD_INFO *ad_tp9963_get_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_TP9963_CHIP_MAX)) {
		AD_ERR("ad get info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_TP9963_CHIP_MAX);
		return NULL;
	}
	return &g_ad_tp9963_info[chip_id];
}

static AD_TP9963_INFO *ad_tp9963_get_private_info(UINT32 chip_id)
{
	if (unlikely(chip_id >= AD_TP9963_CHIP_MAX)) {
		AD_ERR("ad get private info fail. chip_id (%u) > max (%u)\r\n", chip_id, AD_TP9963_CHIP_MAX);
		return NULL;
	}
	return &g_ad_tp9963_info_private[chip_id];
}

static void ad_tp9963_set2def(AD_INFO *ad_info)
{
	UINT32 i;

	if (ad_info == NULL) {
		AD_ERR("[%s] set2def fail. NULL ad info\r\n", AD_TP9963_MODULE_NAME);
		return;
	}
	ad_info->status = AD_STS_UNKNOWN;

	for (i = 0; i < ad_info->vin_max; i++) {
		ad_info->vin[i].active = FALSE;
		ad_info->vin[i].vloss = TRUE;
		ad_info->vin[i].ui_format.width = AD_TP9963_DFT_WIDTH;
		ad_info->vin[i].ui_format.height = AD_TP9963_DFT_HEIGHT;
		ad_info->vin[i].ui_format.fps = AD_TP9963_DFT_FPS;
		ad_info->vin[i].ui_format.prog = AD_TP9963_DFT_PROG;

		ad_info->vin[i].cur_format.width = AD_TP9963_DFT_WIDTH;
		ad_info->vin[i].cur_format.height = AD_TP9963_DFT_HEIGHT;
		ad_info->vin[i].cur_format.fps = AD_TP9963_DFT_FPS;
		ad_info->vin[i].cur_format.prog = AD_TP9963_DFT_PROG;

		ad_info->vin[i].det_format.width = AD_TP9963_DFT_WIDTH;
		ad_info->vin[i].det_format.height = AD_TP9963_DFT_HEIGHT;
		ad_info->vin[i].det_format.fps = AD_TP9963_DFT_FPS;
		ad_info->vin[i].det_format.prog = AD_TP9963_DFT_PROG;
	}

	for (i = 0; i < ad_info->vout_max; i++) {
		ad_info->vout[i].active = FALSE;
	}
}

#if 0
#endif

static ER ad_tp9963_set_reg_page(UINT32 chip_id, UINT32 page)
{
	ad_tp9963_i2c_write(chip_id, AD_TP9963_PAGE_REG_OFS, page, AD_TP9963_I2C_DATA_LEN);
	return E_OK;
}

static ER ad_tp9963_get_device_id(UINT32 chip_id, UINT32 *device_id)
{
	AD_TP9963_DEVICE_ID_15_8_REG id_h = {0};
	AD_TP9963_DEVICE_ID_7_0_REG id_l = {0};

	ad_tp9963_i2c_read(chip_id, AD_TP9963_DEVICE_ID_15_8_REG_OFS, &id_h.reg, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_DEVICE_ID_7_0_REG_OFS, &id_l.reg, AD_TP9963_I2C_DATA_LEN);

	*device_id = (id_h.bit.DEVICE_ID << 8) | id_l.bit.DEVICE_ID;

	return E_OK;
}

static ER ad_tp9963_get_revision_id(UINT32 chip_id, UINT32 *revision_id)
{
	AD_TP9963_REVISION_REG id = {0};

	ad_tp9963_i2c_read(chip_id, AD_TP9963_REVISION_REG_OFS, &id.reg, AD_TP9963_I2C_DATA_LEN);

	*revision_id = id.bit.REVISION;

	return E_OK;
}

static ER ad_tp9963_get_vloss(UINT32 chip_id, BOOL *vloss)
{
	AD_TP9963_VIDEO_INPUT_STATUS_REG input_sts = {0};

	ad_tp9963_i2c_read(chip_id, AD_TP9963_VIDEO_INPUT_STATUS_REG_OFS, &input_sts.reg, AD_TP9963_I2C_DATA_LEN);

	*vloss = (input_sts.bit.VDLOSS != FALSE);

	return E_OK;
}

static ER ad_tp9963_get_hlock(UINT32 chip_id, BOOL *hlock)
{
	AD_TP9963_VIDEO_INPUT_STATUS_REG input_sts = {0};

	ad_tp9963_i2c_read(chip_id, AD_TP9963_VIDEO_INPUT_STATUS_REG_OFS, &input_sts.reg, AD_TP9963_I2C_DATA_LEN);

	*hlock = (input_sts.bit.HLOCK != FALSE);

	return E_OK;
}

static ER ad_tp9963_get_vlock(UINT32 chip_id, BOOL *vlock)
{
	AD_TP9963_VIDEO_INPUT_STATUS_REG input_sts = {0};

	ad_tp9963_i2c_read(chip_id, AD_TP9963_VIDEO_INPUT_STATUS_REG_OFS, &input_sts.reg, AD_TP9963_I2C_DATA_LEN);

	*vlock = (input_sts.bit.VLOCK != FALSE);

	return E_OK;
}

static ER ad_tp9963_get_carrier(UINT32 chip_id, BOOL *carrier)
{
	AD_TP9963_VIDEO_INPUT_STATUS_REG input_sts = {0};

	ad_tp9963_i2c_read(chip_id, AD_TP9963_VIDEO_INPUT_STATUS_REG_OFS, &input_sts.reg, AD_TP9963_I2C_DATA_LEN);

	*carrier = (input_sts.bit.CDET == FALSE);

	return E_OK;
}

static ER ad_tp9963_get_eq_detect(UINT32 chip_id, UINT32 *eq_detect)
{
	AD_TP9963_VIDEO_INPUT_STATUS_REG input_sts = {0};

	ad_tp9963_i2c_read(chip_id, AD_TP9963_VIDEO_INPUT_STATUS_REG_OFS, &input_sts.reg, AD_TP9963_I2C_DATA_LEN);

	*eq_detect = input_sts.bit.EQDET;

	return E_OK;
}

static ER ad_tp9963_get_cvstd(UINT32 chip_id, UINT32 *cvstd)
{
	AD_TP9963_DETECTION_STATUS_REG det_sts = {0};

	ad_tp9963_i2c_read(chip_id, AD_TP9963_DETECTION_STATUS_REG_OFS, &det_sts.reg, AD_TP9963_I2C_DATA_LEN);

	*cvstd = det_sts.bit.CVSTD;

	return E_OK;
}

static ER ad_tp9963_get_mipi_clk_rate(UINT32 chip_id, UINT32 *val)
{
	*val = 297000000;
	return E_OK;
}

#define AD_TP9963_UNIT_INTERVAL(rate) ((rate) < 1 ? 1000000000 : (1000000000 / (rate)))
#define AD_TP9963_REG_DLY(rate) (AD_TP9963_UNIT_INTERVAL(rate) * 8)
#define AD_TP9963_REG_DLY_2_TIME_DLY(reg_dly, rate) (((reg_dly) + 1) * AD_TP9963_REG_DLY(rate))
#define AD_TP9963_TIME_DLY_2_REG_DLY(time_dly, rate) ((time_dly) < AD_TP9963_REG_DLY(rate) ? 0 : (((time_dly) / AD_TP9963_REG_DLY(rate)) - 1))

static ER ad_tp9963_get_mipi_t_prep_reg(UINT32 chip_id, UINT32 *val)
{
	ad_tp9963_set_reg_page(chip_id, MIPI_PAGE);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_MIPI_T_PREP_REG_OFS, val, AD_TP9963_I2C_DATA_LEN);
	return E_OK;
}

static ER ad_tp9963_set_mipi_t_prep_reg(UINT32 chip_id, UINT32 val)
{
	ad_tp9963_set_reg_page(chip_id, MIPI_PAGE);
	ad_tp9963_i2c_write(chip_id, AD_TP9963_MIPI_T_PREP_REG_OFS, val, AD_TP9963_I2C_DATA_LEN);
	return E_OK;
}

static ER ad_tp9963_get_mipi_t_trail_reg(UINT32 chip_id, UINT32 *val)
{
	ad_tp9963_set_reg_page(chip_id, MIPI_PAGE);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_MIPI_T_TRAIL_REG_OFS, val, AD_TP9963_I2C_DATA_LEN);
	return E_OK;
}

static ER ad_tp9963_set_mipi_t_trail_reg(UINT32 chip_id, UINT32 val)
{
	ad_tp9963_set_reg_page(chip_id, MIPI_PAGE);
	ad_tp9963_i2c_write(chip_id, AD_TP9963_MIPI_T_TRAIL_REG_OFS, val, AD_TP9963_I2C_DATA_LEN);
	return E_OK;
}

static ER ad_tp9963_get_mipi_t_prep_time(UINT32 chip_id, UINT32 *val)
{
	UINT32 clk_rate = 0, reg = 0;

	ad_tp9963_get_mipi_clk_rate(chip_id, &clk_rate);
	ad_tp9963_get_mipi_t_prep_reg(chip_id, &reg);
	*val = AD_TP9963_REG_DLY_2_TIME_DLY(reg+1, clk_rate); // add extra HS0 (8UI time)
	return E_OK;
}

static ER ad_tp9963_set_mipi_t_prep_time(UINT32 chip_id, UINT32 val)
{
	UINT32 clk_rate = 0, reg;

	ad_tp9963_get_mipi_clk_rate(chip_id, &clk_rate);
	reg = AD_TP9963_TIME_DLY_2_REG_DLY(val, clk_rate);
	if (reg >= 1) {
		reg--; // consider extra HS0 (8UI time)
	}
	ad_tp9963_set_mipi_t_prep_reg(chip_id, reg);
	return E_OK;
}

static ER ad_tp9963_get_mipi_t_trail_time(UINT32 chip_id, UINT32 *val)
{
	UINT32 clk_rate = 0, reg = 0;

	ad_tp9963_get_mipi_clk_rate(chip_id, &clk_rate);
	ad_tp9963_get_mipi_t_trail_reg(chip_id, &reg);
	*val = AD_TP9963_REG_DLY_2_TIME_DLY(reg, clk_rate);
	return E_OK;
}

static ER ad_tp9963_set_mipi_t_trail_time(UINT32 chip_id, UINT32 val)
{
	UINT32 clk_rate = 0, reg;

	ad_tp9963_get_mipi_clk_rate(chip_id, &clk_rate);
	reg = AD_TP9963_TIME_DLY_2_REG_DLY(val, clk_rate);
	ad_tp9963_set_mipi_t_trail_reg(chip_id, reg);
	return E_OK;
}

static ER ad_tp9963_get_mipi_clk_lane_stop(UINT32 chip_id, UINT32 lane_id, UINT32 *stop)
{
	AD_TP9963_MIPI_STOPCLK_REG stopclk_reg = {0};

	ad_tp9963_set_reg_page(chip_id, MIPI_PAGE);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_MIPI_STOPCLK_REG_OFS, &stopclk_reg.reg, AD_TP9963_I2C_DATA_LEN);

	switch (lane_id) {
	case 0:
		*stop = (stopclk_reg.bit.STOPCLK != 0);
		break;
	default:
		break;
	}

	return E_OK;
}

static ER ad_tp9963_set_mipi_clk_lane_stop(UINT32 chip_id, UINT32 lane_id, UINT32 stop)
{
	AD_TP9963_MIPI_STOPCLK_REG stopclk_reg = {0};

	ad_tp9963_set_reg_page(chip_id, MIPI_PAGE);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_MIPI_STOPCLK_REG_OFS, &stopclk_reg.reg, AD_TP9963_I2C_DATA_LEN);

	switch (lane_id) {
	case 0:
		stopclk_reg.bit.STOPCLK = (stop != 0);
		break;
	default:
		break;
	}

	ad_tp9963_i2c_write(chip_id, AD_TP9963_MIPI_STOPCLK_REG_OFS, stopclk_reg.reg, AD_TP9963_I2C_DATA_LEN);

	return E_OK;
}

static ER ad_tp9963_get_virtual_ch_src(UINT32 chip_id, UINT32 virtual_ch_id, UINT32 *vin_id)
{
	AD_TP9963_MIPI_VIRTUAL_CHANNEL_ID_REG vch_id_reg = {0};

	ad_tp9963_set_reg_page(chip_id, MIPI_PAGE);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_MIPI_VIRTUAL_CHANNEL_ID_REG_OFS, &vch_id_reg.reg, AD_TP9963_I2C_DATA_LEN);

	switch (virtual_ch_id) {
	case 0:
		*vin_id = vch_id_reg.bit.CH1_VCI;
		break;
	case 1:
		*vin_id = vch_id_reg.bit.CH2_VCI;
		break;
	case 2:
		*vin_id = vch_id_reg.bit.CH3_VCI;
		break;
	case 3:
		*vin_id = vch_id_reg.bit.CH4_VCI;
		break;
	default:
		break;
	}

	return E_OK;
}

static ER ad_tp9963_set_virtual_ch_src(UINT32 chip_id, UINT32 virtual_ch_id, UINT32 vin_id)
{
	AD_TP9963_MIPI_VIRTUAL_CHANNEL_ID_REG vch_id_reg = {0};

	ad_tp9963_set_reg_page(chip_id, MIPI_PAGE);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_MIPI_VIRTUAL_CHANNEL_ID_REG_OFS, &vch_id_reg.reg, AD_TP9963_I2C_DATA_LEN);

	switch (virtual_ch_id) {
	case 0:
		vch_id_reg.bit.CH1_VCI = vin_id;
		break;
	case 1:
		vch_id_reg.bit.CH2_VCI = vin_id;
		break;
	case 2:
		vch_id_reg.bit.CH3_VCI = vin_id;
		break;
	case 3:
		vch_id_reg.bit.CH4_VCI = vin_id;
		break;
	default:
		break;
	}

	ad_tp9963_i2c_write(chip_id, AD_TP9963_MIPI_VIRTUAL_CHANNEL_ID_REG_OFS, vch_id_reg.reg, AD_TP9963_I2C_DATA_LEN);

	return E_OK;
}

static ER ad_tp9963_get_pattern_gen_mode(UINT32 chip_id, UINT32 vin_id, AD_TP9963_PATTERN_GEN_MODE *mode)
{
	AD_TP9963_COLOR_HPLL_FREERUN_CTRL_REG ctrl_reg = {0};

	ad_tp9963_set_reg_page(chip_id, vin_id);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_COLOR_HPLL_FREERUN_CTRL_REG_OFS, &ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);

	if (ctrl_reg.bit.LCS == 1 && ctrl_reg.bit.FCS == 1) {
		*mode = AD_TP9963_PATTERN_GEN_MODE_FORCE;
	} else if (ctrl_reg.bit.LCS == 1 && ctrl_reg.bit.FCS == 0) {
		*mode = AD_TP9963_PATTERN_GEN_MODE_AUTO;
	} else {
		*mode = AD_TP9963_PATTERN_GEN_MODE_DISABLE;
	}

	return E_OK;
}

static ER ad_tp9963_set_pattern_gen_mode(UINT32 chip_id, UINT32 vin_id, AD_TP9963_PATTERN_GEN_MODE mode)
{
	AD_TP9963_COLOR_HPLL_FREERUN_CTRL_REG ctrl_reg = {0};

	ad_tp9963_set_reg_page(chip_id, vin_id);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_COLOR_HPLL_FREERUN_CTRL_REG_OFS, &ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);

	switch (mode) {
	case AD_TP9963_PATTERN_GEN_MODE_DISABLE:
		ctrl_reg.bit.LCS = 0;
		ctrl_reg.bit.FCS = 0;
		break;
	case AD_TP9963_PATTERN_GEN_MODE_AUTO:
	default:
		ctrl_reg.bit.LCS = 1;
		ctrl_reg.bit.FCS = 0;
		break;
	case AD_TP9963_PATTERN_GEN_MODE_FORCE:
		ctrl_reg.bit.LCS = 1;
		ctrl_reg.bit.FCS = 1;
		break;
	}

	ad_tp9963_i2c_write(chip_id, AD_TP9963_COLOR_HPLL_FREERUN_CTRL_REG_OFS, ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);

	return E_OK;
}

static ER ad_tp9963_get_sywd(UINT32 chip_id, UINT32 *sywd)
{
	AD_TP9963_DETECTION_STATUS_REG det_sts = {0};

	ad_tp9963_i2c_read(chip_id, AD_TP9963_DETECTION_STATUS_REG_OFS, &det_sts.reg, AD_TP9963_I2C_DATA_LEN);

	*sywd = det_sts.bit.SYWD;

	return E_OK;
}

static ER ad_tp9963_set_clamp_ctrl(UINT32 chip_id, UINT32 mode)
{
	AD_TP9963_CLAMP_CTRL_REG clamp_ctrl = {0};

	ad_tp9963_i2c_read(chip_id, AD_TP9963_CLAMP_CTRL_REG_OFS, &clamp_ctrl.reg, AD_TP9963_I2C_DATA_LEN);

	clamp_ctrl.bit.CLMD = (mode & 0x1);

	ad_tp9963_i2c_write(chip_id, AD_TP9963_CLAMP_CTRL_REG_OFS, clamp_ctrl.reg, AD_TP9963_I2C_DATA_LEN);

	return E_OK;
}

static void ad_tp9963_reset_default(UINT32 chip_id, UINT32 ch)
{
    UINT32 tmp = 0;

	ad_tp9963_i2c_write(chip_id, 0x07, 0xC0, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x0B, 0xC0, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_read(chip_id, 0x26, &tmp, AD_TP9963_I2C_DATA_LEN);
	tmp &= 0xfe;
	ad_tp9963_i2c_write(chip_id, 0x26, tmp, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_read(chip_id, 0xa7, &tmp, AD_TP9963_I2C_DATA_LEN);
	tmp &= 0xfe;
	ad_tp9963_i2c_write(chip_id, 0xa7, tmp, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_read(chip_id, 0x06, &tmp, AD_TP9963_I2C_DATA_LEN);
	tmp &= 0xfb;
	ad_tp9963_i2c_write(chip_id, 0x06, tmp, AD_TP9963_I2C_DATA_LEN);
}

static ER ad_tp9963_read_egain(UINT32 chip_id, UINT32 *egain)
{
	ad_tp9963_i2c_write(chip_id, AD_TP9963_TEST_REG_OFS, 0x0, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_INTERNAL_STATUS_REG_OFS, egain, AD_TP9963_I2C_DATA_LEN);

	return E_OK;
}

static ER ad_tp9963_egain(UINT32 chip_id, UINT32 CGAIN_STD)
{
	UINT32 tmp = 0, cgain = 0, retry = 30;

	ad_tp9963_i2c_write(chip_id, AD_TP9963_TEST_REG_OFS, 0x06, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_INTERNAL_STATUS_REG_OFS, &cgain, AD_TP9963_I2C_DATA_LEN);

	if (cgain < CGAIN_STD) {

		while (retry) {
			retry--;

			ad_tp9963_i2c_read(chip_id, AD_TP9963_EQ2_CTRL_REG_OFS, &tmp, AD_TP9963_I2C_DATA_LEN);
			tmp &= 0x3f;
			while (max(CGAIN_STD, cgain) - min(CGAIN_STD, cgain)) {
				if (tmp) tmp--;
				else break;
				cgain++;
			}

			ad_tp9963_i2c_write(chip_id, AD_TP9963_EQ2_CTRL_REG_OFS, 0x80|tmp, AD_TP9963_I2C_DATA_LEN);
			if (0 == tmp) break;
			vos_util_delay_ms(40);
			ad_tp9963_i2c_write(chip_id, AD_TP9963_TEST_REG_OFS, 0x06, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_read(chip_id, AD_TP9963_INTERNAL_STATUS_REG_OFS, &cgain, AD_TP9963_I2C_DATA_LEN);

			if (cgain > (CGAIN_STD+1)) {
				ad_tp9963_i2c_read(chip_id, AD_TP9963_EQ2_CTRL_REG_OFS, &tmp, AD_TP9963_I2C_DATA_LEN);
				tmp &= 0x3f;
				tmp += 0x02;
				if (tmp > 0x3f) tmp = 0x3f;
				ad_tp9963_i2c_write(chip_id, AD_TP9963_EQ2_CTRL_REG_OFS, 0x80|tmp, AD_TP9963_I2C_DATA_LEN);
				if (0x3f == tmp) break;
				vos_util_delay_ms(40);
				ad_tp9963_i2c_read(chip_id, AD_TP9963_INTERNAL_STATUS_REG_OFS, &cgain, AD_TP9963_I2C_DATA_LEN);
			}
			if ((max(cgain, CGAIN_STD) - min(cgain, CGAIN_STD)) < 2) break;
		}
	}

	return E_OK;
}

static ER ad_tp9963_set_csi_out_dly(UINT32 chip_id)
{
#if 0
#define AD_TP9963_PRE_DLY 40
	UINT32 t_prep, t_trail, out_dly, va;

	ad_tp9963_get_mipi_t_prep_time(chip_id, &t_prep);
	ad_tp9963_get_mipi_t_trail_time(chip_id, &t_trail);
	out_dly = t_prep + (t_trail / 2) - AD_TP9963_PRE_DLY;
	AD_DUMP("out_dly = 0x%x(%uns)\r\n", out_dly/16, out_dly);

	va = (UINT32)ioremap_cache(0xf0280080, 4);
	*(UINT32 *)va =  (out_dly/16) << 8;
//*(UINT32 *)va =  0x2000; // 1ch
*(UINT32 *)va =  0x700; // 2ch
	iounmap((void *)va);
#endif
	return E_OK;
}

typedef struct {
	UINT8 addr;
	UINT8 val;
} AD_TP9963_I2C_REG;

static ER ad_tp9963_i2c_write_tbl(UINT32 chip_id, const AD_TP9963_I2C_REG *tbl, UINT32 cnt)
{
	UINT32 i;

	for (i = 0; i < cnt; i++) {

		if (ad_tp9963_i2c_write(chip_id, tbl[i].addr, tbl[i].val, AD_TP9963_I2C_DATA_LEN) != E_OK) {
			return E_SYS;
		}
	}

	return E_OK;
}

const AD_TP9963_I2C_REG ad_tp9963_mode_pal[] = {
	{0x06, 0x32},
	{0x02, 0x47},
	{0x07, 0x80},
	{0x0b, 0x80},
	{0x0c, 0x13},
	{0x0d, 0x51},
	{0x15, 0x03},
	{0x16, 0xf0},
	{0x17, 0xa0},
	{0x18, 0x17},
	{0x19, 0x20},
	{0x1a, 0x15},
	{0x1c, 0x06},
	{0x1d, 0xc0},
	{0x20, 0x48},
	{0x21, 0x84},
	{0x22, 0x37},
	{0x23, 0x3f},
	{0x2b, 0x70},
	{0x2c, 0x2a},
	{0x2d, 0x4b},
	{0x2e, 0x56},
	{0x30, 0x7a},
	{0x31, 0x4a},
	{0x32, 0x4d},
	{0x33, 0xfb},
	{0x35, 0x65},
	{0x39, 0x04},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_ntsc[] = {
	{0x02, 0x47},
	{0x07, 0x80},
	{0x0b, 0x80},
	{0x0c, 0x13},
	{0x0d, 0x50},
	{0x15, 0x03},
	{0x16, 0xd6},
	{0x17, 0xa0},
	{0x18, 0x12},
	{0x19, 0xf0},
	{0x1a, 0x05},
	{0x1c, 0x06},
	{0x1d, 0xb4},
	{0x20, 0x40},
	{0x21, 0x84},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x2b, 0x70},
	{0x2c, 0x2a},
	{0x2d, 0x4b},
	{0x2e, 0x57},
	{0x30, 0x62},
	{0x31, 0xbb},
	{0x32, 0x96},
	{0x33, 0xcb},
	{0x35, 0x65},
	{0x39, 0x04},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_720p25_tvi[] = {
	{0x02, 0x42},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x13},
	{0x0d, 0x50},
	{0x15, 0x13},
	{0x16, 0x15},
	{0x17, 0x00},
	{0x18, 0x19},
	{0x19, 0xd0},
	{0x1a, 0x25},
	{0x1c, 0x07},   //1280*720, 25fps
	{0x1d, 0xbc},   //1280*720, 25fps
	{0x20, 0x30},
	{0x21, 0x84},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x2b, 0x60},
	{0x2c, 0x0a},
	{0x2d, 0x30},
	{0x2e, 0x70},
	{0x30, 0x48},
	{0x31, 0xbb},
	{0x32, 0x2e},
	{0x33, 0x90},
	{0x35, 0x25},
	{0x39, 0x08},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_720p25_ahd[] = {
	{0x02, 0x42},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x13},
	{0x0d, 0x70},
	{0x15, 0x13},
	{0x16, 0x58},
	{0x17, 0x00},
	{0x18, 0x1b},
	{0x19, 0xd0},
	{0x1a, 0x25},
	{0x1c, 0x87},   //1280*720, 25fps
	{0x1d, 0xba},   //1280*720, 25fps
	{0x20, 0x38},
	{0x21, 0x46},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x25, 0xfe},
	{0x26, 0x01},
	{0x27, 0xad},
	{0x2b, 0x60},
	{0x2c, 0x3a},
	{0x2d, 0x48},
	{0x2e, 0x40},
	{0x30, 0x4f},
	{0x31, 0x10},
	{0x32, 0x08},
	{0x33, 0x40},
	{0x35, 0x25},
	{0x39, 0x08},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_720p30_tvi[] = {
	{0x02, 0x42},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x13},
	{0x0d, 0x50},
	{0x15, 0x13},
	{0x16, 0x15},
	{0x17, 0x00},
	{0x18, 0x19},
	{0x19, 0xd0},
	{0x1a, 0x25},
	{0x1c, 0x06},   //1280*720, 30fps
	{0x1d, 0x72},   //1280*720, 30fps
	{0x20, 0x30},
	{0x21, 0x84},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x2b, 0x60},
	{0x2c, 0x0a},
	{0x2d, 0x30},
	{0x2e, 0x70},
	{0x30, 0x48},
	{0x31, 0xbb},
	{0x32, 0x2e},
	{0x33, 0x90},
	{0x35, 0x25},
	{0x39, 0x08},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_720p30_ahd[] = {
	{0x02, 0x42},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x13},
	{0x0d, 0x70},
	{0x15, 0x13},
	{0x16, 0x58},
	{0x17, 0x00},
	{0x18, 0x1b},
	{0x19, 0xd0},
	{0x1a, 0x25},
	{0x1c, 0x86},   //1280*720, 30fps
	{0x1d, 0x70},   //1280*720, 30fps
	{0x20, 0x38},
	{0x21, 0x46},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x25, 0xfe},
	{0x26, 0x01},
	{0x27, 0xad},
	{0x2b, 0x60},
	{0x2c, 0x3a},
	{0x2d, 0x48},
	{0x2e, 0x40},
	{0x30, 0x4e},
	{0x31, 0xe5},
	{0x32, 0x00},
	{0x33, 0xf0},
	{0x35, 0x25},
	{0x39, 0x08},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_1080p25_tvi[] = {
	{0x02, 0x40},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x50},
	{0x15, 0x03},
	{0x16, 0xd2},
	{0x17, 0x80},
	{0x18, 0x29},
	{0x19, 0x38},
	{0x1a, 0x47},
	{0x1c, 0x0a},   //1920*1080, 25fps
	{0x1d, 0x50},   //
	{0x20, 0x30},
	{0x21, 0x84},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x2b, 0x60},
	{0x2c, 0x0a},
	{0x2d, 0x30},
	{0x2e, 0x70},
	{0x30, 0x48},
	{0x31, 0xbb},
	{0x32, 0x2e},
	{0x33, 0x90},
	{0x35, 0x05},
	{0x39, 0x0C},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_1080p25_ahd[] = {
	{0x02, 0x40},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x70},
	{0x15, 0x03},
	{0x16, 0xd0},
	{0x17, 0x80},
	{0x18, 0x2a},
	{0x19, 0x38},
	{0x1a, 0x47},
	{0x1c, 0x8a},
	{0x1d, 0x4e},
	{0x20, 0x3c},
	{0x21, 0x46},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x27, 0xad},
	{0x2b, 0x60},
	{0x2c, 0x3a},
	{0x2d, 0x48},
	{0x2e, 0x40},
	{0x30, 0x52},
	{0x31, 0xc3},
	{0x32, 0x7d},
	{0x33, 0xa0},
	{0x35, 0x05},
	{0x39, 0x0C},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_1080p30_tvi[] = {
	{0x02, 0x40},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x50},
	{0x15, 0x03},
	{0x16, 0xd2},
	{0x17, 0x80},
	{0x18, 0x29},
	{0x19, 0x38},
	{0x1a, 0x47},
	{0x1c, 0x08},   //1920*1080, 30fps
	{0x1d, 0x98},   //
	{0x20, 0x30},
	{0x21, 0x84},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x2b, 0x60},
	{0x2c, 0x0a},
	{0x2d, 0x30},
	{0x2e, 0x70},
	{0x30, 0x48},
	{0x31, 0xbb},
	{0x32, 0x2e},
	{0x33, 0x90},
	{0x35, 0x05},
	{0x39, 0x0C},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_1080p30_ahd[] = {
	{0x02, 0x40},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x70},
	{0x15, 0x03},
	{0x16, 0xd0},
	{0x17, 0x80},
	{0x18, 0x2a},
	{0x19, 0x38},
	{0x1a, 0x47},
	{0x1c, 0x88},   //1920*1080, 30fps
	{0x1d, 0x96},   //
	{0x20, 0x38},
	{0x21, 0x46},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x27, 0xad},
	{0x2b, 0x60},
	{0x2c, 0x3a},
	{0x2d, 0x48},
	{0x2e, 0x40},
	{0x30, 0x52},
	{0x31, 0xca},
	{0x32, 0xf0},
	{0x33, 0x20},
	{0x35, 0x05},
	{0x39, 0x0C},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_1080p50[] = {
	{0x02, 0x40},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x50},
	{0x15, 0x03},
	{0x16, 0xe2},
	{0x17, 0x80},
	{0x18, 0x29},
	{0x19, 0x38},
	{0x1a, 0x47},
	{0x1c, 0x0a},   //1920*1080, 25fps
	{0x1d, 0x4e},   //
	{0x20, 0x38},
	{0x21, 0x84},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x27, 0xad},
	{0x2b, 0x60},
	{0x2c, 0x0a},
	{0x2d, 0x40},
	{0x2e, 0x70},
	{0x30, 0x74},
	{0x31, 0x9b},
	{0x32, 0xa5},
	{0x33, 0xe0},
	{0x35, 0x05},
	{0x38, 0x40},
	{0x39, 0x68},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_1080p60[] = {
	{0x02, 0x40},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x50},
	{0x15, 0x03},
	{0x16, 0xf0},
	{0x17, 0x80},
	{0x18, 0x29},
	{0x19, 0x38},
	{0x1a, 0x47},
	{0x1c, 0x08},   //1920*1080, 60fps
	{0x1d, 0x96},   //
	{0x20, 0x38},
	{0x21, 0x84},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x27, 0xad},
	{0x2b, 0x60},
	{0x2c, 0x0a},
	{0x2d, 0x40},
	{0x2e, 0x70},
	{0x30, 0x74},
	{0x31, 0x9b},
	{0x32, 0xa5},
	{0x33, 0xe0},
	{0x35, 0x05},
	{0x38, 0x40},
	{0x39, 0x68},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_1440p25[] = {
	{0x02, 0x50},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x50},
	{0x15, 0x23},
	{0x16, 0x1b},
	{0x17, 0x00},
	{0x18, 0x38},
	{0x19, 0xa0},
	{0x1a, 0x5a},
	{0x1c, 0x0f},   //2560*1440, 25fps
	{0x1d, 0x76},   //
	{0x20, 0x50},
	{0x21, 0x84},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x27, 0xad},
	{0x2b, 0x60},
	{0x2c, 0x0a},
	{0x2d, 0x58},
	{0x2e, 0x70},
	{0x30, 0x74},
	{0x31, 0x58},
	{0x32, 0x9f},
	{0x33, 0x60},
	{0x35, 0x15},
	{0x36, 0xdc},
	{0x38, 0x40},
	{0x39, 0x48},
};

const AD_TP9963_I2C_REG ad_tp9963_mode_1440p30[] = {
	{0x02, 0x50},
	{0x07, 0xc0},
	{0x0b, 0xc0},
	{0x0c, 0x03},
	{0x0d, 0x50},
	{0x15, 0x23},
	{0x16, 0x1b},
	{0x17, 0x00},
	{0x18, 0x38},
	{0x19, 0xa0},
	{0x1a, 0x5a},
	{0x1c, 0x0c},   //2560*1440, 30fps
	{0x1d, 0xe2},   //
	{0x20, 0x50},
	{0x21, 0x84},
	{0x22, 0x36},
	{0x23, 0x3c},
	{0x27, 0xad},
	{0x2b, 0x60},
	{0x2c, 0x0a},
	{0x2d, 0x58},
	{0x2e, 0x70},
	{0x30, 0x74},
	{0x31, 0x58},
	{0x32, 0x9f},
	{0x33, 0x60},
	{0x35, 0x15},
	{0x36, 0xdc},
	{0x38, 0x40},
	{0x39, 0x48},
};

static ER ad_tp9963_set_video_mode(UINT32 chip_id, UINT32 ch, UINT32 fmt, UINT32 std)
{
	const unsigned char REG42_43[] = {0x01, 0x02, 0x00, 0x00, 0x03};
	const unsigned char MASK42_43[] = {0xfe, 0xfd, 0xff, 0xff, 0xfc};
	AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG clk_ctrl_reg = {0};
	AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG clk_ctrl2_reg = {0};

	AD_IND("SET VIDEO MODE chip%u ch%u fmt%u(0x%x) std%u\r\n", chip_id, ch, fmt, fmt, std);

	ad_tp9963_set_reg_page(chip_id, ch);
	ad_tp9963_i2c_write(chip_id, 0x06, 0x12, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x50, 0x00, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x51, 0x00, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x54, 0x03, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, &clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_read(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, &clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);

	switch (fmt) {
	case TP2802_PAL:
		clk_ctrl_reg.reg |= REG42_43[ch];
		clk_ctrl2_reg.reg &= MASK42_43[ch];
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_pal, sizeof(ad_tp9963_mode_pal)/sizeof(typeof(ad_tp9963_mode_pal[0])));
		break;
	case TP2802_NTSC:
		clk_ctrl_reg.reg |= REG42_43[ch];
		clk_ctrl2_reg.reg &= MASK42_43[ch];
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_ntsc, sizeof(ad_tp9963_mode_ntsc)/sizeof(typeof(ad_tp9963_mode_ntsc[0])));
		break;
	case TP2802_720P25:
		if (std == STD_HDA) {
			clk_ctrl_reg.reg &= MASK42_43[ch];
			clk_ctrl2_reg.reg |= REG42_43[ch];
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_720p25_ahd, sizeof(ad_tp9963_mode_720p25_ahd)/sizeof(typeof(ad_tp9963_mode_720p25_ahd[0])));
		} else {
			clk_ctrl_reg.reg |= REG42_43[ch];
			clk_ctrl2_reg.reg &= MASK42_43[ch];
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_720p25_tvi, sizeof(ad_tp9963_mode_720p25_tvi)/sizeof(typeof(ad_tp9963_mode_720p25_tvi[0])));
		}
		break;
	case TP2802_720P30:
		if (std == STD_HDA) {
			clk_ctrl_reg.reg &= MASK42_43[ch];
			clk_ctrl2_reg.reg |= REG42_43[ch];
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_720p30_ahd, sizeof(ad_tp9963_mode_720p30_ahd)/sizeof(typeof(ad_tp9963_mode_720p30_ahd[0])));
		} else {
			clk_ctrl_reg.reg |= REG42_43[ch];
			clk_ctrl2_reg.reg &= MASK42_43[ch];
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_720p30_tvi, sizeof(ad_tp9963_mode_720p30_tvi)/sizeof(typeof(ad_tp9963_mode_720p30_tvi[0])));
		}
		break;
	case TP2802_1080P25:
		if (std == STD_HDA) {
			clk_ctrl_reg.reg |= REG42_43[ch];
			clk_ctrl2_reg.reg &= MASK42_43[ch];
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_1080p25_ahd, sizeof(ad_tp9963_mode_1080p25_ahd)/sizeof(typeof(ad_tp9963_mode_1080p25_ahd[0])));
		} else {
			clk_ctrl_reg.reg &= MASK42_43[ch];
			clk_ctrl2_reg.reg &= MASK42_43[ch];
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_1080p25_tvi, sizeof(ad_tp9963_mode_1080p25_tvi)/sizeof(typeof(ad_tp9963_mode_1080p25_tvi[0])));
		}
		break;
	case TP2802_1080P30:
		if (std == STD_HDA) {
			clk_ctrl_reg.reg |= REG42_43[ch];
			clk_ctrl2_reg.reg &= MASK42_43[ch];
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_1080p30_ahd, sizeof(ad_tp9963_mode_1080p30_ahd)/sizeof(typeof(ad_tp9963_mode_1080p30_ahd[0])));
		} else {
			clk_ctrl_reg.reg &= MASK42_43[ch];
			clk_ctrl2_reg.reg &= MASK42_43[ch];
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
			ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_1080p30_tvi, sizeof(ad_tp9963_mode_1080p30_tvi)/sizeof(typeof(ad_tp9963_mode_1080p30_tvi[0])));
		}
		break;
	case TP2802_1080P50:
		clk_ctrl_reg.reg &= MASK42_43[ch];
		clk_ctrl2_reg.reg &= MASK42_43[ch];
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_1080p50, sizeof(ad_tp9963_mode_1080p50)/sizeof(typeof(ad_tp9963_mode_1080p50[0])));
		break;
	case TP2802_1080P60:
		clk_ctrl_reg.reg &= MASK42_43[ch];
		clk_ctrl2_reg.reg &= MASK42_43[ch];
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_1080p60, sizeof(ad_tp9963_mode_1080p60)/sizeof(typeof(ad_tp9963_mode_1080p60[0])));
		break;
	case TP2802_QHD25:
		clk_ctrl_reg.reg &= MASK42_43[ch];
		clk_ctrl2_reg.reg &= MASK42_43[ch];
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_1440p25, sizeof(ad_tp9963_mode_1440p25)/sizeof(typeof(ad_tp9963_mode_1440p25[0])));
		break;
	case TP2802_QHD30:
		clk_ctrl_reg.reg &= MASK42_43[ch];
		clk_ctrl2_reg.reg &= MASK42_43[ch];
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL_REG_OFS, clk_ctrl_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write(chip_id, AD_TP9963_ADC_DECODER_CLOCK_CTRL2_REG_OFS, clk_ctrl2_reg.reg, AD_TP9963_I2C_DATA_LEN);
		ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mode_1440p30, sizeof(ad_tp9963_mode_1440p30)/sizeof(typeof(ad_tp9963_mode_1440p30[0])));
		break;
	default:
		AD_ERR("[%s] Unknown video fmt %d\r\n", AD_TP9963_MODULE_NAME, fmt);
		return E_SYS;
	}

	return E_OK;
}

const AD_TP9963_I2C_REG ad_tp9963_mipi_mode_2ch4lane_594m[] = {
	{0x21, 0x24},
	{0x14, 0x00},
	{0x15, 0x00},
	{0x2a, 0x08},
	{0x2b, 0x06},
	{0x2c, 0x11},
	{0x2e, 0x0a},
	{0x10, 0xa0},
	{0x10, 0x20},
};

const AD_TP9963_I2C_REG ad_tp9963_mipi_mode_2ch2lane_594m[] = {
	{0x21, 0x22},
	{0x14, 0x00},
	{0x15, 0x01},
	{0x2a, 0x08},
	{0x2b, 0x06},
	{0x2c, 0x11},
	{0x2e, 0x0a},
	{0x10, 0xa0},
	{0x10, 0x20},
};

static ER ad_tp9963_set_mipi_output(UINT32 chip_id, UINT32 mode)
{
	AD_IND("SET MIPI OUTPUT chip%u mode%u\r\n", chip_id, mode);

	ad_tp9963_set_reg_page(chip_id, MIPI_PAGE);

	ad_tp9963_i2c_write(chip_id, 0x02, 0x78, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x03, 0x70, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x04, 0x70, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x05, 0x70, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x06, 0x70, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x13, 0xef, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x20, 0x00, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x21, 0x22, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x22, 0x20, AD_TP9963_I2C_DATA_LEN);
	ad_tp9963_i2c_write(chip_id, 0x23, 0x9e, AD_TP9963_I2C_DATA_LEN);

	switch (mode) {
	case MIPI_2CH4LANE_594M:
		ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mipi_mode_2ch4lane_594m, sizeof(ad_tp9963_mipi_mode_2ch4lane_594m)/sizeof(typeof(ad_tp9963_mipi_mode_2ch4lane_594m[0])));
		break;
	case MIPI_2CH2LANE_594M:
		ad_tp9963_i2c_write_tbl(chip_id, ad_tp9963_mipi_mode_2ch2lane_594m, sizeof(ad_tp9963_mipi_mode_2ch2lane_594m)/sizeof(typeof(ad_tp9963_mipi_mode_2ch2lane_594m[0])));
		break;
	default:
		AD_ERR("[%s] Unknown mipi mode %d\r\n", AD_TP9963_MODULE_NAME, mode);
		return E_SYS;
	}

	return E_OK;
}

static UINT32 ad_tp9963_get_scan_mode(AD_DET_TYPE det_type)
{
	UINT32 i, cnt;

	// count total bits of det_type
	i = 0;
	cnt = 0;
	while ((1 << i) < AD_TYPE_MAX) {
		if (det_type & (1 << i)) {
			cnt++;
		}
		i++;
	}

	if (cnt > 1) {
		AD_ERR("[%s] Only support detect one type (0x%x)\r\n", AD_TP9963_MODULE_NAME, det_type);
		return -1;
	}

	switch (det_type) {
	case AD_TYPE_AUTO:
		return SCAN_AUTO;
	case AD_TYPE_AHD:
		return SCAN_HDA;
	case AD_TYPE_TVI:
		return SCAN_TVI;
	default:
		AD_ERR("[%s] Unsupport det_type 0x%x\r\n", AD_TP9963_MODULE_NAME, det_type);
		return SCAN_AUTO;
	}
}

static UINT32 ad_tp9963_get_scan_std(UINT32 scan_mode)
{
	UINT32 scan_std[] = {
		[SCAN_DISABLE] = AD_TP9963_DFT_STD,
		[SCAN_AUTO] = AD_TP9963_DFT_STD,
		[SCAN_TVI] = STD_TVI,
		[SCAN_HDA] = STD_HDA,
		[SCAN_HDC] = STD_HDC,
		[SCAN_MANUAL] = AD_TP9963_DFT_STD,
		[SCAN_TEST] = AD_TP9963_DFT_STD,
	};
	UINT32 scan_std_num = sizeof(scan_std) / sizeof(typeof(scan_std[0]));
	UINT32 ret = AD_TP9963_DFT_STD;

	if (scan_mode >= scan_std_num) {
		AD_ERR("[%s] Unsupport scan_mode %u\r\n", AD_TP9963_MODULE_NAME, scan_mode);
	} else {
		ret = scan_std[scan_mode];
	}

	return ret;
}

// implementation of video mode converting
static ER ad_tp9963_det_mode_to_vin_mode(AD_TP9963_DET_RAW_MODE raw_mode, AD_TP9963_DET_VIN_MODE *vin_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tp9963_det_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No det map info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tp9963_det_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&raw_mode, &g_ad_tp9963_det_map[i_mode].raw_mode, sizeof(AD_TP9963_DET_RAW_MODE)) == 0) {
			memcpy(vin_mode, &g_ad_tp9963_det_map[i_mode].vin_mode, sizeof(AD_TP9963_DET_VIN_MODE));
			return E_OK;
		}
	}

	AD_IND("[%s] Unknown raw mode %u, eq %u\r\n", AD_TP9963_MODULE_NAME, raw_mode.mode, raw_mode.eq);
	return E_SYS;
}

static ER ad_tp9963_vin_mode_to_ch_mode(AD_TP9963_VIN_MODE vin_mode, AD_TP9963_CH_MODE *ch_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tp9963_vin_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vin map info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tp9963_vin_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vin_mode, &g_ad_tp9963_vin_map[i_mode].vin_mode, sizeof(AD_TP9963_VIN_MODE)) == 0) {
			*ch_mode = g_ad_tp9963_vin_map[i_mode].ch_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vin mode %u %u %u %u\r\n", AD_TP9963_MODULE_NAME, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.prog);
	return E_SYS;
}

/*static ER ad_tp9963_vout_mode_to_port_mode(AD_TP9963_VOUT_MODE vout_mode, AD_TP9963_PORT_MODE *port_mode)
{
	UINT32 i_mode, mode_num, tbl_size = sizeof(typeof(g_ad_tp9963_vout_map[0]));

	if (unlikely(tbl_size == 0)) {
		AD_ERR("[%s] No vout map info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	mode_num = sizeof(g_ad_tp9963_vout_map)/tbl_size;

	for (i_mode = 0; i_mode < mode_num; i_mode++) {

		if (memcmp(&vout_mode, &g_ad_tp9963_vout_map[i_mode].vout_mode, sizeof(AD_TP9963_VOUT_MODE)) == 0) {
			*port_mode = g_ad_tp9963_vout_map[i_mode].port_mode;
			return E_OK;
		}
	}

	AD_ERR("[%s] Unknown vout mode\r\n", AD_TP9963_MODULE_NAME);
	return E_SYS;
}*/

// implementation of video mode detection

/*
	TP9963 can't recognize AHD video format, and can only detect if signal is plugged-in.
	So user needs to set AD_CFGID_DET_CAM_TYPE to tell driver do AHD special detection process.
	In this process, when AD return signal is plugged-in, driver will force change video mode
	from support list (sup_det_mode) one by one, and see if AD can lock the signal.
	When signal is locked, return vloss=0.
*/
static ER ad_tp9963_det_ahd(UINT32 chip_id, UINT32 vin_id, AD_TP9963_DET_VIDEO_INFO *video_info)
{
	char state_str[][16] = {
		"UNPLUG",
		"VIDEO_IN",
		"VDHD_LOCK",
		"UNLOCK",
		"EQ_LOCK",
	};
	AD_TP9963_DET_RAW_MODE sup_det_mode[] = {
		{.mode = TP2802_1080P25, 	.eq = 0},
		{.mode = TP2802_720P30, 	.eq = 0},
		{.mode = TP2802_720P25, 	.eq = 0},
		{.mode = TP2802_1080P30, 	.eq = 0},
	};
	AD_TP9963_INFO *ad_private_info;
	AD_TP9963_DET_RAW_MODE det_raw_mode = {0};
	AD_TP9963_DET_VIN_MODE det_vin_mode = {0};
	AD_TP9963_VIN_MODE vin_mode = {0};
	AD_TP9963_CH_MODE ch_mode = {0};
	AD_TP9963_WATCHDOG_INFO *wdi;
	UINT32 sts_str_num = sizeof(state_str) / sizeof(typeof(state_str[0]));
	BOOL vloss = TRUE, hlock = FALSE, vlock = FALSE;

	if (unlikely(chip_id >= AD_TP9963_CHIP_MAX)) {
		AD_ERR("[%s] ad det video info fail. chip_id (%u) > max (%u)\r\n", AD_TP9963_MODULE_NAME, chip_id, AD_TP9963_CHIP_MAX);
		return E_SYS;
	}

	if (unlikely(vin_id >= AD_TP9963_VIN_MAX)) {
		AD_ERR("[%s] ad det video info fail. vin_id (%u) > max (%u)\r\n", AD_TP9963_MODULE_NAME, vin_id, AD_TP9963_VIN_MAX);
		return E_SYS;
	}

	ad_private_info = ad_tp9963_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad det video info fail. NULL ad private info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	wdi = &ad_private_info->wdi[vin_id];

	ad_tp9963_set_reg_page(chip_id, vin_id);
	ad_tp9963_get_vloss(chip_id, &vloss);


/******************************************************************************/
/*                                                                            */
/*                    state machine for video checking                        */
/*                                                                            */
/******************************************************************************/
	if (vloss) { // no video

		if (VIDEO_UNPLUG != wdi->state) { // switch to no video
			AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_UNPLUG]);
			wdi->state = VIDEO_UNPLUG;
			wdi->count = 0;
			wdi->mode = INVALID_FORMAT;
		}

		if (0 == wdi->count) { // first time into no video
			// set to default mode
			vin_mode.width = AD_TP9963_DFT_WIDTH;
			vin_mode.height = AD_TP9963_DFT_HEIGHT;
			vin_mode.fps = AD_TP9963_DFT_FPS;
			vin_mode.prog = AD_TP9963_DFT_PROG;
			vin_mode.std = STD_HDA;
			if (ad_tp9963_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
				AD_WRN("[%s] ad det wrn. get ch mode fail (VLOSS)\r\n", AD_TP9963_MODULE_NAME);
			}
			ad_tp9963_set_video_mode(chip_id, vin_id, ch_mode.mode, ch_mode.std);

			ad_tp9963_reset_default(chip_id, vin_id);
			wdi->count++;

		} else if (wdi->count < MAX_COUNT) {
			wdi->count++;
		}

	} else { // there is video

		ad_tp9963_get_hlock(chip_id, &hlock);
		ad_tp9963_get_vlock(chip_id, &vlock);

		if (hlock && vlock) { // video locked

			if (VIDEO_LOCKED == wdi->state) { // previous state already locked
				if (wdi->count < MAX_COUNT) {
					wdi->count++;
				}

			} else if (VIDEO_UNPLUG == wdi->state) { // previous state is unplug
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_IN]);
				wdi->state = VIDEO_IN;
				wdi->count = 0;

			} else {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_LOCKED]);
				wdi->state = VIDEO_LOCKED;
				wdi->count = 0;

				// fine-tune detect speed. idx will increase by 1 every time when state == VIDEO_IN, even if state will be locked in next run.
				// so decrease idx by 1 at first time state == locked, to save the current locked idx.
				ad_private_info->ahd_det_idx[vin_id] = (ad_private_info->ahd_det_idx[vin_id] == 0) ? ((sizeof(sup_det_mode)/sizeof(typeof(sup_det_mode[0]))) - 1) : (ad_private_info->ahd_det_idx[vin_id] - 1);
			}

		} else { // video in but unlocked

			if (VIDEO_UNPLUG == wdi->state) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_IN]);
				wdi->state = VIDEO_IN;
				wdi->count = 0;

			} else if (VIDEO_LOCKED == wdi->state) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_UNLOCK]);
				wdi->state = VIDEO_UNLOCK;
				wdi->count = 0;

			} else if (AD_TP9963_VIDEO_EQ_LOCKED == wdi->state) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_UNLOCK]);
				wdi->state = VIDEO_UNLOCK;
				wdi->count = 0;

			} else {
				if (wdi->count < MAX_COUNT) wdi->count++;
				if (VIDEO_UNLOCK == wdi->state && wdi->count > 2) {
					AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_IN]);
					wdi->state = VIDEO_IN;
					wdi->count = 0;
					ad_tp9963_reset_default(chip_id, vin_id);
				}
			}
		}
	}

	AD_IND("[chip%u ch%u STS = %s (count %u)]\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], wdi->count);

/******************************************************************************/
/*                                                                            */
/*               video in but vd/hd unlock, check video mode                  */
/*                                                                            */
/******************************************************************************/
	if (VIDEO_IN == wdi->state) { // current state is video in

		det_raw_mode = sup_det_mode[ad_private_info->ahd_det_idx[vin_id]];

		AD_IND("[chip%u ch%u] test idx %u (mode %u)\r\n", chip_id, vin_id, ad_private_info->ahd_det_idx[vin_id], det_raw_mode.mode);

		ad_private_info->ahd_det_idx[vin_id] = (ad_private_info->ahd_det_idx[vin_id] + 1) % (sizeof(sup_det_mode)/sizeof(typeof(sup_det_mode[0])));

		ad_private_info->raw_mode[vin_id] = det_raw_mode;

		if (ad_tp9963_det_mode_to_vin_mode(det_raw_mode, &det_vin_mode) != E_OK) {
			goto do_vloss;
		}

		ad_private_info->det_vin_mode[vin_id] = det_vin_mode;

		vin_mode.width = det_vin_mode.width;
		vin_mode.height = det_vin_mode.height;
		vin_mode.fps = det_vin_mode.fps;
		vin_mode.prog = det_vin_mode.prog;
		vin_mode.std = det_vin_mode.std;
		if (ad_tp9963_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
			AD_WRN("[%s] ad det wrn. get ch mode fail (VIDEO IN)\r\n", AD_TP9963_MODULE_NAME);
		}
		ad_tp9963_set_video_mode(chip_id, vin_id, ch_mode.mode, ch_mode.std);

		wdi->std = ch_mode.std;
		wdi->mode = ch_mode.mode;
	}

/******************************************************************************/
/*                                                                            */
/*                        vd/hd lock, video present                           */
/*                                                                            */
/******************************************************************************/
	if (VIDEO_LOCKED == wdi->state) {
		goto do_video_present;
	}

do_vloss:
	video_info->vloss = TRUE;
	video_info->width = AD_TP9963_DFT_WIDTH;
	video_info->height = AD_TP9963_DFT_HEIGHT;
	video_info->fps = AD_TP9963_DFT_FPS;
	video_info->prog = AD_TP9963_DFT_PROG;
	video_info->std = AD_TP9963_DFT_STD;
	return E_OK;

do_video_present:
	video_info->vloss = FALSE;
	video_info->width = ad_private_info->det_vin_mode[vin_id].width;
	video_info->height = ad_private_info->det_vin_mode[vin_id].height;
	video_info->fps = ad_private_info->det_vin_mode[vin_id].fps;
	video_info->prog = ad_private_info->det_vin_mode[vin_id].prog;
	video_info->std = ad_private_info->det_vin_mode[vin_id].std;
	return E_OK;
}

static ER ad_tp9963_det_video_info(UINT32 chip_id, UINT32 vin_id, AD_TP9963_DET_VIDEO_INFO *video_info)
{
	char state_str[][16] = {
		"UNPLUG",
		"VIDEO_IN",
		"VDHD_LOCK",
		"UNLOCK",
		"EQ_LOCK",
	};
	AD_TP9963_INFO *ad_private_info;
	AD_TP9963_DET_RAW_MODE det_raw_mode = {0};
	AD_TP9963_DET_VIN_MODE det_vin_mode = {0};
	AD_TP9963_VIN_MODE vin_mode = {0};
	AD_TP9963_CH_MODE ch_mode = {0};
	AD_TP9963_WATCHDOG_INFO *wdi;
	UINT32 i, eq = 1, cvstd = INVALID_FORMAT, sywd = 0, tmp = 0, gain = 0;
	UINT32 sts_str_num = sizeof(state_str) / sizeof(typeof(state_str[0]));
	BOOL vloss = TRUE, hlock = FALSE, vlock = FALSE, carrier = FALSE;

	if (unlikely(chip_id >= AD_TP9963_CHIP_MAX)) {
		AD_ERR("[%s] ad det video info fail. chip_id (%u) > max (%u)\r\n", AD_TP9963_MODULE_NAME, chip_id, AD_TP9963_CHIP_MAX);
		return E_SYS;
	}

	if (unlikely(vin_id >= AD_TP9963_VIN_MAX)) {
		AD_ERR("[%s] ad det video info fail. vin_id (%u) > max (%u)\r\n", AD_TP9963_MODULE_NAME, vin_id, AD_TP9963_VIN_MAX);
		return E_SYS;
	}

	ad_private_info = ad_tp9963_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad det video info fail. NULL ad private info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	wdi = &ad_private_info->wdi[vin_id];

	ad_tp9963_set_reg_page(chip_id, vin_id);
	ad_tp9963_get_vloss(chip_id, &vloss);


/******************************************************************************/
/*                                                                            */
/*                    state machine for video checking                        */
/*                                                                            */
/******************************************************************************/
	if (vloss) { // no video

		if (VIDEO_UNPLUG != wdi->state) { // switch to no video
			AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_UNPLUG]);
			wdi->state = VIDEO_UNPLUG;
			wdi->count = 0;
			if (SCAN_MANUAL != wdi->scan) {
				wdi->mode = INVALID_FORMAT;
			}
		}

		if (0 == wdi->count) { // first time into no video
			// set to default mode
			vin_mode.width = AD_TP9963_DFT_WIDTH;
			vin_mode.height = AD_TP9963_DFT_HEIGHT;
			vin_mode.fps = AD_TP9963_DFT_FPS;
			vin_mode.prog = AD_TP9963_DFT_PROG;
			vin_mode.std = ad_tp9963_get_scan_std(wdi->scan);
			if (ad_tp9963_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
				AD_WRN("[%s] ad det wrn. get ch mode fail (VLOSS)\r\n", AD_TP9963_MODULE_NAME);
			}
			ad_tp9963_set_video_mode(chip_id, vin_id, ch_mode.mode, ch_mode.std);

			ad_tp9963_reset_default(chip_id, vin_id);
			wdi->count++;

		} else if (wdi->count < MAX_COUNT) {
			wdi->count++;
		}

	} else { // there is video

		ad_tp9963_get_hlock(chip_id, &hlock);
		ad_tp9963_get_vlock(chip_id, &vlock);

		if (hlock && vlock) { // video locked

			if (VIDEO_LOCKED == wdi->state) { // previous state already locked
				if (wdi->eq_locked) { // first carrier lock
					AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[AD_TP9963_VIDEO_EQ_LOCKED]);
					wdi->state = AD_TP9963_VIDEO_EQ_LOCKED;
					wdi->count = 0;
				} else if (wdi->count < MAX_COUNT) {
					wdi->count++;
				}

			} else if (AD_TP9963_VIDEO_EQ_LOCKED == wdi->state) {
				if (wdi->count < MAX_COUNT) {
					wdi->count++;
				}

			} else if (VIDEO_UNPLUG == wdi->state) { // previous state is unplug
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_IN]);
				wdi->state = VIDEO_IN;
				wdi->count = 0;

			} else if (INVALID_FORMAT != wdi->mode) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_LOCKED]);
				wdi->state = VIDEO_LOCKED;
				wdi->eq_locked = 0;
				wdi->count = 0;
			}

		} else { // video in but unlocked

			if (VIDEO_UNPLUG == wdi->state) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_IN]);
				wdi->state = VIDEO_IN;
				wdi->count = 0;

			} else if (VIDEO_LOCKED == wdi->state) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_UNLOCK]);
				wdi->state = VIDEO_UNLOCK;
				wdi->count = 0;

			} else if (AD_TP9963_VIDEO_EQ_LOCKED == wdi->state) {
				AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_UNLOCK]);
				wdi->state = VIDEO_UNLOCK;
				wdi->count = 0;

			} else {
				if (wdi->count < MAX_COUNT) wdi->count++;
				if (VIDEO_UNLOCK == wdi->state && wdi->count > 2) {
					AD_IND("[chip%u ch%u] %s -> %s\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], state_str[VIDEO_IN]);
					wdi->state = VIDEO_IN;
					wdi->count = 0;
					if (SCAN_MANUAL != wdi->scan) ad_tp9963_reset_default(chip_id, vin_id);
				}
			}
		}

		if (wdi->force) { // manual reset for V1/2 switching
			wdi->state = VIDEO_UNPLUG;
			wdi->count = 0;
			wdi->mode = INVALID_FORMAT;
			wdi->force = 0;
			ad_tp9963_reset_default(chip_id, vin_id);
			AD_IND("[chip%u ch%u] FORCE RESET\r\n", chip_id, vin_id);
		}
	}

	AD_IND("[chip%u ch%u STS = %s (count %u)]\r\n", chip_id, vin_id, (wdi->state >= sts_str_num) ? "UNKNOWN" : state_str[wdi->state], wdi->count);

/******************************************************************************/
/*                                                                            */
/*               video in but vd/hd unlock, check video mode                  */
/*                                                                            */
/******************************************************************************/
	if (VIDEO_IN == wdi->state) { // current state is video in

		if (SCAN_MANUAL != wdi->scan) {
			ad_tp9963_get_cvstd(chip_id, &cvstd);
			ad_tp9963_get_sywd(chip_id, &sywd);
			ad_tp9963_get_eq_detect(chip_id, &eq);

			AD_IND("[VIDEO IN DETECTION] cvstd %x, sywd %x, eq %x\r\n", cvstd, sywd, eq);

			det_raw_mode.mode = cvstd;
			if (cvstd == TP2802_SD) { // using eq to determine NTSC or PAL
				if ((wdi->count/2) % 2) { // TP2802_PAL
					det_raw_mode.eq = 0;
				} else { // TP2802_NTSC
					det_raw_mode.eq = 1;
				}
			} else {
				det_raw_mode.eq = eq;
			}

			ad_private_info->raw_mode[vin_id] = det_raw_mode;

			if (ad_tp9963_det_mode_to_vin_mode(det_raw_mode, &det_vin_mode) != E_OK) {
				goto do_vloss;
			}

			if (SCAN_AUTO != wdi->scan) {
				det_vin_mode.std = ad_tp9963_get_scan_std(wdi->scan);
			}

			ad_private_info->det_vin_mode[vin_id] = det_vin_mode;

			vin_mode.width = det_vin_mode.width;
			vin_mode.height = det_vin_mode.height;
			vin_mode.fps = det_vin_mode.fps;
			vin_mode.prog = det_vin_mode.prog;
			vin_mode.std = det_vin_mode.std;
			if (ad_tp9963_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
				AD_WRN("[%s] ad det wrn. get ch mode fail (VIDEO IN)\r\n", AD_TP9963_MODULE_NAME);
			}
			ad_tp9963_set_video_mode(chip_id, vin_id, ch_mode.mode, ch_mode.std);

			wdi->std = ch_mode.std;
			wdi->mode = ch_mode.mode;
		}
	}

/******************************************************************************/
/*                                                                            */
/*                    vd/hd lock but eq unlock, set egain                     */
/*                                                                            */
/******************************************************************************/
	if (VIDEO_LOCKED == wdi->state) { // check eq stable

		if (0 == wdi->count) {
			ad_tp9963_set_clamp_ctrl(chip_id, 1);

			if (SCAN_AUTO == wdi->scan || SCAN_TVI == wdi->scan) {
#if 0
				if (TP2802_720P30V2 == wdi->mode || TP2802_720P25V2 == wdi->mode) {
					ad_tp9963_get_sywd(chip_id, &sywd);
					AD_IND("CVSTD%02x  ch%02x chip%2x\r\n", sywd, vin_id, chip_id);

					if (sywd == 0) {
						AD_IND("720P V1 Detected ch%02x chip%2x\r\n", vin_id, chip_id);
						wdi->mode &= 0xf7;
						ad_tp9963_set_video_mode(chip_id, vin_id, wdi->mode, STD_TVI); // to speed the switching
					}

				} else if (TP2802_720P30 == wdi->mode || TP2802_720P25 == wdi->mode) {
					ad_tp9963_get_sywd(chip_id, &sywd);
					AD_IND("CVSTD%02x  ch%02x chip%2x\r\n", sywd, vin_id, chip_id);

					if (sywd != 0) {
						AD_IND("720P V2 Detected ch%02x chip%2x\r\n", vin_id, chip_id);
						wdi->mode |= 0x08;
						ad_tp9963_set_video_mode(chip_id, vin_id, wdi->mode, STD_TVI); // to speed the switching
					}
				}
#endif

				// these code need to keep bottom
				// disable rx and reset v/h pos
				ad_tp9963_i2c_read(chip_id, AD_TP9963_RX_CTRL_REG_OFS, &tmp, AD_TP9963_I2C_DATA_LEN);
				tmp &= 0xFE;
				ad_tp9963_i2c_write(chip_id, AD_TP9963_RX_CTRL_REG_OFS, tmp, AD_TP9963_I2C_DATA_LEN);
				ad_tp9963_i2c_write(chip_id, AD_TP9963_READ_V_POS_CTRL_REG_OFS, 0x06, AD_TP9963_I2C_DATA_LEN);
				ad_tp9963_i2c_write(chip_id, AD_TP9963_READ_H_POS_CTRL_REG_OFS, 0x60, AD_TP9963_I2C_DATA_LEN);
			}

			vos_util_delay_ms(100);

			// enable rx
			ad_tp9963_i2c_read(chip_id, AD_TP9963_RX_CTRL_REG_OFS, &tmp, AD_TP9963_I2C_DATA_LEN);
			tmp |= 0x01;
			ad_tp9963_i2c_write(chip_id, AD_TP9963_RX_CTRL_REG_OFS, tmp, AD_TP9963_I2C_DATA_LEN);

			vos_util_delay_ms(300); // pre-delay for eq stable
		}

		for (i = 0; i < AD_TP9963_WATCHDOG_GAIN_NUM; i++) {
			ad_tp9963_read_egain(chip_id, &gain);
			wdi->gain[i] = gain;
			vos_util_delay_ms(30);
		}
		AD_IND("[EQ GAIN] %u %u %u %u\r\n", wdi->gain[0], wdi->gain[1], wdi->gain[2], wdi->gain[3]);

		wdi->eq_locked = 1;
		for (i = 0; i < AD_TP9963_WATCHDOG_GAIN_NUM-1; i++) {
			if ((max(wdi->gain[i], wdi->gain[AD_TP9963_WATCHDOG_GAIN_NUM-1]) - min(wdi->gain[i], wdi->gain[AD_TP9963_WATCHDOG_GAIN_NUM-1])) >= 0x02) {
				wdi->eq_locked = 0;
				break;
			}
		}

		if (wdi->eq_locked) {
			if (STD_TVI != wdi->std) {
				ad_tp9963_i2c_write(chip_id, AD_TP9963_EQ2_CTRL_REG_OFS, 0x80|(wdi->gain[AD_TP9963_WATCHDOG_GAIN_NUM-1]>>2), AD_TP9963_I2C_DATA_LEN); // manual mode

			} else { // TVI
			}
			vos_util_delay_ms(100);

			if (SCAN_AUTO == wdi->scan) {
				// TBD
			}

			if (STD_TVI != wdi->std) {
				ad_tp9963_egain(chip_id, 0x0c);

			} else { // TVI
			}
		}
	}

/******************************************************************************/
/*                                                                            */
/*                          eq lock, check carrier                            */
/*                                                                            */
/******************************************************************************/
	if (AD_TP9963_VIDEO_EQ_LOCKED == wdi->state) { // check carrier lock
		ad_tp9963_get_carrier(chip_id, &carrier);

		if (SCAN_AUTO == wdi->scan) {

			if ((wdi->mode & (~FLAG_HALF_MODE)) < TP2802_3M18) {

				ad_tp9963_get_cvstd(chip_id, &cvstd);
				if (cvstd != (wdi->mode&0x07) && cvstd < TP2802_SD) {
					AD_IND("Current mode %02x not match with detected cvstd %02x. Force reset\r\n", wdi->mode, cvstd);
					wdi->force = 1;
				} else if (!carrier) {
					AD_IND("Egain stable but no carrier. Force reset\r\n");
					wdi->force = 1;
				} else {
					goto do_video_present;
				}
			}

		} else { // non-auto scan
			if (carrier) { // if no carrier caused by user wrong mode, vloss forever
				goto do_video_present;
			} else {
				ad_tp9963_get_eq_detect(chip_id, &eq);
				AD_IND("[ch%u] Egain stable but no carrier. det_type %x, eq %x\r\n", vin_id, ad_private_info->det_type[vin_id], eq);
			}
		}
	}

do_vloss:
	video_info->vloss = TRUE;
	video_info->width = AD_TP9963_DFT_WIDTH;
	video_info->height = AD_TP9963_DFT_HEIGHT;
	video_info->fps = AD_TP9963_DFT_FPS;
	video_info->prog = AD_TP9963_DFT_PROG;
	video_info->std = AD_TP9963_DFT_STD;
	return E_OK;

do_video_present:
	video_info->vloss = FALSE;
	video_info->width = ad_private_info->det_vin_mode[vin_id].width;
	video_info->height = ad_private_info->det_vin_mode[vin_id].height;
	video_info->fps = ad_private_info->det_vin_mode[vin_id].fps;
	video_info->prog = ad_private_info->det_vin_mode[vin_id].prog;
	video_info->std = ad_private_info->det_vin_mode[vin_id].std;
	return E_OK;
}

static ER ad_tp9963_dbg_dump_help(UINT32 chip_id)
{
	AD_DUMP("\r\n=================================== AD %s DUMP HELP BEGIN (CHIP %u) ===================================\r\n", AD_TP9963_MODULE_NAME, chip_id);

	AD_DUMP("CMD: dumpinfo\r\n");
	AD_DUMP("DESC: Show decoder information.\r\n");
	AD_DUMP("\r\n");

	AD_DUMP("CMD: det\r\n");
	AD_DUMP("DESC: Trigger detection process once.\r\n");
	AD_DUMP("\r\n");

	AD_DUMP("CMD: get\r\n");
	AD_DUMP("DESC: Get configuration.\r\n");
	AD_DUMP("\r\n");

	AD_DUMP("CMD: set\r\n");
	AD_DUMP("DESC: Set configuration.\r\n");
	AD_DUMP("\r\n");

	AD_DUMP("\r\n=================================== AD %s DUMP HELP END ===================================\r\n", AD_TP9963_MODULE_NAME);

	return E_OK;
}

static ER ad_tp9963_dbg_dump_info(UINT32 chip_id)
{
#define GET_STR(strary, idx) ((UINT32)(idx) < sizeof(strary)/sizeof(typeof(strary[0])) ? strary[idx] : "Unknown")
	char std_str[][4] = {
		"TVI",
		"AHD",
	};
	char det_type_str[][4] = {
		"AHD",
		"TVI",
		"CVI",
		"SDI",
	};
	AD_TP9963_INFO *ad_private_info;
	UINT32 i, i_vin;

	ad_private_info = ad_tp9963_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad dump info fail. NULL ad private info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	AD_DUMP("\r\n=================================== AD %s DUMP INFO BEGIN (CHIP %u) ===================================\r\n", AD_TP9963_MODULE_NAME, chip_id);

	AD_DUMP("%3s | %8s %4s %8s   %s\r\n",
		"vin", "raw_mode", "eq", "det_std", "det_type");
	AD_DUMP("==================================================================================================\r\n");
	for (i_vin = 0; i_vin < AD_TP9963_VIN_MAX; i_vin++) {

		AD_DUMP("%3u | %8x %4u %8s   ",
			i_vin, ad_private_info->raw_mode[i_vin].mode, ad_private_info->raw_mode[i_vin].eq, GET_STR(std_str, ad_private_info->det_vin_mode[i_vin].std));

		if (ad_private_info->det_type[i_vin] == 0) {
			AD_DUMP("AUTO");
		} else {
			for (i = 0; i < sizeof(det_type_str) / sizeof(typeof(det_type_str[0])); i++) {
				if (ad_private_info->det_type[i_vin] & (1 << i)) {
					AD_DUMP("%s ", det_type_str[i]);
				}
			}
		}
		AD_DUMP("\r\n");
	}

	AD_DUMP("\r\n=================================== AD %s DUMP INFO END ===================================\r\n", AD_TP9963_MODULE_NAME);

	AD_DUMP("\r\n");

	return E_OK;
}

#if 0
#endif

static ER ad_tp9963_open(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_INIT_INFO *init_info;
	AD_DRV_OPEN_INFO drv_open_info = {0};
	AD_DRV_DBG_BANK_REG_DUMP_INFO reg_info = {0};

	ad_info = ad_tp9963_get_info(chip_id);
	init_info = (AD_INIT_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	ad_tp9963_set2def(ad_info);

	ad_info->i2c_addr = init_info->i2c_addr;

	drv_open_info.module_name = AD_TP9963_MODULE_NAME;
	drv_open_info.version = AD_DRV_MODULE_VERSION_VAR;
	drv_open_info.comm_info = ad_info;
	drv_open_info.i2c_addr_len = AD_TP9963_I2C_ADDR_LEN;
	drv_open_info.i2c_data_len = AD_TP9963_I2C_DATA_LEN;
	drv_open_info.i2c_data_msb = AD_TP9963_I2C_DATA_MSB;
	drv_open_info.i2c_retry_num = AD_TP9963_I2C_RETRY_NUM;
	drv_open_info.pwr_ctl = init_info->pwr_info;
	drv_open_info.dbg_func = AD_DRV_DBG_FUNC_COMMON | AD_DRV_DBG_FUNC_DUMP_BANK_REG;
	reg_info.bank_addr = AD_TP9963_BANK_ADDR;
	reg_info.bank_num = sizeof(g_ad_tp9963_bank) / sizeof(typeof(g_ad_tp9963_bank[0]));
	reg_info.bank = g_ad_tp9963_bank;
	reg_info.dump_per_row = AD_TP9963_REG_DUMP_PER_ROW;
	drv_open_info.dbg_bank_reg_dump_info = &reg_info;
	ad_drv_open(chip_id, &drv_open_info);

	g_ad_tp9963_info[chip_id].status |= AD_STS_OPEN;

	return E_OK;
}

static ER ad_tp9963_close(UINT32 chip_id, void *ext_data)
{
	ad_drv_close(chip_id);
	g_ad_tp9963_info[chip_id].status &= ~AD_STS_OPEN;
	return E_OK;
}

static ER ad_tp9963_init(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_TP9963_INFO *ad_private_info;
	AD_TP9963_VIN_MODE vin_mode = {0};
	AD_TP9963_CH_MODE ch_mode = {0};
	UINT32 device_id = 0, revision_id = 0, i_vout;

	ad_info = ad_tp9963_get_info(chip_id);
	ad_private_info = ad_tp9963_get_private_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad init fail. NULL ad private info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	// check if this chip had been inited
	if (ad_private_info->is_inited) {
		goto done;
	}

	ad_tp9963_set_reg_page(chip_id, CH_ALL);

	ad_tp9963_get_device_id(chip_id, &device_id);
	ad_tp9963_get_revision_id(chip_id, &revision_id);

	AD_DUMP("Detected device id 0x%x, revision id 0x%x\r\n", device_id, revision_id);
	if (device_id != 0x2863 && revision_id != 0x00) {
		AD_WRN("Identification mismatch\r\n");
	}

	// set video mode
	vin_mode.width = AD_TP9963_DFT_WIDTH;
	vin_mode.height = AD_TP9963_DFT_HEIGHT;
	vin_mode.fps = AD_TP9963_DFT_FPS;
	vin_mode.prog = AD_TP9963_DFT_PROG;
	vin_mode.std = AD_TP9963_DFT_STD;
	if (ad_tp9963_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK) {
		AD_WRN("[%s] ad init wrn. get ch mode fail\r\n", AD_TP9963_MODULE_NAME);
	}

	ad_tp9963_set_video_mode(chip_id, CH_ALL, ch_mode.mode, ch_mode.std);
	ad_tp9963_set_pattern_gen_mode(chip_id, CH_ALL, AD_TP9963_PATTERN_GEN_MODE_AUTO);

	// set mipi mode
	ad_tp9963_set_mipi_clk_lane_stop(chip_id, 0, TRUE);
	ad_tp9963_set_mipi_output(chip_id, MIPI_2CH4LANE_594M);
	ad_tp9963_set_mipi_clk_lane_stop(chip_id, 0, FALSE);
	ad_tp9963_set_csi_out_dly(chip_id);

	for (i_vout = 0; i_vout < AD_TP9963_VOUT_MAX; i_vout++) {
		ad_private_info->mipi_mode[i_vout] = AD_TP9963_MIPI_MODE_UNKNOWN;
	}
	ad_private_info->is_inited = TRUE;

done:
	g_ad_tp9963_info[chip_id].status |= AD_STS_INIT;

	return E_OK;
}

static ER ad_tp9963_uninit(UINT32 chip_id, void *ext_data)
{
	AD_TP9963_INFO *ad_private_info;

	ad_private_info = ad_tp9963_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad uninit fail. NULL ad private info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	ad_private_info->is_inited = FALSE;
	g_ad_tp9963_info[chip_id].status &= ~AD_STS_INIT;

	return E_OK;
}

static ER ad_tp9963_get_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_VIRTUAL_CHANNEL_INFO *virtual_ch;
	AD_VOUT_DATA_FMT_INFO *data_fmt;
	AD_MODE_INFO *mode_info;
	ER rt = E_OK;

	ad_info = ad_tp9963_get_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad get info fail. NULL ad info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_DATA_FORMAT:
			data_fmt = (AD_VOUT_DATA_FMT_INFO *)data;

			data_fmt->fmt = AD_DATA_FMT_422;
			data_fmt->protocal = AD_DATA_PROTCL_CCIR601;
			AD_IND("[ad GET outport fmt = %d protocol= %d\r\n", data_fmt->fmt, data_fmt->protocal);
			break;
		case AD_CFGID_VIRTUAL_CH_ID:
			virtual_ch = (AD_VIRTUAL_CHANNEL_INFO *)data;

			if (unlikely(virtual_ch->vin_id >= ad_info->vin_max)) {
				AD_ERR("[%s] virtual_ch wrong input\r\n", AD_TP9963_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			virtual_ch->id = virtual_ch->vin_id;
			break;
		case AD_CFGID_MODE_INFO:
			mode_info = (AD_MODE_INFO *)data;

			mode_info->bus_type = AD_BUS_TYPE_SERIAL;
			mode_info->in_clock = 27000000;
			mode_info->out_clock = 74250000;
			mode_info->data_rate = 297000000;
			mode_info->mipi_lane_num = ad_info->vout[mode_info->out_id].data_lane_num;
			mode_info->protocal = AD_DATA_PROTCL_CCIR601;
			mode_info->yuv_order = AD_DATA_ORDER_UYVY;
			mode_info->crop_start.ofst_x = 0;
			mode_info->crop_start.ofst_y = 0;
			break;
		case AD_CFGID_MIPI_MANUAL_ID:
			// not necessory
			// do nothing to avoid the error msg from default case
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_TP9963_MODULE_NAME, item);
			break;
	};

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad get info (%d) fail (%d)\r\n", AD_TP9963_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_tp9963_set_cfg(UINT32 chip_id, AD_CFGID item, void *data)
{
	AD_INFO *ad_info;
	AD_TP9963_INFO *ad_private_info;
	AD_INFO_CFG *cfg_info;
	ER rt = E_OK;

	ad_info = ad_tp9963_get_info(chip_id);
	ad_private_info = ad_tp9963_get_private_info(chip_id);

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad private info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	switch (item) {
		case AD_CFGID_I2C_FUNC:
			rt |= ad_drv_i2c_set_i2c_rw_if((ad_i2c_rw_if)data);
			break;
		case AD_CFGID_DET_CAM_TYPE:
			cfg_info = (AD_INFO_CFG *)data;

			if (unlikely(cfg_info->vin_id >= ad_info->vin_max)) {
				AD_ERR("[%s] cfg_info wrong input\r\n", AD_TP9963_MODULE_NAME);
				rt = E_PAR;
				break;
			}

			ad_private_info->det_type[cfg_info->vin_id] = cfg_info->data;
			// configure detect info when det_type is set
			ad_private_info->wdi[cfg_info->vin_id].scan = ad_tp9963_get_scan_mode(cfg_info->data);
			if (ad_private_info->wdi[cfg_info->vin_id].scan == SCAN_HDA) {
				AD_DUMP("[%s] Notice: vin_id %u can't detect video format when scan mode is AHD. take care video format in chgmode\r\n", AD_TP9963_MODULE_NAME, cfg_info->vin_id);
				ad_private_info->wdi[cfg_info->vin_id].state = VIDEO_LOCKED; // force state to locked, to prevent detection switch video mode after chgmode correctly
			} else {
				ad_private_info->wdi[cfg_info->vin_id].state = VIDEO_UNPLUG;
			}
			ad_private_info->wdi[cfg_info->vin_id].count = 0;
			ad_private_info->wdi[cfg_info->vin_id].mode = INVALID_FORMAT;
			ad_private_info->wdi[cfg_info->vin_id].force = 0;
			// pre-configure det_vin_mode.std for user call chgmode without detection
			ad_private_info->det_vin_mode[cfg_info->vin_id].std = ad_tp9963_get_scan_std(ad_private_info->wdi[cfg_info->vin_id].scan);
			break;
		default:
			AD_ERR("[%s] not support AD_CFGID item(%d) \n", AD_TP9963_MODULE_NAME, item);
			break;
	}

	if (unlikely(rt != E_OK)) {
		AD_ERR("[%s] ad set info (%d) fail (%d)\r\n", AD_TP9963_MODULE_NAME, item, rt);
	}

	return rt;
}

static ER ad_tp9963_chgmode(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_TP9963_INFO *ad_private_info;
	AD_CHGMODE_INFO *chgmode_info;
	UINT32 vin_id, vout_id;
	AD_TP9963_VIN_MODE vin_mode = {0};
	AD_TP9963_CH_MODE ch_mode = {0};

	ad_info = ad_tp9963_get_info(chip_id);
	ad_private_info = ad_tp9963_get_private_info(chip_id);
	chgmode_info = (AD_CHGMODE_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL ad info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad set info fail. NULL ad private info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	if (unlikely(chgmode_info == NULL)) {
		AD_ERR("[%s] ad chgmode fail. NULL chgmode_info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	vin_id = chgmode_info->vin_id;
	vout_id = chgmode_info->vout_id;

	if (unlikely(vin_id >= AD_TP9963_VIN_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vin_id(%u) >= max(%u)\r\n", AD_TP9963_MODULE_NAME, vin_id, AD_TP9963_VIN_MAX);
		return E_SYS;
	}

	if (unlikely(vout_id >= AD_TP9963_VOUT_MAX)) {
		AD_ERR("[%s] ad chgmode fail. vout_id(%u) >= max(%u)\r\n", AD_TP9963_MODULE_NAME, vout_id, AD_TP9963_VOUT_MAX);
		return E_SYS;
	}

	vin_mode.width = chgmode_info->ui_info.width;
	vin_mode.height = chgmode_info->ui_info.height << (chgmode_info->ui_info.prog ? 0 : 1);
	vin_mode.fps = chgmode_info->ui_info.fps;
	vin_mode.prog = chgmode_info->ui_info.prog;
	vin_mode.std = ad_private_info->det_vin_mode[vin_id].std;
	AD_IND("chip%d vin%d: w = %d, h = %d, fps = %d, std = %d!!\r\n", chip_id, vin_id, vin_mode.width, vin_mode.height, vin_mode.fps, vin_mode.std);

	if (unlikely(ad_tp9963_vin_mode_to_ch_mode(vin_mode, &ch_mode) != E_OK)) {
		AD_ERR("[%s] ad chgmode fail. get ch mode fail\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	ad_tp9963_set_reg_page(chip_id, vin_id);
	ad_tp9963_set_video_mode(chip_id, vin_id, ch_mode.mode, ch_mode.std);

	// set mipi mode
	if (ad_private_info->mipi_mode[vout_id] == AD_TP9963_MIPI_MODE_UNKNOWN) {
		// mipi mode affect every channel, so can only be set at first time chgmode after init
		ad_tp9963_set_mipi_clk_lane_stop(chip_id, 0, TRUE);
		switch (ad_info->vout[vout_id].data_lane_num) {
		case AD_MIPI_LANE_NUM_2:
			ad_tp9963_set_mipi_output(chip_id, MIPI_2CH2LANE_594M);
			break;
		case AD_MIPI_LANE_NUM_4:
			ad_tp9963_set_mipi_output(chip_id, MIPI_2CH4LANE_594M);
			break;
		default:
			AD_ERR("[%s port%u] ad chgmode fail. unsupport data lane num %u\r\n", AD_TP9963_MODULE_NAME, vout_id, ad_info->vout[vout_id].data_lane_num);
			return E_SYS;
		}
		ad_tp9963_set_mipi_clk_lane_stop(chip_id, 0, FALSE);
		ad_tp9963_set_csi_out_dly(chip_id);
		ad_private_info->mipi_mode[vout_id] = ad_info->vout[vout_id].data_lane_num;

	} else if (ad_info->vout[vout_id].data_lane_num != ad_private_info->mipi_mode[vout_id]) {
		// if it's not the first time chgmode, check mipi mode info is the same with previous chgmode
		AD_ERR("[%s port%u] ad chgmode fail. data lane (%u) must be the same with first time chgmode (%u)\r\n", AD_TP9963_MODULE_NAME, vout_id, ad_info->vout[vout_id].data_lane_num, ad_private_info->mipi_mode[vout_id]);
		return E_SYS;
	} else {
		AD_IND("[%s ch%u port%u] skip set mipi mode\r\n", AD_TP9963_MODULE_NAME, vin_id, vout_id);
	}

	chgmode_info->cur_info.width = vin_mode.width;
	chgmode_info->cur_info.height = vin_mode.height;
	chgmode_info->cur_info.fps = vin_mode.fps;
	chgmode_info->cur_info.prog = vin_mode.prog;
	chgmode_info->cur_update = TRUE;
	ad_private_info->det_vin_mode[vin_id].std = vin_mode.std;

	return E_OK;
}

static ER ad_tp9963_watchdog_cb(UINT32 chip_id, void *ext_data)
{
	AD_INFO *ad_info;
	AD_TP9963_INFO *ad_private_info;
	AD_TP9963_DET_VIDEO_INFO video_info = {0};
	AD_DECET_INFO *detect_info;
	UINT32 i_vin;

	ad_info = ad_tp9963_get_info(chip_id);
	detect_info = (AD_DECET_INFO *)ext_data;

	if (unlikely(ad_info == NULL)) {
		AD_ERR("[%s] ad watchdog cb fail. NULL ad info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	if ((g_ad_tp9963_info[chip_id].status & AD_STS_INIT) != AD_STS_INIT) {
		AD_ERR("ad(%d) status(0x%.8x) error\r\n", chip_id, g_ad_tp9963_info[chip_id].status);
		return E_PAR;
	}

	ad_private_info = ad_tp9963_get_private_info(chip_id);

	if (unlikely(ad_private_info == NULL)) {
		AD_ERR("[%s] ad watchdog cb fail. NULL ad private info\r\n", AD_TP9963_MODULE_NAME);
		return E_SYS;
	}

	i_vin = detect_info->vin_id;
	if (ad_private_info->wdi[i_vin].scan == SCAN_HDA) { // do ahd module detection
		if (unlikely(ad_tp9963_det_ahd(chip_id, i_vin, &video_info) != E_OK)) {
			AD_ERR("[%s vin%u] ad watchdog cb fail (ahd)\r\n", AD_TP9963_MODULE_NAME, i_vin);
			return E_SYS;
		}
	} else { // do normal detection
		if (unlikely(ad_tp9963_det_video_info(chip_id, i_vin, &video_info) != E_OK)) {
			AD_ERR("[%s vin%u] ad watchdog cb fail\r\n", AD_TP9963_MODULE_NAME, i_vin);
			return E_SYS;
		}
	}

	detect_info->vin_loss = video_info.vloss;
	detect_info->det_info.width = video_info.width;
	detect_info->det_info.height = video_info.height >> (video_info.prog ? 0 : 1);
	detect_info->det_info.fps = video_info.fps;
	detect_info->det_info.prog = video_info.prog;
	AD_IND("cb [%d] loss=%d w=%d h=%d fps=%d pro=%d std=%d\r\n", i_vin, video_info.vloss, video_info.width, video_info.height, video_info.fps, video_info.prog, video_info.std);

	return E_OK;
}

static ER ad_tp9963_i2c_write(UINT32 chip_id, UINT32 reg_addr, UINT32 value, UINT32 data_len)
{
	return ad_drv_i2c_write(chip_id, reg_addr, value, data_len);
}

static ER ad_tp9963_i2c_read(UINT32 chip_id, UINT32 reg_addr, UINT32 *value, UINT32 data_len)
{
	return ad_drv_i2c_read(chip_id, reg_addr, value, data_len);
}

static ER ad_tp9963_dbg(char *str_cmd)
{
#define AD_TP9963_DBG_CMD_LEN 64	// total characters of cmd string
#define AD_TP9963_DBG_CMD_MAX 10	// number of cmd
	char str_cmd_buf[AD_TP9963_DBG_CMD_LEN+1] = {0}, *_str_cmd = str_cmd_buf;
	char *cmd_list[AD_TP9963_DBG_CMD_MAX] = {0};
	UINT32 cmd_num;
	UINT32 narg[AD_TP9963_DBG_CMD_MAX] = {0};
	UINT32 val = 0, val2 = 0, i_chip = 0, i_vin;
	AD_TP9963_DET_VIDEO_INFO det_info = {0};
	AD_TP9963_VIN_MODE vin_mode = {0};
	AD_TP9963_CH_MODE ch_mode = {0};

	ad_drv_dbg(str_cmd);

	// must copy cmd string to another buffer before parsing, to prevent
	// content being modified
	strncpy(_str_cmd, str_cmd, AD_TP9963_DBG_CMD_LEN);
	cmd_num = ad_drv_dbg_parse(_str_cmd, cmd_list, AD_TP9963_DBG_CMD_MAX);

	if (cmd_num == 0 || strcmp(cmd_list[0], "") == 0 || strcmp(cmd_list[0], "help") == 0 || strcmp(cmd_list[0], "?") == 0) {
		ad_tp9963_dbg_dump_help(i_chip);

	} else if (strcmp(cmd_list[0], "dumpinfo") == 0) {
		ad_tp9963_dbg_dump_info(i_chip);

	} else if (strcmp(cmd_list[0], "det") == 0) {
		for (i_vin = 0; i_vin < AD_TP9963_VIN_MAX; i_vin++) {
			ad_tp9963_det_video_info(i_chip, i_vin, &det_info);
			g_ad_tp9963_info[i_chip].vin[i_vin].vloss = det_info.vloss;
			g_ad_tp9963_info[i_chip].vin[i_vin].det_format.width = det_info.width;
			g_ad_tp9963_info[i_chip].vin[i_vin].det_format.height = det_info.height;
			g_ad_tp9963_info[i_chip].vin[i_vin].det_format.fps = det_info.fps;
			g_ad_tp9963_info[i_chip].vin[i_vin].det_format.prog = det_info.prog;
		}

	} else if (strcmp(cmd_list[0], "get") == 0) {

		if (cmd_num == 1) {
			AD_DUMP("lp: get lower-power time information.\r\n");
			AD_DUMP("vch: get virtual channel's vin source id.\r\n");
			AD_DUMP("patgen: get pattern gen mode.\r\n");

		} else if (strcmp(cmd_list[1], "lp") == 0) {
			ad_tp9963_get_mipi_t_prep_reg(i_chip, &val);
			ad_tp9963_get_mipi_t_prep_time(i_chip, &val2);
			AD_DUMP("tprep = 0x%x(%u) = %u(ns)\r\n", val, val, val2);
			ad_tp9963_get_mipi_t_trail_reg(i_chip, &val);
			ad_tp9963_get_mipi_t_trail_time(i_chip, &val2);
			AD_DUMP("ttrail = 0x%x(%u) = %u(ns)\r\n", val, val, val2);

		} else if (strcmp(cmd_list[1], "clane_stop") == 0) {
			ad_tp9963_get_mipi_clk_lane_stop(i_chip, 0, &val);
			AD_DUMP("clk_lane%u = %s\r\n", 0, val ? "STOP" : "NORMAL");

		} else if (strcmp(cmd_list[1], "vch") == 0) {
			ad_tp9963_get_virtual_ch_src(i_chip, 0, &val);
			AD_DUMP("virtual_ch_id_%u = %#x\r\n", 0, val);
			ad_tp9963_get_virtual_ch_src(i_chip, 1, &val);
			AD_DUMP("virtual_ch_id_%u = %#x\r\n", 1, val);
			ad_tp9963_get_virtual_ch_src(i_chip, 2, &val);
			AD_DUMP("virtual_ch_id_%u = %#x\r\n", 2, val);
			ad_tp9963_get_virtual_ch_src(i_chip, 3, &val);
			AD_DUMP("virtual_ch_id_%u = %#x\r\n", 3, val);

		} else if (strcmp(cmd_list[1], "patgen") == 0) {
			for (i_vin = 0; i_vin < AD_TP9963_VIN_MAX; i_vin++) {
				ad_tp9963_get_pattern_gen_mode(i_chip, i_vin, (AD_TP9963_PATTERN_GEN_MODE *)&val);
				AD_DUMP("vin%u patgen mode = %s\r\n", i_vin,
					val == AD_TP9963_PATTERN_GEN_MODE_FORCE ? "FORCE" :
					val == AD_TP9963_PATTERN_GEN_MODE_AUTO ? "AUTO" :
					"DISABLE");
			}
		}

	} else if (strcmp(cmd_list[0], "set") == 0) {

		if (cmd_num == 1) {
			AD_DUMP("vfmt VIN(u) W(u) H(u) FPS(u) [prog(u)] [std(u)]: set video format of VIN. std: 0-TVI, 1-AHD\r\n");
			AD_DUMP("tprep_reg VAL(x): set t_prep register.\r\n");
			AD_DUMP("tprep_time VAL(x): set t_prep time(ns).\r\n");
			AD_DUMP("ttrail_reg VAL(x): set t_trail register.\r\n");
			AD_DUMP("ttrail_time VAL(x): set t_trail time(ns).\r\n");
			AD_DUMP("clane_stop STOP(u): set clock lane stop. 0: normal, 1: stop\r\n");
			AD_DUMP("vch VCH_ID(u) VIN_ID(u): set virtual channel's vin source id. 0~3\r\n");
			AD_DUMP("patgen MODE(u): set pattern gen mode. 0: disable, 1: auto, 2: force\r\n");

		} else if (cmd_num >= 6 && strcmp(cmd_list[1], "vfmt") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1 && // vin_id
				sscanf_s(cmd_list[3], "%u", &narg[1]) == 1 && // width
				sscanf_s(cmd_list[4], "%u", &narg[2]) == 1 && // height
				sscanf_s(cmd_list[5], "%u", &narg[3]) == 1) { // fps

				if (cmd_num < 7 || sscanf_s(cmd_list[6], "%u", &narg[4]) != 1) {
					narg[4] = TRUE; // prog
				}

				if (cmd_num < 8 || sscanf_s(cmd_list[7], "%u", &narg[5]) != 1) {
					narg[5] = STD_TVI; // std
				}

				vin_mode.width = narg[1];
				vin_mode.height = narg[2];
				vin_mode.fps = narg[3] * 100;
				vin_mode.prog = narg[4];
				vin_mode.std = narg[5];
				if (unlikely(ad_tp9963_vin_mode_to_ch_mode(vin_mode, &ch_mode) == E_OK)) {
					ad_tp9963_set_reg_page(i_chip, narg[0]);
					ad_tp9963_set_video_mode(i_chip, narg[0], ch_mode.mode, ch_mode.std);
				} else {
					AD_ERR("set err\r\n");
				}

			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "tprep_reg") == 0) {
			if (sscanf_s(cmd_list[2], "%x", &narg[0]) == 1) {
				ad_tp9963_set_mipi_t_prep_reg(i_chip, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "tprep_time") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9963_set_mipi_t_prep_time(i_chip, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "ttrail_reg") == 0) {
			if (sscanf_s(cmd_list[2], "%x", &narg[0]) == 1) {
				ad_tp9963_set_mipi_t_trail_reg(i_chip, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "ttrail_time") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9963_set_mipi_t_trail_time(i_chip, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "clane_stop") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9963_set_mipi_clk_lane_stop(i_chip, 0, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 4 && strcmp(cmd_list[1], "vch") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1 && sscanf_s(cmd_list[3], "%u", &narg[1]) == 1) {
				ad_tp9963_set_virtual_ch_src(i_chip, narg[0], narg[1]);
			} else {
				AD_ERR("set err\r\n");
			}

		} else if (cmd_num >= 3 && strcmp(cmd_list[1], "patgen") == 0) {
			if (sscanf_s(cmd_list[2], "%u", &narg[0]) == 1) {
				ad_tp9963_set_pattern_gen_mode(i_chip, CH_ALL, narg[0]);
			} else {
				AD_ERR("set err\r\n");
			}
		}
	}

	return E_OK;
}

AD_DEV* ad_get_obj_tp9963(void)
{
	return &g_ad_tp9963_obj;
}

