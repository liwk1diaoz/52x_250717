/**
	@brief Source file of vendor ai net test code.

	@file ai_test_net.h

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _AI_TEST_CONFIG_NET_H_
#define _AI_TEST_CONFIG_NET_H_

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "ai_test_int.h"
#include "ai_test_util.h"
#include "ai_test_net.h"
#include "ai_test_push.h"
#include "ai_test_stress.h"

/*-----------------------------------------------------------------------------*/
/* Network Functions                                                           */
/*-----------------------------------------------------------------------------*/

typedef struct _FOURTH_LOOP_DATA {
	int                  loop_count;//載入不同圖片測試次數
	GOLDEN_SAMPLE_RESULT golden_sample;
} FOURTH_LOOP_DATA;

typedef struct _THIRD_LOOP_DATA {
	int              loop_count;//要測試幾次start、stop
	FOURTH_LOOP_DATA fourth_loop_data;//載入不同輸入圖片的loop數據
	MEM_PARM         input_mem;
	MEM_PARM         scale_mem;
    UINT32           in_id;
	VENDOR_AI_BUF    src_img;
	VENDOR_AI_BUF    scale_img;
} THIRD_LOOP_DATA;

typedef struct _SECOND_LOOP_DATA {
	int             loop_count;//要測試幾次open、close
	THIRD_LOOP_DATA third_loop_data;//從start到stop的loop數據
} SECOND_LOOP_DATA;

typedef struct _FIRST_LOOP_DATA {
	CHAR                         first_loop_path[256];
	int                          loop_count;//從載入model bin開始的測試要loop幾次
	NET_PATH_ID                  in_path;
	NET_PATH_ID                  net_path;//ai open的path
	INT32                        invalid_net_path;//net_path是否為非法，0表示合法，ai open不能報錯；1表示不合法，ai open必須報錯
	NET_PATH_ID                  op_path;
	SECOND_LOOP_DATA             second_loop_data;//從open到close的loop數據
	NET_MODEL_CONFIG             *model_cfg;//所有model bin相關設定，需用malloc分配
	OPERATOR_CONFIG              *op_cfg;//所有operator相關設定，需用malloc分配
	int                          second_cfg_count;//共有多少model bin或operator
	MEM_PARM                     model_mem;
	MEM_PARM                     diff_model_mem;
	MEM_PARM                     work_mem;
	MEM_PARM                     rbuf_mem;
	CHAR                         out_class_labels[MAX_CLASS_NUM * VENDOR_AIS_LBL_LEN];
	VENDOR_AI_NET_CFG_BUF_OPT    net_buf_opt;//ai 的 buf opt 參數
	INT32                        invalid_buf_opt;//buf opt 是否為非法值，0表示合法，ai open不能報錯；1表示不合法，ai open必須報錯
	VENDOR_AI_NET_CFG_JOB_OPT    net_job_opt;//ai 的 job opt 參數
	int                          schd_parm_inc;//job opt 的參數是否要改變，0 表示不變，1/-1 表示遞增/遞減
	INT32                        invalid_job_opt;//job opt是否為非法值，0表示合法，ai open不能報錯；1表示不合法，ai open必須報錯
	INT32                        null_work_buf;//測試work buffer為空指標
	INT32                        non_align_work_buf;//測試work buffer address非32 byte對齊
	VENDOR_AI_NET_INFO           net_info;
	pthread_t                    thread_id;
	int                          test_success;
	BOOL                         perf_analyze;
	int                          ignore_set_input_fail;//通常用在一個model要測試多張圖的項目，如果model遇到格式不合的圖片會忽略set in錯誤，換下一張圖測試
	NET_PATH_ID                  net_path_max;
	BOOL                         net_path_loop;
	BOOL                         ignore_set_res_dim_fail;
	BOOL                         cpu_perf_analyze;
	BOOL                         perf_analyze_report;
	BOOL                         proc_id_test;
	BOOL                         multi_scale_set_imm;
	struct _FIRST_LOOP_DATA      *share_model_master;
	NET_PATH_ID                  mem_seq;
} FIRST_LOOP_DATA;

typedef struct _GLOBAL_DATA {
	HD_COMMON_MEM_INIT_CONFIG mem_cfg;
	FIRST_LOOP_DATA           *first_loop_data;//表示要進行幾次load model bin的測試，每次測試都有不同設定，需用malloc分配
	int                       first_loop_count;//共有幾個first_loop_data
	int                       sequential; //所有first loop測試是平行進行還是依序進行，0表示平行、1表示依序
	int                       sequential_loop_count; //如果sequential為1，此參數表示要loop所有first_loop幾次；sequential為0則本參數無用
	UINT32                    timeout; //整個測試程式超過幾秒算卡住，0表示不偵測卡住
	UINT32                    ai_proc_schedule; //VENDOR_AI_PROC_SCHD
	pthread_t                 thread_id;
	CPU_STRESS_DATA           cpu_stress[2];
	DMA_STRESS_DATA           dma_stress[2];
	BOOL                      perf_compare;
	FLOAT                     time_threshold_upper;
	FLOAT                     time_threshold_upper_short;
	FLOAT                     time_threshold_lower;
	FLOAT                     bw_threshold_upper;
	FLOAT                     bw_threshold_lower;
	FLOAT                     size_threshold_lower;
	FLOAT                     size_threshold_upper;
	FLOAT                     memory_threshold_lower;
	FLOAT                     memory_threshold_upper;
	FLOAT                     loading_threshold_upper;
	FLOAT                     loading_threshold_lower;
	int                       dump_output;
	int                       load_output;
	CHAR                      generate_golden_output[256];
	INT32                     verify_result;//是否強制複寫NET_IN_CONFIG內的verify_result，0不複寫，-1複寫verify_result成0，1複寫verify_result成1
	INT32                     float_to_fix;//是否強制複寫NET_IN_CONFIG內的float_to_fix，0不複寫，-1複寫float_to_fix成0，1複寫float_to_fix成1
	int                       net_supported_num_lower;
	int                       net_supported_num_upper;
	int                       net_supported_num_check;
	int                       share_model;//是否進行 share model 測試
	int                       wait_load_share_model_memory_thread_number;
	int                       wait_free_share_model_memory_thread_number;
	int                       wait_load_share_model_memory_thread_number_2nd;
	int                       wait_free_share_model_memory_thread_number_2nd;
	INT32                     init_uninit_per_thread;//是否每個thread各自做 vendor_ai_init()、vendor_ai_uninit()
	CHAR                      perf_result_filename[128];
	int                       multi_process;
	int                       multi_process_loop_count;
	int                       multi_process_signal;
	int                       ctrl_c_reset;
	BOOL                      disable_perf_compare_last;
	BOOL                      disable_perf_compare_cpu;
	BOOL                      disable_perf_compare_dma;
} GLOBAL_DATA;

typedef struct _PROCESS_DATA {
	int                       process_id;
	int                       shm_id;
	int                       result;
	int                       parent_id; //only for signal usage
	int                       process_num; //only for parent process usage
} PROCESS_DATA;

extern int       global_fail;

extern HD_RESULT ai_test_network_init_global(UINT32 ai_proc_schedule, int share_model);
extern HD_RESULT ai_test_network_sel_proc_id(UINT32 proc_id);
extern HD_RESULT ai_test_network_uninit_global(void);
extern INT32     ai_test_common_mem_config(GLOBAL_DATA *global_data);
// SET
extern HD_RESULT ai_test_network_set_config(UINT32 ai_proc_schedule);
extern HD_RESULT ai_test_network_set_config_job(NET_PATH_ID net_path, VENDOR_AI_NET_CFG_JOB_OPT *opt, int schd_parm_inc);
extern HD_RESULT ai_test_network_set_config_buf(NET_PATH_ID net_path, UINT32 method, UINT32 ddr_id);
// GET
extern HD_RESULT ai_test_network_open(NET_PATH_ID net_path, VENDOR_AI_NET_INFO *net_info);
extern HD_RESULT ai_test_network_alloc_out_buf(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_free_out_buf(NET_PATH_ID net_path);
extern HD_RESULT ai_test_network_dump_postproc_buf(NET_PATH_ID net_path, CHAR *out_class_labels, void *p_out);
extern HD_RESULT ai_test_network_dump_detout_buf(NET_PATH_ID net_path, void *p_out);
extern HD_RESULT ai_test_network_get_out_buf(NET_PATH_ID net_path, void *p_out, INT32 wait_ms);

extern HD_RESULT ai_test_input_init(void);
extern HD_RESULT ai_test_input_uninit(void);
extern INT32     ai_test_input_mem_config(NET_PATH_ID net_path, HD_COMMON_MEM_INIT_CONFIG* p_mem_cfg, void* p_cfg, INT32 i);
extern HD_RESULT ai_test_input_set_config(NET_PATH_ID net_path, NET_IN_CONFIG* p_in_cfg);
extern HD_RESULT ai_test_input_open(NET_PATH_ID net_path, VENDOR_AI_NET_PARAM_ID param_id, NET_IN_CONFIG *in_cfg, MEM_PARM *input_mem, VENDOR_AI_BUF *src_img);
extern HD_RESULT ai_test_input_close(MEM_PARM *input_mem);
extern HD_RESULT ai_test_input_start(NET_PATH_ID net_path);
extern HD_RESULT ai_test_input_stop(NET_PATH_ID net_path);

extern VOID *network_sequential_thread(VOID *arg);
extern VOID *network_parallel_thread(VOID *arg);

extern GLOBAL_DATA* ai_test_get_global_data(void);

extern PROCESS_DATA* ai_test_get_process_data(void);

extern int op_first_loop(FIRST_LOOP_DATA *first_loop_data);

extern int wait_for_loading_share_model(FIRST_LOOP_DATA *first_loop);
extern int syc_for_freeing_share_model(FIRST_LOOP_DATA *first_loop);

#endif //_AI_TEST_CONFIG_NET_H_


