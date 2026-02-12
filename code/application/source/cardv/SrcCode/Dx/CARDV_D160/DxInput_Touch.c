/**
    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.

    @file       DetKey.c
    @ingroup    mIPRJAPKeyIO

    @brief      Scan key, modedial
                Scan key, modedial

    @note       Nothing.

    @date       2017/05/02
*/

/** \addtogroup mIPRJAPKeyIO */
//@{

#include "DxCfg.h"
#include "IOCfg.h"
#include "DxInput.h"
#include "KeyDef.h"
#include "comm/hwclock.h"
#include "comm/hwpower.h"
#if 0
#include "rtc.h"
#include "Delay.h"
#endif
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          DxKey
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

#if defined __FREERTOS
void DetTP_Init()
{


}
#endif

void DetTP_GetXY(INT32 *pX, INT32 *pY)
{
	*pX = 0;
	*pY = 0;
}

BOOL DetTP_IsPenDown(void)
{

	return FALSE;
}

