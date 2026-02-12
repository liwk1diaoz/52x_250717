/**
    @brief 		Header file of fileout library.

    @file 		fileout_init.h

    @ingroup 	mFileOut

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef FILEOUT_INT_H
#define FILEOUT_INT_H

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#if defined (__UITRON) || defined (__ECOS)
#include "Type.h"
#include "ImageUnit_FileOut.h"
#include "ImageUnit_FileOut_CB.h"
#include "ImageUnit_FileOut_ID.h"
#include "ImageUnit_FileOut_Tsk.h"
#include "ImageUnit_FileOut_Util.h"
#include "ImageUnit_FileOut_SxCmd.h"
#include "ImageUnit_FileOut_Port.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          ISF_FILEOUT
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////
#else
#include <string.h>
#include "kwrap/type.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include "kwrap/stdio.h"
#include "kwrap/error_no.h"
#include "kwrap/flag.h"
#include "kwrap/perf.h"
#include <kwrap/semaphore.h>
#include "kwrap/task.h"
#include "kwrap/util.h"
#include "FileSysTsk.h"

#include "hd_fileout_lib.h"
#if defined(__FREERTOS)
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          FILEOUT
#define __DBGLVL__          2
#include "kwrap/debug.h"
extern unsigned int FILEOUT_debug_level;
///////////////////////////////////////////////////////////////////////////////
#else
#define CHKPNT      printf("\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__)
#define DBGD(x)     printf("\033[0;35m%s=%d\033[0m\r\n",#x,x)                  ///< Show a color sting of variable name and variable deciaml value
#define DBGH(x)     printf("\033[0;35m%s=0x%08X\033[0m\r\n",#x,x)
#define DBG_ERR(fmtstr, args...) printf("\033[0;31mERR:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_WRN(fmtstr, args...) printf("\033[0;33mWRN:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_DUMP(fmtstr, args...) printf(fmtstr, ##args)
#define vk_printk   printf
#define NVT_DBG_WRN 2

#define TRACE_LOG 0
#define PERF_LOG 0

#if TRACE_LOG //debug
#define DBG_IND(fmtstr, args...) printf("%s(): " fmtstr, __func__, ##args)
#define DBG_FUNC_BEGIN(fmtstr, args...) printf(fmtstr, ##args)
#define DBG_FUNC_END(fmtstr, args...) printf(fmtstr, ##args)
#else //release
#define DBG_IND(...)
#define DBG_FUNC_BEGIN(...)
#define DBG_FUNC_END(...)
#endif

#if PERF_LOG
#define PERF_PRINT(fmtstr, args...) printf("\033[0;35mPERF:%s() \033[0m" fmtstr, __func__, ##args)
#define PERF_VAL(name) struct timeval name
#define PERF_MARK(p) gettimeofday(&p, NULL)
#define PERF_DIFF(from, to) ((to.tv_sec * 1000000 + to.tv_usec) - (from.tv_sec * 1000000 + from.tv_usec))
#define PROFILE_TASK_BUSY()
#define PROFILE_TASK_IDLE()
#else
#define PERF_PRINT(...)
#define PERF_VAL(name)
#define PERF_MARK(p)
#define PERF_DIFF(from, to)
#define PROFILE_TASK_BUSY()
#define PROFILE_TASK_IDLE()
#endif
#endif

// BACKWARD COMPATIBLE
#define ISF_UNIT_FILEOUT                 0
#define ISF_MAX_FILEOUT                  2
#define ISF_IN_BASE  		             128
#define ISF_OUT_BASE  	                 0
#define ISF_FILEOUT_IN_NUM               16
#define ISF_FILEOUT_OUT_NUM              16
#define ISF_OK                           E_OK
#define ISF_ERR_FAILED                   E_SYS

#define ISF_FILEOUT_FOP_NONE             HD_FILEOUT_FOP_NONE
#define ISF_FILEOUT_FOP_CREATE           HD_FILEOUT_FOP_CREATE
#define ISF_FILEOUT_FOP_CLOSE            HD_FILEOUT_FOP_CLOSE
#define ISF_FILEOUT_FOP_CONT_WRITE       HD_FILEOUT_FOP_CONT_WRITE
#define ISF_FILEOUT_FOP_SEEK_WRITE       HD_FILEOUT_FOP_SEEK_WRITE
#define ISF_FILEOUT_FOP_FLUSH            HD_FILEOUT_FOP_FLUSH
#define ISF_FILEOUT_FOP_DISCARD          HD_FILEOUT_FOP_DISCARD
#define ISF_FILEOUT_FOP_READ             HD_FILEOUT_FOP_READ_WRITE
#define ISF_FILEOUT_FOP_SNAPSHOT         HD_FILEOUT_FOP_SNAPSHOT

#define ISF_FILEOUT_BUF                  HD_FILEOUT_BUF

#define FILEOUT_CB_EVENT                 HD_FILEOUT_CB_EVENT
#define FILEOUT_CB_EVENT_NAMING          HD_FILEOUT_CB_EVENT_NAMING
#define FILEOUT_CB_EVENT_OPENED          HD_FILEOUT_CB_EVENT_OPENED
#define FILEOUT_CB_EVENT_CLOSED          HD_FILEOUT_CB_EVENT_CLOSED
#define FILEOUT_CB_EVENT_DELETE          HD_FILEOUT_CB_EVENT_DELETE
#define FILEOUT_CB_EVENT_FS_ERR          HD_FILEOUT_CB_EVENT_FS_ERR

#define FILEOUT_CB_ERRCODE               HD_FILEOUT_ERRCODE
#define FILEOUT_CB_ERRCODE_CARD_SLOW     HD_FILEOUT_ERRCODE_CARD_SLOW
#define FILEOUT_CB_ERRCODE_CARD_WR_ERR   HD_FILEOUT_ERRCODE_CARD_WR_ERR
#define FILEOUT_CB_ERRCODE_LOOPREC_FULL  HD_FILEOUT_ERRCODE_LOOPREC_FULL
#define FILEOUT_CB_ERRCODE_SNAPSHOT_ERR  HD_FILEOUT_ERRCODE_SNAPSHOT_ERR
#define FILEOUT_CB_ERRCODE_FOP_SLOW      HD_FILEOUT_ERRCODE_FOP_SLOW

#define FILEOUT_CB_RETVAL                HD_FILEOUT_RETVAL
#define FILEOUT_CB_RETVAL_DONE           HD_FILEOUT_RETVAL_DONE
#define FILEOUT_CB_RETVAL_NOFREE_SPACE   HD_FILEOUT_RETVAL_NOFREE_SPACE
#define FILEOUT_CB_RETVAL_PARAM_ERR      HD_FILEOUT_RETVAL_PARAM_ERR
#define FILEOUT_CB_RETVAL_DELETE_FAIL    HD_FILEOUT_RETVAL_DELETE_FAIL

#define FILEOUT_FEVENT                   HD_FILEOUT_FEVENT
#define FILEOUT_FEVENT_NORMAL            HD_FILEOUT_FEVENT_NORMAL
#define FILEOUT_FEVENT_EMR               HD_FILEOUT_FEVENT_EMR
#define FILEOUT_FEVENT_BSINCARD          HD_FILEOUT_FEVENT_BSINCARD

#define FILEOUT_CB_EVDATA_NAMING         HD_FILEOUT_CBINFO
#define FILEOUT_CB_EVDATA_OPENED         HD_FILEOUT_CBINFO
#define FILEOUT_CB_EVDATA_CLOSED         HD_FILEOUT_CBINFO
#define FILEOUT_CB_EVDATA_DELETE         HD_FILEOUT_CBINFO
#define FILEOUT_CB_EVDATA_FS_ERR         HD_FILEOUT_CBINFO

#define FILEOUT_EVENT_CB                 HD_FILEOUT_CALLBACK

#define FILEOUT_IPORT(p)                 p

// INCLUDE PROTECTED
#include "fileout_cb.h"
#include "fileout_id.h"
#include "fileout_port.h"
#include "fileout_sxcmd.h"
#include "fileout_tsk.h"
#include "fileout_util.h"
#endif

/*-----------------------------------------------------------------------------*/
/* Macro Function Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define FILEOUT_DUMP(fmtstr, args...) DBG_DUMP("[FILEOUT] " fmtstr, ##args)
#define FOTEST_DUMP(fmtstr, args...) DBG_DUMP("^G[FILEOUT_TEST] %s(): " fmtstr, __func__, ##args)
#define IS_QUE_EMPTY(p) (p->queue_head == p->queue_tail)
#define QUE_NUM(p) ((p->queue_head >= p->queue_tail ? 0 : FILEOUT_OP_BUF_NUM) + p->queue_head - p->queue_tail)

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define FILEOUT_DEFAULT_MAX_POP_SIZE (4*1024*1024UL)
#define FILEOUT_QUEUE_RES_FILE_SIZE (10*1024*1024UL)
#define FILEOUT_QUEUE_MAX_FILE_SIZE (4*1024*1024*1024UL-FILEOUT_QUEUE_RES_FILE_SIZE)
#define FILEOUT_DEFAULT_STRGBLK_SIZE (120*1024*1024UL)
#define FILEOUT_MIN_APPEND_SIZE (1*1024*1024UL)

#define FILEOUT_DUR_LIMIT_CREATE    1000
#define FILEOUT_DUR_LIMIT_WRITE     400
#define FILEOUT_DUR_LIMIT_FLUSH     400
#define FILEOUT_DUR_LIMIT_CLOSE     1000
#define FILEOUT_DUR_LIMIT_SNAPSHOT  1000
#define FILEOUT_DUR_LIMIT_CB_NAMING	1000
#define FILEOUT_DUR_LIMIT_CB_CLOSED	1000
#define FILEOUT_DUR_LIMIT_CB_DELETE	1000

//TEST
#define FILEOUT_TEST_DEBUG_OPS      DISABLE
#define FILEOUT_TEST_FILE_CONTENT   DISABLE

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/
typedef enum {
	FILEOUT_LOGTYPE_BASE = 1,
	FILEOUT_LOGTYPE_QUE_SIZE = FILEOUT_LOGTYPE_BASE,
	FILEOUT_LOGTYPE_QUE_INFO,
	FILEOUT_LOGTYPE_FDB_INFO,
	FILEOUT_LOGTYPE_DUR_INFO,
	FILEOUT_LOGTYPE_BUF_INFO,
	FILEOUT_LOGTYPE_DBG_INFO,
	FILEOUT_LOGTYPE_MAX,
	ENUM_DUMMY4WORD(FILEOUT_LOGTYPE)
} FILEOUT_LOGTYPE;

#endif //FILEOUT_INT_H