#ifndef _VPET_API_INT_H_
#define _VPET_API_INT_H_

#include "vpet_api.h"

//=============================================================================
// extern functions
//=============================================================================
extern UINT32 vpet_api_get_item_size(VPET_ITEM item);
extern ER vpet_api_get_cmd(VPET_ITEM item, UINT32 addr);
extern ER vpet_api_set_cmd(VPET_ITEM item, UINT32 addr);
extern UINT32 vpet_get_param_update(UINT32 id, VPET_ITEM item);
extern void vpet_reset_param_update(UINT32 id, VPET_ITEM item);
extern BOOL vpe_update[VPE_ID_MAX_NUM][VPET_ITEM_MAX];

#endif

