#ifndef _UI_GFX_H_
#define _UI_GFX_H_
#include "UIWnd/UIFlow.h"

//UIGfx
extern void UI_InstallID(void);
extern void UI_UninstallID(void);

extern void UI_GfxInit(void);
extern void UI_DispInit(void);
extern void UI_ControlShowInit(void);
extern void UI_SetLayoutSize(UINT32 Width, UINT32 Height);
extern UINT32 UI_GetOSDSize(void);
extern void UIDisplay_Init(UINT8 iDD, BOOL bClear, ISIZE *pDeviceSize);

extern UIRender demoRender;

#endif //_UI_GFX_H_
