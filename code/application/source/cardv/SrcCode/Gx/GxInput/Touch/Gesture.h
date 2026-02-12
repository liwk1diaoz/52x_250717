/**
    @file       Utility.h
    @ingroup    mISYSUtil

    @brief      Gesture function collection.

    Copyright   Novatek Microelectronics Corp. 2004.  All rights reserved.
*/

#ifndef _Gesture_H
#define _Gesture_H

#include "kwrap/type.h"
#include "GxInput.h"
/*
    @addtogroup mISYSGesture
@{
*/
extern TP_GESTURE_CODE Ges_SetPress(BOOL bCurrPenDown, INT32 X, INT32 Y);

extern UINT16  g_uiTPClickTH;
extern UINT16  g_uiSlideHorizontalTH;
extern UINT16  g_uiSlideVerticalTH;
/**
@}
*/

#endif //  _Gesture_H

