#ifndef _SDE_API_INT_H_
#define _SDE_API_INT_H_

#include "sde_alg.h"
#include "sde_lib.h"

#define SDE_MIN(a, b)         (((INT32)(a) < (INT32)(b)) ? (INT32)(a) : (INT32)(b))

extern BOOL sde_opened;
extern SDE_PARAM sde_drv_param[SDE_ID_MAX_NUM];

//extern INT32 sde_api_cb_flow(ISP_ID id, ISP_EVENT evt, UINT32 frame_cnt, void *param);

#endif

