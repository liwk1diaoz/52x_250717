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

#ifndef UIFRAMEWORK_INTERNAL_H
#define UIFRAMEWORK_INTERNAL_H

#include "NvtUser/NvtUser.h"
#include "MessageElement.h"
#include <kwrap/error_no.h>

#define UX_POSTEVT      1

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIFwk
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

extern void Ux_DumpEventHistory(void);
//Event-queue of UIFramework tsk
extern void Ux_InitMessage(void);
extern BOOL Ux_IsInitMessage(void);
extern void Ux_UnInitMessage(void);

extern USER_CB_FP gpfNvtuserCB;
//@}

#endif //UIFRAMEWORK_INTERNAL_H
