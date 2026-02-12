/*
    Sensor Interface DAL library - ctrl sensor interface

    Sensor Interface DAL library.

    @file       sen_ctrl_if.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "sen_ctrl_if_int.h"

UINT32 g_ctl_sen_if_kdrv_hdl[CTL_SEN_ID_MAX_SUPPORT] = {0};
UINT32 g_ctl_sen_if_open_chk[CTL_SEN_ID_MAX_SUPPORT] = {0}; // set when open, clr when stop, avoid disable after open [IVOT_N02003_CO-21][fast-boot]

static INT32 start_tge(CTL_SEN_ID id)
{
	INT32 rt = 0;
	UINT32 i, tge_sync;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	tge_sync = g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync;

	if (g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync == 0) {
		rt = sen_ctrl_if_tge_get_obj()->start(id);
	} else {
		// set start flg
		g_ctl_sen_ctrl_obj[id]->tge_sync_obj.tge_start_flg = TRUE;
		for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
			if (!sen_ctrl_chk_add_ctx(i)) {
				continue;
			}
			if (g_ctl_sen_ctrl_obj[i]->tge_sync_obj.tge_start_flg) {
				tge_sync &= ~(1 << i);
			}
		}

		if (tge_sync == 0) {
			// start tge
			rt = sen_ctrl_if_tge_get_obj()->reset(id, g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync, TRUE);
			// reset start flg
			for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
				if (!sen_ctrl_chk_add_ctx(i)) {
					continue;
				}
				if ((int)(g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync & (1 << i)) == (int)(1 << i)) {
					g_ctl_sen_ctrl_obj[i]->tge_sync_obj.tge_start_flg = FALSE;
				}
			}
		}
	}
	return rt;
}

static INT32 stop_tge(CTL_SEN_ID id)
{
	INT32 rt = 0;
	UINT32 i, tge_sync;
	BOOL b_stop = FALSE;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	tge_sync = g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync;

	if (g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync == 0) {
		rt = sen_ctrl_if_tge_get_obj()->stop(id);
	} else {
		// chk tge is stop
		for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
			if (!sen_ctrl_chk_add_ctx(i)) {
				continue;
			}
			if ((int)(g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync & (1 << i)) == (int)(1 << i)) {
				if (g_ctl_sen_ctrl_obj[i]->tge_sync_obj.tge_stop_flg) {
					// tge already stop
					b_stop = TRUE;
				}
			}
		}

		// stop tge
		if (b_stop == FALSE) {
			rt = sen_ctrl_if_tge_get_obj()->reset(id, g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync, FALSE);
		}

		// set stop flg
		g_ctl_sen_ctrl_obj[id]->tge_sync_obj.tge_stop_flg = TRUE;

		// chk all sync_bit stop flg
		for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
			if (!sen_ctrl_chk_add_ctx(i)) {
				continue;
			}
			if (g_ctl_sen_ctrl_obj[i]->tge_sync_obj.tge_stop_flg) {
				tge_sync &= ~(1 << i);
			}
		}

		// reset stop flg
		if (tge_sync == 0) {
			for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
				if (!sen_ctrl_chk_add_ctx(i)) {
					continue;
				}
				if ((int)(g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync & (1 << i)) == (int)(1 << i)) {
					g_ctl_sen_ctrl_obj[i]->tge_sync_obj.tge_stop_flg = FALSE;
				}
			}
		}
	}
	return rt;
}

static UINT32 chk_stop_if(CTL_SEN_ID id)
{
	UINT32 stop_bit = 0;
	UINT32 i, tge_sync;
	BOOL b_stop = FALSE;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return 0;
	}

	tge_sync = g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync;

	if (g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync == 0) {
		stop_bit = 1 << id;
	} else {
		// chk if is stop
		for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
			if (!sen_ctrl_chk_add_ctx(i)) {
				continue;
			}
			if ((int)(g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync & (1 << i)) == (int)(1 << i)) {
				if (g_ctl_sen_ctrl_obj[i]->tge_sync_obj.if_stop_flg) {
					// if already stop
					b_stop = TRUE;
				}
			}
		}

		// set if stop bit
		if (b_stop == FALSE) {
			stop_bit = g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync;
		}

		// set stop flg
		g_ctl_sen_ctrl_obj[id]->tge_sync_obj.if_stop_flg = TRUE;

		// chk all sync_bit stop flg
		for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
			if (!sen_ctrl_chk_add_ctx(i)) {
				continue;
			}
			if (g_ctl_sen_ctrl_obj[i]->tge_sync_obj.if_stop_flg) {
				tge_sync &= ~(1 << i);
			}
		}

		// reset stop flg
		if (tge_sync == 0) {
			for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
				if (!sen_ctrl_chk_add_ctx(i)) {
					continue;
				}
				if ((int)(g_ctl_sen_init_obj[id]->init_ext_obj.if_info.tge_sync & (1 << i)) == (int)(1 << i)) {
					g_ctl_sen_ctrl_obj[i]->tge_sync_obj.if_stop_flg = FALSE;
				}
			}
		}
	}
	return stop_bit;
}


static INT32 sen_ctrl_if_open(CTL_SEN_ID id)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_SIGNAL_PARAM signal_info;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);
	if (rt_er == E_OK) {
		if (attr_cmdif_info.type == CTL_SEN_CMDIF_TYPE_VX1) {
			return CTL_SEN_E_OK;
		}
	}

	if (g_ctl_sen_if_kdrv_hdl[id] != 0) {
		CTL_SEN_DBG_IND("id %d is opened\r\n", id);
		return CTL_SEN_E_OK;
	}

	switch (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type) {
	case CTL_SEN_IF_TYPE_MIPI:
		rt = sen_ctrl_if_csi_get_obj()->open(id);
		g_ctl_sen_if_open_chk[id] = TRUE;
		break;
	case CTL_SEN_IF_TYPE_LVDS:
		g_ctl_sen_if_open_chk[id] = TRUE;
		rt = sen_ctrl_if_lvds_get_obj()->open(id);
		break;
	case CTL_SEN_IF_TYPE_SLVSEC:
		g_ctl_sen_if_open_chk[id] = TRUE;
		rt = sen_ctrl_if_slvsec_get_obj()->open(id);
		break;
	case CTL_SEN_IF_TYPE_SIEPATGEN:
	case CTL_SEN_IF_TYPE_PARALLEL:
	case CTL_SEN_IF_TYPE_DUMMY:
		g_ctl_sen_if_kdrv_hdl[id] = TRUE;
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (type %d)\r\n", g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type);
		return rt;
	}


	/* cfg TGE engein*/
	if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en) {
		rt = sen_ctrl_if_tge_get_obj()->open(id);
	} else {
		/* check sensor is master sensor */
		memset((void *)&signal_info, 0, sizeof(CTL_SENDRV_GET_ATTR_SIGNAL_PARAM));
		rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_SIGNAL, &signal_info);
		if (rt_er != E_OK) {
			rt = CTL_SEN_E_SENDRV_GET_FAIL;
		} else {
			if (signal_info.type == CTL_SEN_SIGNAL_SLAVE) {
				CTL_SEN_DBG_ERR("slave sensor (CTL_SEN_SIGNAL_SLAVE) need to enable tge engine\r\n");
				rt = CTL_SEN_E_IN_PARAM;
			}
		}
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("tge fail\r\n");
		return rt;
	}

	return rt;
}

static INT32 sen_ctrl_if_close(CTL_SEN_ID id)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);
	if (rt_er == E_OK) {
		if (attr_cmdif_info.type == CTL_SEN_CMDIF_TYPE_VX1) {
			return CTL_SEN_E_OK;
		}
	}

	switch (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type) {
	case CTL_SEN_IF_TYPE_MIPI:
		rt = sen_ctrl_if_csi_get_obj()->close(id);
		break;
	case CTL_SEN_IF_TYPE_LVDS:
		rt = sen_ctrl_if_lvds_get_obj()->close(id);
		break;
	case CTL_SEN_IF_TYPE_SLVSEC:
		rt = sen_ctrl_if_slvsec_get_obj()->close(id);
		break;
	case CTL_SEN_IF_TYPE_SIEPATGEN:
	case CTL_SEN_IF_TYPE_PARALLEL:
	case CTL_SEN_IF_TYPE_DUMMY:
		g_ctl_sen_if_kdrv_hdl[id] = 0;
		break;
	default:
		rt += CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (type %d)\r\n", g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type);
		return rt;
	}

	if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en) {
		rt = sen_ctrl_if_tge_get_obj()->close(id);
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("tge fail\r\n");
		return rt;
	}

	return rt;
}

static BOOL sen_ctrl_if_is_opend(CTL_SEN_ID id)
{
	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return FALSE;
	}

	if (g_ctl_sen_if_kdrv_hdl[id] != 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static INT32 sen_ctrl_if_start(CTL_SEN_ID id, BOOL b_sen_chgmode)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;
	CTL_SEN_IF_TYPE if_type = g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);
	if (rt_er == E_OK) {
		if (attr_cmdif_info.type == CTL_SEN_CMDIF_TYPE_VX1) {
			return CTL_SEN_E_OK;
		}
	}

	if (g_ctl_sen_if_kdrv_hdl[id] == 0) {
		CTL_SEN_DBG_ERR("id %d not open\r\n", id);
		return CTL_SEN_E_SYS;
	}

	switch (if_type) {
	case CTL_SEN_IF_TYPE_MIPI:
		if (!b_sen_chgmode) {
			if (sen_uti_get_sendrv_en_mipi(id)) {
				// control by sendrv
			} else {
				rt = sen_ctrl_if_csi_get_obj()->start(id);
			}
		}
		break;
	case CTL_SEN_IF_TYPE_LVDS:
		if (b_sen_chgmode) {
			rt = sen_ctrl_if_lvds_get_obj()->start(id);
		}
		break;
	case CTL_SEN_IF_TYPE_SLVSEC:
		if (!b_sen_chgmode) {
			rt = sen_ctrl_if_slvsec_get_obj()->start(id);
		}
		break;
	case CTL_SEN_IF_TYPE_SIEPATGEN:
	case CTL_SEN_IF_TYPE_PARALLEL:
	case CTL_SEN_IF_TYPE_DUMMY:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (type %d)\r\n", g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type);
		return rt;
	}

	if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en) {
		if (b_sen_chgmode) {
			rt = start_tge(id);
		}
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("tge fail\r\n");
		return rt;
	}

	return rt;
}

static INT32 sen_ctrl_if_stop(CTL_SEN_ID id)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;
	UINT32 i, stop_if_bit = chk_stop_if(id);

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	for (i = 0; i < CTL_SEN_ID_MAX_SUPPORT; i++) {
		if (stop_if_bit == 0) {
			break;
		}
		if (!sen_ctrl_chk_add_ctx(i)) {
			continue;
		}
		if ((int)(stop_if_bit & (1 << i)) == (int)(1 << i)) {
			rt_er = sendrv_get(i, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);
			if (rt_er == E_OK) {
				if (attr_cmdif_info.type == CTL_SEN_CMDIF_TYPE_VX1) {
					//return E_OK;
					continue;
				}
			}

			if (g_ctl_sen_if_kdrv_hdl[i] == 0) {
				CTL_SEN_DBG_ERR("id %d not open\r\n", i);
				return CTL_SEN_E_SYS;
			}

			switch (g_ctl_sen_init_obj[i]->init_cfg_obj.if_cfg.type) {
			case CTL_SEN_IF_TYPE_MIPI:
				if (g_ctl_sen_if_open_chk[id]) {
					g_ctl_sen_if_open_chk[id] = FALSE;
				} else {
					if (sen_uti_get_sendrv_en_mipi(id)) {
						// control by sendrv
					} else {
						rt = sen_ctrl_if_csi_get_obj()->stop(i);
					}
				}
				break;
			case CTL_SEN_IF_TYPE_LVDS:
				if (g_ctl_sen_if_open_chk[id]) {
					g_ctl_sen_if_open_chk[id] = FALSE;
				} else {
					rt = sen_ctrl_if_lvds_get_obj()->stop(i);
				}
				break;
			case CTL_SEN_IF_TYPE_SLVSEC:
				if (g_ctl_sen_if_open_chk[id]) {
					g_ctl_sen_if_open_chk[id] = FALSE;
				} else {
					rt = sen_ctrl_if_slvsec_get_obj()->stop(i);
				}
				break;
			case CTL_SEN_IF_TYPE_SIEPATGEN:
			case CTL_SEN_IF_TYPE_PARALLEL:
			case CTL_SEN_IF_TYPE_DUMMY:
				break;
			default:
				rt = CTL_SEN_E_SENDRV_PARAM;
				break;
			}
			if (rt != E_OK) {
				CTL_SEN_DBG_ERR("fail (type %d)\r\n", g_ctl_sen_init_obj[i]->init_cfg_obj.if_cfg.type);
				return rt;
			}
		}
	}

	if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en) {
		rt = stop_tge(id);
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("tge fail\r\n");
		return rt;
	}

	return rt;
}

static CTL_SEN_INTE sen_ctrl_if_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag)
{
	ER rt_er;
	CTL_SEN_INTE inte_rt = CTL_SEN_INTE_NONE;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);
	if (rt_er == E_OK) {
		if (attr_cmdif_info.type == CTL_SEN_CMDIF_TYPE_VX1) {
			return E_OK;
		}
	}

	if (g_ctl_sen_if_kdrv_hdl[id] == 0) {
		CTL_SEN_DBG_ERR("id %d not open\r\n", id);
		return CTL_SEN_E_SYS;
	}

	if ((waited_flag & CTL_SEN_TGE_TAG) == CTL_SEN_TGE_TAG) {
		if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en) {
			inte_rt = sen_ctrl_if_tge_get_obj()->wait_interrupt(id, waited_flag);
		}
	} else {
		switch (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type) {
		case CTL_SEN_IF_TYPE_MIPI:
			inte_rt = sen_ctrl_if_csi_get_obj()->wait_interrupt(id, waited_flag);
			break;
		case CTL_SEN_IF_TYPE_LVDS:
			inte_rt = sen_ctrl_if_lvds_get_obj()->wait_interrupt(id, waited_flag);
			break;
		case CTL_SEN_IF_TYPE_SLVSEC:
			inte_rt = sen_ctrl_if_slvsec_get_obj()->wait_interrupt(id, waited_flag);
			break;
		case CTL_SEN_IF_TYPE_SIEPATGEN:
		case CTL_SEN_IF_TYPE_PARALLEL:
		case CTL_SEN_IF_TYPE_DUMMY:
			CTL_SEN_DBG_ERR("N.S. (type %d)\r\n", g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type);
			break;
		default:
			CTL_SEN_DBG_ERR("error (type %d)\r\n", g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type);
			break;
		}
	}

	return inte_rt;
}

static INT32 sen_ctrl_if_set_mode_cfg(CTL_SEN_ID id, CTL_SEN_MODE mode, CTL_SEN_OUTPUT_DEST output_dest)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);
	if (rt_er == E_OK) {
		if (attr_cmdif_info.type == CTL_SEN_CMDIF_TYPE_VX1) {
			return E_OK;
		}
	}

	if (g_ctl_sen_if_kdrv_hdl[id] == 0) {
		CTL_SEN_DBG_ERR("id %d not open\r\n", id);
		return CTL_SEN_E_SYS;
	}

	switch (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type) {
	case CTL_SEN_IF_TYPE_MIPI:
		rt = sen_ctrl_if_csi_get_obj()->set_mode_cfg(id, mode, output_dest);
		break;
	case CTL_SEN_IF_TYPE_LVDS:
		rt = sen_ctrl_if_lvds_get_obj()->set_mode_cfg(id, mode, output_dest);
		break;
	case CTL_SEN_IF_TYPE_SLVSEC:
		rt = sen_ctrl_if_slvsec_get_obj()->set_mode_cfg(id, mode);
		break;
	case CTL_SEN_IF_TYPE_SIEPATGEN:
	case CTL_SEN_IF_TYPE_PARALLEL:
	case CTL_SEN_IF_TYPE_DUMMY:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != E_OK) {
		CTL_SEN_DBG_ERR("fail (type %d)\r\n", g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type);
		return rt;
	}

	if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en) {
		rt = sen_ctrl_if_tge_get_obj()->set_mode_cfg(id, mode);
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("tge fail\r\n");
		return rt;
	}

	return rt;
}

static INT32 sen_ctrl_if_set_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value)
{
	INT32 rt = 0;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	if (g_ctl_sen_if_kdrv_hdl[id] == 0) {
		CTL_SEN_DBG_ERR("id %d not open\r\n", id);
		return CTL_SEN_E_SYS;
	}

	switch (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type) {
	case CTL_SEN_IF_TYPE_MIPI:
		rt = sen_ctrl_if_csi_get_obj()->set_cfg(id, cfg_id, value);
		break;
	case CTL_SEN_IF_TYPE_LVDS:
		rt = sen_ctrl_if_lvds_get_obj()->set_cfg(id, cfg_id, value);
		break;
	case CTL_SEN_IF_TYPE_SLVSEC:
		rt = sen_ctrl_if_slvsec_get_obj()->set_cfg(id, cfg_id, value);
		break;
	case CTL_SEN_IF_TYPE_SIEPATGEN:
	case CTL_SEN_IF_TYPE_PARALLEL:
	case CTL_SEN_IF_TYPE_DUMMY:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != E_OK) {
		CTL_SEN_DBG_ERR("fail (type %d)\r\n", g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type);
	}
	return rt;
}

static INT32 sen_ctrl_if_get_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value)
{
	INT32 rt = 0;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	if (g_ctl_sen_if_kdrv_hdl[id] == 0) {
		CTL_SEN_DBG_ERR("id %d not open\r\n", id);
		return E_OBJ;
	}

	switch (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type) {
	case CTL_SEN_IF_TYPE_MIPI:
		rt = sen_ctrl_if_csi_get_obj()->get_cfg(id, cfg_id, value);
		break;
	case CTL_SEN_IF_TYPE_LVDS:
		rt = sen_ctrl_if_lvds_get_obj()->get_cfg(id, cfg_id, value);
		break;
	case CTL_SEN_IF_TYPE_SLVSEC:
		rt = sen_ctrl_if_slvsec_get_obj()->get_cfg(id, cfg_id, value);
		break;
	case CTL_SEN_IF_TYPE_SIEPATGEN:
	case CTL_SEN_IF_TYPE_PARALLEL:
	case CTL_SEN_IF_TYPE_DUMMY:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != E_OK) {
		CTL_SEN_DBG_ERR("fail (type %d)\r\n", g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type);
	}
	return rt;
}

static INT32 sen_ctrl_if_set_cfg_tge(CTL_SEN_ID id, UINT32 cfg_id, void *value)
{
	INT32 rt = 0;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en) {
		rt = sen_ctrl_if_tge_get_obj()->set_cfg(id, cfg_id, value);
	} else {
		CTL_SEN_DBG_ERR("tge not enable\r\n");
		rt = CTL_SEN_E_IN_PARAM;
	}
	return rt;
}

static INT32 sen_ctrl_if_get_cfg_tge(CTL_SEN_ID id, UINT32 cfg_id, void *value)
{
	INT32 rt = 0;

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.tge.tge_en) {
		rt = sen_ctrl_if_tge_get_obj()->get_cfg(id, cfg_id, value);
	} else {
		CTL_SEN_DBG_ERR("tge not enable\r\n");
		rt = CTL_SEN_E_IN_PARAM;
	}
	return rt;
}

static INT32 sen_ctrl_if_set_cfg_kflow(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value)
{
	return 0;
}

static INT32 sen_ctrl_if_get_cfg_kflow(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value)
{
	INT32 rt = 0;
	KDRV_SSENIF_PARAM_ID cfg_id_kdrv[CTL_SEN_IF_TYPE_DUMMY] = {0};

	if (!sen_ctrl_chk_add_ctx(id)) {
		CTL_SEN_DBG_ERR("input id %d  not init_cfg\r\n", id);
		return CTL_SEN_E_ID_OVFL;
	}

	if (g_ctl_sen_if_kdrv_hdl[id] == 0) {
		CTL_SEN_DBG_ERR("id %d not open\r\n", id);
		return CTL_SEN_E_SYS;
	}

	if (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type > CTL_SEN_IF_TYPE_DUMMY) {
		rt = CTL_SEN_E_IN_PARAM;
		goto end;
	}

	if (cfg_id == CTL_SEN_CFGID_GET_VD_CNT) {
		cfg_id_kdrv[CTL_SEN_IF_TYPE_MIPI] = KDRV_SSENIF_CSI_SENSOR_FS_CNT;
		cfg_id_kdrv[CTL_SEN_IF_TYPE_LVDS] = KDRV_SSENIF_LVDS_SENSOR_VD_CNT;
	}

	if (cfg_id == CTL_SEN_CFGID_GET_SSENIF_ERR_CNT) {
		cfg_id_kdrv[CTL_SEN_IF_TYPE_MIPI] = KDRV_SSENIF_CSI_SENSOR_ERR_CNT;
//		cfg_id_kdrv[CTL_SEN_IF_TYPE_LVDS] = KDRV_SSENIF_LVDS_SENSOR_ERR_CNT; // wait LVDS KDRV ready
	}

	if (cfg_id == CTL_SEN_CFGID_GET_FRAME_NUMBER) {
		cfg_id_kdrv[CTL_SEN_IF_TYPE_MIPI] = KDRV_SSENIF_CSI_FRAME_NUMBER;
	}

	if (cfg_id_kdrv[g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type] == 0) {
		rt = CTL_SEN_E_IN_PARAM;
		goto end;
	}

	switch (g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type) {
	case CTL_SEN_IF_TYPE_MIPI:
		rt = sen_ctrl_if_csi_get_obj()->get_cfg(id, cfg_id_kdrv[CTL_SEN_IF_TYPE_MIPI], value);
		break;
	case CTL_SEN_IF_TYPE_LVDS:
		rt = sen_ctrl_if_lvds_get_obj()->get_cfg(id, cfg_id_kdrv[CTL_SEN_IF_TYPE_LVDS], value);
		break;
	case CTL_SEN_IF_TYPE_SLVSEC:
		rt = sen_ctrl_if_slvsec_get_obj()->get_cfg(id, cfg_id_kdrv[CTL_SEN_IF_TYPE_SLVSEC], value);
		break;
	case CTL_SEN_IF_TYPE_SIEPATGEN:
	case CTL_SEN_IF_TYPE_PARALLEL:
	case CTL_SEN_IF_TYPE_DUMMY:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}

end:
	if (rt != E_OK) {
		CTL_SEN_DBG_ERR("fail (type %d) cfg_id 0x%.8x\r\n", g_ctl_sen_init_obj[id]->init_cfg_obj.if_cfg.type, (unsigned int)cfg_id);
	}
	return rt;
}


static CTL_SEN_CTRL_IF sen_ctrl_if_tab = {
	sen_ctrl_if_open,
	sen_ctrl_if_close,
	sen_ctrl_if_is_opend,

	sen_ctrl_if_start,
	sen_ctrl_if_stop,

	sen_ctrl_if_wait_interrupt,

	sen_ctrl_if_set_mode_cfg,

	sen_ctrl_if_set_cfg,
	sen_ctrl_if_get_cfg,

	sen_ctrl_if_set_cfg_tge,
	sen_ctrl_if_get_cfg_tge,

	sen_ctrl_if_set_cfg_kflow,
	sen_ctrl_if_get_cfg_kflow,
};

CTL_SEN_CTRL_IF *sen_ctrl_if_get_obj(void)
{
	return &sen_ctrl_if_tab;
}

