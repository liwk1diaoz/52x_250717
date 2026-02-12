#ifndef _EIS_INT_H_
#define _EIS_INT_H_

#define THIS_DBGLVL         2//NVT_DBG_WRN
#define __MODULE__          nvt_eis
#define __DBGLVL__          THIS_DBGLVL // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
extern unsigned int nvt_eis_debug_level;

#if defined (__FREERTOS )
#include <stdio.h>
#include <string.h>
#endif
#include <kwrap/dev.h>
#include "kwrap/type.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/cpu.h"
#include "kwrap/task.h"
#include "kwrap/platform.h"
#include "kwrap/util.h"
#include "kwrap/cpu.h"
#include "kwrap/mem.h"
#include "kwrap/file.h"
#include "kwrap/list.h"
#include <kwrap/spinlock.h>
#include "eis_ioctl.h"
#include "kflow_videoprocess/isf_vdoprc.h"

#define debug_msg 			vk_printk

typedef void (*OS_COPY_CB)(void * dest, void * src, int len);

extern ER _eis_tsk_open(void);
extern ER _eis_tsk_close(void);
extern ER _eis_tsk_idle(void);

extern void _eis_msg_dump(void);
extern void _eis_out_dump(void);

extern void _eis_tsk_get_msg(EIS_WAIT_PROC_INFO *p_user);
extern void _eis_tsk_put_out(EIS_PUSH_DATA *p_data, OS_COPY_CB copy);
extern ER _eis_tsk_trigger_proc(VDOPRC_EIS_PROC_INFO *p_proc_info, BOOL in_isr);
extern ER _eis_tsk_get_output(VDOPRC_EIS_2DLUT *p_param);
extern ER _eis_tsk_gyro_latency(UINT32 gyro_latency);

extern ER _eis_tsk_get_path_info(VDOPRC_EIS_PATH_INFO *p_data);
extern void _eis_tsk_set_path_info(EIS_PATH_INFO *p_data);
extern void _eis_tsk_set_dbg_cmd(UINT32 param_id, UINT32 value);
extern void _eis_tsk_set_debug_size(UINT32 size);


//extern THREAD_HANDLE _SECTION(".kercfg_data") EIS_TSK_ID;
extern ID _SECTION(".kercfg_data") FLG_ID_EIS;
//extern THREAD_DECLARE(_eis_tsk, arglist);

#endif
