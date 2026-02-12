#include "MsdcNvtInt.h"
#include <string.h>

void xMsdcNvt_MemPushHostToBk(void)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	MSDCNVT_BACKGROUND_CTRL *pBk = &pCtrl->tBkCtrl;

	pBk->uiTransmitSize = pHost->uiLB_Length;
	memcpy(pBk->pPoolMem, pHost->pPoolMem, pBk->uiTransmitSize);
}

void xMsdcNvt_MemPopBkToHost(void)
{
	MSDCNVT_CTRL *pCtrl = xMsdcNvt_GetCtrl();
	MSDCNVT_HOST_INFO *pHost = &pCtrl->tHostInfo;
	MSDCNVT_BACKGROUND_CTRL *pBk = &pCtrl->tBkCtrl;
	memcpy(pHost->pPoolMem, pBk->pPoolMem, pBk->uiTransmitSize);
}

BOOL xMsdcNvt_Bk_RunCmd(void (*pCall)(void))
{
	MSDCNVT_BACKGROUND_CTRL *pBk = &xMsdcNvt_GetCtrl()->tBkCtrl;

	if (pBk->bCmdRunning) {
		DBG_ERR("Is Running\r\n");
		return FALSE;
	}

	pBk->bCmdRunning = TRUE;
	pBk->fpCall = pCall;
	pBk->TaskID = vos_task_create(MsdcNvtTsk, NULL, "MsdcNvt", 10, 4096);
	vos_task_resume(pBk->TaskID);

	return TRUE;
}

BOOL xMsdcNvt_Bk_IsFinish(void)
{
	if (xMsdcNvt_GetCtrl()->tBkCtrl.bCmdRunning) {
		return FALSE;
	}

	return TRUE;
}

BOOL xMsdcNvt_Bk_HostLock(void)
{
	MSDCNVT_BACKGROUND_CTRL *pBk = &xMsdcNvt_GetCtrl()->tBkCtrl;

	if (pBk->bServiceLock) {
		DBG_ERR("already locked!\r\n");
		return FALSE;
	}

	pBk->bServiceLock = TRUE;
	return TRUE;
}

BOOL xMsdcNvt_Bk_HostUnLock(void)
{
	MSDCNVT_BACKGROUND_CTRL *pBk = &xMsdcNvt_GetCtrl()->tBkCtrl;

	if (!pBk->bServiceLock) {
		DBG_ERR("not locked!\r\n");
		return FALSE;
	} else if (pBk->bCmdRunning) {
		DBG_ERR("Bk is Running\r\n");
		return FALSE;
	}

	pBk->bServiceLock = FALSE;
	return TRUE;
}

BOOL xMsdcNvt_Bk_HostIsLock(void)
{
	return xMsdcNvt_GetCtrl()->tBkCtrl.bServiceLock;
}

THREAD_DECLARE(MsdcNvtTsk, arglist)
{
	MSDCNVT_BACKGROUND_CTRL *pBk = &xMsdcNvt_GetCtrl()->tBkCtrl;
	vos_task_enter();

	PROFILE_TASK_IDLE();
	if (pBk->fpCall) {
		pBk->fpCall();
	}
	PROFILE_TASK_BUSY();

	pBk->bCmdRunning = FALSE;
	THREAD_RETURN(0);
}
