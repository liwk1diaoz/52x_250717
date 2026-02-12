#ifndef _KDRV_SIE_DEBUG_INT_H_
#define _KDRV_SIE_DEBUG_INT_H_
#include "sie_dbg.h"
#include "sie_platform.h"
#include "kdrv_sie.h"
#include "plat/top.h"
#include "kdrv_sie_int.h"
/**
    kdrv_sie_debug_int.h


    @file       kdrv_sie_debug_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/

typedef enum {
	KDRV_SIE_DBG_MSG_OFF = 0,		//disable all debug msg
	KDRV_SIE_DBG_MSG_CTL_INFO,		//dump kdrv_sie parameters info
	KDRV_SIE_DBG_MSG_ALL,			//enable all ctl sie debug msg
	KDRV_SIE_DBG_MSG_MAX
} KDRV_SIE_DBG_MSG_TYPE;

typedef enum {
	KDRV_SIE_DBG_CNT_TYPE_CURR, // clear in open
	KDRV_SIE_DBG_CNT_TYPE_ACCU, // will not clear
	KDRV_SIE_DBG_CNT_TYPE_MAX
} KDRV_SIE_DBG_CNT_TYPE;

#define KDRV_SIE_DFT_LOG_RATE 300

typedef enum {
	KDRV_SIE_DBG_CNT_VD,
	KDRV_SIE_DBG_CNT_ERR_DRMIN1_UDFL,
	KDRV_SIE_DBG_CNT_ERR_DRMIN2_UDFL,
	KDRV_SIE_DBG_CNT_ERR_DRMOUT0_OVFL,
	KDRV_SIE_DBG_CNT_ERR_DRMOUT1_OVFL,
	KDRV_SIE_DBG_CNT_ERR_DRMOUT2_OVFL,
	KDRV_SIE_DBG_CNT_ERR_DPC,
	KDRV_SIE_DBG_CNT_ERR_RAWENC,
	KDRV_SIE_DBG_CNT_ERR_SIECLK,
	KDRV_SIE_DBG_CNT_MAX
} KDRV_SIE_DBG_CNT_SEL;

typedef struct {
	KDRV_SIE_DBG_CNT_SEL dbg_cnt_sel;
	CHAR msg[15];
} KDRV_SIE_DBG_MSG;

typedef struct {
	KDRV_SIE_DBG_MSG_TYPE dbg_msg_type;

	/* debug */
	BOOL		init;
	atomic_t	cnt[KDRV_SIE_DBG_CNT_MAX][KDRV_SIE_DBG_CNT_TYPE_MAX];
	UINT32		err_log_rate;
} KDRV_SIE_DBG_INFO;

void kdrv_sie_dump_info(int (*dump)(const char *fmt, ...));
void kdrv_sie_dbg_set_dbg_level(KDRV_SIE_DBG_LVL dbg_level);
void kdrv_sie_dbg_set_msg_type(KDRV_SIE_PROC_ID id, KDRV_SIE_DBG_MSG_TYPE type);

void kdrv_sie_dbg_init(KDRV_SIE_PROC_ID id);
void kdrv_sie_dbg_uninit(KDRV_SIE_PROC_ID id);
void kdrv_sie_upd_vd_intrpt_sts(KDRV_SIE_PROC_ID id, SIE_ENGINE_STATUS_INFO_CB *info);
void kdrv_sie_upd_intrpt_err_sts(KDRV_SIE_PROC_ID id, UINT32 status);

#endif //_KDRV_SIE_DEBUG_INT_H_
