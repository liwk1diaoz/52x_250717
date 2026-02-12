/**
    @file       UIFrameworkInternal.h

    @brief      UI framework internal header file
                Declare data and function prototype of UI framework internally

    Copyright   Novatek Microelectronics Corp. 2007.  All rights reserved.
*/

/**
@addtogroup mIAPPExtUIFrmwk
@{
*/

#ifndef UIWNDINTERNAL_H
#define UIWNDINTERNAL_H

#define UX_POSTEVT      1

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIWnd
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////
#include <kwrap/debug.h>


//TODO: move from UIFramework to UIControl

extern UIRender *_pUIRender;
extern VControl *pLockedCtrl;

#define DIRTY_MASK      0x00ff
#define DIRTY_THIS      0x007f
#define DIRTY_CHILD     0x0080
#define DIRTY_ALL       (DIRTY_THIS|DIRTY_CHILD)

#define CTRL_HIDE     0x0100

extern INT32 Ux_CreateRelation(VControl *pWnd);
extern NVTRET Ux_GetFocusedWindowWithIndex(VControl **ppWnd, UINT8 *index);
extern INT32 Ux_WndEventProc(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray);
extern INT32 Ux_SendEventInternal(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray);
extern NVTRET Ux_CloseWindowInternal(VControl *pWnd, UINT32 paramNum, UINT32 *paramArray);
extern NVTRET Ux_OpenWindowInternal(VControl *pWnd, UINT32 paramNum, UINT32 *paramArray);
extern void Ux_PostCloseWindowInternal(void);
//@}

#endif //UIWNDINTERNAL_H
