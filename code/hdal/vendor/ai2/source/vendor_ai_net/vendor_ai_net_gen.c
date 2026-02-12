/**
	@brief Source file of vendor net generation sample.

	@file net_gen_sample.c

	@ingroup net_gen_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

/*-----------------------------------------------------------------------------*/
/* Include Files                                                               */
/*-----------------------------------------------------------------------------*/
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include <stdio.h>
#include <string.h>
#include "hd_type.h"
#include "hd_common.h"

#include "kflow_ai_net/kflow_ai_net.h"
#include "kflow_ai_net/kflow_ai_net_comm.h"
#include "kflow_ai_net/nn_net.h"
#include "kflow_ai_net/nn_parm.h"
#include "kflow_ai_net/nn_verinfo.h"
#include "kflow_ai_net/nn_dli.h"
#include "vendor_ai.h"
#include "vendor_ai_dla/vendor_ai_dla.h"
#include "vendor_ai_preproc.h"
#include "vendor_ai_net_gen.h"
#include "vendor_ai_net_flow.h"
#include "vendor_ai_net_debug.h"
#include "nvt_ai.h"
#include "vendor_ai_version.h"
#include "vendor_ai_comm_flow.h"

//=============================================================
#define __CLASS__ 				"[ai][lib][gen]"
#include "vendor_ai_debug.h"
//=============================================================


/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
//#define VENDOR_AIS_NC_MEM_SZ    0x01200000

#define AI_DUMMY_ADDR           0x01234560
#define MODIFY_ENG_ONLINE       0
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
#if CNN_25_MATLAB
static NN_IOMEM g_fist_layer_mem[NN_SUPPORT_NET_MAX] = {0};
#else
#if (NET_GEN_HEAP == 1)
#else
static NN_DATA **g_fist_layer_imem = NULL;
#endif
#endif
VENDOR_AIS_FLOW_MAP_MEM_PARM *g_map_mem = NULL;
#if (NET_GEN_HEAP == 1)
#else
static UINT32 *g_ai_net_gen_init = NULL;
#endif
extern UINT32 g_ai_support_net_max;
extern BOOL is_multi_process;

#if CNN_MULTI_INPUT
static BOOL g_ai_net_is_batch[NN_SUPPORT_NET_MAX] = {0};
static UINT32 g_proc_idx_map_index[NN_SUPPORT_NET_MAX] = {0}; // input index for proc_idx_map_table
#if NET_TABLE_HEAP
UINT32 **g_proc_idx_map_table = NULL; // for input_init and input_uninit proc_idx mapping record
UINT32 *g_proc_max_batch_size= NULL;
#else
static UINT32 g_proc_idx_map_table[NN_SUPPORT_NET_MAX][AI2_MAX_BATCH_NUM] = {0}; // for input_init and input_uninit proc_idx mapping record
#endif 
#endif

UINT32 g_max_out_num = 0;
UINT32 g_max_map_num = 0;
uintptr_t* g_next_cust_parm_addr = 0;
UINT32* g_next_cust_match_in_idx = 0;

/*-----------------------------------------------------------------------------*/
/* Local Functions                                                             */
/*-----------------------------------------------------------------------------*/
UINT32 vendor_ais_get_nextll_addr_cmd(UINT64 cmd);
UINT64 vendor_ais_set_nextll_addr_cmd(UINT64 cmd, UINT32 addr);
UINT64 vendor_ais_ll_nextupd_cmd(UINT32 addr);
VOID vendor_ais_map_ai_app_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id);
#if !CNN_25_MATLAB
HD_RESULT vendor_ais_unpars_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id);
BOOL vendor_ais_ll_is_nextupd_cmd(UINT64 cmd);
VOID vendor_ais_map_ai_ll_addr(UINT32 parm_addr, UINT32 parm_va_ofs, UINT32 parm_pa_ofs, UINT32 model_pa_ofs, UINT32 buf_pa_ofs, UINT32 net_id);
VOID vendor_ais_unmap_ai_app_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end, UINT32 net_id);
VOID vendor_ais_unmap_ai_ll_addr(UINT32 parm_addr, UINT32 parm_va_ofs, UINT32 parm_pa_ofs, UINT32 model_pa_ofs, UINT32 buf_pa_ofs, UINT32 model_pa_end, UINT32 buf_pa_end, UINT32 net_id);
VOID vendor_ais_unmap_drv_addr(NN_MODE mode, UINT32 parm_addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end, UINT32 net_id);
#else
VOID vendor_ais_map_ai_ll_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id);
#endif
VOID vendor_ais_map_drv_addr(NN_MODE mode, UINT32 parm_addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id);
HD_RESULT vendor_ais_pars_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id);
UINT32 vendor_ais_user_mem_va2pa(UINT32 addr, VENDOR_AIS_FLOW_MEM_PARM mem_range);
UINT32 vendor_ais_user_mem_pa2va(UINT32 addr, VENDOR_AIS_FLOW_MEM_PARM mem_range);

#if NN_DLI
NN_DLI_TENSOR_INFO_HEADER *vendor_ais_get_nn_dli_tensor_info_header(NN_GEN_MODEL_HEAD *p_head, UINT32 parm_va_ofs);
VOID vendor_ais_map_nn_dli_tensor_addr(NN_GEN_MODEL_HEAD *p_head, UINT32 parm_va_ofs, UINT32 model_ofs, UINT32 buf_ofs);
VOID vendor_ais_unmap_nn_dli_tensor_addr(NN_GEN_MODEL_HEAD *p_head, UINT32 parm_va_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end);
#endif

/*-----------------------------------------------------------------------------*/
/* Version check                                                               */
/*-----------------------------------------------------------------------------*/

#define AI_LIB_CHK_VER           "02.17"    // lib version must == this version
#define AI_KFLOW_CHK_VER         "02.14"    // kflow version must >= this version
#define AI_KDRV_CHK_VER          "01.17"    // kdrv version must >= this version
#define AI_PLUGIN_CHK_VER        "01.02"    // plugin version must >= this version
#define CPU_PREBUILD_CHK_VER     "02.18"    // cpu prebuild version must >= this version

#define AI_KFLOW_CHK_VERINT      0x00020014 // int value of AI_KFLOW_CHK_VER
#define AI_KDRV_CHK_VERINT       0x00010017 // int value of AI_KDRV_CHK_VER
#define AI_PLUGIN_CHK_VERINT     0x00010002 // int value of AI_PLUGIN_CHK_VER
#define CPU_PREBUILD_CHK_VERINT  0x00020018 // int value of CPU_PREBUILD_CHK_VER

#define VERSION_STRLEN           32

char lib_ver[32] = "version="VENDOR_AI_IMPL_VERSION; //for grep version

CHAR* vendor_ais_chk_sdk_version(VOID)
{
	return VENDOR_AI_IMPL_VERSION;
}

extern VENDOR_AI_ENGINE_PLUGIN *vendor_ai_get_engine (UINT32 eng_id);

BOOL vendor_ais_check_component_ver(VOID)
{
	CHAR* usr_ver = {0};
	CHAR  ker_ver[VERSION_STRLEN];
	UINT32 kdrv_main_ver = 0;
	UINT32 kdrv_sub_ver = 0;
	UINT32 kflow_main_ver = 0;
	UINT32 kflow_sub_ver = 0;
	UINT32 plugin_main_ver = 0;
	UINT32 plugin_sub_ver = 0;
	UINT32 cpu_prebuild_main_ver  = 0;
	UINT32 cpu_prebuild_sub_ver   = 0;

	// chk lib ver
	usr_ver = vendor_ais_chk_sdk_version();
	if(strncmp(usr_ver, AI_LIB_CHK_VER, strlen(AI_LIB_CHK_VER)) != 0) {
		DBG_ERR("library version check fail! library current version(%s) is too old, must equal or larger than required_version(%s)\r\n", usr_ver, AI_LIB_CHK_VER);
		return FALSE;
	}

	// chk kdrv ver
	vendor_ai_get_kdrv_version(ker_ver);
	kdrv_main_ver = strtoul(ker_ver, NULL, 16);
	kdrv_sub_ver  = strtoul(&ker_ver[3], NULL, 16);
	if ((kdrv_main_ver != (AI_KDRV_CHK_VERINT >> 16)) || (kdrv_sub_ver < (AI_KDRV_CHK_VERINT & 0xFFFF))) {
		DBG_ERR("kdrv version check fail! kdrv current version(%#x) is too old, must equal or larger than required_version(%#x)\r\n", (int)((kdrv_main_ver << 16) | kdrv_sub_ver), (int)AI_KDRV_CHK_VERINT);
		return FALSE;
	}

	// chk kflow ver
	vendor_ai_get_kflow_version(ker_ver);
	kflow_main_ver = strtoul(ker_ver, NULL, 16);
	kflow_sub_ver  = strtoul(&ker_ver[3], NULL, 16);
	if ((kflow_main_ver != (AI_KFLOW_CHK_VERINT >> 16)) || (kflow_sub_ver < (AI_KFLOW_CHK_VERINT & 0xFFFF))) {
		DBG_ERR("kflow version check fail! kflow current version(%#x) is too old, must equal or larger than required_version(%#x)\r\n", (int)((kflow_main_ver << 16) | kflow_sub_ver), (int)AI_KFLOW_CHK_VERINT);
		return FALSE;
	}

	// chk all plugin ver
	// see IVOT_TP-325: query version from plugin interface
	{
		VENDOR_AI_ENGINE_PLUGIN* p_cpu_engine = vendor_ai_get_engine(0); //CPU engine
		
		if (p_cpu_engine->get_cb != 0) {
			
			HD_RESULT rv = HD_ERR_NOT_SUPPORT;

			// chk plugin ver
			rv = p_cpu_engine->get_cb(VENDOR_AI_CTRL_NET, VENDOR_AI_CTRL_LYR, 0, (UINT32)32*sizeof(CHAR), (UINT32)ker_ver, VENDOR_AI_PLUGIN_VER, 0, 0);
			if (rv != HD_OK) {
				DBG_ERR("plugin cannot get version?\r\n");
				return FALSE;
			}
			plugin_main_ver = strtoul(ker_ver, NULL, 16);
			plugin_sub_ver  = strtoul(&ker_ver[3], NULL, 16);
			if ((plugin_main_ver != (AI_PLUGIN_CHK_VERINT >> 16)) || (plugin_sub_ver < (AI_PLUGIN_CHK_VERINT & 0xFFFF))) {
				DBG_ERR("plugin version check fail! plugin version(%#x), check_ver(%#x)\r\n", (int)((plugin_main_ver << 16) | plugin_sub_ver), (int)AI_PLUGIN_CHK_VER);
				return FALSE;
			}

			// chk cpu prebuild ver
			rv = p_cpu_engine->get_cb(VENDOR_AI_CTRL_NET, VENDOR_AI_CTRL_LYR, 0, (UINT32)VERSION_STRLEN*sizeof(CHAR), (UINT32)ker_ver, VENDOR_AI_PLUGIN_SUBVER, 0, 0);
			if (rv != HD_OK) {
				DBG_ERR("cpu_prebuild cannot get version?\r\n");
				return FALSE;
			}
			cpu_prebuild_main_ver = strtoul(ker_ver, NULL, 16);
			cpu_prebuild_sub_ver  = strtoul(&ker_ver[3], NULL, 16);
			if ((cpu_prebuild_main_ver != (CPU_PREBUILD_CHK_VERINT >> 16)) || (cpu_prebuild_sub_ver < (CPU_PREBUILD_CHK_VERINT & 0xFFFF))) {
				DBG_ERR("cpu_prebuild version check fail! cpu_prebuild version(%#x), check_ver(%#x)\r\n", (int)((cpu_prebuild_main_ver << 16) | cpu_prebuild_sub_ver), (int)CPU_PREBUILD_CHK_VER);
				return FALSE;
			}
		}
	}

	return TRUE;
}



#if CNN_MULTI_INPUT
UINT32 vendor_ais_get_net_is_batch(UINT32 net_id)
{
	if (net_id >= NN_SUPPORT_NET_MAX) {
		DBG_ERR("invalid net_id\r\n");
		return HD_ERR_ABORT;
	}

	return (UINT32)g_ai_net_is_batch[net_id];
}
#endif

static BOOL vendor_ais_is_null_mem(VENDOR_AIS_FLOW_MEM_PARM mem)
{
	if ((mem.va == 0) || (mem.pa == 0) || (mem.size == 0)) {
		return TRUE;
	}
	return FALSE;
}

UINT32 vendor_ais_user_mem_va2pa(UINT32 addr, VENDOR_AIS_FLOW_MEM_PARM mem_range)
{
	if (addr < mem_range.va) {
		DBG_ERR("Invalid va 0x%08x, mem user begin = 0x%08x, end =0x%08x\r\n"
			   , (int)addr, (int)mem_range.va, (int)(mem_range.va + mem_range.size));
		return addr;
	}
	return addr - mem_range.va + mem_range.pa;
}

UINT32 vendor_ais_user_mem_pa2va(UINT32 addr, VENDOR_AIS_FLOW_MEM_PARM mem_range)
{
	if (addr < mem_range.pa) {
		DBG_ERR("Invalid pa 0x%08x, mem user begin = 0x%08x, end =0x%08x\r\n"
			   , (int)addr, (int)mem_range.pa, (int)(mem_range.pa + mem_range.size));
		return addr;
	}
	return addr - mem_range.pa + mem_range.va;
}

UINT32 vendor_ais_user_parm_va2pa(UINT32 addr, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
	return vendor_ais_user_mem_va2pa(addr, p_proc->map_mem.user_parm);
#else
	return vendor_ais_user_mem_va2pa(addr, g_map_mem[net_id].user_parm);
#endif
}

UINT32 vendor_ais_user_parm_pa2va(UINT32 addr, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
	return vendor_ais_user_mem_pa2va(addr, p_proc->map_mem.user_parm);
#else
	return vendor_ais_user_mem_pa2va(addr, g_map_mem[net_id].user_parm);
#endif
}

UINT32 vendor_ais_user_model_va2pa(UINT32 addr, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
	return vendor_ais_user_mem_va2pa(addr, p_proc->map_mem.user_model);
#else
	return vendor_ais_user_mem_va2pa(addr, g_map_mem[net_id].user_model);
#endif
}

UINT32 vendor_ais_user_model_pa2va(UINT32 addr, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
	return vendor_ais_user_mem_pa2va(addr, p_proc->map_mem.user_model);
#else
	return vendor_ais_user_mem_pa2va(addr, g_map_mem[net_id].user_model);
#endif
}

UINT32 vendor_ais_user_buff_va2pa(UINT32 addr, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
	return vendor_ais_user_mem_va2pa(addr, p_proc->map_mem.user_buff);
#else
	return vendor_ais_user_mem_va2pa(addr, g_map_mem[net_id].user_buff);
#endif
}

UINT32 vendor_ais_user_buff_pa2va(UINT32 addr, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
	return vendor_ais_user_mem_pa2va(addr, p_proc->map_mem.user_buff);
#else
	return vendor_ais_user_mem_pa2va(addr, g_map_mem[net_id].user_buff);
#endif
}

UINT32 vendor_ais_kerl_parm_va2pa(UINT32 addr, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
	return vendor_ais_user_mem_va2pa(addr, p_proc->map_mem.kerl_parm);
#else
	return vendor_ais_user_mem_va2pa(addr, g_map_mem[net_id].kerl_parm);
#endif
}

UINT32 vendor_ais_kerl_parm_pa2va(UINT32 addr, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
	return vendor_ais_user_mem_pa2va(addr, p_proc->map_mem.kerl_parm);
#else
	return vendor_ais_user_mem_pa2va(addr, g_map_mem[net_id].kerl_parm);
#endif
}

ER vendor_ais_cal_size(UINT32 s_addr, UINT32 *p_parm_sz, UINT32 *p_model_sz, UINT32 *p_buf_sz)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
#if CNN_25_MATLAB
	UINT32 net_layer_num;
#else
	UINT32 external_size;	
#endif
	UINT32 model_head_size, mctrl_size, io_mem_addr_size, model_size, parm_size, io_buff_size;
#if CNN_528_PSW
	UINT32 id_list_size;
#endif
	UINT32 bufinfo_size=0;

	NN_GEN_NET_INFO net_info = {0};
	ER er;
	er = vendor_ais_get_net_info(&net_info, s_addr);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}

	p_head              = net_info.p_head;
	proc_layer_num      = p_head->mode_ctrl_num;
	model_head_size     = ALIGN_CEIL_4(sizeof(NN_GEN_MODEL_HEAD));
	mctrl_size          = ALIGN_CEIL_4(sizeof(NN_GEN_MODE_CTRL) * proc_layer_num);
#if CNN_25_MATLAB
	net_layer_num       = p_head->layer_num;
	io_mem_addr_size    = ALIGN_CEIL_4(sizeof(NN_IOMEM) * net_layer_num);
#else
	io_mem_addr_size    = ALIGN_CEIL_4(p_head->iomem_size);
#endif
	model_size          = ALIGN_CEIL_4(p_head->model_size);
	parm_size           = ALIGN_CEIL_4(p_head->parm_size);
	io_buff_size        = ALIGN_CEIL_4(p_head->io_buff_size);
	if (s_addr == 0) {
		DBG_ERR("memory is not initial: 0x%08x\r\n", (int)s_addr);
		return E_CTX;
	}
#if CNN_528_PSW
	id_list_size = p_head->layer_id_list_size;
#if CNN_25_MATLAB
	*p_parm_sz  = model_head_size + mctrl_size + id_list_size + io_mem_addr_size + parm_size;
	//printf(" p_parm_sz = %lu = sum(%lu, %lu, %lu, %lu, %lu)\n", *p_parm_sz, model_head_size , mctrl_size , id_list_size , io_mem_addr_size , parm_size);
#else
	external_size = p_head->external_size;
#if CNN_FMT_V4
	bufinfo_size = p_head->bufinfo_size; // default 0, if V4 will update value
#endif
	*p_parm_sz  = model_head_size + mctrl_size + id_list_size + external_size + bufinfo_size + io_mem_addr_size + parm_size;
	vendor_ai_net_trace(/*proc_id*/0, AI_BUF, "p_parm_sz = %lu = sum(%lu, %lu, %lu, %lu, %lu, %lu, %lu)\n", *p_parm_sz, model_head_size , mctrl_size , id_list_size , external_size , bufinfo_size, io_mem_addr_size , parm_size);
#endif
#else
	*p_parm_sz  = model_head_size + mctrl_size + io_mem_addr_size + parm_size;
#endif
	*p_model_sz = model_size;
	*p_buf_sz   = io_buff_size;

	return E_OK;
}

UINT32 vendor_ais_auto_alloc_mem(VENDOR_AIS_FLOW_MEM_PARM *p_mem, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager)
{
	UINT32 require_sz = 0, parm_sz = 0, model_sz = 0, buf_sz = 0;
	if ((p_mem_manager == NULL) || (p_mem == NULL)
		|| (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return 0;
	}

	vendor_ais_cal_size(p_mem->va, &parm_sz, &model_sz, &buf_sz);
	p_mem_manager->user_parm.pa		= ALIGN_CEIL_16(p_mem->pa);
	p_mem_manager->user_parm.va		= ALIGN_CEIL_16(p_mem->va);
	p_mem_manager->user_parm.size	= parm_sz;

	p_mem_manager->user_model.pa	= ALIGN_CEIL_16(p_mem_manager->user_parm.pa + parm_sz);
	p_mem_manager->user_model.va	= ALIGN_CEIL_16(p_mem_manager->user_parm.va + parm_sz);
	p_mem_manager->user_model.size	= model_sz;

	p_mem_manager->user_buff.pa		= ALIGN_CEIL_16(p_mem_manager->user_model.pa + model_sz);
	p_mem_manager->user_buff.va		= ALIGN_CEIL_16(p_mem_manager->user_model.va + model_sz);
	p_mem_manager->user_buff.size	= buf_sz;

	p_mem_manager->kerl_parm.pa		= p_mem_manager->user_buff.pa + buf_sz;
	p_mem_manager->kerl_parm.va		= p_mem_manager->user_buff.va + buf_sz;
	p_mem_manager->kerl_parm.size	= parm_sz;;

	require_sz = p_mem_manager->kerl_parm.size + p_mem_manager->kerl_parm.va - p_mem->va;

	return require_sz;
}

#if CNN_25_MATLAB
UINT32 vendor_ais_get_max_buf_sz_layer(VENDOR_AIS_FLOW_MEM_PARM *p_mem)
{
	NN_GEN_MODEL_HEAD *p_head;
	NN_IOMEM *p_io_mem;
	UINT32 idx = 0, idx2 = 0;
	UINT32 max_buf_sz = 0;
	NN_GEN_NET_INFO net_info = {0};
	ER er = E_OK;

	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return 0;
	}

	er = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_io_mem = net_info.p_io_mem;

	for (idx = 0; idx < p_head->layer_num; idx++) {
		for (idx2 = 0; idx2 < NN_IMEM_NUM; idx2++) {
			if (max_buf_sz < p_io_mem[idx].SAI[idx2].size) {
				max_buf_sz = p_io_mem[idx].SAI[idx2].size;
			}
		}
		for (idx2 = 0; idx2 < NN_OMEM_NUM; idx2++) {
			if (max_buf_sz < p_io_mem[idx].SAO[idx2].size) {
				max_buf_sz = p_io_mem[idx].SAO[idx2].size;
			}
		}
	}

	return max_buf_sz;
}
#else
UINT32 vendor_ais_get_max_buf_sz_layer(VENDOR_AIS_FLOW_MEM_PARM *p_mem)
{
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 idx = 0, idx2 = 0;
	UINT32 max_buf_sz = 0;
	NN_GEN_NET_INFO net_info = {0};
	HD_RESULT ret = HD_OK;

	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return 0;
	}

	ret = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return ret;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	for (idx = 0; idx < p_head->mode_ctrl_num; idx++) {
		NN_DATA *p_sai, *p_sao;
		UINT32 imem_cnt, omem_cnt;

		p_sai = (NN_DATA*)p_mctrl[idx].iomem.imem_addr;
		p_sao = (NN_DATA*)p_mctrl[idx].iomem.omem_addr;
		imem_cnt = p_mctrl[idx].iomem.imem_cnt;
		omem_cnt = p_mctrl[idx].iomem.omem_cnt;

		for (idx2 = 0; idx2 < imem_cnt; idx2++) {
			if (max_buf_sz < p_sai[idx2].size) {
				max_buf_sz = p_sai[idx2].size;
			}
		}
		for (idx2 = 0; idx2 < omem_cnt; idx2++) {
			if (max_buf_sz < p_sao[idx2].size) {
				max_buf_sz = p_sao[idx2].size;
			}
		}
	}

	return max_buf_sz;
}
#endif
UINT32 vendor_ais_get_nextll_addr_cmd(UINT64 cmd)
{
	return (UINT32)((cmd >> 8) & (UINT64)0xffffffff);
}

/*
    table index :   [0x0, 0xff]         bits : 7_0
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 vendor_ais_set_nextll_addr_cmd(UINT64 cmd, UINT32 addr)
{
	return ((cmd & (UINT64)0xffffff00000000ff) + ((UINT64)addr << (UINT64)8));
}

/*
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 vendor_ais_ll_nextupd_cmd(UINT32 addr)
{
	return (UINT64)(((UINT64)4 << 60) + ((UINT64)addr << 8));
}

BOOL vendor_ais_ll_is_nextupd_cmd(UINT64 cmd)
{
	return ((cmd >> 60) == 4) ? TRUE : FALSE;
}

#if CNN_25_MATLAB
static UINT32 net_map_addr_with_parsflag(UINT32 addr, UINT32 buf_ofs, UINT32 model_ofs)
{
	const UINT32 addr_mask = 0x0fffffff;
	if ((addr & NN_GEN_MODEL_ADDR_TYPE) == NN_GEN_MODEL_ADDR_TYPE) {
		addr = (addr & addr_mask) + model_ofs;
	} else if ((addr & NN_GEN_BUF_ADDR_TYPE) == NN_GEN_BUF_ADDR_TYPE) {
		addr = (addr & addr_mask) + buf_ofs;
	} else if (addr == NN_GEN_NULL_ADDR_TYPE) {
		//null address
	} else if ((addr & 0xf0000000) == 0) {
		//pass first layer input address
	} else {
		DBG_WRN("%s: not support flag: 0x%08x\r\n", __FUNCTION__, (int)addr);
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
		DBG_WRN("%s: not support flag: 0x%08x\r\n", __FUNCTION__, (int)addr);
	}
	return addr;
}

static UINT32 net_unmap_addr(UINT32 addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end)
{
	if ((addr >= model_ofs) && (addr < model_end)) {
		addr = (addr - model_ofs) | NN_GEN_MODEL_ADDR_TYPE;
	} else if ((addr >= buf_ofs) && (addr < buf_end)) {
		addr = (addr - buf_ofs) | NN_GEN_BUF_ADDR_TYPE;
	} else if (addr == NN_GEN_NULL_ADDR_TYPE) {
		//null address
	} else {
		DBG_WRN("%s: invalid address: 0x%08x\r\n", __FUNCTION__, (int)addr);
	}
	return addr;
}
#endif

#if CNN_25_MATLAB
VOID vendor_ais_map_ai_app_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id)
{
	VENDOR_AI_APP_HEAD *p_head = (VENDOR_AI_APP_HEAD *)parm_addr;
	p_head->parm_addr += parm_ofs;

	if (p_head->stripe_head_addr != 0) {
		//multi-stripe engine
		p_head->stripe_head_addr += parm_ofs;
	}

	switch (p_head->mode) {
	case AI_MODE_NEURAL: {
			VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)p_head->parm_addr;
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
			VENDOR_AI_ROIPOOL_PARM *p_parm = (VENDOR_AI_ROIPOOL_PARM *)p_head->parm_addr;
			p_parm->in_addr             = net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->out_addr            = net_map_addr_with_parsflag(p_parm->out_addr, buf_ofs, model_ofs);
			p_parm->roi_ker.roi_addr    = net_map_addr_with_parsflag(p_parm->roi_ker.roi_addr, buf_ofs, model_ofs);
		}
		break;
	case AI_MODE_SVM: {
			VENDOR_AI_SVM_PARM *p_parm = (VENDOR_AI_SVM_PARM *)p_head->parm_addr;
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
			VENDOR_AI_FC_PARM *p_parm = (VENDOR_AI_FC_PARM *)p_head->parm_addr;
			p_parm->in_addr             = net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->out_addr            = net_map_addr_with_parsflag(p_parm->out_addr, buf_ofs, model_ofs);
			p_parm->in_interm_addr      = net_map_addr_with_parsflag(p_parm->in_interm_addr, buf_ofs, model_ofs);
			p_parm->out_interm_addr     = net_map_addr_with_parsflag(p_parm->out_interm_addr, buf_ofs, model_ofs);
			p_parm->fc_ker.weight_addr  = net_map_addr_with_parsflag(p_parm->fc_ker.weight_addr, buf_ofs, model_ofs);
			p_parm->fc_ker.bias_addr    = net_map_addr_with_parsflag(p_parm->fc_ker.bias_addr, buf_ofs, model_ofs);
			p_parm->fcd.quanti_kmeans_addr = net_map_addr_with_parsflag(p_parm->fcd.quanti_kmeans_addr, buf_ofs, model_ofs);
			p_parm->fcd.p_vlc_code     = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_code + parm_ofs);
			p_parm->fcd.p_vlc_valid    = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_valid + parm_ofs);
			p_parm->fcd.p_vlc_ofs      = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_ofs + parm_ofs);
		}
		break;
	case AI_MODE_PERMUTE: {
			VENDOR_AI_PERMUTE_PARM *p_parm = (VENDOR_AI_PERMUTE_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_map_addr_with_parsflag(p_parm->in_addr, buf_ofs, model_ofs);
			p_parm->out_addr    = net_map_addr_with_parsflag(p_parm->out_addr, buf_ofs, model_ofs);
		}
		break;
	case AI_MODE_REORG: {
			VENDOR_AI_REORG_PARM *p_parm = (VENDOR_AI_REORG_PARM *)p_head->parm_addr;
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

VOID vendor_ais_map_ai_ll_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id)
{
	VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)parm_addr;
	UINT64 *p_ll_end_cmd;
	UINT32 map_addr = 0, next_ll_addr = 0;
#if LL_SUPPORT_FIRST_LAYER
//========== for first layer linked list mode ==========
	CNN_LL_PARM *p_cnn_ll_parm;
	NUE_LL_PARM *p_nue_ll_parm;
	NUE2_LL_PARM *p_nue2_ll_parm;
//========== by CCC 191004 ==========
#else
	UINT64 *p_ll_cmd;
	const UINT64 addr_mask 	= (UINT64)0x00000000ffffffff;
	const UINT64 cmd_mask  	= (UINT64)0xffffffff00000000;
#endif


	p_head->parm_addr += parm_ofs;
#if (LL_SUPPORT_FIRST_LAYER == 0)
	p_ll_cmd = (UINT64 *)p_head->parm_addr;
#endif
	p_ll_end_cmd = (UINT64 *)(p_head->parm_addr + p_head->parm_size - sizeof(UINT64));
	next_ll_addr = vendor_ais_get_nextll_addr_cmd(*p_ll_end_cmd);
	if (next_ll_addr != 0) {
		next_ll_addr += parm_ofs;
		*p_ll_end_cmd = vendor_ais_set_nextll_addr_cmd(*p_ll_end_cmd, vendor_ais_user_parm_va2pa(next_ll_addr, net_id));
	}
#if LL_SUPPORT_FIRST_LAYER
//========== for first layer linked list mode ==========
	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(CNN_LL_PARM));
		p_cnn_ll_parm = (CNN_LL_PARM*)p_head->parm_addr;
		map_addr = p_cnn_ll_parm->input.bit.addr;
		p_cnn_ll_parm->input.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		//map_addr = p_cnn_ll_parm->input[1].bit.addr;
		//p_cnn_ll_parm->input[1].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		//map_addr = p_cnn_ll_parm->input[2].bit.addr;
		//p_cnn_ll_parm->input[2].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_cnn_ll_parm->interm_in.bit.addr;
		p_cnn_ll_parm->interm_in.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_cnn_ll_parm->output[0].bit.addr;
		p_cnn_ll_parm->output[0].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_cnn_ll_parm->output[1].bit.addr;
		p_cnn_ll_parm->output[1].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);

		map_addr = p_cnn_ll_parm->weight.bit.addr;
		p_cnn_ll_parm->weight.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_cnn_ll_parm->kmean.bit.addr;
		p_cnn_ll_parm->kmean.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_cnn_ll_parm->bias_bnscal.bit.addr;
		p_cnn_ll_parm->bias_bnscal.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		break;
	case AI_ENG_NUE:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE_LL_PARM));
		p_nue_ll_parm = (NUE_LL_PARM*)p_head->parm_addr;
		map_addr = p_nue_ll_parm->input.bit.addr;
		p_nue_ll_parm->input.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue_ll_parm->elt_in.bit.addr;
		p_nue_ll_parm->elt_in.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue_ll_parm->roi_in.bit.addr;
		p_nue_ll_parm->roi_in.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue_ll_parm->output.bit.addr;
		p_nue_ll_parm->output.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);

		map_addr = p_nue_ll_parm->sv.bit.addr;
		p_nue_ll_parm->sv.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue_ll_parm->alpha.bit.addr;
		p_nue_ll_parm->alpha.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue_ll_parm->rho.bit.addr;
		p_nue_ll_parm->rho.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue_ll_parm->kmean.bit.addr;
		p_nue_ll_parm->kmean.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		break;
	case AI_ENG_NUE2:
		p_nue2_ll_parm = (NUE2_LL_PARM*)p_head->parm_addr;
		map_addr = p_nue2_ll_parm->input[0].bit.addr;
		p_nue2_ll_parm->input[0].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue2_ll_parm->input[1].bit.addr;
		p_nue2_ll_parm->input[1].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue2_ll_parm->input[2].bit.addr;
		p_nue2_ll_parm->input[2].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue2_ll_parm->output[0].bit.addr;
		p_nue2_ll_parm->output[0].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue2_ll_parm->output[1].bit.addr;
		p_nue2_ll_parm->output[1].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		map_addr = p_nue2_ll_parm->output[2].bit.addr;
		p_nue2_ll_parm->output[2].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
		break;
	default:
		DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}
//========== by CCC 191004 ==========
#else
	while (1) {
		if ((*p_ll_cmd) == vendor_ais_ll_nextupd_cmd(NN_GEN_NO_ADDR_UPD_TYPE)) {
			*p_ll_cmd = vendor_ais_ll_nextupd_cmd(vendor_ais_user_parm_va2pa((UINT32)(p_ll_cmd + 1), net_id));
			break;
		} else if (((*p_ll_cmd) & (UINT64)NN_GEN_MODEL_ADDR_TYPE) || ((*p_ll_cmd) & (UINT64)NN_GEN_BUF_ADDR_TYPE)) {
			const UINT32 addr_type = ((*p_ll_cmd) & (UINT64)0xf0000000);
			map_addr = (UINT32)((*p_ll_cmd) & (UINT64)addr_mask);
			*p_ll_cmd = ((*p_ll_cmd) & (UINT64)cmd_mask);
			map_addr = net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
			if (addr_type == NN_GEN_MODEL_ADDR_TYPE) {
				map_addr = vendor_ais_user_model_va2pa(map_addr, net_id);
			} else if (addr_type == NN_GEN_BUF_ADDR_TYPE) {
				map_addr = vendor_ais_user_buff_va2pa(map_addr, net_id);
			} else {
				DBG_WRN("unknown address type in ll va2pa: 0x%08x\r\n", (unsigned int)addr_type);
			}
			*p_ll_cmd = (*p_ll_cmd) + map_addr;
		} else if (((*p_ll_cmd) & 0xffffffff) == NN_GEN_NULL_ADDR_TYPE) {
			//null address
		} else {
			DBG_WRN("unknown address type: 0x%08x\r\n", (unsigned int)((*p_ll_cmd) & (UINT64)0xffffffff));
			break;
		}
		p_ll_cmd++;
	}
#endif
}
#else
VOID vendor_ais_map_ai_app_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id)
{
	VENDOR_AI_APP_HEAD *p_head = (VENDOR_AI_APP_HEAD *)parm_addr;
	p_head->parm_addr += parm_ofs;

	if (p_head->stripe_head_addr != 0) {
		//multi-stripe engine
		p_head->stripe_head_addr += parm_ofs;
	}

	switch (p_head->mode) {
	case AI_MODE_NEURAL: {
			VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)p_head->parm_addr;
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
			VENDOR_AI_ROIPOOL_PARM *p_parm = (VENDOR_AI_ROIPOOL_PARM *)p_head->parm_addr;
			p_parm->in_addr             = net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->out_addr            = net_map_addr_with_parsflag(p_parm->out_addr, model_ofs, buf_ofs);
			p_parm->roi_ker.roi_addr    = net_map_addr_with_parsflag(p_parm->roi_ker.roi_addr, model_ofs, buf_ofs);
		}
		break;
	case AI_MODE_SVM: {
			VENDOR_AI_SVM_PARM *p_parm = (VENDOR_AI_SVM_PARM *)p_head->parm_addr;
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
			VENDOR_AI_FC_PARM *p_parm = (VENDOR_AI_FC_PARM *)p_head->parm_addr;
			p_parm->in_addr             = net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->out_addr            = net_map_addr_with_parsflag(p_parm->out_addr, model_ofs, buf_ofs);
			p_parm->in_interm_addr      = net_map_addr_with_parsflag(p_parm->in_interm_addr, model_ofs, buf_ofs);
			p_parm->out_interm_addr     = net_map_addr_with_parsflag(p_parm->out_interm_addr, model_ofs, buf_ofs);
			p_parm->fc_ker.weight_addr  = net_map_addr_with_parsflag(p_parm->fc_ker.weight_addr, model_ofs, buf_ofs);
			p_parm->fc_ker.bias_addr    = net_map_addr_with_parsflag(p_parm->fc_ker.bias_addr, model_ofs, buf_ofs);
			p_parm->fcd.quanti_kmeans_addr = net_map_addr_with_parsflag(p_parm->fcd.quanti_kmeans_addr, model_ofs, buf_ofs);
			p_parm->fcd.p_vlc_code     = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_code + parm_ofs);
			p_parm->fcd.p_vlc_valid    = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_valid + parm_ofs);
			p_parm->fcd.p_vlc_ofs      = (UINT32 *)((UINT32)p_parm->fcd.p_vlc_ofs + parm_ofs);
		}
		break;
	case AI_MODE_PERMUTE: {
			VENDOR_AI_PERMUTE_PARM *p_parm = (VENDOR_AI_PERMUTE_PARM *)p_head->parm_addr;
			p_parm->in_addr     = net_map_addr_with_parsflag(p_parm->in_addr, model_ofs, buf_ofs);
			p_parm->out_addr    = net_map_addr_with_parsflag(p_parm->out_addr, model_ofs, buf_ofs);
		}
		break;
	case AI_MODE_REORG: {
			VENDOR_AI_REORG_PARM *p_parm = (VENDOR_AI_REORG_PARM *)p_head->parm_addr;
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

VOID vendor_ais_map_ai_ll_addr(UINT32 parm_addr, UINT32 parm_va_ofs, UINT32 parm_pa_ofs, UINT32 model_pa_ofs, UINT32 buf_pa_ofs, UINT32 net_id)
{

	VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)parm_addr;
	UINT64 *p_ll_end_cmd;
	UINT32 map_addr = 0, next_ll_addr = 0;
#if LL_SUPPORT_FIRST_LAYER
//========== for first layer linked list mode ==========
	CNN_LL_PARM *p_cnn_ll_parm;
	NUE_LL_PARM *p_nue_ll_parm;
	NUE2_LL_PARM *p_nue2_ll_parm;
//========== by CCC 191004 ==========
#else
	UINT64 *p_ll_cmd;
	const UINT64 addr_mask 	= (UINT64)0x00000000ffffffff;
	const UINT64 cmd_mask  	= (UINT64)0xffffffff00000000;
#endif
	p_head->parm_addr += parm_va_ofs;
#if (LL_SUPPORT_FIRST_LAYER == 0)
	p_ll_cmd = (UINT64 *)p_head->parm_addr;
#endif
	p_ll_end_cmd = (UINT64 *)(p_head->parm_addr + p_head->parm_size - sizeof(UINT64));
	next_ll_addr = vendor_ais_get_nextll_addr_cmd(*p_ll_end_cmd);
	if (next_ll_addr != 0) {
		next_ll_addr += parm_pa_ofs;
		*p_ll_end_cmd = vendor_ais_set_nextll_addr_cmd(*p_ll_end_cmd, next_ll_addr);
	}
#if LL_SUPPORT_FIRST_LAYER
//========== for first layer linked list mode ==========
	switch (p_head->eng) {
#if defined(_BSP_NA51090_)
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(CNN_LL_PARM));
		p_cnn_ll_parm = (CNN_LL_PARM*)p_head->parm_addr;
		map_addr = p_cnn_ll_parm->input.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_cnn_ll_parm->input.bit.addr = (total_addr & 0x00000000ffffffff);
		p_cnn_ll_parm->input_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_cnn_ll_parm->interm_in.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_cnn_ll_parm->interm_in.bit.addr = (total_addr & 0x00000000ffffffff);
		p_cnn_ll_parm->interm_in_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_cnn_ll_parm->output[0].bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_cnn_ll_parm->output[0].bit.addr = (total_addr & 0x00000000ffffffff);
		p_cnn_ll_parm->output_MSB[0].bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_cnn_ll_parm->output[1].bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_cnn_ll_parm->output[1].bit.addr = (total_addr & 0x00000000ffffffff);
		p_cnn_ll_parm->output_MSB[1].bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_cnn_ll_parm->weight.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_cnn_ll_parm->weight.bit.addr = (total_addr & 0x00000000ffffffff);
		p_cnn_ll_parm->weight_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_cnn_ll_parm->kmean.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_cnn_ll_parm->kmean.bit.addr = (total_addr & 0x00000000ffffffff);
		p_cnn_ll_parm->kmean_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_cnn_ll_parm->bias_bnscal.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_cnn_ll_parm->bias_bnscal.bit.addr = (total_addr & 0x00000000ffffffff);
		p_cnn_ll_parm->bias_bnscal_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		break;
	case AI_ENG_NUE:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE_LL_PARM));
		p_nue_ll_parm = (NUE_LL_PARM*)p_head->parm_addr;
		map_addr = p_nue_ll_parm->input.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue_ll_parm->input.bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue_ll_parm->input_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue_ll_parm->elt_in.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue_ll_parm->elt_in.bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue_ll_parm->elt_in_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue_ll_parm->roi_in.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue_ll_parm->roi_in.bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue_ll_parm->roi_in_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue_ll_parm->output.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue_ll_parm->output.bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue_ll_parm->output_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue_ll_parm->sv.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue_ll_parm->sv.bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue_ll_parm->sv_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue_ll_parm->alpha.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue_ll_parm->alpha.bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue_ll_parm->alpha_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue_ll_parm->rho.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue_ll_parm->rho.bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue_ll_parm->rho_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue_ll_parm->kmean.bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue_ll_parm->kmean.bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue_ll_parm->kmean_MSB.bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		break;
	case AI_ENG_NUE2:
		p_nue2_ll_parm = (NUE2_LL_PARM*)p_head->parm_addr;
		map_addr = p_nue2_ll_parm->input[0].bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue2_ll_parm->input[0].bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue2_ll_parm->input_MSB[0].bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue2_ll_parm->input[1].bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue2_ll_parm->input[1].bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue2_ll_parm->input_MSB[1].bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue2_ll_parm->input[2].bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue2_ll_parm->input[2].bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue2_ll_parm->input_MSB[2].bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue2_ll_parm->output[0].bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue2_ll_parm->output[0].bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue2_ll_parm->output_MSB[0].bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue2_ll_parm->output[1].bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue2_ll_parm->output[1].bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue2_ll_parm->output_MSB[1].bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		map_addr = p_nue2_ll_parm->output[2].bit.addr;
		total_addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		p_nue2_ll_parm->output[2].bit.addr = (total_addr & 0x00000000ffffffff);
		p_nue2_ll_parm->output_MSB[2].bit.addr = ((total_addr & 0xffffffff00000000)>>32);
		break;
#else
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(CNN_LL_PARM));
		p_cnn_ll_parm = (CNN_LL_PARM*)p_head->parm_addr;
		map_addr = p_cnn_ll_parm->input.bit.addr;
		p_cnn_ll_parm->input.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		//map_addr = p_cnn_ll_parm->input[1].bit.addr;
		//p_cnn_ll_parm->input[1].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		//map_addr = p_cnn_ll_parm->input[2].bit.addr;
		//p_cnn_ll_parm->input[2].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_cnn_ll_parm->interm_in.bit.addr;
		p_cnn_ll_parm->interm_in.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_cnn_ll_parm->output[0].bit.addr;
		p_cnn_ll_parm->output[0].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_cnn_ll_parm->output[1].bit.addr;
		p_cnn_ll_parm->output[1].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);

		map_addr = p_cnn_ll_parm->weight.bit.addr;
		p_cnn_ll_parm->weight.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_cnn_ll_parm->kmean.bit.addr;
		p_cnn_ll_parm->kmean.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_cnn_ll_parm->bias_bnscal.bit.addr;
		p_cnn_ll_parm->bias_bnscal.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		break;
	case AI_ENG_NUE:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE_LL_PARM));
		p_nue_ll_parm = (NUE_LL_PARM*)p_head->parm_addr;
		map_addr = p_nue_ll_parm->input.bit.addr;
		p_nue_ll_parm->input.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->elt_in.bit.addr;
		p_nue_ll_parm->elt_in.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->roi_in.bit.addr;
		p_nue_ll_parm->roi_in.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->output.bit.addr;
		p_nue_ll_parm->output.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);

		map_addr = p_nue_ll_parm->sv.bit.addr;
		p_nue_ll_parm->sv.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->alpha.bit.addr;
		p_nue_ll_parm->alpha.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->rho.bit.addr;
		p_nue_ll_parm->rho.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue_ll_parm->kmean.bit.addr;
		p_nue_ll_parm->kmean.bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		break;
	case AI_ENG_NUE2:
		p_nue2_ll_parm = (NUE2_LL_PARM*)p_head->parm_addr;
		map_addr = p_nue2_ll_parm->input[0].bit.addr;
		p_nue2_ll_parm->input[0].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue2_ll_parm->input[1].bit.addr;
		p_nue2_ll_parm->input[1].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue2_ll_parm->input[2].bit.addr;
		p_nue2_ll_parm->input[2].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue2_ll_parm->output[0].bit.addr;
		p_nue2_ll_parm->output[0].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue2_ll_parm->output[1].bit.addr;
		p_nue2_ll_parm->output[1].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		map_addr = p_nue2_ll_parm->output[2].bit.addr;
		p_nue2_ll_parm->output[2].bit.addr = (map_addr == 0) ? 0 : net_map_addr_with_parsflag(map_addr, model_pa_ofs, buf_pa_ofs);
		break;
#endif
	default:
		DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}
//========== by CCC 191004 ==========
#else
	while (1) {
		if ((*p_ll_cmd) == vendor_ais_ll_nextupd_cmd(NN_GEN_NO_ADDR_UPD_TYPE)) {
			*p_ll_cmd = vendor_ais_ll_nextupd_cmd(vendor_ais_user_parm_va2pa((UINT32)(p_ll_cmd + 1), net_id));
			break;
		} else if (((*p_ll_cmd) & (UINT64)NN_GEN_MODEL_ADDR_TYPE) || ((*p_ll_cmd) & (UINT64)NN_GEN_BUF_ADDR_TYPE)) {
			const UINT32 addr_type = ((*p_ll_cmd) & (UINT64)0xf0000000);
			map_addr = (UINT32)((*p_ll_cmd) & (UINT64)addr_mask);
			*p_ll_cmd = ((*p_ll_cmd) & (UINT64)cmd_mask);
			map_addr = net_map_addr_with_parsflag(map_addr, buf_ofs, model_ofs);
			if (addr_type == NN_GEN_MODEL_ADDR_TYPE) {
				map_addr = vendor_ais_user_model_va2pa(map_addr, net_id);
			} else if (addr_type == NN_GEN_BUF_ADDR_TYPE) {
				map_addr = vendor_ais_user_buff_va2pa(map_addr, net_id);
			} else {
				DBG_WRN("unknown address type in ll va2pa: 0x%08x\r\n", (unsigned int)addr_type);
			}
			*p_ll_cmd = (*p_ll_cmd) + map_addr;
		} else if (((*p_ll_cmd) & 0xffffffff) == NN_GEN_NULL_ADDR_TYPE) {
			//null address
		} else {
			DBG_WRN("unknown address type: 0x%08x\r\n", (unsigned int)((*p_ll_cmd) & (UINT64)0xffffffff));
			break;
		}
		p_ll_cmd++;
	}
#endif
}
#endif

VOID vendor_ais_map_drv_addr(NN_MODE mode, UINT32 parm_addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 net_id)
{
	NN_CPU_PARM *p_cpu_parm;
	NN_PRE_PARM *p_pre_parm;
	NN_FC_POST_PARM *p_fc_post_parm;
	NN_POOL_PARM *p_pool_parm;
	NN_CUSTOM_PARM *p_cust_head;
	NN_BNSCALE_PARM *p_bn_scl_parm;
	NN_SOFTMAX_PARM *p_softmax_parm;
	NN_POSTPROC_PARM *p_postproc_parm;
#if USE_NEON
	NN_PRELU_PARM *p_prelu_parm;
	NN_PRIORBOX_PARM *p_priorbox_parm;
	NN_DETOUT_PARM *p_detout_parm;
	INT32 i;
#endif
    NN_PERMUTE_PARM *p_permute_parm;
	NN_REVERSE_PARM *p_reverse_parm;
	NN_NORM_PARM *p_norm_parm;
	NN_LAYER_NORMALIZATION_PARM *p_layer_norm_parm;
#if NN_DLI
	NN_DLI_ElEMENTWISE_PARM *p_dli_elementwise_parm;
	NN_DLI_RESIZE_PARM *p_dli_resize_parm;
	NN_DLI_SOFTMAX_PARM *p_dli_softmax_parm;
#endif

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
		break;
#if CNN_25_MATLAB
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
	case NN_SOFTMAX:
		p_softmax_parm = (NN_SOFTMAX_PARM *)parm_addr;
		p_softmax_parm->in_addr         = net_map_addr_with_parsflag(p_softmax_parm->in_addr, buf_ofs, model_ofs);
		p_softmax_parm->out_addr        = net_map_addr_with_parsflag(p_softmax_parm->out_addr, buf_ofs, model_ofs);
#if USE_NEON
		p_softmax_parm->in_trans_addr   = net_map_addr_with_parsflag(p_softmax_parm->in_trans_addr, buf_ofs, model_ofs);
		p_softmax_parm->out_trans_addr  = net_map_addr_with_parsflag(p_softmax_parm->out_trans_addr, buf_ofs, model_ofs);
#endif
		break;
	case NN_PREPROC:
		p_pre_parm = (NN_PRE_PARM*)parm_addr;
		p_pre_parm->in_addr             = net_map_addr_with_parsflag(p_pre_parm->in_addr, buf_ofs, model_ofs);
		p_pre_parm->out_addr            = net_map_addr_with_parsflag(p_pre_parm->out_addr, buf_ofs, model_ofs);
		p_pre_parm->interm_addr			= net_map_addr_with_parsflag(p_pre_parm->interm_addr, buf_ofs, model_ofs);
		p_pre_parm->sub_img.plane_addr  = net_map_addr_with_parsflag(p_pre_parm->sub_img.plane_addr, buf_ofs, model_ofs);
		if (((p_pre_parm->in_addr & 0xf) != 0) || ((p_pre_parm->out_addr & 0xf) != 0)
			|| ((p_pre_parm->sub_img.plane_addr & 0xf) != 0)) {
			DBG_WRN("preproc address should be 16 alignment: in = 0x%08x, out = 0x%08x, mean = 0x%08x\r\n"
					, (int)p_pre_parm->in_addr, (int)p_pre_parm->out_addr, (int)p_pre_parm->sub_img.plane_addr);
		}
		break;
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
		p_cust_head->input.pa   = vendor_ais_user_buff_va2pa(p_cust_head->input.va, net_id);
		p_cust_head->output.pa  = vendor_ais_user_buff_va2pa(p_cust_head->output.va, net_id);
		if (p_cust_head->model.va > 0)
			p_cust_head->model.pa   = vendor_ais_user_model_va2pa(p_cust_head->model.va, net_id);
		break;
	case NN_BNSCALE:
		p_bn_scl_parm = (NN_BNSCALE_PARM*)parm_addr;
		p_bn_scl_parm->in_addr		= net_map_addr_with_parsflag(p_bn_scl_parm->in_addr, buf_ofs, model_ofs);
		p_bn_scl_parm->out_addr		= net_map_addr_with_parsflag(p_bn_scl_parm->out_addr, buf_ofs, model_ofs);
		p_bn_scl_parm->mean_addr	= net_map_addr_with_parsflag(p_bn_scl_parm->mean_addr, buf_ofs, model_ofs);
		p_bn_scl_parm->alpha_addr	= net_map_addr_with_parsflag(p_bn_scl_parm->alpha_addr, buf_ofs, model_ofs);
		p_bn_scl_parm->beta_addr	= net_map_addr_with_parsflag(p_bn_scl_parm->beta_addr, buf_ofs, model_ofs);
		break;
#if USE_NEON
	case NN_PRELU:
		p_prelu_parm = (NN_PRELU_PARM *)parm_addr;
		p_prelu_parm->in_addr           = net_map_addr_with_parsflag(p_prelu_parm->in_addr, buf_ofs, model_ofs);
		p_prelu_parm->out_addr          = net_map_addr_with_parsflag(p_prelu_parm->out_addr, buf_ofs, model_ofs);
		p_prelu_parm->slope_addr        = net_map_addr_with_parsflag(p_prelu_parm->slope_addr, buf_ofs, model_ofs);
		break;
	case NN_PRIORBOX:
		p_priorbox_parm = (NN_PRIORBOX_PARM *)parm_addr;
		p_priorbox_parm->in_addr        = net_map_addr_with_parsflag(p_priorbox_parm->in_addr, buf_ofs, model_ofs);
		p_priorbox_parm->out_addr       = net_map_addr_with_parsflag(p_priorbox_parm->out_addr, buf_ofs, model_ofs);
		p_priorbox_parm->in_trans_addr  = net_map_addr_with_parsflag(p_priorbox_parm->in_trans_addr, buf_ofs, model_ofs);
		p_priorbox_parm->out_trans_addr = net_map_addr_with_parsflag(p_priorbox_parm->out_trans_addr, buf_ofs, model_ofs);
		break;
	case NN_DETOUT:
		p_detout_parm = (NN_DETOUT_PARM *)parm_addr;
		for (i = 0; i < NN_DETOUT_IN_NUM; i++) {
			p_detout_parm->in_addr[i]   = net_map_addr_with_parsflag(p_detout_parm->in_addr[i], buf_ofs, model_ofs);
		}
		p_detout_parm->out_addr         = net_map_addr_with_parsflag(p_detout_parm->out_addr, buf_ofs, model_ofs);
		p_detout_parm->in_trans_addr    = net_map_addr_with_parsflag(p_detout_parm->in_trans_addr, buf_ofs, model_ofs);
		p_detout_parm->out_trans_addr   = net_map_addr_with_parsflag(p_detout_parm->out_trans_addr, buf_ofs, model_ofs);
		p_detout_parm->tmp_addr         = net_map_addr_with_parsflag(p_detout_parm->tmp_addr, buf_ofs, model_ofs);
		break;
#endif
#else
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
	case NN_SOFTMAX:
		p_softmax_parm = (NN_SOFTMAX_PARM *)parm_addr;
		p_softmax_parm->in_addr		= net_map_addr_with_parsflag(p_softmax_parm->in_addr, model_ofs, buf_ofs);
		p_softmax_parm->out_addr	= net_map_addr_with_parsflag(p_softmax_parm->out_addr, model_ofs, buf_ofs);
#if USE_NEON
#if AI_V4
		p_softmax_parm->tmp_addr        = net_map_addr_with_parsflag(p_softmax_parm->tmp_addr, model_ofs, buf_ofs);
#else
		p_softmax_parm->in_trans_addr   = net_map_addr_with_parsflag(p_softmax_parm->in_trans_addr, model_ofs, buf_ofs);
		p_softmax_parm->out_trans_addr  = net_map_addr_with_parsflag(p_softmax_parm->out_trans_addr, model_ofs, buf_ofs);
#endif
#endif
		break;
	case NN_PREPROC:
		p_pre_parm = (NN_PRE_PARM*)parm_addr;
		p_pre_parm->in_addr             = net_map_addr_with_parsflag(p_pre_parm->in_addr, model_ofs, buf_ofs);
		p_pre_parm->out_addr            = net_map_addr_with_parsflag(p_pre_parm->out_addr, model_ofs, buf_ofs);
		p_pre_parm->interm_addr			= net_map_addr_with_parsflag(p_pre_parm->interm_addr, model_ofs, buf_ofs);
		p_pre_parm->sub_img.plane_addr  = net_map_addr_with_parsflag(p_pre_parm->sub_img.plane_addr, model_ofs, buf_ofs);
		if (((p_pre_parm->in_addr & 0xf) != 0) || ((p_pre_parm->out_addr & 0xf) != 0)
			|| ((p_pre_parm->sub_img.plane_addr & 0xf) != 0)) {
			DBG_WRN("preproc address should be 16 alignment: in = 0x%08x, out = 0x%08x, mean = 0x%08x\r\n"
					, (int)p_pre_parm->in_addr, (int)p_pre_parm->out_addr, (int)p_pre_parm->sub_img.plane_addr);
		}
		break;
	case NN_FC_POST:
		p_fc_post_parm = (NN_FC_POST_PARM*)parm_addr;
#if AI_V4
		p_fc_post_parm->in_addr         = net_map_addr_with_parsflag(p_fc_post_parm->in_addr, model_ofs, buf_ofs);
		p_fc_post_parm->out_addr        = net_map_addr_with_parsflag(p_fc_post_parm->out_addr, model_ofs, buf_ofs);
		p_fc_post_parm->tmp_addr        = net_map_addr_with_parsflag(p_fc_post_parm->tmp_addr, model_ofs, buf_ofs);
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
#if AI_V4
		p_pool_parm->tmp_addr           = net_map_addr_with_parsflag(p_pool_parm->tmp_addr, model_ofs, buf_ofs);
#endif
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
				io_head[i].pa = vendor_ais_user_buff_va2pa(io_head[i].va, net_id);
			}

			// parsing output
			io_head = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM) + p_cust_head->input_num*sizeof(NN_DATA));
			for (i = 0; i < p_cust_head->output_num; i++) {
				io_head[i].va = net_map_addr_with_parsflag(io_head[i].va, model_ofs, buf_ofs);
				io_head[i].pa = vendor_ais_user_buff_va2pa(io_head[i].va, net_id);
			}

			// parsing model
			io_head = (NN_DATA*)(parm_addr + sizeof(NN_CUSTOM_PARM) + (p_cust_head->input_num + p_cust_head->output_num)*sizeof(NN_DATA));
			for (i = 0; i < p_cust_head->model_num; i++) {
				if (io_head[i].va > 0) {
					io_head[i].va = net_map_addr_with_parsflag(io_head[i].va, model_ofs, buf_ofs);
					io_head[i].pa = vendor_ais_user_buff_va2pa(io_head[i].va, net_id);
				}
			}
			if (p_cust_head->temp_buf_addr > 0) {
				p_cust_head->temp_buf_addr = net_map_addr_with_parsflag(p_cust_head->temp_buf_addr, model_ofs, buf_ofs);
			}
		}
#else
		p_cust_head = (NN_CUSTOM_PARM*)parm_addr;
		p_cust_head->input.va	= net_map_addr_with_parsflag(p_cust_head->input.va, model_ofs, buf_ofs);
		p_cust_head->output.va	= net_map_addr_with_parsflag(p_cust_head->output.va, model_ofs, buf_ofs);
		p_cust_head->model.va	= net_map_addr_with_parsflag(p_cust_head->model.va, model_ofs, buf_ofs);
		p_cust_head->input.pa   = vendor_ais_user_buff_va2pa(p_cust_head->input.va, net_id);
		p_cust_head->output.pa  = vendor_ais_user_buff_va2pa(p_cust_head->output.va, net_id);
		if (p_cust_head->model.va > 0)
			p_cust_head->model.pa   = vendor_ais_user_model_va2pa(p_cust_head->model.va, net_id);
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
#if USE_NEON
	case NN_PRELU:
		p_prelu_parm = (NN_PRELU_PARM *)parm_addr;
		p_prelu_parm->in_addr           = net_map_addr_with_parsflag(p_prelu_parm->in_addr, model_ofs, buf_ofs);
		p_prelu_parm->out_addr          = net_map_addr_with_parsflag(p_prelu_parm->out_addr, model_ofs, buf_ofs);
		p_prelu_parm->slope_addr        = net_map_addr_with_parsflag(p_prelu_parm->slope_addr, model_ofs, buf_ofs);
		break;
	case NN_PRIORBOX:
		p_priorbox_parm = (NN_PRIORBOX_PARM *)parm_addr;
		p_priorbox_parm->in_addr        = net_map_addr_with_parsflag(p_priorbox_parm->in_addr, model_ofs, buf_ofs);
		p_priorbox_parm->out_addr       = net_map_addr_with_parsflag(p_priorbox_parm->out_addr, model_ofs, buf_ofs);
#if AI_V4
		p_priorbox_parm->tmp_addr       = net_map_addr_with_parsflag(p_priorbox_parm->tmp_addr, model_ofs, buf_ofs);
#else
		p_priorbox_parm->in_trans_addr  = net_map_addr_with_parsflag(p_priorbox_parm->in_trans_addr, model_ofs, buf_ofs);
		p_priorbox_parm->out_trans_addr = net_map_addr_with_parsflag(p_priorbox_parm->out_trans_addr, model_ofs, buf_ofs);
#endif
		break;
	case NN_DETOUT:
		p_detout_parm = (NN_DETOUT_PARM *)parm_addr;
		for (i = 0; i < NN_DETOUT_IN_NUM; i++) {
			p_detout_parm->in_addr[i]   = net_map_addr_with_parsflag(p_detout_parm->in_addr[i], model_ofs, buf_ofs);
		}
		p_detout_parm->out_addr         = net_map_addr_with_parsflag(p_detout_parm->out_addr, model_ofs, buf_ofs);
#if !AI_V4
		p_detout_parm->in_trans_addr    = net_map_addr_with_parsflag(p_detout_parm->in_trans_addr, model_ofs, buf_ofs);
		p_detout_parm->out_trans_addr   = net_map_addr_with_parsflag(p_detout_parm->out_trans_addr, model_ofs, buf_ofs);
#endif
		p_detout_parm->tmp_addr         = net_map_addr_with_parsflag(p_detout_parm->tmp_addr, model_ofs, buf_ofs);
		break;
#endif // USE_NEON
#endif // CNN_25_MATLAB
    case NN_RESHAPE:
		p_permute_parm = (NN_PERMUTE_PARM *)parm_addr;
		p_permute_parm->in_addr         = net_map_addr_with_parsflag(p_permute_parm->in_addr, model_ofs, buf_ofs);
		p_permute_parm->out_addr        = net_map_addr_with_parsflag(p_permute_parm->out_addr, model_ofs, buf_ofs);
		p_permute_parm->tmp_addr        = net_map_addr_with_parsflag(p_permute_parm->tmp_addr, model_ofs, buf_ofs);
		break;
	case NN_REVERSE:
		p_reverse_parm = (NN_REVERSE_PARM *)parm_addr;
		p_reverse_parm->in_addr         = net_map_addr_with_parsflag(p_reverse_parm->in_addr, model_ofs, buf_ofs);
		p_reverse_parm->out_addr        = net_map_addr_with_parsflag(p_reverse_parm->out_addr, model_ofs, buf_ofs);
		p_reverse_parm->tmp_addr        = net_map_addr_with_parsflag(p_reverse_parm->tmp_addr, model_ofs, buf_ofs);
		break;
	case NN_LSTM: {
		NN_LSTM_PARM* p_lstm_parm = (NN_LSTM_PARM *)parm_addr;
		UINT32 i = 0;
		
		p_lstm_parm->in_addr0 = net_map_addr_with_parsflag(p_lstm_parm->in_addr0, model_ofs, buf_ofs);
		p_lstm_parm->in_addr1 = net_map_addr_with_parsflag(p_lstm_parm->in_addr1, model_ofs, buf_ofs);
		p_lstm_parm->out_addr = net_map_addr_with_parsflag(p_lstm_parm->out_addr, model_ofs, buf_ofs);
		p_lstm_parm->tmp_addr = net_map_addr_with_parsflag(p_lstm_parm->tmp_addr, model_ofs, buf_ofs);
		p_lstm_parm->indicator_parm_addr = net_map_addr_with_parsflag(p_lstm_parm->indicator_parm_addr, model_ofs, buf_ofs);
		
		for (i = 0; i < NN_LSTM_PARM_NUM; i++) {
			p_lstm_parm->feat_parm_addr[i]   = net_map_addr_with_parsflag(p_lstm_parm->feat_parm_addr[i],   model_ofs, buf_ofs);
			p_lstm_parm->static_parm_addr[i] = net_map_addr_with_parsflag(p_lstm_parm->static_parm_addr[i], model_ofs, buf_ofs);
			p_lstm_parm->hidden_parm_addr[i] = net_map_addr_with_parsflag(p_lstm_parm->hidden_parm_addr[i], model_ofs, buf_ofs);
			p_lstm_parm->bias_parm_addr[i]   = net_map_addr_with_parsflag(p_lstm_parm->bias_parm_addr[i],   model_ofs, buf_ofs);
		}
	}
		break;
	case NN_NORM:
		p_norm_parm = (NN_NORM_PARM *)parm_addr;
		p_norm_parm->in_addr            = net_map_addr_with_parsflag(p_norm_parm->in_addr, model_ofs, buf_ofs);
		p_norm_parm->out_addr           = net_map_addr_with_parsflag(p_norm_parm->out_addr, model_ofs, buf_ofs);
		p_norm_parm->tmp_addr           = net_map_addr_with_parsflag(p_norm_parm->tmp_addr, model_ofs, buf_ofs);
		p_norm_parm->scale_addr         = net_map_addr_with_parsflag(p_norm_parm->scale_addr, model_ofs, buf_ofs);
		break;
	case NN_LAYER_NORMALIZATION:
		p_layer_norm_parm = (NN_LAYER_NORMALIZATION_PARM *)parm_addr;
		p_layer_norm_parm->in_addr            = net_map_addr_with_parsflag(p_layer_norm_parm->in_addr, model_ofs, buf_ofs);
		p_layer_norm_parm->out_addr           = net_map_addr_with_parsflag(p_layer_norm_parm->out_addr, model_ofs, buf_ofs);
		p_layer_norm_parm->tmp_addr           = net_map_addr_with_parsflag(p_layer_norm_parm->tmp_addr, model_ofs, buf_ofs);
		p_layer_norm_parm->gamma_addr         = net_map_addr_with_parsflag(p_layer_norm_parm->gamma_addr, model_ofs, buf_ofs);
		p_layer_norm_parm->beta_addr          = net_map_addr_with_parsflag(p_layer_norm_parm->beta_addr, model_ofs, buf_ofs);
		break;
#if NN_DLI
	case NN_DLI_SQRT:
	case NN_DLI_EXP:
	case NN_DLI_LOG:
	case NN_DLI_SIN:
	case NN_DLI_FLOOR:
	case NN_DLI_ROUND:
	case NN_DLI_DIV:
	case NN_DLI_POW:
	case NN_DLI_EQUAL:
	case NN_DLI_GREATER:
	case NN_DLI_LESS:
		p_dli_elementwise_parm = (NN_DLI_ElEMENTWISE_PARM *)parm_addr;
		p_dli_elementwise_parm->temp_buf_va = net_map_addr_with_parsflag(p_dli_elementwise_parm->temp_buf_va, model_ofs, buf_ofs);
		break;
	case NN_DLI_RESIZE:
		p_dli_resize_parm = (NN_DLI_RESIZE_PARM *)parm_addr;
		p_dli_resize_parm->temp_buf_va = net_map_addr_with_parsflag(p_dli_resize_parm->temp_buf_va, model_ofs, buf_ofs);
		break;
	case NN_DLI_SOFTMAX:
		p_dli_softmax_parm = (NN_DLI_SOFTMAX_PARM *)parm_addr;
		p_dli_softmax_parm->temp_buf_va = net_map_addr_with_parsflag(p_dli_softmax_parm->temp_buf_va, model_ofs, buf_ofs);
		break;
#endif
	default :
		break;
	}
}

#if !CNN_25_MATLAB
VOID vendor_ais_unmap_ai_app_addr(UINT32 parm_addr, UINT32 parm_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end, UINT32 net_id)
{
	VENDOR_AI_APP_HEAD *p_head = (VENDOR_AI_APP_HEAD *)parm_addr;

	switch (p_head->mode) {
	case AI_MODE_NEURAL: {
			VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)p_head->parm_addr;
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
			VENDOR_AI_ROIPOOL_PARM *p_parm = (VENDOR_AI_ROIPOOL_PARM *)p_head->parm_addr;
			p_parm->in_addr                     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_addr                    = net_unmap_addr(p_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->roi_ker.roi_addr            = net_unmap_addr(p_parm->roi_ker.roi_addr, model_ofs, buf_ofs, model_end, buf_end);
		}
		break;
	case AI_MODE_SVM: {
			VENDOR_AI_SVM_PARM *p_parm = (VENDOR_AI_SVM_PARM *)p_head->parm_addr;
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
			VENDOR_AI_FC_PARM *p_parm = (VENDOR_AI_FC_PARM *)p_head->parm_addr;
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
			VENDOR_AI_PERMUTE_PARM *p_parm = (VENDOR_AI_PERMUTE_PARM *)p_head->parm_addr;
			p_parm->in_addr                     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_addr                    = net_unmap_addr(p_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		}
		break;
	case AI_MODE_REORG: {
			VENDOR_AI_REORG_PARM *p_parm = (VENDOR_AI_REORG_PARM *)p_head->parm_addr;
			p_parm->in_addr                     = net_unmap_addr(p_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
			p_parm->out_addr                    = net_unmap_addr(p_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
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

VOID vendor_ais_unmap_ai_ll_addr(UINT32 parm_addr, UINT32 parm_va_ofs, UINT32 parm_pa_ofs, UINT32 model_pa_ofs, UINT32 buf_pa_ofs, UINT32 model_pa_end, UINT32 buf_pa_end, UINT32 net_id)
{
	VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)parm_addr;
	UINT64 *p_ll_end_cmd;
	UINT32 next_ll_addr = 0;
	CNN_LL_PARM *p_cnn_ll_parm;
	NUE_LL_PARM *p_nue_ll_parm;
	NUE2_LL_PARM *p_nue2_ll_parm;

	//p_ll_cmd = (UINT64 *)p_head->parm_addr;
	p_ll_end_cmd = (UINT64 *)(p_head->parm_addr + p_head->parm_size - sizeof(UINT64));
	next_ll_addr = vendor_ais_get_nextll_addr_cmd(*p_ll_end_cmd);
	if (next_ll_addr != 0) {
		next_ll_addr -= parm_pa_ofs;
		*p_ll_end_cmd = vendor_ais_set_nextll_addr_cmd(*p_ll_end_cmd, next_ll_addr);
	}
	
	switch (p_head->eng) {
	case AI_ENG_CNN:
	case AI_ENG_CNN2:
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(CNN_LL_PARM));
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
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE_LL_PARM));
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
		//p_ll_cmd = (UINT64 *)(p_head->parm_addr + sizeof(NUE2_LL_PARM));
		p_nue2_ll_parm = (NUE2_LL_PARM*)p_head->parm_addr;
		p_nue2_ll_parm->input[0].bit.addr  = net_unmap_addr(p_nue2_ll_parm->input[0].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue2_ll_parm->input[1].bit.addr  = net_unmap_addr(p_nue2_ll_parm->input[1].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue2_ll_parm->input[2].bit.addr  = net_unmap_addr(p_nue2_ll_parm->input[2].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue2_ll_parm->output[0].bit.addr = net_unmap_addr(p_nue2_ll_parm->output[0].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue2_ll_parm->output[1].bit.addr = net_unmap_addr(p_nue2_ll_parm->output[1].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		p_nue2_ll_parm->output[2].bit.addr = net_unmap_addr(p_nue2_ll_parm->output[2].bit.addr, model_pa_ofs, buf_pa_ofs, model_pa_end, buf_pa_end);
		break;
	default:
		DBG_WRN("unknown engine type : %d\r\n", (int)p_head->eng);
		break;
	}
	
	p_head->parm_addr -= parm_va_ofs;
}

VOID vendor_ais_unmap_drv_addr(NN_MODE mode, UINT32 parm_addr, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end, UINT32 net_id)
{
	NN_CPU_PARM *p_cpu_parm;
	NN_PRE_PARM *p_pre_parm;
	NN_FC_POST_PARM *p_fc_post_parm;
	NN_POOL_PARM *p_pool_parm;
	NN_CUSTOM_PARM *p_cust_head;
	NN_BNSCALE_PARM *p_bn_scl_parm;
	NN_SOFTMAX_PARM *p_softmax_parm;
	NN_POSTPROC_PARM *p_postproc_parm;
#if USE_NEON
	NN_PRELU_PARM *p_prelu_parm;
	NN_PRIORBOX_PARM *p_priorbox_parm;
	NN_DETOUT_PARM *p_detout_parm;
	INT32 i;
#endif
    NN_PERMUTE_PARM *p_permute_parm;
	NN_REVERSE_PARM *p_reverse_parm;
	NN_NORM_PARM *p_norm_parm;
	NN_LAYER_NORMALIZATION_PARM *p_layer_norm_parm;
#if NN_DLI
	NN_DLI_ElEMENTWISE_PARM *p_dli_elementwise_parm;
	NN_DLI_RESIZE_PARM *p_dli_resize_parm;
	NN_DLI_SOFTMAX_PARM *p_dli_softmax_parm;
#endif

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
		break;
	case NN_PROPOSAL:
		p_cpu_parm = (NN_CPU_PARM *)parm_addr;
		p_cpu_parm->addr_in             = net_unmap_addr(p_cpu_parm->addr_in, model_ofs, buf_ofs, model_end, buf_end);
		p_cpu_parm->addr_out            = net_unmap_addr(p_cpu_parm->addr_out, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_POSTPROC:
		p_postproc_parm = (NN_POSTPROC_PARM *)parm_addr;
		p_postproc_parm->in_addr        = net_unmap_addr(p_postproc_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_postproc_parm->out_addr       = net_unmap_addr(p_postproc_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_postproc_parm->tmp_addr       = net_unmap_addr(p_postproc_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_SOFTMAX:
		p_softmax_parm = (NN_SOFTMAX_PARM *)parm_addr;
		p_softmax_parm->in_addr         = net_unmap_addr(p_softmax_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_softmax_parm->out_addr        = net_unmap_addr(p_softmax_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
#if USE_NEON
#if AI_V4
		p_softmax_parm->tmp_addr        = net_unmap_addr(p_softmax_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
#else
		p_softmax_parm->in_trans_addr   = net_unmap_addr(p_softmax_parm->in_trans_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_softmax_parm->out_trans_addr  = net_unmap_addr(p_softmax_parm->out_trans_addr, model_ofs, buf_ofs, model_end, buf_end);
#endif
#endif
		break;
	case NN_PREPROC:
		p_pre_parm = (NN_PRE_PARM *)parm_addr;
		p_pre_parm->in_addr             = net_unmap_addr(p_pre_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_pre_parm->out_addr            = net_unmap_addr(p_pre_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_pre_parm->interm_addr         = net_unmap_addr(p_pre_parm->interm_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_pre_parm->sub_img.plane_addr  = net_unmap_addr(p_pre_parm->sub_img.plane_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_FC_POST:
		p_fc_post_parm = (NN_FC_POST_PARM *)parm_addr;
#if AI_V4
		p_fc_post_parm->in_addr         = net_unmap_addr(p_fc_post_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_fc_post_parm->out_addr        = net_unmap_addr(p_fc_post_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_fc_post_parm->tmp_addr        = net_unmap_addr(p_fc_post_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
#else
		p_fc_post_parm->addr_in         = net_unmap_addr(p_fc_post_parm->addr_in, model_ofs, buf_ofs, model_end, buf_end);
		p_fc_post_parm->addr_out        = net_unmap_addr(p_fc_post_parm->addr_out, model_ofs, buf_ofs, model_end, buf_end);
#endif
		p_fc_post_parm->bias_addr       = net_unmap_addr(p_fc_post_parm->bias_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_POOL:
		p_pool_parm = (NN_POOL_PARM *)parm_addr;
		p_pool_parm->in_addr            = net_unmap_addr(p_pool_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_pool_parm->out_addr           = net_unmap_addr(p_pool_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
#if AI_V4
		p_pool_parm->tmp_addr           = net_unmap_addr(p_pool_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
#endif
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

			if (p_cust_head->temp_buf_addr > 0) {
				p_cust_head->temp_buf_addr = net_unmap_addr(p_cust_head->temp_buf_addr, model_ofs, buf_ofs, model_end, buf_end);
			}
		}
#else
		p_cust_head = (NN_CUSTOM_PARM *)parm_addr;
		p_cust_head->input.va           = net_unmap_addr(p_cust_head->input.va, model_ofs, buf_ofs, model_end, buf_end);
		p_cust_head->output.va          = net_unmap_addr(p_cust_head->output.va, model_ofs, buf_ofs, model_end, buf_end);
		p_cust_head->model.va           = net_unmap_addr(p_cust_head->model.va, model_ofs, buf_ofs, model_end, buf_end);
		p_cust_head->input.pa           = p_cust_head->input.va;
		p_cust_head->output.pa          = p_cust_head->output.va;
		p_cust_head->model.pa           = p_cust_head->model.va;
#endif
		break;
	case NN_BNSCALE:
		p_bn_scl_parm = (NN_BNSCALE_PARM *)parm_addr;
		p_bn_scl_parm->in_addr          = net_unmap_addr(p_bn_scl_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_bn_scl_parm->out_addr         = net_unmap_addr(p_bn_scl_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_bn_scl_parm->mean_addr        = net_unmap_addr(p_bn_scl_parm->mean_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_bn_scl_parm->alpha_addr       = net_unmap_addr(p_bn_scl_parm->alpha_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_bn_scl_parm->beta_addr        = net_unmap_addr(p_bn_scl_parm->beta_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
#if USE_NEON
	case NN_PRELU:
		p_prelu_parm = (NN_PRELU_PARM *)parm_addr;
		p_prelu_parm->in_addr           = net_unmap_addr(p_prelu_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_prelu_parm->out_addr          = net_unmap_addr(p_prelu_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_prelu_parm->slope_addr        = net_unmap_addr(p_prelu_parm->slope_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_PRIORBOX:
		p_priorbox_parm = (NN_PRIORBOX_PARM *)parm_addr;
		p_priorbox_parm->in_addr        = net_unmap_addr(p_priorbox_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_priorbox_parm->out_addr       = net_unmap_addr(p_priorbox_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
#if AI_V4
		p_priorbox_parm->tmp_addr       = net_unmap_addr(p_priorbox_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
#else
		p_priorbox_parm->in_trans_addr  = net_unmap_addr(p_priorbox_parm->in_trans_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_priorbox_parm->out_trans_addr = net_unmap_addr(p_priorbox_parm->out_trans_addr, model_ofs, buf_ofs, model_end, buf_end);
#endif
		break;
	case NN_DETOUT:
		p_detout_parm = (NN_DETOUT_PARM *)parm_addr;
		for (i = 0; i < NN_DETOUT_IN_NUM; i++) {
			p_detout_parm->in_addr[i]   = net_unmap_addr(p_detout_parm->in_addr[i], model_ofs, buf_ofs, model_end, buf_end);
		}
		p_detout_parm->out_addr         = net_unmap_addr(p_detout_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
#if !AI_V4
		p_detout_parm->in_trans_addr    = net_unmap_addr(p_detout_parm->in_trans_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_detout_parm->out_trans_addr   = net_unmap_addr(p_detout_parm->out_trans_addr, model_ofs, buf_ofs, model_end, buf_end);
#endif
		p_detout_parm->tmp_addr         = net_unmap_addr(p_detout_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
#endif // USE_NEON
	case NN_RESHAPE:
		p_permute_parm = (NN_PERMUTE_PARM *)parm_addr;
		p_permute_parm->in_addr         = net_unmap_addr(p_permute_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_permute_parm->out_addr        = net_unmap_addr(p_permute_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_permute_parm->tmp_addr        = net_unmap_addr(p_permute_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_REVERSE:
		p_reverse_parm = (NN_REVERSE_PARM *)parm_addr;
		p_reverse_parm->in_addr         = net_unmap_addr(p_reverse_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_reverse_parm->out_addr        = net_unmap_addr(p_reverse_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_reverse_parm->tmp_addr        = net_unmap_addr(p_reverse_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_LSTM: {
		NN_LSTM_PARM* p_lstm_parm = (NN_LSTM_PARM *)parm_addr;
		UINT32 i = 0;
		
		p_lstm_parm->in_addr0 = net_unmap_addr(p_lstm_parm->in_addr0, model_ofs, buf_ofs, model_end, buf_end);
		p_lstm_parm->in_addr1 = net_unmap_addr(p_lstm_parm->in_addr1, model_ofs, buf_ofs, model_end, buf_end);
		p_lstm_parm->out_addr = net_unmap_addr(p_lstm_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_lstm_parm->tmp_addr = net_unmap_addr(p_lstm_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_lstm_parm->indicator_parm_addr = net_unmap_addr(p_lstm_parm->indicator_parm_addr, model_ofs, buf_ofs, model_end, buf_end);
		
		for (i = 0; i < NN_LSTM_PARM_NUM; i++) {
			p_lstm_parm->feat_parm_addr[i]   = net_unmap_addr(p_lstm_parm->feat_parm_addr[i],   model_ofs, buf_ofs, model_end, buf_end);
			p_lstm_parm->static_parm_addr[i] = net_unmap_addr(p_lstm_parm->static_parm_addr[i], model_ofs, buf_ofs, model_end, buf_end);
			p_lstm_parm->hidden_parm_addr[i] = net_unmap_addr(p_lstm_parm->hidden_parm_addr[i], model_ofs, buf_ofs, model_end, buf_end);
			p_lstm_parm->bias_parm_addr[i]   = net_unmap_addr(p_lstm_parm->bias_parm_addr[i],   model_ofs, buf_ofs, model_end, buf_end);
		}
	}
		break;
	case NN_NORM:
		p_norm_parm = (NN_NORM_PARM *)parm_addr;
		p_norm_parm->in_addr            = net_unmap_addr(p_norm_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_norm_parm->out_addr           = net_unmap_addr(p_norm_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_norm_parm->tmp_addr           = net_unmap_addr(p_norm_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_norm_parm->scale_addr         = net_unmap_addr(p_norm_parm->scale_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_LAYER_NORMALIZATION:
		p_layer_norm_parm = (NN_LAYER_NORMALIZATION_PARM *)parm_addr;
		p_layer_norm_parm->in_addr            = net_unmap_addr(p_layer_norm_parm->in_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_layer_norm_parm->out_addr           = net_unmap_addr(p_layer_norm_parm->out_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_layer_norm_parm->tmp_addr           = net_unmap_addr(p_layer_norm_parm->tmp_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_layer_norm_parm->gamma_addr         = net_unmap_addr(p_layer_norm_parm->gamma_addr, model_ofs, buf_ofs, model_end, buf_end);
		p_layer_norm_parm->beta_addr          = net_unmap_addr(p_layer_norm_parm->beta_addr, model_ofs, buf_ofs, model_end, buf_end);
		break;
#if NN_DLI
	case NN_DLI_SQRT:
	case NN_DLI_EXP:
	case NN_DLI_LOG:
	case NN_DLI_SIN:
	case NN_DLI_FLOOR:
	case NN_DLI_ROUND:
	case NN_DLI_DIV:
	case NN_DLI_POW:
	case NN_DLI_EQUAL:
	case NN_DLI_GREATER:
	case NN_DLI_LESS:
		p_dli_elementwise_parm = (NN_DLI_ElEMENTWISE_PARM *)parm_addr;
		p_dli_elementwise_parm->temp_buf_va = net_unmap_addr(p_dli_elementwise_parm->temp_buf_va, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_DLI_RESIZE:
		p_dli_resize_parm = (NN_DLI_RESIZE_PARM *)parm_addr;
		p_dli_resize_parm->temp_buf_va = net_unmap_addr(p_dli_resize_parm->temp_buf_va, model_ofs, buf_ofs, model_end, buf_end);
		break;
	case NN_DLI_SOFTMAX:
		p_dli_softmax_parm = (NN_DLI_SOFTMAX_PARM *)parm_addr;
		p_dli_softmax_parm->temp_buf_va = net_unmap_addr(p_dli_softmax_parm->temp_buf_va, model_ofs, buf_ofs, model_end, buf_end);
		break;
#endif
	default :
		break;
	}
}
#endif

#if CNN_25_MATLAB
HD_RESULT vendor_ais_pars_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	NN_IOMEM *p_io_mem;
	NN_GEN_NET_INFO net_info = {0};
	HD_RESULT ret = HD_OK;
	UINT32 cfg2;
	ER er = E_OK;
	UINT32 model_pa_ofs, buff_pa_ofs;
	UINT32 parm_va_ofs, model_va_ofs, buff_va_ofs;
	UINT32 process_index = 0, idx = 0;
#if 1
	UINT32 layer_index = 0;
#endif

	if ((p_mem == NULL) || vendor_ais_is_null_mem(p_mem->user_parm)
		|| vendor_ais_is_null_mem(p_mem->user_model) || vendor_ais_is_null_mem(p_mem->user_buff)) {
		DBG_ERR("invalid memory input\r\n");
		return HD_ERR_ABORT;
	}
	er = vendor_ais_get_net_info(&net_info, p_mem->user_parm.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	p_io_mem = net_info.p_io_mem;
	proc_layer_num = p_head->mode_ctrl_num;
	

	parm_va_ofs = ALIGN_CEIL_4(p_mem->user_parm.va);
	model_va_ofs = ALIGN_CEIL_4(p_mem->user_model.va);
	buff_va_ofs = ALIGN_CEIL_4(p_mem->user_buff.va);
	model_pa_ofs = ALIGN_CEIL_4(p_mem->user_model.pa);
	buff_pa_ofs = ALIGN_CEIL_4(p_mem->user_buff.pa);
	
#if 1
	for (layer_index = 0; layer_index < p_head->layer_num; layer_index++) {
		for (idx = 0; idx < NN_IMEM_NUM; idx++) {
			p_io_mem[layer_index].SAI[idx].pa = net_map_addr_with_parsflag(p_io_mem[layer_index].SAI[idx].pa, buff_pa_ofs, model_pa_ofs);
			p_io_mem[layer_index].SAI[idx].va = net_map_addr_with_parsflag(p_io_mem[layer_index].SAI[idx].va, buff_va_ofs, model_va_ofs);
		}
		for (idx = 0; idx < NN_OMEM_NUM; idx++) {
			p_io_mem[layer_index].SAO[idx].pa = net_map_addr_with_parsflag(p_io_mem[layer_index].SAO[idx].pa, buff_pa_ofs, model_pa_ofs);
			p_io_mem[layer_index].SAO[idx].va = net_map_addr_with_parsflag(p_io_mem[layer_index].SAO[idx].va, buff_va_ofs, model_va_ofs);
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
			p_io_mem[layer_index].SAI[idx].pa = net_map_addr_with_parsflag(p_io_mem[layer_index].SAI[idx].pa, buff_pa_ofs, model_pa_ofs);
			p_io_mem[layer_index].SAI[idx].va = net_map_addr_with_parsflag(p_io_mem[layer_index].SAI[idx].va, buff_va_ofs, model_va_ofs);
		}
		for (idx = 0; idx < NN_OMEM_NUM; idx++) {
			p_io_mem[layer_index].SAO[idx].pa = net_map_addr_with_parsflag(p_io_mem[layer_index].SAO[idx].pa, buff_pa_ofs, model_pa_ofs);
			p_io_mem[layer_index].SAO[idx].va = net_map_addr_with_parsflag(p_io_mem[layer_index].SAO[idx].va, buff_va_ofs, model_va_ofs);
		}

#if 0	// for debug
		for (idx = 0; idx < NN_IMEM_NUM; idx++) {
			DBG_DUMP("p_io_mem[%d].SAI[%d] pa = 0x%08x; va = 0x%08x; size=%d; fmt=%08x\r\n", (int)layer_index, (int)idx
					, (int)p_io_mem[layer_index].SAI[idx].pa, (int)p_io_mem[layer_index].SAI[idx].va
					, (int)p_io_mem[layer_index].SAI[idx].size, (int)p_io_mem[layer_index].SAI[idx].fmt);
		}
		for (idx = 0; idx < NN_OMEM_NUM; idx++) {
			DBG_DUMP("p_io_mem[%d].SAO[%d] pa = 0x%08x; va = 0x%08x; size=%d; fmt=%08x\r\n", (int)layer_index, (int)idx
					, (int)p_io_mem[layer_index].SAO[idx].pa, (int)p_io_mem[layer_index].SAO[idx].va
					, (int)p_io_mem[layer_index].SAO[idx].size, (int)p_io_mem[layer_index].SAO[idx].fmt);
		}
#endif
	}
#endif

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		p_mctrl[process_index].addr += parm_va_ofs;
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
		#if 1
			if (process_index == 0) {
				VENDOR_AI_APP_HEAD *p_head = (VENDOR_AI_APP_HEAD *)p_mctrl[process_index].addr;
				if (p_head->mode == AI_MODE_NEURAL) {
					VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)(p_head->parm_addr + parm_va_ofs);
					p_parm->in_addr = 0;
				}
            }
		#endif
			vendor_ais_map_ai_app_addr(p_mctrl[process_index].addr, parm_va_ofs, model_va_ofs, buff_va_ofs, net_id);
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			vendor_ais_map_ai_ll_addr(p_mctrl[process_index].addr, parm_va_ofs, model_va_ofs, buff_va_ofs, net_id);
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			vendor_ais_map_drv_addr((NN_MODE)p_mctrl[process_index].mode, p_mctrl[process_index].addr, model_va_ofs, buff_va_ofs, net_id);
			break;
		}
		//fmem_dcache_sync((UINT32 *)(p_mctrl[process_index].addr), (p_mctrl[process_index].size), DMA_BIDIRECTIONAL);
	}

	return HD_OK;
}
#else
HD_RESULT vendor_ais_pars_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 parm_pa_ofs, model_pa_ofs, buff_pa_ofs;
	UINT32 parm_va_ofs, model_va_ofs, buff_va_ofs;
	NN_GEN_NET_INFO net_info = {0};
	HD_RESULT ret = HD_OK;
	UINT32 process_index = 0, idx = 0;
	UINT32 pre_index = 999;

	if ((p_mem == NULL)
			|| vendor_ais_is_null_mem(p_mem->user_parm)
			|| vendor_ais_is_null_mem(p_mem->user_model)
			|| vendor_ais_is_null_mem(p_mem->user_buff)) {
		DBG_ERR("null memory...\r\n");
		return HD_ERR_ABORT;
	}
	
	ret = vendor_ais_get_net_info(&net_info, p_mem->user_parm.va);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return ret;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	parm_pa_ofs = ALIGN_CEIL_4(p_mem->user_parm.pa);
	model_pa_ofs = ALIGN_CEIL_4(p_mem->user_model.pa);
	buff_pa_ofs = ALIGN_CEIL_4(p_mem->user_buff.pa);

	parm_va_ofs = ALIGN_CEIL_4(p_mem->user_parm.va);
	model_va_ofs = ALIGN_CEIL_4(p_mem->user_model.va);
	buff_va_ofs = ALIGN_CEIL_4(p_mem->user_buff.va);

#if CNN_MULTI_INPUT
	g_ai_net_is_batch[net_id] = 0;
#endif
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		NN_DATA *p_sai, *p_sao;
		UINT32 imem_cnt, omem_cnt;
		UINT32 layer_index = p_mctrl[process_index].layer_index;

		p_mctrl[process_index].iomem.imem_addr += parm_va_ofs;
		p_mctrl[process_index].iomem.omem_addr += parm_va_ofs;
		#if CNN_FMT_V4
		p_mctrl[process_index].in_bufinfo_addr  += parm_va_ofs;
		p_mctrl[process_index].out_bufinfo_addr += parm_va_ofs;
		#endif
		#if CNN_MULTI_INPUT
		if (layer_index == 0 && process_index > 0) {
			p_sai = (NN_DATA *)p_mctrl[process_index].iomem.imem_addr;
			if (p_sai[0].va == 0) {
				g_ai_net_is_batch[net_id] = 1;
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
			p_sao[idx].pa = net_map_addr_with_parsflag(p_sao[idx].pa, model_pa_ofs, buff_pa_ofs);
			p_sao[idx].va = net_map_addr_with_parsflag(p_sao[idx].va, model_va_ofs, buff_va_ofs);
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

#if NN_DLI
	// Map NN_DLI tensor address
	vendor_ais_map_nn_dli_tensor_addr(p_head, parm_va_ofs, model_va_ofs, buff_va_ofs);
#endif

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		p_mctrl[process_index].addr += parm_va_ofs;
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			vendor_ais_map_ai_app_addr(p_mctrl[process_index].addr, parm_va_ofs, model_va_ofs, buff_va_ofs, net_id);
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			vendor_ais_map_ai_ll_addr(p_mctrl[process_index].addr, parm_va_ofs, parm_pa_ofs, model_pa_ofs, buff_pa_ofs, net_id);
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			vendor_ais_map_drv_addr((NN_MODE)p_mctrl[process_index].mode, p_mctrl[process_index].addr, model_va_ofs, buff_va_ofs, net_id);
			break;
		}
		//fmem_dcache_sync((UINT32 *)(p_mctrl[process_index].addr), (p_mctrl[process_index].size), DMA_BIDIRECTIONAL);
#if NN_DLI
		if(p_mctrl[process_index].mode >= NN_DLI_FIRST &&
			p_mctrl[process_index].mode <= NN_DLI_LAST) {
			VENDOR_AI_ENGINE_PLUGIN* p_cpu_engine = vendor_ai_get_engine(0); //CPU engine
			if (p_cpu_engine->get_cb != 0) {
				HD_RESULT rv = HD_ERR_NOT_SUPPORT;
				// chk plugin support NN_DLI operation
				rv = p_cpu_engine->get_cb(VENDOR_AI_CTRL_NET, VENDOR_AI_CTRL_LYR, 0, 0, 0, VENDOR_AI_PLUGIN_SIGN, NULL, NULL);
				if (rv != HD_OK) {
					DBG_ERR("plugin(cpu) cannot support NN_DLI op 0x%08lX\r\n", p_mctrl[process_index].mode);
					return rv;
				}
			}
		}
#endif
	}
	return HD_OK;
}

HD_RESULT vendor_ais_unpars_net(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 net_id)
{
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 parm_pa_ofs, model_pa_ofs, buff_pa_ofs;
	UINT32 parm_va_ofs, model_va_ofs, buff_va_ofs;
	UINT32 model_pa_end, buff_pa_end;
	UINT32 model_va_end, buff_va_end;
	UINT32 process_index = 0, idx = 0;
	NN_GEN_NET_INFO net_info = {0};
	HD_RESULT ret = HD_OK;
	UINT32 pre_index = 999;

	if ((p_mem == NULL)
			|| vendor_ais_is_null_mem(p_mem->user_parm)
			|| vendor_ais_is_null_mem(p_mem->user_model)
			|| vendor_ais_is_null_mem(p_mem->user_buff)) {
		DBG_ERR("null memory...\r\n");
		return HD_ERR_ABORT;
	}

	ret = vendor_ais_get_net_info(&net_info, p_mem->user_parm.va);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return ret;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	parm_pa_ofs = ALIGN_CEIL_4(p_mem->user_parm.pa);
	model_pa_ofs = ALIGN_CEIL_4(p_mem->user_model.pa);
	buff_pa_ofs = ALIGN_CEIL_4(p_mem->user_buff.pa);

	parm_va_ofs = ALIGN_CEIL_4(p_mem->user_parm.va);
	model_va_ofs = ALIGN_CEIL_4(p_mem->user_model.va);
	buff_va_ofs = ALIGN_CEIL_4(p_mem->user_buff.va);

	model_pa_end = p_mem->user_model.pa + p_mem->user_model.size;
	buff_pa_end = p_mem->user_buff.pa + p_mem->user_buff.size;

	model_va_end = p_mem->user_model.va + p_mem->user_model.size;
	buff_va_end = p_mem->user_buff.va + p_mem->user_buff.size;

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		NN_DATA *p_sai, *p_sao;
		UINT32 imem_cnt, omem_cnt;
		UINT32 layer_index = p_mctrl[process_index].layer_index;
		
		if (layer_index == pre_index) {
			continue;
		}
		pre_index = layer_index;

		p_sai = (NN_DATA *)p_mctrl[process_index].iomem.imem_addr;
		p_sao = (NN_DATA *)p_mctrl[process_index].iomem.omem_addr;
		imem_cnt = p_mctrl[process_index].iomem.imem_cnt;
		omem_cnt = p_mctrl[process_index].iomem.omem_cnt;

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

#if NN_DLI
	// Unmap NN_DLI tensor address
	vendor_ais_unmap_nn_dli_tensor_addr(p_head, parm_va_ofs, model_va_ofs, buff_va_ofs, model_va_end, buff_va_end);
#endif

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		switch (p_mctrl[process_index].trig_src) {
		case NN_GEN_TRIG_APP_AI_DRV:
			vendor_ais_unmap_ai_app_addr(p_mctrl[process_index].addr, parm_va_ofs, model_va_ofs, buff_va_ofs, model_va_end, buff_va_end, net_id);
			break;
		case NN_GEN_TRIG_LL_AI_DRV:
			vendor_ais_unmap_ai_ll_addr(p_mctrl[process_index].addr, parm_va_ofs, parm_pa_ofs, model_pa_ofs, buff_pa_ofs, model_pa_end, buff_pa_end, net_id);
			break;
		case NN_GEN_TRIG_COMMON:
		default:
			//if ((NN_MODE)p_mctrl[process_index].mode == NN_CUSTOMER) {
			//	vendor_ai_cust_uninit(p_mctrl[process_index].eng, p_mctrl[process_index].addr, net_id);
			//}
			vendor_ais_unmap_drv_addr((NN_MODE)p_mctrl[process_index].mode, p_mctrl[process_index].addr, model_va_ofs, buff_va_ofs, model_va_end, buff_va_end, net_id);
			break;
		}
		//fmem_dcache_sync((UINT32 *)(p_mctrl[process_index].addr), (p_mctrl[process_index].size), DMA_BIDIRECTIONAL);
		p_mctrl[process_index].addr -= parm_va_ofs;

		p_mctrl[process_index].iomem.imem_addr -= parm_va_ofs;
		p_mctrl[process_index].iomem.omem_addr -= parm_va_ofs;
		#if CNN_FMT_V4
		p_mctrl[process_index].in_bufinfo_addr  -= parm_va_ofs;
		p_mctrl[process_index].out_bufinfo_addr -= parm_va_ofs;
		#endif
	}

	return HD_OK;
}
#endif

#if MODIFY_ENG_ONLINE
HD_RESULT vendor_ais_pars_engid(UINT32 eng_number, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc;
#else
#endif
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 process_index = 0;
	ER er = E_OK;
	NN_GEN_NET_INFO net_info = {0};
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem = NULL;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 proc_layer_num;
	
	if (eng_number > 1) {
		DBG_ERR("invalid input engine\r\n");
		return HD_ERR_ABORT;
	}
	
	if (net_id > g_ai_support_net_max) {
		DBG_ERR("invalid net_id\r\n");
		return HD_ERR_ABORT;
	}

#if (NET_GEN_HEAP == 1)
	p_proc = _vendor_ai_info(net_id);
	p_mem = &(p_proc->map_mem);
#else
	p_mem = &g_map_mem[net_id];
#endif
	
	if ((p_mem == NULL) || vendor_ais_is_null_mem(p_mem->user_parm)
		|| vendor_ais_is_null_mem(p_mem->user_model) || vendor_ais_is_null_mem(p_mem->user_buff)) {
		DBG_ERR("invalid memory input\r\n");
		return HD_ERR_ABORT;
	}
	
	er = vendor_ais_get_net_info(&net_info, p_mem->user_parm.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;
	
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		if ((p_mctrl[process_index].eng == NN_GEN_ENG_CNN || p_mctrl[process_index].eng == NN_GEN_ENG_CNN2)) {
			p_mctrl[process_index].eng = (eng_number == 0)?NN_GEN_ENG_CNN:NN_GEN_ENG_CNN2;		
		}
	}
	
	return HD_OK;
}

HD_RESULT vendor_ais_pars_engid_perlayer(UINT32 eng_number_addr, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc;
#else
#endif
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 process_index = 0;
	ER er = E_OK;
	NN_GEN_NET_INFO net_info = {0};
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem = NULL;
	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 proc_layer_num;
	UINT8* usr_eng_info = (UINT8*)eng_number_addr;
	
	if (usr_eng_info == NULL) {
		DBG_ERR("invalid input addr\r\n");
		return HD_ERR_ABORT;
	}
	
	if (net_id > g_ai_support_net_max) {
		DBG_ERR("invalid net_id\r\n");
		return HD_ERR_ABORT;
	}
	
#if (NET_GEN_HEAP == 1)
	p_proc = _vendor_ai_info(net_id);
	p_mem = &(p_proc->map_mem);
#else
	p_mem = &g_map_mem[net_id];
#endif
	
	if ((p_mem == NULL) || vendor_ais_is_null_mem(p_mem->user_parm)
		|| vendor_ais_is_null_mem(p_mem->user_model) || vendor_ais_is_null_mem(p_mem->user_buff)) {
		DBG_ERR("invalid memory input\r\n");
		return HD_ERR_ABORT;
	}
	
	er = vendor_ais_get_net_info(&net_info, p_mem->user_parm.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;
	
	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		if ((p_mctrl[process_index].eng == NN_GEN_ENG_CNN || p_mctrl[process_index].eng == NN_GEN_ENG_CNN2)) {		
			p_mctrl[process_index].eng = (usr_eng_info[process_index] == 0)?NN_GEN_ENG_CNN:NN_GEN_ENG_CNN2;		
		}
	}
	
	return HD_OK;
}
#endif

VOID vendor_ais_net_gen_en_init(BOOL enable, UINT32 init, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
	if (enable) {
		p_proc->gen_init |= init;
	} else {
		p_proc->gen_init &= (~init);
	}
#else
	if (enable) {
		g_ai_net_gen_init[net_id] |= init;
	} else {
		g_ai_net_gen_init[net_id] &= (~init);
	}
#endif
}
#if CNN_25_MATLAB
static HD_RESULT vendor_ais_net_def_in(NN_GEN_MODE_CTRL *p_first_mctrl)
{
	if ((p_first_mctrl == NULL) || (p_first_mctrl->addr == 0) || (p_first_mctrl->size == 0)) {
		return HD_ERR_ABORT;
	}

	if (p_first_mctrl->layer_index != 0) {
		///< only for first layer
		return HD_ERR_NOT_SUPPORT;
	}

	if (p_first_mctrl->trig_src == NN_GEN_TRIG_APP_AI_DRV) {
		KDRV_AI_APP_HEAD *p_app_head = (KDRV_AI_APP_HEAD *)p_first_mctrl->addr;
		switch (p_app_head->mode) {
		case AI_MODE_NEURAL: {
				KDRV_AI_NEURAL_PARM *p_parm = (KDRV_AI_NEURAL_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					if (p_parm->out0_dma_en) {
						p_parm->in_addr = p_parm->out0_addr;
					} else if (p_parm->out1_dma_en) {
						p_parm->in_addr = p_parm->out1_addr;
					}
				}
				if (p_parm->elt.addr == 0) {
					p_parm->elt.addr = p_parm->in_addr;
				}
			}
			break;
		case AI_MODE_ROIPOOL: {
				KDRV_AI_ROIPOOL_PARM *p_parm = (KDRV_AI_ROIPOOL_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					p_parm->in_addr = p_parm->out_addr;
				}
				if (p_parm->roi_ker.roi_addr == 0) {
					p_parm->roi_ker.roi_addr = p_parm->in_addr;
				}
			}
			break;
		case AI_MODE_SVM: {
				KDRV_AI_SVM_PARM *p_parm = (KDRV_AI_SVM_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					p_parm->in_addr = p_parm->out_addr;
				}
			}
			break;
		case AI_MODE_FC: {
				KDRV_AI_FC_PARM *p_parm = (KDRV_AI_FC_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					p_parm->in_addr = p_parm->out_addr;
				}
			}
			break;
		case AI_MODE_PERMUTE: {
				KDRV_AI_PERMUTE_PARM *p_parm = (KDRV_AI_PERMUTE_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					p_parm->in_addr = p_parm->out_addr;
				}
			}
			break;
		case AI_MODE_REORG: {
				KDRV_AI_REORG_PARM *p_parm = (KDRV_AI_REORG_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					p_parm->in_addr = p_parm->out_addr;
				}
			}
			break;
		default:
			DBG_ERR("unknown input mode(%d) in app\r\n", (int)p_first_mctrl->mode);
			break;
		}
	}

	return HD_OK;
}
#else
static HD_RESULT vendor_ais_net_def_in(NN_GEN_MODE_CTRL *p_first_mctrl)
{
	if ((p_first_mctrl == NULL) || (p_first_mctrl->addr == 0) || (p_first_mctrl->size == 0)) {
		return HD_ERR_ABORT;
	}

	if (p_first_mctrl->layer_index != 0) {
		///< only for first layer
		return HD_ERR_NOT_SUPPORT;
	}

	if (p_first_mctrl->trig_src == NN_GEN_TRIG_APP_AI_DRV) {
		KDRV_AI_APP_HEAD *p_app_head = (KDRV_AI_APP_HEAD *)p_first_mctrl->addr;
		switch (p_app_head->mode) {
		case AI_MODE_NEURAL: {
				KDRV_AI_NEURAL_PARM *p_parm = (KDRV_AI_NEURAL_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					if (p_parm->out0_dma_en) {
						p_parm->in_addr = AI_DUMMY_ADDR;
					} else if (p_parm->out1_dma_en) {
						p_parm->in_addr = AI_DUMMY_ADDR;
					}
				}
				if (p_parm->elt.addr == 0) {
					p_parm->elt.addr = AI_DUMMY_ADDR;
				}
			}
			break;
		case AI_MODE_ROIPOOL: {
				KDRV_AI_ROIPOOL_PARM *p_parm = (KDRV_AI_ROIPOOL_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					p_parm->in_addr = AI_DUMMY_ADDR;
				}
				if (p_parm->roi_ker.roi_addr == 0) {
					p_parm->roi_ker.roi_addr = AI_DUMMY_ADDR;
				}
			}
			break;
		case AI_MODE_SVM: {
				KDRV_AI_SVM_PARM *p_parm = (KDRV_AI_SVM_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					p_parm->in_addr = AI_DUMMY_ADDR;
				}
			}
			break;
		case AI_MODE_FC: {
				KDRV_AI_FC_PARM *p_parm = (KDRV_AI_FC_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					p_parm->in_addr = AI_DUMMY_ADDR;
				}
			}
			break;
		case AI_MODE_PERMUTE: {
				KDRV_AI_PERMUTE_PARM *p_parm = (KDRV_AI_PERMUTE_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					p_parm->in_addr = AI_DUMMY_ADDR;
				}
			}
			break;
		case AI_MODE_REORG: {
				KDRV_AI_REORG_PARM *p_parm = (KDRV_AI_REORG_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == 0) {
					p_parm->in_addr = AI_DUMMY_ADDR;
				}
			}
			break;
		default:
			DBG_ERR("unknown input mode(%d) in app\r\n", (int)p_first_mctrl->mode);
			break;
		}
	}

	return HD_OK;
}

static HD_RESULT vendor_ais_net_undef_in(NN_GEN_MODE_CTRL *p_first_mctrl)
{
	if ((p_first_mctrl == NULL) || (p_first_mctrl->addr == 0) || (p_first_mctrl->size == 0)) {
		return HD_ERR_ABORT;
	}

	if (p_first_mctrl->layer_index != 0) {
		///< only for first layer
		return HD_ERR_NOT_SUPPORT;
	}

	if (p_first_mctrl->trig_src == NN_GEN_TRIG_APP_AI_DRV) {
		KDRV_AI_APP_HEAD *p_app_head = (KDRV_AI_APP_HEAD *)p_first_mctrl->addr;
		switch (p_app_head->mode) {
		case AI_MODE_NEURAL: {
				KDRV_AI_NEURAL_PARM *p_parm = (KDRV_AI_NEURAL_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == AI_DUMMY_ADDR) {
					p_parm->in_addr = 0;
				}
				if (p_parm->elt.addr == AI_DUMMY_ADDR) {
					p_parm->elt.addr = 0;
				}
			}
			break;
		case AI_MODE_ROIPOOL: {
				KDRV_AI_ROIPOOL_PARM *p_parm = (KDRV_AI_ROIPOOL_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == AI_DUMMY_ADDR) {
					p_parm->in_addr = 0;
				}
				if (p_parm->roi_ker.roi_addr == AI_DUMMY_ADDR) {
					p_parm->roi_ker.roi_addr = 0;
				}
			}
			break;
		case AI_MODE_SVM: {
				KDRV_AI_SVM_PARM *p_parm = (KDRV_AI_SVM_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == AI_DUMMY_ADDR) {
					p_parm->in_addr = 0;
				}
			}
			break;
		case AI_MODE_FC: {
				KDRV_AI_FC_PARM *p_parm = (KDRV_AI_FC_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == AI_DUMMY_ADDR) {
					p_parm->in_addr = 0;
				}
			}
			break;
		case AI_MODE_PERMUTE: {
				KDRV_AI_PERMUTE_PARM *p_parm = (KDRV_AI_PERMUTE_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == AI_DUMMY_ADDR) {
					p_parm->in_addr = 0;
				}
			}
			break;
		case AI_MODE_REORG: {
				KDRV_AI_REORG_PARM *p_parm = (KDRV_AI_REORG_PARM *)p_app_head->parm_addr;
				if (p_parm->in_addr == AI_DUMMY_ADDR) {
					p_parm->in_addr = 0;
				}
			}
			break;
		default:
			DBG_ERR("unknown input mode(%d) in app\r\n", (int)p_first_mctrl->mode);
			break;
		}
	}

	return HD_OK;
}
#endif

HD_RESULT vendor_ais_check_net(VENDOR_AIS_FLOW_MAP_MEM_PARM mem)
{
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	UINT32 proc_layer_num;


	NN_GEN_MODE_CTRL *p_mctrl;
	UINT32 process_index = 0;
	ER er = E_OK;

	er = vendor_ais_get_net_info(&net_info, mem.user_parm.va);
	if (er != E_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return er;
	}
	p_head = net_info.p_head;
	p_mctrl = net_info.p_mctrl;
	proc_layer_num = p_head->mode_ctrl_num;

	for (process_index = 0; process_index < proc_layer_num; process_index++) {
		if (p_mctrl[process_index].trig_src != NN_GEN_TRIG_APP_AI_DRV) {
			continue;
		}

		vendor_ais_net_def_in(&p_mctrl[process_index]);

/*
see IVOT_TP-325: not used any more

		er = nn_check_net_parm(p_mctrl[process_index].addr);
		if (er != E_OK) {
			DBG_ERR("nn_check_net_parm fail : %x; process idx = %d\r\n", (int)er, (int)process_index);
			return HD_ERR_FAIL;
		}
*/
	}
#if !CNN_25_MATLAB
		vendor_ais_net_undef_in(&p_mctrl[process_index]);
#endif
	
	return HD_OK;
}

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
HD_RESULT vendor_ais_net_gen_chk_vers(VENDOR_AIS_FLOW_MEM_PARM *p_model, UINT32 net_id)
{
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	HD_RESULT ret = HD_OK;

	ret = vendor_ais_get_net_info(&net_info, p_model->va);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return ret;
	}
	if (net_info.p_mctrl[0].trig_src == 0) DBG_WRN("<<NOTE>> THIS IS APP MODE BIN !!!\n");
	p_head = net_info.p_head;

	ret = vendor_ais_chk_vers(p_head->chip.id, p_head->chip.gentool_vers, net_id);
	return ret;
}

INT32 vendor_ais_net_gen_get_id_list_size(VENDOR_AIS_FLOW_MEM_PARM *p_model)
{
	NN_GEN_NET_INFO net_info = {0};
	HD_RESULT ret = HD_OK;

	ret = vendor_ais_get_net_info(&net_info, p_model->va);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return (-1);
	}
	return net_info.p_head->layer_id_list_size;
}

HD_RESULT _vendor_ai_net_gen_chk_model_is_fresh_loaded(VENDOR_AIS_FLOW_MEM_PARM *p_model)
{
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODE_CTRL *p_mctrl = NULL;
	UINT32 proc_layer_num=0;
	HD_RESULT ret = HD_OK;
	UINT32 idx=0;

	ret = vendor_ais_get_net_info(&net_info, p_model->va);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return ret;
	}

	p_mctrl = net_info.p_mctrl;
	proc_layer_num = net_info.p_head->mode_ctrl_num;

	for (idx=0; idx< proc_layer_num; idx++) {
		if (p_mctrl[idx].addr >= p_model->va) {
			DBG_ERR("mctrl[%lu].addr is NOT offset-type!!\r\n", idx);
			return HD_ERR_FAIL;
		}
		if (p_mctrl[idx].iomem.imem_addr >= p_model->va) {
			DBG_ERR("mctrl[%lu].iomem.imem_addr is NOT offset-type!!\r\n", idx);
			return HD_ERR_FAIL;
		}
		if (p_mctrl[idx].iomem.omem_addr >= p_model->va) {
			DBG_ERR("mctrl[%lu].iomem.omem_addr is NOT offset-type!!\r\n", idx);
			return HD_ERR_FAIL;
		}
		if (p_mctrl[idx].in_bufinfo_addr >= p_model->va) {
			DBG_ERR("mctrl[%lu].in_bufinfo_addr is NOT offset-type!!\r\n", idx);
			return HD_ERR_FAIL;
		}
		if (p_mctrl[idx].out_bufinfo_addr >= p_model->va) {
			DBG_ERR("mctrl[%lu].out_bufinfo_addr is NOT offset-type!!\r\n", idx);
			return HD_ERR_FAIL;
		}
	}
	return HD_OK;
}

#if 1
extern VENDOR_AIS_FLOW_MEM_OFS g_ai_mem_ofs;
#endif
HD_RESULT vendor_ais_net_gen_init(VENDOR_AIS_FLOW_MAP_MEM_PARM map_mem, UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc;
#else
#endif
	VENDOR_AIS_FLOW_MAP_MEM_INFO map_mem_info;
	HD_RESULT ret;

	/*
		init memory size
	*/
	memcpy(&map_mem_info.parm, &map_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM));
	map_mem_info.net_id = net_id;

	/*
		init preproc

	ret = vendor_ais_pre_init();
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_pre_init error %d\r\n", ret);
        return HD_ERR_ABORT;
	} else {
	    vendor_ais_net_gen_en_init(TRUE, AI_PREPROC_INIT, net_id);
	}
	*/

	/*
		open vendor ai
	*/

	ret = vendor_ais_open_net(net_id);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_open_net error %d\r\n", ret);
		return HD_ERR_ABORT;
	} else {
        if (is_multi_process) {
			vendor_ai_dla_open_path(net_id);
		}
		vendor_ais_net_gen_en_init(TRUE, AI_FLOW_IOCTL_INIT, net_id);
	}

#if LL_BASE_TEST
	/* set mem offset for test */
	//printf("gen_init->set_mem_ofs: id(%lu) ofs(0x%lx)\r\n", g_ai_mem_ofs.net_id, g_ai_mem_ofs.ofs);
	ret = vendor_ais_set_mem_ofs(g_ai_mem_ofs.net_id, g_ai_mem_ofs.ofs);
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_set_mem_ofs error %d\r\n", ret);
		return ret;
	}
#endif

	/*
	    map memory with kernel flow and get kernel va
	*/
	ret = vendor_ais_remap_kerl_mem(&map_mem_info);
	if (ret != HD_OK) {
		return ret;
	} else {
		vendor_ais_net_gen_en_init(TRUE, AI_KERL_MEM_INIT, net_id);
	}
#if (NET_GEN_HEAP == 1)
	p_proc = _vendor_ai_info(net_id);
	p_proc->map_mem = map_mem_info.parm;
#else
	g_map_mem[net_id] = map_mem_info.parm;
#endif

	/*
	    map memory with ai driver
	*/
#if (NET_GEN_HEAP == 1)
	ret = vendor_ais_flow_user_init((p_proc->map_mem), net_id);
#else
	ret = vendor_ais_flow_user_init(g_map_mem[net_id], net_id);
#endif
	if (ret != HD_OK) {
		DBG_ERR("vendor_ai_drv_init error %d\r\n", ret);
        return HD_ERR_ABORT;
	} else {
	    vendor_ais_net_gen_en_init(TRUE, AI_DRV_IOCTL_INIT, net_id);
	}

	/*
	    map model address in user space
	*/
#if CNN_AI_FASTBOOT
	if (vendor_ais_is_fastboot() == DISABLE) { // fastboot has already fix addr in _vendor_ai_net_map_fastboot_mem()
#endif
#if (NET_GEN_HEAP == 1)
		ret = vendor_ais_pars_net(&(p_proc->map_mem), net_id);
#else
		ret = vendor_ais_pars_net(&g_map_mem[net_id], net_id);
#endif
		if (ret != HD_OK) {
			DBG_ERR("vendor_ais_pars_net fail %d\r\n", ret);
	        return ret;
		}
#if CNN_AI_FASTBOOT
	}
#endif

	/*
		parsing model
	*/
#if 0  // calling this can't do APP bin
#if (NET_GEN_HEAP == 1)
	ret = vendor_ais_check_net(&(p_proc->map_mem));
#else
	ret = vendor_ais_check_net(g_map_mem[net_id]);
#endif
	if (ret != HD_OK) {
		DBG_ERR("vendor_ais_check_net fail %d\r\n", ret);
        return HD_ERR_ABORT;
	}
#endif
	/*
	    map model address in kernel space
	*/
#if CNN_AI_FASTBOOT
	if (vendor_ais_is_fastboot() == DISABLE) { // fastboot has already fix addr in _vendor_ai_net_map_fastboot_mem()
#endif
		ret = vendor_ais_pars_kerl_mem(&map_mem_info);
		if (ret != HD_OK) {
			return ret;
		}
#if CNN_AI_FASTBOOT
	}
#endif

	return HD_OK;
}

HD_RESULT vendor_ais_net_gen_uninit(UINT32 net_id)
{
#if (NET_GEN_HEAP == 1)
	VENDOR_AI_NET_INFO_PROC *p_proc;
#else
#endif
	VENDOR_AIS_FLOW_MAP_MEM_INFO map_mem_info;
#if (NET_GEN_HEAP == 1)
	p_proc = _vendor_ai_info(net_id);
	map_mem_info.parm = p_proc->map_mem;
#else
	map_mem_info.parm = g_map_mem[net_id];
#endif
	map_mem_info.net_id = net_id;

	vendor_ais_unpars_kerl_mem(&map_mem_info);

#if (NEW_AI_FLOW == 1)
#else
#if 0 //LL_SUPPORT_ROI
	vendor_ai_drv_uninit_link_info(net_id);
#endif
#if !CNN_25_MATLAB
#if (NET_GEN_HEAP == 1)
	vendor_ais_unpars_net(&(p_proc->map_mem), net_id);
#else
	vendor_ais_unpars_net(&g_map_mem[net_id], net_id);
#endif
#endif
#endif

#if (NET_GEN_HEAP == 1)
	if (p_proc->gen_init & AI_KERL_MEM_INIT) {
#else
	if (g_ai_net_gen_init[net_id] & AI_KERL_MEM_INIT) {
#endif
		vendor_ais_unmap_kerl_mem(&map_mem_info);
		vendor_ais_net_gen_en_init(FALSE, AI_KERL_MEM_INIT, net_id);
	}
#if (NET_GEN_HEAP == 1)
	if (p_proc->gen_init & AI_DRV_IOCTL_INIT) {
#else
	if (g_ai_net_gen_init[net_id] & AI_DRV_IOCTL_INIT) {
#endif
		vendor_ais_flow_user_uninit(net_id);
		vendor_ais_net_gen_en_init(FALSE, AI_DRV_IOCTL_INIT, net_id);
	}
#if (NET_GEN_HEAP == 1)
	if (p_proc->gen_init & AI_FLOW_IOCTL_INIT) {
#else
	if (g_ai_net_gen_init[net_id] & AI_FLOW_IOCTL_INIT) {
#endif
		if (is_multi_process) {
			vendor_ai_dla_close_path(net_id);
		}
		vendor_ais_close_net(net_id);
		
		vendor_ais_net_gen_en_init(FALSE, AI_FLOW_IOCTL_INIT, net_id);
	}
    /*
#if (NET_GEN_HEAP == 1)
	if (p_proc->gen_init & AI_PREPROC_INIT) {
#else
	if (g_ai_net_gen_init[net_id] & AI_PREPROC_INIT) {
#endif
		vendor_ais_pre_uninit();
		vendor_ais_net_gen_en_init(FALSE, AI_PREPROC_INIT, net_id);
	}
    */

	return HD_OK;
}

HD_RESULT vendor_ais_update_1st_layer_parm(VENDOR_AIS_IMG_PARM *p_input_info, NN_GEN_MODE_CTRL *p_1st_parm)
{
	NN_GEN_TRIG_SRC trigsrc;
	NN_MODE nn_mode;

	if ((p_input_info == NULL) || (p_1st_parm == NULL)) {
		DBG_ERR("null input ...r\n");
		return HD_ERR_BAD_DATA;
	}

	nn_mode = p_1st_parm->mode;
	trigsrc = p_1st_parm->trig_src;

	if (trigsrc == NN_GEN_TRIG_APP_AI_DRV) {
		VENDOR_AI_APP_HEAD *p_app_head = (VENDOR_AI_APP_HEAD *)p_1st_parm->addr;
		switch (p_app_head->mode) {
		case AI_MODE_NEURAL: {
				VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)p_app_head->parm_addr;
				if ((p_parm->size.width > p_input_info->width) || (p_parm->size.height > p_input_info->height)
					|| (p_parm->size.channel > p_input_info->channel) || (p_parm->in_ofs.line_ofs > p_input_info->line_ofs)) {
					DBG_ERR("input size(%d, %d, %d, %d) < model first layer size(%d, %d, %d, %d)\r\n"
							, (int)p_input_info->width, (int)p_input_info->height
							, (int)p_input_info->channel, (int)p_input_info->line_ofs
							, (int)p_parm->size.width, (int)p_parm->size.height, (int)p_parm->size.channel, (int)p_parm->in_ofs.line_ofs);
					return HD_ERR_ABORT;
				}
			}
			break;
		case AI_MODE_PREPROC: {
				KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)p_app_head->parm_addr;
#if (NEW_AI_FLOW == 1)
                //calculate ideal cycle
                p_1st_parm->idea_cycle = p_input_info->width * p_input_info->height;
#endif
				p_parm->in_size.width = p_input_info->width;
				p_parm->in_size.height = p_input_info->height;
				switch (p_input_info->fmt) {
				case HD_VIDEO_PXLFMT_YUV420:
#if AI_SUPPORT_MULTI_FMT
#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
					p_parm->src_fmt = AI_PREPROC_SRC_YUV420_NV21;
#else
					p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
#endif
#else
					p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
					p_parm->in_ofs[0].line_ofs = p_input_info->width;
                    p_parm->in_ofs[1].line_ofs = p_input_info->width;
#endif
					p_parm->in_ofs[0].line_ofs = p_input_info->line_ofs;
                    p_parm->in_ofs[1].line_ofs = p_input_info->line_ofs;
                    //p_parm->in_ofs[2].line_ofs = 0;
					break;
				/*case HD_VIDEO_PXLFMT_YUV422_ONE:
					p_parm->in_fmt = NET_IMG_YUV422;
					break;*/
				case HD_VIDEO_PXLFMT_Y8:
					p_parm->src_fmt = AI_PREPROC_SRC_YONLY;
					p_parm->in_ofs[0].line_ofs = p_input_info->line_ofs;
					break;
				case HD_VIDEO_PXLFMT_RGB888_PLANAR:
					p_parm->src_fmt = AI_PREPROC_SRC_RGB;
					p_parm->in_ofs[0].line_ofs = p_input_info->line_ofs;
                    p_parm->in_ofs[1].line_ofs = p_input_info->line_ofs;
                    p_parm->in_ofs[2].line_ofs = p_input_info->line_ofs;
					break;
				default:
					DBG_WRN("not support input format: 0x%08x\r\n", p_input_info->fmt);
					p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
					break;
				}
			}
			break;
		case AI_MODE_ROIPOOL:
		case AI_MODE_SVM:
		case AI_MODE_FC:
		case AI_MODE_PERMUTE:
		case AI_MODE_REORG:
		default:
			DBG_ERR("unknown input mode(%d) in app\r\n", (int)nn_mode);
			break;
		}

	} else if (trigsrc == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
		//========== for first layer linked list mode ==========
		VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)p_1st_parm->addr;

		switch (p_head->eng) {
		case AI_ENG_CNN:
		case AI_ENG_CNN2:
		case AI_ENG_NUE:
		break;
		case AI_ENG_NUE2: {
			NUE2_LL_PARM *p_parm = (NUE2_LL_PARM*)p_head->parm_addr;
#if (NEW_AI_FLOW == 1)
            //calculate ideal cycle
            p_1st_parm->idea_cycle = p_input_info->width * p_input_info->height;
#endif
			p_parm->size0.bit.width = p_input_info->width;
			p_parm->size0.bit.height = p_input_info->height;

			if (p_input_info->width < p_parm->scale_size.bit.h_scl_size) {
				p_parm->scale0.bit.h_rate = 0; // scale up case should set 0
			} else {
				p_parm->scale0.bit.h_rate = (((p_input_info->width-1)/(p_parm->scale_size.bit.h_scl_size-1))-1);
			}

			if (p_input_info->height < p_parm->scale_size.bit.v_scl_size) {
				p_parm->scale0.bit.v_rate = 0; // scale up case should set 0
			} else {
				p_parm->scale0.bit.v_rate = (((p_input_info->height-1)/(p_parm->scale_size.bit.v_scl_size-1))-1);
			}

			p_parm->scale1.bit.h_sfact = (((p_input_info->width-1)*65536/(p_parm->scale_size.bit.h_scl_size-1)) & 0xffff);
			p_parm->scale1.bit.v_sfact = (((p_input_info->height-1)*65536/(p_parm->scale_size.bit.v_scl_size-1)) & 0xffff);
			switch ((UINT32)p_input_info->fmt) {
			case HD_VIDEO_PXLFMT_YUV420:
#if AI_SUPPORT_MULTI_FMT
#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
				// force to assign fmt in ll cmd (for 32x YUV420_NV21)
				{
					UINT64* tmp_cmd = (UINT64*)p_head->parm_addr;
					UINT32 tmp_idx = 0;
					// search the register which contain yuv_mode register
					while (1) {
						if ((tmp_cmd[tmp_idx] >> 63) == 1) {
							if (((tmp_cmd[tmp_idx] >> 32) & 0xFF) == 0x04) {
								// modify the format
								UINT32* p_reg_val = (UINT32*)(&tmp_cmd[tmp_idx]);
								//if (p_input_info->fmt_type) { // Not support dynamic setting
								if (1) {
									p_reg_val[0] = p_reg_val[0] | 0x200000;
								} else {
									p_reg_val[0] = p_reg_val[0] & 0xFFFDFFFFF;
								}
								break;
							} else {
								// find next cmd
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
				p_parm->ilofs[0].bit.ofs = p_input_info->line_ofs;
				p_parm->ilofs[1].bit.ofs = p_input_info->line_ofs;
				//p_parm->in_ofs[2].line_ofs = 0;
				break;
			/*case HD_VIDEO_PXLFMT_YUV422_ONE:
				p_parm->in_fmt = NET_IMG_YUV422;
				break;*/
			case HD_VIDEO_PXLFMT_Y8:
				//p_parm->src_fmt = AI_PREPROC_SRC_YONLY;
				p_parm->ilofs[0].bit.ofs = p_input_info->line_ofs;
				break;
			case HD_VIDEO_PXLFMT_RGB888_PLANAR:
			case HD_VIDEO_PXLFMT_BGR888_PLANAR: // AI_Tool will handle the order of SAI[]
				//p_parm->src_fmt = AI_PREPROC_SRC_RGB;
				p_parm->ilofs[0].bit.ofs = p_input_info->line_ofs;
				p_parm->ilofs[1].bit.ofs = p_input_info->line_ofs;
				p_parm->ilofs[2].bit.ofs = p_input_info->line_ofs;
				break;
			default:
				DBG_WRN("not support input format: 0x%08x\r\n", p_input_info->fmt);
				//p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
				break;
			}
		}
		break;
		default:
			DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
			break;
		}
		//========== by CCC 191004 ==========
#else
		DBG_ERR("first layer can't be linked list\r\n");
		return HD_ERR_ABORT;
#endif

	} else if (trigsrc == NN_GEN_TRIG_COMMON) {
		//const UINT32 parm_addr = p_1st_parm->addr;
		switch (nn_mode) {
		case NN_PREPROC: {
				/*NN_PRE_PARM *p_parm = (NN_PRE_PARM *)parm_addr;
				p_parm->width = p_input_info->width;
				p_parm->height = p_input_info->height;
				p_parm->in_lofs = p_input_info->line_ofs;
				switch (p_input_info->fmt) {
				case HD_VIDEO_PXLFMT_YUV420:
					p_parm->in_fmt = NET_IMG_YUV420;
					break;
				case HD_VIDEO_PXLFMT_YUV422_ONE:
					p_parm->in_fmt = NET_IMG_YUV422;
					break;
				case HD_VIDEO_PXLFMT_Y8:
					p_parm->in_fmt = NET_IMG_YONLY;
					break;
				case HD_VIDEO_PXLFMT_RGB888_PLANAR:
					p_parm->in_fmt = NET_IMG_RGB_PLANE;
					break;
				default:
					DBG_WRN("not support input format: 0x%08x\r\n", p_input_info->fmt);
					p_parm->in_fmt = NET_IMG_YUV420;
					break;
				}*/
			}
			break;
		case NN_CUSTOMER: {
				NN_CUSTOM_PARM *p_parm = (NN_CUSTOM_PARM*)p_1st_parm->addr;
				NN_CUSTOM_DIM* p_input_dim = (NN_CUSTOM_DIM*)(p_1st_parm->addr + sizeof(NN_CUSTOM_PARM) +
				(p_parm->input_num + p_parm->output_num + p_parm->model_num)*sizeof(NN_DATA));
				p_input_dim[0].dim[0] = p_input_info->width;
				p_input_dim[0].dim[1] = p_input_info->height;
				p_input_dim[0].dim[2] = p_input_info->channel;
				p_input_dim[0].dim[3] = p_input_info->batch_num;
				p_input_dim[0].ofs[0] = p_input_info->line_ofs;
				p_input_dim[0].ofs[1] = p_input_info->channel_ofs;
				p_input_dim[0].ofs[2] = p_input_info->batch_ofs;
			}
			break;
		case NN_POSTPROC:
		case NN_SOFTMAX:
		case NN_FC_POST:
		case NN_POOL:
		case NN_BNSCALE:
		default:
			DBG_ERR("unknown input mode(%d) in common\r\n", (int)nn_mode);
			break;
		}

	} else {
		DBG_ERR("unknown first layer trigger source: %d\r\n", (int)trigsrc);
		return HD_ERR_ABORT;
	}

	return HD_OK;
}

extern HD_RESULT vendor_ai_cpu_cust_set_out_dim(UINT32 proc_id, UINT32 layer_id, UINT32 out_id, NN_CUSTOM_DIM* p_output_dim);

HD_RESULT vendor_ai_cpu_cust_set_out_dim(UINT32 proc_id, UINT32 layer_id, UINT32 out_id, NN_CUSTOM_DIM* p_output_dim)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager;
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	NN_GEN_MODE_CTRL *p_mctrl;
	//NN_DATA *p_sai, *p_sao;
	HD_RESULT rv = HD_OK;
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 dst_tag  = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('F')<<24));
	UINT32 dst_tag2 = (UINT32)((UINT32)('O') | ((UINT32)('I')<<8) | ((UINT32)('N')<<16) | ((UINT32)('2')<<24));
	UINT32 num_output_layer = 0, tmp_size = 0;
	NN_IN_OUT_BUF_INFO2* ext_info2 = NULL;

	if (p_output_dim == NULL) {
		DBG_ERR("p_output_dim is null?\r\n");
		return HD_ERR_NULL_PTR;
	}

	p_mem_manager = &p_proc->mem_manager;
	if (p_mem_manager == NULL) {
		DBG_ERR("p_mem_manager is null?\r\n");
		return HD_ERR_INV;
	}

	p_mem = &p_mem_manager->user_parm;
	if ((p_mem == NULL) || (p_mem->pa == 0) || (p_mem->va == 0) || (p_mem->size == 0)) {
		DBG_ERR("null input\r\n");
		return HD_ERR_INV;
	}

	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	rv = HD_ERR_PATH;

	p_mctrl = net_info.p_mctrl;

	// get layer info
	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	//printf("custnn: update ext_info of proc[%d] layer[%d] out[%d]!\n", (int)proc_id, (int)layer_id, (int)out_id);

	// try to find dst_tag2 first
	tmp_size = 0;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag2) { // bit1 represent tag_type
			ext_info2 = (NN_IN_OUT_BUF_INFO2 *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			if (ext_info2 == NULL) {
				DBG_ERR("ext_info2 pointer NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

	// original dst_tag
	tmp_size = 0;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINT32)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) { // bit1 represent tag_type
			NN_IN_OUT_BUF_INFO* ext_info = (NN_IN_OUT_BUF_INFO *)(((UINT32)p_tmp_head) + 2*sizeof(UINT32));
			if (ext_info == NULL) {
				DBG_ERR("ext_info pointer NULL!\r\n");
				rv = HD_ERR_NULL_PTR;
				goto exit;
			}
			num_output_layer = (p_tmp_head[0] - 2*sizeof(UINT32)) / sizeof(NN_IN_OUT_BUF_INFO); // number of external info

			for(UINT32 i = 0; i < num_output_layer; i++) {
				UINT32 buf_attr, info_mctrl_id, info_port_id;
				UINT32 imem_addr, omem_addr;

				// parse ext_id
				buf_attr = (ext_info[i].ext_id & 0xf0000000) >> 28;   // 0x9: input buffer, 0xa: output buffer
				//port_id = (ext_info[i].ext_id & 0xfff0000) >> 16;
				info_mctrl_id = (ext_info[i].ext_id & 0xffff);
				info_port_id = (ext_info[i].ext_id & 0xfff0000) >> 16;

				// debug
				/*printf("buf_attr(0x%lx) port_id(0x%lx) mctrl_id(0x%lx) ext_info_idx(%lu)\n", buf_attr, info_port_id, info_mctrl_id, i);
				printf("========= i(%d) ==============\n", (int)i);
				printf("layer_name = %s\n", ext_info[i].layer_name);
				printf("layer_type = %s\n", ext_info[i].layer_type);
				printf("caffe_layer_index = %d\n", ext_info[i].caffe_layer_index);
				printf("fusion_layer_index = %d\n", ext_info[i].fusion_layer_index);
				printf("out_bitdepth = %d\n", ext_info[i].out_bitdepth);
				printf("out_sign_bit_num = %d\n", ext_info[i].out_sign_bit_num);
				printf("out_int_bit_num = %d\n", ext_info[i].out_int_bit_num);
				printf("out_frac_bit_num = %d\n", ext_info[i].out_frac_bit_num);
				printf("out_scale_ratio = %f\n", ext_info[i].out_scale_ratio);
				printf("width = %d\n", ext_info[i].width);
				printf("height = %d\n", ext_info[i].height);
				printf("channel = %d\n", ext_info[i].channel);
				printf("batch = %d\n", ext_info[i].batch);
				printf("time = %d\n", ext_info[i].time);
				printf("out_lofs = %d\n", ext_info[i].out_lofs);
				printf("out_ch_ofs = %d\n", ext_info[i].out_ch_ofs);
				printf("out_batch_ofs  = %lu\n", ext_info[i].out_batch_ofs);
				printf("out_time_ofs = %lu\n", ext_info[i].out_time_ofs);
				printf("ext_id(0x%lx)\n", ext_info[i].ext_id);
				printf("dataorder = %s\n", ext_info[i].data_order);
				printf("\n\n");*/

				// check va fixing
				imem_addr = p_mctrl[info_mctrl_id].iomem.imem_addr;
				if (imem_addr < p_mem->va) imem_addr += p_mem->va; // if not fix yet(call this funciton before gen_init(), fix it
				omem_addr = p_mctrl[info_mctrl_id].iomem.omem_addr;
				if (omem_addr < p_mem->va) omem_addr += p_mem->va; // if not fix yet(call this funciton before gen_init(), fix it

				// config to NN_DATA
				if (buf_attr == 0x9) {
					//p_sai = (NN_DATA*)imem_addr;
					//printf("p_sai: ext_info_num(%u), mctrl_id(%lu), port_id(%lu)\n", p_sai[port_id].fmt.reserved, mctrl_id, port_id);
				} else if (buf_attr == 0xa) {
					if ((layer_id == info_mctrl_id) && (out_id == info_port_id)) {
						//printf("custnn: update OK!\n");
						//p_sao = (NN_DATA*)omem_addr;
						//printf("p_sao: ext_info_num(%u), mctrl_id(%lu), port_id(%lu)\n", p_sao[port_id].fmt.reserved, mctrl_id, port_id);
						ext_info[i].width = p_output_dim->dim[0];
						ext_info[i].height = p_output_dim->dim[1];
						ext_info[i].channel = p_output_dim->dim[2];
						ext_info[i].batch = p_output_dim->dim[3];
						ext_info[i].time = p_output_dim->dim[4];

						ext_info[i].out_lofs = p_output_dim->ofs[0];
						ext_info[i].out_ch_ofs = p_output_dim->ofs[1];
						ext_info[i].out_batch_ofs = p_output_dim->ofs[2];
						ext_info[i].out_time_ofs = p_output_dim->ofs[3];
						if (ext_info2 != NULL) {
							ext_info2[i].width = p_output_dim->dim[0];
							ext_info2[i].height = p_output_dim->dim[1];
							ext_info2[i].channel = p_output_dim->dim[2];
							ext_info2[i].batch = p_output_dim->dim[3];
							ext_info2[i].time = p_output_dim->dim[4];

							ext_info2[i].out_lofs = p_output_dim->ofs[0];
							ext_info2[i].out_ch_ofs = p_output_dim->ofs[1];
						}
						rv = HD_OK;
					}
				} else {
					DBG_ERR("Invalid buf_attr(0x%lx)\r\n", buf_attr);
					rv = HD_ERR_NG;
					goto exit;
				}
			}
			break;
		} else {
			if (p_tmp_head[0] == 0) { // bit0 represent size
				break;
			}
			tmp_size += p_tmp_head[0]; // add tmp_size, move to next
		}
	}

	if(rv != HD_OK) {
		DBG_ERR("custnn: proc[%d] set dim of layer[%d] out[%d] is failed!\r\n", (int)proc_id, (int)layer_id, (int)out_id);
	}

exit:
	return rv;
}

HD_RESULT vendor_ai_cust_set_next_info(UINT32 max_out, UINT32 max_map, uintptr_t* next_cust_parm_addr, UINT32* next_cust_match_in_idx)
{
    g_max_out_num = max_out;
    g_max_map_num = max_map;
    g_next_cust_parm_addr = next_cust_parm_addr;
    g_next_cust_match_in_idx = next_cust_match_in_idx;
    
    return HD_OK;
}

HD_RESULT vendor_ai_cust_set_next_process_mctrl(UINT32 next_parm_addr, UINT32 next_match_in_idx, UINT32 out_id, UINT32 map_in_idx, UINT32 net_id)
{
	if (net_id >= NN_SUPPORT_NET_MAX) {
		printf("vendor_ai_cust_set_next_process_mctrl: illegal net id %d\n", (int)net_id);
		return -1;
	}
	
	if (out_id >= g_max_out_num) {
		printf("vendor_ai_cust_set_next_process_mctrl: illegal out_id %d\n", (int)out_id);
		return -1;
	}
	
	if (map_in_idx >= g_max_map_num) {
		printf("vendor_ai_cust_set_next_process_mctrl: illegal map_in_idx %d\n", (int)map_in_idx);
		return -1;
	}	
	
	g_next_cust_parm_addr[net_id*(g_max_out_num*g_max_map_num)+ out_id*(g_max_map_num) + map_in_idx] = next_parm_addr;
    g_next_cust_match_in_idx[net_id*(g_max_out_num*g_max_map_num)+ out_id*(g_max_map_num) + map_in_idx] = next_match_in_idx;
	
	return HD_OK;
}


/* 
	Use current output addr to seach next cust layer input addr
*/
HD_RESULT vendor_ai_cpu_cust_set_next_parm_addr(UINT32 proc_id, UINT32 layer_id)
{
	NN_GEN_MODEL_HEAD *p_head = _vendor_ai_net_get_head(proc_id);
	NN_GEN_MODE_CTRL *p_mctrl = _vendor_ai_net_get_mctrl(proc_id);
	UINT32 proc_layer_num = p_head->mode_ctrl_num;
	UINT32 map_in_idx = 0;

	for (UINT32 j = 0; j < OUT_BUF_NUM(p_mctrl + layer_id); j++) {
			map_in_idx = 0;
			
		for (UINT32 p = 0; p < proc_layer_num; p++) {
			for (UINT32 i = 0; i < IN_BUF_NUM(p_mctrl + p); i++) {
				if (OUT_BUF_INDEX(p_mctrl + layer_id, j)  == IN_BUF_INDEX(p_mctrl + p, i) && p_mctrl[p].mode == NN_CUSTOMER) {
					vendor_ai_cust_set_next_process_mctrl(p_mctrl[p].addr, i, j, map_in_idx, proc_id);
					map_in_idx++;
					
					//printf("current customer layer process (%d) matches final customer layer process (%d).\n", (int)process_index, (int)final_cust_process_index);
					if (map_in_idx >= g_max_map_num) {
						break;
					}
				}
			}
		}
	}


	return HD_OK;
}

HD_RESULT vendor_ais_update_1st_layer_parm_v2(VENDOR_AIS_IMG_PARM_V2 *p_input_info, NN_GEN_MODE_CTRL *p_1st_parm)
{
	NN_GEN_TRIG_SRC trigsrc;
	NN_MODE nn_mode;

	if ((p_input_info == NULL) || (p_1st_parm == NULL)) {
		DBG_ERR("null input ...r\n");
		return HD_ERR_BAD_DATA;
	}

	nn_mode = p_1st_parm->mode;
	trigsrc = p_1st_parm->trig_src;

	if (trigsrc == NN_GEN_TRIG_APP_AI_DRV) {
		VENDOR_AI_APP_HEAD *p_app_head = (VENDOR_AI_APP_HEAD *)p_1st_parm->addr;
		switch (p_app_head->mode) {
		case AI_MODE_NEURAL: {
				VENDOR_AI_NEURAL_PARM *p_parm = (VENDOR_AI_NEURAL_PARM *)p_app_head->parm_addr;
				if ((p_parm->size.width > p_input_info->img_ch->width) || (p_parm->size.height > p_input_info->img_ch->height)
					|| (p_parm->size.channel > p_input_info->img_ch->channel) || (p_parm->in_ofs.line_ofs > p_input_info->img_ch->line_ofs)) {
					DBG_ERR("input size(%d, %d, %d, %d) < model first layer size(%d, %d, %d, %d)\r\n"
							, (int)p_input_info->img_ch->width, (int)p_input_info->img_ch->height
							, (int)p_input_info->img_ch->channel, (int)p_input_info->img_ch->line_ofs
							, (int)p_parm->size.width, (int)p_parm->size.height, (int)p_parm->size.channel, (int)p_parm->in_ofs.line_ofs);
					return HD_ERR_ABORT;
				}
			}
			break;
		case AI_MODE_PREPROC: {
				KDRV_AI_PREPROC_PARM *p_parm = (KDRV_AI_PREPROC_PARM *)p_app_head->parm_addr;
#if (NEW_AI_FLOW == 1)
                //calculate ideal cycle
                p_1st_parm->idea_cycle = p_input_info->img_ch->width * p_input_info->img_ch->height;
#endif
				p_parm->in_size.width = p_input_info->img_ch->width;
				p_parm->in_size.height = p_input_info->img_ch->height;
				switch (p_input_info->img_ch->fmt) {
				case HD_VIDEO_PXLFMT_YUV420:
#if AI_SUPPORT_MULTI_FMT
#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
					p_parm->src_fmt = AI_PREPROC_SRC_YUV420_NV21;
#else
					p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
#endif
#else
					p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
					p_parm->in_ofs[0].line_ofs = p_input_info->img_ch[0]->width;
                    p_parm->in_ofs[1].line_ofs = p_input_info->img_ch[1]->width;
#endif
					p_parm->in_ofs[0].line_ofs = p_input_info->img_ch[0].line_ofs;
                    p_parm->in_ofs[1].line_ofs = p_input_info->img_ch[1].line_ofs;
                    //p_parm->in_ofs[2].line_ofs = 0;
					break;
				/*case HD_VIDEO_PXLFMT_YUV422_ONE:
					p_parm->in_fmt = NET_IMG_YUV422;
					break;*/
				case HD_VIDEO_PXLFMT_Y8:
					p_parm->src_fmt = AI_PREPROC_SRC_YONLY;
					p_parm->in_ofs[0].line_ofs = p_input_info->img_ch[0].line_ofs;
					break;
				case HD_VIDEO_PXLFMT_RGB888_PLANAR:
					p_parm->src_fmt = AI_PREPROC_SRC_RGB;
					p_parm->in_ofs[0].line_ofs = p_input_info->img_ch[0].line_ofs;
                    p_parm->in_ofs[1].line_ofs = p_input_info->img_ch[1].line_ofs;
                    p_parm->in_ofs[2].line_ofs = p_input_info->img_ch[2].line_ofs;
					break;
				default:
					DBG_WRN("not support input format: 0x%08x\r\n", p_input_info->img_ch->fmt);
					p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
					break;
				}
			}
			break;
		case AI_MODE_ROIPOOL:
		case AI_MODE_SVM:
		case AI_MODE_FC:
		case AI_MODE_PERMUTE:
		case AI_MODE_REORG:
		default:
			DBG_ERR("unknown input mode(%d) in app\r\n", (int)nn_mode);
			break;
		}

	} else if (trigsrc == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
		//========== for first layer linked list mode ==========
		VENDOR_AI_LL_HEAD *p_head = (VENDOR_AI_LL_HEAD *)p_1st_parm->addr;

		switch (p_head->eng) {
		case AI_ENG_CNN:
		case AI_ENG_CNN2:
		case AI_ENG_NUE:
		break;
		case AI_ENG_NUE2: {
			NUE2_LL_PARM *p_parm = (NUE2_LL_PARM*)p_head->parm_addr;
#if (NEW_AI_FLOW == 1)
            //calculate ideal cycle
            p_1st_parm->idea_cycle = p_input_info->img_ch->width * p_input_info->img_ch->height;
#endif
			p_parm->size0.bit.width = p_input_info->img_ch->width;
			p_parm->size0.bit.height = p_input_info->img_ch->height;

			if (p_input_info->img_ch->width < p_parm->scale_size.bit.h_scl_size) {
				p_parm->scale0.bit.h_rate = 0; // scale up case should set 0
			} else {
				p_parm->scale0.bit.h_rate = (((p_input_info->img_ch->width-1)/(p_parm->scale_size.bit.h_scl_size-1))-1);
			}

			if (p_input_info->img_ch->height < p_parm->scale_size.bit.v_scl_size) {
				p_parm->scale0.bit.v_rate = 0; // scale up case should set 0
			} else {
				p_parm->scale0.bit.v_rate = (((p_input_info->img_ch->height-1)/(p_parm->scale_size.bit.v_scl_size-1))-1);
			}

			p_parm->scale1.bit.h_sfact = (((p_input_info->img_ch->width-1)*65536/(p_parm->scale_size.bit.h_scl_size-1)) & 0xffff);
			p_parm->scale1.bit.v_sfact = (((p_input_info->img_ch->height-1)*65536/(p_parm->scale_size.bit.v_scl_size-1)) & 0xffff);
			switch ((UINT32)p_input_info->img_ch->fmt) {
			case HD_VIDEO_PXLFMT_YUV420:
#if AI_SUPPORT_MULTI_FMT
#if defined(_BSP_NA51068_) || defined(_BSP_NA51090_)
				// force to assign fmt in ll cmd (for 32x YUV420_NV21)
				{
					UINT64* tmp_cmd = (UINT64*)p_head->parm_addr;
					UINT32 tmp_idx = 0;
					// search the register which contain yuv_mode register
					while (1) {
						if ((tmp_cmd[tmp_idx] >> 63) == 1) {
							if (((tmp_cmd[tmp_idx] >> 32) & 0xFF) == 0x04) {
								// modify the format
								UINT32* p_reg_val = (UINT32*)(&tmp_cmd[tmp_idx]);
								//if (p_input_info->fmt_type) { // Not support dynamic setting
								if (1) {
									p_reg_val[0] = p_reg_val[0] | 0x200000;
								} else {
									p_reg_val[0] = p_reg_val[0] & 0xFFFDFFFFF;
								}
								break;
							} else {
								// find next cmd
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
				p_parm->ilofs[0].bit.ofs = p_input_info->img_ch[0].line_ofs;
				p_parm->ilofs[1].bit.ofs = p_input_info->img_ch[1].line_ofs;
				//p_parm->in_ofs[2].line_ofs = 0;
				break;
			/*case HD_VIDEO_PXLFMT_YUV422_ONE:
				p_parm->in_fmt = NET_IMG_YUV422;
				break;*/
			case HD_VIDEO_PXLFMT_Y8:
				//p_parm->src_fmt = AI_PREPROC_SRC_YONLY;
				p_parm->ilofs[0].bit.ofs = p_input_info->img_ch[0].line_ofs;
				break;
			case HD_VIDEO_PXLFMT_RGB888_PLANAR:
			case HD_VIDEO_PXLFMT_BGR888_PLANAR: // AI_Tool will handle the order of SAI[]
				//p_parm->src_fmt = AI_PREPROC_SRC_RGB;
				p_parm->ilofs[0].bit.ofs = p_input_info->img_ch[0].line_ofs;
				p_parm->ilofs[1].bit.ofs = p_input_info->img_ch[1].line_ofs;
				p_parm->ilofs[2].bit.ofs = p_input_info->img_ch[2].line_ofs;
				break;
			default:
				DBG_WRN("not support input format: 0x%08x\r\n", p_input_info->img_ch->fmt);
				//p_parm->src_fmt = AI_PREPROC_SRC_YUV420;
				break;
			}
		}
		break;
		default:
			DBG_ERR("unknown engine type : %d\r\n", (int)p_head->eng);
			break;
		}
		//========== by CCC 191004 ==========
#else
		DBG_ERR("first layer can't be linked list\r\n");
		return HD_ERR_ABORT;
#endif

	} else if (trigsrc == NN_GEN_TRIG_COMMON) {
		//const UINT32 parm_addr = p_1st_parm->addr;
		switch (nn_mode) {
		case NN_PREPROC: {
				/*NN_PRE_PARM *p_parm = (NN_PRE_PARM *)parm_addr;
				p_parm->width = p_input_info->width;
				p_parm->height = p_input_info->height;
				p_parm->in_lofs = p_input_info->line_ofs;
				switch (p_input_info->fmt) {
				case HD_VIDEO_PXLFMT_YUV420:
					p_parm->in_fmt = NET_IMG_YUV420;
					break;
				case HD_VIDEO_PXLFMT_YUV422_ONE:
					p_parm->in_fmt = NET_IMG_YUV422;
					break;
				case HD_VIDEO_PXLFMT_Y8:
					p_parm->in_fmt = NET_IMG_YONLY;
					break;
				case HD_VIDEO_PXLFMT_RGB888_PLANAR:
					p_parm->in_fmt = NET_IMG_RGB_PLANE;
					break;
				default:
					DBG_WRN("not support input format: 0x%08x\r\n", p_input_info->fmt);
					p_parm->in_fmt = NET_IMG_YUV420;
					break;
				}*/
			}
			break;
		case NN_POSTPROC:
		case NN_SOFTMAX:
		case NN_FC_POST:
		case NN_POOL:
		case NN_CUSTOMER:
		case NN_BNSCALE:
		default:
			DBG_ERR("unknown input mode(%d) in common\r\n", (int)nn_mode);
			break;
		}

	} else {
		DBG_ERR("unknown first layer trigger source: %d\r\n", (int)trigsrc);
		return HD_ERR_ABORT;
	}

	return HD_OK;
}


#define DISABLE_MMAP 1
#if CNN_25_MATLAB
HD_RESULT vendor_ais_create_1st_layer_addr(VENDOR_AIS_IMG_PARM *p_input_info, NN_IOMEM *p_1st_mem, NN_GEN_MODE_CTRL *p_1st_parm)
{
	NN_GEN_TRIG_SRC trigsrc;
	NN_MODE nn_mode;
	NN_IOMEM *p_iomem = p_1st_mem;

	memset(p_iomem, 0, sizeof(NN_IOMEM));
	if ((p_input_info == NULL) || (p_1st_mem == NULL) || (p_1st_parm == NULL)) {
		DBG_ERR("null input ...r\n");
		return HD_ERR_BAD_DATA;
	}
	nn_mode = p_1st_parm->mode;
	trigsrc = p_1st_parm->trig_src;
#if DISABLE_MMAP
	if (p_input_info->pa == 0) {
		DBG_ERR("input source pa = NULL???\n");
		return HD_ERR_BAD_DATA;
	}
	// p_input_info->va is optional, only used if layer 0 is CPU layer
	if ((p_1st_parm->eng == NN_GEN_ENG_CPU) && (p_input_info->va == 0)) {
		DBG_ERR("input source va = NULL??? (layer 0 is CPU layer)!\n");
		return HD_ERR_BAD_DATA;
	}
#endif
	if (trigsrc == NN_GEN_TRIG_APP_AI_DRV || trigsrc == NN_GEN_TRIG_LL_AI_DRV) {
		const UINT32 img_size = p_input_info->line_ofs * p_input_info->height;
		p_iomem->SAI[0].pa      = p_input_info->pa;
		if (nn_mode != NN_PREPROC) {
			p_iomem->SAI[0].size = p_input_info->line_ofs * p_input_info->height * p_input_info->channel;
#if DISABLE_MMAP
			p_iomem->SAI[0].va = p_input_info->va;
#else
			p_iomem->SAI[0].va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_iomem->SAI[0].pa, p_iomem->SAI[0].size);
#endif
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_RGB888_PLANAR) {
			//p_iomem->SAI[0].size = img_size * 3;
			p_iomem->SAI[0].size = p_input_info->line_ofs * p_input_info->height;
			p_iomem->SAI[1].size = p_input_info->line_ofs * p_input_info->height;
			p_iomem->SAI[2].size = p_input_info->line_ofs * p_input_info->height;
			p_iomem->SAI[1].pa   = p_iomem->SAI[0].pa + (p_iomem->SAI[0].size);
			p_iomem->SAI[2].pa   = p_iomem->SAI[1].pa + (p_iomem->SAI[1].size);
#if DISABLE_MMAP
			p_iomem->SAI[0].va = p_input_info->va;
#else
			p_iomem->SAI[0].va   = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_iomem->SAI[0].pa, p_iomem->SAI[0].size + p_iomem->SAI[1].size + p_iomem->SAI[2].size);
#endif
			p_iomem->SAI[1].va	 = p_iomem->SAI[0].va + p_iomem->SAI[0].size;
            p_iomem->SAI[2].va	 = p_iomem->SAI[1].va + p_iomem->SAI[1].size;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_YUV420) {
			p_iomem->SAI[0].size = p_input_info->line_ofs * p_input_info->height;
			p_iomem->SAI[1].size = (p_input_info->line_ofs * p_input_info->height) >> 1;
			p_iomem->SAI[1].pa   = p_iomem->SAI[0].pa + (p_iomem->SAI[0].size);
#if DISABLE_MMAP
			p_iomem->SAI[0].va = p_input_info->va;
#else
			p_iomem->SAI[0].va   = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_iomem->SAI[0].pa, p_iomem->SAI[0].size + p_iomem->SAI[1].size);
#endif
			p_iomem->SAI[1].va	 = p_iomem->SAI[0].va + p_iomem->SAI[0].size;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_YUV422_ONE) {
			//p_iomem->SAI[0].size = img_size * 2;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_Y8) {
			//p_iomem->SAI[0].size = img_size;
			p_iomem->SAI[0].size = p_input_info->line_ofs * p_input_info->height * p_input_info->channel;
#if DISABLE_MMAP
			p_iomem->SAI[0].va = p_input_info->va;
#else
			p_iomem->SAI[0].va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_iomem->SAI[0].pa, p_iomem->SAI[0].size);
#endif
		} else {
			p_iomem->SAI[0].size = img_size;
#if DISABLE_MMAP
			p_iomem->SAI[0].va = p_input_info->va;
#else
			p_iomem->SAI[0].va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_iomem->SAI[0].pa, p_iomem->SAI[0].size);
#endif
		}
	} else if (trigsrc == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
//========== for first layer linked list mode ==========
		p_iomem->SAI[0].pa = p_input_info->pa;
		p_iomem->SAI[0].size = p_input_info->line_ofs * p_input_info->height * p_input_info->channel;
//========== by CCC 191004 ==========
#else
		DBG_ERR("first layer can't be linked list\r\n");
		return HD_ERR_ABORT;
#endif

	} else if (trigsrc == NN_GEN_TRIG_COMMON) {
		const UINT32 img_size = p_input_info->line_ofs * p_input_info->height;
		p_iomem->SAI[0].pa = p_input_info->pa;
		if (nn_mode != NN_PREPROC) {
			p_iomem->SAI[0].size = img_size * p_input_info->channel;
		}/* else if (p_input_info->fmt == HD_VIDEO_PXLFMT_RGB888_PLANAR) {
			p_iomem->SAI[0].size = img_size * 3;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_YUV420) {
			p_iomem->SAI[0].size = (img_size * 3 + 1) >> 1;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_YUV422_ONE) {
			p_iomem->SAI[0].size = img_size * 2;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_Y8) {
			p_iomem->SAI[0].size = img_size;
		} else {
			p_iomem->SAI[0].size = img_size;
		}*/
#if DISABLE_MMAP
		p_iomem->SAI[0].va = p_input_info->va;
#else
		p_iomem->SAI[0].va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_iomem->SAI[0].pa, p_iomem->SAI[0].size);
#endif
	} else {
		DBG_ERR("unknown first layer trigger source: %d\r\n", (int)trigsrc);
		return HD_ERR_ABORT;
	}

#if 0
#else
	//p_iomem->SAI[0].va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_iomem->SAI[0].pa, p_iomem->SAI[0].size);
#endif

	return HD_OK;
}

HD_RESULT vendor_ais_destory_1st_layer_addr(NN_IOMEM *p_1st_mem)
{
#if (DISABLE_MMAP == 0)
	const UINT32 io_mem_cnt = sizeof(NN_IOMEM) / sizeof(NN_DATA);
	UINT32 idx;
	NN_DATA *p_data = (NN_DATA *)p_1st_mem;
#if 1
	if (p_data[2].va > 0 && p_data[0].va + p_data[0].size + p_data[1].size == p_data[2].va) {
		if ((p_data[0].size != 0) && (p_data[0].va != 0)) {
			HD_RESULT ret = hd_common_mem_munmap((void *)p_data[0].va, p_data[0].size+p_data[1].size+p_data[2].size);
			if (ret != HD_OK) {
				DBG_ERR("memory uninit fail : %x\r\n", ret);
			}
		}
	} else {


		if ((p_data[0].size != 0) && (p_data[0].va != 0)) {


			HD_RESULT ret = hd_common_mem_munmap((void *)p_data[0].va, p_data[0].size+p_data[1].size);
			if (ret != HD_OK) {
				DBG_ERR("memory uninit fail : %x\r\n", ret);
			}
		}
		
		for (idx = 2; idx < io_mem_cnt; idx++) {
			if ((p_data[idx].size != 0) && (p_data[idx].va != 0)) {
#if 0
#else
				HD_RESULT ret = hd_common_mem_munmap((void *)p_data[idx].va, p_data[idx].size);
				if (ret != HD_OK) {
					DBG_ERR("memory uninit fail : %x\r\n", ret);
				}
#endif
			}
		}
	}
		
#else
	for (idx = 0; idx < io_mem_cnt; idx++) {
		if ((p_data[idx].size != 0) && (p_data[idx].va != 0)) {
#if 0
#else
			HD_RESULT ret = hd_common_mem_munmap((void *)p_data[idx].va, p_data[idx].size);
			if (ret != HD_OK) {
				DBG_ERR("memory[%d] uninit fail : %x\r\n", (int)idx, ret);
			}
#endif
		}
	}
#endif
#endif
	return HD_OK;
}

HD_RESULT vendor_ais_net_input_init(VENDOR_AIS_IMG_PARM *p_input_info, UINT32 net_id)
{
	NN_GEN_MODE_CTRL *p_1st_mctrl;
	NN_MODE nn_mode;
	const UINT32 first_layer = 0;
	NN_IOMEM *p_1st_iomem = &g_fist_layer_mem[net_id];
	VENDOR_AIS_FLOW_MEM_PARM *p_mem_parm = &g_map_mem[net_id].user_parm;
	
	UINT64 ts, ts_new;
	UINT64 ts_inputaddr, ts_inputpara, ts_updateaddr, ts_updatepara;

	memset(p_1st_iomem, 0, sizeof(NN_IOMEM));
	p_1st_mctrl = vendor_ais_get_proclayer(first_layer, *p_mem_parm);
	if (p_1st_mctrl == NULL) {
		DBG_ERR("vendor_ais_get_proclayer fail...");
		return HD_ERR_BAD_DATA;
	}
	nn_mode = p_1st_mctrl->mode;

	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
		ts = hd_gettime_us();
	}
	///< get first layer input address from streaming
	vendor_ais_create_1st_layer_addr(p_input_info, p_1st_iomem, p_1st_mctrl);
	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_inputaddr = ts_new-ts;
		vendor_ai_net_trace(net_id, AI_BUF|AI_PERF, "set() - >>> inputaddr fill time = %llu\r\n", ts_inputaddr);
	}

	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts = ts_new;
	}
	///< update first layer parameters based on streaming
	vendor_ais_update_1st_layer_parm(p_input_info, p_1st_mctrl);
	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_inputpara = ts_new-ts;
		vendor_ai_net_trace(net_id, AI_BUF|AI_PERF, "set() - >>> inputpara fill time = %llu\r\n", ts_inputpara);
	}

	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts = ts_new;
	}
	///< update first layer input address in user & kernel space
	vendor_ais_proc_input_init(nn_mode, p_1st_iomem, *p_mem_parm, net_id);
	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_updateaddr = ts_new-ts;
		vendor_ai_net_trace(net_id, AI_BUF|AI_PERF, "set() - >>> updateaddr ker time = %llu\r\n", ts_updateaddr);
	}

	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts = ts_new;
	}
	///< update first layer parameters in kernel space
	vendor_ais_update_proclayer(first_layer, net_id);
	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_updatepara = ts_new-ts;
		vendor_ai_net_trace(net_id, AI_BUF|AI_PERF, "set() - >>> updatepara ker time = %llu\r\n", ts_updatepara);
	}

	return HD_OK;
}

HD_RESULT vendor_ais_net_input_uninit(UINT32 net_id)
{
	NN_IOMEM *p_1st_iomem = &g_fist_layer_mem[net_id];

	UINT64 ts, ts_new;
	UINT64 ts_inputaddr, ts_inputpara;

	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
		ts = hd_gettime_us();
	}
	///< release virtual address of streaming
	vendor_ais_destory_1st_layer_addr(p_1st_iomem);
	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_inputaddr = ts_new-ts;
		vendor_ai_net_trace(net_id, AI_BUF|AI_PERF, "proc() - >>> inputaddr clr time = %llu\r\n", ts_inputaddr);
	}

	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts = ts_new;
	}
	///< restore first layer input address in user & kernel space
	vendor_ais_proc_input_uninit(p_1st_iomem, g_map_mem[net_id].user_parm, net_id);
	if (vendor_ai_net_get_trace(net_id) & AI_PERF) {
	    ts_new = hd_gettime_us();
		ts_inputpara = ts_new-ts;
		vendor_ai_net_trace(net_id, AI_BUF|AI_PERF, "proc() - >>> inputpara clr time = %llu\r\n", ts_inputpara);
	}

	return HD_OK;
}

#else
HD_RESULT _vendor_ais_net_check_multi_input(UINT32 proc_id)
{
	HD_RESULT rv = HD_OK;
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(proc_id);
	VENDOR_AIS_FLOW_MEM_PARM *p_mem;
	NN_GEN_NET_INFO net_info = {0};
	NN_GEN_MODEL_HEAD *p_head;
	VENDOR_AI_NET_CFG_PROC *p_cfg = NULL;
	UINT32 dst_tag = (UINT32)((UINT32)('I') | ((UINT32)('L')<<8) | ((UINT32)('I')<<16) | ((UINT32)('F')<<24));
	UINT32 tmp_size = 0;
	UINT32 *p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 mctrl_id;
	FLOAT batch_ratio = 1;
	
	p_cfg = _vendor_ai_cfg(proc_id);

	p_mem = &p_proc->mem_manager.user_parm;
	rv = vendor_ais_get_net_info(&net_info, p_mem->va);
	if (rv != HD_OK) {
		DBG_ERR("vendor_ais_get_net_info fail...\r\n");
		return rv;
	}

	p_head = net_info.p_head;
	p_ex_head = (UINT32*)(p_mem->va + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((uintptr_t)p_ex_head) + tmp_size);
		if (p_tmp_head[1] == dst_tag) {
			UINT32 *p_input_blob_head = (UINT32 *)(((uintptr_t)p_tmp_head) + 2*sizeof(UINT32));
			UINT32 input_blob_ofs = 0;
			UINT32 input_blob_num = 0;
			UINT32 input_blob_io_num = 0;
			UINT32 *p_input_blob_info = NULL;
			UINT32 i; // means input_blob_idx
			UINT32 j; // means input_io_idx of each blob
			UINT32 k; // means input table idx that user set

			// get input blob num
			input_blob_num = p_input_blob_head[0];

			for (i = 0; i < input_blob_num; i++) {
				// get input blob info
				input_blob_io_num = p_input_blob_head[2*(i+1) - 1];
				input_blob_ofs = p_input_blob_head[2*(i+1)];

				if (p_cfg->batch_info.enable) {
					if(i == 0) {
						batch_ratio = (FLOAT)input_blob_io_num / p_cfg->batch_info.num;
						input_blob_io_num = p_cfg->batch_info.num;
					} else {
						input_blob_io_num = (UINT32)(AI_ROUND((FLOAT)input_blob_io_num / batch_ratio));
					}
					
				}
				//DBG_DUMP("i(%lu) input_blob_io_num(0x%lx) input_blob_ofs(0x%lx)\n", i, input_blob_io_num, input_blob_ofs);
				p_input_blob_info = (UINT32 *)(((uintptr_t)p_input_blob_head) + input_blob_ofs);
				if (p_input_blob_info == NULL) {
					DBG_ERR("p_input_blob_info is NULL\r\n");
					return HD_ERR_NG;
				}

				// config in_path_list
				for (j = 0; j < input_blob_io_num; j++) {
					//DBG_DUMP("i(%lu) j(%lu) list_cnt(%lu)\n", i, j, list_cnt);
					mctrl_id = (p_input_blob_info[j] & 0xFFFFFF00)>>8; // higher 24-bits
					for (k = 0; k < g_proc_idx_map_index[proc_id]; k++) {
						if(mctrl_id == g_proc_idx_map_table[proc_id][k])
							break;
					}
					if(k == g_proc_idx_map_index[proc_id]) {
						UINT32 port_id = p_input_blob_info[j] & 0xFF;  // lower 8-bits
						DBG_ERR("proc_id(%lu) multi-input is not ready, in_path(0x%lx) is not set!!\r\n", proc_id, VENDOR_AI_NET_PARAM_IN(mctrl_id, port_id));
						return HD_ERR_NG;
					}
				}
			}
			return HD_OK;
		} else {
			if (p_tmp_head[0] == 0) {
				break;
			}
			tmp_size += p_tmp_head[0];
		}
	}

	DBG_ERR("get input blob info fail...\n");

	return HD_ERR_NG;
}

HD_RESULT vendor_ais_create_1st_layer_addr(VENDOR_AIS_IMG_PARM *p_input_info, NN_DATA *p_1st_imem, NN_GEN_MODE_CTRL *p_1st_parm)
{
	NN_GEN_TRIG_SRC trigsrc;
	NN_MODE nn_mode;
	NN_DATA *p_sai = p_1st_imem;

	if ((p_input_info == NULL) || (p_1st_imem == NULL) || (p_1st_parm == NULL)) {
		DBG_ERR("null input ...\r\n");
		return HD_ERR_BAD_DATA;
	}
	nn_mode = p_1st_parm->mode;
	trigsrc = p_1st_parm->trig_src;
	
	if (p_input_info->pa == 0) {
		DBG_ERR("input source pa = NULL???\n");
		return HD_ERR_BAD_DATA;
	}
	// p_input_info->va is optional, only used if layer 0 is CPU layer
	if ((p_1st_parm->eng == NN_GEN_ENG_CPU) && (p_input_info->va == 0)) {
		DBG_ERR("input source va = NULL??? (layer 0 is CPU layer)!\n");
		return HD_ERR_BAD_DATA;
	}

	if (trigsrc == NN_GEN_TRIG_APP_AI_DRV || trigsrc == NN_GEN_TRIG_LL_AI_DRV) {
		const UINT32 img_size = p_input_info->line_ofs * p_input_info->height;
		p_sai[BUF_IN_IDX].pa      = p_input_info->pa;
		if (nn_mode != NN_PREPROC) {
			p_sai[BUF_IN_IDX].size = p_input_info->line_ofs * p_input_info->height * p_input_info->channel;
			p_sai[BUF_IN_IDX].va = p_input_info->va;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_RGB888_PLANAR || p_input_info->fmt == HD_VIDEO_PXLFMT_BGR888_PLANAR) { // AI_Tool will handle the order of SAI[]
			//p_sai[BUF_IN_IDX].size = img_size * 3;
			p_sai[BUF_IN_IDX].size = p_input_info->line_ofs * p_input_info->height;
			p_sai[NUE2_IN_IDX1].size = p_input_info->line_ofs * p_input_info->height;
			p_sai[NUE2_IN_IDX2].size = p_input_info->line_ofs * p_input_info->height;
			p_sai[NUE2_IN_IDX1].pa   = p_sai[BUF_IN_IDX].pa + (p_sai[BUF_IN_IDX].size);
			p_sai[NUE2_IN_IDX2].pa   = p_sai[NUE2_IN_IDX1].pa + (p_sai[NUE2_IN_IDX1].size);
			p_sai[BUF_IN_IDX].va = p_input_info->va;
			p_sai[NUE2_IN_IDX1].va	 = p_sai[BUF_IN_IDX].va + p_sai[BUF_IN_IDX].size;
            p_sai[NUE2_IN_IDX2].va	 = p_sai[NUE2_IN_IDX1].va + p_sai[NUE2_IN_IDX1].size;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_YUV420) {
			p_sai[BUF_IN_IDX].size = p_input_info->line_ofs * p_input_info->height;
			p_sai[NUE2_IN_IDX1].size = (p_input_info->line_ofs * p_input_info->height) >> 1;
			p_sai[NUE2_IN_IDX1].pa   = p_sai[BUF_IN_IDX].pa + (p_sai[BUF_IN_IDX].size);
			p_sai[BUF_IN_IDX].va = p_input_info->va;
			p_sai[NUE2_IN_IDX1].va	 = p_sai[BUF_IN_IDX].va + p_sai[BUF_IN_IDX].size;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_YUV422_ONE) {
			//p_sai[BUF_IN_IDX].size = img_size * 2;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_Y8) {
			//p_sai[BUF_IN_IDX].size = img_size;
			p_sai[BUF_IN_IDX].size = p_input_info->line_ofs * p_input_info->height * p_input_info->channel;
			p_sai[BUF_IN_IDX].va = p_input_info->va;
		} else {
			p_sai[BUF_IN_IDX].size = img_size;
			p_sai[BUF_IN_IDX].va = p_input_info->va;
		}
	} else if (trigsrc == NN_GEN_TRIG_LL_AI_DRV) {
#if LL_SUPPORT_FIRST_LAYER
//========== for first layer linked list mode ==========
		p_sai[BUF_IN_IDX].pa = p_input_info->pa;
		p_sai[BUF_IN_IDX].size = p_input_info->line_ofs * p_input_info->height * p_input_info->channel;
//========== by CCC 191004 ==========
#else
		DBG_ERR("first layer can't be linked list\r\n");
		return HD_ERR_ABORT;
#endif

	} else if (trigsrc == NN_GEN_TRIG_COMMON) {
		const UINT32 img_size = p_input_info->line_ofs * p_input_info->height;
		p_sai[BUF_IN_IDX].pa = p_input_info->pa;
		if (nn_mode != NN_PREPROC) {
			p_sai[BUF_IN_IDX].size = img_size * p_input_info->channel;
		}/* else if (p_input_info->fmt == HD_VIDEO_PXLFMT_RGB888_PLANAR) {
			p_sai[BUF_IN_IDX].size = img_size * 3;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_YUV420) {
			p_sai[BUF_IN_IDX].size = (img_size * 3 + 1) >> 1;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_YUV422_ONE) {
			p_sai[BUF_IN_IDX].size = img_size * 2;
		} else if (p_input_info->fmt == HD_VIDEO_PXLFMT_Y8) {
			p_sai[BUF_IN_IDX].size = img_size;
		} else {
			p_sai[BUF_IN_IDX].size = img_size;
		}*/

		p_sai[BUF_IN_IDX].va = p_input_info->va;

	} else {
		DBG_ERR("unknown first layer trigger source: %d\r\n", (int)trigsrc);
		return HD_ERR_ABORT;
	}

#if 0
#else
	//p_iomem->SAI[0].va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_iomem->SAI[0].pa, p_iomem->SAI[0].size);
#endif

	return HD_OK;
}

HD_RESULT vendor_ais_create_1st_layer_addr_v2(VENDOR_AIS_IMG_PARM_V2 *p_input_info, NN_DATA *p_1st_imem, NN_GEN_MODE_CTRL *p_1st_parm)
{
	NN_GEN_TRIG_SRC trigsrc;
	NN_MODE nn_mode;
	NN_DATA *p_sai = p_1st_imem;

	if ((p_input_info == NULL) || (p_1st_imem == NULL) || (p_1st_parm == NULL)) {
		DBG_ERR("null input ...\r\n");
		return HD_ERR_BAD_DATA;
	}
	nn_mode = p_1st_parm->mode;
	trigsrc = p_1st_parm->trig_src;

	if (p_input_info->img_ch[0].pa == 0) {
		DBG_ERR("input source pa = NULL???\n");
		return HD_ERR_BAD_DATA;
	}
	// p_input_info->va is optional, only used if layer 0 is CPU layer
	if ((p_1st_parm->eng == NN_GEN_ENG_CPU) && (p_input_info->img_ch[0].va == 0)) {
		DBG_ERR("input source va = NULL??? (layer 0 is CPU layer)!\n");
		return HD_ERR_BAD_DATA;
	}

	if (trigsrc == NN_GEN_TRIG_APP_AI_DRV || trigsrc == NN_GEN_TRIG_LL_AI_DRV) {
		const UINT32 img_size = p_input_info->img_ch->line_ofs * p_input_info->img_ch->height;
		p_sai[BUF_IN_IDX].pa      = p_input_info->img_ch[0].pa;
		if (nn_mode != NN_PREPROC) {
			p_sai[BUF_IN_IDX].size = p_input_info->img_ch->line_ofs * p_input_info->img_ch->height * p_input_info->img_ch->channel;
			p_sai[BUF_IN_IDX].va = p_input_info->img_ch->va;
		} else if (p_input_info->img_ch->fmt == HD_VIDEO_PXLFMT_RGB888_PLANAR || p_input_info->img_ch->fmt == HD_VIDEO_PXLFMT_BGR888_PLANAR) { // AI_Tool will handle the order of SAI[]
			//p_sai[BUF_IN_IDX].size = img_size * 3;
			p_sai[BUF_IN_IDX].size = p_input_info->img_ch[0].line_ofs * p_input_info->img_ch[0].height;
			p_sai[NUE2_IN_IDX1].size = p_input_info->img_ch[1].line_ofs * p_input_info->img_ch[1].height;
			p_sai[NUE2_IN_IDX2].size = p_input_info->img_ch[2].line_ofs * p_input_info->img_ch[2].height;
			p_sai[NUE2_IN_IDX1].pa   = p_input_info->img_ch[1].pa;
			p_sai[NUE2_IN_IDX2].pa   = p_input_info->img_ch[2].pa;
			p_sai[BUF_IN_IDX].va = p_input_info->img_ch[0].va;
			p_sai[NUE2_IN_IDX1].va	 = p_input_info->img_ch[1].va;
			p_sai[NUE2_IN_IDX2].va	 = p_input_info->img_ch[2].va;
		} else if (p_input_info->img_ch->fmt == HD_VIDEO_PXLFMT_YUV420) {
			p_sai[BUF_IN_IDX].size = p_input_info->img_ch[0].line_ofs * p_input_info->img_ch[0].height;
			p_sai[NUE2_IN_IDX1].size = (p_input_info->img_ch[1].line_ofs * p_input_info->img_ch[1].height) >> 1;
			p_sai[NUE2_IN_IDX1].pa   = p_input_info->img_ch[1].pa;
			p_sai[BUF_IN_IDX].va     = p_input_info->img_ch[0].va;
			p_sai[NUE2_IN_IDX1].va	 = p_input_info->img_ch[1].va;
		} else if (p_input_info->img_ch->fmt == HD_VIDEO_PXLFMT_YUV422_ONE) {
			//p_sai[BUF_IN_IDX].size = img_size * 2;
		} else if (p_input_info->img_ch->fmt == HD_VIDEO_PXLFMT_Y8) {
			//p_sai[BUF_IN_IDX].size = img_size;
			p_sai[BUF_IN_IDX].size = p_input_info->img_ch->line_ofs * p_input_info->img_ch->height * p_input_info->img_ch->channel;
			p_sai[BUF_IN_IDX].va = p_input_info->img_ch->va;
		} else {
			p_sai[BUF_IN_IDX].size = img_size;
			p_sai[BUF_IN_IDX].va = p_input_info->img_ch->va;
		}
	} else if (trigsrc == NN_GEN_TRIG_COMMON) {
		const UINT32 img_size = p_input_info->img_ch->line_ofs * p_input_info->img_ch->height;
		p_sai[BUF_IN_IDX].pa = p_input_info->img_ch->pa;
		if (nn_mode != NN_PREPROC) {
			p_sai[BUF_IN_IDX].size = img_size * p_input_info->img_ch->channel;
		}

		p_sai[BUF_IN_IDX].va = p_input_info->img_ch->va;

	} else {
		DBG_ERR("unknown first layer trigger source: %d\r\n", (int)trigsrc);
		return HD_ERR_ABORT;
	}

#if 0
#else
	//p_iomem->SAI[0].va = (UINT32)hd_common_mem_mmap(HD_COMMON_MEM_MEM_TYPE_CACHE, p_iomem->SAI[0].pa, p_iomem->SAI[0].size);
#endif

	return HD_OK;
}


HD_RESULT vendor_ais_release_1st_layer_addr(NN_DATA *p_1st_imem)
{
	return HD_OK;
}

HD_RESULT vendor_ais_net_input_init(VENDOR_AIS_IMG_PARM *p_input_info, UINT32 net_id, UINT32 layer_id)
{
	NN_GEN_MODE_CTRL *p_1st_mctrl;
	NN_MODE nn_mode;
#if CNN_MULTI_INPUT
	UINT32 proc_idx;
#else
	const UINT32 first_layer = 0;
#endif
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
#if (NET_GEN_HEAP == 1)
	NN_DATA *p_1st_imem = p_proc->first_layer_imem;
#else
	NN_DATA *p_1st_imem = g_fist_layer_imem[net_id];
#endif
#if (NET_GEN_HEAP == 1)
	VENDOR_AIS_FLOW_MEM_PARM *p_mem_parm = &(p_proc->map_mem.user_parm);
#else
	VENDOR_AIS_FLOW_MEM_PARM *p_mem_parm = &g_map_mem[net_id].user_parm;
#endif
	UINT32 in_buf_num = p_proc->priv.src_layer.in_buf_num;

#if CNN_MULTI_INPUT
	// check in_buf_cnt
	if (in_buf_num > 0) {
		//layer_id assigns to proc_idx
		proc_idx = layer_id;
		// update input proc_idx to mapping table
		g_proc_idx_map_table[net_id][g_proc_idx_map_index[net_id]] = layer_id;
		g_proc_idx_map_index[net_id]++;
	} else {
		DBG_ERR("input_init fail, in_buf_num(%lu)\n", in_buf_num);
		return HD_ERR_NG;
	}
#endif

	///< clear
	memset(p_1st_imem, 0, sizeof(NN_DATA) * NN_IMEM_NUM);

	///< get first mode control
#if CNN_MULTI_INPUT
	p_1st_mctrl = vendor_ais_get_proclayer(proc_idx, *p_mem_parm);
#else
	p_1st_mctrl = vendor_ais_get_proclayer(first_layer, *p_mem_parm);
#endif

	if (p_1st_mctrl == NULL) {
		DBG_ERR("vendor_ais_get_proclayer fail...");
		return HD_ERR_BAD_DATA;
	}
	nn_mode = p_1st_mctrl->mode;

	///< get first layer input address from streaming
	vendor_ais_create_1st_layer_addr(p_input_info, p_1st_imem, p_1st_mctrl);

	///< update first layer parameters based on streaming
	vendor_ais_update_1st_layer_parm(p_input_info, p_1st_mctrl);

	///< update first layer input address in user & kernel space
#if CNN_MULTI_INPUT
	vendor_ais_proc_input_init2(nn_mode, p_1st_imem, *p_mem_parm, proc_idx, net_id);
#else
	vendor_ais_proc_input_init(nn_mode, p_1st_imem, *p_mem_parm, net_id);
#endif

	///< update first layer parameters in kernel space
#if CNN_MULTI_INPUT
	vendor_ais_update_proclayer(proc_idx, net_id);
#else
	vendor_ais_update_proclayer(first_layer, net_id);
#endif

	return HD_OK;
}

HD_RESULT vendor_ais_net_input_init_v2(VENDOR_AIS_IMG_PARM_V2 *p_input_info, UINT32 net_id, UINT32 layer_id)
{
	NN_GEN_MODE_CTRL *p_1st_mctrl;
	NN_MODE nn_mode;
#if CNN_MULTI_INPUT
	UINT32 proc_idx;
#else
	const UINT32 first_layer = 0;
#endif
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
#if (NET_GEN_HEAP == 1)
	NN_DATA *p_1st_imem = p_proc->first_layer_imem;
#else
	NN_DATA *p_1st_imem = g_fist_layer_imem[net_id];
#endif
#if (NET_GEN_HEAP == 1)
	VENDOR_AIS_FLOW_MEM_PARM *p_mem_parm = &(p_proc->map_mem.user_parm);
#else
	VENDOR_AIS_FLOW_MEM_PARM *p_mem_parm = &g_map_mem[net_id].user_parm;
#endif
	UINT32 in_buf_num = p_proc->priv.src_layer.in_buf_num;

#if CNN_MULTI_INPUT
	// check in_buf_cnt
	if (in_buf_num > 0) {
		//layer_id assigns to proc_idx
		proc_idx = layer_id;
		// update input proc_idx to mapping table
		g_proc_idx_map_table[net_id][g_proc_idx_map_index[net_id]] = layer_id;
		g_proc_idx_map_index[net_id]++;
	} else {
		DBG_ERR("input_init_v2 fail, in_buf_num(%lu)\n", in_buf_num);
		return HD_ERR_NG;
	}
#endif

	///< clear
	memset(p_1st_imem, 0, sizeof(NN_DATA) * NN_IMEM_NUM);

	///< get first mode control
#if CNN_MULTI_INPUT
	p_1st_mctrl = vendor_ais_get_proclayer(proc_idx, *p_mem_parm);
#else
	p_1st_mctrl = vendor_ais_get_proclayer(first_layer, *p_mem_parm);
#endif

	if (p_1st_mctrl == NULL) {
		DBG_ERR("vendor_ais_get_proclayer fail...");
		return HD_ERR_BAD_DATA;
	}
	nn_mode = p_1st_mctrl->mode;

	///< get first layer input address from streaming
	vendor_ais_create_1st_layer_addr_v2(p_input_info, p_1st_imem, p_1st_mctrl);

	///< update first layer parameters based on streaming
	vendor_ais_update_1st_layer_parm_v2(p_input_info, p_1st_mctrl);

	///< update first layer input address in user & kernel space
#if CNN_MULTI_INPUT
	vendor_ais_proc_input_init2(nn_mode, p_1st_imem, *p_mem_parm, proc_idx, net_id);
#else
	vendor_ais_proc_input_init(nn_mode, p_1st_imem, *p_mem_parm, net_id);
#endif

	///< update first layer parameters in kernel space
#if CNN_MULTI_INPUT
	vendor_ais_update_proclayer(proc_idx, net_id);
#else
	vendor_ais_update_proclayer(first_layer, net_id);
#endif

	return HD_OK;
}


HD_RESULT vendor_ais_net_input_uninit(UINT32 net_id)
{
	VENDOR_AI_NET_INFO_PROC *p_proc = _vendor_ai_info(net_id);
#if (NET_GEN_HEAP == 1)
	NN_DATA *p_1st_imem = p_proc->first_layer_imem;
#else
	NN_DATA *p_1st_imem = g_fist_layer_imem[net_id];
#endif
#if (NET_GEN_HEAP == 1)
	VENDOR_AIS_FLOW_MEM_PARM *p_mem_parm = &(p_proc->map_mem.user_parm);
#else
	VENDOR_AIS_FLOW_MEM_PARM *p_mem_parm = &g_map_mem[net_id].user_parm;
#endif
	UINT32 in_buf_num = p_proc->priv.src_layer.in_buf_num;
	UINT32 i;
#if CNN_MULTI_INPUT
	UINT32 proc_idx = 0;
	VENDOR_AI_NET_CFG_PROC *p_cfg = _vendor_ai_cfg(net_id);
#endif
	
	///< release virtual address of streaming
	vendor_ais_release_1st_layer_addr(p_1st_imem);

	///< restore first layer input address in user & kernel space
#if CNN_MULTI_INPUT
	for (i = 0; i < (in_buf_num - p_proc->priv.src_layer.cnt); i++) {
		if (g_proc_idx_map_index[net_id] == 0) {
			DBG_ERR("net_id(%lu) multi-batch uninit fail, batch num(%ld)\r\n", net_id, p_cfg->batch_info.num);
			return HD_ERR_NG;
		}
		g_proc_idx_map_index[net_id]--;
		proc_idx = g_proc_idx_map_table[net_id][g_proc_idx_map_index[net_id]];
		vendor_ais_proc_input_uninit2(p_1st_imem, (*p_mem_parm), proc_idx, net_id);
	}
#else
	vendor_ais_proc_input_uninit(p_1st_imem, (*p_mem_parm), net_id);
#endif
	
	///< clear
	memset(p_1st_imem, 0, sizeof(NN_DATA) * NN_IMEM_NUM);

	return HD_OK;
}

#endif
HD_RESULT vendor_ais_net_engine_open(VENDOR_AI_ENG engine, UINT32 net_id)
{
	/*
		open engine
	*/
	#if APPLY_OPEN_FIRST
	//vendor_ai_drv_open(engine, net_id);
	#endif
	//vendor_ai_drv_open(AI_ENG_CNN, net_id);
	//vendor_ai_drv_open(AI_ENG_CNN2, net_id);
	//vendor_ai_drv_open(AI_ENG_NUE, net_id);
	//vendor_ai_drv_open(AI_ENG_NUE2, net_id);

	return HD_OK;
}

HD_RESULT vendor_ais_net_engine_close(VENDOR_AI_ENG engine, UINT32 net_id)
{
	/*
		close engine
	*/
	#if APPLY_OPEN_FIRST
	//vendor_ai_drv_close(engine, net_id);
	#endif
	//vendor_ai_drv_close(AI_ENG_CNN, net_id);
	//vendor_ai_drv_close(AI_ENG_CNN2, net_id);
	//vendor_ai_drv_close(AI_ENG_NUE, net_id);
	//vendor_ai_drv_close(AI_ENG_NUE2, net_id);


	return HD_OK;
}

HD_RESULT vendor_ai_net_gen_context_init(VOID)
{
#if (NET_GEN_HEAP == 1)
#else
	UINT i;
#endif

#if (NET_GEN_HEAP == 1)
#else
	g_fist_layer_imem = (NN_DATA **)vendor_ai_malloc(sizeof(NN_DATA) * g_ai_support_net_max);
	if (g_fist_layer_imem == NULL) {
		return HD_ERR_NOMEM;
	}
	memset(g_fist_layer_imem, 0x0, sizeof(NN_DATA) * g_ai_support_net_max);
	for (i = 0; i < g_ai_support_net_max; i++) {
		g_fist_layer_imem[i] = (NN_DATA *)vendor_ai_malloc(sizeof(NN_DATA) * NN_IMEM_NUM);
		if (g_fist_layer_imem[i] == NULL) {
			return HD_ERR_NOMEM;
		}
		memset(g_fist_layer_imem[i], 0x0, sizeof(NN_DATA) * NN_IMEM_NUM);
	}
#endif

#if (NET_GEN_HEAP == 1)
#else
	g_map_mem = (VENDOR_AIS_FLOW_MAP_MEM_PARM *)vendor_ai_malloc(sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM) * g_ai_support_net_max);
	if (g_map_mem == NULL) {
		return HD_ERR_NOMEM;
	}
	memset(g_map_mem, 0x0, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM) * g_ai_support_net_max);
#endif

#if (NET_GEN_HEAP == 1)
#else
	g_ai_net_gen_init = (UINT32 *)vendor_ai_malloc(sizeof(UINT32) * g_ai_support_net_max);
	if (g_ai_net_gen_init == NULL) {
		return HD_ERR_NOMEM;
	}
	memset(g_ai_net_gen_init, 0x0, sizeof(UINT32) * g_ai_support_net_max);
#endif

	return HD_OK;
}

HD_RESULT vendor_ai_net_gen_context_uninit(VOID)
{
#if (NET_GEN_HEAP == 1)
#else
	UINT i;
#endif

#if (NET_GEN_HEAP == 1)
#else
	for (i = 0; i < g_ai_support_net_max; i++) {
		if (g_fist_layer_imem[i]) {
			vendor_ai_free(g_fist_layer_imem[i], sizeof(NN_DATA) * NN_IMEM_NUM);
		}
	}
	if (g_fist_layer_imem) {
		vendor_ai_free(g_fist_layer_imem, sizeof(NN_DATA) * g_ai_support_net_max);
	}
#endif
#if (NET_GEN_HEAP == 1)
#else
	if (g_map_mem) {
		vendor_ai_free(g_map_mem, sizeof(VENDOR_AIS_FLOW_MAP_MEM_PARM) * g_ai_support_net_max);
	}
#endif
#if (NET_GEN_HEAP == 1)
#else
	if (g_ai_net_gen_init) {
		vendor_ai_free(g_ai_net_gen_init, sizeof(UINT32) * g_ai_support_net_max);
	}
#endif
	return HD_OK;
}

#if NET_TABLE_HEAP
HD_RESULT vendor_ais_net_alloc_map_table_addr(void)
{
	g_proc_idx_map_table = (UINT32**)vendor_ai_malloc(sizeof(UINT32*) * g_ai_support_net_max);
	if (g_proc_idx_map_table == NULL) {
		DBG_ERR("cannot alloc g_proc_idx_map_table for g_ai_support_net_max =%d\r\n", (int)g_ai_support_net_max);
		return HD_ERR_HEAP;
	}
	memset(g_proc_idx_map_table, 0, sizeof(UINT32*) * g_ai_support_net_max);

	g_proc_max_batch_size = (UINT32*)vendor_ai_malloc(sizeof(UINT32) * g_ai_support_net_max);
	if (g_proc_max_batch_size == NULL) {
		DBG_ERR("cannot alloc g_proc_max_batch_size for g_ai_support_net_max =%d\r\n", (int)g_ai_support_net_max);
		return HD_ERR_HEAP;
	}
	memset(g_proc_max_batch_size, 0, sizeof(UINT32) * g_ai_support_net_max);
	return HD_OK;
}

HD_RESULT vendor_ais_net_free_map_table_addr(void)
{
	vendor_ai_free(g_proc_idx_map_table, sizeof(UINT32*) * g_ai_support_net_max);
	vendor_ai_free(g_proc_max_batch_size, sizeof(UINT32) * g_ai_support_net_max);
	return HD_OK;
}

HD_RESULT vendor_ais_net_alloc_map_table(UINT32 proc_id, VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem_manager_tmp)
{ 
	g_proc_max_batch_size[proc_id] = vendor_ais_net_get_input_layer_index(p_mem_manager_tmp->user_parm);
	g_proc_idx_map_table[proc_id] = (UINT32 *)vendor_ai_malloc(g_proc_max_batch_size[proc_id] * sizeof(UINT32));
	if (g_proc_idx_map_table[proc_id] == NULL) {
		DBG_ERR("cannot alloc g_proc_idx_map_table[proc_id] for proc_id = %lu\r\n",proc_id);
		return HD_ERR_HEAP;
	}
	memset(g_proc_idx_map_table[proc_id], 0, g_proc_max_batch_size[proc_id] * sizeof(UINT32));
	return HD_OK;
}

HD_RESULT vendor_ais_net_free_map_table(UINT32 proc_id)
{ 
	vendor_ai_free(g_proc_idx_map_table[proc_id], g_proc_max_batch_size[proc_id]); 
	return HD_OK;
}
#endif

#if NN_DLI
NN_DLI_TENSOR_INFO_HEADER *vendor_ais_get_nn_dli_tensor_info_header(NN_GEN_MODEL_HEAD *p_head, UINT32 parm_va_ofs)
{
	UINT32* p_ex_head = NULL;
	UINT32 ex_total_size = 0;
	UINT32 tmp_size = 0;
	NN_DLI_TENSOR_INFO_HEADER *p_tensor_info_header = NULL;

	p_ex_head = (UINT32*)(parm_va_ofs + sizeof(NN_GEN_MODEL_HEAD) + p_head->mode_ctrl_num*sizeof(NN_GEN_MODE_CTRL) + p_head->layer_id_list_size);
	ex_total_size = p_head->external_size;

	// Try to find tensor info header
	tmp_size = 0;
	while (tmp_size < ex_total_size) {
		UINT32* p_tmp_head = (UINT32*)(((UINTPTR)p_ex_head) + tmp_size);
		if (p_tmp_head[0] == 0) { // bit0 represent size
			break;
		}
		if (p_tmp_head[1] == NN_DLI_TENSOR_INFO_HEADER_SIGN) { // bit1 represent tag_type
			p_tensor_info_header = (NN_DLI_TENSOR_INFO_HEADER *)p_tmp_head;
			break;
		}

		tmp_size += p_tmp_head[0]; // add tmp_size, move to next
	}

	return p_tensor_info_header;
}

VOID vendor_ais_map_nn_dli_tensor_addr(NN_GEN_MODEL_HEAD *p_head, UINT32 parm_va_ofs, UINT32 model_ofs, UINT32 buf_ofs)
{
	NN_DLI_TENSOR_INFO_HEADER *p_tensor_info_header = NULL;
	NN_DLI_TENSOR_INFO *p_tensor_info = NULL;

	p_tensor_info_header = vendor_ais_get_nn_dli_tensor_info_header(p_head, parm_va_ofs);
	if(p_tensor_info_header == NULL) {
		return;
	}

	// Get Tensor Info array
	p_tensor_info = (NN_DLI_TENSOR_INFO *)(((UINTPTR)p_tensor_info_header) + sizeof(NN_DLI_TENSOR_INFO_HEADER));

	//DBG_ERR("p_tensor_info_header: %p, p_tensor_info: %p\r\n", p_tensor_info_header, p_tensor_info);
	for(UINT32 i = 0; i < p_tensor_info_header->nums; i++) {
		p_tensor_info[i].data_va = net_map_addr_with_parsflag(p_tensor_info[i].data_va, model_ofs, buf_ofs);
	}

	return;
}

VOID vendor_ais_unmap_nn_dli_tensor_addr(NN_GEN_MODEL_HEAD *p_head, UINT32 parm_va_ofs, UINT32 model_ofs, UINT32 buf_ofs, UINT32 model_end, UINT32 buf_end)
{
	NN_DLI_TENSOR_INFO_HEADER *p_tensor_info_header = NULL;
	NN_DLI_TENSOR_INFO *p_tensor_info = NULL;


	p_tensor_info_header = vendor_ais_get_nn_dli_tensor_info_header(p_head, parm_va_ofs);
	if(p_tensor_info_header == NULL) {
		return;
	}

	// Get Tensor Info array
	p_tensor_info = (NN_DLI_TENSOR_INFO *)(((UINTPTR)p_tensor_info_header) + sizeof(NN_DLI_TENSOR_INFO_HEADER));

	//DBG_ERR("p_tensor_info_header: %p, p_tensor_info: %p\r\n", p_tensor_info_header, p_tensor_info);
	for(UINT32 i = 0; i < p_tensor_info_header->nums; i++) {
		p_tensor_info[i].data_va = net_unmap_addr(p_tensor_info[i].data_va, model_ofs, buf_ofs, model_end, buf_end);
	}

	return;
}
#endif
