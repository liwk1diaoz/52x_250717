/*
    Sensor Interface DAL library - ctrl sensor interface (lvds)

    Sensor Interface DAL library.

    @file       sen_ctrl_if_lvds.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "sen_ctrl_if_int.h"
static CTL_SEN_INTE sen_ctrl_if_lvds_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);
static UINT8 last_stop_method_idx[CTL_SEN_ID_MAX_SUPPORT] = {0}; // for lvds close, force disable case (need to wait fmd, otherwise the SIE may receive an incomplete frame)
static BOOL sen_ctrl_lvds_en[CTL_SEN_ID_MAX_SUPPORT] = {0};

static BOOL sen_lvds_get_force_dis(CTL_SEN_ID id)
{
	CTL_SEN_INIT_EXT_OBJ init_ext_obj = g_ctl_sen_init_obj[id]->init_ext_obj;

	if (init_ext_obj.if_info.timeout_ms) {
		return TRUE; // force disable
	}
	return FALSE; // wait fmd to disable
}

static INT32 sen_ctrl_if_lvds_open(CTL_SEN_ID id)
{
	INT32 rt = 0;
	UINT32 engine = sen_get_drvdev_lvds(id);

	if (engine == CTL_SEN_IGNORE) {
		return CTL_SEN_E_IN_PARAM;
	}

	if (kdrv_ssenif_open(KDRV_CHIP_SSENIF, engine) == 0) {
		g_ctl_sen_if_kdrv_hdl[id] = KDRV_DEV_ID(KDRV_CHIP_SSENIF, engine, 0);
		rt = CTL_SEN_E_OK;
	}  else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}

	last_stop_method_idx[id] = 1;

	if (sen_lvds_get_force_dis(id)) {
		kdrv_ssenif_set(g_ctl_sen_if_kdrv_hdl[id], KDRV_SSENIF_LVDS_STOP_METHOD, KDRV_SSENIF_STOPMETHOD_IMMEDIATELY);
	}

	return rt;
}

static INT32 sen_ctrl_if_lvds_close(CTL_SEN_ID id)
{
	INT32 kdrv_rt, rt = 0;
	UINT32 engine = sen_get_drvdev_lvds(id);

	if (engine == CTL_SEN_IGNORE) {
		return CTL_SEN_E_IN_PARAM;
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

static INT32 sen_ctrl_if_lvds_start(CTL_SEN_ID id)
{
	INT32 kdrv_rt, rt = 0;

	kdrv_rt = kdrv_ssenif_trigger(g_ctl_sen_if_kdrv_hdl[id], ENABLE);
	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	sen_ctrl_lvds_en[id] = TRUE;
	return rt;
}

static INT32 sen_ctrl_if_lvds_stop(CTL_SEN_ID id)
{
	INT32 kdrv_rt, rt = 0;

	if ((sen_lvds_get_force_dis(id)) && (sen_ctrl_lvds_en[id])) {
		// force disable case need to wait fmd, otherwise the SIE may receive an incomplete frame
		sen_ctrl_if_lvds_wait_interrupt(id, (CTL_SEN_INTE_FRAMEEND << (last_stop_method_idx[id] - 1)));
	}

	kdrv_rt = kdrv_ssenif_trigger(g_ctl_sen_if_kdrv_hdl[id], DISABLE);
	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	sen_ctrl_lvds_en[id] = FALSE;
	return rt;
}

static CTL_SEN_INTE sen_ctrl_if_lvds_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag)
{
	UINT32 value = sen_ctrl_inte_cov2_ssenif(waited_flag);
	UINT32 rst;

	rst = kdrv_ssenif_get(g_ctl_sen_if_kdrv_hdl[id], KDRV_SSENIF_LVDS_WAIT_INTERRUPT, &value);

	if (rst != E_OK) {
		CTL_SEN_DBG_ERR("fail! (id %d val 0x%.8x)\r\n", (int)id, (int)value);
	}

	return sen_ctrl_ssenifinte_cov2_sen(value);
}

KDRV_SSENIF_LANESEL lvds_lane_sel[SEN_LVDS_DL_NUM] = {
	KDRV_SSENIF_LANESEL_HSI_D0, KDRV_SSENIF_LANESEL_HSI_D1, KDRV_SSENIF_LANESEL_HSI_D2, KDRV_SSENIF_LANESEL_HSI_D3,
	KDRV_SSENIF_LANESEL_HSI_D4, KDRV_SSENIF_LANESEL_HSI_D5, KDRV_SSENIF_LANESEL_HSI_D6, KDRV_SSENIF_LANESEL_HSI_D7,
};

static INT32 cfg_lvds_ctrl_word(CTL_SEN_ID id, CTL_SEN_LVDS_CTRLPTN *ctrl_ptn, UINT32 idx)
{
#define IF_HDL g_ctl_sen_if_kdrv_hdl[id]

	INT32 kdrv_rt = 0;

	if (ctrl_ptn != NULL) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_DLANE_CW_HD,    KDRV_SSENIFLVDS_CONTROL_WORD(lvds_lane_sel[idx], ctrl_ptn->ctrl_hd));
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_DLANE_CW_VD,    KDRV_SSENIFLVDS_CONTROL_WORD(lvds_lane_sel[idx], ctrl_ptn->ctrl_vd[0]));
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_DLANE_CW_VD2,   KDRV_SSENIFLVDS_CONTROL_WORD(lvds_lane_sel[idx], ctrl_ptn->ctrl_vd[1]));
		//#NT#2018/06/05#Silvia Wu -begin
		//#NT# need to consider output SIE controller
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_DLANE_CW_VD3,     KDRV_SSENIFLVDS_CONTROL_WORD(lvds_lane_sel[idx], ctrl_ptn->ctrl_vd[2]));
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_DLANE_CW_VD4,     KDRV_SSENIFLVDS_CONTROL_WORD(lvds_lane_sel[idx], ctrl_ptn->ctrl_vd[3]));
		//#NT#2018/06/05#Silvia Wu -end

	} else {
		CTL_SEN_DBG_ERR("id %d get sendrv lvds ctrl word error\r\n", id);
	}
	return kdrv_rt;
}

static INT32 sen_ctrl_if_lvds_set_mode_cfg(CTL_SEN_ID id, CTL_SEN_MODE mode, CTL_SEN_OUTPUT_DEST output_dest)
{
#define IF_HDL g_ctl_sen_if_kdrv_hdl[id]

	INT32 kdrv_rt = 0, rt = 0;
	ER rt_er;
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;
	CTL_SEN_INIT_EXT_OBJ init_ext_obj = g_ctl_sen_init_obj[id]->init_ext_obj;
	BOOL b_set_ctrl_word = FALSE;
	BOOL sendrv_param_error = FALSE, in_param_error = FALSE;

	KDRV_SSENIF_STOPMETHOD stop_method[CTL_SEN_MAX_OUTPUT_SIE_IDX + 1] = {
		KDRV_SSENIF_STOPMETHOD_IMMEDIATELY, KDRV_SSENIF_STOPMETHOD_SIE1_FRAMEEND, KDRV_SSENIF_STOPMETHOD_SIE2_FRAMEEND,
		KDRV_SSENIF_STOPMETHOD_IMMEDIATELY, KDRV_SSENIF_STOPMETHOD_SIE4_FRAMEEND, KDRV_SSENIF_STOPMETHOD_SIE5_FRAMEEND
	};
	KDRV_SSENIF_PARAM_ID cfgid_outorder[SEN_LVDS_DL_NUM] = {
		KDRV_SSENIF_LVDS_OUTORDER_0, KDRV_SSENIF_LVDS_OUTORDER_1, KDRV_SSENIF_LVDS_OUTORDER_2, KDRV_SSENIF_LVDS_OUTORDER_3,
		KDRV_SSENIF_LVDS_OUTORDER_4, KDRV_SSENIF_LVDS_OUTORDER_5, KDRV_SSENIF_LVDS_OUTORDER_6, KDRV_SSENIF_LVDS_OUTORDER_7,
	};
	/*KDRV_SSENIF_LANESEL lvds_pixelorder_mask[SEN_LVDS_CTL_NUM] = {
	    (KDRV_SSENIF_LANESEL_HSI_LOW4 | KDRV_SSENIF_LANESEL_HSI_HIGH4),
	    (KDRV_SSENIF_LANESEL_HSI_HIGH4),
	    (KDRV_SSENIF_LANESEL_HSI_D2 | KDRV_SSENIF_LANESEL_HSI_D3),
	    (KDRV_SSENIF_LANESEL_HSI_D6 | KDRV_SSENIF_LANESEL_HSI_D7),
	    (KDRV_SSENIF_LANESEL_HSI_D1),
	    (KDRV_SSENIF_LANESEL_HSI_D3),
	    (KDRV_SSENIF_LANESEL_HSI_D5),
	    (KDRV_SSENIF_LANESEL_HSI_D7),
	};*/
	KDRV_SSENIF_PARAM_ID imgid_to_sie[CTL_SEN_MAX_OUTPUT_SIE_IDX] = {KDRV_SSENIF_LVDS_IMGID_TO_SIE, KDRV_SSENIF_LVDS_IMGID_TO_SIE2, KDRV_SSENIF_LVDS_IMGID_TO_SIE3, KDRV_SSENIF_LVDS_IMGID_TO_SIE4, KDRV_SSENIF_LVDS_IMGID_TO_SIE5};

	UINT32 idx, i, lvds_dl_mask = 0, order_cnt = 0, lvds_bit_cnt = 0, mask;
	//UINT32 pixel_order[SEN_LVDS_DL_NUM] = {0, 0, 0, 0, 0, 0, 0, 0}, sel_frm_bit_ofs[CTL_SEN_MFRAME_MAX_NUM] = {0, 0};
	UINT32 sel_frm_bit_ofs[CTL_SEN_MFRAME_MAX_NUM] = {0, 0};
	KDRV_SSENIF_LANESEL valid_lane = 0;
	CTL_SEN_LVDS_CTRLPTN *ctrl_ptn;

	CTL_SENDRV_GET_ATTR_BASIC_PARAM attr_info;
	CTL_SENDRV_GET_MODE_BASIC_PARAM mode_info;
	CTL_SENDRV_GET_SPEED_PARAM speed_info;
	CTL_SENDRV_GET_MODE_LVDS_PARAM lvds_info;
	CTL_SENDRV_GET_ATTR_IF_PARAM attr_if_info;

	memset((void *)&attr_info, 0, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
	memset((void *)&mode_info, 0, sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
	memset((void *)&speed_info, 0, sizeof(CTL_SENDRV_GET_SPEED_PARAM));
	memset((void *)&lvds_info, 0, sizeof(CTL_SENDRV_GET_MODE_LVDS_PARAM));

	/* get information */
	mode_info.mode = mode;
	speed_info.mode = mode;
	lvds_info.mode = mode;
	rt_er = E_OK;
	attr_if_info.type = CTL_SEN_IF_TYPE_LVDS;
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_BASIC, &attr_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_BASIC, &mode_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_SPEED, &speed_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_LVDS, &lvds_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_IF, &attr_if_info);
	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("id %d get sendrv info fail\r\n", id);
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	if (attr_info.vendor == CTL_SEN_VENDOR_OTHERS) {
		CTL_SEN_DBG_ERR("need to implement\r\n");
		return CTL_SEN_E_NS;
	}

	/* set lvds cfg */
	if (attr_info.vendor == CTL_SEN_VENDOR_SONY) {
		if (mode_info.mode_type == CTL_SEN_MODE_STAGGER_HDR) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_SONY_LI_DOL);
		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_SONY_NONEHDR);
		}
	} else if (attr_info.vendor == CTL_SEN_VENDOR_OMNIVISION) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_OMNIVISION);
	} else if (attr_info.vendor == CTL_SEN_VENDOR_ONSEMI) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_ONSEMI);
	} else if (attr_info.vendor == CTL_SEN_VENDOR_PANASONIC) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_PANASONIC);
	} else if (attr_info.vendor == CTL_SEN_VENDOR_OTHERS) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_OTHERS);
	}

	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_DLANE_NUMBER, lvds_info.data_lane);
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_VALID_WIDTH, mode_info.valid_size.w);
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_VALID_HEIGHT, mode_info.valid_size.h);
	i = sen_ctrl_clanesel_conv2_ssenif(id, init_cfg_obj.pin_cfg.clk_lane_sel);
	if (i != CTL_SEN_SKIP_SET_KDRV) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_CLANE_SWITCH, (KDRV_SSENIF_CLANE)i);
	}
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_TIMEOUT_PERIOD, init_ext_obj.if_info.timeout_ms);
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_PIXEL_DEPTH, mode_info.pixel_depth);

	last_stop_method_idx[id] = 1;
	if (mode_info.frame_num > CTL_SEN_MFRAME_MAX_NUM) {
		CTL_SEN_DBG_ERR("%s error\r\n", CTL_SEN_CMD2STR(mode_info.frame_num));
		//kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_STOP_METHOD, stop_method[1]);
		last_stop_method_idx[id] = 1;
		sendrv_param_error = TRUE;
	} else {
		if (output_dest == CTL_SEN_OUTPUT_DEST_AUTO) {
			//kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_STOP_METHOD, stop_method[1]);
			last_stop_method_idx[id] = 1;
		}
		for (i = 0; i < CTL_SEN_MAX_OUTPUT_SIE_IDX; i++) {
			if ((int)(output_dest & (1 << i)) == (int)(1 << i)) {
				//kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_STOP_METHOD, stop_method[i + 1]);
				last_stop_method_idx[id] = i + 1;
			}
		}
	}

	if (sen_lvds_get_force_dis(id)) {
		kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_STOP_METHOD, stop_method[0]);
	} else {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_STOP_METHOD, stop_method[last_stop_method_idx[id]]);
	}

	if ((mode_info.mode_type == CTL_SEN_MODE_STAGGER_HDR) || (mode_info.mode_type == CTL_SEN_MODE_PDAF)) {
		if (output_dest == CTL_SEN_OUTPUT_DEST_AUTO) {
			CTL_SEN_DBG_ERR("hdr sensor mode need to assingn output_dest (cannot be auto)\r\n");
			in_param_error = TRUE;
		} else {
			UINT32 frame_cnt = 0;

			for (i = 0; i < CTL_SEN_MAX_OUTPUT_SIE_IDX; i++) {
				if ((int)(output_dest & (1 << i)) == (int)(1 << i)) {
					/* chk output sie */
					if (sen_get_ssenif_cap(id) & sen_get_ssenif_cap_sie(i)) {
						/* set krv parameters */
						kdrv_rt |= kdrv_ssenif_set(IF_HDL, imgid_to_sie[i], lvds_info.sel_frm_id[frame_cnt]);
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
	}

	for (idx = 0; idx < SEN_LVDS_DL_NUM; idx++) {
		if (init_cfg_obj.pin_cfg.sen_2_serial_pin_map[idx] != CTL_SEN_IGNORE) {
			for (i = 0; i < SEN_LVDS_DL_NUM; i++) {
				if (init_cfg_obj.pin_cfg.sen_2_serial_pin_map[idx] == lvds_info.output_pixel_order[i]) {
					kdrv_rt |= kdrv_ssenif_set(IF_HDL, cfgid_outorder[i], lvds_lane_sel[idx]);
					lvds_dl_mask |= lvds_lane_sel[idx];
//					lvds_pixelorder_mask[mapif_id] &= ~(lvds_lane_sel[pixel_order[idx]]);
					valid_lane |= lvds_lane_sel[lvds_info.output_pixel_order[init_cfg_obj.pin_cfg.sen_2_serial_pin_map[idx]]];
					order_cnt++;
					break;
				}
			}
		}
	}

	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_SYNCCODE_DEFAULT, KDRV_SSENIFLVDS_CONTROL_WORD(lvds_dl_mask, 0));

	/* [OPTIONAL] */
	if (attr_info.vendor == CTL_SEN_VENDOR_OTHERS) {
		for (i = 0; i < 16; i++) {
			mask = 1 << i;
			if (mask & lvds_info.sel_bit_ofs) {
				if (lvds_bit_cnt >= CTL_SEN_MFRAME_MAX_NUM) {
					CTL_SEN_DBG_ERR("sensor driver sel_bit_ofs out of range (0x%.8x)\r\n", lvds_info.sel_bit_ofs);
					sendrv_param_error = TRUE;
					break;
				}
				sel_frm_bit_ofs[lvds_bit_cnt] = i;
				lvds_bit_cnt++;
			}
		}
		//#NT#2018/06/04#Silvia Wu -begin
		//#NT# need convert to ssenif input
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_PIXEL_INORDER, lvds_info.data_in_order);
		//#NT#2018/06/04#Silvia Wu -end
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_FSET_BIT, lvds_info.fset_bit);
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_IMGID_BIT0, sel_frm_bit_ofs[0]);
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_IMGID_BIT1, sel_frm_bit_ofs[1]);
		//#NT#2018/06/05#Silvia Wu -begin
		//#NT# need to consider output SIE controller
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_IMGID_BIT2, sel_frm_bit_ofs[2]);
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_LVDS_IMGID_BIT3, sel_frm_bit_ofs[3]);
		//#NT#2018/06/05#Silvia Wu -end

	}

	if (attr_info.vendor == CTL_SEN_VENDOR_OTHERS) {
		if (attr_if_info.info.lvds.fp_get_ctrl_ptn != NULL) {
			b_set_ctrl_word = TRUE;
		}
	}

	if (b_set_ctrl_word) {
		for (idx = 0; idx < SEN_LVDS_DL_NUM; idx++) {
			if (init_cfg_obj.pin_cfg.sen_2_serial_pin_map[idx] != CTL_SEN_IGNORE) {
				for (i = 0; i < SEN_LVDS_DL_NUM; i++) {
					if (init_cfg_obj.pin_cfg.sen_2_serial_pin_map[idx] == lvds_info.output_pixel_order[i]) {
						ctrl_ptn = attr_if_info.info.lvds.fp_get_ctrl_ptn(i, mode_info.pixel_depth, mode_info.mode_type);
						kdrv_rt |= cfg_lvds_ctrl_word(id, ctrl_ptn, i);
					}
				}
			}
		}

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


static INT32 sen_ctrl_if_lvds_set_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value)
{
#define IF_HDL g_ctl_sen_if_kdrv_hdl[id]

	INT32 kdrv_rt, rt = 0;

	if ((cfg_id & KDRV_SSENIF_LVDS_BASE) != KDRV_SSENIF_LVDS_BASE) {
		CTL_SEN_DBG_ERR("pls use %s as input cfg_id (input:0x%.8x)\r\n", CTL_SEN_CMD2STR(KDRV_SSENIF_LVDS_BASE), cfg_id);
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

static INT32 sen_ctrl_if_lvds_get_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value)
{
#define IF_HDL g_ctl_sen_if_kdrv_hdl[id]

	INT32 kdrv_rt, rt = 0;

	if ((cfg_id & KDRV_SSENIF_LVDS_BASE) != KDRV_SSENIF_LVDS_BASE) {
		CTL_SEN_DBG_ERR("pls use %s as input cfg_id (input:0x%.8x)\r\n", CTL_SEN_CMD2STR(KDRV_SSENIF_LVDS_BASE), cfg_id);
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

static CTL_SEN_CTRL_IF_LVDS sen_ctrl_if_lvds_tab = {

	sen_ctrl_if_lvds_open,
	sen_ctrl_if_lvds_close,

	sen_ctrl_if_lvds_start,
	sen_ctrl_if_lvds_stop,

	sen_ctrl_if_lvds_wait_interrupt,

	sen_ctrl_if_lvds_set_mode_cfg,

	sen_ctrl_if_lvds_set_cfg,
	sen_ctrl_if_lvds_get_cfg,
};

CTL_SEN_CTRL_IF_LVDS *sen_ctrl_if_lvds_get_obj(void)
{
	return &sen_ctrl_if_lvds_tab;
}

