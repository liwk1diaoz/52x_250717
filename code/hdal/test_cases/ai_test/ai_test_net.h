/**
	@brief Source file of vendor ai net test code.

	@file ai_test_net.h

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _AI_TEST_NET_H_
#define _AI_TEST_NET_H_

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "ai_test_int.h"
#include "ai_test_util.h"
#include "vendor_ai.h"
#include "vendor_ai_cpu/vendor_ai_cpu.h"
#include "vendor_ai_cpu_postproc.h"
#include "vendor_ai_cpu_detout.h"

/*-----------------------------------------------------------------------------*/
/* Network Functions                                                             */
/*-----------------------------------------------------------------------------*/

typedef struct _NET_MODEL_CONFIG {

	CHAR model_filename[256];
	CHAR label_filename[256];
	INT32 binsize;
	CHAR diff_filename[256];
	INT32 diff_binsize;

	//int job_method;
	//int job_wait_ms;
	//int buf_method;

} NET_MODEL_CONFIG;

typedef struct _NET_PROC {

	//NET_PROC_CONFIG net_cfg;
	MEM_PARM proc_mem;
	MEM_PARM diff_mem;
	UINT32 proc_id;

	CHAR out_class_labels[MAX_CLASS_NUM * VENDOR_AIS_LBL_LEN];
	MEM_PARM rslt_mem;
	MEM_PARM io_mem;

} NET_PROC;

typedef struct _TEST_PROC_ID {
	UINT32 proc_id;
	BOOL   enable;
} TEST_PROC_ID;


extern HD_RESULT ai_test_network_init(void);
extern HD_RESULT ai_test_network_sel_proc_id(UINT32 proc_id);
extern HD_RESULT ai_test_network_uninit(void);
extern INT32     ai_test_network_mem_config(NET_PATH_ID net_path, HD_COMMON_MEM_INIT_CONFIG *p_mem_cfg, void *p_cfg, INT32 i);
// SET
extern HD_RESULT ai_test_network_set_config(void);
extern HD_RESULT ai_test_network_set_config_job(NET_PATH_ID net_path, UINT32 method, INT32 wait_ms);
extern HD_RESULT ai_test_network_set_config_buf(NET_PATH_ID net_path, UINT32 method, UINT32 ddr_id);
extern HD_RESULT ai_test_network_set_config_model(NET_PATH_ID net_path, NET_MODEL_CONFIG *p_model_cfg);
extern HD_RESULT ai_test_network_set_in_buf(NET_PATH_ID net_path, UINT32 layer_id, UINT32 in_id, VENDOR_AI_BUF *p_inbuf);
extern HD_RESULT ai_test_network_set_unknown(NET_PATH_ID net_path);
// GET
extern HD_RESULT ai_test_network_get_in_buf_byid(NET_PATH_ID net_path, UINT32 layer_id, UINT32 in_id);
extern HD_RESULT ai_test_network_get_out_buf_byid(NET_PATH_ID net_path, UINT32 layer_id, UINT32 out_id);
extern HD_RESULT ai_test_network_get_first_in_buf(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_get_first_out_buf(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_get_out_buf_byname(NET_PATH_ID net_path, VENDOR_AI_BUF_NAME *p_buf_name);
extern HD_RESULT ai_test_network_get_unknown(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_clr_config_model(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_open(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_close(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_alloc_out_buf(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_free_out_buf(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_dump_postproc_buf(NET_PATH_ID net_path, void *p_out);
extern HD_RESULT ai_test_network_dump_detout_buf(NET_PATH_ID net_path, void *p_out);
extern HD_RESULT ai_test_network_get_out_buf(NET_PATH_ID net_path, void *p_out, INT32 wait_ms);
extern HD_RESULT ai_test_network_alloc_io_buf(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_alloc_null_io_buf(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_alloc_non_align_io_buf(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_free_io_buf(NET_PATH_ID net_path);

#endif //_AI_TEST_NET_H_


