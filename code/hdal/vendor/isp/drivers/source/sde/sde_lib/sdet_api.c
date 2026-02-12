#if defined(__FREERTOS)
#include "string.h"
#else
#include <linux/syscalls.h>
#include <linux/moduleparam.h>
#endif
#include "kwrap/error_no.h"
#include "kwrap/type.h"

#include "sde_dev_int.h"
#include "sde_lib.h"
#include "sde_api_int.h"
#include "sdet_api.h"
#include "sde_api.h"
#include "sde_dbg.h"
#include "sde_main.h"
#include "sde_version.h"

//=============================================================================
// extern include
//=============================================================================
extern SDE_IF_PARAM_PTR *sde_if_param[SDE_ID_MAX_NUM];

//=============================================================================
// global
//=============================================================================
typedef void (*sdet_fp)(UINT32 addr);

//=============================================================================
// function declaration
//=============================================================================
static void sdet_api_get_version(UINT32 addr);
static void sdet_api_get_size_tab(UINT32 addr);
static void sdet_api_get_io_info(UINT32 addr);
static void sdet_api_get_ctrl(UINT32 addr);
static void sdet_api_set_open(UINT32 addr);
static void sdet_api_set_close(UINT32 addr);
static void sdet_api_set_trigger(UINT32 addr);
static void sdet_api_set_io_info(UINT32 addr);
static void sdet_api_set_ctrl(UINT32 addr);
static void sdet_api_reserve(UINT32 addr);

#define RESERVE_SIZE 0

static SDET_INFO sdet_info = { {
	//id                             size
	{SDET_ITEM_VERSION,              sizeof(UINT32)                   },
	{SDET_ITEM_SIZE_TAB,             sizeof(SDET_INFO)                },
	{SDET_ITEM_RESERVE_02,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_03,           RESERVE_SIZE                     },
	{SDET_ITEM_OPEN,                 0                                },
	{SDET_ITEM_CLOSE,                0                                },
	{SDET_ITEM_TRIGGER,              sizeof(SDET_TRIG_INFO)           },
	{SDET_ITEM_RESERVE_07,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_08,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_09,           RESERVE_SIZE                     },
	{SDET_ITEM_IO_INFO,              sizeof(SDET_IO_INFO)             },
	{SDET_ITEM_CTRL_PARAM,           sizeof(SDET_CTRL_PARAM)          },
	{SDET_ITEM_RESERVE_12,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_13,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_14,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_15,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_16,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_17,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_18,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_19,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_20,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_21,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_22,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_23,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_24,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_25,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_26,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_27,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_28,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_29,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_30,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_31,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_32,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_33,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_34,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_35,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_36,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_37,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_38,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_39,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_40,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_41,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_42,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_43,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_44,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_45,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_46,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_47,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_48,           RESERVE_SIZE                     },
	{SDET_ITEM_RESERVE_49,           RESERVE_SIZE                     }
} };

static sdet_fp sdet_get_tab[SDET_ITEM_MAX] = {
	sdet_api_get_version,
	sdet_api_get_size_tab,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 5
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_get_io_info,          // 10
	sdet_api_get_ctrl,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 15
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 20
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 25
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 30
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 35
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 40
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 45
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve
};

static sdet_fp sdet_set_tab[SDET_ITEM_MAX] = {
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_set_open,
	sdet_api_set_close,            // 5
	sdet_api_set_trigger,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_set_io_info,          // 10
	sdet_api_set_ctrl,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 15
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 20
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 25
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 30
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 35
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 40
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,              // 45
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve,
	sdet_api_reserve
};

UINT32 sdet_api_get_item_size(SDET_ITEM item)
{
	return sdet_info.size_tab[item][1];
}

static void sdet_api_get_version(UINT32 addr)
{
	UINT32 *data = (UINT32 *)addr;

	*data = sde_get_version();
}

static void sdet_api_get_size_tab(UINT32 addr)
{
	memcpy((SDET_INFO *)addr, &sdet_info, sizeof(SDET_INFO));
}

static void sdet_api_get_io_info(UINT32 addr)
{
	SDET_IO_INFO *data = (SDET_IO_INFO *)addr;

	memcpy(&(data->io_info), sde_if_param[data->id]->io_info, sizeof(SDE_IF_IO_INFO));
}

static void sdet_api_get_ctrl(UINT32 addr)
{
	SDET_CTRL_PARAM *data = (SDET_CTRL_PARAM *)addr;

	memcpy(&(data->ctrl), sde_if_param[data->id]->ctrl, sizeof(SDE_IF_CTRL_PARAM));
}

ER sdet_api_get_cmd(SDET_ITEM item, UINT32 addr)
{
	if (sdet_get_tab[item] == NULL) {
		DBG_ERR("sdet_get_tab(%d) NULL!!\r\n", item);
		return E_SYS;
	}
	if ((item != SDET_ITEM_VERSION) && (item != SDET_ITEM_SIZE_TAB) && (*((UINT32 *)addr) >= SDE_ID_MAX_NUM)) {
		DBG_ERR("item(%d) id(%d) out of range\r\n", item, *((UINT32 *)addr));
		return E_SYS;
	}
	if (item >= SDET_ITEM_MAX) {
		DBG_ERR("item(%d) out of range\r\n", item);
		return E_SYS;
	}

	sdet_get_tab[item](addr);
	return E_OK;
}

static void sdet_api_set_open(UINT32 addr)
{
	ER rt = E_OK;
	SDE_OPENOBJ sde_drv_obj;

	if (sde_opened) {
		return;
	}

	sde_drv_obj.FP_SDEISR_CB = NULL;
	sde_drv_obj.uiSdeClockSel = 480;

	rt = sde_open(&sde_drv_obj);
	if (rt == E_OK) {
		sde_opened = TRUE;
	}
}

static void sdet_api_set_close(UINT32 addr)
{
	ER rt = E_OK;

	if (sde_opened == FALSE) {
		return;
	}

	rt = sde_close();
	if (rt == E_OK) {
		sde_opened = FALSE;
	}
}

static void sdet_api_set_trigger(UINT32 addr)
{
	SDET_TRIG_INFO *data = (SDET_TRIG_INFO *)addr;

	sde_trigger(data->id);
}

static void sdet_api_set_io_info(UINT32 addr)
{
	SDET_IO_INFO *data = (SDET_IO_INFO *)addr;
	int align_byte = 4;

	memcpy(sde_if_param[data->id]->io_info, &(data->io_info), sizeof(SDE_IF_IO_INFO));

	if (sde_if_param[data->id]->io_info->in0_lofs % 4 != 0) {
		sde_if_param[data->id]->io_info->in0_lofs = ALIGN_CEIL(sde_if_param[data->id]->io_info->in0_lofs, align_byte);
		DBG_DUMP("in0_lofs must be 4x, force %d to %d !!", data->io_info.in0_lofs, sde_if_param[data->id]->io_info->in0_lofs);
	}
	if (sde_if_param[data->id]->io_info->in1_lofs % 4 != 0) {
		sde_if_param[data->id]->io_info->in1_lofs = ALIGN_CEIL(sde_if_param[data->id]->io_info->in1_lofs, align_byte);
		DBG_DUMP("in1_lofs must be 4x, force %d to %d !!", data->io_info.in1_lofs, sde_if_param[data->id]->io_info->in1_lofs);
	}
	if (sde_if_param[data->id]->io_info->cost_lofs % 4 != 0) {
		sde_if_param[data->id]->io_info->cost_lofs = ALIGN_CEIL(sde_if_param[data->id]->io_info->cost_lofs, align_byte);
		DBG_DUMP("cost_lofs must be 4x, force %d to %d !!", data->io_info.cost_lofs, sde_if_param[data->id]->io_info->cost_lofs);
	}
	if (sde_if_param[data->id]->io_info->out0_lofs % 4 != 0) {
		sde_if_param[data->id]->io_info->out0_lofs = ALIGN_CEIL(sde_if_param[data->id]->io_info->out0_lofs, align_byte);
		DBG_DUMP("out0_lofs must be 4x, force %d to %d !!", data->io_info.out0_lofs, sde_if_param[data->id]->io_info->out0_lofs);
	}
	if (sde_if_param[data->id]->io_info->out1_lofs % 4 != 0) {
		sde_if_param[data->id]->io_info->out1_lofs = ALIGN_CEIL(sde_if_param[data->id]->io_info->out1_lofs, align_byte);
		DBG_DUMP("out1_lofs must be 4x, force %d to %d !!", data->io_info.out1_lofs, sde_if_param[data->id]->io_info->out1_lofs);
	}
}

static void sdet_api_set_ctrl(UINT32 addr)
{
	SDET_CTRL_PARAM *data = (SDET_CTRL_PARAM *)addr;

	memcpy(sde_if_param[data->id]->ctrl, &(data->ctrl), sizeof(SDE_IF_CTRL_PARAM));
}

static void sdet_api_reserve(UINT32 addr)
{
	return;
}

ER sdet_api_set_cmd(SDET_ITEM item, UINT32 addr)
{
	if (sdet_set_tab[item] == NULL) {
		DBG_ERR("sdet_set_tab(%d) NULL!!\r\n", item);
		return E_SYS;
	}
	if ((item != SDET_ITEM_OPEN) && (item != SDET_ITEM_CLOSE) && (*((UINT32 *)addr) >= SDE_ID_MAX_NUM)) {
		DBG_ERR("item(%d) id(%d) out of range\r\n", item, *((UINT32 *)addr));
		return E_SYS;
	}
	if (item >= SDET_ITEM_MAX) {
		DBG_ERR("item(%d) out of range\r\n", item);
		return E_SYS;
	}

	sdet_set_tab[item](addr);
	return E_OK;
}

