/**
	@brief Source file of vendor net flow sample.

	@file net_flow_sample.c

	@ingroup net_flow_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/

#include "kwrap/type.h"
#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/cpu.h"  //for vos_cpu_dcache_sync()
#include "kwrap/util.h"

#include "ai_ioctl.h"
#include "kdrv_ai.h"
#include "nue2_lmt.h"
#include "kflow_ai_version.h"

#include "kflow_ai_net/kflow_ai_net_platform.h"
#include "kflow_ai_net/nn_net.h"
#include "kflow_ai_net/nn_parm.h"
#include "kflow_ai_net/nn_diff.h"
#include "kflow_ai_net/kflow_ai_net_comm.h"
#include "kflow_ai_net_int.h"
#include "linux/kflow_ai_net_proc.h"
//=============================================================
#define __CLASS__ 				"[ai][kflow][flow]"
#include "kflow_ai_debug.h"
//=============================================================
#include "kflow_ai_net/kflow_ai_core_callback.h"
#include "kflow_cpu/kflow_cpu_callback.h"
#include "kflow_dsp/kflow_dsp_callback.h"

#if CNN_AI_FASTBOOT
#include "kdrv_builtin/kdrv_builtin.h"
#endif

#if defined (__LINUX)
#include <linux/vmalloc.h>
#include "linux/kflow_ai_net_proc.h"
#define mem_alloc	vmalloc
#define mem_free	vfree
#else
#include "rtos/kflow_ai_net_proc.h"
#include <malloc.h>
#define mem_alloc	malloc
#define mem_free	free
#endif

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
#include "kflow_common/nvtmpp.h"
#else //_BSP_NA51068_
#include "kflow_common/ms/ms_core.h"
#endif

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define OUT_PATH        "output"

#define NN_DUMP_LAYER       ENABLE

#define SUPPORT_DEPTHWISE   ENABLE
#define LL_BASE_TEST        DISABLE // for ll_base test

#define SWAP(x, y){ \
    int tmp = x;\
    x = y;\
    y = tmp; \
  }

#if NN_DLI
#define VENDOR_AI_NN_DLI_CPU_CHK_VERINT      12 // Check Vendor AI library support NN_DLI operations
#endif
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
#define AI_MAX_MODIFY_NUM       1280
BOOL							g_ai_net_init = 0;
UINT32							g_ai_net_init_count = 0;
BOOL							g_ai_net_path_open[AI_SUPPORT_NET_MAX] = {0};
BOOL                            g_ai_net_path_is_used[AI_SUPPORT_NET_MAX] = {0};
BOOL                            g_ai_net_path_need_reset[AI_SUPPORT_NET_MAX] = {0};
SEM_HANDLE                      g_ai_path_sem_id;
SEM_HANDLE                      g_ai_common_lock;
BOOL                            g_ai_net_is_multi_process = 0;
BOOL                            g_ai_net_has_set_is_multi_process = 0;
#if LL_BASE_TEST
UINT32 test_ofs = 0xa0000;
#endif

UINT32							g_ai_support_net_max = 128;
typedef struct _KFLOW_AI_NET_GLOBAL {
	VENDOR_AIS_FLOW_MAP_MEM_PARM    *g_ai_map_mem;
	VENDOR_AIS_FLOW_MAP_MEM_PARM	*g_ai_user_mem_in_kerl;
	BOOL							*g_ai_net_state;
#if CNN_MULTI_INPUT
	BOOL							*g_ai_is_batch;
#endif
#if CNN_25_MATLAB
	NN_IOMEM						*g_ai_input_mem;
#else
	NN_DATA							**g_ai_input_imem;
#endif
	BOOL							*g_ai_input_set;
	UINT32							*g_ai_net_addr;
	SEM_HANDLE 						*g_ai_state_SEM_ID;
	UINT32                          **g_ai_ll_modify_idx;
	UINT64                          **g_ai_ll_modify_cmd;
	INT32                          *g_ai_ll_modify_num;
	UINT32                          **g_ai_ll_modify_idx_batch;
	UINT64                          **g_ai_ll_modify_cmd_batch;
	INT32                          *g_ai_ll_modify_num_batch;
	BOOL 							mem_dump;
	VENDOR_AIS_MEM_INFO 			mem_info;
	
} KFLOW_AI_NET_GLOBAL;
KFLOW_AI_NET_GLOBAL	kflow_ai_net_global = {0};
#if CNN_AI_FASTBOOT
UINT32 kflow_ai_net_fboot_dump_en = 0;
#endif

#if CNN_MULTI_INPUT
#define NN_SUPPORT_BATCH_NUM        16
#if NET_TABLE_HEAP
NN_DATA							    (**g_ai_input_batch_imem)[NN_IMEM_NUM] = 0;
UINT32                              **g_ai_input_layer_map_table = 0;
#else
NN_DATA							    g_ai_input_batch_imem[AI_SUPPORT_NET_MAX][NN_SUPPORT_BATCH_NUM][NN_IMEM_NUM] = {0};
UINT32                              g_ai_input_layer_map_table[AI_SUPPORT_NET_MAX][NN_SUPPORT_BATCH_NUM] = {0};
#endif
UINT32                              g_ai_input_layer_map_num[AI_SUPPORT_NET_MAX] = {0};
#endif

#define MULTI_SCALE_SUPPORT_NUM AI_SUPPORT_NET_MAX
#define AI_VENDOR_AI_CHK_VERINT      10 // int value of AI_LIB_CHK_VER

UINT32 g_scl_size[MULTI_SCALE_SUPPORT_NUM][2] = {0};
UINT32 g_scl_restore_en[MULTI_SCALE_SUPPORT_NUM] = {0};
/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
UINT32 nvt_ai_get_nextll_addr_cmd(UINT64 cmd);
UINT64 nvt_ai_set_nextll_addr_cmd(UINT64 cmd, UINT32 addr);
UINT64 nvt_ai_ll_nextupd_cmd(UINT32 addr);
BOOL nvt_ai_ll_is_nextupd_cmd(UINT64 cmd);
VOID nvt_ai_map_ai_app_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id);
#if CNN_25_MATLAB
VOID nvt_ai_map_ai_ll_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id);
#else
VOID nvt_ai_map_ai_ll_addr(UINT32 parm_addr, UINT32 parm_va_ofs, UINT32 parm_pa_ofs, UINT32 model_pa_ofs, UINT32 buf_pa_ofs, UINT32 net_id);
VOID nvt_ai_unmap_ai_app_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end, UINT32 net_id);
VOID nvt_ai_unmap_ai_ll_addr(UINT32 parm_addr, UINT32 parm_va_ofs, UINT32 parm_pa_ofs, UINT32 model_pa_ofs, UINT32 buf_pa_ofs, UINT32 model_pa_end, UINT32 buf_pa_end, UINT32 net_id);
VOID nvt_ai_unmap_drv_addr(NN_MODE mode, UINT32 parm_addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end, UINT32 net_id);
#endif
VOID nvt_ai_map_drv_addr(NN_MODE mode, UINT32 parm_addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id);
ER nvt_ai_get_net_info(NN_GEN_NET_INFO *p_info, UINT32 net_addr);
UINT32 str_sub_fmt_to_int(CHAR* vers);

extern UINT32 kdrv_ai_drv_get_net_supported_num(VOID);

#define KER_ENABLE_PADDING 1
#if KER_ENABLE_PADDING
static ER nvt_ai_get_ll_reg_val(UINT32 ll_addr, UINT32 reg_ofs, UINT32 *reg_val)
{
	UINT64* tmp_cmd = NULL;
	UINT32 tmp_idx = 0;
	UINT32 tmp_ll_type = 0;

	if (reg_val == NULL || ll_addr == 0) {
		DBG_ERR("nvt_ai_get_ll_reg_val invalid input\r\n");
		return E_NOEXS;
	}

	tmp_cmd = (UINT64*)ll_addr;
	while (1) {
		tmp_ll_type = (UINT32)(tmp_cmd[tmp_idx] >> 60);
		if (tmp_ll_type == 8) {
			if (((tmp_cmd[tmp_idx] >> 32) & 0xFF) == reg_ofs) {
				// modify the format
				UINT32 ll_reg_val = (UINT32)(tmp_cmd[tmp_idx]);
				*reg_val = ll_reg_val;
				break;
			} else {
				tmp_idx++;
			}
		} else if (tmp_ll_type == 0 || tmp_ll_type == 2) {
			// ll end or ll next
			break;
		} else {
			// find next cmd
			tmp_idx++;
		}
	}

	return E_OK;
}

static ER nvt_ai_set_ll_reg_val(UINT32 ll_addr, UINT32 reg_ofs, UINT32 reg_val)
{
	UINT64* tmp_cmd = NULL;
	UINT32 tmp_idx = 0;
	UINT32 tmp_ll_type = 0;

	if (ll_addr == 0) {
		DBG_ERR("nvt_ai_set_ll_reg_val invalid input\r\n");
		return E_NOEXS;
	}

	tmp_cmd = (UINT64*)ll_addr;
	while (1) {
		tmp_ll_type = (UINT32)(tmp_cmd[tmp_idx] >> 60);
		if (tmp_ll_type == 8) {
			if (((tmp_cmd[tmp_idx] >> 32) & 0xFF) == reg_ofs) {
				// modify the format
				UINT32* p_reg_val = (UINT32*)(&tmp_cmd[tmp_idx]);
				p_reg_val[0] = reg_val;
				break;
			} else {
				tmp_idx++;
			}
		} else if (tmp_ll_type == 0 || tmp_ll_type == 2) {
			// ll end or ll next
			break;
		} else {
			// find next cmd
			tmp_idx++;
		}
	}

	return E_OK;
}
#endif

static BOOL nvt_ai_is_null_mem(VENDOR_AIS_FLOW_MEM_PARM mem)
{
	if ((mem.va == 0) || (mem.pa == 0) || (mem.size == 0)) {
		return TRUE;
	}
	return FALSE;
}

static BOOL nvt_ai_is_mem_overflow(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem)
{
#if 0 // TODO: need to be discussed, making to new check rules
	if (p_mem->user_parm.size > (p_mem->user_model.pa - p_mem->user_parm.pa)) {
		DBG_WRN("user parm mem size(%d) is overflow...\r\n", (int)p_mem->user_parm.size);
		DBG_WRN("model pa = 0x%08X, parm pa = 0x%08X\r\n", (int)p_mem->user_model.pa, (int)p_mem->user_parm.pa);
		return TRUE;
	}
	if (p_mem->user_model.size > (p_mem->user_buff.pa - p_mem->user_model.pa)) {
		DBG_WRN("user model mem size(%d) is overflow...\r\n", (int)p_mem->user_model.size);
		DBG_WRN( "buff pa = 0x%08X, model pa = 0x%08X\r\n", (int)p_mem->user_buff.pa, (int)p_mem->user_model.pa);
		return TRUE;
	}
	if (p_mem->user_buff.size > (p_mem->kerl_parm.pa - p_mem->user_buff.pa)) {
		DBG_WRN("user buffer mem size(%d) is overflow...\r\n", (int)p_mem->user_buff.size);
		DBG_WRN("ker1 pa = 0x%08X, buff pa = 0x%08X\r\n", (int)p_mem->kerl_parm.pa, (int)p_mem->user_buff.pa);
		return TRUE;
	}
	if (p_mem->kerl_parm.size > (p_mem->user_model.pa - p_mem->user_parm.pa)) {
		DBG_WRN("kerl parm mem size(%d) is overflow...\r\n", (int)p_mem->kerl_parm.size);
		DBG_WRN("model pa = 0x%08X, parm pa = 0x%08X\r\n", (int)p_mem->user_model.pa, (int)p_mem->user_parm.pa);
		return TRUE;
	}
#endif
	return FALSE;
}

static BOOL nvt_ai_is_net_id_overflow(UINT32 net_id)
{
	if (net_id > (g_ai_support_net_max - 1)) {
		DBG_WRN("network id(%d) is overflow...\r\n", (int)net_id);
		return TRUE;
	} else {
		return FALSE;
	}
}

#if CNN_AI_FASTBOOT
ER nvt_ai_get_fboot_dump_en(UINT32 net_id, UINT32 *p_enable)
{
	if (p_enable == NULL) {
		DBG_ERR("p_enable == NULL\r\n");
		return E_SYS;
	}

	*p_enable = kflow_ai_net_fboot_dump_en;

	return E_OK;
}

#define AI_FASTBOOT_MAP_VA(addr, old_base, new_base)  (new_base + (addr - old_base)) // real_va = new_base + (offset)
ER nvt_ai_fix_fastboot_mem_va(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	UINT32 proc_layer_num;
	UINT32 process_index = 0, idx = 0;
	UINT32 old_base_va, new_base_va;
	UINT32 old_io_base_va, new_io_base_va;
	UINT32 pre_index = 999;

	// backup old_base_va
	old_base_va = p_mem->kerl_parm.va;
	old_io_base_va = p_mem->user_buff.va;

	// remap va as new base
	p_mem->kerl_parm.va = (UINT32)nvt_ai_pa2va_remap(p_mem->kerl_parm.pa, p_mem->kerl_parm.size);
	p_mem->user_model.va = (UINT32)nvt_ai_pa2va_remap(p_mem->user_model.pa, p_mem->user_model.size);
	p_mem->user_buff.va = (UINT32)nvt_ai_pa2va_remap(p_mem->user_buff.pa, p_mem->user_buff.size);
	new_base_va = p_mem->kerl_parm.va;
	new_io_base_va = p_mem->user_buff.va;

	// fix va in kerl space
	/*DBG_IND("======[kerl_space][fix_fastboot_mem]===================\n");
	DBG_IND("    kerl_parm (0x%x, 0x%x, 0x%x)\n", p_mem->kerl_parm.pa, p_mem->kerl_parm.va, p_mem->kerl_parm.size);
	DBG_IND("    user_model(0x%x, 0x%x, 0x%x)\n", p_mem->user_model.pa, p_mem->user_model.va, p_mem->user_model.size);
	DBG_IND("    user_buff (0x%x, 0x%x, 0x%x)\n", p_mem->user_buff.pa, p_mem->user_buff.va, p_mem->user_buff.size);
	DBG_IND("%s:%d [kerl_parm] old_base(0x%x), new_base(0x%x)\r\n", __func__, __LINE__, old_base_va, new_base_va);
	DBG_IND("%s:%d [io_buff] old_base(0x%x), new_base(0x%x)\r\n", __func__, __LINE__, old_io_base_va, new_io_base_va);*/

	er = nvt_ai_get_net_info(&net_info, p_mem->kerl_parm.va);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_LL_AI_DRV:
			if (p_mctrl[process_index].tot_trig_eng_times) {
				UINT32 layer_index = p_mctrl[process_index].layer_index;
				KDRV_AI_LL_HEAD *p_ll_head;
				NN_DATA *p_sai, *p_sao;
				UINT32 imem_cnt, omem_cnt;

				// map mctrl va
				p_mctrl[process_index].addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].addr, old_base_va, new_base_va);

				// map ll_head va
				p_ll_head = (KDRV_AI_LL_HEAD *)p_mctrl[process_index].addr;
				p_ll_head->parm_addr = AI_FASTBOOT_MAP_VA(p_ll_head->parm_addr, old_base_va, new_base_va);

				// map iomem va
				p_mctrl[process_index].iomem.imem_addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].iomem.imem_addr, old_base_va, new_base_va);
				p_mctrl[process_index].iomem.omem_addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].iomem.omem_addr, old_base_va, new_base_va);

				#if CNN_FMT_V4
				p_mctrl[process_index].in_bufinfo_addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].in_bufinfo_addr, old_base_va, new_base_va);
				p_mctrl[process_index].out_bufinfo_addr = AI_FASTBOOT_MAP_VA(p_mctrl[process_index].out_bufinfo_addr, old_base_va, new_base_va);
				#endif

				if (layer_index == pre_index) {
					continue;
				}
				pre_index = layer_index;

				p_sai = (NN_DATA *)p_mctrl[process_index].iomem.imem_addr;
				p_sao = (NN_DATA *)p_mctrl[process_index].iomem.omem_addr;
				imem_cnt = p_mctrl[process_index].iomem.imem_cnt;
				omem_cnt = p_mctrl[process_index].iomem.omem_cnt;

				// map sai va
				for (idx = 0; idx < imem_cnt; idx++) {
					p_sai[idx].va = AI_FASTBOOT_MAP_VA(p_sai[idx].va, old_io_base_va, new_io_base_va);
				}

				// map sao va
				for (idx = 0; idx < omem_cnt; idx++) {
					p_sao[idx].va = AI_FASTBOOT_MAP_VA(p_sao[idx].va, old_io_base_va, new_io_base_va);
				}
			}
			break;
		case NN_GEN_TRIG_APP_AI_DRV:
		case NN_GEN_TRIG_COMMON:
		default:
			DBG_ERR("invalid trig_src(%d), proc_idx(%u)\r\n", p_mctrl[process_index].trig_src, process_index);
			break;
		}
	}

	return E_OK;
}
#endif

#if CNN_MULTI_INPUT
#define AI_INPUT_ADDR_TYPE 0
UINT32 nvt_ai_net_get_input_layer_index(VENDOR_AIS_FLOW_MEM_PARM mem)
{
	UINT32 preproc_idx_num = 0;
#if !CNN_25_MATLAB
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	ER er = E_OK;
	UINT32 i = 0;
	UINT32 proc_layer_num;

	if (mem.va == 0) {
		DBG_ERR("input memory is 0...");
		return 0;
	}

	er = nvt_ai_get_net_info(&net_info, mem.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return 0;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	for (i = 0; i < proc_layer_num; i++) {
		NN_DATA* p_input;
		UINT32 imem_addr = p_mctrl[i].iomem.imem_addr;
		if (imem_addr < mem.va) imem_addr += mem.va; // if not fix yet(call this funciton before gen_init(), fix it

		p_input = (NN_DATA*)imem_addr;
		if (p_input[0].va == AI_INPUT_ADDR_TYPE) {
#if NET_TABLE_HEAP
#else
			if (preproc_idx_num > AI2_MAX_BATCH_NUM) {
				DBG_ERR("preproc_idx_num(%lu) exceeds AI2_MAX_BATCH_NUM(%d) limit !!\r\n", preproc_idx_num, AI2_MAX_BATCH_NUM);
				return 0;
			}
#endif
			preproc_idx_num++;
		} else {
			// suppose the input layers are always set at the first n layers...
			continue;
		}
	}
#endif

	return preproc_idx_num;
}
#endif

ER nvt_ai_open_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_map_mem;
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_user_mem;
	UINT32 kerl_va = 0;
	
	if ((p_mem == NULL)
			|| nvt_ai_is_null_mem(p_mem->user_parm)
			|| nvt_ai_is_null_mem(p_mem->user_model)
			|| nvt_ai_is_null_mem(p_mem->user_buff)
			|| nvt_ai_is_null_mem(p_mem->kerl_parm)) {
		DBG_ERR("null input...\r\n");
		return E_CTX;
	}
	if (nvt_ai_is_mem_overflow(p_mem) || nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("fail...\r\n");
		return E_CTX;
	}

	p_map_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];
	p_user_mem = &kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id];

	if (kflow_ai_net_global.g_ai_net_state[net_id]) {
		DBG_WRN("ai mem[%d] is already init!\r\n", (int)net_id);
		return E_SYS;
	}
	kflow_ai_net_global.g_ai_net_state[net_id] = TRUE;

#if CNN_AI_FASTBOOT
	if (kdrv_ai_is_fastboot() == DISABLE) { // under fastboot, do not remap kerl_va, it will be mapped in nvt_ai_fix_fastboot_mem_va() later
#endif
		kerl_va = (UINT32)nvt_ai_pa2va_remap(p_mem->kerl_parm.pa, p_mem->kerl_parm.size);
		p_mem->kerl_parm.va = kerl_va;
#if CNN_AI_FASTBOOT
	}
#endif

	memcpy(p_map_mem, p_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));

	memcpy(p_user_mem, p_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));

	p_user_mem->user_parm.va = (UINT32)nvt_ai_pa2va_remap(p_user_mem->user_parm.pa, p_user_mem->user_parm.size);

	p_user_mem->user_model.va = (UINT32)nvt_ai_pa2va_remap(p_user_mem->user_model.pa, p_user_mem->user_model.size);

	p_user_mem->user_buff.va = (UINT32)nvt_ai_pa2va_remap(p_user_mem->user_buff.pa, p_user_mem->user_buff.size);

#if CNN_AI_FASTBOOT
		if (kdrv_ai_is_fastboot()) {
			// fix fastboot mem va (in kernel space)
			if (nvt_ai_fix_fastboot_mem_va(p_map_mem, net_id) != E_OK) {
				DBG_ERR("nvt_ai_fix_fastboot_mem_va fail...\r\n");
			}
		}
#endif

#if CNN_AI_FASTBOOT
	if (kdrv_ai_is_fastboot() == DISABLE) { // under fastboot, should not use user_parm to replace kerl_parm
#endif
		if (p_user_mem->user_parm.size != 0) {
			memcpy((VOID *)p_map_mem->kerl_parm.va, (VOID *)p_user_mem->user_parm.va, p_user_mem->user_parm.size);
		}
#if CNN_AI_FASTBOOT
	}
#endif

#if (0)
	msleep(500);
	DBG_WRN("user parm mem: pa=0x%08x, va=0x%08x, size=%d\r\n"
			, (int)p_map_mem->user_parm.pa, (int)p_map_mem->user_parm.va, (int)p_map_mem->user_parm.size);
	DBG_WRN("user model mem: pa=0x%08x, va=0x%08x, size=%d\r\n"
			, (int)p_map_mem->user_model.pa, (int)p_map_mem->user_model.va, (int)p_map_mem->user_model.size);
	DBG_WRN("user buffer mem: pa=0x%08x, va=0x%08x, size=%d\r\n"
			, (int)p_map_mem->user_buff.pa, (int)p_map_mem->user_buff.va, (int)p_map_mem->user_buff.size);
	DBG_WRN("kerl parm mem: pa=0x%08x, va=0x%08x, size=%d\r\n"
			, (int)p_map_mem->kerl_parm.pa, (int)p_map_mem->kerl_parm.va, (int)p_map_mem->kerl_parm.size);

	DBG_WRN("user parm mem in kerl: pa=0x%08x, va=0x%08x, size=%d\r\n"
			, (int)p_user_mem->user_parm.pa, (int)p_user_mem->user_parm.va, (int)p_user_mem->user_parm.size);
	DBG_WRN("user model mem in kerl: pa=0x%08x, va=0x%08x, size=%d\r\n"
			, (int)p_user_mem->user_model.pa, (int)p_user_mem->user_model.va, (int)p_user_mem->user_model.size);
	DBG_WRN("user buffer mem in kerl: pa=0x%08x, va=0x%08x, size=%d\r\n"
			, (int)p_user_mem->user_buff.pa, (int)p_user_mem->user_buff.va, (int)p_user_mem->user_buff.size);
	msleep(500);
#endif
	return E_OK;
}

BOOL nvt_ai_is_dynamic_batch(UINT32 net_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_map_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	//NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 dst_tag = (UINT32)((UINT32)('B') | ((UINT32)('M')<<8) | ((UINT32)('I')<<16) | ((UINT32)('F')<<24));
	UINT32 tmp_size = 0;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	ER er = E_OK;


	p_map_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];

	er = nvt_ai_get_net_info(&net_info, p_map_mem->kerl_parm.va);
    if (er != E_OK) {
        DBG_ERR("nvt_ai_get_net_info fail...\r\n");
        return FALSE;
    }
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)( p_map_mem->kerl_parm.va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) {
			return TRUE;
		} else {
			if (p_tmp_head[0] == 0) {
				break;
			}
			tmp_size += p_tmp_head[0];
		}
	}


	return FALSE;
}
ER nvt_ai_dynamic_batch_init(UINT32 net_id)
{	
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_map_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	ER er = E_OK;

	p_map_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];

	er = nvt_ai_get_net_info(&net_info, p_map_mem->kerl_parm.va);
    if (er != E_OK) {
        DBG_ERR("nvt_ai_get_net_info fail...\r\n");
        return er;
    }
	p_head = net_info.p_head;
	proc_layer_num = p_head->mode_ctrl_num;
	kflow_ai_net_global.g_ai_ll_modify_idx_batch[net_id] = (UINT32 *)nvt_ai_mem_alloc(sizeof(UINT32) * proc_layer_num);
	if (kflow_ai_net_global.g_ai_ll_modify_idx_batch[net_id] == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_ll_modify_idx_batch[net_id], 0x0, sizeof(UINT32) * proc_layer_num);	

	kflow_ai_net_global.g_ai_ll_modify_cmd_batch[net_id] = (UINT64 *)nvt_ai_mem_alloc(sizeof(UINT64) * proc_layer_num);
	if (kflow_ai_net_global.g_ai_ll_modify_cmd_batch[net_id] == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_ll_modify_cmd_batch[net_id], 0x0, sizeof(UINT64) * proc_layer_num);

	return E_OK;
}
ER nvt_ai_dynamic_batch_uninit(UINT32 net_id)
{
	if (kflow_ai_net_global.g_ai_ll_modify_idx_batch[net_id]) {
			nvt_ai_mem_free(kflow_ai_net_global.g_ai_ll_modify_idx_batch[net_id]);
			kflow_ai_net_global.g_ai_ll_modify_idx_batch[net_id] = 0;
		}

	if (kflow_ai_net_global.g_ai_ll_modify_cmd_batch[net_id]) {
			nvt_ai_mem_free(kflow_ai_net_global.g_ai_ll_modify_cmd_batch[net_id]);
			kflow_ai_net_global.g_ai_ll_modify_cmd_batch[net_id] = 0;
		}	
	return E_OK;
}

ER nvt_ai_copy_net_from_user(UINT32 net_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_map_mem;
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_user_mem;

	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("fail...\r\n");
		return E_CTX;
	}

	p_map_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];
	p_user_mem = &kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id];

	//user space parm
	{
		NN_GEN_MODE_CTRL *p_mctrl = 0;
		KDRV_AI_APP_HEAD *p_head = 0;
		//KDRV_AI_FC_PARM *p_parm = 0;

		//re-link
		p_mctrl = (NN_GEN_MODE_CTRL*)(p_user_mem->user_parm.va);
		p_mctrl->addr = ((UINT32)p_mctrl + sizeof(NN_GEN_MODE_CTRL));
		p_head = (KDRV_AI_APP_HEAD *)(p_mctrl->addr);
		p_head->parm_addr = ((UINT32)p_head + sizeof(KDRV_AI_APP_HEAD) + sizeof(KDRV_AI_LL_HEAD));
		//p_parm = (KDRV_AI_FC_PARM *)(p_head->parm_addr);
	}
	//copy
	if (p_user_mem->user_parm.size != 0) {
		memcpy((VOID *)p_map_mem->kerl_parm.va, (VOID *)p_user_mem->user_parm.va, p_user_mem->user_parm.size);
	}
	//kernel space parm
	{
		NN_GEN_MODE_CTRL *p_mctrl = 0;
		KDRV_AI_APP_HEAD *p_head = 0;

		//re-link
		p_mctrl = (NN_GEN_MODE_CTRL*)(p_map_mem->kerl_parm.va);
		p_mctrl->addr = ((UINT32)p_mctrl + sizeof(NN_GEN_MODE_CTRL));
		p_head = (KDRV_AI_APP_HEAD *)(p_mctrl->addr);
		p_head->parm_addr = ((UINT32)p_head + sizeof(KDRV_AI_APP_HEAD) + sizeof(KDRV_AI_LL_HEAD));

        if(p_head->mode == AI_MODE_FC) {
            KDRV_AI_FC_PARM *p_parm = (KDRV_AI_FC_PARM *)(p_head->parm_addr);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		    //pa2va
		    p_parm->in_addr = nvtmpp_sys_pa2va(p_parm->in_addr);
		    p_parm->out_interm_addr = nvtmpp_sys_pa2va(p_parm->out_interm_addr);
		    p_parm->fc_ker.weight_addr = nvtmpp_sys_pa2va(p_parm->fc_ker.weight_addr);
#else
		    p_parm->in_addr = ms_user_pa_to_va(p_parm->in_addr, 0);
		    p_parm->out_interm_addr = ms_user_pa_to_va(p_parm->out_interm_addr, 0);
		    p_parm->fc_ker.weight_addr = ms_user_pa_to_va(p_parm->fc_ker.weight_addr, 0);
#endif
        } else if(p_head->mode == AI_MODE_PREPROC) {
            //preproc only need pa, because in kdrv_ai_trigger, g_nue2_parm.dmaio_addr.is_pa = 1
            /*
            KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)(p_head->parm_addr);
#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
		    //pa2va
		    if(p_parm->src_fmt == AI_PREPROC_SRC_YUV420) {
		        p_parm->in_addr[0] = nvtmpp_sys_pa2va(p_parm->in_addr[0]);
		        p_parm->in_addr[1] = nvtmpp_sys_pa2va(p_parm->in_addr[1]);
		    } else if(p_parm->src_fmt == AI_PREPROC_SRC_RGB) {
		        p_parm->in_addr[0] = nvtmpp_sys_pa2va(p_parm->in_addr[0]);
		        p_parm->in_addr[1] = nvtmpp_sys_pa2va(p_parm->in_addr[1]);
                p_parm->in_addr[2] = nvtmpp_sys_pa2va(p_parm->in_addr[2]);
            } else {
                p_parm->in_addr[0] = nvtmpp_sys_pa2va(p_parm->in_addr[0]);
            }
            if(p_parm->rst_fmt == AI_PREPROC_SRC_YUV420) {
		        p_parm->out_addr[0] = nvtmpp_sys_pa2va(p_parm->out_addr[0]);
		        p_parm->out_addr[1] = nvtmpp_sys_pa2va(p_parm->out_addr[1]);
		    } else if(p_parm->rst_fmt == AI_PREPROC_SRC_RGB) {
		        p_parm->out_addr[0] = nvtmpp_sys_pa2va(p_parm->out_addr[0]);
		        p_parm->out_addr[1] = nvtmpp_sys_pa2va(p_parm->out_addr[1]);
                p_parm->out_addr[2] = nvtmpp_sys_pa2va(p_parm->out_addr[2]);
            } else {
                p_parm->out_addr[0] = nvtmpp_sys_pa2va(p_parm->out_addr[0]);
            }
#else
		    if(p_parm->src_fmt == AI_PREPROC_SRC_YUV420) {
		        p_parm->in_addr[0] = ms_user_pa_to_va(p_parm->in_addr[0], 0);
		        p_parm->in_addr[1] = ms_user_pa_to_va(p_parm->in_addr[1], 0);
		    } else if(p_parm->src_fmt == AI_PREPROC_SRC_RGB) {
		        p_parm->in_addr[0] = ms_user_pa_to_va(p_parm->in_addr[0], 0);
		        p_parm->in_addr[1] = ms_user_pa_to_va(p_parm->in_addr[1], 0);
                p_parm->in_addr[2] = ms_user_pa_to_va(p_parm->in_addr[1], 0);
            } else {
                p_parm->in_addr[0] = ms_user_pa_to_va(p_parm->in_addr[0], 0);
            }
            if(p_parm->rst_fmt == AI_PREPROC_SRC_YUV420) {
		        p_parm->out_addr[0] = ms_user_pa_to_va(p_parm->out_addr[0], 0);
		        p_parm->out_addr[1] = ms_user_pa_to_va(p_parm->out_addr[1], 0);
		    } else if(p_parm->rst_fmt == AI_PREPROC_SRC_RGB) {
		        p_parm->out_addr[0] = ms_user_pa_to_va(p_parm->out_addr[0], 0);
		        p_parm->out_addr[1] = ms_user_pa_to_va(p_parm->out_addr[1], 0);
                p_parm->out_addr[2] = ms_user_pa_to_va(p_parm->out_addr[1], 0);
            } else {
                p_parm->out_addr[0] = ms_user_pa_to_va(p_parm->out_addr[0], 0);
            }
#endif
            */
        } else {
            DBG_ERR("Single op not support this KDRV_AI_MODE, MODE = %d...\r\n",p_head->mode);
		    return E_CTX;
        }
    }
	return E_OK;
}

ER nvt_ai_do_iounmap(UINT32 net_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_map_mem;
	NN_GEN_MODE_CTRL *p_mctrl = 0;
	KDRV_AI_LL_HEAD *p_head = 0;


	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("fail...\r\n");
		return E_CTX;
	}

	p_map_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];
	
	p_mctrl = (NN_GEN_MODE_CTRL*)(p_map_mem->kerl_parm.va);
	p_head = (KDRV_AI_LL_HEAD *)(p_mctrl->addr);
	
	nvt_ai_pa2va_unmap(p_head->parm_addr, p_map_mem->user_buff.pa);

	return E_OK;
}

ER nvt_ai_copy_net_from_user_ll(UINT32 net_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_map_mem;
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_user_mem;

	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("fail...\r\n");
		return E_CTX;
	}

	p_map_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];
	p_user_mem = &kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id];
	/*
	//user space parm
	{
		NN_GEN_MODE_CTRL *p_mctrl = 0;
		KDRV_AI_LL_HEAD *p_head = 0;

		//re-link
		p_mctrl = (NN_GEN_MODE_CTRL*)(p_user_mem->user_parm.va);
		p_mctrl->addr = ((UINT32)p_mctrl + sizeof(NN_GEN_MODE_CTRL) + sizeof(KDRV_AI_APP_HEAD));
		p_head = (KDRV_AI_LL_HEAD *)(p_mctrl->addr);

	}
	*/
	//copy
	if (p_user_mem->user_parm.size != 0) {
		memcpy((VOID *)p_map_mem->kerl_parm.va, (VOID *)p_user_mem->user_parm.va, p_user_mem->user_parm.size);
	}
	//kernel space parm
	{
		NN_GEN_MODE_CTRL *p_mctrl = 0;
		KDRV_AI_LL_HEAD *p_head = 0;

		//re-link
		p_mctrl = (NN_GEN_MODE_CTRL*)(p_map_mem->kerl_parm.va);
		p_mctrl->addr = ((UINT32)p_mctrl + sizeof(NN_GEN_MODE_CTRL) + sizeof(KDRV_AI_APP_HEAD));
		p_head = (KDRV_AI_LL_HEAD *)(p_mctrl->addr);

		/*
		DBGD(p_mctrl->mode);
		DBGD(p_mctrl->idea_cycle);
		DBGD(p_mctrl->eng);
		DBGD(p_mctrl->trig_src);
		DBGD(p_mctrl->tot_trig_eng_times);
		DBGH(p_mctrl->addr);
		DBGH(p_head->mode);
		DBGH(p_head->parm_addr);
		DBGD(p_head->parm_size);
		DBGD(p_head->eng);
		*/
		p_map_mem->user_buff.pa = p_head->parm_addr; //for iounmap
		p_head->parm_addr = (UINT32)nvt_ai_pa2va_remap(p_head->parm_addr, p_head->parm_size);
		/*
		DBG_IND("after pa2va:\r\n");
		DBGH((UINT32)p_mctrl);
		DBGH(p_mctrl->addr);
		DBGH((UINT32)p_head);
		DBGH(p_head->parm_addr);
		*/
	}
	return E_OK;
}

UINT32 nvt_ai_get_fc_input_parm_in_kerl(UINT32 net_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_user_mem;

	//user space parm
	NN_GEN_MODE_CTRL *p_mctrl = 0;

	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("fail...\r\n");
		return E_CTX;
	}

	p_user_mem = &kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id];

	p_mctrl = (NN_GEN_MODE_CTRL*)(p_user_mem->user_parm.va);

	return ((UINT32)p_mctrl + sizeof(NN_GEN_MODE_CTRL) + sizeof(KDRV_AI_APP_HEAD) + sizeof(KDRV_AI_LL_HEAD));
}

ER nvt_ai_close_net(UINT32 net_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_map_mem;
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_user_mem;

	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("fail...\r\n");
		return E_CTX;
	}

	p_map_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];
	p_user_mem = &kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id];

	if (!kflow_ai_net_global.g_ai_net_state[net_id]) {
		DBG_WRN("ai mem[%d] is already uninit!\r\n", (int)net_id);
		return E_SYS;
	}
	kflow_ai_net_global.g_ai_net_state[net_id] = FALSE;
#if CNN_MULTI_INPUT
	kflow_ai_net_global.g_ai_is_batch[net_id] = FALSE;
#endif

	nvt_ai_pa2va_unmap(p_user_mem->user_parm.va, p_user_mem->user_parm.pa);
	nvt_ai_pa2va_unmap(p_user_mem->user_model.va, p_user_mem->user_model.pa);
	nvt_ai_pa2va_unmap(p_user_mem->user_buff.va, p_user_mem->user_buff.pa);
	nvt_ai_pa2va_unmap(p_map_mem->kerl_parm.va, p_map_mem->kerl_parm.pa);

	memset(p_map_mem, 0, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));
	memset(p_user_mem, 0, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));

	return E_OK;
}

ER nvt_ai_uninit_net(void);
ER nvt_ai_uninit_net_path(UINT32 net_id);

ER nvt_ai_reset_net(void)
{
	UINT32 i, map_idx, max_map_num;
	
	if (g_ai_net_init > 0) {
		if (g_ai_net_is_multi_process && g_ai_net_init_count != 0) {
			return E_OK;
		}
		DBG_DUMP("kflow_ai_net - init: already init?\r\n");
		DBG_DUMP("kflow_ai_net - reset - begin\r\n");
		for (i=0; i<g_ai_support_net_max; i++) {
			//reset input
			if (kflow_ai_net_global.g_ai_input_set[i]) {
				DBG_DUMP(" <2> in[%d] force close\r\n", i);
				// search map table
				max_map_num = g_ai_input_layer_map_num[i]; // backup max_map_num
				for (map_idx = 0; map_idx < max_map_num; map_idx++) {
					// if TRUE, means this proc_idx has already do set_input(map) before
					// we need to do clr_input(unmap)
					if (nvt_ai_clr_input2(kflow_ai_net_global.g_ai_net_addr[i], map_idx, i) != E_OK) {
						DBG_ERR("clr_input2 fail, net_addr(0x%x), map_idx(%u), net_id(%u)\n",
						kflow_ai_net_global.g_ai_net_addr[i], map_idx, i);
						return E_SYS;
					}
				}
			}
			//reset net
			if (kflow_ai_net_global.g_ai_net_state[i]) {
				DBG_DUMP(" <1> net[%d] force close\r\n", i);
				nvt_ai_close_net(i);
			}
			//SEM_DESTROY(kflow_ai_net_global.g_ai_state_SEM_ID[i]);
#if NET_TABLE_HEAP
			nvt_ai_free_map_table(i);
#endif
		}
		//g_ai_net_init = 0;
		nvt_ai_uninit_net();
		
		DBG_DUMP("kflow_ai_net - reset - end\r\n");
	}

	return E_OK;
}

ER nvt_ai_reset_net_path(UINT32 net_id)
{
	UINT32 map_idx, max_map_num;
	//DBG_DUMP("kflow_ai_net - init: already init?\r\n");
	DBG_DUMP("kflow_ai_net - partial reset net_id = %u - begin\r\n",net_id);
	//reset input
	if (kflow_ai_net_global.g_ai_input_set[net_id]) {
		DBG_DUMP(" <2> in[%u] force close\r\n", net_id);
		// search map table
		max_map_num = g_ai_input_layer_map_num[net_id]; // backup max_map_num
		for (map_idx = 0; map_idx < max_map_num; map_idx++) {
			// if TRUE, means this proc_idx has already do set_input(map) before
			// we need to do clr_input(unmap)
			if (nvt_ai_clr_input2(kflow_ai_net_global.g_ai_net_addr[net_id], map_idx, net_id) != E_OK) {
				DBG_ERR("clr_input2 fail, net_addr(0x%x), map_idx(%u), net_id(%u)\n",
				kflow_ai_net_global.g_ai_net_addr[net_id], map_idx, net_id);
				return E_SYS;
			}
		}
	}
	//reset net
	if (kflow_ai_net_global.g_ai_net_state[net_id]) {
		DBG_DUMP(" <1> net[%u] force close\r\n", net_id);
		nvt_ai_close_net(net_id);
	}
#if NET_TABLE_HEAP
	nvt_ai_free_map_table(net_id);
#endif
	//SEM_DESTROY(kflow_ai_net_global.g_ai_state_SEM_ID[i]);
	//g_ai_net_init = 0;
	nvt_ai_uninit_net_path(net_id);
	
	DBG_DUMP("kflow_ai_net - reset net_id = %u - end\r\n",net_id);

	return E_OK;
}

void kflow_ai_mem_dump(UINT32 mask)
{
	kflow_ai_net_global.mem_dump = mask;
}

void nvt_ai_mem_info_update_1(VENDOR_AIS_MEM_INFO *mem_info)
{
	if (mem_info == NULL) {
		return;
	}
	kflow_ai_net_global.mem_info.value[0] = mem_info->value[0];
	
	if (kflow_ai_net_global.mem_dump > 0) {
		DBG_DUMP("mem_info: user %d\r\n", kflow_ai_net_global.mem_info.value[0]);
	}
}

ER nvt_ai_get_max_net(VENDOR_AIS_FLOW_ID *id_info)
{
	if (id_info == NULL) {
		return E_SYS;
	}
	id_info->ai_support_net_max = kdrv_ai_drv_get_net_supported_num();
	g_ai_support_net_max = id_info->ai_support_net_max;
	if (g_ai_support_net_max < 1) {
		return E_SYS;
	}
	return E_OK;
}

ER nvt_ai_init_net(VENDOR_AIS_FLOW_ID *id_info)
{
	UINT32 i;
	
	if (id_info == NULL) {
		return E_SYS;
	}
	
	id_info->ai_support_net_max = kdrv_ai_drv_get_net_supported_num();
	g_ai_support_net_max = id_info->ai_support_net_max;
	if (g_ai_support_net_max < 1) {
		return E_SYS;
	}
	
	if (g_ai_net_is_multi_process) {
		g_ai_net_init_count += 1;
		if (g_ai_net_init > 0) {
			return E_OK;
		}
	} else {
		if (g_ai_net_init > 0)
		return E_SYS;
	}
	
	memset(&(kflow_ai_net_global.mem_info), 0x0, sizeof(VENDOR_AIS_MEM_INFO));

//	DBG_IND("g_ai_support_net_max(%u) kflow_ai_net_global(%#lx)\n", g_ai_support_net_max, (long)&kflow_ai_net_global);

	kflow_ai_net_global.g_ai_map_mem = (VENDOR_AIS_FLOW_MAP_MEM_PARM *)mem_alloc(sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_map_mem == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_map_mem, 0x0, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM) * g_ai_support_net_max);
//	DBG_IND("g_ai_map_mem(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_map_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM) * g_ai_support_net_max);

	kflow_ai_net_global.g_ai_user_mem_in_kerl = (VENDOR_AIS_FLOW_MAP_MEM_PARM *)mem_alloc(sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_user_mem_in_kerl == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_user_mem_in_kerl, 0x0, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM) * g_ai_support_net_max);
//	DBG_IND("g_ai_user_mem_in_kerl(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_user_mem_in_kerl, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM) * g_ai_support_net_max);

	kflow_ai_net_global.g_ai_net_state = (BOOL *)mem_alloc(sizeof(BOOL) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_net_state == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_net_state, 0x0, sizeof(BOOL) * g_ai_support_net_max);
//	DBG_IND("g_ai_net_state(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_net_state, sizeof(BOOL) * g_ai_support_net_max);

#if CNN_MULTI_INPUT
	kflow_ai_net_global.g_ai_is_batch = (BOOL *)mem_alloc(sizeof(BOOL) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_is_batch == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_is_batch, 0x0, sizeof(BOOL) * g_ai_support_net_max);
//	DBG_IND("g_ai_is_batch(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_is_batch, sizeof(BOOL) * g_ai_support_net_max);
#endif

#if CNN_25_MATLAB
	kflow_ai_net_global.g_ai_input_mem = (NN_IOMEM **)mem_alloc(sizeof(NN_IOMEM) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_input_mem == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_input_mem, 0x0, sizeof(NN_IOMEM) * g_ai_support_net_max);
	for (i = 0; i < g_ai_support_net_max; i++) {
		kflow_ai_net_global.g_ai_input_imem[i] = (NN_IOMEM *)mem_alloc(sizeof(NN_IOMEM) * NN_IMEM_NUM);
		if (kflow_ai_net_global.g_ai_input_imem[i] == NULL) {
			return E_NOMEM;
		}
		memset(kflow_ai_net_global.g_ai_input_imem[i], 0x0, sizeof(NN_IOMEM) * NN_IMEM_NUM);
	}

#else
	kflow_ai_net_global.g_ai_input_imem = (NN_DATA **)mem_alloc(sizeof(NN_DATA *) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_input_imem == NULL) {
		return E_NOMEM;
	}
//	memset(kflow_ai_net_global.g_ai_input_imem, 0x0, sizeof(NN_DATA) * g_ai_support_net_max);
//	DBG_IND("g_ai_input_imem(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_input_imem, sizeof(NN_DATA) * g_ai_support_net_max);
	for (i = 0; i < g_ai_support_net_max; i++) {
		kflow_ai_net_global.g_ai_input_imem[i] = (NN_DATA *)mem_alloc(sizeof(NN_DATA) * NN_IMEM_NUM);
		if (kflow_ai_net_global.g_ai_input_imem[i] == NULL) {
			return E_NOMEM;
		}
		memset(kflow_ai_net_global.g_ai_input_imem[i], 0x0, sizeof(NN_DATA) * NN_IMEM_NUM);
//		DBG_IND("g_ai_input_imem[%d](%#lx) size(%u)\n", i, (long)kflow_ai_net_global.NN_DATA[i], sizeof(NN_DATA) * NN_IMEM_NUM);
	}
#endif
	kflow_ai_net_global.g_ai_input_set = (BOOL *)mem_alloc(sizeof(BOOL) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_input_set == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_input_set, 0x0, sizeof(BOOL) * g_ai_support_net_max);
//	DBG_IND("g_ai_input_set(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_input_set, sizeof(BOOL) * g_ai_support_net_max);

	kflow_ai_net_global.g_ai_net_addr = (UINT32 *)mem_alloc(sizeof(UINT32) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_net_addr == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_net_addr, 0x0, sizeof(UINT32) * g_ai_support_net_max);
//	DBG_IND("g_ai_net_addr(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_net_addr, sizeof(UINT32) * g_ai_support_net_max);

	kflow_ai_net_global.g_ai_state_SEM_ID = (SEM_HANDLE *)mem_alloc(sizeof(SEM_HANDLE) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_state_SEM_ID == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_state_SEM_ID, 0x0, sizeof(SEM_HANDLE) * g_ai_support_net_max);
//	DBG_IND("g_ai_state_SEM_ID(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_state_SEM_ID, sizeof(SEM_HANDLE) * g_ai_support_net_max);
	
	kflow_ai_net_global.g_ai_ll_modify_idx = (UINT32 **)mem_alloc(sizeof(UINT32 *) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_ll_modify_idx == NULL) {
		return E_NOMEM;
	}
//	memset(kflow_ai_net_global.g_ai_ll_modify_idx, 0x0, sizeof(UINT32) * g_ai_support_net_max);
//	DBG_IND("g_ai_ll_modify_idx(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_ll_modify_idx, sizeof(UINT32 *) * g_ai_support_net_max);

	for (i = 0; i < g_ai_support_net_max; i++) {
		kflow_ai_net_global.g_ai_ll_modify_idx[i] = (UINT32 *)mem_alloc(sizeof(UINT32) * AI_MAX_MODIFY_NUM);
		if (kflow_ai_net_global.g_ai_ll_modify_idx[i] == NULL) {
			return E_NOMEM;
		}
		memset(kflow_ai_net_global.g_ai_ll_modify_idx[i], 0x0, sizeof(UINT32) * g_ai_support_net_max);
//		DBG_IND("g_ai_ll_modify_idx[%d](%#lx) size(%u)\n", i, (long)kflow_ai_net_global.g_ai_ll_modify_idx[i], sizeof(UINT32) * AI_MAX_MODIFY_NUM);
	}

	kflow_ai_net_global.g_ai_ll_modify_cmd = (UINT64 **)mem_alloc(sizeof(UINT64 *) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_ll_modify_cmd == NULL) {
		return E_NOMEM;
	}
//	memset(kflow_ai_net_global.g_ai_ll_modify_cmd, 0x0, sizeof(UINT64) * g_ai_support_net_max);
//	DBG_IND("g_ai_ll_modify_cmd(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_ll_modify_cmd, sizeof(UINT64 *) * g_ai_support_net_max);

	for (i = 0; i < g_ai_support_net_max; i++) {
		kflow_ai_net_global.g_ai_ll_modify_cmd[i] = (UINT64 *)mem_alloc(sizeof(UINT64) * AI_MAX_MODIFY_NUM);
		if (kflow_ai_net_global.g_ai_ll_modify_cmd[i] == NULL) {
			return E_NOMEM;
		}
		memset(kflow_ai_net_global.g_ai_ll_modify_cmd[i], 0x0, sizeof(UINT64) * g_ai_support_net_max);
//		DBG_IND("g_ai_ll_modify_cmd[%d](%#lx) size(%u)\n", i, (long)kflow_ai_net_global.g_ai_ll_modify_cmd[i], sizeof(UINT64) * AI_MAX_MODIFY_NUM);
	}

	kflow_ai_net_global.g_ai_ll_modify_num = (INT32 *)mem_alloc(sizeof(INT32) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_ll_modify_num == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_ll_modify_num, 0x0, sizeof(INT32) * g_ai_support_net_max);
//	DBG_IND("g_ai_ll_modify_num(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_ll_modify_num, sizeof(UINT32) * g_ai_support_net_max);

	kflow_ai_net_global.g_ai_ll_modify_idx_batch = (UINT32 **)mem_alloc(sizeof(UINT32 *) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_ll_modify_idx_batch == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_ll_modify_idx_batch, 0x0, sizeof(UINT32*) * g_ai_support_net_max);
//	DBG_IND("g_ai_ll_modify_idx(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_ll_modify_idx, sizeof(UINT32 *) * g_ai_support_net_max);

// 	for (i = 0; i < g_ai_support_net_max; i++) {
// 		kflow_ai_net_global.g_ai_ll_modify_idx_batch[i] = (UINT32 *)mem_alloc(sizeof(UINT32) * AI_MAX_MODIFY_NUM);
// 		if (kflow_ai_net_global.g_ai_ll_modify_idx_batch[i] == NULL) {
// 			return E_NOMEM;
// 		}
// 		memset(kflow_ai_net_global.g_ai_ll_modify_idx_batch[i], 0x0, sizeof(UINT32) * g_ai_support_net_max);
// //		DBG_IND("g_ai_ll_modify_idx[%d](%#lx) size(%u)\n", i, (long)kflow_ai_net_global.g_ai_ll_modify_idx[i], sizeof(UINT32) * AI_MAX_MODIFY_NUM);
// 	}

	kflow_ai_net_global.g_ai_ll_modify_cmd_batch = (UINT64 **)mem_alloc(sizeof(UINT64 *) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_ll_modify_cmd_batch == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_ll_modify_cmd_batch, 0x0, sizeof(UINT64*) * g_ai_support_net_max);
//	DBG_IND("g_ai_ll_modify_cmd(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_ll_modify_cmd, sizeof(UINT64 *) * g_ai_support_net_max);

// 	for (i = 0; i < g_ai_support_net_max; i++) {
// 		kflow_ai_net_global.g_ai_ll_modify_cmd_batch[i] = (UINT64 *)mem_alloc(sizeof(UINT64) * AI_MAX_MODIFY_NUM);
// 		if (kflow_ai_net_global.g_ai_ll_modify_cmd_batch[i] == NULL) {
// 			return E_NOMEM;
// 		}
// 		memset(kflow_ai_net_global.g_ai_ll_modify_cmd_batch[i], 0x0, sizeof(UINT64) * g_ai_support_net_max);
// //		DBG_IND("g_ai_ll_modify_cmd[%d](%#lx) size(%u)\n", i, (long)kflow_ai_net_global.g_ai_ll_modify_cmd[i], sizeof(UINT64) * AI_MAX_MODIFY_NUM);
// 	}

	kflow_ai_net_global.g_ai_ll_modify_num_batch = (INT32 *)mem_alloc(sizeof(INT32) * g_ai_support_net_max);
	if (kflow_ai_net_global.g_ai_ll_modify_num_batch == NULL) {
		return E_NOMEM;
	}
	memset(kflow_ai_net_global.g_ai_ll_modify_num_batch, 0x0, sizeof(INT32) * g_ai_support_net_max);
//	DBG_IND("g_ai_ll_modify_num(%#lx) size(%u)\n", (long)kflow_ai_net_global.g_ai_ll_modify_num, sizeof(UINT32) * g_ai_support_net_max);

#if NET_TABLE_HEAP
	g_ai_input_layer_map_table = (UINT32 **)nvt_ai_mem_alloc(sizeof(UINT32 *) * g_ai_support_net_max);
	if (g_ai_input_layer_map_table == NULL) {
		return E_NOMEM;
	}
	memset(g_ai_input_layer_map_table, 0x0, sizeof(UINT32 *) * g_ai_support_net_max);

	g_ai_input_batch_imem = (NN_DATA (**)[NN_IMEM_NUM])nvt_ai_mem_alloc(sizeof(NN_DATA (*)[NN_IMEM_NUM]) * g_ai_support_net_max);
	if (g_ai_input_batch_imem == NULL) {
		return E_NOMEM;
	}
	memset(g_ai_input_batch_imem, 0x0, sizeof(NN_DATA (*)[NN_IMEM_NUM]) * g_ai_support_net_max);
#endif
	for (i=0; i<g_ai_support_net_max; i++) {
		SEM_CREATE(kflow_ai_net_global.g_ai_state_SEM_ID[i], 1);
	}
	if (g_ai_net_is_multi_process) {
		SEM_CREATE(g_ai_path_sem_id, 1);
	}
	SEM_CREATE(g_ai_common_lock, 1);
	//kflow_ai_core_cb_init_cb();
	//kflow_ai_cpu_init_cb();
	//kflow_ai_dsp_init_cb();
	kflow_ai_net_proc_init();

	g_ai_net_init = 1;
	return E_OK;
}


ER nvt_ai_uninit_net(void)
{
	UINT32 i;

	if (g_ai_net_is_multi_process) {
		if (g_ai_net_init_count > 1)
			return E_OK;
	} else {
		if (g_ai_net_init == 0)
			return E_SYS;
	}

	//kflow_ai_core_cb_uninit_cb();
	//kflow_ai_cpu_uninit_cb();
	//kflow_ai_dsp_uninit_cb();
	kflow_ai_net_proc_uninit();
	SEM_DESTROY(g_ai_common_lock);

	if (g_ai_net_is_multi_process) {
		SEM_DESTROY(g_ai_path_sem_id);
	}

	for (i=0; i<g_ai_support_net_max; i++) {
		SEM_DESTROY(kflow_ai_net_global.g_ai_state_SEM_ID[i]);
	}

	if (kflow_ai_net_global.g_ai_map_mem) {
		mem_free(kflow_ai_net_global.g_ai_map_mem);
	}
	if (kflow_ai_net_global.g_ai_user_mem_in_kerl) {
		mem_free(kflow_ai_net_global.g_ai_user_mem_in_kerl);
	}
	if (kflow_ai_net_global.g_ai_net_state) {
		mem_free(kflow_ai_net_global.g_ai_net_state);
	}
#if CNN_MULTI_INPUT
	if (kflow_ai_net_global.g_ai_is_batch) {
		mem_free(kflow_ai_net_global.g_ai_is_batch);
	}
#endif
#if CNN_25_MATLAB
	for (i = 0; i < g_ai_support_net_max; i++) {
		if (kflow_ai_net_global.g_ai_input_imem[i]) {
			mem_free(kflow_ai_net_global.g_ai_input_imem[i]);
		}
	}
	if (kflow_ai_net_global.g_ai_input_mem) {
		mem_free(kflow_ai_net_global.g_ai_input_mem);
	}
#else
	for (i = 0; i < g_ai_support_net_max; i++) {
		if (kflow_ai_net_global.g_ai_input_imem[i]) {
			mem_free(kflow_ai_net_global.g_ai_input_imem[i]);
		}
	}
	if (kflow_ai_net_global.g_ai_input_imem) {
		mem_free(kflow_ai_net_global.g_ai_input_imem);
	}
#endif
	if (kflow_ai_net_global.g_ai_input_set) {
		mem_free(kflow_ai_net_global.g_ai_input_set);
	}
	if (kflow_ai_net_global.g_ai_net_addr) {
		mem_free(kflow_ai_net_global.g_ai_net_addr);
	}
	if (kflow_ai_net_global.g_ai_state_SEM_ID) {
		mem_free(kflow_ai_net_global.g_ai_state_SEM_ID);
	}
	// for (i = 0; i < g_ai_support_net_max; i++) {
	// 	if (kflow_ai_net_global.g_ai_ll_modify_idx_batch[i]) {
	// 		mem_free(kflow_ai_net_global.g_ai_ll_modify_idx_batch[i]);
	// 	}
	// }
	if (kflow_ai_net_global.g_ai_ll_modify_idx_batch) {
		mem_free(kflow_ai_net_global.g_ai_ll_modify_idx_batch);
	}
	// for (i = 0; i < g_ai_support_net_max; i++) {
	// 	if (kflow_ai_net_global.g_ai_ll_modify_cmd_batch[i]) {
	// 		mem_free(kflow_ai_net_global.g_ai_ll_modify_cmd_batch[i]);
	// 	}
	// }
	if (kflow_ai_net_global.g_ai_ll_modify_cmd_batch) {
		mem_free(kflow_ai_net_global.g_ai_ll_modify_cmd_batch);
	}
	if (kflow_ai_net_global.g_ai_ll_modify_num_batch) {
		mem_free(kflow_ai_net_global.g_ai_ll_modify_num_batch);
	}
	for (i = 0; i < g_ai_support_net_max; i++) {
		if (kflow_ai_net_global.g_ai_ll_modify_idx[i]) {
			mem_free(kflow_ai_net_global.g_ai_ll_modify_idx[i]);
		}
	}
	if (kflow_ai_net_global.g_ai_ll_modify_idx) {
		mem_free(kflow_ai_net_global.g_ai_ll_modify_idx);
	}
	for (i = 0; i < g_ai_support_net_max; i++) {
		if (kflow_ai_net_global.g_ai_ll_modify_cmd[i]) {
			mem_free(kflow_ai_net_global.g_ai_ll_modify_cmd[i]);
		}
	}
	if (kflow_ai_net_global.g_ai_ll_modify_cmd) {
		mem_free(kflow_ai_net_global.g_ai_ll_modify_cmd);
	}
	if (kflow_ai_net_global.g_ai_ll_modify_num) {
		mem_free(kflow_ai_net_global.g_ai_ll_modify_num);
	}
#if NET_TABLE_HEAP
	if (g_ai_input_layer_map_table) {
		nvt_ai_mem_free(g_ai_input_layer_map_table);
	}

	if (g_ai_input_batch_imem) {
		nvt_ai_mem_free(g_ai_input_batch_imem);
	}
#endif
	g_ai_net_init = 0;
	if (g_ai_net_is_multi_process) {
		memset(g_ai_net_path_is_used, 0x0, sizeof(BOOL)*g_ai_support_net_max);
		memset(g_ai_net_path_need_reset, 0x0, sizeof(BOOL)*g_ai_support_net_max);
	}
	return E_OK;
}

ER nvt_ai_uninit_net_path(UINT32 net_id)
{
	if (g_ai_net_init == 0)
		return E_SYS;

	SEM_DESTROY(kflow_ai_net_global.g_ai_state_SEM_ID[net_id]);
	memset(kflow_ai_net_global.g_ai_state_SEM_ID + net_id, 0x0, sizeof(SEM_HANDLE));
	SEM_CREATE(kflow_ai_net_global.g_ai_state_SEM_ID[net_id], 1);

	memset(kflow_ai_net_global.g_ai_map_mem + net_id, 0x0, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));
	
	memset(kflow_ai_net_global.g_ai_user_mem_in_kerl + net_id, 0x0, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));
	
	memset(kflow_ai_net_global.g_ai_net_state + net_id, 0x0, sizeof(BOOL));
	
#if CNN_MULTI_INPUT
	memset(kflow_ai_net_global.g_ai_is_batch + net_id, 0x0, sizeof(BOOL));
#endif
	if (kflow_ai_net_global.g_ai_input_imem[net_id]) {
		memset(kflow_ai_net_global.g_ai_input_imem[net_id], 0x0, sizeof(NN_DATA) * NN_IMEM_NUM);
	}
	
	memset(kflow_ai_net_global.g_ai_input_set + net_id, 0x0, sizeof(BOOL));
	
	memset(kflow_ai_net_global.g_ai_net_addr + net_id, 0x0, sizeof(UINT32));

	memset(kflow_ai_net_global.g_ai_ll_modify_idx_batch[net_id], 0x0, sizeof(UINT32));

	memset(kflow_ai_net_global.g_ai_ll_modify_cmd_batch[net_id], 0x0, sizeof(UINT64));

	memset(kflow_ai_net_global.g_ai_ll_modify_num_batch + net_id, 0x0, sizeof(INT32));
	
	memset(kflow_ai_net_global.g_ai_ll_modify_idx_batch[net_id], 0x0, sizeof(UINT32));

	memset(kflow_ai_net_global.g_ai_ll_modify_cmd_batch[net_id], 0x0, sizeof(UINT64));

	memset(kflow_ai_net_global.g_ai_ll_modify_num_batch + net_id, 0x0, sizeof(INT32));
#if NET_TABLE_HEAP
	memset(g_ai_input_layer_map_table + net_id, 0x0, sizeof(UINT32 *));

	memset(g_ai_input_batch_imem + net_id, 0x0, sizeof(NN_DATA (*)[NN_IMEM_NUM]));
#endif
	return E_OK;
}

ER nvt_ai_lock_net(UINT32 net_id)
{
	if (g_ai_net_init == 0)
		return E_SYS;

	if (net_id >= g_ai_support_net_max)
		return E_SYS;

	if (SEM_WAIT_INTERRUPTIBLE(kflow_ai_net_global.g_ai_state_SEM_ID[net_id])) {
		return E_RLWAI;
	}
	return E_OK;
}

ER nvt_ai_unlock_net(UINT32 net_id)
{
	if (g_ai_net_init == 0)
		return E_SYS;
	
	if (net_id >= g_ai_support_net_max)
		return E_SYS;
	
	SEM_SIGNAL(kflow_ai_net_global.g_ai_state_SEM_ID[net_id]);
	return E_OK;
}

ER nvt_ai_get_kerl_start_mem(UINT32 net_id, VENDOR_AIS_FLOW_KERL_START_MEM *p_kerl_start)
{
	if (p_kerl_start == NULL) {
		DBG_ERR("p_kerl_start == NULL\r\n");
		return E_SYS;
	}

#if 0
	DBG_IND("(mem_in_kerl)kerl_parm: pa(0x%x) va(0x%x) size(0x%x)\r\n", 
			kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].kerl_parm.pa,
			kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].kerl_parm.va,
			kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].kerl_parm.size);

	DBG_IND("(mem_in_kerl)user_buff: pa(0x%x) va(0x%x) size(0x%x)\r\n", 
			kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff.pa,
			kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff.va,
			kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff.size);
#endif

	p_kerl_start->kerl_parm.pa = kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].kerl_parm.pa;
	p_kerl_start->kerl_parm.va = kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].kerl_parm.va;
	p_kerl_start->kerl_parm.size = kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].kerl_parm.size;
	
	p_kerl_start->user_buff.pa = kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff.pa;
	p_kerl_start->user_buff.va = kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff.va;
	p_kerl_start->user_buff.size = kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff.size;

	return E_OK;
}

UINT32 nvt_ai_get_nextll_addr_cmd(UINT64 cmd)
{
	return (UINT32)((cmd >> 8) & (UINT64)0xffffffff);
}

/*
    table index :   [0x0, 0xff]         bits : 7_0
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 nvt_ai_set_nextll_addr_cmd(UINT64 cmd, UINT32 addr)
{
	return ((cmd & (UINT64)0xffffff00000000ff) + ((UINT64)addr << (UINT64)8));
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 nvt_ai_set_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)8 << 60) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/*
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 nvt_ai_ll_nextupd_cmd(UINT32 addr)
{
	return (UINT64)(((UINT64)4 << 60) + ((UINT64)addr << 8));
}

BOOL nvt_ai_ll_is_nextupd_cmd(UINT64 cmd)
{
	return ((cmd >> 60) == 4) ? TRUE : FALSE;
}

#if CNN_25_MATLAB
static UINT32 net_map_addr_with_parsflag(UINT32 addr, UINT32 buf_ofs, UINT32 model_ofs)
{
	if ((addr & NN_GEN_MODEL_ADDR_TYPE) == NN_GEN_MODEL_ADDR_TYPE) {
		addr = (addr & NN_GEN_ADDR_MASK) + model_ofs;
	} else if ((addr & NN_GEN_BUF_ADDR_TYPE) == NN_GEN_BUF_ADDR_TYPE) {
		addr = (addr & NN_GEN_ADDR_MASK) + buf_ofs;
	} else if (addr == NN_GEN_NULL_ADDR_TYPE) {
		//null address
	} else if ((addr & NN_GEN_ADDR_TYPE_MASK) == 0) {
		//pass first layer input address
	} else {
		DBG_ERR("%s: not support flag: 0x%08x\r\n", __FUNCTION__, (int)addr);
	}
	return addr;
}
#else
static UINT32 net_map_addr_with_parsflag(UINT32 addr, UINT32 model_ofs, UINT32 buf_ofs)
{
	if ((addr & NN_GEN_MODEL_ADDR_TYPE) == NN_GEN_MODEL_ADDR_TYPE) {
		addr = (addr & NN_GEN_ADDR_MASK) + model_ofs;
	} else if ((addr & NN_GEN_BUF_ADDR_TYPE) == NN_GEN_BUF_ADDR_TYPE) {
		addr = (addr & NN_GEN_ADDR_MASK) + buf_ofs;
	} else if (addr == NN_GEN_NULL_ADDR_TYPE) {
		//null address
	} else if ((addr & NN_GEN_ADDR_TYPE_MASK) == 0) {
		//pass first layer input address
	} else {
		DBG_ERR("not support flag: 0x%08x\r\n", (int)addr);
	}
	return addr;
}
#endif
#if !CNN_25_MATLAB
static UINT32 net_unmap_addr(UINT32 addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end)
{
	if ((addr >= model_ofs) && (addr < model_end)) {
		addr = (addr - model_ofs) | NN_GEN_MODEL_ADDR_TYPE;
	} else if ((addr >= buf_ofs) && (addr < buf_end)) {
		addr = (addr - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
	} else if (addr == NN_GEN_NULL_ADDR_TYPE) {
		//null address
	} else {
		DBG_ERR("invalid address: 0x%08x\r\n", (int)addr);
	}
	return addr;
}
#endif

#if CNN_25_MATLAB
VOID nvt_ai_map_ai_app_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id)
{
	KDRV_AI_APP_HEAD *p_head = (KDRV_AI_APP_HEAD *)parm_addr;

	p_head->parm_addr += parm_ofs;
	if (p_head->stripe_head_addr != 0) {
		//multi-stripe engine
		p_head->stripe_head_addr += parm_ofs;
	}
	
	switch (p_head->mode) {
	case AI_MODE_NEURAL: {
			KDRV_AI_NEURAL_PARM *p_parm 	= (KDRV_AI_NEURAL_PARM *)p_head->parm_addr;
			p_parm->in_addr         		= net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->out0_addr       		= net_map_addr_with_parsflag(p_parm->out0_addr, buf_ofs, model_ofs);
			p_parm->out1_addr       		= net_map_addr_with_parsflag(p_parm->out1_addr, buf_ofs, model_ofs);
			p_parm->in_interm_addr  		= net_map_addr_with_parsflag(p_parm->in_interm_addr, buf_ofs, model_ofs);
			p_parm->out_interm_addr 		= net_map_addr_with_parsflag(p_parm->out_interm_addr, buf_ofs, model_ofs);
			p_parm->tmp_buf_addr    		= net_map_addr_with_parsflag(p_parm->tmp_buf_addr, buf_ofs, model_ofs);
			p_parm->conv.weight_addr        = net_map_addr_with_parsflag(p_parm->conv.weight_addr, buf_ofs, model_ofs);
			p_parm->conv.bias_addr  		= net_map_addr_with_parsflag(p_parm->conv.bias_addr, buf_ofs, model_ofs);
			p_parm->conv.fcd.quanti_kmeans_addr = net_map_addr_with_parsflag(p_parm->conv.fcd.quanti_kmeans_addr, buf_ofs, model_ofs);
			p_parm->conv.fcd.p_vlc_code     = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_code + parm_ofs);
			p_parm->conv.fcd.p_vlc_valid    = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_valid + parm_ofs);
			p_parm->conv.fcd.p_vlc_ofs      = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_ofs + parm_ofs);
			p_parm->norm.bn_scl.bn_scale_addr 	= net_map_addr_with_parsflag(p_parm->norm.bn_scl.bn_scale_addr, buf_ofs, model_ofs);
			p_parm->elt.addr        		= net_map_addr_with_parsflag(p_parm->elt.addr, buf_ofs, model_ofs);
		}
		break;
	case AI_MODE_ROIPOOL: {
			KDRV_AI_ROIPOOL_PARM *p_parm = (KDRV_AI_ROIPOOL_PARM *)p_head->parm_addr;
			p_parm->in_addr             = net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->out_addr            = net_map_addr_with_parsflag(p_parm->out_addr, buf_ofs, model_ofs);
			p_parm->roi_ker.roi_addr    = net_map_addr_with_parsflag(p_parm->roi_ker.roi_addr, buf_ofs, model_ofs);
		}
		break;
	case AI_MODE_SVM: {
			KDRV_AI_SVM_PARM *p_parm = (KDRV_AI_SVM_PARM *)p_head->parm_addr;
			p_parm->in_addr             = net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->out_addr            = net_map_addr_with_parsflag(p_parm->out_addr, buf_ofs, model_ofs);
			p_parm->svm_ker.sv_addr     = net_map_addr_with_parsflag(p_parm->svm_ker.sv_addr, buf_ofs, model_ofs);
			p_parm->svm_ker.alpha_addr  = net_map_addr_with_parsflag(p_parm->svm_ker.alpha_addr, buf_ofs, model_ofs);
			p_parm->fcd.quanti_kmeans_addr = net_map_addr_with_parsflag(p_parm->fcd.quanti_kmeans_addr, buf_ofs, model_ofs);
			p_parm->fcd.p_vlc_code      = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_code + parm_ofs);
			p_parm->fcd.p_vlc_valid     = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_valid + parm_ofs);
			p_parm->fcd.p_vlc_ofs       = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_ofs + parm_ofs);
		}
		break;
	case AI_MODE_FC: {
			KDRV_AI_FC_PARM *p_parm = (KDRV_AI_FC_PARM *)p_head->parm_addr;
			p_parm->in_addr             = net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->out_addr            = net_map_addr_with_parsflag(p_parm->out_addr, buf_ofs, model_ofs);
			p_parm->in_interm_addr      = net_map_addr_with_parsflag(p_parm->in_interm_addr, buf_ofs, model_ofs);
			p_parm->out_interm_addr     = net_map_addr_with_parsflag(p_parm->out_interm_addr, buf_ofs, model_ofs);
			p_parm->fc_ker.weight_addr  = net_map_addr_with_parsflag(p_parm->fc_ker.weight_addr, buf_ofs, model_ofs);
			p_parm->fc_ker.bias_addr    = net_map_addr_with_parsflag(p_parm->fc_ker.bias_addr, buf_ofs, model_ofs);
			p_parm->fcd.quanti_kmeans_addr = net_map_addr_with_parsflag(p_parm->fcd.quanti_kmeans_addr, buf_ofs, model_ofs);
			p_parm->fcd.p_vlc_code      = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_code + parm_ofs);
			p_parm->fcd.p_vlc_valid     = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_valid + parm_ofs);
			p_parm->fcd.p_vlc_ofs       = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_ofs + parm_ofs);
		}
		break;
	case AI_MODE_PERMUTE: {
			KDRV_AI_PERMUTE_PARM *p_parm = (KDRV_AI_PERMUTE_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->out_addr    = net_map_addr_with_parsflag(p_parm->out_addr, buf_ofs, model_ofs);
		}
		break;
	case AI_MODE_REORG: {
			KDRV_AI_REORG_PARM *p_parm = (KDRV_AI_REORG_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->out_addr    = net_map_addr_with_parsflag(p_parm->out_addr, buf_ofs, model_ofs);
		}
        break;
    case AI_MODE_ANCHOR: {
			KDRV_AI_ANCHOR_PARM *p_parm = (KDRV_AI_ANCHOR_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->w_addr    = net_map_addr_with_parsflag(p_parm->w_addr, buf_ofs, model_ofs);
            p_parm->b_addr     = net_map_addr_with_parsflag(p_parm->b_addr, buf_ofs, model_ofs);
            p_parm->tbl_addr     = net_map_addr_with_parsflag(p_parm->tbl_addr, buf_ofs, model_ofs);
            p_parm->out_addr     = net_map_addr_with_parsflag(p_parm->out_addr, buf_ofs, model_ofs);
		}
		break;
    case AI_MODE_SOFTMAX: {
			KDRV_AI_SOFTMAX_PARM *p_parm = (KDRV_AI_SOFTMAX_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->out_addr    = net_map_addr_with_parsflag(p_parm->out_addr, buf_ofs, model_ofs);
		}
        break;
    case AI_MODE_PREPROC: {
			KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)p_head->parm_addr;
			p_parm->in_addr[0]     = net_map_addr_with_parsflag(p_parm->in_addr[0], buf_ofs, model_ofs);
            p_parm->in_addr[1]     = net_map_addr_with_parsflag(p_parm->in_addr[1], buf_ofs, model_ofs);
            p_parm->in_addr[2]     = net_map_addr_with_parsflag(p_parm->in_addr[2], buf_ofs, model_ofs);
			p_parm->out_addr[0]    = net_map_addr_with_parsflag(p_parm->out_addr[0], buf_ofs, model_ofs);
            p_parm->out_addr[1]    = net_map_addr_with_parsflag(p_parm->out_addr[1], buf_ofs, model_ofs);
            p_parm->out_addr[2]    = net_map_addr_with_parsflag(p_parm->out_addr[2], buf_ofs, model_ofs);
		}
		break;
	default:
		break;
	}
}
#else
VOID nvt_ai_map_ai_app_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id)
{
	KDRV_AI_APP_HEAD *p_head = (KDRV_AI_APP_HEAD *)parm_addr;
	p_head->parm_addr += parm_ofs;
	if (p_head->stripe_head_addr != 0) {
		//multi-stripe engine
		p_head->stripe_head_addr += parm_ofs;
	}

	switch (p_head->mode) {
	case AI_MODE_NEURAL: {
			KDRV_AI_NEURAL_PARM *p_parm 	= (KDRV_AI_NEURAL_PARM *)p_head->parm_addr;
			p_parm->in_addr         		= net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->out0_addr       		= net_map_addr_with_parsflag(p_parm->out0_addr, model_ofs, buf_ofs);
			p_parm->out1_addr       		= net_map_addr_with_parsflag(p_parm->out1_addr, model_ofs, buf_ofs);
			p_parm->in_interm_addr  		= net_map_addr_with_parsflag(p_parm->in_interm_addr, model_ofs, buf_ofs);
			p_parm->out_interm_addr 		= net_map_addr_with_parsflag(p_parm->out_interm_addr, model_ofs, buf_ofs);
			p_parm->tmp_buf_addr    		= net_map_addr_with_parsflag(p_parm->tmp_buf_addr, model_ofs, buf_ofs);
			p_parm->conv.weight_addr        = net_map_addr_with_parsflag(p_parm->conv.weight_addr, model_ofs, buf_ofs);
			p_parm->conv.bias_addr  		= net_map_addr_with_parsflag(p_parm->conv.bias_addr, model_ofs, buf_ofs);
			p_parm->conv.fcd.quanti_kmeans_addr = net_map_addr_with_parsflag(p_parm->conv.fcd.quanti_kmeans_addr, model_ofs, buf_ofs);
			p_parm->conv.fcd.p_vlc_code     = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_code + parm_ofs);
			p_parm->conv.fcd.p_vlc_valid    = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_valid + parm_ofs);
			p_parm->conv.fcd.p_vlc_ofs      = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_ofs + parm_ofs);
			p_parm->norm.bn_scl.bn_scale_addr 	= net_map_addr_with_parsflag(p_parm->norm.bn_scl.bn_scale_addr, model_ofs, buf_ofs);
			p_parm->elt.addr        		= net_map_addr_with_parsflag(p_parm->elt.addr, model_ofs, buf_ofs);
		}
		break;
	case AI_MODE_ROIPOOL: {
			KDRV_AI_ROIPOOL_PARM *p_parm = (KDRV_AI_ROIPOOL_PARM *)p_head->parm_addr;
			p_parm->in_addr             = net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->out_addr            = net_map_addr_with_parsflag(p_parm->out_addr, model_ofs, buf_ofs);
			p_parm->roi_ker.roi_addr    = net_map_addr_with_parsflag(p_parm->roi_ker.roi_addr, model_ofs, buf_ofs);
		}
		break;
	case AI_MODE_SVM: {
			KDRV_AI_SVM_PARM *p_parm = (KDRV_AI_SVM_PARM *)p_head->parm_addr;
			p_parm->in_addr             = net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->out_addr            = net_map_addr_with_parsflag(p_parm->out_addr, model_ofs, buf_ofs);
			p_parm->svm_ker.sv_addr     = net_map_addr_with_parsflag(p_parm->svm_ker.sv_addr, model_ofs, buf_ofs);
			p_parm->svm_ker.alpha_addr  = net_map_addr_with_parsflag(p_parm->svm_ker.alpha_addr, model_ofs, buf_ofs);
			p_parm->fcd.quanti_kmeans_addr = net_map_addr_with_parsflag(p_parm->fcd.quanti_kmeans_addr, model_ofs, buf_ofs);
			p_parm->fcd.p_vlc_code      = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_code + parm_ofs);
			p_parm->fcd.p_vlc_valid     = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_valid + parm_ofs);
			p_parm->fcd.p_vlc_ofs       = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_ofs + parm_ofs);
		}
		break;
	case AI_MODE_FC: {
			KDRV_AI_FC_PARM *p_parm = (KDRV_AI_FC_PARM *)p_head->parm_addr;
			p_parm->in_addr             = net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->out_addr            = net_map_addr_with_parsflag(p_parm->out_addr, model_ofs, buf_ofs);
			p_parm->in_interm_addr      = net_map_addr_with_parsflag(p_parm->in_interm_addr, model_ofs, buf_ofs);
			p_parm->out_interm_addr     = net_map_addr_with_parsflag(p_parm->out_interm_addr, model_ofs, buf_ofs);
			p_parm->fc_ker.weight_addr  = net_map_addr_with_parsflag(p_parm->fc_ker.weight_addr, model_ofs, buf_ofs);
			p_parm->fc_ker.bias_addr    = net_map_addr_with_parsflag(p_parm->fc_ker.bias_addr, model_ofs, buf_ofs);
			p_parm->fcd.quanti_kmeans_addr = net_map_addr_with_parsflag(p_parm->fcd.quanti_kmeans_addr, model_ofs, buf_ofs);
			p_parm->fcd.p_vlc_code      = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_code + parm_ofs);
			p_parm->fcd.p_vlc_valid     = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_valid + parm_ofs);
			p_parm->fcd.p_vlc_ofs       = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_ofs + parm_ofs);
		}
		break;
	case AI_MODE_PERMUTE: {
			KDRV_AI_PERMUTE_PARM *p_parm = (KDRV_AI_PERMUTE_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->out_addr    = net_map_addr_with_parsflag(p_parm->out_addr, model_ofs, buf_ofs);
		}
		break;
	case AI_MODE_REORG: {
			KDRV_AI_REORG_PARM *p_parm = (KDRV_AI_REORG_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->out_addr    = net_map_addr_with_parsflag(p_parm->out_addr, model_ofs, buf_ofs);
		}
        break;
    case AI_MODE_ANCHOR: {
			KDRV_AI_ANCHOR_PARM *p_parm = (KDRV_AI_ANCHOR_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->w_addr    = net_map_addr_with_parsflag(p_parm->w_addr, model_ofs, buf_ofs);
            p_parm->b_addr     = net_map_addr_with_parsflag(p_parm->b_addr, model_ofs, buf_ofs);
            p_parm->tbl_addr     = net_map_addr_with_parsflag(p_parm->tbl_addr, model_ofs, buf_ofs);
            p_parm->out_addr     = net_map_addr_with_parsflag(p_parm->out_addr, model_ofs, buf_ofs);
		}
		break;
    case AI_MODE_SOFTMAX: {
			KDRV_AI_SOFTMAX_PARM *p_parm = (KDRV_AI_SOFTMAX_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->out_addr    = net_map_addr_with_parsflag(p_parm->out_addr, model_ofs, buf_ofs);
		}
        break;
    case AI_MODE_PREPROC: {
			KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)p_head->parm_addr;
			p_parm->in_addr[0]     = net_map_addr_with_parsflag(p_parm->in_addr[0], model_ofs, buf_ofs);
            p_parm->in_addr[1]     = net_map_addr_with_parsflag(p_parm->in_addr[1], model_ofs, buf_ofs);
            p_parm->in_addr[2]     = net_map_addr_with_parsflag(p_parm->in_addr[2], model_ofs, buf_ofs);
			p_parm->out_addr[0]    = net_map_addr_with_parsflag(p_parm->out_addr[0], model_ofs, buf_ofs);
            p_parm->out_addr[1]    = net_map_addr_with_parsflag(p_parm->out_addr[1], model_ofs, buf_ofs);
            p_parm->out_addr[2]    = net_map_addr_with_parsflag(p_parm->out_addr[2], model_ofs, buf_ofs);
		}
		break;
	default:
		break;
	}
}
#endif

#if SUPPORT_DEPTHWISE
VOID nvt_ai_map_ai_multi_ll_addr(UINT32 parm_addr, UINT32 parm_va_ofs, UINT32 parm_pa_ofs, UINT32 model_pa_ofs, UINT32 buf_pa_ofs, UINT32 net_id)
{
	KDRV_AI_LL_HEAD *p_head = (KDRV_AI_LL_HEAD *)parm_addr;
	UINT64 *p_ll_end_cmd;
	UINT32 map_addr = 0, next_ll_addr = 0;
	CNN_LL_PARM *p_cnn_ll_parm;
	NUE_LL_PARM *p_nue_ll_parm;
	NUE2_LL_PARM *p_nue2_ll_parm;
	UINT32 parm_start_addr = 0;
	UINT32 end_flg = 0;

	p_head->parm_addr += parm_va_ofs;
	p_ll_end_cmd = (UINT64 *)(p_head->parm_addr + p_head->parm_size - sizeof(UINT64));
	next_ll_addr = nvt_ai_get_nextll_addr_cmd(*p_ll_end_cmd);
	if (next_ll_addr != 0) {
		next_ll_addr += parm_pa_ofs;
        *p_ll_end_cmd = nvt_ai_set_nextll_addr_cmd(*p_ll_end_cmd, next_ll_addr);
	}

	parm_start_addr = p_head->parm_addr;
	while (end_flg == 0) {
		UINT32 ll_cmd_tail = p_head->parm_addr + p_head->parm_size - sizeof(UINT64);
		switch (p_head->eng) {
		case AI_ENG_CNN:
		case AI_ENG_CNN2:
			p_cnn_ll_parm = (CNN_LL_PARM*)parm_start_addr;
			map_addr = p_cnn_ll_parm->input.bit.addr;
			p_cnn_ll_parm->input.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_cnn_ll_parm->interm_in.bit.addr;
			p_cnn_ll_parm->interm_in.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_cnn_ll_parm->output[0].bit.addr;
			p_cnn_ll_parm->output[0].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_cnn_ll_parm->output[1].bit.addr;
			p_cnn_ll_parm->output[1].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_cnn_ll_parm->weight.bit.addr;
			p_cnn_ll_parm->weight.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_cnn_ll_parm->kmean.bit.addr;
			p_cnn_ll_parm->kmean.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_cnn_ll_parm->bias_bnscal.bit.addr;
			p_cnn_ll_parm->bias_bnscal.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			// search next ll or null command
			parm_start_addr = parm_start_addr + sizeof(CNN_LL_PARM);
			break;
		case AI_ENG_NUE:
			p_nue_ll_parm = (NUE_LL_PARM*)parm_start_addr;
			map_addr = p_nue_ll_parm->input.bit.addr;
			p_nue_ll_parm->input.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue_ll_parm->elt_in.bit.addr;
			p_nue_ll_parm->elt_in.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue_ll_parm->roi_in.bit.addr;
			p_nue_ll_parm->roi_in.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue_ll_parm->output.bit.addr;
			p_nue_ll_parm->output.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);

			map_addr = p_nue_ll_parm->sv.bit.addr;
			p_nue_ll_parm->sv.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue_ll_parm->alpha.bit.addr;
			p_nue_ll_parm->alpha.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue_ll_parm->rho.bit.addr;
			p_nue_ll_parm->rho.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue_ll_parm->kmean.bit.addr;
			p_nue_ll_parm->kmean.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			// search next ll or null command
			parm_start_addr = parm_start_addr + sizeof(NUE_LL_PARM);
			break;
		case AI_ENG_NUE2:
			p_nue2_ll_parm = (NUE2_LL_PARM*)parm_start_addr;
			map_addr = p_nue2_ll_parm->input[0].bit.addr;
			p_nue2_ll_parm->input[0].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue2_ll_parm->input[1].bit.addr;
			p_nue2_ll_parm->input[1].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue2_ll_parm->input[2].bit.addr;
			p_nue2_ll_parm->input[2].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue2_ll_parm->output[0].bit.addr;
			p_nue2_ll_parm->output[0].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue2_ll_parm->output[1].bit.addr;
			p_nue2_ll_parm->output[1].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			map_addr = p_nue2_ll_parm->output[2].bit.addr;
			p_nue2_ll_parm->output[2].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
			// search next ll or null command
			parm_start_addr = parm_start_addr + sizeof(NUE2_LL_PARM);
			break;
		default:
			DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
			break;
		}
		while (1) {
			UINT64* p_tmp_cmd = (UINT64*)parm_start_addr;
			UINT32 cmd_type = (UINT32)(p_tmp_cmd[0] >> 60);

			parm_start_addr = parm_start_addr + sizeof(UINT64);
			// if temp addr is over the end of the parm addr, then set end flg and break the loop
			if (parm_start_addr > ll_cmd_tail) {
				end_flg = 1;
				break;
			}
			// if command type is next ll, then parse the next ll addr
			if (cmd_type == 2) {
				next_ll_addr = nvt_ai_get_nextll_addr_cmd(*p_tmp_cmd);
				if (next_ll_addr != 0) {
					next_ll_addr += parm_pa_ofs;
					*p_tmp_cmd = nvt_ai_set_nextll_addr_cmd(*p_tmp_cmd, next_ll_addr);
				}
				break;
			}
		}
	}
}

VOID nvt_ai_unmap_ai_multi_ll_addr(UINT32 parm_addr, UINT32 parm_va_ofs, UINT32 parm_pa_ofs, UINT32 model_pa_ofs, UINT32 buf_pa_ofs, UINT32 model_pa_end, UINT32 buf_pa_end, UINT32 net_id)
{
	KDRV_AI_LL_HEAD *p_head = (KDRV_AI_LL_HEAD *)parm_addr;
	UINT64 *p_ll_end_cmd;
	UINT32 next_ll_addr = 0;
	CNN_LL_PARM *p_cnn_ll_parm;
	NUE_LL_PARM *p_nue_ll_parm;
	NUE2_LL_PARM *p_nue2_ll_parm;
	UINT32 parm_start_addr = 0;
	UINT32 end_flg = 0;

	p_ll_end_cmd = (UINT64 *)(p_head->parm_addr + p_head->parm_size - sizeof(UINT64));
	next_ll_addr = nvt_ai_get_nextll_addr_cmd(*p_ll_end_cmd);
	if (next_ll_addr != 0) {
		next_ll_addr -= parm_pa_ofs;
		*p_ll_end_cmd = nvt_ai_set_nextll_addr_cmd(*p_ll_end_cmd, next_ll_addr);
	}

	parm_start_addr = p_head->parm_addr;

	while (end_flg == 0) {
		UINT32 ll_cmd_tail = p_head->parm_addr + p_head->parm_size - sizeof(UINT64);

		switch (p_head->eng) {
		case AI_ENG_CNN:
		case AI_ENG_CNN2:
			p_cnn_ll_parm = (CNN_LL_PARM*)parm_start_addr;
			p_cnn_ll_parm->input.bit.addr = net_unmap_addr(p_cnn_ll_parm->input.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_cnn_ll_parm->interm_in.bit.addr = net_unmap_addr(p_cnn_ll_parm->interm_in.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_cnn_ll_parm->output[0].bit.addr = net_unmap_addr(p_cnn_ll_parm->output[0].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_cnn_ll_parm->output[1].bit.addr = net_unmap_addr(p_cnn_ll_parm->output[1].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_cnn_ll_parm->weight.bit.addr = net_unmap_addr(p_cnn_ll_parm->weight.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_cnn_ll_parm->kmean.bit.addr = net_unmap_addr(p_cnn_ll_parm->kmean.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_cnn_ll_parm->bias_bnscal.bit.addr = net_unmap_addr(p_cnn_ll_parm->bias_bnscal.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);

			// search next ll or null command
			parm_start_addr = parm_start_addr + sizeof(CNN_LL_PARM);
			break;
		case AI_ENG_NUE:
			p_nue_ll_parm = (NUE_LL_PARM*)parm_start_addr;

			p_nue_ll_parm->input.bit.addr  = net_unmap_addr(p_nue_ll_parm->input.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue_ll_parm->elt_in.bit.addr = net_unmap_addr(p_nue_ll_parm->elt_in.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue_ll_parm->roi_in.bit.addr = net_unmap_addr(p_nue_ll_parm->roi_in.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue_ll_parm->output.bit.addr = net_unmap_addr(p_nue_ll_parm->output.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue_ll_parm->sv.bit.addr     = net_unmap_addr(p_nue_ll_parm->sv.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue_ll_parm->alpha.bit.addr  = net_unmap_addr(p_nue_ll_parm->alpha.bit.addr , model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue_ll_parm->rho.bit.addr    = net_unmap_addr(p_nue_ll_parm->rho.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue_ll_parm->kmean.bit.addr  = net_unmap_addr(p_nue_ll_parm->kmean.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);

			// search next ll or null command
			parm_start_addr = parm_start_addr + sizeof(NUE_LL_PARM);
			break;
		case AI_ENG_NUE2:
			p_nue2_ll_parm = (NUE2_LL_PARM*)parm_start_addr;
			p_nue2_ll_parm->input[0].bit.addr  = net_unmap_addr(p_nue2_ll_parm->input[0].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue2_ll_parm->input[1].bit.addr  = net_unmap_addr(p_nue2_ll_parm->input[1].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue2_ll_parm->input[2].bit.addr  = net_unmap_addr(p_nue2_ll_parm->input[2].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue2_ll_parm->output[0].bit.addr = net_unmap_addr(p_nue2_ll_parm->output[0].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue2_ll_parm->output[1].bit.addr = net_unmap_addr(p_nue2_ll_parm->output[1].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
			p_nue2_ll_parm->output[2].bit.addr = net_unmap_addr(p_nue2_ll_parm->output[2].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);

			// search next ll or null command
			parm_start_addr = parm_start_addr + sizeof(NUE2_LL_PARM);
			break;
		default:
			DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
			break;
		}
		while (1) {
			UINT64* p_tmp_cmd = (UINT64*)parm_start_addr;
			UINT32 cmd_type = (UINT32)(p_tmp_cmd[0] >> 60);

			parm_start_addr = parm_start_addr + sizeof(UINT64);

			// if temp addr is over the end of the parm addr, then set end flg and break the loop
			if (parm_start_addr > ll_cmd_tail) {
				end_flg = 1;
				break;
			}
			// if command type is next ll, then parse the next ll addr
			if (cmd_type == 2) {
				next_ll_addr = nvt_ai_get_nextll_addr_cmd(*p_tmp_cmd);
				if (next_ll_addr != 0) {
					next_ll_addr -= parm_pa_ofs;
					*p_tmp_cmd = nvt_ai_set_nextll_addr_cmd(*p_tmp_cmd, next_ll_addr);
				}
				break;
			}
		}
	}
	p_head->parm_addr -= parm_va_ofs;
}
#endif
#if CNN_25_MATLAB
VOID nvt_ai_map_ai_ll_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id)
#else
VOID nvt_ai_map_ai_ll_addr(UINT32 parm_addr, UINT32 parm_va_ofs, UINT32 parm_pa_ofs, UINT32 model_pa_ofs, UINT32 buf_pa_ofs, UINT32 net_id)
#endif
{
	KDRV_AI_LL_HEAD *p_head = (KDRV_AI_LL_HEAD *)parm_addr;
#if LL_SUPPORT_FIRST_LAYER
#else
	UINT64 *p_ll_cmd;
#endif
	UINT64 *p_ll_end_cmd;
	UINT32 map_addr = 0, next_ll_addr = 0;
#if LL_SUPPORT_FIRST_LAYER
	CNN_LL_PARM *p_cnn_ll_parm;
	NUE_LL_PARM *p_nue_ll_parm;
	NUE2_LL_PARM *p_nue2_ll_parm;
#else
	const UINT64 addr_mask 	= (UINT64)0x00000000ffffffff;
	const UINT64 cmd_mask  	= (UINT64)0xffffffff00000000;
#endif
#if CNN_25_MATLAB
	p_head->parm_addr += parm_ofs;
#else
	p_head->parm_addr += parm_va_ofs;
#endif
#if LL_SUPPORT_FIRST_LAYER
#else
	p_ll_cmd = (UINT64 *)p_head->parm_addr;
#endif
	p_ll_end_cmd = (UINT64 *)(p_head->parm_addr + p_head->parm_size - sizeof(UINT64));
	next_ll_addr = nvt_ai_get_nextll_addr_cmd(*p_ll_end_cmd);
	if (next_ll_addr != 0) {
#if CNN_25_MATLAB
		next_ll_addr += parm_ofs;
        *p_ll_end_cmd = nvt_ai_set_nextll_addr_cmd(*p_ll_end_cmd, nvt_ai_va2pa(next_ll_addr));
#else	
		next_ll_addr += parm_pa_ofs;
        *p_ll_end_cmd = nvt_ai_set_nextll_addr_cmd(*p_ll_end_cmd, next_ll_addr);
		//DBG_IND("ofs: parm_va_ofs: 0x%08X, parm_pa_ofs: 0x%08X,, model_pa_ofs: 0x%08X,, buf_pa_ofs: 0x%08X\n", (unsigned int)parm_va_ofs, (unsigned int)parm_pa_ofs, (unsigned int)model_pa_ofs, (unsigned int)buf_pa_ofs);
#endif
	}

#if LL_SUPPORT_FIRST_LAYER
#if CNN_25_MATLAB
	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(CNN_LL_PARM));
		p_cnn_ll_parm = (CNN_LL_PARM*)p_head->parm_addr;
		map_addr = p_cnn_ll_parm->input.bit.addr;
		p_cnn_ll_parm->input.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		//map_addr = p_cnn_ll_parm->input[1].bit.addr;
		//p_cnn_ll_parm->input[1].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_ofs, buf_ofs);
		//map_addr = p_cnn_ll_parm->input[2].bit.addr;
		//p_cnn_ll_parm->input[2].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_ofs, buf_ofs);
		map_addr = p_cnn_ll_parm->interm_in.bit.addr;
		p_cnn_ll_parm->interm_in.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_cnn_ll_parm->output[0].bit.addr;
		p_cnn_ll_parm->output[0].bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_cnn_ll_parm->output[1].bit.addr;
		p_cnn_ll_parm->output[1].bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));

		map_addr = p_cnn_ll_parm->weight.bit.addr;
		p_cnn_ll_parm->weight.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_cnn_ll_parm->kmean.bit.addr;
		p_cnn_ll_parm->kmean.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_cnn_ll_parm->bias_bnscal.bit.addr;
		p_cnn_ll_parm->bias_bnscal.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
#if LL_BASE_TEST
		DBG_DUMP("CNN - 0x%x\n", test_ofs);
		if (p_cnn_ll_parm->input.bit.addr != 0) {
			p_cnn_ll_parm->input.bit.addr -= test_ofs;
		}
		if (p_cnn_ll_parm->interm_in.bit.addr != 0) {
			p_cnn_ll_parm->interm_in.bit.addr -= test_ofs;
		}
		if (p_cnn_ll_parm->output[0].bit.addr != 0) {
			p_cnn_ll_parm->output[0].bit.addr -= test_ofs;
		}
		if (p_cnn_ll_parm->output[1].bit.addr != 0) {
			p_cnn_ll_parm->output[1].bit.addr -= test_ofs;
		}
#endif
		break;
	case AI_ENG_NUE:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE_LL_PARM));
		p_nue_ll_parm = (NUE_LL_PARM*)p_head->parm_addr;
		map_addr = p_nue_ll_parm->input.bit.addr;
		p_nue_ll_parm->input.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue_ll_parm->elt_in.bit.addr;
		p_nue_ll_parm->elt_in.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue_ll_parm->roi_in.bit.addr;
		p_nue_ll_parm->roi_in.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue_ll_parm->output.bit.addr;
		p_nue_ll_parm->output.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));

		map_addr = p_nue_ll_parm->sv.bit.addr;
		p_nue_ll_parm->sv.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue_ll_parm->alpha.bit.addr;
		p_nue_ll_parm->alpha.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue_ll_parm->rho.bit.addr;
		p_nue_ll_parm->rho.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue_ll_parm->kmean.bit.addr;
		p_nue_ll_parm->kmean.bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
#if LL_BASE_TEST
		DBG_DUMP("NUE - 0x%x\n", test_ofs);
		if (p_nue_ll_parm->input.bit.addr != 0) {
			p_nue_ll_parm->input.bit.addr -= test_ofs;
		}
		if (p_nue_ll_parm->elt_in.bit.addr != 0) {
			p_nue_ll_parm->elt_in.bit.addr -= test_ofs;
		}
		if (p_nue_ll_parm->roi_in.bit.addr != 0) {
			p_nue_ll_parm->roi_in.bit.addr -= test_ofs;
		}
		if (p_nue_ll_parm->output.bit.addr != 0) {
			p_nue_ll_parm->output.bit.addr -= test_ofs;
		}
#endif
		break;
	case AI_ENG_NUE2:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE2_LL_PARM));
		p_nue2_ll_parm = (NUE2_LL_PARM*)p_head->parm_addr;
		map_addr = p_nue2_ll_parm->input[0].bit.addr;
		p_nue2_ll_parm->input[0].bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue2_ll_parm->input[1].bit.addr;
		p_nue2_ll_parm->input[1].bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue2_ll_parm->input[2].bit.addr;
		p_nue2_ll_parm->input[2].bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue2_ll_parm->output[0].bit.addr;
		p_nue2_ll_parm->output[0].bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue2_ll_parm->output[1].bit.addr;
		p_nue2_ll_parm->output[1].bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
		map_addr = p_nue2_ll_parm->output[2].bit.addr;
		p_nue2_ll_parm->output[2].bit.addr = (map_addr == 0) ? 0 : nvt_ai_va2pa(net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs));
#if LL_BASE_TEST
		DBG_DUMP("NUE2 - 0x%x\n", test_ofs);
		if (p_nue2_ll_parm->input[0].bit.addr != 0) {
			p_nue2_ll_parm->input[0].bit.addr -= test_ofs;
		}
		if (p_nue2_ll_parm->input[1].bit.addr != 0) {
			p_nue2_ll_parm->input[1].bit.addr -= test_ofs;
		}
		if (p_nue2_ll_parm->input[2].bit.addr != 0) {
			p_nue2_ll_parm->input[2].bit.addr -= test_ofs;
		}
		if (p_nue2_ll_parm->output[0].bit.addr != 0) {
			p_nue2_ll_parm->output[0].bit.addr -= test_ofs;
		}
		if (p_nue2_ll_parm->output[1].bit.addr != 0) {
			p_nue2_ll_parm->output[1].bit.addr -= test_ofs;
		}
		if (p_nue2_ll_parm->output[2].bit.addr != 0) {
			p_nue2_ll_parm->output[2].bit.addr -= test_ofs;
		}
#endif
		break;
	default:
		DBG_WRN("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}
#else
	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(CNN_LL_PARM));
		p_cnn_ll_parm = (CNN_LL_PARM*)p_head->parm_addr;
		map_addr = p_cnn_ll_parm->input.bit.addr;
		p_cnn_ll_parm->input.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		//map_addr = p_cnn_ll_parm->input[1].bit.addr;
		//p_cnn_ll_parm->input[1].bit.addr =  net_map_addr_with_parsflag(map_addr, model_ofs, buf_ofs);
		//map_addr = p_cnn_ll_parm->input[2].bit.addr;
		//p_cnn_ll_parm->input[2].bit.addr =  net_map_addr_with_parsflag(map_addr, model_ofs, buf_ofs);
		map_addr = p_cnn_ll_parm->interm_in.bit.addr;
		p_cnn_ll_parm->interm_in.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_cnn_ll_parm->output[0].bit.addr;
		p_cnn_ll_parm->output[0].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_cnn_ll_parm->output[1].bit.addr;
		p_cnn_ll_parm->output[1].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);

		map_addr = p_cnn_ll_parm->weight.bit.addr;
		p_cnn_ll_parm->weight.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_cnn_ll_parm->kmean.bit.addr;
		p_cnn_ll_parm->kmean.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_cnn_ll_parm->bias_bnscal.bit.addr;
		p_cnn_ll_parm->bias_bnscal.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);

		break;
	case AI_ENG_NUE:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE_LL_PARM));
		p_nue_ll_parm = (NUE_LL_PARM*)p_head->parm_addr;
		map_addr = p_nue_ll_parm->input.bit.addr;
		p_nue_ll_parm->input.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->elt_in.bit.addr;
		p_nue_ll_parm->elt_in.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->roi_in.bit.addr;
		p_nue_ll_parm->roi_in.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->output.bit.addr;
		p_nue_ll_parm->output.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);

		map_addr = p_nue_ll_parm->sv.bit.addr;
		p_nue_ll_parm->sv.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->alpha.bit.addr;
		p_nue_ll_parm->alpha.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->rho.bit.addr;
		p_nue_ll_parm->rho.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->kmean.bit.addr;
		p_nue_ll_parm->kmean.bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		break;
	case AI_ENG_NUE2:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE2_LL_PARM));
		p_nue2_ll_parm = (NUE2_LL_PARM*)p_head->parm_addr;
		map_addr = p_nue2_ll_parm->input[0].bit.addr;
		p_nue2_ll_parm->input[0].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue2_ll_parm->input[1].bit.addr;
#if CNN_MULTI_INPUT // wait for Charles provides new fixed model , 11/24
		if (kflow_ai_net_global.g_ai_is_batch[net_id]) {
			//DBG_ERR("tmp hardcode (NUE2)input[1].bit.addr to 0 <<<<<<<<<<<<<<\r\n");
			p_nue2_ll_parm->input[1].bit.addr = 0;
		} else {
			p_nue2_ll_parm->input[1].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		}
#else
		p_nue2_ll_parm->input[1].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
#endif
		map_addr = p_nue2_ll_parm->input[2].bit.addr;
		p_nue2_ll_parm->input[2].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue2_ll_parm->output[0].bit.addr;
		p_nue2_ll_parm->output[0].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue2_ll_parm->output[1].bit.addr;
		p_nue2_ll_parm->output[1].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue2_ll_parm->output[2].bit.addr;
		p_nue2_ll_parm->output[2].bit.addr =  net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		break;
	default:
		DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}
#endif
#else
	while (1) {
		if ((*p_ll_cmd) == nvt_ai_ll_nextupd_cmd(NN_GEN_NO_ADDR_UPD_TYPE)) {
			*p_ll_cmd = nvt_ai_ll_nextupd_cmd(nvt_ai_va2pa((UINT32)(p_ll_cmd + 1)));
			break;
		} else if (((*p_ll_cmd) & (UINT64)NN_GEN_MODEL_ADDR_TYPE) || ((*p_ll_cmd) & (UINT64)NN_GEN_BUF_ADDR_TYPE)) {
			map_addr = (UINT32)((*p_ll_cmd) & (UINT64)addr_mask);
			*p_ll_cmd = ((*p_ll_cmd) & (UINT64)cmd_mask);
			map_addr = net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
			map_addr = nvt_ai_va2pa(map_addr);
			*p_ll_cmd = (*p_ll_cmd) + map_addr;
		} else if (((*p_ll_cmd) & 0xffffffff) == NN_GEN_NULL_ADDR_TYPE) {
			//null address
		} else {
			DBG_WRN("unknown address type: %08x\r\n", (unsigned int)((*p_ll_cmd) & (UINT64)0xffffffff));
			break;
		}
		p_ll_cmd++;
	}
#endif
}

#if CNN_25_MATLAB
VOID nvt_ai_map_drv_addr(NN_MODE mode, UINT32 parm_addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id)
{
	NN_CPU_PARM *p_cpu_parm;
	NN_FC_POST_PARM *p_fc_post_parm;
	NN_POOL_PARM *p_pool_parm;
	NN_CUSTOM_PARM *p_cust_head;
	NN_BNSCALE_PARM *p_bn_scl_parm;
	//NN_SOFTMAX_PARM *p_softmax_parm;
	NN_POSTPROC_PARM *p_postproc_parm;

	switch (mode) {
	case NN_CONV:
	case NN_DECONV:
	case NN_MATMUL:
	case NN_CORR:
		break;
	case NN_SVM:
	case NN_FC:
	case NN_ROIPOOLING:
	case NN_ELTWISE:
	case NN_REORGANIZATION:
	case NN_RESHAPE:
		break;
	case NN_PROPOSAL:
		p_cpu_parm = (NN_CPU_PARM *)parm_addr;
		p_cpu_parm->addr_in     = net_map_addr_with_parsflag(p_cpu_parm->addr_in, buf_ofs, model_ofs);
		p_cpu_parm->addr_out    = net_map_addr_with_parsflag(p_cpu_parm->addr_out, buf_ofs, model_ofs);
		break;
	case NN_POSTPROC:
		p_postproc_parm = (NN_POSTPROC_PARM *)parm_addr;
		p_postproc_parm->in_addr = net_map_addr_with_parsflag(p_postproc_parm->in_addr, buf_ofs, model_ofs);
		p_postproc_parm->out_addr = net_map_addr_with_parsflag(p_postproc_parm->out_addr, buf_ofs, model_ofs);
		p_postproc_parm->tmp_addr = net_map_addr_with_parsflag(p_postproc_parm->tmp_addr, buf_ofs, model_ofs);
		break;
    /*
	case NN_SOFTMAX:
		p_softmax_parm = (NN_SOFTMAX_PARM *)parm_addr;
		p_softmax_parm->in_addr		= net_map_addr_with_parsflag(p_softmax_parm->in_addr, buf_ofs, model_ofs);
		p_softmax_parm->out_addr	= net_map_addr_with_parsflag(p_softmax_parm->out_addr, buf_ofs, model_ofs);
		break;
	case NN_PREPROC:
		//DBG_ERR("pre-processing can't be in kernel space\r\n");
		break;
    */
	case NN_FC_POST:
		p_fc_post_parm = (NN_FC_POST_PARM*)parm_addr;
		p_fc_post_parm->addr_in     = net_map_addr_with_parsflag(p_fc_post_parm->addr_in, buf_ofs, model_ofs);
		p_fc_post_parm->addr_out    = net_map_addr_with_parsflag(p_fc_post_parm->addr_out, buf_ofs, model_ofs);
		p_fc_post_parm->bias_addr   = net_map_addr_with_parsflag(p_fc_post_parm->bias_addr, buf_ofs, model_ofs);
		break;
	case NN_POOL:
		p_pool_parm = (NN_POOL_PARM*)parm_addr;
		p_pool_parm->in_addr	= net_map_addr_with_parsflag(p_pool_parm->in_addr, buf_ofs, model_ofs);
		p_pool_parm->out_addr	= net_map_addr_with_parsflag(p_pool_parm->out_addr, buf_ofs, model_ofs);
		break;
	case NN_CUSTOMER:
		p_cust_head = (NN_CUSTOM_PARM*)parm_addr;
		p_cust_head->input.va	= net_map_addr_with_parsflag(p_cust_head->input.va, buf_ofs, model_ofs);
		p_cust_head->output.va	= net_map_addr_with_parsflag(p_cust_head->output.va, buf_ofs, model_ofs);
		p_cust_head->model.va	= net_map_addr_with_parsflag(p_cust_head->model.va, buf_ofs, model_ofs);
		p_cust_head->input.pa   = nvt_ai_va2pa(p_cust_head->input.va);
		p_cust_head->output.pa  = nvt_ai_va2pa(p_cust_head->output.va);
		p_cust_head->model.pa   = nvt_ai_va2pa(p_cust_head->model.va);
		break;
	case NN_BNSCALE:
		p_bn_scl_parm = (NN_BNSCALE_PARM*)parm_addr;
		p_bn_scl_parm->in_addr		= net_map_addr_with_parsflag(p_bn_scl_parm->in_addr, buf_ofs, model_ofs);
		p_bn_scl_parm->out_addr		= net_map_addr_with_parsflag(p_bn_scl_parm->out_addr, buf_ofs, model_ofs);
		p_bn_scl_parm->mean_addr	= net_map_addr_with_parsflag(p_bn_scl_parm->mean_addr, buf_ofs, model_ofs);
		p_bn_scl_parm->alpha_addr	= net_map_addr_with_parsflag(p_bn_scl_parm->alpha_addr, buf_ofs, model_ofs);
		p_bn_scl_parm->beta_addr	= net_map_addr_with_parsflag(p_bn_scl_parm->beta_addr, buf_ofs, model_ofs);
		break;
	default :
		break;
	}
}
#else
VOID nvt_ai_map_drv_addr(NN_MODE mode, UINT32 parm_addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id)
{
	NN_CPU_PARM *p_cpu_parm;
	NN_FC_POST_PARM *p_fc_post_parm;
	NN_POOL_PARM *p_pool_parm;
	NN_CUSTOM_PARM *p_cust_head;
	NN_BNSCALE_PARM *p_bn_scl_parm;
	//NN_SOFTMAX_PARM *p_softmax_parm;
	NN_POSTPROC_PARM *p_postproc_parm;
	NN_LAYER_NORMALIZATION_PARM *p_layerNorm_parm;
	
	switch (mode) {
	case NN_CONV:
	case NN_DECONV:
	case NN_MATMUL:
	case NN_CORR:
		break;
	case NN_SVM:
	case NN_FC:
	case NN_ROIPOOLING:
	case NN_ELTWISE:
	case NN_REORGANIZATION:
	case NN_RESHAPE:
		break;
	case NN_PROPOSAL:
		p_cpu_parm = (NN_CPU_PARM *)parm_addr;
		p_cpu_parm->addr_in     = net_map_addr_with_parsflag(p_cpu_parm->addr_in, model_ofs, buf_ofs);
		p_cpu_parm->addr_out    = net_map_addr_with_parsflag(p_cpu_parm->addr_out, model_ofs, buf_ofs);
		break;
	case NN_POSTPROC:
		p_postproc_parm = (NN_POSTPROC_PARM *)parm_addr;
		p_postproc_parm->in_addr = net_map_addr_with_parsflag(p_postproc_parm->in_addr, model_ofs, buf_ofs);
		p_postproc_parm->out_addr = net_map_addr_with_parsflag(p_postproc_parm->out_addr, model_ofs, buf_ofs);
		p_postproc_parm->tmp_addr = net_map_addr_with_parsflag(p_postproc_parm->tmp_addr, model_ofs, buf_ofs);
		break;
    /*
	case NN_SOFTMAX:
		p_softmax_parm = (NN_SOFTMAX_PARM *)parm_addr;
		p_softmax_parm->in_addr		= net_map_addr_with_parsflag(p_softmax_parm->in_addr, buf_ofs, model_ofs);
		p_softmax_parm->out_addr	= net_map_addr_with_parsflag(p_softmax_parm->out_addr, buf_ofs, model_ofs);
		break;
	case NN_PREPROC:
		//DBG_ERR("pre-processing can't be in kernel space\r\n");
		break;
    */
	case NN_FC_POST:
		p_fc_post_parm = (NN_FC_POST_PARM*)parm_addr;
#if AI_V4
		p_fc_post_parm->in_addr     = net_map_addr_with_parsflag(p_fc_post_parm->in_addr, model_ofs, buf_ofs);
		p_fc_post_parm->out_addr    = net_map_addr_with_parsflag(p_fc_post_parm->out_addr, model_ofs, buf_ofs);
#else
		p_fc_post_parm->addr_in     = net_map_addr_with_parsflag(p_fc_post_parm->addr_in, model_ofs, buf_ofs);
		p_fc_post_parm->addr_out    = net_map_addr_with_parsflag(p_fc_post_parm->addr_out, model_ofs, buf_ofs);
#endif
		p_fc_post_parm->bias_addr   = net_map_addr_with_parsflag(p_fc_post_parm->bias_addr, model_ofs, buf_ofs);
		break;
	case NN_POOL:
		p_pool_parm = (NN_POOL_PARM*)parm_addr;
		p_pool_parm->in_addr	= net_map_addr_with_parsflag(p_pool_parm->in_addr, model_ofs, buf_ofs);
		p_pool_parm->out_addr	= net_map_addr_with_parsflag(p_pool_parm->out_addr, model_ofs, buf_ofs);
		break;
	case NN_LAYER_NORMALIZATION:
		p_layerNorm_parm = (NN_LAYER_NORMALIZATION_PARM*)parm_addr;
		p_layerNorm_parm->in_addr		= net_map_addr_with_parsflag(p_layerNorm_parm->in_addr, model_ofs, buf_ofs);
		p_layerNorm_parm->tmp_addr		= net_map_addr_with_parsflag(p_layerNorm_parm->tmp_addr, model_ofs, buf_ofs);
		p_layerNorm_parm->gamma_addr	= net_map_addr_with_parsflag(p_layerNorm_parm->gamma_addr, model_ofs, buf_ofs);
		p_layerNorm_parm->beta_addr		= net_map_addr_with_parsflag(p_layerNorm_parm->beta_addr, model_ofs, buf_ofs);
		p_layerNorm_parm->out_addr		= net_map_addr_with_parsflag(p_layerNorm_parm->out_addr, model_ofs, buf_ofs);
		break;
	case NN_CUSTOMER:
#if CUST_SUPPORT_MULTI_IO
		{
			NN_DATA* io_head = NULL;
			UINT32 i = 0;

			p_cust_head = (NN_CUSTOM_PARM*)parm_addr;

			// parsing input
			io_head = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM));
			for (i = 0; i < p_cust_head->input_num; i++) {
				io_head[i].va = net_map_addr_with_parsflag(io_head[i].va, model_ofs, buf_ofs);
				io_head[i].pa = nvt_ai_va2pa(io_head[i].va);
				if (io_head[i].pa == 0) {
					DBG_ERR("Cannot find pa by input va(%#x), idx(%d)\r\n", io_head[i].va, i);
					break;
				}
			}

			// parsing output
			io_head = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM) + p_cust_head->input_num*sizeof(NN_DATA));
			for (i = 0; i < p_cust_head->output_num; i++) {
				io_head[i].va = net_map_addr_with_parsflag(io_head[i].va, model_ofs, buf_ofs);
				io_head[i].pa = nvt_ai_va2pa(io_head[i].va);
				if (io_head[i].pa == 0) {
					DBG_ERR("Cannot find pa by output va(%#x), idx(%d)\r\n", io_head[i].va, i);
					break;
				}
			}

			// parsing model
			io_head = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM) + (p_cust_head->input_num + p_cust_head->output_num)*sizeof(NN_DATA));
			for (i = 0; i < p_cust_head->model_num; i++) {
				io_head[i].va = net_map_addr_with_parsflag(io_head[i].va, model_ofs, buf_ofs);
				io_head[i].pa = nvt_ai_va2pa(io_head[i].va);
				if (io_head[i].pa == 0) {
					DBG_ERR("Cannot find pa by model va(%#x), idx(%d)\r\n", io_head[i].va, i);
					break;
				}
			}
		}
#else
		p_cust_head = (NN_CUSTOM_PARM*)parm_addr;
		p_cust_head->input.va	= net_map_addr_with_parsflag(p_cust_head->input.va, model_ofs, buf_ofs);
		p_cust_head->output.va	= net_map_addr_with_parsflag(p_cust_head->output.va, model_ofs, buf_ofs);
		p_cust_head->model.va	= net_map_addr_with_parsflag(p_cust_head->model.va, model_ofs, buf_ofs);
		p_cust_head->input.pa   = nvt_ai_va2pa(p_cust_head->input.va);
		p_cust_head->output.pa  = nvt_ai_va2pa(p_cust_head->output.va);
		p_cust_head->model.pa   = nvt_ai_va2pa(p_cust_head->model.va);
#endif
		break;
	case NN_BNSCALE:
		p_bn_scl_parm = (NN_BNSCALE_PARM*)parm_addr;
		p_bn_scl_parm->in_addr		= net_map_addr_with_parsflag(p_bn_scl_parm->in_addr, model_ofs, buf_ofs);
		p_bn_scl_parm->out_addr		= net_map_addr_with_parsflag(p_bn_scl_parm->out_addr, model_ofs, buf_ofs);
		p_bn_scl_parm->mean_addr	= net_map_addr_with_parsflag(p_bn_scl_parm->mean_addr, model_ofs, buf_ofs);
		p_bn_scl_parm->alpha_addr	= net_map_addr_with_parsflag(p_bn_scl_parm->alpha_addr, model_ofs, buf_ofs);
		p_bn_scl_parm->beta_addr	= net_map_addr_with_parsflag(p_bn_scl_parm->beta_addr, model_ofs, buf_ofs);
		break;
	default :
		break;
	}
}

VOID nvt_ai_unmap_ai_app_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end, UINT32 net_id)
{
	KDRV_AI_APP_HEAD *p_head = (KDRV_AI_APP_HEAD *)parm_addr;

	switch (p_head->mode) {
	case AI_MODE_NEURAL: {
			KDRV_AI_NEURAL_PARM *p_parm = (KDRV_AI_NEURAL_PARM *)p_head->parm_addr;
			p_parm->in_addr                     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out0_addr                   = net_unmap_addr(p_parm->out0_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out1_addr                   = net_unmap_addr(p_parm->out1_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->in_interm_addr              = net_unmap_addr(p_parm->in_interm_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_interm_addr             = net_unmap_addr(p_parm->out_interm_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->tmp_buf_addr                = net_unmap_addr(p_parm->tmp_buf_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->conv.weight_addr            = net_unmap_addr(p_parm->conv.weight_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->conv.bias_addr              = net_unmap_addr(p_parm->conv.bias_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->conv.fcd.quanti_kmeans_addr = net_unmap_addr(p_parm->conv.fcd.quanti_kmeans_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->conv.fcd.p_vlc_code         = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_code - parm_ofs);
			p_parm->conv.fcd.p_vlc_valid        = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_valid - parm_ofs);
			p_parm->conv.fcd.p_vlc_ofs          = (UINT32 *)((UINT32)p_parm->conv.fcd.p_vlc_ofs - parm_ofs);
			p_parm->norm.bn_scl.bn_scale_addr   = net_unmap_addr(p_parm->norm.bn_scl.bn_scale_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->elt.addr                    = net_unmap_addr(p_parm->elt.addr, model_ofs, buf_ofs, model_end, buf_end);
		}
		break;
	case AI_MODE_ROIPOOL: {
			KDRV_AI_ROIPOOL_PARM *p_parm = (KDRV_AI_ROIPOOL_PARM *)p_head->parm_addr;
			p_parm->in_addr                     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_addr                    = net_unmap_addr(p_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->roi_ker.roi_addr            = net_unmap_addr(p_parm->roi_ker.roi_addr, model_ofs, buf_ofs, model_end, buf_end);
		}
		break;
	case AI_MODE_SVM: {
			KDRV_AI_SVM_PARM *p_parm = (KDRV_AI_SVM_PARM *)p_head->parm_addr;
			p_parm->in_addr                     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_addr                    = net_unmap_addr(p_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->svm_ker.sv_addr             = net_unmap_addr(p_parm->svm_ker.sv_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->svm_ker.alpha_addr          = net_unmap_addr(p_parm->svm_ker.alpha_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->fcd.quanti_kmeans_addr      = net_unmap_addr(p_parm->fcd.quanti_kmeans_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->fcd.p_vlc_code              = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_code - parm_ofs);
			p_parm->fcd.p_vlc_valid             = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_valid - parm_ofs);
			p_parm->fcd.p_vlc_ofs               = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_ofs - parm_ofs);
		}
		break;
	case AI_MODE_FC: {
			KDRV_AI_FC_PARM *p_parm = (KDRV_AI_FC_PARM *)p_head->parm_addr;
			p_parm->in_addr                     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_addr                    = net_unmap_addr(p_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->in_interm_addr              = net_unmap_addr(p_parm->in_interm_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_interm_addr             = net_unmap_addr(p_parm->out_interm_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->fc_ker.weight_addr          = net_unmap_addr(p_parm->fc_ker.weight_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->fc_ker.bias_addr            = net_unmap_addr(p_parm->fc_ker.bias_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->fcd.quanti_kmeans_addr      = net_unmap_addr(p_parm->fcd.quanti_kmeans_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->fcd.p_vlc_code              = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_code - parm_ofs);
			p_parm->fcd.p_vlc_valid             = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_valid - parm_ofs);
			p_parm->fcd.p_vlc_ofs               = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_ofs - parm_ofs);
		}
		break;
	case AI_MODE_PERMUTE: {
			KDRV_AI_PERMUTE_PARM *p_parm = (KDRV_AI_PERMUTE_PARM *)p_head->parm_addr;
			p_parm->in_addr                     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_addr                    = net_unmap_addr(p_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		}
		break;
	case AI_MODE_REORG: {
			KDRV_AI_REORG_PARM *p_parm = (KDRV_AI_REORG_PARM *)p_head->parm_addr;
			p_parm->in_addr                     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_addr                    = net_unmap_addr(p_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		}
		break;
    case AI_MODE_ANCHOR: {
			KDRV_AI_ANCHOR_PARM *p_parm = (KDRV_AI_ANCHOR_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->w_addr    = net_unmap_addr(p_parm->w_addr, model_ofs, buf_ofs, model_end, buf_end);
            p_parm->b_addr     = net_unmap_addr(p_parm->b_addr, model_ofs, buf_ofs, model_end, buf_end);
            p_parm->tbl_addr     = net_unmap_addr(p_parm->tbl_addr, model_ofs, buf_ofs, model_end, buf_end);
            p_parm->out_addr     = net_unmap_addr(p_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		}
		break;
    case AI_MODE_SOFTMAX: {
			KDRV_AI_SOFTMAX_PARM *p_parm = (KDRV_AI_SOFTMAX_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_addr    = net_unmap_addr(p_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		}
        break;
    case AI_MODE_PREPROC: {
			KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)p_head->parm_addr;
			p_parm->in_addr[0]     = net_unmap_addr(p_parm->in_addr[0], model_ofs, buf_ofs, model_end, buf_end);
            p_parm->in_addr[1]     = net_unmap_addr(p_parm->in_addr[1], model_ofs, buf_ofs, model_end, buf_end);
            p_parm->in_addr[2]     = net_unmap_addr(p_parm->in_addr[2], model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_addr[0]    = net_unmap_addr(p_parm->out_addr[0], model_ofs, buf_ofs, model_end, buf_end);
            p_parm->out_addr[1]    = net_unmap_addr(p_parm->out_addr[1], model_ofs, buf_ofs, model_end, buf_end);
            p_parm->out_addr[2]    = net_unmap_addr(p_parm->out_addr[2], model_ofs, buf_ofs, model_end, buf_end);
		}
		break;
	default:
		break;
	}
	
	p_head->parm_addr -= parm_ofs;
	if (p_head->stripe_head_addr != 0) {
		//multi-stripe engine
		p_head->stripe_head_addr -= parm_ofs;
	}
}

VOID nvt_ai_unmap_ai_ll_addr(UINT32 parm_addr, UINT32 parm_va_ofs, UINT32 parm_pa_ofs, UINT32 model_pa_ofs, UINT32 buf_pa_ofs, UINT32 model_pa_end, UINT32 buf_pa_end, UINT32 net_id)
{
	KDRV_AI_LL_HEAD *p_head = (KDRV_AI_LL_HEAD *)parm_addr;
#if CNN_25_MATLAB
	UINT64 *p_ll_cmd;
#endif
	UINT64 *p_ll_end_cmd;
	UINT32 next_ll_addr = 0;
	CNN_LL_PARM *p_cnn_ll_parm;
	NUE_LL_PARM *p_nue_ll_parm;
	NUE2_LL_PARM *p_nue2_ll_parm;

#if CNN_25_MATLAB
	p_ll_cmd = (UINT64 *)p_head->parm_addr;
#endif
	p_ll_end_cmd = (UINT64 *)(p_head->parm_addr + p_head->parm_size - sizeof(UINT64));
	next_ll_addr = nvt_ai_get_nextll_addr_cmd(*p_ll_end_cmd);
	if (next_ll_addr != 0) {
		next_ll_addr -= parm_pa_ofs;
		*p_ll_end_cmd = nvt_ai_set_nextll_addr_cmd(*p_ll_end_cmd, next_ll_addr);
	}

	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
#if CNN_25_MATLAB
		p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(CNN_LL_PARM));
#endif
		p_cnn_ll_parm = (CNN_LL_PARM*)p_head->parm_addr;
		p_cnn_ll_parm->input.bit.addr = net_unmap_addr(p_cnn_ll_parm->input.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_cnn_ll_parm->interm_in.bit.addr = net_unmap_addr(p_cnn_ll_parm->interm_in.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_cnn_ll_parm->output[0].bit.addr = net_unmap_addr(p_cnn_ll_parm->output[0].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_cnn_ll_parm->output[1].bit.addr = net_unmap_addr(p_cnn_ll_parm->output[1].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_cnn_ll_parm->weight.bit.addr = net_unmap_addr(p_cnn_ll_parm->weight.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_cnn_ll_parm->kmean.bit.addr = net_unmap_addr(p_cnn_ll_parm->kmean.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_cnn_ll_parm->bias_bnscal.bit.addr = net_unmap_addr(p_cnn_ll_parm->bias_bnscal.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		break;
	case AI_ENG_NUE:
#if CNN_25_MATLAB
		p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE_LL_PARM));
#endif
		p_nue_ll_parm = (NUE_LL_PARM*)p_head->parm_addr;
		p_nue_ll_parm->input.bit.addr  = net_unmap_addr(p_nue_ll_parm->input.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue_ll_parm->elt_in.bit.addr = net_unmap_addr(p_nue_ll_parm->elt_in.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue_ll_parm->roi_in.bit.addr = net_unmap_addr(p_nue_ll_parm->roi_in.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue_ll_parm->output.bit.addr = net_unmap_addr(p_nue_ll_parm->output.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue_ll_parm->sv.bit.addr     = net_unmap_addr(p_nue_ll_parm->sv.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue_ll_parm->alpha.bit.addr  = net_unmap_addr(p_nue_ll_parm->alpha.bit.addr , model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue_ll_parm->rho.bit.addr    = net_unmap_addr(p_nue_ll_parm->rho.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue_ll_parm->kmean.bit.addr  = net_unmap_addr(p_nue_ll_parm->kmean.bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		break;
	case AI_ENG_NUE2:
#if CNN_25_MATLAB
		p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE2_LL_PARM));
#endif
		p_nue2_ll_parm = (NUE2_LL_PARM*)p_head->parm_addr;
		p_nue2_ll_parm->input[0].bit.addr  = net_unmap_addr(p_nue2_ll_parm->input[0].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue2_ll_parm->input[1].bit.addr  = net_unmap_addr(p_nue2_ll_parm->input[1].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue2_ll_parm->input[2].bit.addr  = net_unmap_addr(p_nue2_ll_parm->input[2].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue2_ll_parm->output[0].bit.addr = net_unmap_addr(p_nue2_ll_parm->output[0].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue2_ll_parm->output[1].bit.addr = net_unmap_addr(p_nue2_ll_parm->output[1].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue2_ll_parm->output[2].bit.addr = net_unmap_addr(p_nue2_ll_parm->output[2].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		break;
	default:
		DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}


	p_head->parm_addr -= parm_va_ofs;

}

VOID nvt_ai_unmap_drv_addr(NN_MODE mode, UINT32 parm_addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end, UINT32 net_id)
{
	NN_CPU_PARM *p_cpu_parm;
	NN_FC_POST_PARM *p_fc_post_parm;
	NN_POOL_PARM *p_pool_parm;
	NN_CUSTOM_PARM *p_cust_head;
	NN_BNSCALE_PARM *p_bn_scl_parm;
	//NN_SOFTMAX_PARM *p_softmax_parm;
	NN_POSTPROC_PARM *p_postproc_parm;
	NN_LAYER_NORMALIZATION_PARM *p_layerNorm_parm;

	switch (mode) {
	case NN_CONV:
	case NN_DECONV:
	case NN_MATMUL:
	case NN_CORR:
		break;
	case NN_SVM:
	case NN_FC:
	case NN_ROIPOOLING:
	case NN_ELTWISE:
	case NN_REORGANIZATION:
	case NN_RESHAPE:
		break;
	case NN_PROPOSAL:
		p_cpu_parm = (NN_CPU_PARM *)parm_addr;
		p_cpu_parm->addr_in     = net_unmap_addr(p_cpu_parm->addr_in, model_ofs, buf_ofs, model_end, buf_end);
		p_cpu_parm->addr_out    = net_unmap_addr(p_cpu_parm->addr_out, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_POSTPROC:
		p_postproc_parm = (NN_POSTPROC_PARM *)parm_addr;
		p_postproc_parm->in_addr = net_unmap_addr(p_postproc_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_postproc_parm->out_addr = net_unmap_addr(p_postproc_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_postproc_parm->tmp_addr = net_unmap_addr(p_postproc_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
    /*
	case NN_SOFTMAX:
		p_softmax_parm = (NN_SOFTMAX_PARM *)parm_addr;
		p_softmax_parm->in_addr		= net_map_addr_with_parsflag(p_softmax_parm->in_addr, buf_ofs, model_ofs);
		p_softmax_parm->out_addr	= net_map_addr_with_parsflag(p_softmax_parm->out_addr, buf_ofs, model_ofs);
		break;
	case NN_PREPROC:
		//DBG_ERR("pre-processing can't be in kernel space\r\n");
		break;
    */
	case NN_FC_POST:
		p_fc_post_parm = (NN_FC_POST_PARM*)parm_addr;
#if AI_V4
		p_fc_post_parm->in_addr     = net_unmap_addr(p_fc_post_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_fc_post_parm->out_addr    = net_unmap_addr(p_fc_post_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
#else
		p_fc_post_parm->addr_in     = net_unmap_addr(p_fc_post_parm->addr_in, model_ofs, buf_ofs, model_end, buf_end);
		p_fc_post_parm->addr_out    = net_unmap_addr(p_fc_post_parm->addr_out, model_ofs, buf_ofs, model_end, buf_end);
#endif
		p_fc_post_parm->bias_addr   = net_unmap_addr(p_fc_post_parm->bias_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_POOL:
		p_pool_parm = (NN_POOL_PARM*)parm_addr;
		p_pool_parm->in_addr	= net_unmap_addr(p_pool_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_pool_parm->out_addr	= net_unmap_addr(p_pool_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_LAYER_NORMALIZATION:
		p_layerNorm_parm = (NN_LAYER_NORMALIZATION_PARM*)parm_addr;
		p_layerNorm_parm->in_addr		= net_unmap_addr(p_layerNorm_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_layerNorm_parm->tmp_addr		= net_unmap_addr(p_layerNorm_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_layerNorm_parm->gamma_addr	= net_unmap_addr(p_layerNorm_parm->gamma_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_layerNorm_parm->beta_addr		= net_unmap_addr(p_layerNorm_parm->beta_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_layerNorm_parm->out_addr		= net_unmap_addr(p_layerNorm_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_CUSTOMER:
#if CUST_SUPPORT_MULTI_IO
		{
			NN_DATA* io_head = NULL;
			UINT32 i = 0;

			p_cust_head = (NN_CUSTOM_PARM*)parm_addr;

			// parsing input
			io_head = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM));
			for (i = 0; i < p_cust_head->input_num; i++) {
				io_head[i].va = net_unmap_addr(io_head[i].va, model_ofs, buf_ofs, model_end, buf_end);
				io_head[i].pa = io_head[i].va;
			}

			// parsing output
			io_head = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM) + p_cust_head->input_num*sizeof(NN_DATA));
			for (i = 0; i < p_cust_head->output_num; i++) {
				io_head[i].va = net_unmap_addr(io_head[i].va, model_ofs, buf_ofs, model_end, buf_end);
				io_head[i].pa = io_head[i].va;
			}

			// parsing model
			io_head = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM) + (p_cust_head->input_num + p_cust_head->output_num)*sizeof(NN_DATA));
			for (i = 0; i < p_cust_head->model_num; i++) {
				io_head[i].va = net_unmap_addr(io_head[i].va, model_ofs, buf_ofs, model_end, buf_end);
				io_head[i].pa = io_head[i].va;
			}
		}
#else
		p_cust_head = (NN_CUSTOM_PARM*)parm_addr;
		p_cust_head->input.va	= net_unmap_addr(p_cust_head->input.va, model_ofs, buf_ofs, model_end, buf_end);
		p_cust_head->output.va	= net_unmap_addr(p_cust_head->output.va, model_ofs, buf_ofs, model_end, buf_end);
		p_cust_head->model.va	= net_unmap_addr(p_cust_head->model.va, model_ofs, buf_ofs, model_end, buf_end);
		p_cust_head->input.pa   = p_cust_head->input.va;
		p_cust_head->output.pa  = p_cust_head->output.va;
		p_cust_head->model.pa   = p_cust_head->model.va;
#endif
		break;
	case NN_BNSCALE:
		p_bn_scl_parm = (NN_BNSCALE_PARM*)parm_addr;
		p_bn_scl_parm->in_addr		= net_unmap_addr(p_bn_scl_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_bn_scl_parm->out_addr		= net_unmap_addr(p_bn_scl_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_bn_scl_parm->mean_addr	= net_unmap_addr(p_bn_scl_parm->mean_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_bn_scl_parm->alpha_addr	= net_unmap_addr(p_bn_scl_parm->alpha_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_bn_scl_parm->beta_addr	= net_unmap_addr(p_bn_scl_parm->beta_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	default :
		break;
	}
}
#endif
#if CNN_25_MATLAB
#define SYNC_MTCNN 1
ER nvt_ai_pars_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_IOMEM *p_io_mem;
	UINT32 parm_ofs, model_ofs, buff_ofs;
	UINT32 process_index = 0, idx = 0;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
#if 1
	UINT32 layer_index = 0;
#endif

	if ((p_mem == NULL) || (p_mem->user_parm.va == 0) || (p_mem->user_model.va == 0) || (p_mem->user_buff.va == 0) ) {
		DBG_ERR("invalid memory input\r\n");
		return E_CTX;
	}
	
	if (nvt_ai_is_mem_overflow(p_mem) || nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_pars_net fail...\r\n");
		return E_CTX;
	}

    er = nvt_ai_get_net_info(&net_info, p_mem->kerl_parm.va);
    if (er != E_OK) {
        DBG_ERR("nvt_ai_get_net_info fail...\r\n");
        return er;
    }

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	p_io_mem = net_info.p_io_mem;
	proc_layer_num = p_head->mode_ctrl_num;


	parm_ofs = ALIGN_CEIL_4(p_mem->kerl_parm.va);
	model_ofs = ALIGN_CEIL_4(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_model.va);
	buff_ofs = ALIGN_CEIL_4(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff.va);
#if 1
	for (layer_index = 0; layer_index < p_head->layer_num; layer_index++) {
		for (idx = 0; idx < NN_IMEM_NUM; idx++) {
			if (layer_index == 0) {
				continue;
			}
			p_io_mem[layer_index].SAI[idx].va = net_map_addr_with_parsflag(p_io_mem[layer_index].SAI[idx].va, buff_ofs, model_ofs);
			p_io_mem[layer_index].SAI[idx].pa = nvt_ai_va2pa(p_io_mem[layer_index].SAI[idx].va);
		}
		for (idx = 0; idx < NN_OMEM_NUM; idx++) {
			p_io_mem[layer_index].SAO[idx].va = net_map_addr_with_parsflag(p_io_mem[layer_index].SAO[idx].va, buff_ofs, model_ofs);
			p_io_mem[layer_index].SAO[idx].pa = nvt_ai_va2pa(p_io_mem[layer_index].SAO[idx].va);
		}
	}
#else
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		BOOL is_io_remmap = TRUE;
		UINT32 layer_index = p_mctrl[process_index].layer_index;
		for (idx = (process_index + 1); idx < proc_layer_num; idx++) {
			if (layer_index == p_mctrl[idx].layer_index) {
				is_io_remmap = FALSE;
				break;
			} else {
				is_io_remmap = TRUE;
			}
		}

		if (!is_io_remmap) {
			continue;
		}

		for (idx = 0; idx < NN_IMEM_NUM; idx++) {
			if (layer_index == 0) {
				continue;
			}
			p_io_mem[layer_index].SAI[idx].va = net_map_addr_with_parsflag(p_io_mem[layer_index].SAI[idx].va, buff_ofs, model_ofs);
			p_io_mem[layer_index].SAI[idx].pa = nvt_ai_va2pa(p_io_mem[layer_index].SAI[idx].va);
		}
		for (idx = 0; idx < NN_OMEM_NUM; idx++) {
			p_io_mem[layer_index].SAO[idx].va = net_map_addr_with_parsflag(p_io_mem[layer_index].SAO[idx].va, buff_ofs, model_ofs);
			p_io_mem[layer_index].SAO[idx].pa = nvt_ai_va2pa(p_io_mem[layer_index].SAO[idx].va);
		}

#if 0	// for debug
		for (idx = 0; idx < NN_IMEM_NUM; idx++) {
			DBG_DUMP("p_io_mem[%d].SAI[%d] pa = 0x%08x; va = 0x%08x; size=%08x; fmt=%08x\r\n", (int)layer_index, (int)idx
					, (int)p_io_mem[layer_index].SAI[idx].pa, (int)p_io_mem[layer_index].SAI[idx].va
					, (int)p_io_mem[layer_index].SAI[idx].size, (int)p_io_mem[layer_index].SAI[idx].fmt);
		}
		for (idx = 0; idx < NN_OMEM_NUM; idx++) {
			DBG_DUMP("p_io_mem[%d].SAO[%d] pa = 0x%08x; va = 0x%08x; size=%08x; fmt=%08x\r\n", (int)layer_index, (int)idx
					, (int)p_io_mem[layer_index].SAO[idx].pa, (int)p_io_mem[layer_index].SAO[idx].va
					, (int)p_io_mem[layer_index].SAO[idx].size, (int)p_io_mem[layer_index].SAO[idx].fmt);

		}
#endif
	}
#endif

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		KDRV_AI_ENG engine;
		p_mctrl[process_index].addr += parm_ofs;
	if (p_mctrl[process_index].eng == NN_GEN_ENG_CNN) {
			engine = AI_ENG_CNN;
		} else if (p_mctrl[process_index].eng == NN_GEN_ENG_CNN2) {
			engine = AI_ENG_CNN2;
		} else if (p_mctrl[process_index].eng == NN_GEN_ENG_NUE) {
			engine = AI_ENG_NUE;
		} else if (p_mctrl[process_index].eng == NN_GEN_ENG_NUE2) {
			engine = AI_ENG_NUE2;
		} else {
			engine = AI_ENG_UNKNOWN;
		}
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			if (kdrv_ai_get_eng_caps(engine) == 0) {
				DBG_ERR("invalid app engine id %d, parsing fail...\r\n", (unsigned int)engine);
				return E_CTX;
			}
#if SYNC_MTCNN
			if (process_index == 0) {
				KDRV_AI_APP_HEAD *p_head = (KDRV_AI_APP_HEAD *)p_mctrl[process_index].addr;
				if (p_head->mode == AI_MODE_NEURAL) {
					KDRV_AI_NEURAL_PARM *p_parm = (KDRV_AI_NEURAL_PARM *)(p_head->parm_addr + parm_ofs);
					p_parm->in_addr = 0;
				}
            }
#endif
			nvt_ai_map_ai_app_addr(p_mctrl[process_index].addr, parm_ofs, model_ofs, buff_ofs, net_id);
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			if (kdrv_ai_get_eng_caps(engine) == 0) {
				DBG_ERR("invalid LL engine id %d, parsing fail...\r\n", (unsigned int)engine);
				return E_CTX;
			}
			nvt_ai_map_ai_ll_addr(p_mctrl[process_index].addr, parm_ofs, model_ofs, buff_ofs, net_id);
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			nvt_ai_map_drv_addr((NN_MODE)p_mctrl[process_index].mode, p_mctrl[process_index].addr, model_ofs, buff_ofs, net_id);
			break;
		}
		//fmem_dcache_sync((UINT32 *)(p_mctrl[process_index].addr), (p_mctrl[process_index].size), DMA_BIDIRECTIONAL);
	    vos_cpu_dcache_sync((p_mctrl[process_index].addr), (p_mctrl[process_index].size), VOS_DMA_TO_DEVICE); ///< cache clean - output to engine's input
	}

	return E_OK;
}
#else
#define SYNC_MTCNN 1
ER nvt_ai_pars_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_user_mem;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 parm_pa_ofs, model_pa_ofs, buff_pa_ofs;
	UINT32 parm_va_ofs, model_va_ofs, buff_va_ofs;
	UINT32 process_index = 0, idx = 0;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	UINT32 pre_index = 999;
#if NN_DLI
	CHAR vendor_ai_ver[VENDOR_AI_VERSION_LEN+1] = {'\0'};
	UINT32 vendor_ai_sub_ver = 0;
#endif

	if ((p_mem == NULL)
			|| nvt_ai_is_null_mem(p_mem->user_parm)
			|| nvt_ai_is_null_mem(p_mem->user_model)
			|| nvt_ai_is_null_mem(p_mem->user_buff)
			|| nvt_ai_is_null_mem(p_mem->kerl_parm)
			|| nvt_ai_is_null_mem(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_model)
			|| nvt_ai_is_null_mem(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff)) {
		DBG_ERR("null memory...\r\n");
		return E_CTX;
	}

	if (nvt_ai_is_mem_overflow(p_mem) || nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_pars_net fail...\r\n");
		return E_CTX;
	}

    er = nvt_ai_get_net_info(&net_info, p_mem->kerl_parm.va);
    if (er != E_OK) {
        DBG_ERR("nvt_ai_get_net_info fail...\r\n");
        return er;
    }

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	p_user_mem = &kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id];

	parm_pa_ofs = ALIGN_CEIL_4(p_mem->kerl_parm.pa);
	model_pa_ofs = ALIGN_CEIL_4(p_user_mem->user_model.pa);
	buff_pa_ofs = ALIGN_CEIL_4(p_user_mem->user_buff.pa);

	parm_va_ofs = ALIGN_CEIL_4(p_mem->kerl_parm.va);
	model_va_ofs = ALIGN_CEIL_4(p_user_mem->user_model.va);
	buff_va_ofs = ALIGN_CEIL_4(p_user_mem->user_buff.va);
	//DBG_IND("parm_pa_ofs = 0x%08x\n",parm_pa_ofs);
	//DBG_IND("model_pa_ofs = 0x%08x\n",model_pa_ofs);
	//DBG_IND("buff_pa_ofs = 0x%08x\n",buff_pa_ofs);
	//DBG_IND("parm_va_ofs = 0x%08x\n",parm_va_ofs);
	//DBG_IND("model_va_ofs = 0x%08x\n",model_va_ofs);
	//DBG_IND("buff_va_ofs = 0x%08x\n",buff_va_ofs);
#if CNN_MULTI_INPUT
	kflow_ai_net_global.g_ai_is_batch[net_id] = 0;
#endif

#if NN_DLI
	strncpy(vendor_ai_ver,kflow_ai_get_gen_version(),sizeof(CHAR)*VENDOR_AI_VERSION_LEN);
	vendor_ai_sub_ver = str_sub_fmt_to_int(vendor_ai_ver);
#endif

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		UINT32 layer_index = 0;
		NN_DATA *p_sai = NULL, *p_sao = NULL;
		UINT32 imem_cnt = 0, omem_cnt = 0;

		
		p_mctrl[process_index].iomem.imem_addr += parm_va_ofs;
		p_mctrl[process_index].iomem.omem_addr += parm_va_ofs;
		#if CNN_FMT_V4
		p_mctrl[process_index].in_bufinfo_addr  += parm_va_ofs;
		p_mctrl[process_index].out_bufinfo_addr += parm_va_ofs;
		#endif

		layer_index = p_mctrl[process_index].layer_index;
		#if CNN_MULTI_INPUT
		if (layer_index == 0 && process_index > 0) {
			p_sai = (NN_DATA *)p_mctrl[process_index].iomem.imem_addr;
			if (p_sai[0].va == 0) {
				kflow_ai_net_global.g_ai_is_batch[net_id] = 1;
			}
		}
		#endif

		if (layer_index == pre_index) {
			continue;
		}
		pre_index = layer_index;

		p_sai = (NN_DATA *)p_mctrl[process_index].iomem.imem_addr;
		p_sao = (NN_DATA *)p_mctrl[process_index].iomem.omem_addr;
		imem_cnt = p_mctrl[process_index].iomem.imem_cnt;
		omem_cnt = p_mctrl[process_index].iomem.omem_cnt;

		for (idx = 0; idx < imem_cnt; idx++) {
			p_sai[idx].pa = net_map_addr_with_parsflag(p_sai[idx].pa, model_pa_ofs, buff_pa_ofs);
			p_sai[idx].va = net_map_addr_with_parsflag(p_sai[idx].va, model_va_ofs, buff_va_ofs);
		}
		for (idx = 0; idx < omem_cnt; idx++) {
			//DBG_IND("before fix: layer_index(%u) p_sao[%u].pa = 0x%08x\n",layer_index,idx, p_sao[idx].pa);
			//DBG_IND("before fix: layer_index(%u) p_sao[%u].va = 0x%08x\n",layer_index,idx, p_sao[idx].va);
			p_sao[idx].pa = net_map_addr_with_parsflag(p_sao[idx].pa, model_pa_ofs, buff_pa_ofs);
			p_sao[idx].va = net_map_addr_with_parsflag(p_sao[idx].va, model_va_ofs, buff_va_ofs);
			//DBG_IND("after fix: layer_index(%u) p_sao[%u].pa = 0x%08x\n",layer_index,idx, p_sao[idx].pa);
			//DBG_IND("after fix: layer_index(%u) p_sao[%u].va = 0x%08x\n",layer_index,idx, p_sao[idx].va);
		}

#if 0   // for debug
		for (idx = 0; idx < imem_cnt; idx++) {
			DBG_DUMP("p_io_mem[%d].SAI[%d] pa = 0x%08x; va = 0x%08x; size=%d; fmt=0x%08x\r\n", (int)layer_index, (int)idx
					, (int)p_sai[idx].pa, (int)p_sai[idx].va, (int)p_sai[idx].size, (int)p_sai[idx].fmt);
		}
		for (idx = 0; idx < omem_cnt; idx++) {
			DBG_DUMP("p_io_mem[%d].SAO[%d] pa = 0x%08x; va = 0x%08x; size=%d; fmt=0x%08x\r\n", (int)layer_index, (int)idx
					, (int)p_sao[idx].pa, (int)p_sao[idx].va, (int)p_sao[idx].size, (int)p_sao[idx].fmt);
		}
#endif
	}	

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		p_mctrl[process_index].addr += parm_va_ofs;
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
#if SYNC_MTCNN
			if (process_index == 0) {
				KDRV_AI_APP_HEAD *p_head = (KDRV_AI_APP_HEAD *)p_mctrl[process_index].addr;
				if (p_head->mode == AI_MODE_NEURAL) {
					KDRV_AI_NEURAL_PARM *p_parm = (KDRV_AI_NEURAL_PARM *)(p_head->parm_addr + parm_va_ofs);
					p_parm->in_addr = 0;
				}
            }
#endif
			nvt_ai_map_ai_app_addr(p_mctrl[process_index].addr, parm_va_ofs, model_va_ofs, buff_va_ofs, net_id);
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
#if SUPPORT_DEPTHWISE
			if (p_mctrl[process_index].mode == NN_DEPTHWISE || p_mctrl[process_index].mode == NN_CORR) {
				nvt_ai_map_ai_multi_ll_addr(p_mctrl[process_index].addr, parm_va_ofs, parm_pa_ofs, model_pa_ofs, buff_pa_ofs, net_id);
			} else {
				nvt_ai_map_ai_ll_addr(p_mctrl[process_index].addr, parm_va_ofs, parm_pa_ofs, model_pa_ofs, buff_pa_ofs, net_id);
			}
#else
			nvt_ai_map_ai_ll_addr(p_mctrl[process_index].addr, parm_va_ofs, parm_pa_ofs, model_pa_ofs, buff_pa_ofs, net_id);
#endif
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			nvt_ai_map_drv_addr((NN_MODE)p_mctrl[process_index].mode, p_mctrl[process_index].addr, model_va_ofs, buff_va_ofs, net_id);
			break;
		}
		//fmem_dcache_sync((UINT32 *)(p_mctrl[process_index].addr), (p_mctrl[process_index].size), DMA_BIDIRECTIONAL);
		vos_cpu_dcache_sync((p_mctrl[process_index].addr), (p_mctrl[process_index].size), VOS_DMA_TO_DEVICE);  ///< cache clean - output to engine's input

#if NN_DLI
		if(p_mctrl[process_index].mode >= NN_DLI_FIRST &&
			p_mctrl[process_index].mode <= NN_DLI_LAST) {
			// old .so with new .ko, return err
			if(vendor_ai_sub_ver < VENDOR_AI_NN_DLI_CPU_CHK_VERINT){
				DBG_ERR("Vendor AI lib version (%d) doesn't support NN_DLI op 0x%08X, check version(%d), parsing fail...\r\n",
					vendor_ai_sub_ver, p_mctrl[process_index].mode, VENDOR_AI_NN_DLI_CPU_CHK_VERINT);
				return E_CTX;
			}
		}
#endif
	}
	//vos_cpu_dcache_sync(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_parm.va, kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_parm.size, VOS_DMA_TO_DEVICE);
#if NET_TABLE_HEAP
	if(nvt_ai_alloc_map_table(net_id, nvt_ai_net_get_input_layer_index(p_mem->kerl_parm)) != E_OK) {
		DBG_ERR("nvt_ai_alloc_map_table fail...\r\n");
		return E_NOMEM;
	}
#endif

	return E_OK;
}

ER nvt_ai_unpars_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id)
{
#if 0
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_user_mem;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 parm_pa_ofs, model_pa_ofs, buff_pa_ofs;
	UINT32 parm_va_ofs, model_va_ofs, buff_va_ofs;
	UINT32 model_pa_end, buff_pa_end;
	UINT32 model_va_end, buff_va_end;
	UINT32 process_index = 0, idx = 0;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	UINT32 pre_index = 999;

	if ((p_mem == NULL)
			|| nvt_ai_is_null_mem(p_mem->user_parm)
			|| nvt_ai_is_null_mem(p_mem->user_model)
			|| nvt_ai_is_null_mem(p_mem->user_buff)
			|| nvt_ai_is_null_mem(p_mem->kerl_parm)
			|| nvt_ai_is_null_mem(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_model)
			|| nvt_ai_is_null_mem(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff)) {
		DBG_ERR("null memory...\r\n");
		return E_CTX;
	}

	if (nvt_ai_is_mem_overflow(p_mem) || nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_unpars_net fail...\r\n");
		return E_CTX;
	}

	er = nvt_ai_get_net_info(&net_info, p_mem->kerl_parm.va);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	p_user_mem = &kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id];

	parm_pa_ofs = ALIGN_CEIL_4(p_mem->kerl_parm.pa);
	model_pa_ofs = ALIGN_CEIL_4(p_user_mem->user_model.pa);
	buff_pa_ofs = ALIGN_CEIL_4(p_user_mem->user_buff.pa);

	parm_va_ofs = ALIGN_CEIL_4(p_mem->kerl_parm.va);
	model_va_ofs = ALIGN_CEIL_4(p_user_mem->user_model.va);
	buff_va_ofs = ALIGN_CEIL_4(p_user_mem->user_buff.va);

	model_pa_end = p_user_mem->user_model.pa + p_user_mem->user_model.size;
	buff_pa_end = p_user_mem->user_buff.pa + p_user_mem->user_buff.size;

	model_va_end = p_user_mem->user_model.va + p_user_mem->user_model.size;
	buff_va_end = p_user_mem->user_buff.va + p_user_mem->user_buff.size;

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		UINT32 layer_index = p_mctrl[process_index].layer_index;
		NN_DATA *p_sai, *p_sao;
		UINT32 imem_cnt, omem_cnt;

		p_sai = (NN_DATA *)p_mctrl[process_index].iomem.imem_addr;
		p_sao = (NN_DATA *)p_mctrl[process_index].iomem.omem_addr;
		imem_cnt = p_mctrl[process_index].iomem.imem_cnt;
		omem_cnt = p_mctrl[process_index].iomem.omem_cnt;

		if (layer_index == pre_index) {
			continue;
		}
		pre_index = layer_index;
		for (idx = 0; idx < imem_cnt; idx++) {
			p_sai[idx].pa = net_unmap_addr(p_sai[idx].pa, model_pa_ofs, buff_pa_ofs, model_pa_end, buff_pa_end);
			p_sai[idx].va = net_unmap_addr(p_sai[idx].va, model_va_ofs, buff_va_ofs, model_va_end, buff_va_end);
		}
		for (idx = 0; idx < omem_cnt; idx++) {
			p_sao[idx].pa = net_unmap_addr(p_sao[idx].pa, model_pa_ofs, buff_pa_ofs, model_pa_end, buff_pa_end);
			p_sao[idx].va = net_unmap_addr(p_sao[idx].va, model_va_ofs, buff_va_ofs, model_va_end, buff_va_end);
		}
		
		

#if 0   // for debug
		for (idx = 0; idx < imem_cnt; idx++) {
			DBG_DUMP("p_io_mem[%d].SAI[%d] pa = 0x%08x; va = 0x%08x; size=%d; fmt=0x%08x\r\n", (int)layer_index, (int)idx
					, (int)p_sai[idx].pa, (int)p_sai[idx].va, (int)p_sai[idx].size, (int)p_sai[idx].fmt);
		}
		for (idx = 0; idx < omem_cnt; idx++) {
			DBG_DUMP("p_io_mem[%d].SAO[%d] pa = 0x%08x; va = 0x%08x; size=%d; fmt=0x%08x\r\n", (int)layer_index, (int)idx
					, (int)p_sao[idx].pa, (int)p_sao[idx].va, (int)p_sao[idx].size, (int)p_sao[idx].fmt);
		}
#endif
	}

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			nvt_ai_unmap_ai_app_addr(p_mctrl[process_index].addr, parm_va_ofs, model_va_ofs, buff_va_ofs, model_va_end, buff_va_end, net_id);
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
#if SUPPORT_DEPTHWISE
			if (p_mctrl[process_index].mode == NN_DEPTHWISE || p_mctrl[process_index].mode == NN_CORR) {
				nvt_ai_unmap_ai_multi_ll_addr(p_mctrl[process_index].addr, parm_va_ofs, parm_pa_ofs, model_pa_ofs, buff_pa_ofs, model_pa_end, buff_pa_end, net_id);
			} else {
				nvt_ai_unmap_ai_ll_addr(p_mctrl[process_index].addr, parm_va_ofs, parm_pa_ofs, model_pa_ofs, buff_pa_ofs, model_pa_end, buff_pa_end, net_id);
			}
#else
			nvt_ai_unmap_ai_ll_addr(p_mctrl[process_index].addr, parm_va_ofs, parm_pa_ofs, model_pa_ofs, buff_pa_ofs, model_pa_end, buff_pa_end, net_id);
#endif
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			nvt_ai_unmap_drv_addr((NN_MODE)p_mctrl[process_index].mode, p_mctrl[process_index].addr, model_va_ofs, buff_va_ofs, model_va_end, buff_va_end, net_id);
			break;
		}
		//fmem_dcache_sync((UINT32 *)(p_mctrl[process_index].addr), (p_mctrl[process_index].size), DMA_BIDIRECTIONAL);
		vos_cpu_dcache_sync((p_mctrl[process_index].addr), (p_mctrl[process_index].size), VOS_DMA_TO_DEVICE);
		p_mctrl[process_index].addr -= parm_va_ofs;

		p_mctrl[process_index].iomem.imem_addr -= parm_va_ofs;
		p_mctrl[process_index].iomem.omem_addr -= parm_va_ofs;
		#if CNN_FMT_V4
		p_mctrl[process_index].in_bufinfo_addr  -= parm_va_ofs;
		p_mctrl[process_index].out_bufinfo_addr -= parm_va_ofs;
		#endif
	}
#endif
#if NET_TABLE_HEAP
	nvt_ai_free_map_table(net_id);
#endif

	return E_OK;
}
#endif
	
unsigned int nvt_ai_user2kerl_va(unsigned int addr, UINT32 net_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem;
	
	if (nvt_ai_is_net_id_overflow(net_id)) {
		return 0;
	}
	
	p_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];

	if (addr < p_mem->user_parm.va) {
		DBG_ERR("user va %08x should > %08x\r\n", (int)addr, (int)p_mem->user_parm.va);
		return addr;
	}
	return addr - p_mem->user_parm.va + p_mem->kerl_parm.va;
}

unsigned int nvt_ai_kerl2user_va(unsigned int addr, UINT32 net_id)
{
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem;
	
	if (nvt_ai_is_net_id_overflow(net_id)) {
		return 0;
	}
	
	p_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];

	if (addr < p_mem->kerl_parm.va) {
		DBG_ERR("kernel va %08x should > %08x\r\n", (int)addr, (int)p_mem->kerl_parm.va);
		return addr;
	}
	return addr - p_mem->kerl_parm.va + p_mem->user_parm.va;
}

#if CNN_25_MATLAB
ER nvt_ai_proc_assign_input_addr(UINT32 proc_layer_num, NN_GEN_MODE_CTRL *p_mctrl, NN_IOMEM *p_mem_ofs)
{
	UINT32 process_index = 0, layer_index = 0;

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		layer_index = p_mctrl[process_index].layer_index;
		if (layer_index != 0) {
			continue;
		}

		if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_APP_AI_DRV) {
			KDRV_AI_APP_HEAD *p_app_head = (KDRV_AI_APP_HEAD *)p_mctrl[process_index].addr;
			switch (p_app_head->mode) {
			case AI_MODE_NEURAL: {
					KDRV_AI_NEURAL_PARM *p_parm = (KDRV_AI_NEURAL_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_mem_ofs->SAI[0].va;
					//p_parm->elt.addr += p_mem_ofs->SAI[1].va;
				}
				break;
			case AI_MODE_ROIPOOL: {
					KDRV_AI_ROIPOOL_PARM *p_parm = (KDRV_AI_ROIPOOL_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_mem_ofs->SAI[0].va;
					p_parm->roi_ker.roi_addr += p_mem_ofs->SAI[5].va;
				}
				break;
			case AI_MODE_SVM: {
					KDRV_AI_SVM_PARM *p_parm = (KDRV_AI_SVM_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_mem_ofs->SAI[0].va;
				}
				break;
			case AI_MODE_FC: {
					KDRV_AI_FC_PARM *p_parm = (KDRV_AI_FC_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_mem_ofs->SAI[0].va;
				}
				break;
			case AI_MODE_PERMUTE: {
					KDRV_AI_PERMUTE_PARM *p_parm = (KDRV_AI_PERMUTE_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_mem_ofs->SAI[0].va;
				}
				break;
			case AI_MODE_REORG: {
					KDRV_AI_REORG_PARM *p_parm = (KDRV_AI_REORG_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_mem_ofs->SAI[0].va;
				}
				break;
            case AI_MODE_PREPROC: {
                KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)p_app_head->parm_addr;
                p_parm->in_addr[0] += p_mem_ofs->SAI[0].va;
                p_parm->in_addr[1] += p_mem_ofs->SAI[1].va;
                p_parm->in_addr[2] += p_mem_ofs->SAI[2].va;
                }
                break;
			default:
				DBG_ERR("unknown input mode(%d) in app\r\n", (int)p_mctrl[process_index].mode);
				break;
			}
		} else if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
//========== for first layer linked list mode ==========
			KDRV_AI_LL_HEAD *p_ll_head = (KDRV_AI_LL_HEAD *)p_mctrl[process_index].addr;
			switch (p_ll_head->eng) {
			case AI_ENG_CNN: 
			case AI_ENG_CNN2: {
					CNN_LL_PARM *p_parm = (CNN_LL_PARM *)p_ll_head->parm_addr;
					p_parm->input.bit.addr += p_mem_ofs->SAI[0].pa;
					//p_parm->input[1].bit.addr += p_sai[BUF_IN_IDX].va;
					//p_parm->input[2].bit.addr += p_sai[BUF_IN_IDX].va;
					//p_parm->interm_in.bit.addr += p_mem_ofs->SAI[0].pa;
#if LL_BASE_TEST
					p_parm->input.bit.addr -= test_ofs;
					if (p_ll_head->eng == AI_ENG_CNN) {
						DBG_DUMP("CNN first layer - 0x%x\n", test_ofs);
					} else {
						DBG_DUMP("CNN2 first layer - 0x%x\n", test_ofs);
					}
#endif
				}
				break;
			case AI_ENG_NUE: {
					NUE_LL_PARM *p_parm = (NUE_LL_PARM *)p_ll_head->parm_addr;
					p_parm->input.bit.addr += p_mem_ofs->SAI[0].pa;
					//p_parm->elt_in.bit.addr += p_sai[NUE_ELT_IDX].va;
					//p_parm->roi_in.bit.addr += p_sai[NUE_ROI_IDX].va;
#if LL_BASE_TEST
					p_parm->input.bit.addr -= test_ofs;
					DBG_DUMP("NUE first layer - 0x%x\n", test_ofs);
#endif
				}
				break;
			case AI_ENG_NUE2: {
					NUE2_LL_PARM *p_parm = (NUE2_LL_PARM *)p_ll_head->parm_addr;
					p_parm->input[0].bit.addr += p_mem_ofs->SAI[0].pa;
					p_parm->input[1].bit.addr += p_mem_ofs->SAI[1].pa;
					p_parm->input[2].bit.addr += p_mem_ofs->SAI[2].pa;
					//p_parm->elt_in.bit.addr += p_sai[NUE_ELT_IDX].va;
					//p_parm->roi_in.bit.addr += p_sai[NUE_ROI_IDX].va;
#if LL_BASE_TEST
					p_parm->input[0].bit.addr -= test_ofs;
					p_parm->input[1].bit.addr -= test_ofs;
					p_parm->input[2].bit.addr -= test_ofs;
					DBG_DUMP("NUE2 first layer - 0x%x\n", test_ofs);
#endif
				}
				break;
			default:
				DBG_ERR("unknown engine type : %d\r\n", (int)p_ll_head->eng);
				break;
			}
//========== by CCC 191004 ==========
#else
			DBG_ERR("first layer can't be linked list\r\n");
			return E_OBJ;
#endif
		} else if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_COMMON) {
			const UINT32 parm_addr = p_mctrl[process_index].addr;
			switch (p_mctrl[process_index].mode) {
			case NN_POSTPROC: {
					NN_POSTPROC_PARM *p_parm = (NN_POSTPROC_PARM *)parm_addr;
					p_parm->in_addr += p_mem_ofs->SAI[0].va;
				}
				break;
			case NN_SOFTMAX: {
					NN_SOFTMAX_PARM *p_parm = (NN_SOFTMAX_PARM *)parm_addr;
					p_parm->in_addr += p_mem_ofs->SAI[0].va;
				}
				break;
                /*
			case NN_PREPROC:
				//DBG_ERR("pre-processing can't be in kernel space\r\n");
				break;
				*/
			case NN_FC_POST: {
					NN_FC_POST_PARM *p_parm = (NN_FC_POST_PARM*)parm_addr;
					p_parm->addr_in += p_mem_ofs->SAI[0].va;
				}
				break;
			case NN_POOL: {
					NN_POOL_PARM *p_parm = (NN_POOL_PARM*)parm_addr;
					p_parm->in_addr += p_mem_ofs->SAI[0].va;
				}
				break;
			case NN_CUSTOMER: {
					NN_CUSTOM_PARM *p_parm = (NN_CUSTOM_PARM*)parm_addr;
					p_parm->input.va += p_mem_ofs->SAI[0].va;
					p_parm->input.pa += p_mem_ofs->SAI[0].pa;
				}
				break;
			case NN_BNSCALE: {
					NN_BNSCALE_PARM *p_parm = (NN_BNSCALE_PARM*)parm_addr;
					p_parm->in_addr += p_mem_ofs->SAI[0].va;
				}
				break;
			default:
				DBG_ERR("unknown input mode(%d) in common\r\n", (int)p_mctrl[process_index].mode);
				break;
			}
		} else {
			DBG_ERR("unknown first layer trigger source: %d\r\n", (int)p_mctrl[process_index].trig_src);
			return E_OBJ;
		}
	}

	return E_OK;
}

ER nvt_ai_set_input(UINT32 net_addr, NN_IOMEM *p_in_io_mem, UINT32 net_id)
{
	UINT32 start_addr = 0;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_IOMEM *p_io_mem, *p_mem_ofs, *p_1st_mem;
	UINT32 idx = 0;
	NN_DATA *p_in_data, *p_model_data;
	const UINT32 io_mem_cnt = sizeof(NN_IOMEM) / sizeof(NN_DATA);
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	
	if ((net_addr == 0) || (p_in_io_mem == NULL)) {
		DBG_ERR("null input\r\n");
		return E_NOEXS;
	}
	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_proc_input_init fail...\r\n");
		return E_CTX;
	}
	if (kflow_ai_net_global.g_ai_input_set[net_id]) {
		DBG_ERR("input is already init\r\n");
		return E_NOEXS;
	}

	p_1st_mem = &kflow_ai_net_global.g_ai_input_mem[net_id];
	kflow_ai_net_global.g_ai_net_addr[net_id] = net_addr;
	memcpy(p_1st_mem, p_in_io_mem, sizeof(NN_IOMEM));
	start_addr = net_addr - g_ai_map_mem[net_id].user_parm.va + g_ai_map_mem[net_id].kerl_parm.va;
	er = nvt_ai_get_net_info(&net_info, start_addr);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	p_io_mem = net_info.p_io_mem;
	proc_layer_num  = p_head->mode_ctrl_num;

	///< update first layer io buffer
	p_in_data       = (NN_DATA *)p_1st_mem;
	p_model_data    = (NN_DATA *)(&p_io_mem[0]);
	
	for (idx = 0; idx < io_mem_cnt; idx++) {
		if ((p_in_data[idx].size == 0) || (p_in_data[idx].pa == 0)) {
			continue;
		}
		p_in_data[idx].va = nvt_ai_pa2va_remap_wo_sync(p_in_data[idx].pa, p_in_data[idx].size);
		memcpy(&p_model_data[idx], &p_in_data[idx], sizeof(NN_DATA));
	}

	///< update first layer address of parameter
	p_mem_ofs = &kflow_ai_net_global.g_ai_input_mem[net_id];
	nvt_ai_proc_assign_input_addr(proc_layer_num, p_mctrl, p_mem_ofs);
	kflow_ai_net_global.g_ai_input_set[net_id] = TRUE;

	return E_OK;
}

ER nvt_ai_clr_input(UINT32 net_addr, UINT32 net_id)
{
	UINT32 start_addr = 0;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_IOMEM first_mem_ofs;
	NN_IOMEM *p_io_mem, *p_1st_mem;
	UINT32 idx = 0;
	NN_DATA *p_model_data, *p_mem_ofs, *p_in_data;
	const UINT32 io_mem_cnt = sizeof(NN_IOMEM) / sizeof(NN_DATA);
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;

	if (net_addr == 0) {
		DBG_ERR("null input\r\n");
		return E_NOEXS;
	}
	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_proc_input_uninit fail...\r\n");
		return E_CTX;
	}
	
	if (!kflow_ai_net_global.g_ai_input_set[net_id]) {
		DBG_ERR("input is already uninit\r\n");
		return E_NOEXS;
	}
	p_1st_mem = &kflow_ai_net_global.g_ai_input_mem[net_id];

	start_addr = net_addr - g_ai_map_mem[net_id].user_parm.va + g_ai_map_mem[net_id].kerl_parm.va;
	er = nvt_ai_get_net_info(&net_info, start_addr);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	p_io_mem = net_info.p_io_mem;
	proc_layer_num  = p_head->mode_ctrl_num;
	///< restore first layer io buffer
	memcpy(&first_mem_ofs, p_1st_mem, sizeof(NN_IOMEM));
	p_in_data       = (NN_DATA *)p_1st_mem;
	p_model_data    = (NN_DATA *)(&p_io_mem[0]);
	p_mem_ofs = (NN_DATA *)(&first_mem_ofs);
	for (idx = 0; idx < io_mem_cnt; idx++) {
		if ((p_in_data[idx].size == 0) || (p_in_data[idx].pa == 0)) {
			continue;
		}
		p_mem_ofs[idx].pa = -p_mem_ofs[idx].pa;
		p_mem_ofs[idx].va = -p_mem_ofs[idx].va;
		nvt_ai_pa2va_unmap(p_in_data[idx].va, p_in_data[idx].pa);
		memset(&p_model_data[idx], 0, sizeof(NN_DATA));
	}
	memset(p_1st_mem, 0, sizeof(NN_IOMEM));
	///< restore first layer address of parameter
	nvt_ai_proc_assign_input_addr(proc_layer_num, p_mctrl, &first_mem_ofs);
	kflow_ai_net_global.g_ai_input_set[net_id] = FALSE;
	kflow_ai_net_global.g_ai_net_addr[net_id] = 0;

	return E_OK;
}
#else
ER nvt_ai_proc_assign_input_addr(UINT32 proc_layer_num, NN_GEN_MODE_CTRL *p_mctrl, NN_DATA *p_imem_ofs)
{
	UINT32 process_index = 0;
	NN_DATA *p_sai;
	//UINT32 tmp_layer_index = 0;

	p_sai = p_imem_ofs;
	//layer_index = p_mctrl->layer_index;

	{ //for (process_index = 0; process_index < proc_layer_num; process_index++) {
		/*tmp_layer_index = p_mctrl[process_index].layer_index;
		if (tmp_layer_index != layer_index) {
			continue;
		}*/

		if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_APP_AI_DRV) {
			KDRV_AI_APP_HEAD *p_app_head = (KDRV_AI_APP_HEAD *)p_mctrl[process_index].addr;
			switch (p_app_head->mode) {
			case AI_MODE_NEURAL: {
					KDRV_AI_NEURAL_PARM *p_parm = (KDRV_AI_NEURAL_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
					//p_parm->elt.addr += p_sai[CNN_INTERM_IDX].va;
				}
				break;
			case AI_MODE_ROIPOOL: {
					KDRV_AI_ROIPOOL_PARM *p_parm = (KDRV_AI_ROIPOOL_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
					p_parm->roi_ker.roi_addr += p_sai[NUE_ROI_IDX].va;
				}
				break;
			case AI_MODE_SVM: {
					KDRV_AI_SVM_PARM *p_parm = (KDRV_AI_SVM_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case AI_MODE_FC: {
					KDRV_AI_FC_PARM *p_parm = (KDRV_AI_FC_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case AI_MODE_PERMUTE: {
					KDRV_AI_PERMUTE_PARM *p_parm = (KDRV_AI_PERMUTE_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case AI_MODE_REORG: {
					KDRV_AI_REORG_PARM *p_parm = (KDRV_AI_REORG_PARM *)p_app_head->parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
            case AI_MODE_PREPROC: {
                KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)p_app_head->parm_addr;
                p_parm->in_addr[0] += p_sai[NUE2_IN_IDX0].va;
                p_parm->in_addr[1] += p_sai[NUE2_IN_IDX1].va;
                p_parm->in_addr[2] += p_sai[NUE2_IN_IDX2].va;
                }
                break;
			default:
				DBG_ERR("unknown input mode(%d) in app\r\n", (int)p_mctrl[process_index].mode);
				break;
			}
		} else if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
//========== for first layer linked list mode ==========
			KDRV_AI_LL_HEAD *p_ll_head = (KDRV_AI_LL_HEAD *)p_mctrl[process_index].addr;
			switch (p_ll_head->eng) {
			case AI_ENG_CNN: 
			case AI_ENG_CNN2: {
					CNN_LL_PARM *p_parm = (CNN_LL_PARM *)p_ll_head->parm_addr;
#if CNN_AI_FASTBOOT
					if (kdrv_ai_is_fastboot()) {
						// The init value of builtin bit.addr has been fixed before (is not 0), so "+=" cannot be used.
						p_parm->input.bit.addr = p_sai[BUF_IN_IDX].pa;
					} else {
						p_parm->input.bit.addr += p_sai[BUF_IN_IDX].pa;
					}
#else
					p_parm->input.bit.addr += p_sai[BUF_IN_IDX].pa;
#endif
					//p_parm->input[1].bit.addr += p_sai[BUF_IN_IDX].va;
					//p_parm->input[2].bit.addr += p_sai[BUF_IN_IDX].va;
					//p_parm->interm_in.bit.addr += p_sai[CNN_INTERM_IDX].pa;
				}
				break;
			case AI_ENG_NUE: {
					NUE_LL_PARM *p_parm = (NUE_LL_PARM *)p_ll_head->parm_addr;
#if CNN_AI_FASTBOOT
					if (kdrv_ai_is_fastboot()) {
						// The init value of builtin bit.addr has been fixed before (is not 0), so "+=" cannot be used.
						p_parm->input.bit.addr = p_sai[BUF_IN_IDX].pa;
					} else {
						p_parm->input.bit.addr += p_sai[BUF_IN_IDX].pa;
					}
#else
					p_parm->input.bit.addr += p_sai[BUF_IN_IDX].pa;
#endif
					//p_parm->elt_in.bit.addr += p_sai[NUE_ELT_IDX].va;
					//p_parm->roi_in.bit.addr += p_sai[NUE_ROI_IDX].va;
				}
				break;
			case AI_ENG_NUE2: {
					NUE2_LL_PARM *p_parm = (NUE2_LL_PARM *)p_ll_head->parm_addr;
#if CNN_AI_FASTBOOT
					if (kdrv_ai_is_fastboot()) {
						// The init value of builtin bit.addr has been fixed before (is not 0), so "+=" cannot be used.
						p_parm->input[0].bit.addr = p_sai[NUE2_IN_IDX0].pa;
						p_parm->input[1].bit.addr = p_sai[NUE2_IN_IDX1].pa;
						p_parm->input[2].bit.addr = p_sai[NUE2_IN_IDX2].pa;
					} else {
						p_parm->input[0].bit.addr += p_sai[NUE2_IN_IDX0].pa;
						p_parm->input[1].bit.addr += p_sai[NUE2_IN_IDX1].pa;
						p_parm->input[2].bit.addr += p_sai[NUE2_IN_IDX2].pa;
						#if 0 // debug
						DBG_IND("NUE2 sai: pa[0]=0x%x pa[1]=0x%x pa[2]=0x%x\n",
								p_sai[NUE2_IN_IDX0].pa, p_sai[NUE2_IN_IDX1].pa, p_sai[NUE2_IN_IDX2].pa);
						DBG_IND("NUE2 bit: addr[0]=0x%x addr[1]=0x%x addr[2]=0x%x\n\n",
								p_parm->input[0].bit.addr, p_parm->input[1].bit.addr, p_parm->input[2].bit.addr);
						#endif
					}
#else
					p_parm->input[0].bit.addr += p_sai[NUE2_IN_IDX0].pa;
					p_parm->input[1].bit.addr += p_sai[NUE2_IN_IDX1].pa;
					p_parm->input[2].bit.addr += p_sai[NUE2_IN_IDX2].pa;
#endif
					//p_parm->elt_in.bit.addr += p_sai[NUE_ELT_IDX].va;
					//p_parm->roi_in.bit.addr += p_sai[NUE_ROI_IDX].va;
				}
				break;
			default:
				DBG_ERR("unknown engine type : %d\r\n", (int)p_ll_head->eng);
				break;
			}
//========== by CCC 191004 ==========
#else
			DBG_ERR("first layer can't be linked list\r\n");
			return E_OBJ;
#endif
		} else if (p_mctrl[process_index].trig_src == NN_GEN_TRIG_COMMON) {
			const UINT32 parm_addr = p_mctrl[process_index].addr;
			switch (p_mctrl[process_index].mode) {
			case NN_POSTPROC: {
					NN_POSTPROC_PARM *p_parm = (NN_POSTPROC_PARM *)parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case NN_SOFTMAX: {
					NN_SOFTMAX_PARM *p_parm = (NN_SOFTMAX_PARM *)parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
                /*
			case NN_PREPROC:
				//DBG_ERR("pre-processing can't be in kernel space\r\n");
				break;
				*/
			case NN_FC_POST: {
					NN_FC_POST_PARM *p_parm = (NN_FC_POST_PARM*)parm_addr;
#if AI_V4
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
#else
					p_parm->addr_in += p_sai[BUF_IN_IDX].va;
#endif
				}
				break;
			case NN_POOL: {
					NN_POOL_PARM *p_parm = (NN_POOL_PARM*)parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case NN_LAYER_NORMALIZATION: {
					NN_LAYER_NORMALIZATION_PARM *p_parm = (NN_LAYER_NORMALIZATION_PARM*)parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			case NN_CUSTOMER: {
#if CUST_SUPPORT_MULTI_IO
					NN_DATA* io_head = NULL;
					UINT32 i = 0;

					NN_CUSTOM_PARM* p_cust_head = (NN_CUSTOM_PARM*)parm_addr;

					// parsing input
					io_head = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM));
					for (i = 0; i < p_cust_head->input_num; i++) {
						io_head[i].va += p_sai[i].va;
						io_head[i].pa += p_sai[i].pa;
					}
#else
					NN_CUSTOM_PARM *p_parm = (NN_CUSTOM_PARM*)parm_addr;
					p_parm->input.va += p_sai[BUF_IN_IDX].va;
					p_parm->input.pa += p_sai[BUF_IN_IDX].pa;
#endif
				}
				break;
			case NN_BNSCALE: {
					NN_BNSCALE_PARM *p_parm = (NN_BNSCALE_PARM*)parm_addr;
					p_parm->in_addr += p_sai[BUF_IN_IDX].va;
				}
				break;
			default:
				DBG_ERR("unknown input mode(%d) in common\r\n", (int)p_mctrl[process_index].mode);
				break;
			}
		} else {
			DBG_ERR("unknown first layer trigger source: %d\r\n", (int)p_mctrl[process_index].trig_src);
			return E_OBJ;
		}
	}

	return E_OK;
}

ER nvt_ai_set_input(UINT32 net_addr, NN_DATA *p_imem, UINT32 imem_cnt, UINT32 net_id)
{
	UINT32 mem_va = 0;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num = 0, idx = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_1st_imem, *p_imem_ofs, *p_model_data;



	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	
	if ((net_addr == 0) || (p_imem == NULL)) {
		DBG_ERR("null input\r\n");
		return E_NOEXS;
	}
	if (imem_cnt > NN_IMEM_NUM) {
		DBG_ERR("over input(%d)\r\n", imem_cnt);
		return E_NOEXS;
	}
	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_proc_input_init fail...\r\n");
		return E_CTX;
	}
	if (kflow_ai_net_global.g_ai_input_set[net_id]) {
		DBG_ERR("input is already init\r\n");
		return E_NOEXS;
	}

	p_1st_imem = kflow_ai_net_global.g_ai_input_imem[net_id];
	if(p_1st_imem == NULL) {
		DBG_ERR("NULL g_ai_input_imem to set... net_id(%d)\r\n", net_id);
		return er;
	}
	kflow_ai_net_global.g_ai_net_addr[net_id] = net_addr;
	memcpy(p_1st_imem, p_imem, sizeof(NN_DATA) * imem_cnt);
	
	mem_va = net_addr - kflow_ai_net_global.g_ai_map_mem[net_id].user_parm.va + kflow_ai_net_global.g_ai_map_mem[net_id].kerl_parm.va;

	er = nvt_ai_get_net_info(&net_info, mem_va);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	proc_layer_num  = p_head->mode_ctrl_num;

	///< update first layer io buffer
	p_model_data = (NN_DATA*)p_mctrl[0].iomem.imem_addr;
	
	for (idx = 0; idx < imem_cnt; idx++) {
		p_imem_ofs = &p_1st_imem[idx];
		if ((p_imem_ofs == NULL) || (p_imem_ofs->size == 0) || (p_imem_ofs->pa == 0)) {
			continue;
		}
		p_imem_ofs->va = nvt_ai_pa2va_remap_wo_sync(p_imem_ofs->pa, p_imem_ofs->size);
		memcpy(&p_model_data[idx], p_imem_ofs, sizeof(NN_DATA));
	}

	///< update first layer address of parameter

	er = nvt_ai_proc_assign_input_addr(proc_layer_num, p_mctrl, p_1st_imem);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_proc_assign_input_addr fail...\r\n");
		return er;
	}
	kflow_ai_net_global.g_ai_input_set[net_id] = TRUE;

	return E_OK;
}

ER nvt_ai_clr_input(UINT32 net_addr, UINT32 net_id)
{
	UINT32 mem_va = 0;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num = 0, imem_cnt = 0, idx = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_1st_mem, *p_imem_ofs, *p_model_data;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	NN_DATA first_mem_ofs[NN_IMEM_NUM] = {0};

	if (net_addr == 0) {
		DBG_ERR("null input\r\n");
		return E_NOEXS;
	}
	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_proc_input_uninit fail...\r\n");
		return E_CTX;
	}
	
	if (!kflow_ai_net_global.g_ai_input_set[net_id]) {
		DBG_ERR("input is already uninit\r\n");
		return E_NOEXS;
	}
	p_1st_mem = kflow_ai_net_global.g_ai_input_imem[net_id];
	if(p_1st_mem == NULL) {
		DBG_ERR("NULL g_ai_input_imem to clr... net_id(%d)\r\n", net_id);
		return er;
	}

	mem_va = net_addr - kflow_ai_net_global.g_ai_map_mem[net_id].user_parm.va + kflow_ai_net_global.g_ai_map_mem[net_id].kerl_parm.va;
	er = nvt_ai_get_net_info(&net_info, mem_va);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	proc_layer_num  = p_head->mode_ctrl_num;
	
	memcpy(&first_mem_ofs, p_1st_mem, sizeof(NN_DATA) * NN_IMEM_NUM);
	
	///< restore first layer io buffer
	p_model_data = (NN_DATA*)p_mctrl[0].iomem.imem_addr;
	p_imem_ofs = first_mem_ofs;
	imem_cnt = p_mctrl[0].iomem.imem_cnt;

	for (idx = 0; idx < imem_cnt; idx++) {
		if ((p_imem_ofs[idx].size == 0) || (p_imem_ofs[idx].pa == 0)) {
			continue;
		}

		vos_cpu_dcache_sync(p_imem_ofs[idx].va, p_imem_ofs[idx].size, VOS_DMA_BIDIRECTIONAL);
		nvt_ai_pa2va_unmap(p_imem_ofs[idx].va, p_imem_ofs[idx].pa);

		memset(&p_model_data[idx], 0, sizeof(NN_DATA));

		p_imem_ofs[idx].pa = -1 * ((INT32)p_imem_ofs[idx].pa);
		p_imem_ofs[idx].va = -1 * ((INT32)p_imem_ofs[idx].va);

	}


	///< restore first layer address of parameter
	er = nvt_ai_proc_assign_input_addr(proc_layer_num, p_mctrl, p_imem_ofs);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_proc_assign_input_addr fail...\r\n");
		return er;
	}
	kflow_ai_net_global.g_ai_input_set[net_id] = FALSE;
	kflow_ai_net_global.g_ai_net_addr[net_id] = 0;

	///< clear
	memset(kflow_ai_net_global.g_ai_input_imem[net_id], 0, sizeof(NN_DATA) * NN_IMEM_NUM);

	return E_OK;
}

#if CNN_MULTI_INPUT
ER nvt_ai_set_input2(UINT32 net_addr, NN_DATA *p_imem, UINT32 imem_cnt, UINT32 proc_idx, UINT32 net_id)
{
	UINT32 mem_va = 0;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num = 0, idx = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_1st_imem, *p_imem_ofs, *p_model_data;
	UINT32 i = 0;
	UINT32 map_idx = 0;


	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;

	if ((net_addr == 0) || (p_imem == NULL)) {
		DBG_ERR("null input\r\n");
		return E_NOEXS;
	}
	if (imem_cnt > NN_IMEM_NUM) {
		DBG_ERR("over input(%d)\r\n", imem_cnt);
		return E_NOEXS;
	}
	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_proc_input_init fail...\r\n");
		return E_CTX;
	}
	/*if (kflow_ai_net_global.g_ai_input_set[net_id]) {
		DBG_ERR("input is already init\r\n");
		return E_NOEXS;
	}*/

	// search map table
	// we can't use this to interpret used in before
	for (i = 0; i < g_ai_input_layer_map_num[net_id]; i++) {
		if (proc_idx == g_ai_input_layer_map_table[net_id][i]) {
			// if TRUE, means this proc_idx has already do set_input(map) before
			// we need to do clr_input(unmap) first
			if (nvt_ai_clr_input2(net_addr, proc_idx, net_id) != E_OK) {
				return E_SYS;
			}
			break;
		}
	}

	if (i == g_ai_input_layer_map_num[net_id]) { // if TRUE, means this is a new set_input (nothing found in map_table)
		// create new map table
#if NET_TABLE_HEAP
#else
		if (g_ai_input_layer_map_num[net_id] >= NN_SUPPORT_BATCH_NUM) {
			DBG_ERR("proc idx exceeds the max batch num...");
			return E_NOEXS;
		}
#endif
		g_ai_input_layer_map_table[net_id][g_ai_input_layer_map_num[net_id]] = proc_idx;
		g_ai_input_layer_map_num[net_id]++;
	}
	map_idx = i;



	//p_1st_imem = kflow_ai_net_global.g_ai_input_imem[net_id];
	p_1st_imem = g_ai_input_batch_imem[net_id][map_idx];
	if(p_1st_imem == NULL) {
		DBG_ERR("NULL g_ai_input_imem to set... net_id(%d)\r\n", net_id);
		return er;
	}
	kflow_ai_net_global.g_ai_net_addr[net_id] = net_addr;
	memcpy(p_1st_imem, p_imem, sizeof(NN_DATA) * imem_cnt);

	mem_va = net_addr - kflow_ai_net_global.g_ai_map_mem[net_id].user_parm.va + kflow_ai_net_global.g_ai_map_mem[net_id].kerl_parm.va;

	er = nvt_ai_get_net_info(&net_info, mem_va);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	proc_layer_num = (kflow_ai_net_global.g_ai_is_batch[net_id] == 1)? 1 : p_head->mode_ctrl_num;

	///< update first layer io buffer
	p_model_data = (NN_DATA*)p_mctrl[proc_idx].iomem.imem_addr;

	for (idx = 0; idx < imem_cnt; idx++) {
		p_imem_ofs = &p_1st_imem[idx];
		if ((p_imem_ofs == NULL) || (p_imem_ofs->size == 0) || (p_imem_ofs->pa == 0)) {
			continue;
		}
		if (p_mctrl[proc_idx].trig_src == NN_GEN_TRIG_LL_AI_DRV) {
			// no need to map va since ll cmd only need pa
			p_imem_ofs->va = 0;
		} else {
			p_imem_ofs->va = nvt_ai_pa2va_remap_wo_sync(p_imem_ofs->pa, p_imem_ofs->size);
		}
		// update addr & size
		p_model_data[idx].pa = p_imem_ofs[idx].pa;
		p_model_data[idx].va = p_imem_ofs[idx].va;
		p_model_data[idx].size = p_imem_ofs[idx].size;
	}

	///< update first layer address of parameter
	er = nvt_ai_proc_assign_input_addr(proc_layer_num, &p_mctrl[proc_idx], p_1st_imem);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_proc_assign_input_addr fail...\r\n");
		return er;
	}
	kflow_ai_net_global.g_ai_input_set[net_id] = TRUE;

	return E_OK;
}

ER nvt_ai_clr_input2(UINT32 net_addr, UINT32 proc_idx, UINT32 net_id)
{
	UINT32 mem_va = 0;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num = 0, imem_cnt = 0, idx = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_1st_mem, *p_imem_ofs, *p_model_data;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	NN_DATA first_mem_ofs[NN_IMEM_NUM] = {0};
	UINT32 i = 0;
	UINT32 map_idx = 0;
	UINT32 tmp = 0;

	if (net_addr == 0) {
		DBG_ERR("null input\r\n");
		return E_NOEXS;
	}
	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_proc_input_uninit fail...\r\n");
		return E_CTX;
	}
	
	if (!kflow_ai_net_global.g_ai_input_set[net_id]) {
		DBG_ERR("input is already uninit\r\n");
		return E_NOEXS;
	}
	p_1st_mem = kflow_ai_net_global.g_ai_input_imem[net_id];
	if(p_1st_mem == NULL) {
		DBG_ERR("NULL g_ai_input_imem to clr... net_id(%d)\r\n", net_id);
		return er;
	}

	for (i = 0; i < g_ai_input_layer_map_num[net_id]; i++) {
		if (proc_idx == g_ai_input_layer_map_table[net_id][i]) {
			break;
		}
	}

	if (i == g_ai_input_layer_map_num[net_id]) {	
		DBG_ERR("[%u] no vailable proc idx... i(%u), map_num(%u)\n", net_id, i, g_ai_input_layer_map_num[net_id]);
		return E_NOEXS;
	}

	map_idx = i;

	// restore table
	g_ai_input_layer_map_table[net_id][i] = 0;
	tmp = g_ai_input_layer_map_table[net_id][i];
	g_ai_input_layer_map_table[net_id][i] = g_ai_input_layer_map_table[net_id][g_ai_input_layer_map_num[net_id] - 1];
	g_ai_input_layer_map_table[net_id][g_ai_input_layer_map_num[net_id] - 1] = tmp;
	g_ai_input_layer_map_num[net_id]--;

	mem_va = net_addr - kflow_ai_net_global.g_ai_map_mem[net_id].user_parm.va + kflow_ai_net_global.g_ai_map_mem[net_id].kerl_parm.va;
	er = nvt_ai_get_net_info(&net_info, mem_va);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	proc_layer_num = (kflow_ai_net_global.g_ai_is_batch[net_id] == 1)? 1 : p_head->mode_ctrl_num;

	p_1st_mem = g_ai_input_batch_imem[net_id][map_idx];
	
	memcpy(&first_mem_ofs, p_1st_mem, sizeof(NN_DATA) * NN_IMEM_NUM);
	
	///< restore first layer io buffer
	p_model_data = (NN_DATA*)p_mctrl[proc_idx].iomem.imem_addr;
	p_imem_ofs = first_mem_ofs;
	imem_cnt = p_mctrl[proc_idx].iomem.imem_cnt;

	for (idx = 0; idx < imem_cnt; idx++) {
		if ((p_imem_ofs[idx].size == 0) || (p_imem_ofs[idx].pa == 0)) {
			continue;
		}
		if (p_mctrl[proc_idx].trig_src != NN_GEN_TRIG_LL_AI_DRV) {
			// case : if ll mode, then no need to unmap since no ioremap in previous step (ll cmd only need pa)
			nvt_ai_pa2va_unmap(p_imem_ofs[idx].va, p_imem_ofs[idx].pa);
		}
		// clear addr & size
		p_model_data[idx].pa = 0;
		p_model_data[idx].va = 0;
		p_model_data[idx].size = 0;

		// turn to negative (for reduction assignment)
		p_imem_ofs[idx].pa = -1 * ((INT32)p_imem_ofs[idx].pa);
		p_imem_ofs[idx].va = -1 * ((INT32)p_imem_ofs[idx].va);

	}


	///< restore first layer address of parameter
	er = nvt_ai_proc_assign_input_addr(proc_layer_num, &p_mctrl[proc_idx], p_imem_ofs);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_proc_assign_input_addr fail...\r\n");
		return er;
	}

	if (g_ai_input_layer_map_num[net_id] == 0) { // make sure all input have been clear
		kflow_ai_net_global.g_ai_input_set[net_id] = FALSE;
	}

	///< clear
	//memset(kflow_ai_net_global.g_ai_input_imem[net_id], 0, sizeof(NN_DATA) * NN_IMEM_NUM);
	memset(g_ai_input_batch_imem[net_id][map_idx], 0, (NN_IMEM_NUM * sizeof(NN_DATA)));
	if (g_ai_input_layer_map_num[net_id] > 0) {
		memcpy(g_ai_input_batch_imem[net_id][map_idx], g_ai_input_batch_imem[net_id][g_ai_input_layer_map_num[net_id]], (NN_IMEM_NUM * sizeof(NN_DATA)));
		memset(g_ai_input_batch_imem[net_id][g_ai_input_layer_map_num[net_id]], 0, (NN_IMEM_NUM * sizeof(NN_DATA)));
	}

	return E_OK;
}

/*
	clear all input map_table and reset map_num to 0.
*/
ER nvt_ai_clr_all_input2(UINT32 net_addr, UINT32 proc_idx, UINT32 net_id)
{
	UINT32 mem_va = 0;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num = 0, imem_cnt = 0, idx = 0;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_DATA *p_1st_mem, *p_imem_ofs, *p_model_data;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;
	NN_DATA first_mem_ofs[NN_IMEM_NUM] = {0};
	UINT32 i = 0;
	UINT32 map_idx = 0;
	UINT32 tmp = 0;

	if (net_addr == 0) {
		DBG_ERR("null input\r\n");
		return E_NOEXS;
	}
	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_proc_input_uninit fail...\r\n");
		return E_CTX;
	}
	
	if (!kflow_ai_net_global.g_ai_input_set[net_id]) {
		DBG_ERR("input is already uninit\r\n");
		return E_NOEXS;
	}
	p_1st_mem = kflow_ai_net_global.g_ai_input_imem[net_id];
	if(p_1st_mem == NULL) {
		DBG_ERR("NULL g_ai_input_imem to clr... net_id(%d)\r\n", net_id);
		return er;
	}

	for (i = 0; i < g_ai_input_layer_map_num[net_id]; i++) {
		if (proc_idx == g_ai_input_layer_map_table[net_id][i]) {
			break;
		}
	}

	if (i == g_ai_input_layer_map_num[net_id]) {	
		DBG_ERR("[%u] no vailable proc idx... i(%u), map_num(%u)\n", net_id, i, g_ai_input_layer_map_num[net_id]);
		return E_NOEXS;
	}

	map_idx = i;

	// restore table
	g_ai_input_layer_map_table[net_id][i] = 0;
	tmp = g_ai_input_layer_map_table[net_id][i];
	g_ai_input_layer_map_table[net_id][i] = g_ai_input_layer_map_table[net_id][g_ai_input_layer_map_num[net_id] - 1];
	g_ai_input_layer_map_table[net_id][g_ai_input_layer_map_num[net_id] - 1] = tmp;
	g_ai_input_layer_map_num[net_id]--;

	mem_va = net_addr - kflow_ai_net_global.g_ai_map_mem[net_id].user_parm.va + kflow_ai_net_global.g_ai_map_mem[net_id].kerl_parm.va;
	er = nvt_ai_get_net_info(&net_info, mem_va);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	proc_layer_num = (kflow_ai_net_global.g_ai_is_batch[net_id] == 1)? 1 : p_head->mode_ctrl_num;

	p_1st_mem = g_ai_input_batch_imem[net_id][map_idx];
	
	memcpy(&first_mem_ofs, p_1st_mem, sizeof(NN_DATA) * NN_IMEM_NUM);
	
	///< restore first layer io buffer
	p_model_data = (NN_DATA*)p_mctrl[proc_idx].iomem.imem_addr;
	p_imem_ofs = first_mem_ofs;
	imem_cnt = p_mctrl[proc_idx].iomem.imem_cnt;

	for (idx = 0; idx < imem_cnt; idx++) {
		if ((p_imem_ofs[idx].size == 0) || (p_imem_ofs[idx].pa == 0)) {
			continue;
		}
		if (p_mctrl[proc_idx].trig_src != NN_GEN_TRIG_LL_AI_DRV) {
			// case : if ll mode, then no need to unmap since no ioremap in previous step (ll cmd only need pa)
			vos_cpu_dcache_sync(p_imem_ofs[idx].va, p_imem_ofs[idx].size, VOS_DMA_BIDIRECTIONAL);
			nvt_ai_pa2va_unmap(p_imem_ofs[idx].va, p_imem_ofs[idx].pa);
		}
		// clear addr & size
		p_model_data[idx].pa = 0;
		p_model_data[idx].va = 0;
		p_model_data[idx].size = 0;

		// turn to negative (for reduction assignment)
		p_imem_ofs[idx].pa = -1 * ((INT32)p_imem_ofs[idx].pa);
		p_imem_ofs[idx].va = -1 * ((INT32)p_imem_ofs[idx].va);

	}


	///< restore first layer address of parameter
	er = nvt_ai_proc_assign_input_addr(proc_layer_num, &p_mctrl[proc_idx], p_imem_ofs);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_proc_assign_input_addr fail...\r\n");
		return er;
	}
	if (g_ai_input_layer_map_num[net_id] == 0) { // make sure all input have been clear
		kflow_ai_net_global.g_ai_input_set[net_id] = FALSE;
	}
	kflow_ai_net_global.g_ai_net_addr[net_id] = 0;

	///< clear
	//memset(kflow_ai_net_global.g_ai_input_imem[net_id], 0, sizeof(NN_DATA) * NN_IMEM_NUM);
	memset(g_ai_input_batch_imem[net_id][map_idx], 0, (NN_IMEM_NUM * sizeof(NN_DATA)));
	if (g_ai_input_layer_map_num[net_id] > 0) {
		memcpy(g_ai_input_batch_imem[net_id][map_idx], g_ai_input_batch_imem[net_id][g_ai_input_layer_map_num[net_id]], (NN_IMEM_NUM * sizeof(NN_DATA)));
		memset(g_ai_input_batch_imem[net_id][g_ai_input_layer_map_num[net_id]], 0, (NN_IMEM_NUM * sizeof(NN_DATA)));
	}

	return E_OK;
}

#endif // end of "#if CNN_MULTI_INPUT"
#endif

ER nvt_ai_update_layer(UINT32 layer, UINT32 net_id)
{
	NN_GEN_MODE_CTRL *p_user_mctrl;
	NN_GEN_MODE_CTRL *p_kerl_mctrl;
	UINT32 process_index;
	INT32 user_addr_ofs = 0;
	VENDOR_AIS_FLOW_MEM_PARM *p_user_parm;
	VENDOR_AIS_FLOW_MEM_PARM *p_kerl_parm;
	NN_GEN_NET_INFO user_net_info;
	NN_GEN_NET_INFO kerl_net_info;
	NN_GEN_MODEL_HEAD *p_user_head;
	ER er;

	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("exeed net_id!\r\n");
		return E_CTX;
	}

	if (!kflow_ai_net_global.g_ai_net_state[net_id]) {
		DBG_ERR("ai mem[%d] should be init!\r\n", (int)net_id);
		return E_SYS;
	}

	user_addr_ofs = kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_parm.va - kflow_ai_net_global.g_ai_map_mem[net_id].user_parm.va;
	p_user_parm = &kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_parm;
	p_kerl_parm  = &kflow_ai_net_global.g_ai_map_mem[net_id].kerl_parm;

	///< flush data
	//fmem_dcache_sync((UINT32 *)p_user_parm->va, p_user_parm->size, DMA_BIDIRECTIONAL);
	vos_cpu_dcache_sync(p_user_parm->va, p_user_parm->size, VOS_DMA_TO_DEVICE); ///< cache clean - output to engine's input

	er = nvt_ai_get_net_info(&user_net_info, p_user_parm->va);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}
	p_user_head = user_net_info.p_head;
	p_user_mctrl = user_net_info.p_mctrl;
	
	er = nvt_ai_get_net_info(&kerl_net_info, p_kerl_parm->va);
	if (er != E_OK) {
		DBG_ERR("nvt_ai_get_net_info fail...\r\n");
		return er;
	}
	p_kerl_mctrl = kerl_net_info.p_mctrl;
	if (layer >= p_user_head->mode_ctrl_num) {
		UINT32 clamp_mctrl_num = p_user_head->mode_ctrl_num - 1;
		DBG_WRN("mode control layer is overflow, clamp -> %d\r\n", (int)clamp_mctrl_num);
		layer = clamp_mctrl_num;
	}

	process_index = layer;
	if (p_user_mctrl[process_index].trig_src == NN_GEN_TRIG_APP_AI_DRV) {
		const KDRV_AI_APP_HEAD *p_user_app_head = (KDRV_AI_APP_HEAD *)(p_user_mctrl[process_index].addr + user_addr_ofs);
		const KDRV_AI_APP_HEAD *p_kerl_app_head = (KDRV_AI_APP_HEAD *)p_kerl_mctrl[process_index].addr;
		const UINT32 user_app_parm_addr = p_user_app_head->parm_addr + user_addr_ofs;
		const UINT32 kerl_app_parm_addr = p_kerl_app_head->parm_addr;
		switch (p_kerl_app_head->mode) {
		case AI_MODE_NEURAL: {
				KDRV_AI_NEURAL_PARM *p_user_parm = (KDRV_AI_NEURAL_PARM *)user_app_parm_addr;
				KDRV_AI_NEURAL_PARM *p_kerl_parm = (KDRV_AI_NEURAL_PARM *)kerl_app_parm_addr;
				KDRV_AI_NEURAL_PARM bk_kerl_parm;

				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(KDRV_AI_NEURAL_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(KDRV_AI_NEURAL_PARM));

				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out0_addr = bk_kerl_parm.out0_addr;
				p_kerl_parm->out1_addr = bk_kerl_parm.out1_addr;
				p_kerl_parm->in_interm_addr = bk_kerl_parm.in_interm_addr;
				p_kerl_parm->out_interm_addr = bk_kerl_parm.out_interm_addr;
				p_kerl_parm->tmp_buf_addr = bk_kerl_parm.tmp_buf_addr;
				p_kerl_parm->conv.weight_addr = bk_kerl_parm.conv.weight_addr;
				p_kerl_parm->conv.bias_addr = bk_kerl_parm.conv.bias_addr;
				p_kerl_parm->conv.fcd.quanti_kmeans_addr = bk_kerl_parm.conv.fcd.quanti_kmeans_addr;
				p_kerl_parm->conv.fcd.p_vlc_code = bk_kerl_parm.conv.fcd.p_vlc_code;
				p_kerl_parm->conv.fcd.p_vlc_valid = bk_kerl_parm.conv.fcd.p_vlc_valid;
				p_kerl_parm->conv.fcd.p_vlc_ofs = bk_kerl_parm.conv.fcd.p_vlc_ofs;
				p_kerl_parm->norm.bn_scl.bn_scale_addr = bk_kerl_parm.norm.bn_scl.bn_scale_addr;
				p_kerl_parm->elt.addr = bk_kerl_parm.elt.addr;
			}
			break;
		case AI_MODE_ROIPOOL: {
				KDRV_AI_ROIPOOL_PARM *p_user_parm = (KDRV_AI_ROIPOOL_PARM *)user_app_parm_addr;
				KDRV_AI_ROIPOOL_PARM *p_kerl_parm = (KDRV_AI_ROIPOOL_PARM *)kerl_app_parm_addr;
				KDRV_AI_ROIPOOL_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(KDRV_AI_ROIPOOL_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(KDRV_AI_ROIPOOL_PARM));
				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out_addr = bk_kerl_parm.out_addr;
				p_kerl_parm->roi_ker.roi_addr = bk_kerl_parm.roi_ker.roi_addr;
			}
			break;
		case AI_MODE_SVM: {
				KDRV_AI_SVM_PARM *p_user_parm = (KDRV_AI_SVM_PARM *)user_app_parm_addr;
				KDRV_AI_SVM_PARM *p_kerl_parm = (KDRV_AI_SVM_PARM *)kerl_app_parm_addr;
				KDRV_AI_SVM_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(KDRV_AI_SVM_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(KDRV_AI_SVM_PARM));
				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out_addr = bk_kerl_parm.out_addr;
				p_kerl_parm->svm_ker.sv_addr = bk_kerl_parm.svm_ker.sv_addr;
				p_kerl_parm->svm_ker.alpha_addr = bk_kerl_parm.svm_ker.alpha_addr;
				p_kerl_parm->fcd.quanti_kmeans_addr = bk_kerl_parm.fcd.quanti_kmeans_addr;
				p_kerl_parm->fcd.p_vlc_code = bk_kerl_parm.fcd.p_vlc_code;
				p_kerl_parm->fcd.p_vlc_valid = bk_kerl_parm.fcd.p_vlc_valid;
				p_kerl_parm->fcd.p_vlc_ofs = bk_kerl_parm.fcd.p_vlc_ofs;
			}
			break;
		case AI_MODE_FC: {
				KDRV_AI_FC_PARM *p_user_parm = (KDRV_AI_FC_PARM *)user_app_parm_addr;
				KDRV_AI_FC_PARM *p_kerl_parm = (KDRV_AI_FC_PARM *)kerl_app_parm_addr;
				KDRV_AI_FC_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(KDRV_AI_FC_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(KDRV_AI_FC_PARM));
				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out_addr = bk_kerl_parm.out_addr;
				p_kerl_parm->in_interm_addr = bk_kerl_parm.in_interm_addr;
				p_kerl_parm->out_interm_addr = bk_kerl_parm.out_interm_addr;
				p_kerl_parm->fc_ker.weight_addr = bk_kerl_parm.fc_ker.weight_addr;
				p_kerl_parm->fc_ker.bias_addr = bk_kerl_parm.fc_ker.bias_addr;
				p_kerl_parm->fcd.quanti_kmeans_addr = bk_kerl_parm.fcd.quanti_kmeans_addr;
				p_kerl_parm->fcd.p_vlc_code = bk_kerl_parm.fcd.p_vlc_code;
				p_kerl_parm->fcd.p_vlc_valid = bk_kerl_parm.fcd.p_vlc_valid;
				p_kerl_parm->fcd.p_vlc_ofs = bk_kerl_parm.fcd.p_vlc_ofs;
			}
			break;
		case AI_MODE_PERMUTE: {
				KDRV_AI_PERMUTE_PARM *p_user_parm = (KDRV_AI_PERMUTE_PARM *)user_app_parm_addr;
				KDRV_AI_PERMUTE_PARM *p_kerl_parm = (KDRV_AI_PERMUTE_PARM *)kerl_app_parm_addr;
				KDRV_AI_PERMUTE_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(KDRV_AI_PERMUTE_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(KDRV_AI_PERMUTE_PARM));
				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out_addr = bk_kerl_parm.out_addr;
			}
			break;
		case AI_MODE_REORG: {
				KDRV_AI_REORG_PARM *p_user_parm = (KDRV_AI_REORG_PARM *)user_app_parm_addr;
				KDRV_AI_REORG_PARM *p_kerl_parm = (KDRV_AI_REORG_PARM *)kerl_app_parm_addr;
				KDRV_AI_REORG_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(KDRV_AI_REORG_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(KDRV_AI_REORG_PARM));
				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out_addr = bk_kerl_parm.out_addr;
			}
			break;
        case AI_MODE_PREPROC: {
			    KDRV_AI_PREPROC_PARM *p_user_parm = (KDRV_AI_PREPROC_PARM *)user_app_parm_addr;
				KDRV_AI_PREPROC_PARM *p_kerl_parm = (KDRV_AI_PREPROC_PARM *)kerl_app_parm_addr;
				KDRV_AI_PREPROC_PARM bk_kerl_parm;
#if defined(_BSP_NA51055_)
				UINT32 max_width  = FOR_USER_98520_NUE2_IN_WIDTH_MAX;
				UINT32 max_height = (nvt_get_chip_id() == CHIP_NA51055) ? FOR_USER_98520_NUE2_IN_HEIGHT_MAX : FOR_USER_98528_NUE2_IN_HEIGHT_MAX;
				if (p_user_parm->in_size.width > max_width || p_user_parm->in_size.height > max_height) {
					DBG_ERR("input size %dx%d exceed limit %dx%d!\n", (unsigned int)p_user_parm->in_size.width,
					(unsigned int)p_user_parm->in_size.height, (unsigned int)max_width, (unsigned int)max_height);
					return E_SYS;
				}
#endif

                //calculate ideal cycle
                p_kerl_mctrl->idea_cycle = p_user_mctrl->idea_cycle;

				memcpy((VOID*)(&bk_kerl_parm), (VOID*)p_kerl_parm, sizeof(KDRV_AI_PREPROC_PARM));
				memcpy((VOID*)p_kerl_parm, (VOID*)p_user_parm, sizeof(KDRV_AI_PREPROC_PARM));
				p_kerl_parm->in_addr[0] = bk_kerl_parm.in_addr[0];
                p_kerl_parm->in_addr[1] = bk_kerl_parm.in_addr[1];
                p_kerl_parm->in_addr[2] = bk_kerl_parm.in_addr[2];
				p_kerl_parm->out_addr[0] = bk_kerl_parm.out_addr[0];
                p_kerl_parm->out_addr[1] = bk_kerl_parm.out_addr[1];
                p_kerl_parm->out_addr[2] = bk_kerl_parm.out_addr[2];
            }
			break;
		default:
			DBG_ERR("unknown input mode(%d) in app\r\n", (int)p_kerl_app_head->mode);
			break;
		}
	} else if (p_user_mctrl[process_index].trig_src == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
	//========== for first layer linked list mode ==========
		const KDRV_AI_LL_HEAD *p_user_ll_head = (KDRV_AI_LL_HEAD  *)(p_user_mctrl[process_index].addr + user_addr_ofs);
		const KDRV_AI_LL_HEAD *p_kerl_ll_head = (KDRV_AI_LL_HEAD  *)p_kerl_mctrl[process_index].addr;
		const UINT32 user_ll_parm_addr = p_user_ll_head->parm_addr + user_addr_ofs;
		const UINT32 kerl_ll_parm_addr = p_kerl_ll_head->parm_addr;
		
		//assign parameters from user to kernel
		switch (p_kerl_ll_head->eng) {
		case AI_ENG_CNN:
		case AI_ENG_CNN2: {
				CNN_LL_PARM *p_user_parm = (CNN_LL_PARM*)user_ll_parm_addr;
				CNN_LL_PARM *p_kerl_parm = (CNN_LL_PARM*)kerl_ll_parm_addr;
				memcpy(&p_kerl_parm->size0, &p_user_parm->size0, sizeof(CNN_SIZE0_REG));
				memcpy(&p_kerl_parm->size1, &p_user_parm->size1, sizeof(CNN_SIZE1_REG));
			}
			break;
		case AI_ENG_NUE: {
				NUE_LL_PARM *p_user_parm = (NUE_LL_PARM*)user_ll_parm_addr;
				NUE_LL_PARM *p_kerl_parm = (NUE_LL_PARM*)kerl_ll_parm_addr;
				memcpy(&p_kerl_parm->size1, &p_user_parm->size1, sizeof(NUE_SIZE1_REG));
				memcpy(&p_kerl_parm->size2, &p_user_parm->size2, sizeof(NUE_SIZE2_REG));
			}
			break;
		case AI_ENG_NUE2: {
				NUE2_LL_PARM *p_user_parm = (NUE2_LL_PARM*)user_ll_parm_addr;
				NUE2_LL_PARM *p_kerl_parm = (NUE2_LL_PARM*)kerl_ll_parm_addr;
#if defined(_BSP_NA51055_)
				UINT32 max_width  = FOR_USER_98520_NUE2_IN_WIDTH_MAX;
				UINT32 max_height = (nvt_get_chip_id() == CHIP_NA51055) ? FOR_USER_98520_NUE2_IN_HEIGHT_MAX : FOR_USER_98528_NUE2_IN_HEIGHT_MAX;
				if (p_user_parm->size0.bit.width > max_width || p_user_parm->size0.bit.height > max_height) {
					DBG_ERR("input size %dx%d exceed limit %dx%d!\n", (unsigned int)p_user_parm->size0.bit.width,
					(unsigned int)p_user_parm->size0.bit.height, (unsigned int)max_width, (unsigned int)max_height);
					return E_SYS;
				}
#endif

                //calculate ideal cycle
                p_kerl_mctrl->idea_cycle = p_user_mctrl->idea_cycle;

				memcpy(&p_kerl_parm->size0, &p_user_parm->size0, sizeof(NUE2_SIZE0_REG));
				memcpy(&p_kerl_parm->ilofs[0], &p_user_parm->ilofs[0], sizeof(NUE2_LOFS_REG));
				memcpy(&p_kerl_parm->ilofs[1], &p_user_parm->ilofs[1], sizeof(NUE2_LOFS_REG));
				memcpy(&p_kerl_parm->ilofs[2], &p_user_parm->ilofs[2], sizeof(NUE2_LOFS_REG));
//				memcpy(&p_kerl_parm->scale0, &p_user_parm->scale0, sizeof(NUE2_SCL0_REG));
//				memcpy(&p_kerl_parm->scale1, &p_user_parm->scale1, sizeof(NUE2_SCL1_REG));
#if AI_SUPPORT_MULTI_FMT
#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
				// force to assign fmt in ll cmd (for 32x YUV420_NV21)
				{
					UINT64* tmp_cmd = (UINT64*)kerl_ll_parm_addr;
					UINT64* usr_cmd = (UINT64*)user_ll_parm_addr;
					UINT32 tmp_idx = 0;
					// search the register which contain yuv_mode register
					while (1) {
						if ((tmp_cmd[tmp_idx] >> 63) == 1) {
							if (((tmp_cmd[tmp_idx] >> 32) & 0xFF) == 0x04) {
								// modify the format
								UINT32* p_reg_val = (UINT32*)(&tmp_cmd[tmp_idx]);
								UINT32 usr_reg_val = (UINT32)usr_cmd[tmp_idx];
								p_reg_val[0] = usr_reg_val;

								break;
							} else {
								tmp_idx++;
							}
						} else if ((tmp_cmd[tmp_idx] >> 60) == 0) {
							// ll end
							break;
						} else {
							// find next cmd
							tmp_idx++;
						}
					}
				}
#endif
#endif
#if KER_ENABLE_PADDING
				if (net_id < MULTI_SCALE_SUPPORT_NUM) {
					if (g_scl_restore_en[net_id] == 1) {
						// restore the scaling width/height & padding enable
						UINT32 net_in_width = 0, net_in_height = 0;
						UINT32 reg_val = 0;
					   
						// restore scaling w/h
						/*
						er = nvt_ai_get_ll_reg_val(kerl_ll_parm_addr, 0x70, &reg_val);
						if (er != E_OK) {
							DBG_ERR("get ll cmd fail\r\n");
							return er;
						}
						net_in_width = reg_val & 0xFFF;
						net_in_height = (reg_val >> 16) & 0xFFF;
						*/
						net_in_width = g_scl_size[net_id][0];
						net_in_height = g_scl_size[net_id][1];					
						p_kerl_parm->scale_size.bit.h_scl_size = net_in_width;
						p_kerl_parm->scale_size.bit.v_scl_size = net_in_height;	

						// padding disable
						nvt_ai_get_ll_reg_val(kerl_ll_parm_addr, 0x4, &reg_val);
						reg_val = reg_val & 0xFFFFFFFB;
						nvt_ai_set_ll_reg_val(kerl_ll_parm_addr, 0x4, reg_val);
							
					}
					if ((p_kerl_parm->size0.bit.width <= p_kerl_parm->scale_size.bit.h_scl_size 
						&& p_kerl_parm->size0.bit.height < p_kerl_parm->scale_size.bit.v_scl_size)
						|| (p_kerl_parm->size0.bit.width < p_kerl_parm->scale_size.bit.h_scl_size 
						&& p_kerl_parm->size0.bit.height <= p_kerl_parm->scale_size.bit.v_scl_size)) {
						UINT32 reg_val = 0;
						g_scl_size[net_id][0] = p_kerl_parm->scale_size.bit.h_scl_size;
						g_scl_size[net_id][1] = p_kerl_parm->scale_size.bit.v_scl_size;
						g_scl_restore_en[net_id] = 1;
						p_kerl_parm->scale_size.bit.h_scl_size = p_kerl_parm->size0.bit.width;
						p_kerl_parm->scale_size.bit.v_scl_size = p_kerl_parm->size0.bit.height;
						// force enable padding
						nvt_ai_get_ll_reg_val(kerl_ll_parm_addr, 0x4, &reg_val);
						reg_val = reg_val | 0x4;
						nvt_ai_set_ll_reg_val(kerl_ll_parm_addr, 0x4, reg_val);
						// set pad crop width/height
						reg_val = 0 | p_kerl_parm->scale_size.bit.h_scl_size | (p_kerl_parm->scale_size.bit.v_scl_size << 16);
						nvt_ai_set_ll_reg_val(kerl_ll_parm_addr, 0x68, reg_val);
						// set pad value
						reg_val = 0x0;
						nvt_ai_set_ll_reg_val(kerl_ll_parm_addr, 0x74, reg_val);
					} else {
						g_scl_restore_en[net_id] = 0;
					}

					// set scale_h_mode up (horizontal)
					if (p_kerl_parm->size0.bit.width < p_kerl_parm->scale_size.bit.h_scl_size) {
						UINT32 reg_val = 0;
						nvt_ai_get_ll_reg_val(kerl_ll_parm_addr, 0x4, &reg_val);
						reg_val = reg_val | 0x1000000; // OR bit_24: NUE2_SCALE_H_MODE
						nvt_ai_set_ll_reg_val(kerl_ll_parm_addr, 0x4, reg_val);
					} else {
						UINT32 reg_val = 0;
						nvt_ai_get_ll_reg_val(kerl_ll_parm_addr, 0x4, &reg_val);
						reg_val = reg_val & ~(0x1000000); // CLEAR bit_24: NUE2_SCALE_H_MODE
						nvt_ai_set_ll_reg_val(kerl_ll_parm_addr, 0x4, reg_val);
					}

					// set scale_v_mode up (vertical)
					if (p_kerl_parm->size0.bit.height < p_kerl_parm->scale_size.bit.v_scl_size) {
						UINT32 reg_val = 0;
						nvt_ai_get_ll_reg_val(kerl_ll_parm_addr, 0x4, &reg_val);
						reg_val = reg_val | 0x2000000; // OR bit_25: NUE2_SCALE_V_MODE
						nvt_ai_set_ll_reg_val(kerl_ll_parm_addr, 0x4, reg_val);
					} else {
						UINT32 reg_val = 0;
						nvt_ai_get_ll_reg_val(kerl_ll_parm_addr, 0x4, &reg_val);
						reg_val = reg_val & ~(0x2000000); // CLEAR bit_25: NUE2_SCALE_V_MODE
						nvt_ai_set_ll_reg_val(kerl_ll_parm_addr, 0x4, reg_val);
					}
				}
#endif
				if (p_kerl_parm->size0.bit.width < p_kerl_parm->scale_size.bit.h_scl_size) {
					p_kerl_parm->scale0.bit.h_rate = 0; // scale up case should set 0
				} else {
					p_kerl_parm->scale0.bit.h_rate = (((p_kerl_parm->size0.bit.width-1)/(p_kerl_parm->scale_size.bit.h_scl_size-1))-1);
				}

				if (p_kerl_parm->size0.bit.height < p_kerl_parm->scale_size.bit.v_scl_size) {
					p_kerl_parm->scale0.bit.v_rate = 0; // scale up case should set 0
				} else {
					p_kerl_parm->scale0.bit.v_rate = (((p_kerl_parm->size0.bit.height-1)/(p_kerl_parm->scale_size.bit.v_scl_size-1))-1);
				}

				p_kerl_parm->scale1.bit.h_sfact = (((p_kerl_parm->size0.bit.width-1)*65536/(p_kerl_parm->scale_size.bit.h_scl_size-1)) & 0xffff);
				p_kerl_parm->scale1.bit.v_sfact = (((p_kerl_parm->size0.bit.height-1)*65536/(p_kerl_parm->scale_size.bit.v_scl_size-1)) & 0xffff);

			}
			break;
		default:
			DBG_ERR("unknown engine type : %d\r\n", (int)p_kerl_ll_head->eng);
			break;
		}
		vos_cpu_dcache_sync(p_kerl_mctrl[process_index].addr, AI_SYNC_ALIGN_CEIL(p_kerl_mctrl[process_index].size), VOS_DMA_TO_DEVICE);
//========== by CCC 191004 ==========
#else
		DBG_ERR("first layer can't be linked list\r\n");
		return E_SYS;
#endif
	} else if (p_user_mctrl[process_index].trig_src == NN_GEN_TRIG_COMMON) {
		const UINT32 user_parm_addr = p_user_mctrl[process_index].addr;
		const UINT32 kerl_parm_addr = p_kerl_mctrl[process_index].addr;
		switch (p_user_mctrl[process_index].mode) {
		case NN_POSTPROC: {
				NN_POSTPROC_PARM *p_user_parm = (NN_POSTPROC_PARM *)user_parm_addr;
				NN_POSTPROC_PARM *p_kerl_parm = (NN_POSTPROC_PARM *)kerl_parm_addr;
				NN_POSTPROC_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(NN_POSTPROC_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(NN_POSTPROC_PARM));
				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out_addr = bk_kerl_parm.out_addr;
			}
			break;
		case NN_SOFTMAX: {
				NN_SOFTMAX_PARM *p_user_parm = (NN_SOFTMAX_PARM *)user_parm_addr;
				NN_SOFTMAX_PARM *p_kerl_parm = (NN_SOFTMAX_PARM *)kerl_parm_addr;
				NN_SOFTMAX_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(NN_SOFTMAX_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(NN_SOFTMAX_PARM));
				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out_addr = bk_kerl_parm.out_addr;
			}
			break;
            /*
		case NN_PREPROC:
			//DBG_ERR("pre-processing can't be in kernel space\r\n");
			break;
			*/
		case NN_FC_POST: {
				NN_FC_POST_PARM *p_user_parm = (NN_FC_POST_PARM *)user_parm_addr;
				NN_FC_POST_PARM *p_kerl_parm = (NN_FC_POST_PARM *)kerl_parm_addr;
				NN_FC_POST_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(NN_FC_POST_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(NN_FC_POST_PARM));
#if AI_V4
				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out_addr = bk_kerl_parm.out_addr;
#else
				p_kerl_parm->addr_in = bk_kerl_parm.addr_in;
				p_kerl_parm->addr_out = bk_kerl_parm.addr_out;
#endif
				p_kerl_parm->bias_addr = bk_kerl_parm.bias_addr;
			}
			break;
		case NN_POOL: {
				NN_POOL_PARM *p_user_parm = (NN_POOL_PARM *)user_parm_addr;
				NN_POOL_PARM *p_kerl_parm = (NN_POOL_PARM *)kerl_parm_addr;
				NN_POOL_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(NN_POOL_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(NN_POOL_PARM));
				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out_addr = bk_kerl_parm.out_addr;
			}
			break;
		case NN_CUSTOMER: {
				NN_CUSTOM_PARM *p_user_parm = (NN_CUSTOM_PARM *)user_parm_addr;
				NN_CUSTOM_PARM *p_kerl_parm = (NN_CUSTOM_PARM *)kerl_parm_addr;
				NN_CUSTOM_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(NN_CUSTOM_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(NN_CUSTOM_PARM));
#if CUST_SUPPORT_MULTI_IO
				{
					NN_DATA* user_io_head = (NN_DATA*)(user_parm_addr + sizeof(NN_CUSTOM_PARM));
					NN_DATA* kerl_io_head = (NN_DATA*)(kerl_parm_addr + sizeof(NN_CUSTOM_PARM));
					NN_DATA bk_kerl_io_head[96];
					UINT32 kerl_io_num = p_kerl_parm->input_num + p_kerl_parm->output_num + p_kerl_parm->model_num;
					UINT32 user_io_num = p_user_parm->input_num + p_user_parm->output_num + p_user_parm->model_num;

					memcpy((VOID *)(bk_kerl_io_head), (VOID *)kerl_io_head, kerl_io_num*sizeof(NN_DATA));
					memcpy((VOID *)kerl_io_head, (VOID *)user_io_head, user_io_num*sizeof(NN_DATA));
					p_user_parm->input_num = bk_kerl_parm.input_num;
					p_user_parm->output_num = bk_kerl_parm.output_num;
					p_user_parm->model_num = bk_kerl_parm.model_num;
				}
#else
				p_user_parm->input = bk_kerl_parm.input;
				p_user_parm->output = bk_kerl_parm.output;
				p_user_parm->model = bk_kerl_parm.model;
#endif
			}
			break;
		case NN_BNSCALE: {
				NN_BNSCALE_PARM *p_user_parm = (NN_BNSCALE_PARM *)user_parm_addr;
				NN_BNSCALE_PARM *p_kerl_parm = (NN_BNSCALE_PARM *)kerl_parm_addr;
				NN_BNSCALE_PARM bk_kerl_parm;
				memcpy((VOID *)(&bk_kerl_parm), (VOID *)p_kerl_parm, sizeof(NN_BNSCALE_PARM));
				memcpy((VOID *)p_kerl_parm, (VOID *)p_user_parm, sizeof(NN_BNSCALE_PARM));
				p_kerl_parm->in_addr = bk_kerl_parm.in_addr;
				p_kerl_parm->out_addr = bk_kerl_parm.out_addr;
				p_kerl_parm->mean_addr = bk_kerl_parm.mean_addr;
				p_kerl_parm->alpha_addr = bk_kerl_parm.alpha_addr;
				p_kerl_parm->beta_addr = bk_kerl_parm.beta_addr;
			}
			break;
		default:
			DBG_ERR("unknown input mode(%d) in common\r\n", (int)p_user_mctrl[process_index].mode);
			break;
		}
	} else {
		DBG_ERR("unknown first layer trigger source: %d\r\n", (int)p_user_mctrl[process_index].trig_src);
		return E_SYS;
	}
	return E_OK;
}

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
ER nvt_ai_set_ll_base(VENDOR_AIS_FLOW_LL_BASE *p_ll_base)
{
	INT32 ret;
	KDRV_AI_ENG eng, eng2;
	UINT32 addr;

	if (p_ll_base == NULL) {
		DBG_ERR("p_ll_base is null\r\n");
		return E_SYS;
	}

	eng = p_ll_base->eng;
	addr = p_ll_base->addr;

	ret = kdrv_ai_set_ll_base_addr(eng, addr);
	if (ret < 0) {
		DBG_ERR("set ll base addr fail, eng(%d) addr(0x%x)\r\n", eng, addr);
		return E_SYS;
	}

	DBG_DUMP("nvt_ai_set_ll_base: OK eng(%d) addr(0x%x) <-----------------\r\n", eng, addr);

	/* only for CNN2 */
	if (p_ll_base->eng == AI_ENG_CNN) {
		eng2 = AI_ENG_CNN2;
		ret = kdrv_ai_set_ll_base_addr(eng2, addr);
		if (ret < 0) {
			DBG_ERR("set ll base addr fail, eng2(%d) addr(0x%x)\r\n", eng2, addr);
			return E_SYS;
		}
		DBG_DUMP("nvt_ai_set_ll_base: OK eng2(%d) addr(0x%x) <-----------------\r\n", eng2, addr);
	}

	return E_OK;
}
#endif

ER nvt_ai_set_mem_ofs(VENDOR_AIS_FLOW_MEM_OFS *p_mem_ofs)
{
#if LL_BASE_TEST
	if (p_mem_ofs == NULL) {
		DBG_ERR("p_mem_ofs == NULL");
		return E_SYS;
	}

	test_ofs = p_mem_ofs->ofs;
	DBG_DUMP("nvt_ai_set_mem_ofs: offset(0x%x) <====================\r\n", test_ofs);
#else
	DBG_DUMP("Not support! Please enable LL_BASE_TEST first\r\n");
#endif
	return E_OK;
}

UINT32 str_sub_fmt_to_int(CHAR* vers)
{
	return ((vers[3]-'0')*10 + (vers[4]-'0'));
}

ER nvt_ai_chk_vers(VENDOR_AIS_FLOW_VERS *p_vers_info)
{
	UINT32 new_fmt_sub = 1; //0: old fmt sub, 1: new fmt sub
	CHAR vendor_ai_ver[VENDOR_AI_VERSION_LEN+1] = {'\0'};
	UINT32 vendor_ai_sub_ver = 0;

#if defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	UINT32 chip_id = nvt_get_chip_id();

	if ((chip_id == CHIP_NA51055) ||
#if defined(_BSP_NA51089_)
		(chip_id == CHIP_NA51089) ||
#endif
		(chip_id == CHIP_NA51084)) {
		kflow_ai_set_gen_version(p_vers_info, chip_id);
	}

	if (chip_id == CHIP_NA51055) {
		// 520 chip
#if CNN_25_MATLAB
		switch (p_vers_info->chip_id) {
			case NN_CHIP_CNN25_A_NEW_FMT:
				return E_OK;

			case NN_CHIP_CNN25_B_NEW_FMT:
			case NN_CHIP_CNN25_C_NEW_FMT:
			default:
				DBG_ERR("model.bin chip-id is not supported by this chip! id(0x%08x), gentool(0x%08x) | chip(0x%08x)\r\n",
						(unsigned int)p_vers_info->chip_id, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
				return E_NOSPT;
		}
#else
		UINT8  fmt_main = (UINT8)(((p_vers_info->chip_id) & 0x00FF0000) >> 16);
		UINT8  fmt_sub  = (UINT8)(((p_vers_info->chip_id) & 0xFF000000) >> 24);
		UINT16  id = (UINT16)((p_vers_info->chip_id) & 0xFFFF);

		if (fmt_main != NN_CHIP_AI2) {
			DBG_ERR("model.bin main-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
					(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
			return E_NOSPT;
		}

		strncpy(vendor_ai_ver,kflow_ai_get_gen_version(),sizeof(CHAR)*VENDOR_AI_VERSION_LEN);
		vendor_ai_sub_ver = str_sub_fmt_to_int(vendor_ai_ver);
		
		// old .so with new .ko, return err
		if(vendor_ai_sub_ver < AI_VENDOR_AI_CHK_VERINT){
			new_fmt_sub = 0;
		}
		if(new_fmt_sub){
			//we only care the right most LEGAL_LSB bits
			//if user set a bit higher than the bit, then we will give an err
			if(((fmt_sub | SUBVER_ILLEGAL_CHK_MASK) >> LEGAL_LSB)!=0 || (fmt_sub & NN_CHIP_AI_SUBVER2_MASK)!=0){
				DBG_ERR("model.bin sub-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
									(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
				return E_NOSPT;
			}
		}
		else{
			switch (fmt_sub) {
				case NN_CHIP_AI_SUBVER0:  // (non-depthwise) gentool
				case NN_CHIP_AI_SUBVER1:  // (depthwise)     gentool
					break;

				case NN_CHIP_AI_SUBVER2:  // reserved

				default:
					DBG_ERR("model.bin sub-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
							(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
					return E_NOSPT;
			}

		}
		
		switch (id) {
			case NN_CHIP_CNN25_A:
				return E_OK;

			case NN_CHIP_CNN25_B:
			case NN_CHIP_CNN25_C:
			case NN_CHIP_CNN25_D:
			default:
				DBG_ERR("model.bin chip-id is not supported by this chip!  id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
						(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
				return E_NOSPT;
		}
#endif
	} else if (chip_id == CHIP_NA51084) {
		// 528 chip
#if CNN_25_MATLAB
		switch (p_vers_info->chip_id) {
			case NN_CHIP_CNN25_A_NEW_FMT:
			case NN_CHIP_CNN25_B_NEW_FMT:
			case NN_CHIP_CNN25_C_NEW_FMT:
				return E_OK;

			default:
				DBG_ERR("model.bin chip-id is not supported by this chip! id(0x%08x), gentool(0x%08x) | chip(0x%08x)\r\n",
						(unsigned int)p_vers_info->chip_id, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
				return E_NOSPT;
		}
#else
		UINT8  fmt_main = (UINT8)(((p_vers_info->chip_id) & 0x00FF0000) >> 16);
		UINT8  fmt_sub  = (UINT8)(((p_vers_info->chip_id) & 0xFF000000) >> 24);
		UINT16  id = (UINT16)((p_vers_info->chip_id) & 0xFFFF);

		if (fmt_main != NN_CHIP_AI2) {
			DBG_ERR("model.bin main-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
					(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
			return E_NOSPT;
		}

		strncpy(vendor_ai_ver,kflow_ai_get_gen_version(),sizeof(CHAR)*VENDOR_AI_VERSION_LEN);
		vendor_ai_sub_ver = str_sub_fmt_to_int(vendor_ai_ver);

		// old .so with new .ko, return err
		if(vendor_ai_sub_ver < AI_VENDOR_AI_CHK_VERINT){
			new_fmt_sub = 0;
		}

		if(new_fmt_sub){
			//we only care the right most LEGAL_LSB bits
			//if user set a bit higher than the bit, then we will give an err
			if(((fmt_sub | SUBVER_ILLEGAL_CHK_MASK) >> LEGAL_LSB)!=0 || (fmt_sub & NN_CHIP_AI_SUBVER2_MASK)!=0){
				DBG_ERR("model.bin sub-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
									(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
				return E_NOSPT;
			}

		}
		else{
			switch (fmt_sub) {
					case NN_CHIP_AI_SUBVER0:  // (non-depthwise) gentool
					case NN_CHIP_AI_SUBVER1:  // (depthwise)     gentool
						break;

					case NN_CHIP_AI_SUBVER2:  // reserved
					default:
						DBG_ERR("model.bin sub-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
								(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
						return E_NOSPT;
				}
		}
		
		switch (id) {
			case NN_CHIP_CNN25_A:
			case NN_CHIP_CNN25_B:
			case NN_CHIP_CNN25_C:
				return E_OK;

			case NN_CHIP_CNN25_D:
			default:
				DBG_ERR("model.bin chip-id is not supported by this chip!  id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
						(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
				return E_NOSPT;
		}
#endif
#if defined(_BSP_NA51089_)
	} else if (chip_id == CHIP_NA51089) {
		// 560 chip
		UINT8  fmt_main = (UINT8)(((p_vers_info->chip_id) & 0x00FF0000) >> 16);
		UINT8  fmt_sub  = (UINT8)(((p_vers_info->chip_id) & 0xFF000000) >> 24);
		UINT16  id = (UINT16)((p_vers_info->chip_id) & 0xFFFF);

		if (fmt_main != NN_CHIP_AI2) {
			DBG_ERR("model.bin main-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
					(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
			return E_NOSPT;
		}

		strncpy(vendor_ai_ver,kflow_ai_get_gen_version(),sizeof(CHAR)*VENDOR_AI_VERSION_LEN);
		vendor_ai_sub_ver = str_sub_fmt_to_int(vendor_ai_ver);

		// old .so with new .ko, return err
		if(vendor_ai_sub_ver < AI_VENDOR_AI_CHK_VERINT){
			new_fmt_sub = 0;
		}

		if(new_fmt_sub){
			//we only care the right most LEGAL_LSB bits
			//if user set a bit higher than the bit, then we will give an err
			if(((fmt_sub | SUBVER_ILLEGAL_CHK_MASK) >> LEGAL_LSB)!=0 || (fmt_sub & NN_CHIP_AI_SUBVER2_MASK)!=0){
				DBG_ERR("model.bin sub-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
									(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
				return E_NOSPT;
			}
		}
		else{
			switch (fmt_sub) {
					case NN_CHIP_AI_SUBVER0:  // (non-depthwise) gentool
					case NN_CHIP_AI_SUBVER1:  // (depthwise)     gentool
						break;

					case NN_CHIP_AI_SUBVER2:  // reserved
					default:
						DBG_ERR("model.bin sub-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
								(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
						return E_NOSPT;
				}
		}
		switch (id) {
			case NN_CHIP_CNN25_D:
				return E_OK;

			case NN_CHIP_CNN25_A:
			case NN_CHIP_CNN25_B:
			case NN_CHIP_CNN25_C:
			default:
				DBG_ERR("model.bin chip-id is not supported by this chip!  id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
						(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
				return E_NOSPT;
		}
#endif
	} else {
		DBG_ERR("nvt_get_chip_id() = 0x%08x ?\r\n", (unsigned int)chip_id);
		return E_NOSPT;
	}
#else //defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
#if CNN_25_MATLAB
	switch (p_vers_info->chip_id) {
		case NN_CHIP_CNN25_A_NEW_FMT:
			return E_OK;

		case NN_CHIP_CNN25_B_NEW_FMT:
		case NN_CHIP_CNN25_C_NEW_FMT:
		default:
			DBG_ERR("model.bin chip-id is not supported by this chip!  id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
					(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
			return E_NOSPT;
	}
#else
	UINT8  fmt_main = (UINT8)(((p_vers_info->chip_id) & 0x00FF0000) >> 16);
	UINT8  fmt_sub  = (UINT8)(((p_vers_info->chip_id) & 0xFF000000) >> 24);
	UINT16  id = (UINT16)((p_vers_info->chip_id) & 0xFFFF);

	if (fmt_main != NN_CHIP_AI2) {
		DBG_ERR("model.bin main-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
				(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
		return E_NOSPT;
	}

	strncpy(vendor_ai_ver,kflow_ai_get_gen_version(),sizeof(CHAR)*VENDOR_AI_VERSION_LEN);
	vendor_ai_sub_ver = str_sub_fmt_to_int(vendor_ai_ver);

	// old .so with new .ko, return err
	if(vendor_ai_sub_ver < AI_VENDOR_AI_CHK_VERINT){
		new_fmt_sub = 0;
	}

	if(new_fmt_sub){
		//we only care the right most LEGAL_LSB bits
		//if user set a bit higher than the bit, then we will give an err
		if(((fmt_sub | SUBVER_ILLEGAL_CHK_MASK) >> LEGAL_LSB)!=0 || (fmt_sub & NN_CHIP_AI_SUBVER2_MASK)!=0){
			DBG_ERR("model.bin sub-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
								(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
			return E_NOSPT;
		}
	}
	else{
		switch (fmt_sub) {
			case NN_CHIP_AI_SUBVER0:  // (non-depthwise) gentool
			case NN_CHIP_AI_SUBVER1:  // (depthwise)     gentool
				break;

			case NN_CHIP_AI_SUBVER2:  // reserved
			default:
				DBG_ERR("model.bin sub-format is not supported by ai2! id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
						(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
				return E_NOSPT;
		}
	}

	switch (id) {
		case NN_CHIP_CNN25_A:
			return E_OK;

		case NN_CHIP_CNN25_B:
		case NN_CHIP_CNN25_C:
		case NN_CHIP_CNN25_D:
		default:
			DBG_ERR("model.bin chip-id is not supported by this chip!  id(0x%08x), fmt_main(0x%02x), fmt_sub(0x%02x), gentool(0x%08x) | chip(0x%08x)\r\n",
					(unsigned int)p_vers_info->chip_id, (unsigned int)fmt_main, (unsigned int)fmt_sub, (unsigned int)p_vers_info->gentool_vers, (unsigned int)chip_id);
			return E_NOSPT;
	}
#endif //CNN_25_MATLAB


#endif //defined(_BSP_NA51055_) || defined(_BSP_NA51089_)
	return E_NOSPT;
}

ER nvt_ai_get_net_info(NN_GEN_NET_INFO *p_info, UINT32 net_addr)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num, model_head_size, mctrl_size;
	NN_GEN_MODE_CTRL *p_mctrl;
#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
#endif
#if CNN_528_PSW
	UINT32 *p_id_list;
#if CNN_25_MATLAB
	UINT32 id_list_size;
#endif
#endif
	if ((p_info == NULL) || (net_addr == 0)) {
		DBG_ERR("null input...\r\n");
		return E_SYS;
	}

	p_head = (NN_GEN_MODEL_HEAD *)net_addr;
	proc_layer_num = p_head->mode_ctrl_num;
	model_head_size = ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD));
	mctrl_size = ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * proc_layer_num);
	p_mctrl = (NN_GEN_MODE_CTRL *)((UINT32)p_head + model_head_size);
#if CNN_528_PSW
	p_id_list = (UINT32 *)((UINT32)p_mctrl + mctrl_size);
#if CNN_25_MATLAB
	id_list_size = p_head->layer_id_list_size;
	p_io_mem = (NN_IOMEM *)((UINT32)p_id_list + id_list_size);
#endif
#else
	p_io_mem = (NN_IOMEM *)((UINT32)p_mctrl + mctrl_size);
#endif
	
	p_info->p_head      = p_head;
	p_info->p_mctrl     = p_mctrl;
#if CNN_25_MATLAB
	p_info->p_io_mem    = p_io_mem;
#endif
#if CNN_528_PSW
	p_info->p_id_list   = p_id_list;
#endif

	return E_OK;
}

INT32 get_base_index_by_in_buf_id(NN_GEN_MODE_CTRL *p_mctrl, UINT32 buf_id, UINT32 buf_index, UINT32 mode_ctrl_num)
{
	INT idx = 0;

	for (idx = 0; idx < (INT32)mode_ctrl_num; idx++) {
		if (IN_BUF_INDEX(&p_mctrl[idx], buf_index) == (INT32)buf_id) {  // p_mctrl[idx].in_buf_index[buf_index]
			return idx;
		}
	}
	return -1;
}

INT32 get_base_index_by_out_buf_id(NN_GEN_MODE_CTRL *p_mctrl, UINT32 buf_id, UINT32 buf_index, UINT32 mode_ctrl_num)
{
	INT idx = 0;

	for (idx = 0; idx < (INT32)mode_ctrl_num; idx++) {
		if (OUT_BUF_INDEX(&p_mctrl[idx], buf_index) == (INT32)buf_id) {  // p_mctrl[idx].out_buf_index[buf_index]
			return idx;
		}
	}
	return -1;
}

VOID nvt_ai_update_app_info(UINT32 parm_addr, NN_DIFF_PARM* p_diff_param, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id, UINT32 mode)
{
	KDRV_AI_APP_HEAD *p_head = (KDRV_AI_APP_HEAD *)parm_addr;
	UINT32 upd_addr = 0;

	switch (p_head->mode) {
	case AI_MODE_NEURAL: {
			KDRV_AI_NEURAL_PARM *p_parm 	= (KDRV_AI_NEURAL_PARM *)p_head->parm_addr;

			if (mode == 0) {
#if CNN_25_MATLAB
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[0], buf_ofs, model_ofs);
#else
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[0], model_ofs, buf_ofs);
#endif
				p_diff_param->stripe_inaddr[0] = p_parm->in_addr;
			} else {
				upd_addr = p_diff_param->stripe_inaddr[0];
				p_diff_param->stripe_inaddr[0] = (p_parm->in_addr - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			p_parm->in_addr         		= upd_addr;

			if (mode == 0) {
#if CNN_25_MATLAB
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], buf_ofs, model_ofs);
#else
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], model_ofs, buf_ofs);
#endif
				p_diff_param->stripe_outaddr[0] = (p_parm->out_interm_dma_en)?p_parm->out_interm_addr:p_parm->out0_addr;
			} else {
				upd_addr = p_diff_param->stripe_outaddr[0];
				p_diff_param->stripe_outaddr[0] = (((p_parm->out_interm_dma_en)?p_parm->out_interm_addr:p_parm->out0_addr) - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			if (p_parm->out_interm_dma_en)
				p_parm->out_interm_addr			= upd_addr;
			else
				p_parm->out0_addr       		= upd_addr;

			if (mode == 0) {
#if CNN_25_MATLAB
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[1], buf_ofs, model_ofs);
#else
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[1], model_ofs, buf_ofs);
#endif
				p_diff_param->stripe_outaddr[1] = p_parm->out1_addr;
			} else {
				upd_addr = p_diff_param->stripe_outaddr[1];
				p_diff_param->stripe_outaddr[1] = (p_parm->out1_addr - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			p_parm->out1_addr       		= upd_addr;



			SWAP(p_parm->size.width, p_diff_param->in_width);
			SWAP(p_parm->size.height, p_diff_param->in_height);



			SWAP(p_parm->out0_ofs.line_ofs, p_diff_param->out_ofs[0].line_ofs);
			SWAP(p_parm->out0_ofs.channel_ofs, p_diff_param->out_ofs[0].channel_ofs);
			SWAP(p_parm->out1_ofs.line_ofs, p_diff_param->out_ofs[1].line_ofs);
			SWAP(p_parm->out1_ofs.channel_ofs, p_diff_param->out_ofs[1].channel_ofs);
			SWAP(p_parm->in_ofs.line_ofs, p_diff_param->in_ofs[0].line_ofs);
			SWAP(p_parm->in_ofs.channel_ofs, p_diff_param->in_ofs[0].channel_ofs);
			SWAP(p_parm->elt.in_ofs.line_ofs, p_diff_param->in_ofs[1].line_ofs);
			SWAP(p_parm->elt.in_ofs.channel_ofs, p_diff_param->in_ofs[1].channel_ofs);
			SWAP(p_parm->pool.local.pad.top_pad_num, p_diff_param->pool_pad.top_pad_num);
			SWAP(p_parm->pool.local.pad.bot_pad_num, p_diff_param->pool_pad.bot_pad_num);
			SWAP(p_parm->pool.local.pad.left_pad_num, p_diff_param->pool_pad.left_pad_num);
			SWAP(p_parm->pool.local.pad.right_pad_num, p_diff_param->pool_pad.right_pad_num);
			SWAP(p_parm->conv.pad.top_pad_num, p_diff_param->conv_pad.top_pad_num);
			SWAP(p_parm->conv.pad.bot_pad_num, p_diff_param->conv_pad.bot_pad_num);
			SWAP(p_parm->conv.pad.left_pad_num, p_diff_param->conv_pad.left_pad_num);
			SWAP(p_parm->conv.pad.right_pad_num, p_diff_param->conv_pad.right_pad_num);

		}
		break;
	case AI_MODE_ROIPOOL:
		break;
	case AI_MODE_SVM:
		break;
	case AI_MODE_FC:
		break;
	case AI_MODE_PERMUTE: {
			KDRV_AI_PERMUTE_PARM *p_parm = (KDRV_AI_PERMUTE_PARM *)p_head->parm_addr;

			if (mode == 0) {
#if CNN_25_MATLAB
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[0], buf_ofs, model_ofs);
#else
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[0], model_ofs, buf_ofs);
#endif
				p_diff_param->stripe_inaddr[0] = p_parm->in_addr;
			} else {
				upd_addr = p_diff_param->stripe_inaddr[0];
				p_diff_param->stripe_inaddr[0] = (p_parm->in_addr - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			p_parm->in_addr         		= upd_addr;

			if (mode == 0) {
#if CNN_25_MATLAB
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], buf_ofs, model_ofs);
#else
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], model_ofs, buf_ofs);
#endif
				p_diff_param->stripe_outaddr[0] = p_parm->out_addr;
			} else {
				upd_addr = p_diff_param->stripe_outaddr[0];
				p_diff_param->stripe_outaddr[0] = (p_parm->out_addr - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			p_parm->out_addr       		= upd_addr;
			SWAP(p_parm->size.width, p_diff_param->in_width);
			SWAP(p_parm->size.height, p_diff_param->in_height);
		}
		break;
	case AI_MODE_REORG: {
			KDRV_AI_REORG_PARM *p_parm = (KDRV_AI_REORG_PARM *)p_head->parm_addr;

			if (mode == 0) {
#if CNN_25_MATLAB
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[0], buf_ofs, model_ofs);
#else
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[0], model_ofs, buf_ofs);
#endif
				p_diff_param->stripe_inaddr[0] = p_parm->in_addr;
			} else {
				upd_addr = p_diff_param->stripe_inaddr[0];
				p_diff_param->stripe_inaddr[0] = (p_parm->in_addr - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			p_parm->in_addr         		= upd_addr;

			if (mode == 0) {
#if CNN_25_MATLAB
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], buf_ofs, model_ofs);
#else
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], model_ofs, buf_ofs);
#endif
				p_diff_param->stripe_outaddr[0] = p_parm->out_addr;
			} else {
				upd_addr = p_diff_param->stripe_outaddr[0];
				p_diff_param->stripe_outaddr[0] = (p_parm->out_addr - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			p_parm->out_addr       		= upd_addr;
			SWAP(p_parm->size.width, p_diff_param->in_width);
			SWAP(p_parm->size.height, p_diff_param->in_height);
		}
	break;
case AI_MODE_PREPROC: {
			KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)p_head->parm_addr;

			if (mode == 0) {
#if CNN_25_MATLAB
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], buf_ofs, model_ofs);
#else
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], model_ofs, buf_ofs);
#endif
				p_diff_param->stripe_outaddr[0] = p_parm->out_addr[0];
			} else {
				upd_addr = p_diff_param->stripe_outaddr[0];
				p_diff_param->stripe_outaddr[0] = (p_parm->out_addr[0] - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			p_parm->out_addr[0]       		= upd_addr;

			if (mode == 0) {
#if CNN_25_MATLAB
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[1], buf_ofs, model_ofs);
#else
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[1], model_ofs, buf_ofs);
#endif
				p_diff_param->stripe_outaddr[1] = p_parm->out_addr[1];
			} else {
				upd_addr = p_diff_param->stripe_outaddr[1];
				p_diff_param->stripe_outaddr[1] = (p_parm->out_addr[1] - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			p_parm->out_addr[1]       		= upd_addr;

			if (mode == 0) {
#if CNN_25_MATLAB
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[2], buf_ofs, model_ofs);
#else
				upd_addr = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[2], model_ofs, buf_ofs);
#endif
				p_diff_param->stripe_outaddr[2] = p_parm->out_addr[2];
			} else {
				upd_addr = p_diff_param->stripe_outaddr[2];
				p_diff_param->stripe_outaddr[2] = (p_parm->out_addr[2] - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
			}
			p_parm->out_addr[2]       		= upd_addr;

			SWAP(p_parm->out_ofs[0].line_ofs, p_diff_param->out_ofs[0].line_ofs);
			SWAP(p_parm->out_ofs[1].line_ofs, p_diff_param->out_ofs[1].line_ofs);
			SWAP(p_parm->out_ofs[2].line_ofs, p_diff_param->out_ofs[2].line_ofs);
			SWAP(p_parm->scale_ker.scl_out_size.width, p_diff_param->sca_width);
			SWAP(p_parm->scale_ker.scl_out_size.height, p_diff_param->sca_height);
			SWAP(p_parm->sub_ker.sub_in_w, p_diff_param->mean_width);
			SWAP(p_parm->sub_ker.sub_in_h, p_diff_param->mean_height);
			SWAP(p_parm->pad_ker.crop_w, p_diff_param->tc_width);
			SWAP(p_parm->pad_ker.crop_h, p_diff_param->tc_height);
			SWAP(p_parm->pad_ker.pad_out_w, p_diff_param->pad_out_width);
			SWAP(p_parm->pad_ker.pad_out_h, p_diff_param->pad_out_height);
	}
		break;
	default:
		break;
	}
}

INT prt_cnt = 0;
VOID nvt_ai_update_ll_info(NN_GEN_MODE_CTRL *p_mctrl, UINT32 parm_addr, NN_DIFF_PARM* p_diff_param, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id, UINT32 mode)
{
	KDRV_AI_LL_HEAD *p_head = (KDRV_AI_LL_HEAD *)parm_addr;
	UINT64 *p_ll_cmd;//, *p_ll_end_cmd;
	UINT32 cmd_idx = 0;
	UINT32 reg_ofs = 0;
	UINT32 upd_addr = 0, upd_va = 0;

	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		p_ll_cmd = (UINT64 *)p_head->parm_addr;
		while (((p_ll_cmd[cmd_idx] >> 60) & 0xf) == 8) {
			reg_ofs = (p_ll_cmd[cmd_idx] >> 32) & 0xfff;
			switch(reg_ofs) {
				case 0x10:
				if (p_diff_param->stripe_inaddr[0] > 0) {
					if (mode == 0) {
#if CNN_25_MATLAB
						upd_addr = vos_cpu_get_phy_addr(net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[0], buf_ofs, model_ofs));
#else
						upd_va = net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[0], model_ofs, buf_ofs);
						if (upd_va == 0) {
							DBG_WRN("%s NULL va, maybe it's input? CNN 0x10 stripe_addr(%#x) model_ofs(%#x) buf_ofs(%#x)\n", __FUNCTION__, p_diff_param->stripe_inaddr[0], model_ofs, buf_ofs);
							break;
						}
						upd_addr = vos_cpu_get_phy_addr(upd_va);
						if (upd_addr == VOS_ADDR_INVALID) {
							DBG_ERR("Invalid upd_va (%#x)\n", upd_va);
						}
#endif
						p_diff_param->stripe_inaddr[0] = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					} else {
						upd_addr = p_diff_param->stripe_inaddr[0];
						p_diff_param->stripe_inaddr[0] = ((p_ll_cmd[cmd_idx] & 0xFFFFFFFF) - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
					}
					p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_addr);
				}
				break;
				case 0x20: {
				UINT32 upd_val = p_diff_param->in_ofs[0].line_ofs;
				p_diff_param->in_ofs[0].line_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x24: {
				UINT32 upd_val = p_diff_param->in_ofs[0].channel_ofs;
				p_diff_param->in_ofs[0].channel_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x28: {
				UINT32 upd_val = p_diff_param->in_ofs[1].line_ofs;
				p_diff_param->in_ofs[1].line_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x2c: {
				UINT32 upd_val = p_diff_param->in_ofs[1].channel_ofs;
				p_diff_param->in_ofs[1].channel_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x44:
				if (p_diff_param->stripe_outaddr[0] > 0) {
					if (mode == 0) {
#if CNN_25_MATLAB
						upd_addr = vos_cpu_get_phy_addr(net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], buf_ofs, model_ofs));
#else
						upd_va = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], model_ofs, buf_ofs);
						if (upd_va == 0) {
							DBG_WRN("%s NULL va, CNN 0x44 stripe_addr(%#x) model_ofs(%#x) buf_ofs(%#x)\n", __FUNCTION__, p_diff_param->stripe_outaddr[0], model_ofs, buf_ofs);
							break;
						}
						upd_addr = vos_cpu_get_phy_addr(upd_va);
						if (upd_addr == VOS_ADDR_INVALID) {
							DBG_ERR("Invalid upd_va (%#x)\n", upd_va);
						}
#endif
						p_diff_param->stripe_outaddr[0] = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					} else {
						upd_addr = p_diff_param->stripe_outaddr[0];
						p_diff_param->stripe_outaddr[0] = ((p_ll_cmd[cmd_idx] & 0xFFFFFFFF) - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
					}
					p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_addr);
				}
				break;
				case 0x48:
				if (p_diff_param->stripe_outaddr[1] > 0) {
					if (mode == 0) {
#if CNN_25_MATLAB
						upd_addr = vos_cpu_get_phy_addr(net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[1], buf_ofs, model_ofs));
#else
						upd_va = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[1], model_ofs, buf_ofs);
						if (upd_va == 0) {
							DBG_WRN("%s NULL va, CNN 0x48 stripe_addr(%#x) model_ofs(%#x) buf_ofs(%#x)\n", __FUNCTION__, p_diff_param->stripe_outaddr[1], model_ofs, buf_ofs);
							break;
						}
						upd_addr = vos_cpu_get_phy_addr(upd_va);
						if (upd_addr == VOS_ADDR_INVALID) {
							DBG_ERR("Invalid upd_va (%#x)\n", upd_va);
						}
#endif
						p_diff_param->stripe_outaddr[1] = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					} else {
						upd_addr = p_diff_param->stripe_outaddr[1];
						p_diff_param->stripe_outaddr[1] = ((p_ll_cmd[cmd_idx] & 0xFFFFFFFF) - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
					}
					p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_addr);
				}
				break;
				case 0x4c: {
				UINT32 upd_val = p_diff_param->out_ofs[0].line_ofs;
				p_diff_param->out_ofs[0].line_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x50: {
				UINT32 upd_val = p_diff_param->out_ofs[0].channel_ofs;
				p_diff_param->out_ofs[0].channel_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x54: {
				UINT32 upd_val = p_diff_param->out_ofs[1].line_ofs;
				p_diff_param->out_ofs[1].line_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x58: {
				UINT32 upd_val = p_diff_param->out_ofs[1].channel_ofs;
				p_diff_param->out_ofs[1].channel_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x5c: {
				UINT32 upd_val = 0 | p_diff_param->in_width | (p_diff_param->in_height << 12);
				p_diff_param->in_width = p_ll_cmd[cmd_idx] & 0x3FF;
				p_diff_param->in_height = (p_ll_cmd[cmd_idx] >> 12) & 0x3FF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x64: {
				UINT32 upd_val = (p_ll_cmd[cmd_idx] & 0xfffffff9) | (p_diff_param->h_stripe_en << 1) | (p_diff_param->v_stripe_en << 2);
				p_diff_param->h_stripe_en = (p_ll_cmd[cmd_idx] >> 1) & 0x1;
				p_diff_param->v_stripe_en = (p_ll_cmd[cmd_idx] >> 2) & 0x1;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x6c: {
				UINT32 upd_val = (p_ll_cmd[cmd_idx] & 0xfffffff0) | ((p_diff_param->conv_pad.top_pad_num > 0)?1:0)
																| (((p_diff_param->conv_pad.bot_pad_num > 0)?1:0) << 1)
																| (((p_diff_param->conv_pad.left_pad_num > 0)?1:0) << 2)
																| (((p_diff_param->conv_pad.right_pad_num > 0)?1:0) << 3);
				p_diff_param->conv_pad.top_pad_num = (p_ll_cmd[cmd_idx]) & 0x1;
				p_diff_param->conv_pad.bot_pad_num = (p_ll_cmd[cmd_idx] >> 1) & 0x1;
				p_diff_param->conv_pad.left_pad_num = (p_ll_cmd[cmd_idx] >> 2) & 0x1;
				p_diff_param->conv_pad.right_pad_num = (p_ll_cmd[cmd_idx] >> 3) & 0x1;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);

				}
				break;
				case 0x98: {
				UINT32 upd_val = (p_ll_cmd[cmd_idx] & 0xffff0fff) | (((p_diff_param->pool_pad.top_pad_num > 0)?1:0) << 12)
																| (((p_diff_param->pool_pad.bot_pad_num > 0)?1:0) << 13)
																| (((p_diff_param->pool_pad.left_pad_num > 0)?1:0) << 14)
																| (((p_diff_param->pool_pad.right_pad_num > 0)?1:0) << 15);
				p_diff_param->pool_pad.top_pad_num = (p_ll_cmd[cmd_idx] >> 12) & 0x1;
				p_diff_param->pool_pad.bot_pad_num = (p_ll_cmd[cmd_idx] >> 13) & 0x1;
				p_diff_param->pool_pad.left_pad_num = (p_ll_cmd[cmd_idx] >> 14) & 0x1;
				p_diff_param->pool_pad.right_pad_num = (p_ll_cmd[cmd_idx] >> 15) & 0x1;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0xa0: {
				UINT32 upd_val = (p_ll_cmd[cmd_idx] & 0xffffff00) | ((p_diff_param->deconv_pad.top_pad_num & 0x3))
																| ((p_diff_param->deconv_pad.bot_pad_num & 0x3) << 2)
																| ((p_diff_param->deconv_pad.left_pad_num & 0x3) << 4)
																| ((p_diff_param->deconv_pad.right_pad_num & 0x3) << 6);
				p_diff_param->deconv_pad.top_pad_num = (p_ll_cmd[cmd_idx]) & 0x3;
				p_diff_param->deconv_pad.bot_pad_num = (p_ll_cmd[cmd_idx] >> 2) & 0x3;
				p_diff_param->deconv_pad.left_pad_num = (p_ll_cmd[cmd_idx] >> 4) & 0x3;
				p_diff_param->deconv_pad.right_pad_num = (p_ll_cmd[cmd_idx] >> 6) & 0x3;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				default:
				break;
			}

			cmd_idx++;
		}

		break;
	case AI_ENG_NUE:
		p_ll_cmd = (UINT64 *)p_head->parm_addr;
		while (((p_ll_cmd[cmd_idx] >> 60) & 0xf) == 8) {
			reg_ofs = (p_ll_cmd[cmd_idx] >> 32) & 0xfff;
			switch(reg_ofs) {
				case 0x0c:
				if (p_diff_param->stripe_inaddr[0] > 0) {
					if (mode == 0) {
#if CNN_25_MATLAB
						upd_addr = vos_cpu_get_phy_addr(net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[0], buf_ofs, model_ofs));
#else
						upd_va = net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[0], model_ofs, buf_ofs);
						if (upd_va == 0) {
							DBG_WRN("%s NULL va, NUE 0x0c stripe_addr(%#x) model_ofs(%#x) buf_ofs(%#x)\n", __FUNCTION__, p_diff_param->stripe_inaddr[0], model_ofs, buf_ofs);
							break;
						}
						upd_addr = vos_cpu_get_phy_addr(upd_va);
						if (upd_addr == VOS_ADDR_INVALID) {
							DBG_ERR("Invalid upd_va (%#x)\n", upd_va);
						}
#endif
						p_diff_param->stripe_inaddr[0] = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					} else {
						upd_addr = p_diff_param->stripe_inaddr[0];
						p_diff_param->stripe_inaddr[0] = ((p_ll_cmd[cmd_idx] & 0xFFFFFFFF) - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
					}
					p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_addr);
				}
				break;
				case 0x10:
				if (p_diff_param->stripe_inaddr[1] > 0) {
					if (mode == 0) {
#if CNN_25_MATLAB
						upd_addr = vos_cpu_get_phy_addr(net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[1], buf_ofs, model_ofs));
#else
						upd_va = net_map_addr_with_parsflag(p_diff_param->stripe_inaddr[1], model_ofs, buf_ofs);
						if (upd_va == 0) {
							DBG_WRN("%s NULL va, NUE 0x10 stripe_addr(%#x) model_ofs(%#x) buf_ofs(%#x)\n", __FUNCTION__, p_diff_param->stripe_inaddr[1], model_ofs, buf_ofs);
							break;
						}
						upd_addr = vos_cpu_get_phy_addr(upd_va);
						if (upd_addr == VOS_ADDR_INVALID) {
							DBG_ERR("Invalid upd_va (%#x)\n", upd_va);
						}
#endif
						p_diff_param->stripe_inaddr[1] = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					} else {
						upd_addr = p_diff_param->stripe_inaddr[1];
						p_diff_param->stripe_inaddr[1] = ((p_ll_cmd[cmd_idx] & 0xFFFFFFFF) - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
					}
					p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_addr);
				}
				break;
				case 0x2c:
				if (p_diff_param->stripe_outaddr[0] > 0) {
					if (mode == 0) {
#if CNN_25_MATLAB
						upd_addr = vos_cpu_get_phy_addr(net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], buf_ofs, model_ofs));
#else
						upd_va = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], model_ofs, buf_ofs);
						if (upd_va == 0) {
							DBG_WRN("%s NULL va, NUE 0x2c stripe_addr(%#x) model_ofs(%#x) buf_ofs(%#x)\n", __FUNCTION__, p_diff_param->stripe_outaddr[0], model_ofs, buf_ofs);
							break;
						}
						upd_addr = vos_cpu_get_phy_addr(upd_va);
						if (upd_addr == VOS_ADDR_INVALID) {
							DBG_ERR("Invalid upd_va (%#x)\n", upd_va);
						}
#endif
						p_diff_param->stripe_outaddr[0] = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					} else {
						upd_addr = p_diff_param->stripe_outaddr[0];
						p_diff_param->stripe_outaddr[0] = ((p_ll_cmd[cmd_idx] & 0xFFFFFFFF) - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
					}
					p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_addr);
				}
				break;
				case 0x40: {
				UINT32 upd_val = 0 | p_diff_param->in_width | (p_diff_param->in_height << 16);
				p_diff_param->in_width = p_ll_cmd[cmd_idx] & 0x3FFF;
				p_diff_param->in_height = (p_ll_cmd[cmd_idx] >> 16) & 0x3FFF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				default:
				break;
			}
			cmd_idx++;
		}
		break;
	case AI_ENG_NUE2:
		p_ll_cmd = (UINT64 *)p_head->parm_addr;
		while (((p_ll_cmd[cmd_idx] >> 60) & 0xf) == 8) {
			reg_ofs = (p_ll_cmd[cmd_idx] >> 32) & 0xfff;
			switch(reg_ofs) {
				case 0x18:
					if (mode == 0) {
#if CNN_25_MATLAB
						upd_addr = vos_cpu_get_phy_addr(net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], buf_ofs, model_ofs));
#else
						upd_va = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[0], model_ofs, buf_ofs);
						if (upd_va == 0) {
							DBG_WRN("%s NULL va, NUE2 0x18 stripe_addr(%#x) model_ofs(%#x) buf_ofs(%#x)\n", __FUNCTION__, p_diff_param->stripe_outaddr[0], model_ofs, buf_ofs);
							break;
						}
						upd_addr = vos_cpu_get_phy_addr(upd_va);
						if (upd_addr == VOS_ADDR_INVALID) {
							DBG_ERR("Invalid upd_va (%#x)\n", upd_va);
						}

#endif
						p_diff_param->stripe_outaddr[0] = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					} else {
						upd_addr = p_diff_param->stripe_outaddr[0];
						/* skip restore if it's ZERO */
						if (upd_addr == 0) {
							break;
						}
						p_diff_param->stripe_outaddr[0] = ((p_ll_cmd[cmd_idx] & 0xFFFFFFFF) - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
					}
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_addr);
				break;
				case 0x1c:
					if (mode == 0) {
#if CNN_25_MATLAB
						upd_addr = vos_cpu_get_phy_addr(net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[1], buf_ofs, model_ofs));
#else
						if (OUT_BUF_ATTR_GET(p_mctrl, NN_OUT_BUF_ATTR_PREPROC_OUT_FMT) == 4) { //FMT_Y_ONLY
							upd_addr = 0;
						} else {
							upd_va = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[1], model_ofs, buf_ofs);
							if (upd_va == 0) {
								DBG_WRN("%s NULL va, NUE2 0x1c stripe_addr(%#x) model_ofs(%#x) buf_ofs(%#x)\n", __FUNCTION__, p_diff_param->stripe_outaddr[1], model_ofs, buf_ofs);
								break;
							}
							upd_addr = vos_cpu_get_phy_addr(upd_va);
							if (upd_addr == VOS_ADDR_INVALID) {
								DBG_ERR("Invalid upd_va (%#x)\n", upd_va);
							}
						}
#endif
						p_diff_param->stripe_outaddr[1] = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					} else {
						upd_addr = p_diff_param->stripe_outaddr[1];
						/* skip restore if it's ZERO */
						if (upd_addr == 0) {
							break;
						}
						p_diff_param->stripe_outaddr[1] = ((p_ll_cmd[cmd_idx] & 0xFFFFFFFF) - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
					}
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_addr);
				break;
				case 0x20:
					if (mode == 0) {
#if CNN_25_MATLAB
						upd_addr = vos_cpu_get_phy_addr(net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[2], buf_ofs, model_ofs));
#else
						if (OUT_BUF_ATTR_GET(p_mctrl, NN_OUT_BUF_ATTR_PREPROC_OUT_FMT) == 4) { //FMT_Y_ONLY
							upd_addr = 0;
						} else {
							upd_va = net_map_addr_with_parsflag(p_diff_param->stripe_outaddr[2], model_ofs, buf_ofs);
							if (upd_va == 0) {
								DBG_WRN("%s NULL va, NUE2 0x20 stripe_addr(%#x) model_ofs(%#x) buf_ofs(%#x)\n", __FUNCTION__, p_diff_param->stripe_outaddr[2], model_ofs, buf_ofs);
								break;
							}
							upd_addr = vos_cpu_get_phy_addr(upd_va);
							if (upd_addr == VOS_ADDR_INVALID) {
								DBG_ERR("Invalid upd_va (%#x)\n", upd_va);
							}
						}
#endif
						p_diff_param->stripe_outaddr[2] = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					} else {
						upd_addr = p_diff_param->stripe_outaddr[2];
						/* skip restore if it's ZERO */
						if (upd_addr == 0) {
							break;
						}
						p_diff_param->stripe_outaddr[2] = ((p_ll_cmd[cmd_idx] & 0xFFFFFFFF) - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
					}
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_addr);
				break;
				case 0x30: {
					UINT32 upd_val = p_diff_param->out_ofs[0].line_ofs;
					p_diff_param->out_ofs[0].line_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x34: {
					UINT32 upd_val = p_diff_param->out_ofs[1].line_ofs;
					p_diff_param->out_ofs[1].line_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x38: {
					UINT32 upd_val = p_diff_param->out_ofs[2].line_ofs;
					p_diff_param->out_ofs[2].line_ofs = p_ll_cmd[cmd_idx] & 0xFFFFFFFF;
					p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x58: {
				UINT32 upd_val = (p_diff_param->sca_width) | (p_diff_param->sca_height << 16);
				p_diff_param->sca_width  = (p_ll_cmd[cmd_idx]) & 0x7FF;
				p_diff_param->sca_height = (p_ll_cmd[cmd_idx] >> 16) & 0x7FF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x5c: {
				UINT32 upd_val = (p_diff_param->mean_width) | (p_diff_param->mean_height << 16);
				p_diff_param->mean_width  = (p_ll_cmd[cmd_idx]) & 0x7FF;
				p_diff_param->mean_height = (p_ll_cmd[cmd_idx] >> 16) & 0x7FF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x68:{
				UINT32 upd_val = (p_diff_param->tc_width) | (p_diff_param->tc_height << 16);
				p_diff_param->tc_width  = (p_ll_cmd[cmd_idx]) & 0x7FF;
				p_diff_param->tc_height = (p_ll_cmd[cmd_idx] >> 16) & 0x7FF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;
				case 0x70:{
				UINT32 upd_val = (p_diff_param->pad_out_width) | (p_diff_param->pad_out_height << 16);
				p_diff_param->pad_out_width  = (p_ll_cmd[cmd_idx]) & 0x7FF;
				p_diff_param->pad_out_height = (p_ll_cmd[cmd_idx] >> 16) & 0xFFF;
				p_ll_cmd[cmd_idx] = nvt_ai_set_upd_cmd(0xf, reg_ofs, upd_val);
				}
				break;

				default:
				break;
			}
			cmd_idx++;
		}
		break;
	default:
		DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}

	

}

ER nvt_ai_update_net_online(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, NN_DIFF_MODEL_HEAD *p_model_head, UINT32 model_id, UINT32 net_id)
{
	UINT32 model_num = 0;
	ER er = E_OK;
	NN_DIFF_MODE_CTRL* p_model_diff_mode_ctrl = NULL;
	NN_DIFF_HEAD* p_model_diff_head = NULL;
//	NN_DIFF_IOMEM *p_diff_io_mem;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 parm_ofs, model_ofs, buff_ofs;
	UINT32 process_index = 0;//, idx = 0;
	NN_GEN_NET_INFO net_info = {0};
	uintptr_t diff_param_ptr = 0;
	NN_DIFF_PARM* p_diff_param;
	UINT8 fmt_sub, fmt_sub_multiCust;
//	UINT32 proc_idx = 0, proc_num = 0;
//	UINT32 layer_index = 0;
//	UINT32 idx = 0;
//	NN_DATA* p_sai = NULL;
//	NN_DATA* p_sao = NULL;

	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("exeed net_id!\r\n");
		return E_CTX;
	}

	p_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];
	// get original model info
	if ((p_mem == NULL) || (p_mem->user_parm.va == 0) || (p_mem->user_model.va == 0) || (p_mem->user_buff.va == 0) ) {
		DBG_ERR("invalid memory input\r\n");
		return E_CTX;
	}

	if (nvt_ai_is_mem_overflow(p_mem) || nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_pars_net fail...\r\n");
		return E_CTX;
	}

    er = nvt_ai_get_net_info(&net_info, p_mem->kerl_parm.va);
    if (er != E_OK) {
        DBG_ERR("nvt_ai_get_net_info fail...\r\n");
        return er;
    }

	// get diff model info
	if (p_model_head == NULL) {
		DBG_ERR("invalid memory input\r\n");
		return E_CTX;
	}

	model_num = p_model_head->stripe_model_num;

	if (model_id >= model_num) {
		DBG_ERR("model id is exceed the max diff model num\r\n");
		return E_CTX;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	fmt_sub  = (UINT8)(((p_head->chip.id) & 0xFF000000) >> 24);
	fmt_sub_multiCust = (UINT8)(fmt_sub & NN_CHIP_AI_SUBVER3_MASK);

	parm_ofs = ALIGN_CEIL_4(p_mem->kerl_parm.va);
	model_ofs = ALIGN_CEIL_4(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_model.va);
	buff_ofs = ALIGN_CEIL_4(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff.va);

	p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_id*sizeof(NN_DIFF_MODE_CTRL));
	p_model_diff_head = (NN_DIFF_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
//	p_diff_io_mem = (NN_DIFF_IOMEM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_HEAD));
	if (((p_head->chip.gentool_vers & 0xFF000000)>>24) <= 0x15) { // for gentool 2.0
		diff_param_ptr  = (uintptr_t)((uintptr_t)p_model_diff_head + sizeof(NN_DIFF_HEAD) + p_head->layer_num*sizeof(NN_DIFF_IOMEM));
	} else { // for AI_Tool 3.0
		diff_param_ptr = (uintptr_t)((uintptr_t)p_model_diff_head + sizeof(NN_DIFF_HEAD) + p_model_diff_head->io_buff_size);
	}

	/*
	layer_index = 0;
	proc_num = p_head->mode_ctrl_num;
	for (proc_idx = 0; proc_idx < proc_num; proc_idx++) {
		if (p_mctrl[proc_idx].layer_index == layer_index) {
			p_sai = (NN_DATA*)p_mctrl[proc_idx].iomem.imem_addr;
			p_sao = (NN_DATA*)p_mctrl[proc_idx].iomem.omem_addr;
			for (idx = 0; idx < p_mctrl[proc_idx].iomem.imem_cnt; idx++) {
				if (layer_index == 0) {
					break;
				}
				if (p_diff_io_mem[layer_index].SAI[idx].va == 0) {
					continue;
				}
			//DBG_IND("layer %d in %d addr = 0x%08X\n", (int)layer_index, (int)idx, (unsigned int)p_diff_io_mem[layer_index].SAI[idx].va);
			p_sai[idx].va = net_map_addr_with_parsflag(p_diff_io_mem[layer_index].SAI[idx].va, model_ofs, buff_ofs);
			p_sai[idx].pa = nvt_ai_va2pa(p_sai[idx].va);
			p_sai[idx].size = p_diff_io_mem[layer_index].SAI[idx].size;			
			}
			for (idx = 0; idx < p_mctrl[proc_idx].iomem.omem_cnt; idx++) {
				//DBG_IND("layer %d out %d addr = 0x%08X\n", (int)layer_index, (int)idx, (unsigned int)p_diff_io_mem[layer_index].SAO[idx].va);
				p_sao[idx].va = net_map_addr_with_parsflag(p_diff_io_mem[layer_index].SAO[idx].va, model_ofs, buff_ofs);
				p_sao[idx].pa = nvt_ai_va2pa(p_sao[idx].va);
				p_sao[idx].size = p_diff_io_mem[layer_index].SAO[idx].size;
				//DBG_IND("p_diff_io_mem[%u].SAO[%u].size = %u\n",layer_index,idx,p_diff_io_mem[layer_index].SAO[idx].size);
			}
			layer_index++;
			if (layer_index == p_head->layer_num) {
				break;
			}
		}
	}
	*/
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		uintptr_t tmp_idx = (fmt_sub_multiCust==NN_CHIP_AI_SUBVER3? sizeof(NN_DIFF_PARM_MULTICUST)*process_index: sizeof(NN_DIFF_PARM)*process_index);
		p_diff_param = (NN_DIFF_PARM*)(diff_param_ptr + tmp_idx);

		if (p_diff_param->skip_en == 1) {
			p_mctrl[process_index].tot_trig_eng_times = 0;
		} else {
			p_mctrl[process_index].tot_trig_eng_times = 1;
		}
	}
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		UINT32 tmp_process_index = process_index;
		uintptr_t tmp_idx = (fmt_sub_multiCust==NN_CHIP_AI_SUBVER3? sizeof(NN_DIFF_PARM_MULTICUST)*process_index: sizeof(NN_DIFF_PARM)*process_index);
		p_diff_param = (NN_DIFF_PARM*)(diff_param_ptr + tmp_idx);

		if (p_diff_param->skip_en == 1) {
			continue;
		}
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			//TODO: update app parameter from diff info
			nvt_ai_update_app_info(p_mctrl[process_index].addr, p_diff_param, parm_ofs, model_ofs, buff_ofs, net_id, 0);
			if (p_mctrl[process_index].tot_trig_eng_times > 1) {
				UINT32 stripe_idx = 0;
				for(stripe_idx = 1; stripe_idx < p_mctrl[process_index].tot_trig_eng_times; stripe_idx++){
					uintptr_t tmp_idx = (fmt_sub_multiCust==NN_CHIP_AI_SUBVER3? sizeof(NN_DIFF_PARM_MULTICUST)*(process_index+stripe_idx): sizeof(NN_DIFF_PARM)*(process_index+stripe_idx));
					p_diff_param = (NN_DIFF_PARM*)(diff_param_ptr + tmp_idx);
					if (p_diff_param->skip_en == 1) {
						break;
					}
				}
				if (stripe_idx != p_mctrl[process_index].tot_trig_eng_times) {
					if (kflow_ai_net_global.g_ai_ll_modify_num[net_id] < AI_MAX_MODIFY_NUM) {
						kflow_ai_net_global.g_ai_ll_modify_idx[net_id][kflow_ai_net_global.g_ai_ll_modify_num[net_id]] = process_index;
						kflow_ai_net_global.g_ai_ll_modify_cmd[net_id][kflow_ai_net_global.g_ai_ll_modify_num[net_id]] = p_mctrl[process_index].tot_trig_eng_times;
						kflow_ai_net_global.g_ai_ll_modify_num[net_id]++;
						p_mctrl[process_index].tot_trig_eng_times = stripe_idx + 1;
					} else {
						DBG_ERR("tmp buffer for keep original model info is not enough!!\r\n");
					}
				}
			}
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			nvt_ai_update_ll_info(&p_mctrl[process_index], p_mctrl[process_index].addr, p_diff_param, parm_ofs, model_ofs, buff_ofs, net_id, 0);
			if (process_index < proc_layer_num - 1) {
				UINT32 skip_idx = 0;
				for (skip_idx = process_index + 1; skip_idx < proc_layer_num; skip_idx++) {
					uintptr_t tmp_idx = (fmt_sub_multiCust==NN_CHIP_AI_SUBVER3? sizeof(NN_DIFF_PARM_MULTICUST)*skip_idx: sizeof(NN_DIFF_PARM)*skip_idx);
					p_diff_param = (NN_DIFF_PARM*)(diff_param_ptr + tmp_idx);

					if (p_diff_param->skip_en == 0) {
						break;
					}
				}
				if (skip_idx > process_index + 1) {
					KDRV_AI_LL_HEAD *p_ll_head = (KDRV_AI_LL_HEAD *)p_mctrl[process_index].addr;
					UINT64* p_ll_end_cmd = (UINT64 *)(p_ll_head->parm_addr + p_ll_head->parm_size - sizeof(UINT64));
					KDRV_AI_LL_HEAD *p_ll_next_head = (KDRV_AI_LL_HEAD *)p_mctrl[skip_idx-1].addr;
					UINT64* p_ll_next_end_cmd = (UINT64 *)(p_ll_next_head->parm_addr + p_ll_next_head->parm_size - sizeof(UINT64));

					if (kflow_ai_net_global.g_ai_ll_modify_num[net_id] < AI_MAX_MODIFY_NUM) {
						kflow_ai_net_global.g_ai_ll_modify_idx[net_id][kflow_ai_net_global.g_ai_ll_modify_num[net_id]] = process_index;
						kflow_ai_net_global.g_ai_ll_modify_cmd[net_id][kflow_ai_net_global.g_ai_ll_modify_num[net_id]] = *p_ll_end_cmd;
						kflow_ai_net_global.g_ai_ll_modify_num[net_id]++;

						//UINT32 next_ll_addr = nvt_ai_get_nextll_addr_cmd(*p_ll_next_end_cmd);
						*p_ll_end_cmd = *p_ll_next_end_cmd;

						process_index = skip_idx - 1;
					} else {
						DBG_ERR("tmp buffer for keep original model cmd is not enough!!\r\n");
					}
				}
			}
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			//nvt_ai_map_drv_addr((NN_MODE)p_mctrl[process_index].mode, p_mctrl[process_index].addr, model_ofs, buff_ofs, net_id);
			break;
		}
		//fmem_dcache_sync((UINT32 *)(p_mctrl[process_index].addr), (p_mctrl[process_index].size), DMA_BIDIRECTIONAL);
		vos_cpu_dcache_sync(p_mctrl[tmp_process_index].addr, p_mctrl[tmp_process_index].size, VOS_DMA_TO_DEVICE);
	}

	return E_OK;
	
}

ER nvt_ai_restore_net_online(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, NN_DIFF_MODEL_HEAD *p_model_head, UINT32 model_id, UINT32 net_id)
{
	// TODO: restore model as original
		UINT32 model_num = 0;
	ER er = E_OK;
	NN_DIFF_MODE_CTRL* p_model_diff_mode_ctrl = NULL;
	NN_DIFF_HEAD* p_model_diff_head = NULL;
#if CNN_25_MATLAB
	NN_IOMEM *p_diff_io_mem;
#endif
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
#if CNN_25_MATLAB
	NN_IOMEM *p_io_mem;
#endif
	UINT32 parm_ofs, model_ofs, buff_ofs;
	UINT32 process_index = 0;//, idx = 0;
	NN_GEN_NET_INFO net_info = {0};
	UINT32 skip_idx = 0;
	uintptr_t diff_param_ptr = 0;
	NN_DIFF_PARM* p_diff_param;
	UINT8 fmt_sub, fmt_sub_multiCust;
#if 1
	//UINT32 layer_index = 0;
#endif

	if (nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("exceed max net_id\r\n");
		return E_CTX;
	}

	p_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];
	// get original model info
	if ((p_mem == NULL) || (p_mem->user_parm.va == 0) || (p_mem->user_model.va == 0) || (p_mem->user_buff.va == 0) ) {
		DBG_ERR("invalid memory input\r\n");
		return E_CTX;
	}

	if (nvt_ai_is_mem_overflow(p_mem) || nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_pars_net fail...\r\n");
		return E_CTX;
	}


    er = nvt_ai_get_net_info(&net_info, p_mem->kerl_parm.va);
    if (er != E_OK) {
        DBG_ERR("nvt_ai_get_net_info fail...\r\n");
        return er;
    }

	// get diff model info
	if (p_model_head == NULL) {
		DBG_ERR("invalid memory input\r\n");
		return E_CTX;
	}

	model_num = p_model_head->stripe_model_num;

	if (model_id >= model_num) {
		DBG_ERR("model id is exceed the max diff model num\r\n");
		return E_CTX;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	fmt_sub  = (UINT8)(((p_head->chip.id) & 0xFF000000) >> 24);
	fmt_sub_multiCust = (UINT8)(fmt_sub & NN_CHIP_AI_SUBVER3_MASK);

#if CNN_25_MATLAB
	p_io_mem = net_info.p_io_mem;
#endif
	proc_layer_num = p_head->mode_ctrl_num;

	parm_ofs = ALIGN_CEIL_4(p_mem->kerl_parm.va);
	model_ofs = ALIGN_CEIL_4(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_model.va);
	buff_ofs = ALIGN_CEIL_4(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff.va);

	p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_id*sizeof(NN_DIFF_MODE_CTRL));
	p_model_diff_head = (NN_DIFF_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
#if CNN_25_MATLAB
	p_diff_io_mem = (NN_IOMEM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_HEAD));
#endif
	if (((p_head->chip.gentool_vers & 0xFF000000)>>24) <= 0x15) { // for gentool 2.0
		diff_param_ptr  = (uintptr_t)((uintptr_t)p_model_diff_head + sizeof(NN_DIFF_HEAD) + p_head->layer_num*sizeof(NN_DIFF_IOMEM));
	} else { // for AI_Tool 3.0
		diff_param_ptr = (uintptr_t)((uintptr_t)p_model_diff_head + sizeof(NN_DIFF_HEAD) + p_model_diff_head->io_buff_size);
	}
#if CNN_25_MATLAB
	if (0) {
		DBG_DUMP("p_io_mem=%08x p_diff_io_mem=%08x\r\n", (int)p_io_mem, (int)p_diff_io_mem);
	}
#endif
	/*
	for (layer_index = 0; layer_index < p_head->layer_num; layer_index++) {
		for (idx = 0; idx < NN_IMEM_NUM; idx++) {
			if (layer_index == 0) {
				continue;
			}
			SWAP(p_diff_io_mem[layer_index].SAI[idx].va, p_io_mem[layer_index].SAI[idx].va);
			SWAP(p_diff_io_mem[layer_index].SAI[idx].pa, p_io_mem[layer_index].SAI[idx].pa);
			p_diff_io_mem[layer_index].SAI[idx].va = (p_diff_io_mem[layer_index].SAI[idx].va - buff_ofs) | NN_GEN_BUF_ADDR_TYPE;
			p_diff_io_mem[layer_index].SAI[idx].pa = nvt_ai_va2pa(p_diff_io_mem[layer_index].SAI[idx].va);

		}
		for (idx = 0; idx < NN_OMEM_NUM; idx++) {
			SWAP(p_diff_io_mem[layer_index].SAO[idx].va, p_io_mem[layer_index].SAO[idx].va);
			SWAP(p_diff_io_mem[layer_index].SAO[idx].pa, p_io_mem[layer_index].SAO[idx].pa);
			p_diff_io_mem[layer_index].SAO[idx].va = (p_diff_io_mem[layer_index].SAO[idx].va - buff_ofs) | NN_GEN_BUF_ADDR_TYPE;
			p_diff_io_mem[layer_index].SAO[idx].pa = nvt_ai_va2pa(p_diff_io_mem[layer_index].SAO[idx].va);
		}
	}
	*/
	//DBG_IND("in restore: va = 0x%08X\n", (unsigned int)buff_ofs);
	buff_ofs = ALIGN_CEIL_4(kflow_ai_net_global.g_ai_user_mem_in_kerl[net_id].user_buff.pa);
	//DBG_IND("in restore: pa = 0x%08X\n", (unsigned int)buff_ofs);
	if (g_scl_restore_en[net_id]) {
		const KDRV_AI_LL_HEAD *p_kerl_ll_head = (KDRV_AI_LL_HEAD  *)p_mctrl[0].addr;
		const UINT32 kerl_ll_parm_addr = p_kerl_ll_head->parm_addr;
		NUE2_LL_PARM *p_kerl_parm = (NUE2_LL_PARM*)kerl_ll_parm_addr;
		UINT32 reg_val = 0;

		// restore the scaling width/height & padding enable
		p_kerl_parm->scale_size.bit.h_scl_size = g_scl_size[net_id][0];
		p_kerl_parm->scale_size.bit.v_scl_size = g_scl_size[net_id][1];

		// padding disable
		nvt_ai_get_ll_reg_val(kerl_ll_parm_addr, 0x4, &reg_val);
		reg_val = reg_val & 0xFFFFFFFB;
		nvt_ai_set_ll_reg_val(kerl_ll_parm_addr, 0x4, reg_val);
		g_scl_restore_en[net_id] = 0;
	}

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		uintptr_t tmp_idx = (fmt_sub_multiCust==NN_CHIP_AI_SUBVER3? sizeof(NN_DIFF_PARM_MULTICUST)*process_index: sizeof(NN_DIFF_PARM)*process_index);
		p_diff_param = (NN_DIFF_PARM*)(diff_param_ptr + tmp_idx);

		if (p_diff_param->skip_en == 1) {
			continue;
		}
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			//TODO: restore app parameter to original info
			nvt_ai_update_app_info(p_mctrl[process_index].addr, p_diff_param, parm_ofs, model_ofs, buff_ofs, net_id, 1);
			if (kflow_ai_net_global.g_ai_ll_modify_num[net_id] > 0 && process_index == kflow_ai_net_global.g_ai_ll_modify_idx[net_id][skip_idx]) {
				p_mctrl[process_index].tot_trig_eng_times = kflow_ai_net_global.g_ai_ll_modify_cmd[net_id][skip_idx];
				kflow_ai_net_global.g_ai_ll_modify_num[net_id]--;
				skip_idx++;
			}
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			nvt_ai_update_ll_info(&p_mctrl[process_index], p_mctrl[process_index].addr, p_diff_param, parm_ofs, model_ofs, buff_ofs, net_id, 1);
			if (kflow_ai_net_global.g_ai_ll_modify_num[net_id] > 0 && process_index == kflow_ai_net_global.g_ai_ll_modify_idx[net_id][skip_idx]) {
				KDRV_AI_LL_HEAD *p_ll_head = (KDRV_AI_LL_HEAD *)p_mctrl[process_index].addr;
				UINT64* p_ll_end_cmd = (UINT64 *)(p_ll_head->parm_addr + p_ll_head->parm_size - sizeof(UINT64));

				*p_ll_end_cmd = kflow_ai_net_global.g_ai_ll_modify_cmd[net_id][skip_idx];
				kflow_ai_net_global.g_ai_ll_modify_num[net_id]--;
				skip_idx++;
			}
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			//nvt_ai_map_drv_addr((NN_MODE)p_mctrl[process_index].mode, p_mctrl[process_index].addr, model_ofs, buff_ofs, net_id);
			break;
		}
		//fmem_dcache_sync((UINT32 *)(p_mctrl[process_index].addr), (p_mctrl[process_index].size), DMA_BIDIRECTIONAL);
		vos_cpu_dcache_sync(p_mctrl[process_index].addr, p_mctrl[process_index].size, VOS_DMA_TO_DEVICE);
	}

	return E_OK;
	
}

ER nvt_ai_update_net_online_batch(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, NN_DIFF_MODEL_HEAD *p_model_head, UINT32 model_id, UINT32 net_id)
{
	UINT32 model_num = 0;
	ER er = E_OK;
	NN_DIFF_MODE_CTRL* p_model_diff_mode_ctrl = NULL;
	NN_DIFF_BATCH_HEAD* p_model_diff_head = NULL;
	//NN_DIFF_IOMEM *p_diff_io_mem;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;

	//UINT32 parm_ofs, model_ofs, buff_ofs;
	UINT32 process_index = 0;//, idx = 0;
	NN_GEN_NET_INFO net_info = {0};
	NN_DIFF_BATCH_PARM* p_diff_param;


	// TODO: no need to assign g_ai_input_imem?
	/*g_ai_input_imem = (NN_DATA **) kdrv_ai_drv_get_ai_input_imem();
	if (g_ai_input_imem == 0) {
		DBG_ERR("Error, invalid g_ai_input_imem input\r\n");
        return E_CTX;
	}*/


	p_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];
	// get original model info
	if ((p_mem == NULL) || (p_mem->user_parm.va == 0) || (p_mem->user_model.va == 0) || (p_mem->user_buff.va == 0) ) {
		DBG_ERR("invalid memory input\r\n");
		return E_CTX;
	}

	if (nvt_ai_is_mem_overflow(p_mem) || nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_pars_net fail...\r\n");
		return E_CTX;
	}

    er = nvt_ai_get_net_info(&net_info, p_mem->kerl_parm.va);
    if (er != E_OK) {
        DBG_ERR("nvt_ai_get_net_info fail...\r\n");
        return er;
    }
	g_scl_restore_en[net_id] = 0;

	// get diff model info
	if (p_model_head == NULL) {
		DBG_ERR("invalid memory input\r\n");
		return E_CTX;
	}

	model_num = p_model_head->stripe_model_num;

	if (model_id >= model_num) {
		DBG_ERR("model id is exceed the max diff model num\r\n");
		return E_CTX;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	proc_layer_num = p_head->mode_ctrl_num;

	//parm_ofs = ALIGN_CEIL_4(p_mem->kerl_parm.va);
	//model_ofs = ALIGN_CEIL_4(g_ai_user_mem_in_kerl[net_id].user_model.va);
	//buff_ofs = ALIGN_CEIL_4(g_ai_user_mem_in_kerl[net_id].user_buff.va);

	p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_id*sizeof(NN_DIFF_MODE_CTRL));
	p_model_diff_head = (NN_DIFF_BATCH_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
	//p_diff_io_mem = (NN_DIFF_IOMEM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_HEAD));
	//p_diff_param  = (NN_DIFF_PARM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_HEAD) + p_model_diff_head->io_buff_size);
	p_diff_param  = (NN_DIFF_BATCH_PARM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_BATCH_HEAD) + p_head->layer_num*sizeof(NN_DIFF_IOMEM));

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		if (p_diff_param[process_index].skip_en == 1) {
			p_mctrl[process_index].tot_trig_eng_times = 0;
		} else {
			p_mctrl[process_index].tot_trig_eng_times = 1;
		}
	}
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		UINT32 tmp_process_index = process_index;
		if (p_diff_param[process_index].skip_en == 1) {
			continue;
		}
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			// not support in app
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			//nvt_ai_update_ll_info(p_mctrl[process_index].addr, &p_diff_param[process_index], parm_ofs, model_ofs, buff_ofs, net_id, 0);
			if (process_index < proc_layer_num - 1) {
				UINT32 skip_idx = 0;
				for (skip_idx = process_index + 1; skip_idx < proc_layer_num; skip_idx++) {
					if (p_diff_param[skip_idx].skip_en == 0) {
						break;
					}
				}
				if (skip_idx > process_index + 1) {
					KDRV_AI_LL_HEAD *p_ll_head = (KDRV_AI_LL_HEAD *)p_mctrl[process_index].addr;
					UINT64* p_ll_end_cmd = (UINT64 *)(p_ll_head->parm_addr + p_ll_head->parm_size - sizeof(UINT64));
					KDRV_AI_LL_HEAD *p_ll_next_head = (KDRV_AI_LL_HEAD *)p_mctrl[skip_idx-1].addr;
					UINT64* p_ll_next_end_cmd = (UINT64 *)(p_ll_next_head->parm_addr + p_ll_next_head->parm_size - sizeof(UINT64));

					if (kflow_ai_net_global.g_ai_ll_modify_num_batch[net_id] < (int)proc_layer_num) {
						UINT32 modify_queue_idx = kflow_ai_net_global.g_ai_ll_modify_num_batch[net_id];
						kflow_ai_net_global.g_ai_ll_modify_idx_batch[net_id][modify_queue_idx] = process_index;
						kflow_ai_net_global.g_ai_ll_modify_cmd_batch[net_id][modify_queue_idx] = *p_ll_end_cmd;
						kflow_ai_net_global.g_ai_ll_modify_num_batch[net_id]++;

						//UINT32 next_ll_addr = nvt_ai_get_nextll_addr_cmd(*p_ll_next_end_cmd);
						*p_ll_end_cmd = *p_ll_next_end_cmd;

						process_index = skip_idx - 1;
					} else {
						DBG_ERR("tmp buffer for keep original model cmd number(%d) is not enough!!\r\n", kflow_ai_net_global.g_ai_ll_modify_num_batch[net_id]);
					}
				}
			}
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			//nvt_ai_map_drv_addr((NN_MODE)p_mctrl[process_index].mode, p_mctrl[process_index].addr, model_ofs, buff_ofs, net_id);
			break;
		}
		//fmem_dcache_sync((UINT32 *)(p_mctrl[process_index].addr), (p_mctrl[process_index].size), DMA_BIDIRECTIONAL);
		vos_cpu_dcache_sync(p_mctrl[tmp_process_index].addr, p_mctrl[tmp_process_index].size, VOS_DMA_TO_DEVICE);
	}

	return E_OK;
}

ER nvt_ai_restore_net_online_batch(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, NN_DIFF_MODEL_HEAD *p_model_head, UINT32 model_id, UINT32 net_id)
{
	UINT32 model_num = 0;
	ER er = E_OK;
	NN_DIFF_MODE_CTRL* p_model_diff_mode_ctrl = NULL;
	NN_DIFF_BATCH_HEAD* p_model_diff_head = NULL;
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	//UINT32 parm_ofs, model_ofs, buff_ofs;
	UINT32 process_index = 0;//, idx = 0;
	NN_GEN_NET_INFO net_info = {0};
	UINT32 skip_idx = 0;
	NN_DIFF_BATCH_PARM* p_diff_param;

	p_mem = &kflow_ai_net_global.g_ai_map_mem[net_id];
	// get original model info
	if ((p_mem == NULL) || (p_mem->user_parm.va == 0) || (p_mem->user_model.va == 0) || (p_mem->user_buff.va == 0) ) {
		DBG_ERR("invalid memory input\r\n");
		return E_CTX;
	}

	if (nvt_ai_is_mem_overflow(p_mem) || nvt_ai_is_net_id_overflow(net_id)) {
		DBG_ERR("nvt_ai_pars_net fail...\r\n");
		return E_CTX;
	}


    er = nvt_ai_get_net_info(&net_info, p_mem->kerl_parm.va);
    if (er != E_OK) {
        DBG_ERR("nvt_ai_get_net_info fail...\r\n");
        return er;
    }

	// get diff model info
	if (p_model_head == NULL) {
		DBG_ERR("invalid memory input\r\n");
		return E_CTX;
	}

	model_num = p_model_head->stripe_model_num;

	if (model_id >= model_num) {
		DBG_ERR("model id is exceed the max diff model num\r\n");
		return E_CTX;
	}

	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;

	proc_layer_num = p_head->mode_ctrl_num;

	//parm_ofs = ALIGN_CEIL_4(p_mem->kerl_parm.va);
	//model_ofs = ALIGN_CEIL_4(g_ai_user_mem_in_kerl[net_id].user_model.va);
	//buff_ofs = ALIGN_CEIL_4(g_ai_user_mem_in_kerl[net_id].user_buff.va);

	p_model_diff_mode_ctrl = (NN_DIFF_MODE_CTRL*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_id*sizeof(NN_DIFF_MODE_CTRL));
	p_model_diff_head = (NN_DIFF_BATCH_HEAD*)((UINT32)p_model_head + sizeof(NN_DIFF_MODEL_HEAD) + model_num*sizeof(NN_DIFF_MODE_CTRL) + p_model_diff_mode_ctrl->offset);
	//p_diff_io_mem = (NN_DIFF_IOMEM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_HEAD));
	p_diff_param  = (NN_DIFF_BATCH_PARM*)((UINT32)p_model_diff_head + sizeof(NN_DIFF_BATCH_HEAD) + p_model_diff_head->io_buff_size);


	//DBG_IND("in restore: va = 0x%08X\n", (unsigned int)buff_ofs);
	//buff_ofs = ALIGN_CEIL_4(g_ai_user_mem_in_kerl[net_id].user_buff.pa);
	//DBG_IND("in restore: pa = 0x%08X\n", (unsigned int)buff_ofs);
	/*if (g_scl_restore_en[net_id]) {
		const KDRV_AI_LL_HEAD *p_kerl_ll_head = (KDRV_AI_LL_HEAD  *)p_mctrl[0].addr;
		const UINT32 kerl_ll_parm_addr = p_kerl_ll_head->parm_addr;
		NUE2_LL_PARM *p_kerl_parm = (NUE2_LL_PARM*)kerl_ll_parm_addr;
		UINT32 reg_val = 0;

		// restore the scaling width/height & padding enable
		p_kerl_parm->scale_size.bit.h_scl_size = g_scl_size[net_id][0];
		p_kerl_parm->scale_size.bit.v_scl_size = g_scl_size[net_id][1];

		// padding disable
		nvt_ai_get_ll_reg_val(kerl_ll_parm_addr, 0x4, &reg_val);
		reg_val = reg_val & 0xFFFFFFFB;
		nvt_ai_set_ll_reg_val(kerl_ll_parm_addr, 0x4, reg_val);
		g_scl_restore_en[net_id] = 0;
	}*/
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		if (p_diff_param[process_index].skip_en == 1)
			continue;
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			// app not support dynamic batch setting
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			//nvt_ai_update_ll_info(p_mctrl[process_index].addr, &p_diff_param[process_index], parm_ofs, model_ofs, buff_ofs, net_id, 1);

			if (kflow_ai_net_global.g_ai_ll_modify_num_batch[net_id] > 0 && process_index == kflow_ai_net_global.g_ai_ll_modify_idx_batch[net_id][skip_idx]) {
				KDRV_AI_LL_HEAD *p_ll_head = (KDRV_AI_LL_HEAD *)p_mctrl[process_index].addr;
				UINT64* p_ll_end_cmd = (UINT64 *)(p_ll_head->parm_addr + p_ll_head->parm_size - sizeof(UINT64));

				*p_ll_end_cmd = kflow_ai_net_global.g_ai_ll_modify_cmd_batch[net_id][skip_idx];
				kflow_ai_net_global.g_ai_ll_modify_num_batch[net_id]--;
				skip_idx++;
			}
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			//nvt_ai_map_drv_addr((NN_MODE)p_mctrl[process_index].mode, p_mctrl[process_index].addr, model_ofs, buff_ofs, net_id);
			break;
		}
		//fmem_dcache_sync((UINT32 *)(p_mctrl[process_index].addr), (p_mctrl[process_index].size), DMA_BIDIRECTIONAL);
		vos_cpu_dcache_sync(p_mctrl[process_index].addr, p_mctrl[process_index].size, VOS_DMA_TO_DEVICE);
	}

	return E_OK;
}

ER nvt_ai_get_id(UINT32 *proc_id)
{
	UINT32 i = 0;
	SEM_WAIT(g_ai_path_sem_id);
	for(i = 0; i < AI_SUPPORT_NET_MAX; i++) {
		if (g_ai_net_path_is_used[i] == FALSE) {
			g_ai_net_path_is_used[i] = TRUE;
			break;
		}
	}
	*proc_id = i;
	SEM_SIGNAL(g_ai_path_sem_id);
	return E_OK;
}

ER nvt_ai_release_id(UINT32 proc_id)
{
	SEM_WAIT(g_ai_path_sem_id);
	if (g_ai_net_path_is_used[proc_id] == FALSE) {
		DBG_ERR("fail to release proc_id(%d), it's NOT in used !!\r\n", (int)proc_id);
		return E_CTX;
	}
	g_ai_net_path_is_used[proc_id] = FALSE;
	SEM_SIGNAL(g_ai_path_sem_id);
	return E_OK;
}

ER nvt_ai_comm_lock(VOID)
{
	SEM_WAIT(g_ai_common_lock);
	return E_OK;
}

ER nvt_ai_comm_unlock(VOID)
{
	SEM_SIGNAL(g_ai_common_lock);
	return E_OK;
}

BOOL nvt_ai_is_path_need_reset(VOID)
{
	UINT32 i;
	for(i = 0;i < AI_SUPPORT_NET_MAX; i++) {
		if(g_ai_net_path_need_reset[i] == 1)
			return 1;
	}
	return 0;
}
#if NET_TABLE_HEAP
ER nvt_ai_alloc_map_table(UINT32 proc_id, UINT32 max_batch)
{
	g_ai_input_layer_map_table[proc_id] = (UINT32 *)nvt_ai_mem_alloc(sizeof(UINT32) * max_batch);
	if (g_ai_input_layer_map_table[proc_id] == NULL) {
		return E_NOMEM;
	}
	memset(g_ai_input_layer_map_table[proc_id], 0x0, sizeof(UINT32) * max_batch);

	g_ai_input_batch_imem[proc_id] = (NN_DATA (*)[NN_IMEM_NUM])nvt_ai_mem_alloc(sizeof(NN_DATA) * NN_IMEM_NUM * max_batch);
	if (g_ai_input_batch_imem[proc_id] == NULL) {
		return E_NOMEM;
	}
	memset(g_ai_input_batch_imem[proc_id], 0x0, sizeof(NN_DATA) * NN_IMEM_NUM * max_batch);
	
	return E_OK;
}

ER nvt_ai_free_map_table(UINT32 proc_id)
{
	if (g_ai_input_layer_map_table[proc_id]) {
		nvt_ai_mem_free(g_ai_input_layer_map_table[proc_id]);
		g_ai_input_layer_map_table[proc_id] = 0;
	}	
	if (g_ai_input_batch_imem[proc_id]) {
		nvt_ai_mem_free(g_ai_input_batch_imem[proc_id]);
		g_ai_input_batch_imem[proc_id] = 0;
	}
	return E_OK;
}
#endif
MODULE_AUTHOR("Novatek Microelectronics Corp.");
//MODULE_LICENSE("NVT");
MODULE_LICENSE("GPL");
MODULE_VERSION(KFLOW_AI_IMPL_VERSION);
