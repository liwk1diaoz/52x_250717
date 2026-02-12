/*
    Sensor Interface DAL library - ctrl cmd interface

    Sensor Interface DAL library.

    @file       sen_ctrl_cmd_if.c
    @ingroup
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "sen_ctrl_cmdif_int.h"
#if 0
/* CMDIF common API */
#endif
static INT32 sen_ctrl_cmdif_open(CTL_SEN_ID id)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;
	CTL_SEN_INIT_CFG_OBJ init_cfg_obj = g_ctl_sen_init_obj[id]->init_cfg_obj;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);

	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get sensor cmdif fail\r\n");
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	switch (attr_cmdif_info.type) {
	case CTL_SEN_CMDIF_TYPE_VX1:
		if (init_cfg_obj.cmd_if_cfg.vx1.en) {
			rt = sen_ctrl_cmdif_vx1_get_obj()->open(id);
		} else {
			CTL_SEN_DBG_ERR("pls enable CTL_SEN_INIT_CFG_OBJ vx1\r\n");
			rt = CTL_SEN_E_IN_PARAM;
		}
		break;
	case CTL_SEN_CMDIF_TYPE_UNKNOWN:
	case CTL_SEN_CMDIF_TYPE_SIF:
	case CTL_SEN_CMDIF_TYPE_I2C:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (id %d, type %d)\r\n", id, attr_cmdif_info.type);
	}
	return rt;
}

static INT32 sen_ctrl_cmdif_close(CTL_SEN_ID id)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);

	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get sensor cmdif fail\r\n");
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	switch (attr_cmdif_info.type) {
	case CTL_SEN_CMDIF_TYPE_VX1:
		rt = sen_ctrl_cmdif_vx1_get_obj()->close(id);
		break;
	case CTL_SEN_CMDIF_TYPE_UNKNOWN:
	case CTL_SEN_CMDIF_TYPE_SIF:
	case CTL_SEN_CMDIF_TYPE_I2C:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (id %d, type %d)\r\n", id, attr_cmdif_info.type);
	}
	return rt;
}

static BOOL sen_ctrl_cmdif_is_opened(CTL_SEN_ID id)
{
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;
	BOOL is_opened = FALSE;

	if (id >= CTL_SEN_ID_MAX_SUPPORT) {
		CTL_SEN_DBG_ERR("input id %d overflow\r\n", id);
		return FALSE;
	}

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);

	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get sensor cmdif fail\r\n");
		return FALSE;
	}

	switch (attr_cmdif_info.type) {
	case CTL_SEN_CMDIF_TYPE_VX1:
		if (g_ctl_sen_cmdif_kdrv_hdl[id] != 0) {
			is_opened = TRUE;
		} else {
			is_opened = FALSE;
		}
		break;
	case CTL_SEN_CMDIF_TYPE_UNKNOWN:
	case CTL_SEN_CMDIF_TYPE_SIF:
	case CTL_SEN_CMDIF_TYPE_I2C:
		break;
	default:
		break;
	}

	return is_opened;
}

static INT32 sen_ctrl_cmdif_start(CTL_SEN_ID id)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);

	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get sensor cmdif fail\r\n");
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	switch (attr_cmdif_info.type) {
	case CTL_SEN_CMDIF_TYPE_VX1:
		rt = sen_ctrl_cmdif_vx1_get_obj()->start(id);
		break;
	case CTL_SEN_CMDIF_TYPE_UNKNOWN:
	case CTL_SEN_CMDIF_TYPE_SIF:
	case CTL_SEN_CMDIF_TYPE_I2C:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (id %d, type %d)\r\n", id, attr_cmdif_info.type);
	}
	return rt;
}

static INT32 sen_ctrl_cmdif_stop(CTL_SEN_ID id)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);

	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get sensor cmdif fail\r\n");
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	switch (attr_cmdif_info.type) {
	case CTL_SEN_CMDIF_TYPE_VX1:
		rt = sen_ctrl_cmdif_vx1_get_obj()->stop(id);
		break;
	case CTL_SEN_CMDIF_TYPE_UNKNOWN:
	case CTL_SEN_CMDIF_TYPE_SIF:
	case CTL_SEN_CMDIF_TYPE_I2C:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (id %d, type %d)\r\n", id, attr_cmdif_info.type);
	}
	return rt;
}

static INT32 sen_ctrl_cmdif_set_mode_cfg(CTL_SEN_ID id, CTL_SEN_MODE mode, CTL_SEN_OUTPUT_DEST output_dest)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);

	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get sensor cmdif fail\r\n");
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	switch (attr_cmdif_info.type) {
	case CTL_SEN_CMDIF_TYPE_VX1:
		rt = sen_ctrl_cmdif_vx1_get_obj()->set_mode_cfg(id, mode, output_dest);
		break;
	case CTL_SEN_CMDIF_TYPE_UNKNOWN:
	case CTL_SEN_CMDIF_TYPE_SIF:
	case CTL_SEN_CMDIF_TYPE_I2C:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (id %d, type %d)\r\n", id, attr_cmdif_info.type);
	}

	return rt;
}

static CTL_SEN_INTE sen_ctrl_cmdif_wait_interrupt(CTL_SEN_ID id, CTL_SEN_INTE waited_flag)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;
	CTL_SEN_INTE inte = CTL_SEN_INTE_NONE;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);

	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get sensor cmdif fail\r\n");
		return inte;
	}

	switch (attr_cmdif_info.type) {
	case CTL_SEN_CMDIF_TYPE_VX1:
		inte = sen_ctrl_cmdif_vx1_get_obj()->wait_interrupt(id, waited_flag);
		break;
	case CTL_SEN_CMDIF_TYPE_UNKNOWN:
	case CTL_SEN_CMDIF_TYPE_SIF:
	case CTL_SEN_CMDIF_TYPE_I2C:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (id %d, type %d)\r\n", id, attr_cmdif_info.type);
	}
	return inte;
}

static INT32 sen_ctrl_cmdif_set_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 value)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);

	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get sensor cmdif fail\r\n");
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	switch (attr_cmdif_info.type) {
	case CTL_SEN_CMDIF_TYPE_VX1:
		rt = sen_ctrl_cmdif_vx1_get_obj()->set_cfg(id, cfg_id, value);
		break;
	case CTL_SEN_CMDIF_TYPE_UNKNOWN:
	case CTL_SEN_CMDIF_TYPE_SIF:
	case CTL_SEN_CMDIF_TYPE_I2C:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (id %d, type %d)\r\n", id, attr_cmdif_info.type);
	}
	return rt;
}

static INT32 sen_ctrl_cmdif_get_cfg(CTL_SEN_ID id, UINT32 cfg_id, UINT32 *value)
{
	INT32 rt = 0;
	ER rt_er;
	CTL_SENDRV_GET_ATTR_CMDIF_PARAM attr_cmdif_info;

	memset((void *)&attr_cmdif_info, 0, sizeof(CTL_SENDRV_GET_ATTR_CMDIF_PARAM));
	rt_er = sendrv_get(id, CTL_SENDRV_CFGID_GET_ATTR_CMDIF, &attr_cmdif_info);

	if (rt_er != E_OK) {
		CTL_SEN_DBG_ERR("get sensor cmdif fail\r\n");
		return CTL_SEN_E_SENDRV_GET_FAIL;
	}

	switch (attr_cmdif_info.type) {
	case CTL_SEN_CMDIF_TYPE_VX1:
		rt = sen_ctrl_cmdif_vx1_get_obj()->get_cfg(id, cfg_id, value);
		break;
	case CTL_SEN_CMDIF_TYPE_UNKNOWN:
	case CTL_SEN_CMDIF_TYPE_SIF:
	case CTL_SEN_CMDIF_TYPE_I2C:
		break;
	default:
		rt = CTL_SEN_E_SENDRV_PARAM;
		break;
	}
	if (rt != CTL_SEN_E_OK) {
		CTL_SEN_DBG_ERR("fail (id %d, type %d)\r\n", id, attr_cmdif_info.type);
	}

	return E_OK;
}

#if 0
/* extern API */
#endif
static CTL_SEN_CTRL_CMDIF sen_ctrl_cmdif_tab = {
	sen_ctrl_cmdif_open,
	sen_ctrl_cmdif_close,
	sen_ctrl_cmdif_is_opened,
	sen_ctrl_cmdif_start,
	sen_ctrl_cmdif_stop,
	sen_ctrl_cmdif_wait_interrupt,
	sen_ctrl_cmdif_set_mode_cfg,
	sen_ctrl_cmdif_set_cfg,
	sen_ctrl_cmdif_get_cfg,
};

CTL_SEN_CTRL_CMDIF *sen_ctrl_cmdif_get_obj(void)
{
	return &sen_ctrl_cmdif_tab;
}

