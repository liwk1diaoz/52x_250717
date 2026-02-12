/**
	@brief Source file of vendor ai net test code.

	@file ai_test_util.h

	@ingroup ai_net_sample

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _AI_TEST_UTIL_H_
#define _AI_TEST_UTIL_H_

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "hdal.h"

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/

#define HD_DEBUG_MENU_ID_QUIT 0xFE
#define HD_DEBUG_MENU_ID_RETURN 0xFF
#define HD_DEBUG_MENU_ID_LAST (-1)

typedef struct _HD_DBG_MENU {
	int            menu_id;          ///< user command value
	const char     *p_name;          ///< command string
	int      	  (*p_func)(void);  ///< command function
	BOOL           b_enable;         ///< execution option
} HD_DBG_MENU;

extern int my_debug_menu_entry_p(HD_DBG_MENU *p_menu, const char *p_title);

extern const char* my_debug_get_menu_name(void);


/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/

typedef struct _MEM_PARM {
	UINT32 pa;
	UINT32 va;
	UINT32 size;
	UINT32 blk;
	UINT8  ddr_id;
} MEM_PARM;

extern HD_RESULT ai_test_mem_get(MEM_PARM *mem_parm, UINT32 size, int sequential);
extern HD_RESULT ai_test_mem_rel(MEM_PARM *mem_parm);
extern HD_RESULT ai_test_mem_alloc(MEM_PARM *mem_parm, CHAR* name, UINT32 size);
extern HD_RESULT ai_test_mem_free(MEM_PARM *mem_parm);
extern INT32 ai_test_mem_load(MEM_PARM *mem_parm, const CHAR *filename);
extern INT32 ai_test_mem_save(MEM_PARM *mem_parm, UINT32 write_size, const CHAR *filename);
extern VOID ai_test_mem_fill(MEM_PARM *mem_parm, unsigned char data);
extern VOID ai_test_lock_and_sync(VOID);
extern VOID ai_test_unlock(VOID);
extern int ai_test_padding(char *src, int src_w, int src_h, int src_loff, int dst_w, int dst_h);
extern int ai_test_crop(char *src, int src_w, int src_h, int src_loff, char *dst, int dst_w, int dst_h);
extern UINT32 _calc_ai_buf_size(UINT32 loff, UINT32 h, UINT32 c, UINT32 t, UINT32 fmt);

#if defined (__UITRON) || defined(__ECOS)  || defined (__FREERTOS)

#else
extern void cal_linux_cpu_loading(int loop_num, double *cpu_user, double *cpu_sys);
#endif


#endif //_AI_TEST_UTIL_H_

