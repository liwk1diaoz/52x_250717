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

	CHAR input_filename[256];
	UINT32 w;
	UINT32 h;
	UINT32 c;
	UINT32 b;
	UINT32 bitdepth;
	UINT32 loff;
	UINT32 fmt;
	UINT32 is_comb_img;  // 1: image image (or feature-in) is a combination image (or feature-in).
	
} NET_IN_CONFIG;

typedef struct _NET_IN {

	NET_IN_CONFIG in_cfg;
	MEM_PARM input_mem;
	UINT32 in_id;
	VENDOR_AI_BUF src_img;

} NET_IN;

extern NET_IN g_in[16];

extern HD_RESULT ai_test_input_init(void);
extern HD_RESULT ai_test_input_uninit(void);
extern INT32     ai_test_input_mem_config(NET_PATH_ID net_path, HD_COMMON_MEM_INIT_CONFIG *p_mem_cfg, void *p_cfg, INT32 i);
extern HD_RESULT ai_test_input_set_config(IN_PATH_ID in_path, NET_IN_CONFIG *p_in_cfg);
extern HD_RESULT ai_test_input_open(IN_PATH_ID in_path);
extern HD_RESULT ai_test_input_close(IN_PATH_ID in_path);
extern HD_RESULT ai_test_input_start(NET_PATH_ID net_path);
extern HD_RESULT ai_test_input_stop(NET_PATH_ID net_path);
extern HD_RESULT ai_test_input_pull_buf(NET_PATH_ID net_path, VENDOR_AI_BUF *p_in, INT32 wait_ms);

#endif //_AI_TEST_PUSH_H_

