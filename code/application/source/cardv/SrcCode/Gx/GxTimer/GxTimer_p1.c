#include "GxCommon.h"
#include "GxTimer.h"

GX_CALLBACK_PTR g_fpTimerCB = NULL;

void GxTimer_RegCB(GX_CALLBACK_PTR fpTimerCB)
{
	g_fpTimerCB = fpTimerCB;
}

