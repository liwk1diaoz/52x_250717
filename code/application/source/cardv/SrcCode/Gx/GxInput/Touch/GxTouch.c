/**
    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.

    @file       GxTouch.c
    @ingroup    mIPRJAPKeyIO

    @brief      Scan Touch Panel

    @note       Nothing.

    @date       2009/04/22
*/


//#include "kernel.h"
#include "PrjInc.h"
#include "GxInput.h"
#include "DxInput.h"
#include "Gesture.h"
#include "Calibrate.h"
//#include "Debug.h"
#include <stdlib.h>
#include "touch/touch_inc.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>

#include "linux/rtc.h"
#include <errno.h>

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxTouch
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

#define MOVE_MARGIN_DEFAULT_X    1
#define MOVE_MARGIN_DEFAULT_Y    1

#define TOUCH_AND_HOLD_DEFAULT_DELAY_TIME      24   // 20ms * 24 * TOUCH_TIMER_CNT = 20 * 24 * 1 = 480ms
#define TOUCH_AND_HOLD_DEFAULT_REPEAT_RATE      6   // 20ms *  6 * TOUCH_TIMER_CNT = 20 *  6 * 1 = 120ms

static INT32 g_iMoveMarginX = MOVE_MARGIN_DEFAULT_X;
static INT32 g_iMoveMarginY = MOVE_MARGIN_DEFAULT_Y;
static UINT32 g_HoldDelayTime = TOUCH_AND_HOLD_DEFAULT_DELAY_TIME;
static UINT32 g_HoldRepeatRate = TOUCH_AND_HOLD_DEFAULT_REPEAT_RATE;


BOOL       g_bDumpTouch = FALSE;
static BOOL       g_bIsPenDown = FALSE;
// -------------------------------------------------------------------
// Static variables
// -------------------------------------------------------------------
static GX_CALLBACK_PTR   g_fpTouchCB = NULL;
static UINT32      g_ClickInterval = 20;   //  20ms * 20 * TOUCH_TIMER_CNT =  20 *20 * 1 = 400ms
static INT32 g_Touchfd = -1;

/**
  Detect touch panel

  @param void
  @return void
*/
#if (__DBGLVL__ > 1)
static void ShowTouchData(PGX_TOUCH_DATA pTouchData)
{
	DBG_MSG("\r\n");

	if (pTouchData->Gesture == TP_GESTURE_PRESS) {
		DBG_MSG("PRESS");
	} else if (pTouchData->Gesture == TP_GESTURE_MOVE) {
		DBG_MSG("MOVE");
	} else if (pTouchData->Gesture == TP_GESTURE_RELEASE) {
		DBG_MSG("RELEASE");
	} else if (pTouchData->Gesture == TP_GESTURE_CLICK) {
		DBG_MSG("CLICK");
	} else if (pTouchData->Gesture == TP_GESTURE_DOUBLE_CLICK) {
		DBG_MSG("DOUBLE_CLICK");
	} else if (pTouchData->Gesture == TP_GESTURE_SLIDE_UP) {
		DBG_MSG("SLIDE_UP");
	} else if (pTouchData->Gesture == TP_GESTURE_SLIDE_DOWN) {
		DBG_MSG("SLIDE_DOWN");
	} else if (pTouchData->Gesture == TP_GESTURE_SLIDE_LEFT) {
		DBG_MSG("SLIDE_LEFT");
	} else if (pTouchData->Gesture == TP_GESTURE_SLIDE_RIGHT) {
		DBG_MSG("SLIDE_RIGHT");
	}
	DBG_MSG(" Point(%d,%d)\r\n", pTouchData->Point.x, pTouchData->Point.y);
}
#endif

int nvt_system(const char* pCommand)
{
    pid_t pid;
    int status;
    int i = 0;

    if(pCommand == NULL)
    {
        return (1);
    }

    if((pid = fork())<0)
    {
        status = -1;
    }
    else if(pid == 0)
    {
        /* close all descriptors in child sysconf(_SC_OPEN_MAX) */
        for (i = 3; i < sysconf(_SC_OPEN_MAX); i++)
        {
            close(i);
        }

        execl("/bin/sh", "sh", "-c", pCommand, (char *)0);
        _exit(127);
    }
    else
    {
        /*??�{��SIGCHLD��SIG_IGN�ާ@?�A?������status��^�ȡAXOS_System?�k���`�u�@*/
        while(waitpid(pid, &status, 0) < 0)
        {
            if(errno != EINTR)
            {
                status = -1;
                break;
            }
        }
    }

    return status;
}

void GxTouch_Init(void)
{
#if defined __FREERTOS
	DetTP_Init();
#else
#if 1
    INT32 Ret = 0;
    #if (defined(_MODEL_580_SDV_SJ10_) || defined(_MODEL_580_SDV_SJ10_FAST_BT_))
    Ret = nvt_system("modprobe touch_sj10");
    #elif (defined(_MODEL_580_SDV_C300_) || defined(_MODEL_580_SDV_C300_FAST_BT_))
    Ret = nvt_system("modprobe touch_c300");
    Ret = nvt_system("modprobe touch2_c300");
    #else
    Ret = nvt_system("modprobe touch_gt911");
    #endif
    if(0 != Ret)
    {
        DBG_ERR("insmod touch: failed, errno(%d)\n", errno);
        return;
    }else{
        //DBG_DUMP("insmod touch: success\n");
    }
#endif

    //Don't open device immediately after modprobe.
    //Don't use t he delay for  fastboot
    //Move to GxTouch_DetTP
    #if 0
    g_Touchfd = open("/dev/touch", O_RDWR);

    if (g_Touchfd < 0)
    {
        DBG_ERR("open [%s] failed\n","/dev/touch");
        return ;
    }else{
        DBG_DUMP("open [%s] success\n","/dev/touch");
    }
    #endif

#endif
}
void GxTouch_DetTP(void)
{
	GX_TOUCH_DATA TouchData;
	static BOOL bIsFirstPenDown = TRUE;
	static IPOINT LastPoint = {0};
	static UINT32 DoubleClickCnt = 0;
	INT32 x = 0, y = 0;
	BOOL   bPointChanged = TRUE;
	static UINT32 RepeatCnt = 0;

	DoubleClickCnt++;
	INT32 Ret = 0;
	#if defined __FREERTOS
	g_bIsPenDown = DetTP_IsPenDown();
	#else
	if(g_Touchfd<0){
        static UINT32 TouchOpenDevCnt = 0;

        g_Touchfd = open("/dev/touch", O_RDWR);
        if (g_Touchfd < 0)
        {
            TouchOpenDevCnt++;
            if (TouchOpenDevCnt > 50) { // 20ms*20 = 400ms. It is 20ms every scan.
                DBG_ERR("open [%s] failed\n","/dev/touch");
            }
		return;
        }else{
            TouchOpenDevCnt = 0;
            //DBG_DUMP("open [%s] success\n","/dev/touch");
        }
	}

	Ret = ioctl(g_Touchfd, IOC_TOUCH_ISPENDOWN, &g_bIsPenDown);
	if (Ret < 0)
	{
		DBG_ERR("pendown ioctl error");
		return;
	}
	#endif

	if (g_bIsPenDown) {
		#if defined __FREERTOS
		DetTP_GetXY(&x, &y);
		#else
		INT32  xyData[2] = {0};
		Ret = ioctl(g_Touchfd, IOC_TOUCH_GET_XY_VALUE, &xyData);
		if (Ret < 0)
		{
			DBG_ERR("xy ioctl error");
			return;
		}
		#endif
		x=xyData[0];
		y=xyData[1];

		//DBG_ERR("x:%d y:%d\r\n", x, y);

#if (CALIBRATION_FUNC == ENABLE)
		CalibrateXY(&x, &y);
#endif
		TouchData.Gesture = Ges_SetPress(TRUE, x, y);
		if (bIsFirstPenDown) {
			bIsFirstPenDown = FALSE;
		}

		if (LastPoint.x != x || LastPoint.y != y) {
			INT32 iXDelta, iYDelta;
			iXDelta = abs(x - LastPoint.x);
			iYDelta = abs(y - LastPoint.y);
			if (iXDelta >= g_iMoveMarginX || iYDelta >= g_iMoveMarginY) {
				bPointChanged = TRUE;
				LastPoint.x = x;
				LastPoint.y = y;
			} else {
				bPointChanged = FALSE;
			}
		} else {
			bPointChanged = FALSE;
			LastPoint.x = x;
			LastPoint.y = y;
		}

	} else {
		TouchData.Gesture = Ges_SetPress(FALSE, 0, 0);
		bIsFirstPenDown = TRUE;
		//check if it's a "double click"
		if (TouchData.Gesture == TP_GESTURE_CLICK) {
			if (DoubleClickCnt < g_ClickInterval) {
				TouchData.Gesture = TP_GESTURE_DOUBLE_CLICK;
				DoubleClickCnt = g_ClickInterval;
			} else {
				DoubleClickCnt = 0;
			}
		}

	}

	if (TouchData.Gesture != TP_GESTURE_IDLE) {
		TouchData.Point = LastPoint;
#if (__DBGLVL__ > 1)
		if (g_bDumpTouch) {
			ShowTouchData(&TouchData);
		}
#endif
		//just ignore the event when pen is pressing and NOT moving
		if (TouchData.Gesture == TP_GESTURE_MOVE && bPointChanged == FALSE) {
			if (RepeatCnt >= (g_HoldDelayTime + g_HoldRepeatRate)) {
				TouchData.Gesture = TP_GESTURE_HOLD;
				RepeatCnt = g_HoldDelayTime;
			} else {
				RepeatCnt ++;
				return;
			}
		} else {
			RepeatCnt = 0;
		}
		if (TouchData.Gesture > TP_GESTURE_RELEASE && g_fpTouchCB) {
			TP_GESTURE_CODE temp;
			temp = TouchData.Gesture;
			TouchData.Gesture = TP_GESTURE_RELEASE;
			if (g_fpTouchCB) {
				g_fpTouchCB(TOUCH_CB_GESTURE, (UINT32)&TouchData, 0);
			}
			TouchData.Gesture = temp;
		}
		if (TouchData.Gesture == TP_GESTURE_DOUBLE_CLICK && g_fpTouchCB) {
			TP_GESTURE_CODE temp;
			temp = TouchData.Gesture;
			TouchData.Gesture = TP_GESTURE_CLICK;
			if (g_fpTouchCB) {
				g_fpTouchCB(TOUCH_CB_GESTURE, (UINT32)&TouchData, 0);
			}
			TouchData.Gesture = temp;
		}
		if (g_fpTouchCB) {
			g_fpTouchCB(TOUCH_CB_GESTURE, (UINT32)&TouchData, 0);
		}
	}

}

#if 0
void Touch_OnSystem(int cmd)
{
	switch (cmd) {
	case SYSTEM_CMD_POWERON:
		if (g_fpTouchCB) {
			(*g_fpTouchCB)(SYSTEM_CB_CONFIG, 0, 0);
		}
		break;

	case SYSTEM_CMD_POWEROFF:
		break;
	default:
		break;
	}
}
#endif

void   GxTouch_RegCB(GX_CALLBACK_PTR fpTouchCB)
{
	g_fpTouchCB = fpTouchCB;
}

BOOL GxTouch_IsPenDown(void)
{
	return g_bIsPenDown;
}

void   GxTouch_SetCtrl(GXTOUCH_CTRL data, UINT32 value)
{
	switch (data) {
	case GXTCH_DOUBLE_CLICK_INTERVAL:
		g_ClickInterval = value;
		break;
	case GXTCH_MOVE_SENSITIVITY_X:
		g_iMoveMarginX = (INT32)value;
		break;
	case GXTCH_MOVE_SENSITIVITY_Y:
		g_iMoveMarginY = (INT32)value;
		break;
	case GXTCH_CLICK_TH:
		g_uiTPClickTH = (UINT16)value;
		break;
	case GXTCH_SLIDE_TH_HORIZONTAL:
		g_uiSlideHorizontalTH = (UINT16)value;
		break;
	case GXTCH_SLIDE_TH_VERTICAL:
		g_uiSlideVerticalTH = (UINT16)value;
		break;
	case GXTCH_HOLD_DELAY_TIME:
		g_HoldDelayTime = value;
		break;
	case GXTCH_HOLD_REPEAT_RATE:
		g_HoldRepeatRate = value;
		break;
	default:
		DBG_ERR("Ctrl(%d)", data);
		break;
	}
}

UINT32 GxTouch_GetCtrl(GXTOUCH_CTRL data)
{
	UINT32 getv = 0;

	switch (data) {
	case GXTCH_DOUBLE_CLICK_INTERVAL:
		getv = g_ClickInterval;
		break;
	case GXTCH_MOVE_SENSITIVITY_X:
		getv = (UINT32)g_iMoveMarginX;
		break;
	case GXTCH_MOVE_SENSITIVITY_Y:
		getv = (UINT32)g_iMoveMarginY;
		break;
	case GXTCH_CLICK_TH:
		getv = g_uiTPClickTH;
		break;
	case GXTCH_SLIDE_TH_HORIZONTAL:
		getv = g_uiSlideHorizontalTH;
		break;
	case GXTCH_SLIDE_TH_VERTICAL:
		getv = g_uiSlideVerticalTH;
		break;
	case GXTCH_HOLD_DELAY_TIME:
		getv = g_HoldDelayTime;
		break;
	case GXTCH_HOLD_REPEAT_RATE:
		getv = g_HoldRepeatRate;
		break;
	default:
		DBG_ERR("Ctrl(%d)", data);
		getv = 0;
		break;
	}
	return getv;
}
