/*
    Sensor Interface ctrl library - ctrl cmd interface (vx1)

    Sensor Interface ctrl library.

    @file       sen_ctrl_cmd_if_vx1.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "sen_ctrl_cmdif_int.h"
UINT32 g_ctl_sen_cmdif_kdrv_hdl[CTL_SEN_ID_MAX_SUPPORT] = {0};
static INT32 sen_ctrl_cmdif_vx1_open(CTL_SEN_ID id)
{
	UINT32 i, plug = 0;
	INT32 rt = 0, kdrv_rt = 0;
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;

	if (kdrv_ssenif_open(KDRV_CHIP_SSENIF, sen_get_drvdev_vx1(id)) == 0) {
		g_ctl_sen_cmdif_kdrv_hdl[id] = KDRV_DEV_ID(KDRV_CHIP_SSENIF, sen_get_drvdev_vx1(id), 0);

		/* set vx1 tx module, must be set before KDRV_SSENIF_VX1_GET_PLUG */
		kdrv_rt |= kdrv_ssenif_set(g_ctl_sen_cmdif_kdrv_hdl[id], KDRV_SSENIF_VX1_TXTYPE, init_cfg_obj.cmd_if_cfg.vx1.tx_type);
		for (i = 0; i < 10; i++) {
			// vx1 need to get plug before set KDRV_SSENIF_PARAM_ID
			kdrv_rt |= kdrv_ssenif_get(g_ctl_sen_cmdif_kdrv_hdl[id], KDRV_SSENIF_VX1_GET_PLUG, (VOID *)(&plug));
			if (plug == 1) {
				break;
			}
		}
		if (kdrv_rt != 0) {
			return CTL_SEN_E_KDRV_SSENIF;
		}
		if (plug == 0) {
			CTL_SEN_DBG_WRN("get vx1 plug fail\r\n");
		}
	}  else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		return CTL_SEN_E_KDRV_SSENIF;
	}

	return rt;
}

static INT32 sen_ctrl_cmdif_vx1_close(CTL_SEN_ID id)
{
	INT32 rt = 0, kdrv_rt = 0;

	kdrv_rt |= kdrv_ssenif_close(KDRV_CHIP_SSENIF, sen_get_drvdev_vx1(id));

	if (kdrv_rt == 0) {
		g_ctl_sen_cmdif_kdrv_hdl[id] = 0;
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	return rt;
}

static INT32 sen_ctrl_cmdif_vx1_start(CTL_SEN_ID id)
{
	INT32 rt = 0, kdrv_rt = 0;

	kdrv_rt |= kdrv_ssenif_trigger(g_ctl_sen_cmdif_kdrv_hdl[id], ENABLE);
	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	return rt;
}

static INT32 sen_ctrl_cmdif_vx1_stop(CTL_SEN_ID id)
{
	INT32 rt = 0, kdrv_rt = 0;

	kdrv_rt |= kdrv_ssenif_trigger(g_ctl_sen_cmdif_kdrv_hdl[id], DISABLE);
	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	return rt;
}

static CTL_SEN_INTE sen_ctrl_cmdif_vx1_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag)
{
	UINT32 value, rst;
	CTL_SEN_INTE inte = CTL_SEN_INTE_NONE;

	if (((waited_flag & CTL_SEN_INTE_VX1_VD) == CTL_SEN_INTE_VX1_VD)) {
		waited_flag &= ~(CTL_SEN_VX1_TAG);
		value = sen_ctrl_inte_cov2_ssenif(waited_flag);
		rst = kdrv_ssenif_get(g_ctl_sen_cmdif_kdrv_hdl[id], KDRV_SSENIF_VX1_WAIT_INTERRUPT, &value);
		// fixed coverity CID 132839
		if (rst == 0) {
			inte = sen_ctrl_ssenifinte_cov2_sen(rst);
		}
	}

	return inte;
}

static INT32 sen_ctrl_cmdif_vx1_set_mode_cfg(CTL_SEN_ID id, CTL_SEN_MODE mode, CTL_SEN_OUTPUT_DEST output_dest)
{
#define IF_HDL g_ctl_sen_cmdif_kdrv_hdl[id]

	INT32 kdrv_rt = 0, rt = 0;
	ER rt_er;
	UINT32 i;
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;
	CTL_SEN_INIT_EXT_OBJ init_ext_obj = g_ctl_sen_init_obj[id]->init_ext_obj;
	BOOL sendrv_param_error = FALSE, in_param_error = FALSE;

	CTL_SENDRV_GET_ATTR_BASIC_PARAM attr_info;
	CTL_SENDRV_GET_MODE_BASIC_PARAM mode_info;
	CTL_SENDRV_GET_SPEED_PARAM speed_info;
	CTL_SENDRV_GET_MODE_MIPI_PARAM mipi_info;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM cmdif_info;
	KDRV_SSENIFVX1_TXTYPE tx_type = KDRV_SSENIFVX1_TXTYPE_THCV231;
	// only for vx1 controller 0 (KDRV_SSENIF_ENGINE_VX1_0), vx1 controoler 1 always output to SIE4/6
	KDRV_SSENIF_PARAM_ID imgid_to_sie[CTL_SEN_MFRAME_MAX_NUM] = {KDRV_SSENIF_VX1_IMGID_TO_SIE, KDRV_SSENIF_VX1_IMGID_TO_SIE2, KDRV_SSENIF_VX1_IMGID_TO_SIE3, KDRV_SSENIF_VX1_IMGID_TO_SIE4};

	memset((void *)&attr_info, 0, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
	memset((void *)&mode_info, 0, sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
	memset((void *)&speed_info, 0, sizeof(CTL_SENDRV_GET_SPEED_PARAM));
	memset((void *)&mipi_info, 0, sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
	memset((void *)&cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));

	mode_info.mode = mode;
	speed_info.mode = mode;
	mipi_info.mode = mode;
	rt_er = E_OK;
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_BASIC, &attr_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_BASIC, &mode_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_SPEED, &speed_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_MIPI, &mipi_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &cmdif_info);
	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("id %d get sendrv info fail\r\n", id);
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}
	kdrv_rt |= kdrv_ssenif_get(IF_HDL, KDRV_SSENIF_VX1_TXTYPE, (VOID *)(&tx_type));
	if (kdrv_rt != 0) {
		CTL_SEN_DBG_ERR("get vx1 kdrv tx type error %d, force to KDRV_SSENIFVX1_TXTYPE_THCV231\r\n", kdrv_rt);
		tx_type = KDRV_SSENIFVX1_TXTYPE_THCV231;
	}

	/* set vx1 cfg */
	if (attr_info.vendor == CTL_SEN_VENDOR_SONY) {
		if (mode_info.mode_type == CTL_SEN_MODE_STAGGER_HDR) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_SONY_LI_DOL);
		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_SONY_NONEHDR);
		}
	} else if (attr_info.vendor == CTL_SEN_VENDOR_OMNIVISION) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_OMNIVISION);
	} else if (attr_info.vendor == CTL_SEN_VENDOR_ONSEMI) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_ONSEMI);
	} else if (attr_info.vendor == CTL_SEN_VENDOR_PANASONIC) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_PANASONIC);
	} else if (attr_info.vendor == CTL_SEN_VENDOR_OTHERS) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_OTHERS);
	}
	/*
	    KDRV_SSENIFVX1_DATAMUX_3BYTE_RAW/KDRV_SSENIFVX1_DATAMUX_4BYTE_RAW/KDRV_SSENIFVX1_DATAMUX_YUV16: CTL_SIE_PXCLKSRC_VX1_2X
	    KDRV_SSENIFVX1_DATAMUX_YUV8/KDRV_SSENIFVX1_DATAMUX_NONEMUXING: CTL_SIE_PXCLKSRC_VX1_1X
	*/
	if (sen_ctrl_chk_data_fmt_is_raw(mode_info.data_fmt) == TRUE) {
		// raw sensor
		if ((tx_type == KDRV_SSENIFVX1_TXTYPE_THCV231) || (tx_type == KDRV_SSENIFVX1_TXTYPE_THCV235)) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_DATAMUX_SEL, KDRV_SSENIFVX1_DATAMUX_4BYTE_RAW);
		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_DATAMUX_SEL, KDRV_SSENIFVX1_DATAMUX_3BYTE_RAW);
		}
	} else {
		// yuv sensor
		if (mode_info.pixel_depth == CTL_SEN_PIXDEPTH_8BIT) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_DATAMUX_SEL, KDRV_SSENIFVX1_DATAMUX_YUV8);
		} else if (mode_info.pixel_depth == CTL_SEN_PIXDEPTH_16BIT) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_DATAMUX_SEL, KDRV_SSENIFVX1_DATAMUX_YUV16);
		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_DATAMUX_SEL, KDRV_SSENIFVX1_DATAMUX_YUV8);
			CTL_SEN_DBG_ERR("vx1 N.S. yuv sensor output %d bit, force to 8bit vx1 setting\r\n", mode_info.pixel_depth);
			sendrv_param_error = TRUE;
		}
	}

	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_TIMEOUT_PERIOD, init_ext_obj.if_info.timeout_ms);

	if (tx_type == KDRV_SSENIFVX1_TXTYPE_THCV241) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_VALID_WIDTH, mode_info.valid_size.w);

		if (mode_info.pixel_depth == CTL_SEN_PIXDEPTH_10BIT) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_PIXEL_DEPTH, 10);
		} else if (mode_info.pixel_depth == CTL_SEN_PIXDEPTH_12BIT) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_PIXEL_DEPTH, 12);
		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_PIXEL_DEPTH, 10);
			CTL_SEN_DBG_ERR("vx1 TX241 N.S. sensor output %d bit, force to 10bit vx1 setting\r\n", mode_info.pixel_depth);
			sendrv_param_error = TRUE;
		}

		if (init_cfg_obj.pin_cfg.vx1_tx241_cfg_2lane_mode == TRUE) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_VX1LANE_NUMBER, 2);
		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_VX1LANE_NUMBER, 1);
		}

		if (mipi_info.data_lane == CTL_SEN_DATALANE_1) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_MIPILANE_NUMBER, 1);
		} else if (mipi_info.data_lane == CTL_SEN_DATALANE_2) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_MIPILANE_NUMBER, 2);
		} else if (mipi_info.data_lane == CTL_SEN_DATALANE_4) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_MIPILANE_NUMBER, 4);
		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_MIPILANE_NUMBER, 2);
			CTL_SEN_DBG_ERR("vx1 tx241 N.S. mipi datalane num %d, force to 2 lane\r\n", mipi_info.data_lane);
			sendrv_param_error = TRUE;
		}

		if (sen_ctrl_chk_mode_type_is_multiframe(mode_info.mode_type)) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_HDR_ENABLE, ENABLE);
			if (output_dest == CTL_SEN_OUTPUT_DEST_AUTO) {
				CTL_SEN_DBG_ERR("hdr sensor mode need to assingn output_dest (cannot be auto)\r\n");
				in_param_error = TRUE;
			} else {
				CTL_SEN_OUTPUT_DEST tmp_output_dest = output_dest;
				UINT32 frame_cnt = 0;

				for (i = 0; i < CTL_SEN_MFRAME_MAX_NUM; i++) {
					if ((int)(tmp_output_dest & (1 << i)) == (int)(1 << i)) {
						tmp_output_dest &= ~(1 << i);
						kdrv_rt |= kdrv_ssenif_set(IF_HDL, imgid_to_sie[i], mipi_info.sel_frm_id[frame_cnt]);
						frame_cnt++;
					}
				}
				if (tmp_output_dest != 0) {
					CTL_SEN_DBG_ERR("N.S. output_dest remaining bit 0x%.8x (user set: 0x%.8x)\r\n", tmp_output_dest, output_dest);
					in_param_error = TRUE;
				}
				if (frame_cnt != mode_info.frame_num) {
					CTL_SEN_DBG_ERR("output_dest 0x%.8x not match sensor mode frame num %d\r\n", output_dest, mode_info.frame_num);
					in_param_error = TRUE;
				}
			}
		} else {
			if (sen_get_drvdev_vx1(id) == KDRV_SSENIF_ENGINE_VX1_0) {
				if (id < CTL_SEN_MFRAME_MAX_NUM) {
					kdrv_rt |= kdrv_ssenif_set(IF_HDL, imgid_to_sie[id], mipi_info.sel_frm_id[0]);
				} else {
					CTL_SEN_DBG_ERR("vx1 KDRV_DEV_ENGINE 0x%.8x N.S. output SIE %d\r\n", (unsigned int)sen_get_drvdev_vx1(id), id + 1);
					in_param_error = TRUE;
				}
			}
		}

		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_241_INPUT_CLK_FREQ, cmdif_info.vx1.tx241_input_clk_freq);
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_SENSOR_TARGET_MCLK, speed_info.mclk);
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_SENSOR_REAL_MCLK, cmdif_info.vx1.tx241_real_mclk);
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_SENSOR_CKSPEED_BPS, cmdif_info.vx1.tx241_clane_speed);
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_VX1_THCV241_CKO_OUTPUT, init_cfg_obj.pin_cfg.vx1_tx241_cko_pin);
	}

	if (in_param_error) {
		rt = CTL_SEN_E_IN_PARAM;
	} else if (sendrv_param_error) {
		rt = CTL_SEN_E_SENDRV_PARAM;
	} else if (kdrv_rt != 0) {
		CTL_SEN_DBG_ERR("id %d kdrv_rt %d error\r\n", id, kdrv_rt);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}

	return rt;
}

static INT32 sen_ctrl_cmdif_vx1_set_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value)
{
#define IF_HDL g_ctl_sen_cmdif_kdrv_hdl[id]

	INT32 rt = 0, kdrv_rt = 0;

	if ((cfg_id & KDRV_SSENIF_VX1_BASE) != KDRV_SSENIF_VX1_BASE) {
		CTL_SEN_DBG_ERR("pls use %s as input cfg_id (input:0x%.8x)\r\n", CTL_SEN_CMD2STR(KDRV_SSENIF_VX1_BASE), cfg_id);
		return CTL_SEN_E_IN_PARAM;
	}

	kdrv_rt |= kdrv_ssenif_set(IF_HDL, cfg_id, value);

	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d cfg_id %d value %d error\r\n", id, cfg_id, value);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	return rt;
}

static INT32 sen_ctrl_cmdif_vx1_get_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value)
{
#define IF_HDL g_ctl_sen_cmdif_kdrv_hdl[id]

	INT32 rt = 0, kdrv_rt = 0;

	if ((cfg_id & KDRV_SSENIF_VX1_BASE) != KDRV_SSENIF_VX1_BASE) {
		CTL_SEN_DBG_ERR("pls use %s as input cfg_id (input:0x%.8x)\r\n", CTL_SEN_CMD2STR(KDRV_SSENIF_VX1_BASE), cfg_id);
		return CTL_SEN_E_IN_PARAM;
	}

	kdrv_rt |= kdrv_ssenif_get(IF_HDL, cfg_id, (VOID *)value);

	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d cfg_id 0x%.8x error\r\n", id, cfg_id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	return rt;
}

static CTL_SEN_CTRL_CMDIF_VX1 sen_ctrl_cmdif_vx1_tab = {

	sen_ctrl_cmdif_vx1_open,
	sen_ctrl_cmdif_vx1_close,

	sen_ctrl_cmdif_vx1_start,
	sen_ctrl_cmdif_vx1_stop,

	sen_ctrl_cmdif_vx1_wait_interrupt,

	sen_ctrl_cmdif_vx1_set_mode_cfg,

	sen_ctrl_cmdif_vx1_set_cfg,
	sen_ctrl_cmdif_vx1_get_cfg,
};

CTL_SEN_CTRL_CMDIF_VX1 *sen_ctrl_cmdif_vx1_get_obj(void)
{
	return &sen_ctrl_cmdif_vx1_tab;
}

