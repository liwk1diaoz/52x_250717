/**
	@brief Source file of vendor media videocapture.\n

	@file vendor_videocapture.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#if defined(__LINUX)
#include <sys/ioctl.h>
#endif
#include <string.h>
#include "hdal.h"
#define HD_MODULE_NAME VENDOR_VIDEOCAPTURE

#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_ioctl.h"
#include "vendor_videocapture.h"
#include "kflow_videocapture/isf_vdocap.h"
#if defined(__FREERTOS)
#include "kflow_videocapture/ctl_sie.h"
#endif
#include "kflow_common/isf_flow_def.h"
#include "kflow_common/isf_flow_ioctl.h"
#if defined (__FREERTOS)
#include "isp_api.h"
#include "plat/top.h"
#endif

#if defined (__FREERTOS)
#define ISF_OPEN     isf_flow_open
#define ISF_IOCTL    isf_flow_ioctl
#define ISF_CLOSE    isf_flow_close
#endif
#if defined(__LINUX)
#define ISF_OPEN     open
#define ISF_IOCTL    ioctl
#define ISF_CLOSE    close
#endif
#define DBG_ERR(fmt, args...) 	printf("%s: " fmt, __func__, ##args)
#define DBG_DUMP				printf
#define CHKPNT    printf("\033[37mCHK: %d, %s\033[0m\r\n", __LINE__, __func__)


/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define DEV_BASE		ISF_UNIT_VDOCAP
#define DEV_COUNT		ISF_MAX_VDOCAP
#define IN_BASE		ISF_IN_BASE
#define IN_COUNT		1
#define OUT_BASE		ISF_OUT_BASE
#define OUT_COUNT 	1

#define HD_DEV_BASE	HD_DAL_VIDEOCAP_BASE
#define HD_DEV_MAX	HD_DAL_VIDEOCAP_MAX

#define _HD_CONVERT_SELF_ID(dev_id, rv) \
	do { \
		(rv) = HD_ERR_DEV;	\
		if((dev_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((dev_id) >= HD_DEV_BASE && (dev_id) <= HD_DEV_MAX) { \
			UINT32 id = (dev_id) - HD_DEV_BASE; \
			if(id < DEV_COUNT) { \
				(dev_id) = DEV_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

#define _HD_CONVERT_OUT_ID(out_id, rv) \
	do { \
		(rv) = HD_ERR_IO; \
		if((out_id) == 0) { \
			(rv) = HD_ERR_UNIQUE; \
		} else if((out_id) >= HD_OUT_BASE && (out_id) <= HD_OUT_MAX) { \
			UINT32 id = (out_id) - HD_OUT_BASE; \
			if(id < OUT_COUNT) { \
				(out_id) = OUT_BASE + id; \
				(rv) = HD_OK; \
			} \
		} \
	} while(0)

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
extern int _hd_common_get_fd(void);

/*-----------------------------------------------------------------------------*/
/* Local Function Prototype                                                    */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Debug Variables & Functions                                                 */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

#if defined (__FREERTOS)
#include "kflow_videocapture/ctl_sen.h"
#include "rtos_na51055/pll.h"
#include "../source/include/pll_protected_na51055.h"

extern void pll_enable_clock(CG_EN Num);
extern void pll_set_clock_rate(PLL_CLKSEL clk_sel, UINT32 ui_value);

typedef struct {
	BOOL					clk_en;
	CTL_SIE_MCLK_SRC_SEL	mclk_src_sel;
	UINT32					clk_rate;
	CTL_SIE_MCLK_ID    		mclk_id_sel;
} CTL_SIE_MCLK_INFO;

static void _vdocap_pixfmt_to_senfmt(UINT32 pxlfmt, CTL_SEN_DATA_FMT *p_data_fmt, CTL_SEN_PIXDEPTH *p_pixdepth)
{
	if (pxlfmt == VDO_PXLFMT_YUV422) {
		*p_data_fmt = CTL_SEN_DATA_FMT_YUV;
		*p_pixdepth = CTL_SEN_IGNORE;

	} else {
		switch (pxlfmt & 0xFFFF0000) {
		case VDO_PXLFMT_RAW8:
		case VDO_PXLFMT_RAW8_SHDR2:
		case VDO_PXLFMT_RAW8_SHDR3:
		case VDO_PXLFMT_RAW8_SHDR4:
			*p_pixdepth = CTL_SEN_PIXDEPTH_8BIT;
			break;
		case VDO_PXLFMT_RAW10:
		case VDO_PXLFMT_RAW10_SHDR2:
		case VDO_PXLFMT_RAW10_SHDR3:
		case VDO_PXLFMT_RAW10_SHDR4:
			*p_pixdepth = CTL_SEN_PIXDEPTH_10BIT;
			break;
		case VDO_PXLFMT_RAW12:
		case VDO_PXLFMT_RAW12_SHDR2:
		case VDO_PXLFMT_RAW12_SHDR3:
		case VDO_PXLFMT_RAW12_SHDR4:
		case VDO_PXLFMT_NRX12:
		case VDO_PXLFMT_NRX12_SHDR2:
		case VDO_PXLFMT_NRX12_SHDR3:
		case VDO_PXLFMT_NRX12_SHDR4:
			*p_pixdepth = CTL_SEN_PIXDEPTH_12BIT;
			break;
		case VDO_PXLFMT_RAW16:
		case VDO_PXLFMT_RAW16_SHDR2:
		case VDO_PXLFMT_RAW16_SHDR3:
		case VDO_PXLFMT_RAW16_SHDR4:
			*p_pixdepth = CTL_SEN_PIXDEPTH_16BIT;
			break;
		default:
			*p_pixdepth = CTL_SEN_PIXDEPTH_8BIT;
			DBG_ERR("-VdoIN:incorrect pxlfmt(0x%X)!\r\n", pxlfmt);
			break;
		}
		switch (pxlfmt & 0x0000FFFF) {
		case VDO_PIX_RCCB:
			*p_data_fmt = CTL_SEN_DATA_FMT_RCCB;
			break;
		case VDO_PIX_RGBIR:
			*p_data_fmt = CTL_SEN_DATA_FMT_RGBIR;
			break;
		case VDO_PIX_Y:
			*p_data_fmt = CTL_SEN_DATA_FMT_Y_ONLY;
			break;
		default: //default VDO_PIX_RGB
			*p_data_fmt = CTL_SEN_DATA_FMT_RGB;
			break;
		}
	}
}
#if 0
static void _dump_sen_init(VDOCAP_SEN_INIT_CFG *p_data)
{
	UINT32 *p_map;

	DBG_DUMP("######## VENDOR sensor init  ########\r\n");
	DBG_DUMP("driver_name = %s\r\n", p_data->driver_name);
	DBG_DUMP("pin_cfg.pinmux.sensor_pinmux = 0x%X\r\n",
																	p_data->sen_init_cfg.pin_cfg.pinmux.sensor_pinmux);
	DBG_DUMP("       .pinmux.serial_if_pinmux = 0x%X\r\n",
																	p_data->sen_init_cfg.pin_cfg.pinmux.serial_if_pinmux);
	DBG_DUMP("       .pinmux.cmd_if_pinmux = 0x%X\r\n", p_data->sen_init_cfg.pin_cfg.pinmux.cmd_if_pinmux);
	DBG_DUMP("       .clk_lane_sel = %d\r\n", p_data->sen_init_cfg.pin_cfg.clk_lane_sel);
	DBG_DUMP("       .sen_2_serial_pin_map[] =\r\n");
	p_map = p_data->sen_init_cfg.pin_cfg.sen_2_serial_pin_map;
	DBG_DUMP("{%d, %d, %d, %d, %d, %d, %d, %d}\r\n", p_map[0], p_map[1], p_map[2], p_map[3], p_map[4], p_map[5], p_map[6], p_map[7]);
	DBG_DUMP("       .ccir_msblsb_switch = %d\r\n", p_data->sen_init_cfg.pin_cfg.ccir_msblsb_switch);
	DBG_DUMP("       .ccir_vd_hd_pin = %d\r\n", p_data->sen_init_cfg.pin_cfg.ccir_vd_hd_pin);
	DBG_DUMP("       .vx1_tx241_cko_pin = %d\r\n", p_data->sen_init_cfg.pin_cfg.vx1_tx241_cko_pin);
	DBG_DUMP("       .vx1_tx241_cfg_2lane_mode = %d\r\n", p_data->sen_init_cfg.pin_cfg.vx1_tx241_cfg_2lane_mode);

	DBG_DUMP("if_cfg.type = %d\r\n", p_data->sen_init_cfg.if_cfg.type);
	DBG_DUMP("      .tge.tge_en = %d\r\n", p_data->sen_init_cfg.if_cfg.tge.tge_en);
	DBG_DUMP("      .tge.swap = %d\r\n", p_data->sen_init_cfg.if_cfg.tge.swap);
	DBG_DUMP("      .tge.sie_vd_src = %d\r\n", p_data->sen_init_cfg.if_cfg.tge.sie_vd_src);
	DBG_DUMP("      .tge.sie_sync_set = %d\r\n", p_data->sen_init_cfg.if_cfg.tge.sie_sync_set);

	DBG_DUMP("cmd_if_cfg.vx1.en = %d\r\n", p_data->sen_init_cfg.cmd_if_cfg.vx1.en);
	DBG_DUMP("          .vx1.if_sel = %d\r\n", p_data->sen_init_cfg.cmd_if_cfg.vx1.if_sel);
	DBG_DUMP("          .vx1.ctl_sel = %d\r\n", p_data->sen_init_cfg.cmd_if_cfg.vx1.ctl_sel);
	DBG_DUMP("          .vx1.tx_type = %d\r\n", p_data->sen_init_cfg.cmd_if_cfg.vx1.tx_type);

	DBG_DUMP("option.en_mask = %d\r\n", p_data->sen_init_option.en_mask);
	DBG_DUMP("      .sen_map_if = %d\r\n", p_data->sen_init_option.sen_map_if);
	DBG_DUMP("      .if_time_out = %d\r\n", p_data->sen_init_option.if_time_out);

	DBG_DUMP("#######################################\r\n");
}
#endif
static BOOL set_if_cfg(VDOCAP_SEN_INIT_IF_CFG *p_if_cfg, HD_VIDEOCAP_SENSOR_DEVICE *p_sen)
{
	BOOL ret = TRUE;

	switch (p_sen->if_type) {
	case HD_COMMON_VIDEO_IN_MIPI_CSI:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_MIPI;
		break;
	case HD_COMMON_VIDEO_IN_LVDS:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_LVDS;
		break;
	case HD_COMMON_VIDEO_IN_SLVS_EC:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_SLVSEC;
		break;
	case HD_COMMON_VIDEO_IN_P_RAW:
	case HD_COMMON_VIDEO_IN_P_AHD:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_PARALLEL;
		break;
	case HD_COMMON_VIDEO_IN_MIPI_VX1:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_MIPI;
		break;
	case HD_COMMON_VIDEO_IN_P_RAW_VX1:
		p_if_cfg->type = VDOCAP_SEN_IF_TYPE_PARALLEL;
		break;

	default:
		DBG_ERR("not support if_type = %d\r\n", p_sen->if_type);
		ret = FALSE;
		break;
	}

	p_if_cfg->tge.tge_en = p_sen->if_cfg.tge.tge_en;
	p_if_cfg->tge.swap = p_sen->if_cfg.tge.swap;
	p_if_cfg->tge.sie_vd_src = p_sen->if_cfg.tge.vcap_vd_src;
	p_if_cfg->tge.sie_sync_set = p_sen->if_cfg.tge.vcap_sync_set;

	return ret;
}
static BOOL set_cmd_if_cfg(VDOCAP_SEN_INIT_CMDIF_CFG *p_cmd_if_cfg, HD_VIDEOCAP_SENSOR_DEVICE *p_sen)
{
	BOOL ret = TRUE;

	p_cmd_if_cfg->vx1.en = p_sen->if_cfg.vx1.en;
	p_cmd_if_cfg->vx1.if_sel = p_sen->if_cfg.vx1.if_sel;
	p_cmd_if_cfg->vx1.ctl_sel = p_sen->if_cfg.vx1.ctl_sel;
	p_cmd_if_cfg->vx1.tx_type = p_sen->if_cfg.vx1.tx_type;

	return ret;
}
static BOOL set_pin_cfg(VDOCAP_SEN_INIT_PIN_CFG *p_pin_cfg, HD_VIDEOCAP_SENSOR_DEVICE *p_sen)
{
	BOOL ret = TRUE;

	p_pin_cfg->pinmux.sensor_pinmux =  p_sen->pin_cfg.pinmux.sensor_pinmux;
	p_pin_cfg->pinmux.cmd_if_pinmux = p_sen->pin_cfg.pinmux.cmd_if_pinmux;
	p_pin_cfg->pinmux.serial_if_pinmux = p_sen->pin_cfg.pinmux.serial_if_pinmux;
	p_pin_cfg->clk_lane_sel = p_sen->pin_cfg.clk_lane_sel;
	if (VDOCAP_SEN_SER_MAX_DATALANE >= HD_VIDEOCAP_SEN_SER_MAX_DATALANE) {
		memcpy(p_pin_cfg->sen_2_serial_pin_map, p_sen->pin_cfg.sen_2_serial_pin_map, sizeof(p_pin_cfg->sen_2_serial_pin_map));
	} else {
		DBG_ERR("sen_2_serial_pin_map len not match %d,%d\r\n", VDOCAP_SEN_SER_MAX_DATALANE, HD_VIDEOCAP_SEN_SER_MAX_DATALANE);
		ret = FALSE;
	}
	p_pin_cfg->ccir_msblsb_switch = p_sen->pin_cfg.ccir_msblsb_switch;
	p_pin_cfg->ccir_vd_hd_pin = p_sen->pin_cfg.ccir_vd_hd_pin;
	p_pin_cfg->vx1_tx241_cko_pin = p_sen->pin_cfg.vx1_tx241_cko_pin;
	p_pin_cfg->vx1_tx241_cfg_2lane_mode = p_sen->pin_cfg.vx1_tx241_cfg_2lane_mode;
	return ret;
}
static CTL_SEN_DRVDEV _vdocap_lvds_csi_mapping(CTL_SEN_IF_TYPE if_cfg_type, CTL_SEN_CLANE_SEL clk_lane_sel)
{
	CTL_SEN_DRVDEV ret = 0;
	if (if_cfg_type == CTL_SEN_IF_TYPE_LVDS) {
		if (clk_lane_sel == CTL_SEN_CLANE_SEL_LVDS0_USE_C0C4 || clk_lane_sel == CTL_SEN_CLANE_SEL_LVDS0_USE_C2C6) {
			ret = CTL_SEN_DRVDEV_LVDS_0;
		} else {
			ret = CTL_SEN_DRVDEV_LVDS_1;
		}
	} else if (if_cfg_type == CTL_SEN_IF_TYPE_MIPI) {
		if (clk_lane_sel == CTL_SEN_CLANE_SEL_CSI0_USE_C0 || clk_lane_sel == CTL_SEN_CLANE_SEL_CSI0_USE_C2) {
			ret = CTL_SEN_DRVDEV_CSI_0;
		} else {
			ret = CTL_SEN_DRVDEV_CSI_1;
		}
	}
	return ret;
}

static INT32 ctl_sen_init_cfg(UINT32 id, PCTL_SEN_OBJ p_sen_ctl_obj, HD_VIDEOCAP_SEN_CONFIG *p_sen_cfg)
{
	CTL_SEN_INIT_CFG_OBJ cfg_obj = {0};
	VDOCAP_SEN_INIT_CFG sen_init = {0};
	CTL_SEN_PINMUX pinmux[3] = {0};
	INT32 ret = HD_OK;

	//------------- HDAL flow -----------------------------
	memcpy(sen_init.driver_name, p_sen_cfg->sen_dev.driver_name, VDOCAP_SEN_NAME_LEN);
	if (FALSE == set_if_cfg(&sen_init.sen_init_cfg.if_cfg, &p_sen_cfg->sen_dev))
		return HD_ERR_PARAM;
	if (FALSE == set_cmd_if_cfg(&sen_init.sen_init_cfg.cmd_if_cfg, &p_sen_cfg->sen_dev))
		return HD_ERR_PARAM;
	if (FALSE == set_pin_cfg(&sen_init.sen_init_cfg.pin_cfg, &p_sen_cfg->sen_dev))
		return HD_ERR_PARAM;
	memcpy(&sen_init.sen_init_option, &p_sen_cfg->sen_dev.option, sizeof(VDOCAP_SEN_INIT_OPTION));

	//-------------- isf_vdocap flow ----------------------------------------------------------
	//_dump_sen_init(&sen_init);


	#if 1 // hard code, Ben need to modify
		if (id == 0) {
			pinmux[0].func = 4; //PIN_FUNC_SENSOR;
		} else {
			pinmux[0].func = 5; //PIN_FUNC_SENSOR2;
		}
		pinmux[0].cfg = sen_init.sen_init_cfg.pin_cfg.pinmux.sensor_pinmux;
		pinmux[0].pnext = &pinmux[1];

		pinmux[1].func = 6; // PIN_FUNC_MIPI_LVDS;
		pinmux[1].cfg = sen_init.sen_init_cfg.pin_cfg.pinmux.serial_if_pinmux;
		pinmux[1].pnext = &pinmux[2];

		pinmux[2].func = 7; //PIN_FUNC_I2C;
		pinmux[2].cfg = sen_init.sen_init_cfg.pin_cfg.pinmux.cmd_if_pinmux;
		pinmux[2].pnext = NULL;

		cfg_obj.pin_cfg.pinmux.func = pinmux[0].func;
		cfg_obj.pin_cfg.pinmux.cfg = pinmux[0].cfg;
		cfg_obj.pin_cfg.pinmux.pnext = pinmux[0].pnext;
#else
		cfg_obj.pin_cfg.pinmux.sensor_pinmux = sen_init.sen_init_cfg.pin_cfg.pinmux.sensor_pinmux;
		cfg_obj.pin_cfg.pinmux.serial_if_pinmux = sen_init.sen_init_cfg.pin_cfg.pinmux.serial_if_pinmux;
		cfg_obj.pin_cfg.pinmux.cmd_if_pinmux = sen_init.sen_init_cfg.pin_cfg.pinmux.cmd_if_pinmux;
#endif

	cfg_obj.pin_cfg.clk_lane_sel = sen_init.sen_init_cfg.pin_cfg.clk_lane_sel;
	if (sizeof(cfg_obj.pin_cfg.sen_2_serial_pin_map) == sizeof(sen_init.sen_init_cfg.pin_cfg.sen_2_serial_pin_map)) {
		memcpy(cfg_obj.pin_cfg.sen_2_serial_pin_map, sen_init.sen_init_cfg.pin_cfg.sen_2_serial_pin_map, sizeof(cfg_obj.pin_cfg.sen_2_serial_pin_map));
	} else {
		DBG_ERR("VDOCAP_SEN_SER_MAX_DATALANE size not match!\r\n");
		return ISF_ERR_PROCESS_FAIL;
	}
	cfg_obj.pin_cfg.ccir_msblsb_switch = sen_init.sen_init_cfg.pin_cfg.ccir_msblsb_switch;
	cfg_obj.pin_cfg.ccir_vd_hd_pin = sen_init.sen_init_cfg.pin_cfg.ccir_vd_hd_pin;
	cfg_obj.pin_cfg.vx1_tx241_cko_pin = sen_init.sen_init_cfg.pin_cfg.vx1_tx241_cko_pin;
	cfg_obj.pin_cfg.vx1_tx241_cfg_2lane_mode = sen_init.sen_init_cfg.pin_cfg.vx1_tx241_cfg_2lane_mode;

	cfg_obj.if_cfg.type = sen_init.sen_init_cfg.if_cfg.type;
	cfg_obj.if_cfg.tge.tge_en = sen_init.sen_init_cfg.if_cfg.tge.tge_en;
	cfg_obj.if_cfg.tge.swap = sen_init.sen_init_cfg.if_cfg.tge.swap;
	cfg_obj.if_cfg.tge.sie_vd_src = sen_init.sen_init_cfg.if_cfg.tge.sie_vd_src;

	cfg_obj.cmd_if_cfg.vx1.en = sen_init.sen_init_cfg.cmd_if_cfg.vx1.en;
	cfg_obj.cmd_if_cfg.vx1.if_sel = sen_init.sen_init_cfg.cmd_if_cfg.vx1.if_sel;
//	cfg_obj.cmd_if_cfg.vx1.ctl_sel = sen_init.sen_init_cfg.cmd_if_cfg.vx1.ctl_sel;
	cfg_obj.cmd_if_cfg.vx1.tx_type = sen_init.sen_init_cfg.cmd_if_cfg.vx1.tx_type;
	cfg_obj.drvdev |= _vdocap_lvds_csi_mapping(cfg_obj.if_cfg.type, cfg_obj.pin_cfg.clk_lane_sel);

	if (cfg_obj.if_cfg.tge.tge_en) {
		if (id == 0) {
			cfg_obj.drvdev |= CTL_SEN_DRVDEV_TGE_0;
		} else {
			cfg_obj.drvdev |= CTL_SEN_DRVDEV_TGE_1;
		}
	}
	ret = p_sen_ctl_obj->init_cfg((CHAR *)sen_init.driver_name, &cfg_obj);
	if(ret) {
		DBG_ERR("sen init_cfg failed(%d)!\r\n", ret);
		ret = ISF_ERR_INVALID_VALUE;
	}
	return ret;
}

static HD_RESULT _fast_open_sensor(UINT32 id, VOID *p_param)
{
	VENDOR_VIDEOCAP_FAST_OPEN_SENSOR *p_user = (VENDOR_VIDEOCAP_FAST_OPEN_SENSOR *)p_param;
	INT32 ret;
	PCTL_SEN_OBJ p_sen_ctl_obj = ctl_sen_get_object(id);
	CTL_SEN_CHGMODE_OBJ chgmode_obj = {0};
	UINT32 frame_rate;
	UINT32 ctl_sen_buf_size;
	#if defined (__FREERTOS)
	ISP_SENSOR_PRESET_CTRL preset_ctrl;
	#endif
	static BOOL ctl_sen_inited = FALSE;

	ctl_sen_buf_size = ctl_sen_buf_query(2);
	if (ctl_sen_inited == FALSE) {
		//set addr 0 to let ctl_set use internal kmalloc
		ret = ctl_sen_init(0, ctl_sen_buf_size);
		if (ret) {
			DBG_ERR("ctl sen init fail!(%d)\r\n", ret);
			return HD_ERR_INV;
		}
		ctl_sen_inited = TRUE;
	}

	if (p_sen_ctl_obj == NULL || p_sen_ctl_obj->open == NULL) {
		DBG_ERR("no ctl_sen obj(%d)\r\n", (UINT32)id);
		return HD_ERR_NOT_FOUND;
	}
	//DBG_DUMP("id(%d) sen_mode=%d\r\n", (UINT32)id, (UINT32)p_user->sen_mode);
	ctl_sen_init_cfg(id, p_sen_ctl_obj, &p_user->sen_cfg);
	//sensor open
	ret = p_sen_ctl_obj->open();
	if (ret != CTL_SEN_E_OK) {
		switch(ret) {
		case CTL_SEN_E_MAP_TBL:
			DBG_ERR("no sen driver\r\n");
			ret = HD_ERR_NOT_AVAIL;
			break;
		case CTL_SEN_E_SENDRV:
			DBG_ERR("sensor driver error\r\n");
			ret = HD_ERR_FAIL;//map to HD_ERR_FAIL
			break;
		default:
			DBG_ERR("sensor open fail(%d)\r\n", ret);
			ret = HD_ERR_INV;
			break;
		}
		return ret;
	}
	if (p_user->sen_cfg.sen_dev.if_cfg.tge.tge_en && p_user->sen_cfg.sen_dev.if_cfg.tge.vcap_sync_set) {
		UINT32 tge_sync_id = p_user->sen_cfg.sen_dev.if_cfg.tge.vcap_sync_set;
		//signal sync
		//DBG_MSG("VDOCAP[%d] SIGNAL_SYNC = 0x%X\r\n", id, tge_sync_id);
		ret = p_sen_ctl_obj->set_cfg(CTL_SEN_CFGID_INIT_SIGNAL_SYNC, (void *)&tge_sync_id);
		if (ret){
			DBG_ERR("VCAP[%d] INIT_SIGNAL_SYNC failed(%d)\r\n", id, ret);
			return HD_ERR_NOT_AVAIL;
		}
	}
	ret = p_sen_ctl_obj->pwr_ctrl(CTL_SEN_PWR_CTRL_TURN_ON);
	if (ret != CTL_SEN_E_OK) {
		DBG_ERR("VCAP[%d] PWR_ON failed(%d)\r\n", id, ret);
		return HD_ERR_NOT_AVAIL;
	}

	#if defined (__FREERTOS)
	// NOTE: ISP_SENSOR_PRESET_AE: enable preset; ISP_SENSOR_PRESET_DEFAULT: disable preset
	if (p_user->ae_preset.enable) {
		preset_ctrl.mode = ISP_SENSOR_PRESET_AE;
		//preset_ctrl.mode = ISP_SENSOR_PRESET_DEFAULT;
		preset_ctrl.exp_time[0] = p_user->ae_preset.exp_time;
		preset_ctrl.gain_ratio[0] = p_user->ae_preset.gain_ratio;
		// for HDR
		preset_ctrl.exp_time[1] = (p_user->ae_preset.exp_time>>4);
		preset_ctrl.gain_ratio[1] = p_user->ae_preset.gain_ratio;
		p_sen_ctl_obj->set_cfg(CTL_SEN_CFGID_USER_DEFINE1, &preset_ctrl);
		//DBG_DUMP("\r\n[video_capture] preset exp_time = %d, gain_ratio = %d\r\n\r\n", preset_ctrl.exp_time[0], preset_ctrl.gain_ratio[0]);
	}
	#endif

	frame_rate = 100*GET_HI_UINT16(p_user->frc)/GET_LO_UINT16(p_user->frc);
	if (VDOCAP_SEN_MODE_AUTO == p_user->sen_mode) {
		CTL_SEN_DATA_FMT sen_data_fmt;
		CTL_SEN_PIXDEPTH sen_pixdepth;

		chgmode_obj.mode_sel = CTL_SEN_MODESEL_AUTO;
		chgmode_obj.auto_info.frame_rate = frame_rate;
		chgmode_obj.auto_info.size.w =  p_user->dim.w;
		chgmode_obj.auto_info.size.h = p_user->dim.h;
		chgmode_obj.auto_info.frame_num = p_user->out_frame_num;
		_vdocap_pixfmt_to_senfmt(p_user->pxlfmt, &sen_data_fmt, &sen_pixdepth);
		chgmode_obj.auto_info.data_fmt = sen_data_fmt;
		chgmode_obj.auto_info.pixdepth = sen_pixdepth;
		if (p_user->data_lane) {
			chgmode_obj.auto_info.data_lane = p_user->data_lane;
		} else {
			chgmode_obj.auto_info.data_lane = CTL_SEN_IGNORE;
		}
		if (p_user->builtin_hdr) {
			chgmode_obj.auto_info.mode_type_sel = CTL_SEN_MODE_BUILTIN_HDR;
		} else {
			chgmode_obj.auto_info.mode_type_sel = CTL_SEN_IGNORE;
		}
	} else {
		chgmode_obj.mode_sel = CTL_SEN_MODESEL_MANUAL;
		chgmode_obj.manual_info.frame_rate = frame_rate;
		chgmode_obj.manual_info.sen_mode = p_user->sen_mode;
	}

	if (p_user->sen_cfg.shdr_map) {
		chgmode_obj.output_dest = p_user->sen_cfg.shdr_map&0xFFFF;
	} else {
		chgmode_obj.output_dest = 1 << id;
	}
	ret = p_sen_ctl_obj->chgmode(chgmode_obj);
	if (ret) {
		ret = HD_ERR_INV;
	}
	return ret;
}
#endif
/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_videocap_set(UINT32 id, VENDOR_VIDEOCAP_PARAM_ID param_id, VOID *p_param)
{
	#if !defined (__FREERTOS_LITE)
	HD_DAL self_id = HD_GET_DEV(id);
	//HD_IO out_id = HD_GET_OUT(id);
	HD_IO ctrl_id = HD_GET_CTRL(id);
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	int isf_fd;
	int r;
	#endif
	HD_RESULT rv = HD_ERR_NG;

	if (p_param == NULL) {
		return HD_ERR_NULL_PTR;
	}

	#if defined (__FREERTOS)
	if (param_id == VENDOR_VIDEOCAP_PARAM_FAST_OPEN_SENSOR) {
		return _fast_open_sensor(id, p_param);
	}
	#endif

	#if !defined (__FREERTOS_LITE)
	isf_fd = _hd_common_get_fd();
	if (isf_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
	rv = HD_OK;

	if(ctrl_id == HD_CTRL) {
		switch(param_id) {
		case VENDOR_VIDEOCAP_PARAM_AD_MAP: {
			UINT32 *p_user = (UINT32 *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_AD_MAP;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		case VENDOR_VIDEOCAP_PARAM_AD_TYPE: {
			UINT32 *p_user = (UINT32 *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_AD_TYPE;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		case VENDOR_VIDEOCAP_PARAM_MCLK_SRC_SYNC_SET: {
			UINT32 *p_user = (UINT32 *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_MCLK_SRC_SYNC_SET;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		case VENDOR_VIDEOCAP_PARAM_PDAF_MAP: {
			UINT32 *p_user = (UINT32 *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_PDAF_MAP;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		case VENDOR_VIDEOCAP_PARAM_SIE_MAP: {
			UINT32 *p_user = (UINT32 *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_SIE_MAP;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		case VENDOR_VIDEOCAP_PARAM_SET_FPS: {
			UINT32 *p_user = (UINT32 *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_SET_FPS;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		case VENDOR_VIDEOCAP_PARAM_SW_VD_SYNC: {
			UINT32 *p_user = (UINT32 *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_SW_VD_SYNC;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		default: rv = HD_ERR_PARAM; break;
		}
	} else {
		switch(param_id) {
		case VENDOR_VIDEOCAP_PARAM_BUILTIN_HDR: {
			BOOL *p_user = (BOOL *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_BUILTIN_HDR;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		case VENDOR_VIDEOCAP_PARAM_DATA_LANE: {
			UINT32 *p_user = (UINT32 *)p_param;

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_DATA_LANE;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		break;
		case VENDOR_VIDEOCAP_PARAM_ENC_RATE: {
			UINT32 *p_user = (UINT32 *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_ENC_RATE;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		case VENDOR_VIDEOCAP_PARAM_RESET_FC: {
			UINT32 *p_user = (UINT32 *)p_param;

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_RESET_FC;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		break;
		case VENDOR_VIDEOCAP_PARAM_CCIR_INFO: {
			VENDOR_VIDEOCAP_CCIR_INFO *p_user = (VENDOR_VIDEOCAP_CCIR_INFO *)p_param;
			VDOCAP_CCIR_INFO ccir_info;

			ccir_info.field_sel = (VDOCAP_SEN_FIELD_SEL)p_user->field_sel;
			ccir_info.fmt = (VDOCAP_SEN_CCIR_FMT_SEL)p_user->fmt;
			ccir_info.interlace = p_user->interlace;
			ccir_info.mux_data_index = p_user->mux_data_index;

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_CCIR_INFO;
			cmd.value = (UINT32)&ccir_info;
			cmd.size = sizeof(ccir_info);
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		break;
		case VENDOR_VIDEOCAP_PARAM_GYRO_INFO: {
			VENDOR_VIDEOCAP_GYRO_INFO *p_user = (VENDOR_VIDEOCAP_GYRO_INFO *)p_param;
			VDOCAP_GYRO_INFO gyro_info = {0};

			gyro_info.en = p_user->en;
			gyro_info.data_num = p_user->data_num;

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_GYRO_INFO;
			cmd.value = (UINT32)&gyro_info;
			cmd.size = sizeof(gyro_info);
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		break;
		case VENDOR_VIDEOCAP_PARAM_AE_PRESET: {
			VENDOR_VIDEOCAP_AE_PRESET *p_user = (VENDOR_VIDEOCAP_AE_PRESET *)p_param;
			VDOCAP_AE_PRESET ae_preset = {0};

			ae_preset.enable = p_user->enable;
			ae_preset.exp_time = p_user->exp_time;
			ae_preset.gain_ratio = p_user->gain_ratio;

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_AE_PRESET;
			cmd.value = (UINT32)&ae_preset;
			cmd.size = sizeof(ae_preset);
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		case VENDOR_VIDEOCAP_PARAM_BP3_RATIO: {
			UINT32 *p_user = (UINT32 *)p_param;

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_BP3_RATIO;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		break;
		case VENDOR_VIDEOCAP_PARAM_QUEUE_FLUSH_SCHEME: {
			UINT32 *p_user = (UINT32 *)p_param;

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_QUEUE_FLUSH_SCHEME;
			cmd.value = *p_user;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		break;
		case VENDOR_VIDEOCAP_PARAM_MODE_TYPE: {
			UINT32 mode_type = *(UINT32 *)p_param;

			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_MODE_TYPE;
			cmd.value =  mode_type;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_SET_PARAM, &cmd);
			goto _VD_VC1;
		}
		default: rv = HD_ERR_PARAM; break;
		}
	}
	if(rv != HD_OK)
		return rv;

_VD_VC1:
	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:
			rv = HD_OK;
			break;
		default:
			rv = HD_ERR_SYS;
			break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	#endif
	return rv;
}

#if !defined (__FREERTOS_LITE)
HD_RESULT vendor_videocap_get(UINT32 id, VENDOR_VIDEOCAP_PARAM_ID param_id, VOID *p_param)
{
	HD_DAL self_id = HD_GET_DEV(id);
	//HD_IO out_id = HD_GET_OUT(id);
	HD_IO ctrl_id = HD_GET_CTRL(id);
	ISF_FLOW_IOCTL_PARAM_ITEM cmd = {0};
	int isf_fd;
	int r;
	HD_RESULT rv = HD_ERR_NG;

	if (p_param == NULL) {
		return HD_ERR_NULL_PTR;
	}

	isf_fd = _hd_common_get_fd();
	if (isf_fd <= 0) {
		return HD_ERR_UNINIT;
	}
	_HD_CONVERT_SELF_ID(self_id, rv); 	if(rv != HD_OK) {	return rv;}
	rv = HD_OK;

	if(ctrl_id == HD_CTRL) {
		switch(param_id) {
		case VENDOR_VIDEOCAP_PARAM_GET_PLUG: {
			UINT32 *p_user = (UINT32 *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_GET_PLUG;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
			if (r == 0 && cmd.rv == ISF_OK) {
				*p_user = cmd.value;
			}
			goto _VD_VC2;
		}
		case VENDOR_VIDEOCAP_PARAM_GET_PLUG_INFO: {
			VENDOR_VIDEOCAP_GET_PLUG_INFO* p_user = (VENDOR_VIDEOCAP_GET_PLUG_INFO *)p_param;
			//VENDOR_VIDEOCAP_GET_PLUG_INFO get_plug_info = {0};
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_GET_PLUG_INFO;
			//cmd.value = (UINT32)&get_plug_info;
			cmd.value = (UINT32)p_user;
			cmd.size = sizeof(VENDOR_VIDEOCAP_GET_PLUG_INFO);
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
			goto _VD_VC2;
		}
		case VENDOR_VIDEOCAP_PARAM_CSI_ERR_CNT: {
			UINT32 *p_user = (UINT32 *)p_param;
			cmd.dest = ISF_PORT(self_id, ISF_CTRL);
			cmd.param = VDOCAP_PARAM_CSI_ERR_CNT;
			cmd.size = 0;
			r = ISF_IOCTL(isf_fd, ISF_FLOW_CMD_GET_PARAM, &cmd);
			if (r == 0 && cmd.rv == ISF_OK) {
				*p_user = cmd.value;
			}
			goto _VD_VC2;
		}
		default: rv = HD_ERR_PARAM; break;
		}
	} else {
		switch(param_id) {
		default: rv = HD_ERR_PARAM; break;
		}
	}
	if(rv != HD_OK)
		return rv;

_VD_VC2:
	if (r == 0) {
		switch(cmd.rv) {
		case ISF_OK:
			rv = HD_OK;
			break;
		default:
			rv = HD_ERR_SYS;
			break;
		}
	} else {
		if(((int)cmd.rv <= ISF_ERR_BEGIN) && ((int)cmd.rv >= ISF_ERR_END)) {
			rv = cmd.rv; // ISF_ERR is exactly the same with HD_ERR
		} else {
			DBG_ERR("system fail, rv=%d\r\n", cmd.rv);
			rv = cmd.rv; // ISF_ERR is out of range of HD_ERR
		}
	}
	return rv;
}
#endif
