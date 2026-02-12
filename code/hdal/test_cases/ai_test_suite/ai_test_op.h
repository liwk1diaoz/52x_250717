/**
	@brief Source file of vendor ai net test code.

	@file ai_test_op.h

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _AI_TEST_OP_H_
#define _AI_TEST_OP_H_

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include "ai_test_int.h"
#include "ai_test_util.h"
/*-----------------------------------------------------------------------------*/
/* Operator Functions                                                           */
/*-----------------------------------------------------------------------------*/

typedef enum _AI_OP {
	AI_OP_FC                             = 0,
	AI_OP_PREPROC_YUV2RGB                = 1,
	AI_OP_PREPROC_YUV2RGB_SCALE          = 2,
	AI_OP_PREPROC_YUV2RGB_MEANSUB_PLANE  = 3,
	AI_OP_PREPROC_YUV2RGB_MEANSUB_DC     = 4,
	AI_OP_PREPROC_Y2Y_UV2UV              = 5,
	AI_OP_FC_LL_MODE                     = 6,
	ENUM_DUMMY4WORD(AI_OP)
} AI_OP;

typedef struct _OP_PROC {

	UINT32 proc_id;
	int op_opt;
	MEM_PARM input_mem;
	MEM_PARM weight_mem;
	MEM_PARM output_mem;
		
} OP_PROC;

extern HD_RESULT ai_test_operator_init(void);
extern HD_RESULT ai_test_operator_set_config(NET_PATH_ID net_path, int in_op_opt);
extern HD_RESULT ai_test_operator_open(NET_PATH_ID op_path, NET_PATH_ID in_path);
extern HD_RESULT ai_test_operator_close(NET_PATH_ID net_path);

#endif //_AI_TEST_OP_H_


