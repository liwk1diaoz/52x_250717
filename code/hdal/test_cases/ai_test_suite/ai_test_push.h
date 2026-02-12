/**
	@brief Source file of vendor ai net test code.

	@file ai_test_push.h

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _AI_TEST_PUSH_H_
#define _AI_TEST_PUSH_H_

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "ai_test_int.h"
#include "ai_test_util.h"
#include "vendor_ai.h"

/*-----------------------------------------------------------------------------*/
/* Input Functions                                                             */
/*-----------------------------------------------------------------------------*/
typedef struct _NET_IN_CONFIG {

	CHAR   input_filename[256];//輸入圖片的檔案路徑
	UINT32      w;//輸入圖片的w
	UINT32      h;//輸入圖片的h
	UINT32      c;//輸入圖片的c
	UINT32      loff;//輸入圖片的loff
	UINT32      fmt;//輸入圖片的fmt
	UINT32      batch;
	UINT32      bit;
	UINT32      is_comb_img;
	UINT32      float_to_fix;//輸入圖片為float格式且要轉成fix格式，轉了後跟 pc tool 產生的 golden output 比對才會一致

	INT32       invalid;//輸入圖片是否不合法，0表示圖片合法，ai api不能報錯；1表示圖片不合法，ai api必須報錯
	INT32       verify_result;//本圖片經ai處理後是否要跟golden sample做比對，0表示不做比對，1表示要比對
	CHAR        golden_sample_filename[256];//golden sample的檔案路徑，如果不做比對的話就不用填
	MEM_PARM    input_mem;

} NET_IN_CONFIG;

typedef struct _NET_IN_OUT_CONFIG {
	CHAR   input_filename[256];//輸入圖片的檔案路徑
	UINT32      in_w;//輸入圖片的w
	UINT32      in_h;//輸入圖片的h
	UINT32      in_c;//輸入圖片的c
	UINT32      in_loff;//輸入圖片的loff
	UINT32      in_fmt;//輸入圖片的fmt

	UINT32      out_w;//輸出圖片的w
	UINT32      out_h;//輸出圖片的h
	UINT32      out_c;//輸出圖片的c
	UINT32      out_loff;//輸出圖片的loff
	UINT32      out_fmt;//輸出圖片的fmt

	INT32       invalid;//輸入圖片是否不合法，0表示圖片合法，ai api不能報錯；1表示圖片不合法，ai api必須報錯
	INT32       verify_result;//本圖片經ai處理後是否要跟golden sample做比對，0表示不做比對，1表示要比對
	CHAR        golden_sample_filename[256];//golden sample的檔案路徑，如果不做比對的話就不用填
	MEM_PARM    input_mem;
} NET_IN_OUT_CONFIG;

typedef struct _NET_IN_FILE_CONFIG {

	UINT32 input_num;//輸入圖像的數量
	UINT32 in_buf_unset;//輸入VENDOR_AI_BUF為垃圾值
	UINT32 in_buf_zero_size;//輸入圖像的大小為0
	UINT32 in_buf_null;//輸入VENDOR_AI_BUF為NULL
	UINT32 in_buf_zero_addr;//輸入圖像的地址為0
	UINT32 in_buf_unaligned_addr;
	UINT32 proc_after_restart;//檢查 set in、stop、start、proc 會報錯
	UINT32 proc_without_set_input;//檢查 proc 前沒有 set input 會報錯
	NET_IN_CONFIG* in_cfg;//net的所有輸入圖像的資訊
	NET_IN_OUT_CONFIG* in_out_cfg;//op的所有輸入圖像的資訊
	CHAR in_file_filename[256];

} NET_IN_FILE_CONFIG;

typedef struct _NET_IN {

	NET_IN_CONFIG in_cfg;
	MEM_PARM input_mem;
	UINT32 in_id;
	VENDOR_AI_BUF src_img;

} NET_IN;

#endif //_AI_TEST_PUSH_H_

