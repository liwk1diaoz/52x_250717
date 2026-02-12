/*
    Detect touch panel gesture

    Detect touch panel gesture

    @file       Gesture.c
    @ingroup    mISYSGesture
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2005.  All rights reserved.
*/

#include <stdio.h>
#include "kwrap/type.h"
//#include "Debug.h"
#include "Gesture.h"
#include <stdlib.h>


///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          Gesture
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

/**
    @addtogroup mISYSGesture
@{
*/


#define MOVE_POINT_NUM            20

#define TP_CLICK_DEFAULT_TH       20//10
#define SLIDE_HORIZONTAL_DEFAULT_TH    80
#define SLIDE_VERTICAL_DEFAULT_TH      60

UINT16  g_uiTPClickTH = TP_CLICK_DEFAULT_TH;
UINT16  g_uiSlideHorizontalTH = SLIDE_HORIZONTAL_DEFAULT_TH;
UINT16  g_uiSlideVerticalTH = SLIDE_VERTICAL_DEFAULT_TH;


typedef BOOL (*PIS_GESTURE)(void);

static BOOL Ges_IsSlide(void);
static BOOL Ges_IsClick(void);

static PIS_GESTURE pGestureList[] = {
	Ges_IsClick,
	Ges_IsSlide,
	NULL
};

typedef struct _TP_MOVE_TRACK_ {
	UINT16  uiCnt;
	UINT16  uiStart;
	UINT16  uiLast;
	IPOINT   MovePoint[MOVE_POINT_NUM];
} TP_MOVE_TRACK, *PTP_MOVE_TRACK;

static UINT32 uiGesture;
static TP_MOVE_TRACK MoveTrack;
//#NT#2011/02/14#Ben Wang -begin
//#NT#Remove warning [551] of PCLint
//static IPOINT TouchPoint;
//#NT#2011/02/14#Ben Wang -end
static void Ges_ClearMoveTrack(void)
{
	UINT32 i;
	// Clear count and each point
	MoveTrack.uiCnt = 0;
	MoveTrack.uiStart = 0;
	MoveTrack.uiLast = 0;
	for (i = 0; i < MOVE_POINT_NUM; i++) {
		MoveTrack.MovePoint[i].x = 0;
		MoveTrack.MovePoint[i].y = 0;
	}
}

static void Ges_AddMoveTrack(INT32 X, INT32 Y)
{
	if (MoveTrack.uiCnt == MOVE_POINT_NUM) {
		MoveTrack.uiStart = (MoveTrack.uiStart + 1) % MOVE_POINT_NUM;
		MoveTrack.uiLast = (MoveTrack.uiLast + 1) % MOVE_POINT_NUM;
	} else {
		MoveTrack.uiLast = (MoveTrack.uiStart + MoveTrack.uiCnt) % MOVE_POINT_NUM;
	}

	MoveTrack.MovePoint[MoveTrack.uiLast].x = X;
	MoveTrack.MovePoint[MoveTrack.uiLast].y = Y;
	if (MoveTrack.uiCnt != MOVE_POINT_NUM) {
		MoveTrack.uiCnt++;
	}
}

static BOOL Ges_IsSlide(void)
{
	UINT32 iXDelta, iYDelta;
	if (MoveTrack.uiCnt > 0) {
		iXDelta = (UINT32)abs(MoveTrack.MovePoint[MoveTrack.uiStart].x - MoveTrack.MovePoint[MoveTrack.uiLast].x);
		iYDelta = (UINT32)abs(MoveTrack.MovePoint[MoveTrack.uiStart].y - MoveTrack.MovePoint[MoveTrack.uiLast].y);
		DBG_MSG("^M iXDelta = %d, iYDelta = %d\r\n", iXDelta, iYDelta);
		if (MoveTrack.MovePoint[MoveTrack.uiStart].x > MoveTrack.MovePoint[MoveTrack.uiLast].x) {
			if (iXDelta > 2 * iYDelta && iXDelta > g_uiSlideHorizontalTH) {
				uiGesture = TP_GESTURE_SLIDE_LEFT;
				DBG_MSG("^R TP_GESTURE_SLIDE_LEFT\r\n");
				return TRUE;
			}
		} else {
			if (iXDelta > 2 * iYDelta && iXDelta > g_uiSlideHorizontalTH) {
				uiGesture = TP_GESTURE_SLIDE_RIGHT;
				DBG_MSG("^R TP_GESTURE_SLIDE_RIGHT\r\n");
				return TRUE;
			}
		}
#if 0
		if (MoveTrack.MovePoint[MoveTrack.uiStart].y > MoveTrack.MovePoint[MoveTrack.uiLast].y) {
			if (iYDelta > 2 * iXDelta && iYDelta > g_uiSlideVerticalTH) {
				uiGesture = TP_GESTURE_SLIDE_UP;
				DBG_MSG("^R TP_GESTURE_SLIDE_UP\r\n");
				return TRUE;
			}
		} else {
			if (iYDelta > 2 * iXDelta  && iYDelta > g_uiSlideVerticalTH) {
				uiGesture = TP_GESTURE_SLIDE_DOWN;
				DBG_MSG("^R TP_GESTURE_SLIDE_DOWN\r\n");
				return TRUE;
			}
		}
#else
		if (iYDelta > 2 * iXDelta && iYDelta > g_uiSlideVerticalTH) {
			if (MoveTrack.MovePoint[MoveTrack.uiStart].y > MoveTrack.MovePoint[MoveTrack.uiLast].y) {
				uiGesture = TP_GESTURE_SLIDE_UP;
				DBG_MSG("^R TP_GESTURE_SLIDE_UP\r\n");
				return TRUE;
			} else {
				uiGesture = TP_GESTURE_SLIDE_DOWN;
				DBG_MSG("^R TP_GESTURE_SLIDE_DOWN\r\n");
				return TRUE;
			}

		}
#endif
	}
	return FALSE;
}

static BOOL Ges_IsClick(void)
{
	UINT16 iXDelta1, iYDelta1, iXDelta2, iYDelta2;
	UINT16 uiMiddle;
	//Check if it matches click gesture
	if (MoveTrack.uiCnt > 0) {
		uiMiddle = (MoveTrack.uiStart + MoveTrack.uiCnt / 2) % MOVE_POINT_NUM;
		iXDelta1 = (UINT16)abs(MoveTrack.MovePoint[MoveTrack.uiStart].x - MoveTrack.MovePoint[uiMiddle].x);
		iYDelta1 = (UINT16)abs(MoveTrack.MovePoint[MoveTrack.uiStart].y - MoveTrack.MovePoint[uiMiddle].y);
		iXDelta2 = (UINT16)abs(MoveTrack.MovePoint[MoveTrack.uiLast].x - MoveTrack.MovePoint[uiMiddle].x);
		iYDelta2 = (UINT16)abs(MoveTrack.MovePoint[MoveTrack.uiLast].y - MoveTrack.MovePoint[uiMiddle].y);
		//DBG_MSG("iXDelta=%d, iYDelta=%d\r\n", iXDelta, iYDelta);
		if ((iXDelta1 < g_uiTPClickTH) && (iYDelta1 < g_uiTPClickTH)
			&& (iXDelta2 < g_uiTPClickTH) && (iYDelta2 < g_uiTPClickTH)) {
			//#NT#2011/02/14#Ben Wang -begin
			//#NT#Remove warning [551] of PCLint
			//TouchPoint.x  = MoveTrack.MovePoint[MoveTrack.uiLast].x;
			//TouchPoint.y   = MoveTrack.MovePoint[MoveTrack.uiLast].y;
			//#NT#2011/02/14#Ben Wang -end
			uiGesture = TP_GESTURE_CLICK;
			DBG_MSG("^R TP_GESTURE_CLICK\r\n");
			return TRUE;
		}
	}
	return FALSE;
}

static void Ges_Event2Gesture(void)
{
	UINT32 i;
	for (i = 0; pGestureList[i] != NULL; i++) {
		if (pGestureList[i]()) {
			break;
		}
	}
}

TP_GESTURE_CODE Ges_SetPress(BOOL bCurrPenDown, INT32 X, INT32 Y)
{
	static BOOL bLastPenDown = FALSE;
	if (bCurrPenDown) {
		//Touch panel press
		if (!bLastPenDown) {
			//First Touch panel press
			bLastPenDown = bCurrPenDown;
			//Flush move tracking array everytime first PenDown event happens
			Ges_ClearMoveTrack();
			uiGesture = TP_GESTURE_PRESS;
			DBG_MSG("^B Press\r\n");
		} else {
			uiGesture = TP_GESTURE_MOVE;
			//DBG_MSG("^B Move\r\n");
		}
		//#NT#2011/02/14#Ben Wang -begin
		//#NT#Remove warning [551] of PCLint
		//TouchPoint.x  = X;
		//TouchPoint.y   = Y;
		//#NT#2011/02/14#Ben Wang -end
		Ges_AddMoveTrack(X, Y);
	} else {
		//Touch panel release
		if (bLastPenDown) {
			DBG_MSG("^B Release\r\n");
			//First Touch panel release
			bLastPenDown = bCurrPenDown;
			uiGesture = TP_GESTURE_RELEASE;
			// Transfer event to gesture
			Ges_Event2Gesture();
		} else {
			uiGesture = TP_GESTURE_IDLE;
		}
	}
	return (TP_GESTURE_CODE)uiGesture;
}

/**
@}   //addtogroup
*/

