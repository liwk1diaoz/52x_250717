/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_debug_get.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "hd_debug_int.h"

static HD_RESULT hd_debug_get_err_mask_p(void *p_data)
{
	*(unsigned int *)p_data = g_hd_mask_err;
	return HD_OK;
}

static HD_RESULT hd_debug_get_wrn_mask_p(void *p_data)
{
	*(unsigned int *)p_data = g_hd_mask_wrn;
	return HD_OK;
}

static HD_RESULT hd_debug_get_ind_mask_p(void *p_data)
{
	*(unsigned int *)p_data = g_hd_mask_ind;
	return HD_OK;
}

static HD_RESULT hd_debug_get_msg_mask_p(void *p_data)
{
	*(unsigned int *)p_data = g_hd_mask_msg;
	return HD_OK;
}

static HD_RESULT hd_debug_get_func_mask_p(void *p_data)
{
	*(unsigned int *)p_data = g_hd_mask_func;
	return HD_OK;
}

static HD_DBG_CMD_DESC cmd_tbl[] = {
	{HD_DEBUG_PARAM_ERR_MASK, hd_debug_get_err_mask_p},
	{HD_DEBUG_PARAM_WRN_MASK, hd_debug_get_wrn_mask_p},
	{HD_DEBUG_PARAM_IND_MASK, hd_debug_get_ind_mask_p},
	{HD_DEBUG_PARAM_MSG_MASK, hd_debug_get_msg_mask_p},
	{HD_DEBUG_PARAM_FUNC_MASK, hd_debug_get_func_mask_p},
};

HD_RESULT hd_debug_get(HD_DEBUG_PARAM_ID idx, void *p_data)
{
	int i, n = sizeof(cmd_tbl) / sizeof(cmd_tbl[0]);
	for (i = 0; i < n; i++) {
		if (cmd_tbl[i].idx == idx) {
			return cmd_tbl[i].p_func(p_data);
		}
	}
	DBG_ERR("not support id = %d\r\n", idx);
	return HD_ERR_NOT_SUPPORT;
}

