#if defined __KERNEL__
#include <linux/string.h>   // for memset
#else
#include <string.h>         // for memset
#endif
#include "kflow_common/nvtmpp.h"
#include "nvtmpp_module.h"
#include "nvtmpp_debug.h"


typedef struct {
	UINT32              module_count;
	NVTMPP_MODULE       module_list[NVTMPP_MODULE_MAX_NUM];
	UINT32              err_cnt[NVTMPP_MODULE_ER_MAX][NVTMPP_MODULE_MAX_NUM];
} NVTMPP_MODULE_INFO_S;

static NVTMPP_MODULE_INFO_S     g_nvtmpp_module_info;

void nvtmpp_vb_module_init(void)
{
	memset(&g_nvtmpp_module_info, 0x00, sizeof(g_nvtmpp_module_info));
}

NVTMPP_ER nvtmpp_vb_module_add_one(NVTMPP_MODULE module)
{
	NVTMPP_MODULE   *p_module;
	UINT32          i;

	// calculate module count
	for (i = 0; i < NVTMPP_MODULE_MAX_NUM; i++) {
		p_module = &g_nvtmpp_module_info.module_list[i];
		// module already added, just return ok
		if (*p_module == module) {
			goto add_module_success;
		}
		// find a empty entry and add it
		if (*p_module == 0) {
			*p_module = module;
			g_nvtmpp_module_info.module_count++;
			goto add_module_success;
		}
	}
	return NVTMPP_ER_2MODULES;
add_module_success:
	return NVTMPP_ER_OK;
}

UINT32 nvtmpp_vb_get_module_count(void)
{
	return g_nvtmpp_module_info.module_count;
}

NVTMPP_ER nvtmpp_vb_module_to_string(NVTMPP_MODULE module, char *buf, UINT32 buflen)
{
	if (buflen < sizeof(NVTMPP_MODULE) + 1)
		return NVTMPP_ER_PARM;
	buf[0] = (UINT8)module;
	buf[1] = (UINT8)(module >> 8);
	buf[2] = (UINT8)(module >> 16);
	buf[3] = (UINT8)(module >> 24);
	buf[4] = (UINT8)(module >> 32);
	buf[5] = (UINT8)(module >> 40);
	buf[6] = (UINT8)(module >> 48);
	buf[7] = (UINT8)(module >> 56);
	buf[8] = 0;
	return NVTMPP_ER_OK;
}

INT32 nvtmpp_vb_module_to_index(NVTMPP_MODULE module)
{
	UINT32 i;
	CHAR   module_str[sizeof(NVTMPP_MODULE) + 1];

	// normal case
	for (i = 0; i < g_nvtmpp_module_info.module_count; i++) {
		if (g_nvtmpp_module_info.module_list[i] == module) {
			return i;
		}
	}
	// error case
	nvtmpp_vb_module_to_string(module, module_str, sizeof(module_str));
	DBG_ERR("Unknow module %s\r\n", module_str);
	return -1;
}

NVTMPP_MODULE nvtmpp_vb_index_to_module(UINT32 module_id)
{
	if (module_id < g_nvtmpp_module_info.module_count)
		return g_nvtmpp_module_info.module_list[module_id];
	return 0;
}


INT32 nvtmpp_vb_get_all_modules_list_string(CHAR *buff, UINT32 buff_len, UINT32 *module_strlen_list, BOOL align_max_str)
{
	CHAR   module_str[sizeof(NVTMPP_MODULE) + 1];
	UINT32 i, total_string_len = 0, module_strlen;
	UINT32 padding_len, padding_i;

	for (i = 0; i < g_nvtmpp_module_info.module_count; i++) {
		if (total_string_len + sizeof(NVTMPP_MODULE) + 2 >= buff_len) {
			DBG_ERR("buff len %d too small\r\n", buff_len);
			buff[total_string_len] = 0;
			return -1;
		}
		nvtmpp_vb_module_to_string(g_nvtmpp_module_info.module_list[i], module_str, sizeof(module_str));
		module_strlen = strlen(module_str);
		if (TRUE == align_max_str) {
			*module_strlen_list++ = sizeof(NVTMPP_MODULE);
		} else {
			*module_strlen_list++ = module_strlen;
		}
		if (TRUE == align_max_str) {
			padding_len = sizeof(NVTMPP_MODULE) - module_strlen;
			for (padding_i = 0; padding_i < padding_len; padding_i++) {
				*buff++ = ' ';
			}
			total_string_len += padding_len;
		}
		memcpy(buff, module_str, module_strlen);
		buff += module_strlen;
		*buff++ = ' ';
		total_string_len += (module_strlen + 1);
	}
	*buff = 0;
	return 0;
}

void nvtmpp_vb_module_add_err_cnt(UINT32 module_id, NVTMPP_MODULE_ER err_type)
{
	if (err_type >= NVTMPP_MODULE_ER_MAX) {
		return;
	}
	if (module_id >= g_nvtmpp_module_info.module_count) {
		return;
	}
	g_nvtmpp_module_info.err_cnt[err_type][module_id]++;
}

UINT32 nvtmpp_vb_module_get_err_cnt(UINT32 module_id, NVTMPP_MODULE_ER err_type)
{
	if (err_type >= NVTMPP_MODULE_ER_MAX) {
		return 0;
	}
	if (module_id >= g_nvtmpp_module_info.module_count) {
		return 0;
	}
	return g_nvtmpp_module_info.err_cnt[err_type][module_id];
}





