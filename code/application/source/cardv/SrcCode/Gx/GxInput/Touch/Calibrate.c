/**
    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.

    @file       Calibrate.c
    @ingroup

    @brief      Scan Touch Panel

    @note       Nothing.

    @date       2009/04/22
*/

//#include "kernel.h"
#include "GxInput.h"
#include "Calibrate.h"
#include "kwrap/type.h"
#include "kwrap/error_no.h"

//#include "Debug.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          GxTouch
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////


/*
  *                                             /-     -\
 *              /-    -\     /-            -\   |       |
 *              |      |     |              |   |   Xs  |
 *              |  Xd  |     | A    B    C  |   |       |
 *              |      |  =  |              | * |   Ys  |
 *              |  Yd  |     | D    E    F  |   |       |
 *              |      |     |              |   |   1   |
 *              \-    -/     \-            -/   |       |
 *                                              \-     -/
 *
 *
 *    where:
 *
 *           (Xd,Yd) represents the desired display point
 *                    coordinates,
 *
 *           (Xs,Ys) represents the available touch screen
 *                    coordinates, and the matrix
 *
 * Divider =  (Xs0 - Xs2)*(Ys1 - Ys2) - (Xs1 - Xs2)*(Ys0 - Ys2)
 *
 *
 *
 *                 (Xd0 - Xd2)*(Ys1 - Ys2) - (Xd1 - Xd2)*(Ys0 - Ys2)
 *            A = ---------------------------------------------------
 *                                   Divider
 *
 *
 *                 (Xs0 - Xs2)*(Xd1 - Xd2) - (Xd0 - Xd2)*(Xs1 - Xs2)
 *            B = ---------------------------------------------------
 *                                   Divider
 *
 *
 *                 Ys0*(Xs2*Xd1 - Xs1*Xd2) +
 *                             Ys1*(Xs0*Xd2 - Xs2*Xd0) +
 *                                           Ys2*(Xs1*Xd0 - Xs0*Xd1)
 *            C = ---------------------------------------------------
 *                                   Divider
 *
 *
 *                 (Yd0 - Yd2)*(Ys1 - Ys2) - (Yd1 - Yd2)*(Ys0 - Ys2)
 *            D = ---------------------------------------------------
 *                                   Divider
 *
 *
 *                 (Xs0 - Xs2)*(Yd1 - Yd2) - (Yd0 - Yd2)*(Xs1 - Xs2)
 *            E = ---------------------------------------------------
 *                                   Divider
 *
 *
 *                 Ys0*(Xs2*Yd1 - Xs1*Yd2) +
 *                             Ys1*(Xs0*Yd2 - Xs2*Yd0) +
 *                                           Ys2*(Xs1*Yd0 - Xs0*Yd1)
 *            F = ---------------------------------------------------
 *                                   Divider
 */

typedef struct {

	INT64    An,     /* A = An/Divider */
			 Bn,     /* B = Bn/Divider */
			 Cn,     /* C = Cn/Divider */
			 Dn,     /* D = Dn/Divider */
			 En,     /* E = En/Divider */
			 Fn,     /* F = Fn/Divider */
			 Divider ;
} MATRIX;

//Default let Xd = Xs, Yd = Ys
static MATRIX  g_Matrix = {1,//An=1,=>A=1
						   0,//Bn=0,=>B=0
						   0,//Cn=0,=>C=0
						   0,//Dn=0,=>D=0
						   1,//En=1,=>E=1
						   0,//Fn=0,=>F=0
						   1
						  };

static BOOL g_bCalibrated = FALSE;

ER GxTouch_InitCalibration(PGX_TOUCH_CALI pTouchCali)
{
	MATRIX   Matrix = {0};
	INT32   Xs0, Ys0, Xs1, Ys1, Xs2, Ys2;
	INT32   Xd0, Yd0, Xd1, Yd1, Xd2, Yd2;

	Xs0 = (INT32)pTouchCali->TouchPoint[0].x;
	Ys0 = (INT32)pTouchCali->TouchPoint[0].y;
	Xs1 = (INT32)pTouchCali->TouchPoint[1].x;
	Ys1 = (INT32)pTouchCali->TouchPoint[1].y;
	Xs2 = (INT32)pTouchCali->TouchPoint[2].x;
	Ys2 = (INT32)pTouchCali->TouchPoint[2].y;
	Xd0 = (INT32)pTouchCali->UIPoint[0].x;
	Yd0 = (INT32)pTouchCali->UIPoint[0].y;
	Xd1 = (INT32)pTouchCali->UIPoint[1].x;
	Yd1 = (INT32)pTouchCali->UIPoint[1].y;
	Xd2 = (INT32)pTouchCali->UIPoint[2].x;
	Yd2 = (INT32)pTouchCali->UIPoint[2].y;

	Matrix.An = (INT64)(Xd0 - Xd2) * (Ys1 - Ys2) - (INT64)(Xd1 - Xd2) * (Ys0 - Ys2);
	Matrix.Bn = (INT64)(Xs0 - Xs2) * (Xd1 - Xd2) - (INT64)(Xd0 - Xd2) * (Xs1 - Xs2);
	Matrix.Cn = (INT64)Ys0 * ((INT32)(Xs2 * Xd1) - (INT32)(Xs1 * Xd2)) +
				(INT64)Ys1 * ((INT32)(Xs0 * Xd2) - (INT32)(Xs2 * Xd0)) +
				(INT64)Ys2 * ((INT32)(Xs1 * Xd0) - (INT32)(Xs0 * Xd1));
	Matrix.Dn = (INT64)(Yd0 - Yd2) * (Ys1 - Ys2) - (INT64)(Yd1 - Yd2) * (Ys0 - Ys2);
	Matrix.En = (INT64)(Xs0 - Xs2) * (Yd1 - Yd2) - (INT64)(Yd0 - Yd2) * (Xs1 - Xs2);
	Matrix.Fn = (INT64)Ys0 * ((INT32)(Xs2 * Yd1) - (INT32)(Xs1 * Yd2)) +
				(INT64)Ys1 * ((INT32)(Xs0 * Yd2) - (INT32)(Xs2 * Yd0)) +
				(INT64)Ys2 * ((INT32)(Xs1 * Yd0) - (INT32)(Xs0 * Yd1));
	Matrix.Divider = (INT64)(Xs0 - Xs2) * (Ys1 - Ys2) - (INT64)(Xs1 - Xs2) * (Ys0 - Ys2);

	DBG_MSG("UIPoint 1-(%d,%d), 2-(%d,%d), 3-(%d,%d),\r\n", Xd0, Yd0, Xd1, Yd1, Xd2, Yd2);
	DBG_MSG("TouchPoint 1-(%d,%d), 2-(%d,%d), 3-(%d,%d),\r\n", Xs0, Ys0, Xs1, Ys1, Xs2, Ys2);


	DBG_MSG("An=%d, Bn=%d, Cn=%d\r\n", (INT32)Matrix.An, (INT32)Matrix.Bn, (INT32)Matrix.Cn);
	DBG_MSG("Dn=%d, En=%d, Fn=%d\r\n", (INT32)Matrix.Dn, (INT32)Matrix.En, (INT32)Matrix.Fn);
	DBG_MSG("Divider=%d\r\n", (INT32)Matrix.Divider);

	if (Matrix.Divider == 0) {
		DBG_ERR("GxTouch_InitCalibration failed!!!\r\n");
		g_bCalibrated = FALSE;
		return E_SYS;
	} else {
		g_Matrix = Matrix;
		g_bCalibrated = TRUE;
		return E_OK;
	}

}

void CalibrateXY(INT32 *pX, INT32 *pY)
{
//#NT#2015/08/25#KCHong#Modified for touch panel -begin
// Todo: Need to complete this function for touch point calibration
	//*pX = 1480 - *pX ;
	*pY = 320 - *pY;
	return;
//#NT#2015/08/25#KCHong#Modified for touch panel -end
}

void GxTouch_ResetCal(void)
{
	g_bCalibrated = FALSE;
}
