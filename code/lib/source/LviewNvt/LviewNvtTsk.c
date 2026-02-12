#include <stdio.h>
#include <string.h>
#include "kwrap/type.h"
#include "LviewNvtID.h"
#include "LviewNvtInt.h"
#include "LviewNvtTsk.h"
#include "LviewNvtIpc.h"
#include "LviewNvtNonIpc.h"
#include "LviewNvt/LviewNvtAPI.h"
#include "LviewNvt/lviewd.h"


///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          LviewNvtTsk
#define __DBGLVL__          2
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"

#define CFG_LVIEWNVT_INIT_KEY   MAKEFOURCC('L','V','I','E')

static LVIEW_OPS g_lviewOpObj =
{
    #if LVIEW_USE_IPC == ENABLE
	    LviewNvt_Ipc_Open,
	    LviewNvt_Ipc_Close,
	    LviewNvt_Ipc_PushFrame,
	#else
	    LviewNvt_NonIpc_Open,
	    LviewNvt_NonIpc_Close,
	    LviewNvt_NonIpc_PushFrame,
	#endif
};

static LVIEW_OPS *pLviewOp = &g_lviewOpObj;

static LVIEWD_CTRL m_Lviewd_Ctrl = {0};

LVIEWD_CTRL *xLviewNvt_GetCtrl(void)
{
	return &m_Lviewd_Ctrl;
}

ER  LviewNvt_StartDaemon(LVIEWNVT_DAEMON_INFO   *pOpen)
{
	LVIEWD_CTRL    *pCtrl;

	DBG_FUNC_BEGIN("\r\n");
	if (!pOpen) {
		DBG_ERR("pOpen is NULL\r\n");
		return E_PAR;
	}
	#if 0
	if (!pOpen->bitstream_mem_range.addr) {
		DBG_ERR("bitstream_mem_range is 0\r\n");
		return E_PAR;
	}
	#endif
	pCtrl = xLviewNvt_GetCtrl();
	if (pCtrl->uiInitKey == CFG_LVIEWNVT_INIT_KEY) {
		DBG_WRN("Already opened\r\n");
		return E_OK;
	}
	LviewNvt_InstallID();
	SEM_WAIT(LVIEWD_SEM_ID);
	memset(pCtrl, 0, sizeof(LVIEWD_CTRL));
	pCtrl->Open = *pOpen;
	pCtrl->FlagID = LVIEWD_FLG_ID;
	pCtrl->uiInitKey = CFG_LVIEWNVT_INIT_KEY;
	clr_flg(pCtrl->FlagID, 0xFFFFFFFF);
    if (pLviewOp->Open)
		pLviewOp->Open(pOpen);
	SEM_SIGNAL(LVIEWD_SEM_ID);
	DBG_FUNC_END("\r\n");
	return E_OK;
}
ER LviewNvt_StopDaemon(void)
{
	LVIEWD_CTRL *pCtrl = xLviewNvt_GetCtrl();

	DBG_FUNC_BEGIN("\r\n");
	SEM_WAIT(LVIEWD_SEM_ID);
	if (pCtrl->uiInitKey != CFG_LVIEWNVT_INIT_KEY) {
		DBG_WRN("Not opened\r\n");
		SEM_SIGNAL(LVIEWD_SEM_ID);
		return E_OK;
	}
	set_flg(pCtrl->FlagID, FLG_LVIEWD_STOP);
	if (pLviewOp->Close)
		pLviewOp->Close();
	pCtrl->uiInitKey = 0;
	SEM_SIGNAL(LVIEWD_SEM_ID);
	LviewNvt_UnInstallID();
	DBG_FUNC_END("\r\n");
	return E_OK;
}

ER LviewNvt_PushFrame(LVIEWNVT_FRAME_INFO *frame_info)
{
	static UINT32 gframeCount = 0;
	LVIEWD_CTRL *pCtrl = xLviewNvt_GetCtrl();

	if (frame_info == NULL) {
		DBG_ERR("frame_info = 0x%x\r\n", (int)frame_info);
		return E_SYS;
	}
	if (pCtrl->uiInitKey != CFG_LVIEWNVT_INIT_KEY) {
		DBG_WRN("Not opened\r\n");
		return E_OK;
	}
	SEM_WAIT(LVIEWD_SEM_ID);
	if (vos_flag_chk(pCtrl->FlagID, FLG_LVIEWD_STOP) == FLG_LVIEWD_STOP){
		SEM_SIGNAL(LVIEWD_SEM_ID);
		return E_OK;
	}
	gframeCount++;
	if ((gframeCount % 1800) == 0) {
		DBG_DUMP("frameCount=%d \r\n", gframeCount);
	}
	if (pLviewOp->PushFrame)
		pLviewOp->PushFrame(frame_info);
	SEM_SIGNAL(LVIEWD_SEM_ID);
	return E_OK;
}


