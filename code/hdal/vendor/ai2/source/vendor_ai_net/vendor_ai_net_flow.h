/**
	@brief Header file of definition of vendor user-space net flow sample.

	@file net_flow_user_sample.h

	@ingroup net_flow_user_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _VENDOR_AI_NET_FLOW_H_
#define _VENDOR_AI_NET_FLOW_H_

#define NET_FLOW_USR_SAMPLE_LAYER_OUT	TRUE

/********************************************************************
	INCLUDE FILES
********************************************************************/
#include "hd_type.h"
#include "kflow_ai_net/kflow_ai_net.h"
#include "kflow_ai_net/nn_parm.h"
#include "vendor_ai.h"
#include "vendor_ai_plugin.h"
#include "vendor_ai_comm.h"

#include "kdrv_ai.h" //for NEW_AI_FLOW
#if CNN_AI_FASTBOOT
#include "kdrv_builtin/kflow_ai_builtin_api.h"
#endif

/********************************************************************
	MACRO CONSTANT DEFINITIONS
********************************************************************/
#if (NEW_AI_FLOW == 0)
#define NN_SUPPORT_NET_MAX		128
#endif

#if CNN_MULTI_INPUT
#define AI2_MAX_BATCH_NUM  16
#endif

#define AI_ROUND(x) (x + 0.5)

/********************************************************************
	EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
//extern HD_RESULT vendor_ais_init(void);
//extern HD_RESULT vendor_ais_uninit(void);
//extern HD_RESULT vendor_ais_lock_net(UINT32 net_id);
//extern HD_RESULT vendor_ais_unlock_net(UINT32 net_id);
extern HD_RESULT vendor_ais_comm_lock(void);
extern HD_RESULT vendor_ais_comm_unlock(void);
extern HD_RESULT vendor_ais_open_net(UINT32 net_id);
extern HD_RESULT vendor_ais_close_net(UINT32 net_id);
extern HD_RESULT vendor_ais_remap_kerl_mem(VENDOR_AIS_FLOW_MAP_MEM_INFO *p_info);
extern HD_RESULT vendor_ais_unmap_kerl_mem(VENDOR_AIS_FLOW_MAP_MEM_INFO *p_info);
extern HD_RESULT vendor_ais_pars_kerl_mem(VENDOR_AIS_FLOW_MAP_MEM_INFO *p_info);
extern INT32 vendor_ais_flow_user_init(VENDOR_AIS_FLOW_MAP_MEM_PARM mem, UINT32 net_id);
extern INT32 vendor_ais_flow_user_uninit(UINT32 net_id);
#if 0 //LL_SUPPORT_ROI
extern HD_RESULT vendor_ais_set_ll_info(AI_DRV_LL_USR_INFO *ll_usr_info, UINT32 net_id);
#endif
#if (NEW_AI_FLOW == 1)
extern HD_RESULT vendor_ais_start_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, UINT32 net_id, UINT32 order, INT32 wait_ms, UINT32 ddr_id);
extern HD_RESULT vendor_ais_push_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, UINT32 net_id, UINT32 order);
extern HD_RESULT vendor_ais_pull_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, UINT32 net_id, INT32 wait_ms);
extern HD_RESULT vendor_ais_proc_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, UINT32 net_id, UINT32 order, INT32 wait_ms);
extern HD_RESULT vendor_ais_stop_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, UINT32 net_id);
#else
extern HD_RESULT vendor_ais_proc_net(VENDOR_AIS_FLOW_MEM_PARM net_mem, VENDOR_AIS_FLOW_MEM_PARM rslt_mem, UINT32 net_id);
#endif
#if CNN_25_MATLAB
extern HD_RESULT vendor_ais_proc_input_init(NN_MODE mode, NN_IOMEM *p_1st_mem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 net_id);
extern HD_RESULT vendor_ais_proc_input_uninit(NN_IOMEM *p_1st_mem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 net_id);
#else
extern HD_RESULT vendor_ais_unpars_kerl_mem(VENDOR_AIS_FLOW_MAP_MEM_INFO *p_info);
extern HD_RESULT vendor_ais_proc_input_init(NN_MODE mode, NN_DATA *p_1st_imem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 net_id);
extern HD_RESULT vendor_ais_proc_input_uninit(NN_DATA *p_1st_imem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 net_id);
#if CNN_MULTI_INPUT
extern HD_RESULT vendor_ais_proc_input_init2(NN_MODE mode, NN_DATA *p_1st_imem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 proc_idx, UINT32 net_id);
extern HD_RESULT vendor_ais_proc_input_uninit2(NN_DATA *p_1st_imem, VENDOR_AIS_FLOW_MEM_PARM mem, UINT32 proc_idx, UINT32 net_id);
#endif
#endif
extern NN_GEN_MODE_CTRL *vendor_ais_get_proclayer(UINT32 layer, VENDOR_AIS_FLOW_MEM_PARM mem);
extern HD_RESULT vendor_ais_update_proclayer(UINT32 layer, UINT32 net_id);
extern HD_RESULT vendor_ais_get_net_info(NN_GEN_NET_INFO *p_info, UINT32 net_addr);
extern HD_RESULT vendor_ais_set_ext_cb(UINT32 net_id, VENDOR_AI_NET_PROC_CB fp);
extern HD_RESULT vendor_ais_set_ll_base(UINT32 net_id, UINT32 eng, UINT32 addr);
extern HD_RESULT vendor_ais_set_mem_ofs(UINT32 net_id, UINT32 ofs);
extern HD_RESULT vendor_ais_chk_vers(UINT32 chip_id, UINT32 gentool_vers, UINT32 net_id);
#if CNN_MULTI_INPUT
extern UINT32 vendor_ais_net_get_input_layer_index(VENDOR_AIS_FLOW_MEM_PARM mem);
#endif

extern HD_RESULT vendor_ais_pars_diff_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo, UINT32 net_id);
extern HD_RESULT vendor_ais_unpars_diff_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AI_DIFF_MODEL_RESINFO *p_diff_resinfo, UINT32 net_id);

extern HD_RESULT vendor_ais_pars_diff_batch_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem,  VENDOR_AIS_DIFF_BATCH_MODEL_INFO *p_diff_info, UINT32 net_id);
extern HD_RESULT vendor_ais_unpars_diff_batch_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AIS_DIFF_BATCH_MODEL_INFO *p_diff_info, UINT32 net_id);
extern HD_RESULT vendor_ais_get_diff_batch_id_num(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 *batch_id_num);
extern HD_RESULT vendor_ais_get_diff_batch_id_info(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, UINT32 *max_batch_num, UINT32* p_batch_id);
extern HD_RESULT vendor_ais_set_usr_info(VOID* p_usr_info, UINT32 net_id);
HD_RESULT vendor_ais_pars_diff_batch(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AI_BATCH_INFO *p_diff_batch, UINT32 net_id);
HD_RESULT vendor_ais_unpars_diff_batch(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem, VENDOR_AI_BATCH_INFO *p_diff_batch, UINT32 net_id);

extern BOOL vendor_ais_check_diff_batch_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem);

#if (NEW_AI_FLOW == 1)
typedef HD_RESULT (*VENDOR_AIS_JOB_CB)(UINT32 proc_id, UINT32 job_id);

extern HD_RESULT vendor_ais_net_reg_JOB(VENDOR_AIS_JOB_CB fp);
extern HD_RESULT vendor_ais_net_builtin_JOB_done(UINT32 proc_id, UINT32 job_id, VENDOR_AIS_FLOW_MEM_PARM mem);

#if NN_DLI
extern HD_RESULT vendor_ais_net_builtin_CPU_init(UINT32 proc_id, UINT32 job_id, VENDOR_AIS_FLOW_MEM_PARM mem);
#endif
extern HD_RESULT vendor_ais_net_builtin_CPU_exec(UINT32 proc_id, UINT32 job_id, VENDOR_AIS_FLOW_MEM_PARM mem);
#if NN_DLI
extern HD_RESULT vendor_ais_net_builtin_CPU_exit(UINT32 proc_id, UINT32 job_id, VENDOR_AIS_FLOW_MEM_PARM mem);
#endif

extern HD_RESULT vendor_ais_net_builtin_DSP_exec(UINT32 proc_id, UINT32 job_id, VENDOR_AIS_FLOW_MEM_PARM mem);

extern HD_RESULT vendor_ai_init_job_graph(UINT32 net_id);
extern HD_RESULT vendor_ai_uninit_job_graph(UINT32 net_id);
extern HD_RESULT vendor_ai_sig_job_graph(UINT32 net_id);
extern HD_RESULT vendor_ai_wait_job_graph(UINT32 net_id, UINT32* job_id);
#endif

#if CNN_AI_FASTBOOT
extern UINT32 vendor_ais_get_fastboot_dump_enable(UINT32 proc_id);
extern HD_RESULT vendor_ais_get_fastboot_rslt(KFLOW_AI_BUILTIN_RSLT_INFO *p_rslt_info);
extern HD_RESULT vendor_ais_get_kerl_start_mem(VENDOR_AIS_FLOW_KERL_START_MEM *p_kerl_start);
extern INT vendor_ais_is_fastboot(void);
extern HD_RESULT vendor_ais_get_fastboot_builtin_mem(VENDOR_AIS_FLOW_MAP_MEM_PARM *p_mem);
#endif

extern HD_RESULT vendor_ais_get_multiscale_max_dim(UINT32 proc_id, HD_DIM *p_max_dim);
extern HD_RESULT vendor_ais_set_model_id(VENDOR_AI_DIFF_MODEL_RESINFO *p_resinfo);
extern INT32 vendor_ai_get_kflow_version(CHAR* kflow_version);
extern INT32 vendor_ai_get_kdrv_version(CHAR* kdrv_version);

extern UINT32 vendor_ais_net_get_fc_cmd_size(UINT32 max_weight_h, UINT32 net_id);
extern HD_RESULT vendor_ais_set_fc_ll_cmd(VENDOR_AI_OP_FC_CMDBUF *fc_info, UINT32 net_id);

#endif  /* _VENDOR_AI_NET_FLOW_H_ */
