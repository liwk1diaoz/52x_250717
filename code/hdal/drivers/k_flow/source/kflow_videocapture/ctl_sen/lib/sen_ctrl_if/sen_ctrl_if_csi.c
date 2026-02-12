/*
    Sensor Interface DAL library - ctrl sensor interface (csi)

    Sensor Interface DAL library.

    @file       sen_ctrl_if_csi.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "sen_ctrl_if_int.h"
static CTL_SEN_INTE sen_ctrl_if_csi_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);
static UINT8 last_stop_method_idx[CTL_SEN_ID_MAX_SUPPORT] = {0}; // for csi close, force disable case (need to wait fmd, otherwise the SIE may receive an incomplete frame)
static UINT32 csi_open_cnt[CTL_SEN_DRVDEV_IDX_CSI(CTL_SEN_DRVDEV_CSI_MAX) + 1] = {0}; // by csi engine id

static UINT32 sen_csi_manual_bit_conv2_id(CTL_SEN_MIPI_MANUAL_BIT bit)
{
	switch (bit) {
	case CTL_SEN_MIPI_MANUAL_8BIT:
	case CTL_SEN_MIPI_MANUAL_YUV422:
		return 0x2A;
	case CTL_SEN_MIPI_MANUAL_10BIT:
		return 0x2B;
	case CTL_SEN_MIPI_MANUAL_12BIT:
		return 0x2C;
	case CTL_SEN_MIPI_MANUAL_14BIT:
		return 0x2D;
	default:
		CTL_SEN_DBG_ERR("\r\n");
	}
	return 0x2A;
}

BOOL sen_csi_get_force_dis(CTL_SEN_ID id)
{
#if 1
	// kdrv not support force disable [commit id : 6e9ec65913a39d0b41c83ebf31d80ebcc161fa86]
	return FALSE; // wait fmd to disable
#else
	CTL_SEN_INIT_EXT_OBJ init_ext_obj = g_ctl_sen_init_obj[id]->init_ext_obj;

	if (init_ext_obj.if_info.timeout_ms) {
		return TRUE; // force disable
	}
	return FALSE; // wait fmd to disable
#endif
}

static INT32 sen_ctrl_if_csi_open(CTL_SEN_ID id)
{
	INT32 rt = 0;
	UINT32 engine = sen_get_drvdev_csi(id);

	if (engine == CTL_SEN_IGNORE) {
		return CTL_SEN_E_IN_PARAM;
	}

	csi_open_cnt[engine - KDRV_SSENIF_ENGINE_CSI0]++;
	if (csi_open_cnt[engine - KDRV_SSENIF_ENGINE_CSI0] > 1) {
		g_ctl_sen_if_kdrv_hdl[id] = KDRV_DEV_ID(KDRV_CHIP_SSENIF, engine, 0);
		return CTL_SEN_E_OK;
	}

	if (kdrv_ssenif_open(KDRV_CHIP_SSENIF, engine) == 0) {
		g_ctl_sen_if_kdrv_hdl[id] = KDRV_DEV_ID(KDRV_CHIP_SSENIF, engine, 0);
		rt = CTL_SEN_E_OK;
	}  else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}

	last_stop_method_idx[id] = 1;

	if (sen_csi_get_force_dis(id)) {
		kdrv_ssenif_set(g_ctl_sen_if_kdrv_hdl[id], KDRV_SSENIF_CSI_STOP_METHOD, KDRV_SSENIF_STOPMETHOD_IMMEDIATELY);
	}

	return rt;
}

static INT32 sen_ctrl_if_csi_close(CTL_SEN_ID id)
{
	INT32 kdrv_rt, rt = 0;
	UINT32 engine = sen_get_drvdev_csi(id);

	if (engine == CTL_SEN_IGNORE) {
		return CTL_SEN_E_IN_PARAM;
	}

	csi_open_cnt[engine - KDRV_SSENIF_ENGINE_CSI0]--;
	if (csi_open_cnt[engine - KDRV_SSENIF_ENGINE_CSI0] > 0) {
		g_ctl_sen_if_kdrv_hdl[id] = 0;
		return CTL_SEN_E_OK;
	}

	kdrv_rt = kdrv_ssenif_close(KDRV_CHIP_SSENIF, engine);

	if (kdrv_rt == 0) {
		g_ctl_sen_if_kdrv_hdl[id] = 0;
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	return rt;
}

static INT32 sen_ctrl_if_csi_start(CTL_SEN_ID id)
{
	INT32 kdrv_rt, rt = 0;

	kdrv_rt = kdrv_ssenif_trigger(g_ctl_sen_if_kdrv_hdl[id], ENABLE);
	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}

	return rt;
}

static INT32 sen_ctrl_if_csi_stop(CTL_SEN_ID id)
{
	INT32 kdrv_rt, rt = 0;

	kdrv_rt = kdrv_ssenif_trigger(g_ctl_sen_if_kdrv_hdl[id], DISABLE);
	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}

	return rt;
}

UINT8 sen_ctrl_if_csi_get_last_stop_idx(CTL_SEN_ID id)
{
	return last_stop_method_idx[id];
}

static CTL_SEN_INTE sen_ctrl_if_csi_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag)
{
	UINT32 value = sen_ctrl_inte_cov2_ssenif(waited_flag);
	UINT32 rst;

	rst = kdrv_ssenif_get(g_ctl_sen_if_kdrv_hdl[id], KDRV_SSENIF_CSI_WAIT_INTERRUPT, &value);

	if (rst != E_OK) {
		CTL_SEN_DBG_ERR("fail! (id %d val 0x%.8x)\r\n", (int)id, (int)value);
	}

	return sen_ctrl_ssenifinte_cov2_sen(value);
}

static INT32 sen_ctrl_if_csi_set_mode_cfg(CTL_SEN_ID id, CTL_SEN_MODE mode, CTL_SEN_OUTPUT_DEST output_dest)
{
#define IF_HDL g_ctl_sen_if_kdrv_hdl[id]

	INT32 kdrv_rt = 0, rt = 0;
	ER rt_er, rt_manual_iadj, rt_cl_mode, rt_hs_dly;
	UINT32 i;
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;
	CTL_SEN_INIT_EXT_OBJ init_ext_obj = g_ctl_sen_init_obj[id]->init_ext_obj;
	BOOL sendrv_param_error = FALSE, in_param_error = FALSE;

	CTL_SENDRV_GET_ATTR_BASIC_PARAM attr_info;
	CTL_SENDRV_GET_MODE_BASIC_PARAM mode_info;
	CTL_SENDRV_GET_SPEED_PARAM speed_info;
	CTL_SENDRV_GET_MODE_MIPI_PARAM mipi_info;
	CTL_SENDRV_GET_MODE_MIPI_CLANE_CMETHOD mipi_clane_cmethod;
	CTL_SENDRV_GET_MODE_MIPI_EN_USER mipi_en_user;
	CTL_SENDRV_GET_MODE_MANUAL_IADJ mipi_manual_iadj;
	CTL_SENDRV_GET_MODE_MIPI_HSDATAOUT_DLY mipi_hsdataout_dly;

	KDRV_SSENIF_STOPMETHOD stop_method[CTL_SEN_MAX_OUTPUT_SIE_IDX + 1] = {
		KDRV_SSENIF_STOPMETHOD_IMMEDIATELY, KDRV_SSENIF_STOPMETHOD_SIE1_FRAMEEND, KDRV_SSENIF_STOPMETHOD_SIE2_FRAMEEND,
		KDRV_SSENIF_STOPMETHOD_IMMEDIATELY, KDRV_SSENIF_STOPMETHOD_SIE4_FRAMEEND, KDRV_SSENIF_STOPMETHOD_SIE5_FRAMEEND
	};
	KDRV_SSENIF_PARAM_ID dl_pin[CTL_SEN_CSI_MAX_DATALANE] = {
		KDRV_SSENIF_CSI_DATALANE0_PIN, KDRV_SSENIF_CSI_DATALANE1_PIN, KDRV_SSENIF_CSI_DATALANE2_PIN, KDRV_SSENIF_CSI_DATALANE3_PIN
	};
	KDRV_SSENIF_LANESEL input_pin[CTL_SEN_SER_MAX_DATALANE] = {
		KDRV_SSENIF_LANESEL_HSI_D0, KDRV_SSENIF_LANESEL_HSI_D1, KDRV_SSENIF_LANESEL_HSI_D2, KDRV_SSENIF_LANESEL_HSI_D3,
		KDRV_SSENIF_LANESEL_HSI_D4, KDRV_SSENIF_LANESEL_HSI_D5, KDRV_SSENIF_LANESEL_HSI_D6, KDRV_SSENIF_LANESEL_HSI_D7
	};
	KDRV_SSENIF_PARAM_ID b_manual[CTL_SEN_MIPI_MAX_MANUAL] = {KDRV_SSENIF_CSI_LPKT_MANUAL, KDRV_SSENIF_CSI_LPKT_MANUAL2, KDRV_SSENIF_CSI_LPKT_MANUAL3};
	KDRV_SSENIF_PARAM_ID manual_fmt[CTL_SEN_MIPI_MAX_MANUAL] = {KDRV_SSENIF_CSI_MANUAL_FORMAT, KDRV_SSENIF_CSI_MANUAL2_FORMAT, KDRV_SSENIF_CSI_MANUAL3_FORMAT};
	KDRV_SSENIF_PARAM_ID manual_id[CTL_SEN_MIPI_MAX_MANUAL] = {KDRV_SSENIF_CSI_MANUAL_DATA_ID, KDRV_SSENIF_CSI_MANUAL2_DATA_ID, KDRV_SSENIF_CSI_MANUAL3_DATA_ID};
	KDRV_SSENIF_PARAM_ID imgid_to_sie[CTL_SEN_MAX_OUTPUT_SIE_IDX] = {KDRV_SSENIF_CSI_IMGID_TO_SIE, KDRV_SSENIF_CSI_IMGID_TO_SIE2, KDRV_SSENIF_CSI_IMGID_TO_SIE3, KDRV_SSENIF_CSI_IMGID_TO_SIE4, KDRV_SSENIF_CSI_IMGID_TO_SIE5};
	KDRV_SSENIF_PARAM_ID valid_h[CTL_SEN_MAX_OUTPUT_SIE_IDX] = {KDRV_SSENIF_CSI_VALID_HEIGHT_SIE, KDRV_SSENIF_CSI_VALID_HEIGHT_SIE2, KDRV_SSENIF_CSI_VALID_HEIGHT_SIE3, KDRV_SSENIF_CSI_VALID_HEIGHT_SIE4, KDRV_SSENIF_CSI_VALID_HEIGHT_SIE5};

	memset((void *)&attr_info, 0, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
	memset((void *)&mode_info, 0, sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
	memset((void *)&speed_info, 0, sizeof(CTL_SENDRV_GET_SPEED_PARAM));
	memset((void *)&mipi_info, 0, sizeof(CTL_SENDRV_GET_MODE_MIPI_PARAM));
	memset((void *)&mipi_clane_cmethod, 0, sizeof(CTL_SENDRV_GET_MODE_MIPI_CLANE_CMETHOD));
	memset((void *)&mipi_en_user, 0, sizeof(CTL_SENDRV_GET_MODE_MIPI_EN_USER));
	memset((void *)&mipi_manual_iadj, 0, sizeof(CTL_SENDRV_GET_MODE_MANUAL_IADJ));
	memset((void *)&mipi_hsdataout_dly, 0, sizeof(CTL_SENDRV_GET_MODE_MIPI_HSDATAOUT_DLY));

	/* get information */
	mode_info.mode = mode;
	speed_info.mode = mode;
	mipi_info.mode = mode;
	mipi_manual_iadj.mode = mode;
	mipi_clane_cmethod.mode = mode;
	mipi_hsdataout_dly.mode = mode;
	rt_er = E_OK;
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_BASIC, &attr_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_BASIC, &mode_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_SPEED, &speed_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_MIPI, &mipi_info);
	rt_manual_iadj = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_MIPI_MANUAL_IADJ, &mipi_manual_iadj); // [option]
	rt_cl_mode = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_MIPI_CLANE_CMETHOD, &mipi_clane_cmethod); // [option]
	rt_hs_dly = sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_MIPI_HSDATAOUT_DLY, &mipi_hsdataout_dly); // [option]
	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("id %d get sendrv info fail\r\n", id);
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	/* set csi cfg */
	if (attr_info.vendor == CTL_SEN_VENDOR_SONY) {
		if (mode_info.mode_type == CTL_SEN_MODE_STAGGER_HDR) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_SONY_LI_DOL);

		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_SONY_NONEHDR);
		}
	} else if (attr_info.vendor == CTL_SEN_VENDOR_OMNIVISION) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_OMNIVISION);
	} else if (attr_info.vendor == CTL_SEN_VENDOR_ONSEMI) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_ONSEMI);
	} else if (attr_info.vendor == CTL_SEN_VENDOR_PANASONIC) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_PANASONIC);
	} else if (attr_info.vendor == CTL_SEN_VENDOR_OTHERS) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_OTHERS);
	}

	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSOR_TARGET_MCLK, speed_info.mclk);
#if 0//CTL_SEN_FPGA
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSOR_REAL_MCLK, speed_info.mclk / 2);
#else
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSOR_REAL_MCLK, speed_info.mclk);
#endif
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_DLANE_NUMBER, mipi_info.data_lane);
	i = sen_ctrl_clanesel_conv2_ssenif(id, init_cfg_obj.pin_cfg.clk_lane_sel);
	if (i != CTL_SEN_SKIP_SET_KDRV) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_CLANE_SWITCH, (KDRV_SSENIF_CLANE)i);
	}
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_TIMEOUT_PERIOD, init_ext_obj.if_info.timeout_ms);

	last_stop_method_idx[id] = 1;
	if (mode_info.frame_num > CTL_SEN_MFRAME_MAX_NUM) {
		CTL_SEN_DBG_ERR("%s error\r\n", CTL_SEN_CMD2STR(mode_info.frame_num));
		//kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_STOP_METHOD, stop_method[1]);
		last_stop_method_idx[id] = 1;
		sendrv_param_error = TRUE;
	} else {
		if (output_dest == CTL_SEN_OUTPUT_DEST_AUTO) {
			//kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_STOP_METHOD, stop_method[1]);
			last_stop_method_idx[id] = 1;
		}
		for (i = 0; i < CTL_SEN_MAX_OUTPUT_SIE_IDX; i++) {
			if ((int)(output_dest & (1 << i)) == (int)(1 << i)) {
				//kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_STOP_METHOD, stop_method[i + 1]);
				last_stop_method_idx[id] = i + 1;
			}
		}
	}

	if (sen_csi_get_force_dis(id)) {
		kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_STOP_METHOD, stop_method[0]);
	} else {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_STOP_METHOD, stop_method[last_stop_method_idx[id]]);
	}

	if (output_dest == CTL_SEN_OUTPUT_DEST_AUTO) {
		if ((mode_info.mode_type == CTL_SEN_MODE_STAGGER_HDR) || (mode_info.mode_type == CTL_SEN_MODE_PDAF)) {
			CTL_SEN_DBG_ERR("hdr sensor mode need to assingn output_dest (cannot be auto)\r\n");
			in_param_error = TRUE;
		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_VALID_HEIGHT, mode_info.valid_size.h);
		}
	} else if (output_dest != CTL_SEN_OUTPUT_DEST_AUTO) {
		UINT32 frame_cnt = 0;

		for (i = 0; i < CTL_SEN_MAX_OUTPUT_SIE_IDX; i++) {
			if ((int)(output_dest & (1 << i)) == (int)(1 << i)) {
				/* chk output sie */
				if (sen_get_ssenif_cap(id) & sen_get_ssenif_cap_sie(i)) {
					/* set krv parameters */
					kdrv_rt |= kdrv_ssenif_set(IF_HDL, imgid_to_sie[i], mipi_info.sel_frm_id[frame_cnt]);
					kdrv_rt |= kdrv_ssenif_set(IF_HDL, valid_h[i], mode_info.valid_size.h);
					frame_cnt++;
				} else {
					CTL_SEN_DBG_ERR("SSENIF_%d N.S. output to SIE_%d  (user dst: 0x%.8x)\r\n", id, i, output_dest);
					in_param_error = TRUE;
				}
			}
		}

		if (frame_cnt != mode_info.frame_num) {
			CTL_SEN_DBG_ERR("output_dest 0x%.8x not match sensor mode frame num %d\r\n", output_dest, mode_info.frame_num);
			in_param_error = TRUE;
		}
	}

	for (i = 0; i < CTL_SEN_SER_MAX_DATALANE; i++) {
		if (init_cfg_obj.pin_cfg.sen_2_serial_pin_map[i] == CTL_SEN_IGNORE) {
			continue;
		}
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, dl_pin[init_cfg_obj.pin_cfg.sen_2_serial_pin_map[i]], input_pin[i]);
	}

	for (i = 0; i < CTL_SEN_MIPI_MAX_MANUAL; i++) {
		if (mipi_info.manual_info[i].bit == CTL_SEN_MIPI_MANUAL_NONE) {
			continue;
		} else if (mipi_info.manual_info[i].bit == CTL_SEN_MIPI_MANUAL_8BIT) {
#if 1 // coverity warning CID 131064
			if (i == 0) {
				kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_MANUAL_FORMAT, KDRV_SSENIFCSI_MANUAL_DEPACK_8);
			} else if (i == 1) {
				kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_MANUAL2_FORMAT, KDRV_SSENIFCSI_MANUAL_DEPACK_8);
			} else if (i == 2) {
				kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_MANUAL3_FORMAT, KDRV_SSENIFCSI_MANUAL_DEPACK_8);
			}
#else
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, manual_fmt[i], KDRV_SSENIFCSI_MANUAL_DEPACK_8);
#endif
		} else if (mipi_info.manual_info[i].bit == CTL_SEN_MIPI_MANUAL_10BIT) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, manual_fmt[i], KDRV_SSENIFCSI_MANUAL_DEPACK_10);
		} else if (mipi_info.manual_info[i].bit == CTL_SEN_MIPI_MANUAL_12BIT) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, manual_fmt[i], KDRV_SSENIFCSI_MANUAL_DEPACK_12);
		} else if (mipi_info.manual_info[i].bit == CTL_SEN_MIPI_MANUAL_14BIT) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, manual_fmt[i], KDRV_SSENIFCSI_MANUAL_DEPACK_14);
		} else if (mipi_info.manual_info[i].bit == CTL_SEN_MIPI_MANUAL_YUV422) {
#if 1 // coverity warning CID 131064
			if (i == 0) {
				kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_MANUAL_FORMAT, KDRV_SSENIFCSI_MANUAL_DEPACK_8);
			} else if (i == 1) {
				kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_MANUAL2_FORMAT, KDRV_SSENIFCSI_MANUAL_DEPACK_8);
			} else if (i == 2) {
				kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_MANUAL3_FORMAT, KDRV_SSENIFCSI_MANUAL_DEPACK_8);
			}
#else
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, manual_fmt[i], KDRV_SSENIFCSI_MANUAL_DEPACK_8);
#endif
		}

		if (mipi_info.manual_info[i].data_id == CTL_SEN_MIPI_PIXEL_DATA) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, manual_id[i], sen_csi_manual_bit_conv2_id(mipi_info.manual_info[i].bit));
		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, manual_id[i], mipi_info.manual_info[i].data_id);
		}

		kdrv_rt |= kdrv_ssenif_set(IF_HDL, b_manual[i], ENABLE);
	}

	/* set IADJ, for JIRA [NA51055-713] */
	if (rt_manual_iadj != E_OK) {
		// auto set iadj
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSOR_REAL_HSCLK, ((speed_info.data_rate / mipi_info.data_lane) * mode_info.pixel_depth * 2) / 1000);
	} else {
		// sensor driver manual set iadj
		if (mipi_manual_iadj.sel == CTL_SEN_MANUAL_IADJ_SEL_OFF) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSOR_REAL_HSCLK, ((speed_info.data_rate / mipi_info.data_lane) * mode_info.pixel_depth * 2) / 1000);
		} else if (mipi_manual_iadj.sel == CTL_SEN_MANUAL_IADJ_SEL_IADJ) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_IADJ, mipi_manual_iadj.val.iadj);
		} else if (mipi_manual_iadj.sel == CTL_SEN_MANUAL_IADJ_SEL_DATARATE) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSOR_REAL_HSCLK, (mipi_manual_iadj.val.data_rate) / 1000);
		} else {
			CTL_SEN_DBG_ERR("mipi_manual_iadj.sel err %d, force to auto\r\n", mipi_manual_iadj.sel);
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_SENSOR_REAL_HSCLK, ((speed_info.data_rate / mipi_info.data_lane) * mode_info.pixel_depth * 2) / 1000);
		}
	}

	if (rt_cl_mode == E_OK) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_CLANE_CMETHOD, mipi_clane_cmethod.clane_ctl_mode);
	}

	if (rt_hs_dly == E_OK) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_CSI_HSDATAOUT_DLY, mipi_hsdataout_dly.delay);
	}

	/* check error code */
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

static INT32 sen_ctrl_if_csi_set_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value)
{
#define IF_HDL g_ctl_sen_if_kdrv_hdl[id]

	INT32 kdrv_rt, rt = 0;

	if ((cfg_id & KDRV_SSENIF_CSI_BASE) != KDRV_SSENIF_CSI_BASE) {
		CTL_SEN_DBG_ERR("pls use %s as input cfg_id (input:0x%.8x)\r\n", CTL_SEN_CMD2STR(KDRV_SSENIF_CSI_BASE), cfg_id);
		return CTL_SEN_E_IN_PARAM;
	}

	kdrv_rt = kdrv_ssenif_set(IF_HDL, cfg_id, value);

	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d cfg_id 0x%.8x value %d error\r\n", id, cfg_id, value);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	return rt;
}

static INT32 sen_ctrl_if_csi_get_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value)
{
#define IF_HDL g_ctl_sen_if_kdrv_hdl[id]

	INT32 kdrv_rt, rt = 0;

	if ((cfg_id & KDRV_SSENIF_CSI_BASE) != KDRV_SSENIF_CSI_BASE) {
		CTL_SEN_DBG_ERR("pls use %s as input cfg_id (input:0x%.8x)\r\n", CTL_SEN_CMD2STR(KDRV_SSENIF_CSI_BASE), cfg_id);
		return CTL_SEN_E_IN_PARAM;
	}

	kdrv_rt = kdrv_ssenif_get(IF_HDL, cfg_id, (VOID *)value);

	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d cfg_id 0x%.8x error\r\n", id, cfg_id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	return rt;
}

static CTL_SEN_CTRL_IF_CSI sen_ctrl_if_csi_tab = {

	sen_ctrl_if_csi_open,
	sen_ctrl_if_csi_close,

	sen_ctrl_if_csi_start,
	sen_ctrl_if_csi_stop,

	sen_ctrl_if_csi_wait_interrupt,

	sen_ctrl_if_csi_set_mode_cfg,

	sen_ctrl_if_csi_set_cfg,
	sen_ctrl_if_csi_get_cfg,
};

CTL_SEN_CTRL_IF_CSI *sen_ctrl_if_csi_get_obj(void)
{
	return &sen_ctrl_if_csi_tab;
}

