
#include "AppControl/AppControl.h"



extern INT32 Ux_SendEventInternal(VControl *pCtrl, NVTEVT evt, UINT32 paramNum, UINT32 *paramArray);

/**
@}
*/
INT32 Ux_AppDispatchMessage(NVTEVT evt, UINT32 paramNum, UINT32 *paramArray)
{
	INT32 result = NVTEVT_PASS;
	VControl *pAppCtrl = NULL;

	pAppCtrl = Ux_GetActiveApp();
	if (pAppCtrl) {
		result = Ux_SendEventInternal(pAppCtrl, evt, paramNum, paramArray);
	}
	return result;
}

