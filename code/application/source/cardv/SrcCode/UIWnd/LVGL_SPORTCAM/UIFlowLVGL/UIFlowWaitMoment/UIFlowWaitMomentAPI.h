/*
 * UIFlowWaitMomentAPI.h
 *
 *  Created on: 2022¦~4¤ë18¤é
 *      Author: NVT02970
 */

#ifndef UIFLOWWAITMOMENTAPI_H
#define UIFLOWWAITMOMENTAPI_H

#include "UIFlowLVGL/UIFlowLVGL.h"
#include "Resource/Plugin/lvgl_plugin.h"

typedef struct {

	lv_plugin_res_id string_id;

	/* custom text */
	char* text;
	uint32_t size;

} UIFlowWaitMomentAPI_Set_Text_Param;

void  UIFlowWaitMomentAPI_Set_Text(UIFlowWaitMomentAPI_Set_Text_Param param);


#endif /* UIFLOWWAITMOMENTAPI_H */
