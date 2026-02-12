#ifndef _SDET_API_INT_H_
#define _SDET_API_INT_H_

#include "sdet_api.h"

//=============================================================================
// extern functions
//=============================================================================
extern UINT32 sdet_api_get_item_size(SDET_ITEM item);
extern ER sdet_api_get_cmd(SDET_ITEM item, UINT32 addr);
extern ER sdet_api_set_cmd(SDET_ITEM item, UINT32 addr);

#endif

