/*
    Sensor Interface DAL library - ctrl sensor interface (slvsec)

    Sensor Interface DAL library.

    @file       sen_ctrl_if_slvsec.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "sen_ctrl_if_int.h"
static CTL_SEN_INTE sen_ctrl_if_slvsec_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag);
static UINT8 last_stop_method_idx[CTL_SEN_ID_MAX_SUPPORT] = {0}; // for slvsec close, force disable case (need to wait fmd, otherwise the SIE may receive an incomplete frame)
static BOOL sen_ctrl_slvsec_en[CTL_SEN_ID_MAX_SUPPORT] = {0};

static BOOL sen_slvsec_get_force_dis(CTL_SEN_ID id)
{
	CTL_SEN_INIT_EXT_OBJ init_ext_obj = g_ctl_sen_init_obj[id]->init_ext_obj;

	if (init_ext_obj.if_info.timeout_ms) {
		return TRUE; // force disable
	}
	return FALSE; // wait fmd to disable
}

static INT32 sen_ctrl_if_slvsec_open(CTL_SEN_ID id)
{
	INT32 rt = 0;
	UINT32 engine = sen_get_drvdev_slvsec(id);

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

	if (sen_slvsec_get_force_dis(id)) {
		kdrv_ssenif_set(g_ctl_sen_if_kdrv_hdl[id], KDRV_SSENIF_SLVSEC_STOP_METHOD, KDRV_SSENIF_STOPMETHOD_IMMEDIATELY);
	}

	return rt;
}

static INT32 sen_ctrl_if_slvsec_close(CTL_SEN_ID id)
{
	INT32 kdrv_rt, rt = 0;
	UINT32 engine = sen_get_drvdev_slvsec(id);

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

static INT32 sen_ctrl_if_slvsec_start(CTL_SEN_ID id)
{
	INT32 kdrv_rt, rt = 0;

	kdrv_rt = kdrv_ssenif_trigger(g_ctl_sen_if_kdrv_hdl[id], ENABLE);
	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	sen_ctrl_slvsec_en[id] = TRUE;
	return rt;
}

static INT32 sen_ctrl_if_slvsec_stop(CTL_SEN_ID id)
{
	INT32 kdrv_rt, rt = 0;

	if ((sen_slvsec_get_force_dis(id)) && (sen_ctrl_slvsec_en[id])) {
		// force disable case need to wait fmd, otherwise the SIE may receive an incomplete frame
		sen_ctrl_if_slvsec_wait_interrupt(id, (CTL_SEN_INTE_FRAMEEND << (last_stop_method_idx[id] - 1)));
	}

	kdrv_rt = kdrv_ssenif_trigger(g_ctl_sen_if_kdrv_hdl[id], DISABLE);
	if (kdrv_rt == 0) {
		rt = CTL_SEN_E_OK;
	} else {
		CTL_SEN_DBG_ERR("id %d error\r\n", id);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	sen_ctrl_slvsec_en[id] = FALSE;
	return rt;
}

static CTL_SEN_INTE sen_ctrl_if_slvsec_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag)
{
	UINT32 value = sen_ctrl_inte_cov2_ssenif(waited_flag);
	UINT32 rst;

	rst = kdrv_ssenif_get(g_ctl_sen_if_kdrv_hdl[id], KDRV_SSENIF_SLVSEC_WAIT_INTERRUPT, &value);

	if (rst != E_OK) {
		CTL_SEN_DBG_ERR("fail! (id %d val 0x%.8x)\r\n", (int)id, (int)value);
	}

	return sen_ctrl_ssenifinte_cov2_sen(value);
}

static INT32 sen_ctrl_if_slvsec_set_mode_cfg(CTL_SEN_ID id, CTL_SEN_MODE mode)
{
#define IF_HDL g_ctl_sen_if_kdrv_hdl[id]

	INT32 kdrv_rt = 0, rt = 0;
	ER rt_er;
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;
	CTL_SEN_INIT_EXT_OBJ init_ext_obj = g_ctl_sen_init_obj[id]->init_ext_obj;
	BOOL sendrv_param_error = FALSE;

	KDRV_SSENIF_STOPMETHOD stop_method[CTL_SEN_MFRAME_SLVSEC_MAX_NUM + 1] = {
		KDRV_SSENIF_STOPMETHOD_IMMEDIATELY, KDRV_SSENIF_STOPMETHOD_FRAME_END, KDRV_SSENIF_STOPMETHOD_FRAME_END2
	};

	KDRV_SSENIF_LANESEL slvsec_lane_sel[SEN_SLVSEC_CTL_NUM] = {
		KDRV_SSENIF_LANESEL_HSI_D0, KDRV_SSENIF_LANESEL_HSI_D1, KDRV_SSENIF_LANESEL_HSI_D2, KDRV_SSENIF_LANESEL_HSI_D3,
		KDRV_SSENIF_LANESEL_HSI_D4, KDRV_SSENIF_LANESEL_HSI_D5, KDRV_SSENIF_LANESEL_HSI_D6, KDRV_SSENIF_LANESEL_HSI_D7,
	};

	KDRV_SSENIF_PARAM_ID cfgid_outorder[SEN_SLVSEC_CTL_NUM] = {
		KDRV_SSENIF_SLVSEC_DATALANE0_PIN, KDRV_SSENIF_SLVSEC_DATALANE1_PIN, KDRV_SSENIF_SLVSEC_DATALANE2_PIN, KDRV_SSENIF_SLVSEC_DATALANE3_PIN,
		KDRV_SSENIF_SLVSEC_DATALANE4_PIN, KDRV_SSENIF_SLVSEC_DATALANE5_PIN, KDRV_SSENIF_SLVSEC_DATALANE6_PIN, KDRV_SSENIF_SLVSEC_DATALANE7_PIN,
	};

	UINT32 idx, i, slvsec_dl_mask = 0, order_cnt = 0;
	KDRV_SSENIF_LANESEL valid_lane = 0;

	CTL_SENDRV_GET_ATTR_BASIC_PARAM attr_info;
	CTL_SENDRV_GET_MODE_BASIC_PARAM mode_info;
	CTL_SENDRV_GET_SPEED_PARAM speed_info;
	CTL_SENDRV_GET_MODE_SLVSEC_PARAM slvsec_info;
	CTL_SENDRV_GET_ATTR_IF_PARAM attr_if_info;

	memset((void *)&attr_info, 0, sizeof(CTL_SENDRV_GET_ATTR_BASIC_PARAM));
	memset((void *)&mode_info, 0, sizeof(CTL_SENDRV_GET_MODE_BASIC_PARAM));
	memset((void *)&speed_info, 0, sizeof(CTL_SENDRV_GET_SPEED_PARAM));
	memset((void *)&slvsec_info, 0, sizeof(CTL_SENDRV_GET_MODE_SLVSEC_PARAM));

	/* get information */
	mode_info.mode = mode;
	speed_info.mode = mode;
	slvsec_info.mode = mode;
	attr_if_info.type = CTL_SEN_IF_TYPE_SLVSEC;
	rt_er = E_OK;
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_BASIC, &attr_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_BASIC, &mode_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_SPEED, &speed_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_MODE_SLVSEC, &slvsec_info);
	rt_er |= sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_IF, &attr_if_info);
	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("id %d get sendrv info fail\r\n", id);
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	/* set slvsec cfg */
	if (attr_info.vendor == CTL_SEN_VENDOR_SONY) {
		if (mode_info.mode_type == CTL_SEN_MODE_STAGGER_HDR) {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_SONY_LI_DOL);
		} else {
			kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_SENSORTYPE, KDRV_SSENIF_SENSORTYPE_SONY_NONEHDR);
		}
	} else {
		CTL_SEN_DBG_ERR("SLVS-EC DRV not support vendor %d\r\n", attr_info.vendor);
		return CTL_SEN_E_NS;
	}

	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_DLANE_NUMBER, slvsec_info.data_lane);
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_VALID_WIDTH, mode_info.valid_size.w);
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_VALID_HEIGHT, mode_info.valid_size.h);
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_TIMEOUT_PERIOD, init_ext_obj.if_info.timeout_ms);
	kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_PIXEL_DEPTH, mode_info.pixel_depth);
	if (slvsec_info.speed == CTL_SEN_SLVSEC_SPEED_2304) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_SPEED, 2304);
	} else if (slvsec_info.speed == CTL_SEN_SLVSEC_SPEED_1152) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_SPEED, 1152);
	} else {
		CTL_SEN_DBG_ERR("SLVS-EC DRV N.S. speed %d\r\n", slvsec_info.speed);
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_SPEED, 2304);
		sendrv_param_error = TRUE;
	}

	last_stop_method_idx[id] = 1;
	if (mode_info.frame_num > CTL_SEN_MFRAME_SLVSEC_MAX_NUM) {
		CTL_SEN_DBG_ERR("%s error\r\n", CTL_SEN_CMD2STR(mode_info.frame_num));
		//kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_STOP_METHOD, stop_method[1]);
		last_stop_method_idx[id] = 1;
		sendrv_param_error = TRUE;
	} else {
		//kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_STOP_METHOD, stop_method[mode_info.frame_num]);
		last_stop_method_idx[id] = mode_info.frame_num;
	}
	if (sen_slvsec_get_force_dis(id)) {
		kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_STOP_METHOD, stop_method[0]);
	} else {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_STOP_METHOD, stop_method[last_stop_method_idx[id]]);
	}

	if (mode_info.mode_type == CTL_SEN_MODE_STAGGER_HDR) {
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_IMGID_TO_SIE, slvsec_info.sel_frm_id[0]);
		kdrv_rt |= kdrv_ssenif_set(IF_HDL, KDRV_SSENIF_SLVSEC_IMGID_TO_SIE2, slvsec_info.sel_frm_id[1]);
	}

	for (idx = 0; idx < SEN_SLVSEC_CTL_NUM; idx++) {
		if (init_cfg_obj.pin_cfg.sen_2_serial_pin_map[idx] != CTL_SEN_IGNORE) {
			for (i = 0; i < SEN_SLVSEC_CTL_NUM; i++) {
				if (init_cfg_obj.pin_cfg.sen_2_serial_pin_map[idx] == slvsec_info.output_pixel_order[i]) {
					kdrv_rt |= kdrv_ssenif_set(IF_HDL, cfgid_outorder[i], slvsec_lane_sel[idx]);
					slvsec_dl_mask |= slvsec_lane_sel[idx];
					valid_lane |= slvsec_lane_sel[slvsec_info.output_pixel_order[init_cfg_obj.pin_cfg.sen_2_serial_pin_map[idx]]];
					order_cnt++;
					break;
				}
			}
		}
	}

	if (sendrv_param_error) {
		rt = CTL_SEN_E_SENDRV_PARAM;
	} else if (kdrv_rt != 0) {
		CTL_SEN_DBG_ERR("id %d kdrv_rt %d error\r\n", id, kdrv_rt);
		rt = CTL_SEN_E_KDRV_SSENIF;
	}
	return rt;
}

static INT32 sen_ctrl_if_slvsec_set_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value)
{
#define IF_HDL g_ctl_sen_if_kdrv_hdl[id]

	INT32 kdrv_rt, rt = 0;

	if ((cfg_id & KDRV_SSENIF_SLVSEC_BASE) != KDRV_SSENIF_SLVSEC_BASE) {
		CTL_SEN_DBG_ERR("pls use %s as input cfg_id (input:0x%.8x)\r\n", CTL_SEN_CMD2STR(KDRV_SSENIF_SLVSEC_BASE), cfg_id);
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

static INT32 sen_ctrl_if_slvsec_get_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value)
{
#define IF_HDL g_ctl_sen_if_kdrv_hdl[id]

	INT32 kdrv_rt, rt = 0;

	if ((cfg_id & KDRV_SSENIF_SLVSEC_BASE) != KDRV_SSENIF_SLVSEC_BASE) {
		CTL_SEN_DBG_ERR("pls use %s as input cfg_id (input:0x%.8x)\r\n", CTL_SEN_CMD2STR(KDRV_SSENIF_SLVSEC_BASE), cfg_id);
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

static CTL_SEN_CTRL_IF_SLVSEC sen_ctrl_if_slvsec_tab = {

	sen_ctrl_if_slvsec_open,
	sen_ctrl_if_slvsec_close,

	sen_ctrl_if_slvsec_start,
	sen_ctrl_if_slvsec_stop,

	sen_ctrl_if_slvsec_wait_interrupt,

	sen_ctrl_if_slvsec_set_mode_cfg,

	sen_ctrl_if_slvsec_set_cfg,
	sen_ctrl_if_slvsec_get_cfg,
};

CTL_SEN_CTRL_IF_SLVSEC *sen_ctrl_if_slvsec_get_obj(void)
{
	return &sen_ctrl_if_slvsec_tab;
}

