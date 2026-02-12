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

#ifndef UIBACKGROUND_INTERNAL_H
#define UIBACKGROUND_INTERNAL_H

#define FLGFRAMEWORK_BACKGROUND_BUSY    0x00000004
#define FLGFRAMEWORK_BACKGROUND_DONE    0x00000008
#define FLGFRAMEWORK_BACKGROUND_IDLE    0x00000010
#define FLGFRAMEWORK_BACKGROUND_MASK    FLGPTN_BIT_ALL

extern void Bk_InitMessage(void);
extern BOOL Bk_IsInitMessage(void);

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIBkg
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
//#include "DebugModule.h"
///////////////////////////////////////////////////////////////////////////////


#endif
