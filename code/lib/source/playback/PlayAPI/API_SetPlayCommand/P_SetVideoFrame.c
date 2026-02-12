#include "PlaybackTsk.h"
#include "PlaySysFunc.h"

void PB_Set1stVideoFrame(PB_DISP_IDX DispIdx, PURECT pRect)
{
	if (PBDISP_IDX_SEC == DispIdx) {
		memcpy(&g_PB1stVdoRect[PBDISP_IDX_SEC], pRect, sizeof(URECT));
	} else {
		memcpy(&g_PB1stVdoRect[PBDISP_IDX_PRI], pRect, sizeof(URECT));
	}
}

