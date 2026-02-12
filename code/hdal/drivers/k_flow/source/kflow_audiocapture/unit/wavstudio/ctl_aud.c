/**
    General API of ctl_audio

    This file is the header file that define the API and data type for ctl_audio.

    @file       ctl_aud.c
    @ingroup    mIDrvSensor
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#if defined(__FREERTOS) || defined __UITRON || defined __ECOS
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"
#include "kwrap/stdio.h"
#include "kflow_audiocapture/wavstudio_tsk.h"
#include "kflow_audiocapture/ctl_aud.h"
#include "ctl_aud_int.h"
#include "wavstudio_int.h"
#include "wavstudio_aud_intf.h"
#include <string.h>
#else
#include "kwrap/type.h"
#include "kwrap/platform.h"
#include "kflow_audiocapture/wavstudio_tsk.h"
#include "kflow_audiocapture/ctl_aud.h"
#include "ctl_aud_int.h"
#include "wavstudio_int.h"
#include "wavstudio_aud_intf.h"
#endif

#define CTL_AUD_NULL_STR  "NULL" // length must <= CTL_SEN_NAME_LEN

static CHAR aud_null_str[CTL_AUD_NAME_LEN] = CTL_AUD_NULL_STR;

AUD_MAP_TBL aud_map_tbl[CTL_AUD_DRV_MAX_NUM] = {
	{CTL_AUD_NULL_STR, 0},
	{CTL_AUD_NULL_STR, 0},
	{CTL_AUD_NULL_STR, 0},
	{CTL_AUD_NULL_STR, 0}
};

CTL_AUD_OBJ aud_obj[CTL_AUD_ID_MAX] = {0};

//static UINT32 ctl_aud_lock = 0;


static CTL_AUD_ID aud_module_get_map_tbl_id(CHAR *name)
{
	UINT32 i;

	for (i = 0; i < CTL_AUD_DRV_MAX_NUM; i++) {
		if (strcmp(aud_map_tbl[i].name, name) == 0) {
			return i;
		}
	}
	return CTL_AUD_DRV_MAX_NUM;
}

static INT32 aud_module_set_map_tbl_reg(CHAR *name, CTL_AUD_DRV_TAB *auddrv_reg_tab)
{
	UINT32 i, j;

	if (name == NULL) {
		DBG_ERR("name NULL\r\n");
		return E_SYS;
	}

	if (auddrv_reg_tab != NULL) {
		for (i = 0; i < CTL_AUD_DRV_MAX_NUM; i++) {
			if (strcmp(aud_map_tbl[i].name, name) == 0) {
				aud_map_tbl[i].drv_tab = auddrv_reg_tab;
				return E_OK;
			}
			if (strcmp(aud_map_tbl[i].name, aud_null_str) == 0) {
				for (j = 0; j < CTL_AUD_NAME_LEN; j++) {
					aud_map_tbl[i].name[j] = *(CHAR *)(name + j);
				}
				DBG_IND("Reg %s\r\n", aud_map_tbl[i].name);
				aud_map_tbl[i].drv_tab = auddrv_reg_tab;
				return E_OK;
			}
		}
	} else {
		for (i = 0; i < CTL_AUD_DRV_MAX_NUM; i++) {
			if (strcmp(aud_map_tbl[i].name, name) == 0) {
				aud_map_tbl[i].drv_tab = NULL;
				for (j = 0; j < CTL_AUD_NAME_LEN; j++) {
					aud_map_tbl[i].name[j] = aud_null_str[j];
				}
				return E_OK;
			}
		}
	}

	if (auddrv_reg_tab != NULL) {
		DBG_ERR("name %s reg drv fail\r\n", name);
	} else {
		DBG_ERR("name %s not reg drv\r\n", name);
	}
	return E_SYS;
}

#if 0
static void ctl_aud_cfg_pinmux(CTL_AUD_INIT_PIN_CFG *pin_cfg, BOOL b_en)
{
	PIN_GROUP_CONFIG pinmux_cfg[2];
	int ret = 0;
	UINT32 i;

	if (b_en == FALSE) {
		for (i = 0; i < CTL_AUD_ID_MAX; i++) {
			if (aud_obj[i].external == TRUE) {
				break;
			}
		}
		/*only disable I2S pinmux if both cap and out are using embedded codec*/
		if (i != CTL_AUD_ID_MAX) {
			return;
		}
	}

	memset((void *)&pinmux_cfg[0], 0, sizeof(PIN_GROUP_CONFIG)*2);

	pinmux_cfg[0].pin_function = PIN_FUNC_AUDIO;
	pinmux_cfg[1].pin_function = PIN_FUNC_I2C;
	//pinmux_cfg[2].pin_function = PIN_FUNC_UART;

	ret = nvt_pinmux_capture(pinmux_cfg, 2);

	if (b_en) {
		if (!((pinmux_cfg[0].config & pin_cfg->pinmux.audio_pinmux) && ((pinmux_cfg[1].config & pin_cfg->pinmux.cmd_if_pinmux) || (pin_cfg->pinmux.cmd_if_pinmux == 0)))) {
			//DBG_DUMP("Enable pinmux\r\n");
			pinmux_cfg[0].config |= pin_cfg->pinmux.audio_pinmux;
			pinmux_cfg[1].config |= pin_cfg->pinmux.cmd_if_pinmux;

			ret = nvt_pinmux_update(pinmux_cfg, 2);
		}
	} else {
		if (!(pinmux_cfg[0].config == PIN_AUDIO_CFG_NONE)) {
			//DBG_DUMP("Disable pinmux\r\n");
			pinmux_cfg[0].config = PIN_AUDIO_CFG_NONE;
			pinmux_cfg[1].config &= ~pin_cfg->pinmux.cmd_if_pinmux;

			ret = nvt_pinmux_update(pinmux_cfg, 2);
		}
	}

	if (ret) {
		DBG_WRN("pinmux update error!\r\n");
	}
}
#endif

static AUD_MAP_TBL *aud_module_get_map_tbl(CHAR *name)
{
	UINT32 i;

	if (name == NULL) {
		return NULL;
	}

	for (i = 0; i < CTL_AUD_ID_MAX; i++) {
		if (strcmp(aud_map_tbl[i].name, name) == 0) {
			if (aud_map_tbl[i].drv_tab == NULL) {
				DBG_ERR("drv %s not reg\r\n", name);
				return NULL;
			}
			return &aud_map_tbl[i];
		}
	}
	return NULL;
}


INT32 ctl_aud_reg_auddrv(CHAR *name, CTL_AUD_DRV_TAB *reg_tab)
{
	INT32 rt;
	CTL_AUD_ID id;

	id = aud_module_get_map_tbl_id(name);
	if (id != CTL_AUD_DRV_MAX_NUM) {
		return E_SYS;
	}

	rt = aud_module_set_map_tbl_reg(name, reg_tab);

	return rt;
}

INT32 ctl_aud_unreg_auddrv(CHAR *name)
{
	INT32 rt;

	rt = aud_module_set_map_tbl_reg(name, NULL);

	return rt;
}


INT32 ctl_aud_module_init_cfg(CTL_AUD_ID id, CHAR *name, CTL_AUD_INIT_CFG_OBJ *init_cfg_obj)
{
	INT32 rt = 0;
	CTL_AUD_OBJ *obj = &aud_obj[id];
	AUD_MAP_TBL *map_tbl;

	DBG_IND("id=%d, init %s\r\n", id, name);

	if (name[0] != 0) {
		map_tbl = aud_module_get_map_tbl(name);

		if (map_tbl != NULL) {
			obj->map_tbl = map_tbl;
			obj->init_cfg_obj = *init_cfg_obj;
			obj->external = TRUE;
			return rt;
		} else {
			obj->map_tbl = 0;
			DBG_ERR("Driver %s is not registered\r\n", name);
		}
	} else {
		memset((void *)obj, 0, sizeof(CTL_AUD_OBJ));
	}

	obj->external = FALSE;

	return rt;
}

INT32 ctl_aud_module_open(CTL_AUD_ID id)
{
	CTL_AUD_OBJ *obj = &aud_obj[id];
	UINT32 eid = 0;
	ER ret = 0;
	UINT32 param[3] = {0};

	if ((id != CTL_AUD_ID_CAP) && (id != CTL_AUD_ID_OUT)) {
		DBG_ERR("Invalid act = %d\r\n", id);
		return -1;
	}

	eid = (id == CTL_AUD_ID_CAP)? KDRV_DEV_ID(0, KDRV_AUDCAP_ENGINE0, 0) : KDRV_DEV_ID(0, KDRV_AUDOUT_ENGINE0, 0);

	//DBG_DUMP("open name = %s\r\n", aud_obj[id].map_tbl->name);
	if (obj->external == FALSE) {
		//ctl_aud_cfg_pinmux(&obj->init_cfg_obj.pin_cfg, FALSE);
		if (id == CTL_AUD_ID_CAP) {
			param[0] = KDRV_AUDIO_CAP_PATH_AMIC;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_PATH, (VOID*)&param[0]);
		}
		return -2;
	} else {
		if (id == CTL_AUD_ID_CAP) {
			param[0] = KDRV_AUDIO_CAP_PATH_I2S;
			kdrv_audioio_set(eid, KDRV_AUDIOIO_CAP_PATH, (VOID*)&param[0]);
		}
	}

	if (obj->map_tbl->drv_tab == NULL) {
		return 0;
	}


	/* cfg pinmux */
	//ctl_aud_cfg_pinmux(&obj->init_cfg_obj.pin_cfg, TRUE);

	/* cfg i2s */
	param[0] = obj->init_cfg_obj.i2s_cfg.bit_width;
	kdrv_audioio_set(eid, KDRV_AUDIOIO_GLOBAL_I2S_BIT_WIDTH, (VOID*)&param[0]);
	param[0] = obj->init_cfg_obj.i2s_cfg.bit_clk_ratio;
	kdrv_audioio_set(eid, KDRV_AUDIOIO_GLOBAL_I2S_BITCLK_RATIO, (VOID*)&param[0]);
	param[0] = obj->init_cfg_obj.i2s_cfg.op_mode;
	kdrv_audioio_set(eid, KDRV_AUDIOIO_GLOBAL_I2S_OPMODE, (VOID*)&param[0]);

	param[0] = KDRV_AUDIO_I2S_DATA_ORDER_TYPE1;
	kdrv_audioio_set(eid, KDRV_AUDIOIO_GLOBAL_I2S_DATA_ORDER, (VOID*)&param[0]);

	if (obj->map_tbl->drv_tab->set_cfg) {
		ret = obj->map_tbl->drv_tab->set_cfg(0, CTL_AUDDRV_CFGID_SET_CHANNEL, (void*)&obj->init_cfg_obj.i2s_cfg.tdm_ch);
	}

	/*open external driver*/
	if (obj->map_tbl->drv_tab->open) {
		ret = obj->map_tbl->drv_tab->open(id);
	}

	return ret;
}

INT32 ctl_aud_module_close(CTL_AUD_ID id)
{
	CTL_AUD_OBJ *obj;
	ER ret = 0;

	if ((id != CTL_AUD_ID_CAP) && (id != CTL_AUD_ID_OUT)) {
		DBG_ERR("Invalid act = %d\r\n", id);
		return -1;
	}

	obj = &aud_obj[id];

	if (obj->map_tbl == NULL) {
		return 0;
	}

	if (obj->map_tbl->drv_tab == NULL) {
		return 0;
	}

	/*close external driver*/
	if (obj->map_tbl->drv_tab->close) {
		ret = obj->map_tbl->drv_tab->close(id);
	}

	return ret;
}

INT32 ctl_aud_module_start(CTL_AUD_ID id)
{
	CTL_AUD_OBJ *obj;
	ER ret = 0;

	obj = &aud_obj[id];

	if (obj->map_tbl == NULL) {
		return 0;
	}

	if (obj->map_tbl->drv_tab == NULL) {
		return 0;
	}

	if (obj->map_tbl->drv_tab->start) {
		ret = obj->map_tbl->drv_tab->start(id);
	}

	return ret;
}

INT32 ctl_aud_module_stop(CTL_AUD_ID id)
{
	CTL_AUD_OBJ *obj;
	ER ret = 0;

	obj = &aud_obj[id];

	if (obj->map_tbl == NULL) {
		return 0;
	}

	if (obj->map_tbl->drv_tab == NULL) {
		return 0;
	}

	if (obj->map_tbl->drv_tab->stop) {
		ret = obj->map_tbl->drv_tab->stop(id);
	}

	return ret;
}

INT32 ctl_aud_module_set_cfg(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data)
{
	CTL_AUD_OBJ *obj;
	ER ret = 0;

	obj = &aud_obj[id];

	if (obj->map_tbl == NULL) {
		return 0;
	}

	if (obj->map_tbl->drv_tab == NULL) {
		return 0;
	}

	if (obj->map_tbl->drv_tab->set_cfg) {
		ret = obj->map_tbl->drv_tab->set_cfg(id, drv_cfg_id, data);
	}

	return ret;
}

INT32 ctl_aud_module_get_cfg(CTL_AUD_ID id, CTL_AUDDRV_CFGID drv_cfg_id, void *data)
{
	CTL_AUD_OBJ *obj;
	ER ret = 0;

	obj = &aud_obj[id];

	if (obj->map_tbl == NULL) {
		return 0;
	}

	if (obj->map_tbl->drv_tab == NULL) {
		return 0;
	}

	if (obj->map_tbl->drv_tab->get_cfg) {
		ret = obj->map_tbl->drv_tab->get_cfg(id, drv_cfg_id, data);
	}

	return ret;
}

