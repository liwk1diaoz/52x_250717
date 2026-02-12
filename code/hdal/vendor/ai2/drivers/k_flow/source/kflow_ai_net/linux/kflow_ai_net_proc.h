/**
	@brief Header file of debug definition of vendor net flow sample.

	@file kflow_ai_net_proc.h

	@ingroup kflow ai net proc header file

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _KFLOW_AI_NET_PROC_H_
#define _KFLOW_AI_NET_PROC_H_

#include "kflow_ai_net/kflow_ai_net.h"

int kflow_ai_net_proc_create(void);
void kflow_ai_net_proc_remove(void);
int kflow_cmd_out_init(void);
int kflow_cmd_out_uninit(void);
int kflow_cmd_out_prog_debug(void);
int kflow_cmd_out_run_debug(void);
int kflow_ai_cmd_out_wait(KFLOW_AI_IOC_CMD_OUT *p_out);
void kflow_ai_cmd_out_sig(UINT32 proc_id, KFLOW_AI_IOC_CMD_OUT *p_out);
int kflow_ai_set_gen_version(VENDOR_AIS_FLOW_VERS *p_vers_info, UINT32 chip_id);
CHAR* kflow_ai_get_gen_version(void);
int kflow_ai_set_lib_version(KFLOW_AI_IOC_VERSION *p_vers_info);
int kflow_ai_get_debug_info(KFLOW_AI_IOC_DEBUG_INFO *p_debug_info);
int kflow_ai_get_debug_lvl(KFLOW_AI_IOC_DEBUG_LVL *p_debug_lvl);
int kflow_ai_net_proc_init(VOID);
int kflow_ai_net_proc_uninit(VOID);

#endif  /* _KFLOW_AI_NET_PROC_H_ */
