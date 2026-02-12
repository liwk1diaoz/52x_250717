#ifndef __KDRV_VPE_IF_H__
#define __KDRV_VPE_IF_H__

#include "kwrap/type.h"
#include "kdrv_videoprocess/kdrv_vpe.h"

INT32 kdrv_vpe_if_trigger(UINT32 handle, KDRV_VPE_JOB_LIST *p_job_list);
INT32 kdrv_vpe_if_get(UINT32 handle, UINT32 param_id, void *p_param);
INT32 kdrv_vpe_if_set(UINT32 handle, UINT32 param_id, void *p_param);
INT32 kdrv_vpe_if_open(UINT32 chip, UINT32 engine);
INT32 kdrv_vpe_if_close(UINT32 handle);
BOOL kdrv_vpe_if_is_init(void);

typedef INT32(*KDRV_VPE_IF_OPEN_FP)(UINT32 chip, UINT32 engine);
typedef INT32(*KDRV_VPE_IF_CLOSE_FP)(UINT32 handle);
typedef INT32(*KDRV_VPE_IF_GET_FP)(UINT32 handle, UINT32 param_id, void *p_param);
typedef INT32(*KDRV_VPE_IF_SET_FP)(UINT32 handle, UINT32 param_id, void *p_param);
typedef INT32(*KDRV_VPE_IF_TRIGGER_FP)(UINT32 handle, KDRV_VPE_JOB_LIST *p_job_list);
typedef struct {
	KDRV_VPE_IF_OPEN_FP open;
	KDRV_VPE_IF_CLOSE_FP close;
	KDRV_VPE_IF_GET_FP get;
	KDRV_VPE_IF_SET_FP set;
	KDRV_VPE_IF_TRIGGER_FP trigger;
} KDRV_VPE_IF_OBJ;

INT32 kdrv_vpe_if_register(KDRV_VPE_IF_OBJ *p_obj);
INT32 kdrv_vpe_if_init(void);
INT32 kdrv_vpe_if_exit(void);

#endif