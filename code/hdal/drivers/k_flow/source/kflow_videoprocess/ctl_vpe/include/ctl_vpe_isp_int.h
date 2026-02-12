/**
    VPE Ctrl Layer, ISP internal used api

    @file       ctl_vpe_isp_int.h
    @ingroup    mIVPE_Ctrl
    @note       None

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/


#ifndef _CTL_VPE_ISP_INT_H
#define _CTL_VPE_ISP_INT_H

#include "kflow_common/isp_if.h"
#include "kflow_videoprocess/ctl_vpe_isp.h"
#include "ctl_vpe_int.h"

#define CTL_VPE_ISP_HANDLE_MAX_NUMBER		(4)
#define CTL_VPE_ISP_HANDLE_NAME_MAX_LENGTH	(32)
#define CTL_VPE_ISP_HANDLE_STATUS_FREE		(0)
#define CTL_VPE_ISP_HANDLE_STATUS_USED		(1)

#define CTL_VPE_ISP_ID_UNUSED	(0xFFFFFFFF)
#define CTL_VPE_ISP_ID_UNUSED_2	(0x0000FFFF)

#define CTL_VPE_ISP_ROT_MAX_ANGLE 			1800
#define CTL_VPE_ISP_ANGLE_PARSE_VALUE		205887

typedef INT32(*CTL_VPE_ISP_SET_FP)(ISP_ID id, CTL_VPE_ISP_ITEM item, void *data);
typedef INT32(*CTL_VPE_ISP_GET_FP)(ISP_ID id, CTL_VPE_ISP_ITEM item, void *data);

typedef struct {
	CHAR name[CTL_VPE_ISP_HANDLE_NAME_MAX_LENGTH];
	ISP_EVENT_FP fp;
	ISP_EVENT evt;
	UINT32 status;
	CTL_VPE_ISP_CB_MSG cb_msg;
} CTL_VPE_ISP_HANDLE;

INT32 ctl_vpe_isp_event_cb(ISP_ID id, ISP_EVENT evt, CTL_VPE_JOB *p_job, void *param);
void ctl_vpe_isp_drv_init(void);
void ctl_vpe_isp_drv_uninit(void);

ER ctl_vpe_isp_evt_fp_dbg_mode(UINT32 enable);
void ctl_vpe_isp_dump(int (*dump)(const char *fmt, ...));

#endif