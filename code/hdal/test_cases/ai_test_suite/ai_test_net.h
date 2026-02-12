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
/* Network Functions                                                           */
/*-----------------------------------------------------------------------------*/

typedef struct _MULTI_SCALE_VALID_OUT {
	UINT32 w;
	UINT32 h;
	UINT32 valid_out_size;
} MULTI_SCALE_VALID_OUT;

typedef struct _GOLDEN_SAMPLE_RESULT {
	UINT32        size;
	unsigned char *data;
	VENDOR_AI_POSTPROC_RESULT_INFO rslt_postproc_info;
	VENDOR_AI_DETOUT_RESULT_INFO rslt_detout_info;
	UINT32        rslt_outbuf_num;
	VENDOR_AI_BUF *rslt_outbuf;
} GOLDEN_SAMPLE_RESULT;

typedef struct _NET_MODEL_CONFIG {

	CHAR                  model_filename[256];//model bin的檔案路徑
	CHAR                  label_filename[256];//label的檔案路徑
	INT32                 binsize;
	CHAR                  diff_filename[256];//diff model bin的檔案路徑
	INT32                 diff_binsize;
	INT32                 invalid;//本model是否為非法，0表示合法，ai api必須成功載入model；1表示不合法，ais api必須報錯
	INT32                 multi_in;//本model的輸入圖片是否為multi in，0表示非multi in，1表示multi in
	INT32                 multi_out;//本model的輸出是否為multi out，0表示非multi out，1表示multi out
	NET_IN_CONFIG         *in_cfg;//本model需要的輸入圖片，需用malloc分配
	int                   in_cfg_count;//共有多少輸入圖片
	NET_IN_FILE_CONFIG    *in_file_cfg;//本model需要的輸入圖片群組，需用malloc分配
	int                   in_file_cfg_count;//共有多少輸入圖片群組
	CHAR                  model_cfg_filename[256];
	UINT32                model_unset;
	UINT32                model_zero_size;
	UINT32                model_null;
	UINT32                model_zero_addr;
	UINT32                no_set_model_between_close_open;//close後重新open卻沒有重新設定model

	INT32                 diff_res_num;
	INT32                 diff_total_num;
	INT32                 diff_max_w;
	INT32                 diff_max_h;
	INT32                 *diff_width;
	INT32                 *diff_height;
	INT32                 diff_res_loop;
	INT32                 diff_res_random;
	INT32                 diff_res_random_times;
	INT32                 diff_res_padding;
	INT32                 diff_res_padding_num;
	INT32                 *diff_res_w;
	INT32                 *diff_res_h;
	GOLDEN_SAMPLE_RESULT  *diff_golden_sample;
	UINT32                multi_scale_res_num;
	MULTI_SCALE_VALID_OUT *multi_scale_valid_out;
	UINT32                multi_scale_random_num;
    CHAR                  res_filename[256];// filepath of resolution
    INT32                  dyB_res_num;    // total number of batch in dyB_batch
    INT32                  *dyB_batch;    // list of possible batches for dynamic batch patterns
    INT32                  *dyB_batch_random;    // list of possible batches for dynamic batch patterns
    UINT32                dynamic_batch_random_num;
 	  UINT32                in_path_num;
	UINT32                failed;
} NET_MODEL_CONFIG;

typedef struct _OPERATOR_CONFIG {
	int                   op_opt;//要進行的operator種類
	NET_IN_CONFIG         *in_cfg;//本operator需要的輸入圖片，需用malloc分配
	int                   in_cfg_count;//共有多少輸入圖片
	NET_IN_FILE_CONFIG    *in_file_cfg;//本model需要的輸入圖片群組，需用malloc分配
	int                   in_file_cfg_count;//共有多少輸入圖片群組
	CHAR                  op_cfg_filename[256];
} OPERATOR_CONFIG;

typedef struct _CPU_LOADING_ANALYZE {
	DOUBLE usage;
	BOOL  test_start;
	BOOL  test_done;
} CPU_LOADING_ANALYZE;

#endif //_AI_TEST_NET_H_


