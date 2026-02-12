#include "PrjInc.h"

////////////////////////////////////////////////////////////////////////////////
#include "SysCommon.h"
//#include "AppCommon.h"
////////////////////////////////////////////////////////////////////////////////
#include "UIApp/AppLib.h"
#include "Mode/UIMode.h"
#include "Mode/UIModeUpdFw.h"
#if(WIFI_FUNC==ENABLE)
#include "UIModeWifi.h"
#endif
#include "UIApp/Setup/UISetup.h"
#include "UIFrameworkExt.h"
#include "UIWnd/UIFlow.h"
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          UIModeUpdFw
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>

#if (CALIBRATION_FUNC == ENABLE)
#include "EngineerMode.h"
#endif

int PRIMARY_MODE_UPDFW = -1;      ///< UpdFw
int SYS_SUBMODE_UPDFW = -1;
void ModeUpdFw_Open(void);
void ModeUpdFw_Close(void);

void ModeUpdFw_Open(void)
{
    DBG_FUNC_BEGIN("\r\n");
	Ux_SetActiveApp(&UISetupObjCtrl);
	Ux_SendEvent(0, NVTEVT_EXE_OPEN, 0);
#if(UI_FUNC==ENABLE)
	if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_NORMAL) {
		//Ux_OpenWindow((VControl *)(&UIUpdFwWndCtrl), 0);
	} else {
		SYS_SUBMODE *pCurSubMode = System_GetSubModeCtrl();
		if (pCurSubMode && pCurSubMode->pfSubMode_OnOpen) {
			pCurSubMode->pfSubMode_OnOpen();
		}
	}
#endif
    DBG_FUNC_END("\r\n");

}
void ModeUpdFw_Close(void)
{
    DBG_FUNC_BEGIN("\r\n");
#if(UI_FUNC==ENABLE)
	if (System_GetState(SYS_STATE_CURRSUBMODE) == SYS_SUBMODE_NORMAL) {
		//Ux_CloseWindow((VControl *)(&UIUpdFwWndCtrl), 0);
	} else {
		SYS_SUBMODE *pCurSubMode = System_GetSubModeCtrl();
		if (pCurSubMode && pCurSubMode->pfSubMode_OnClose) {
			pCurSubMode->pfSubMode_OnClose();
		}
	}
#endif

	Ux_SendEvent(0, NVTEVT_EXE_CLOSE, 0);
    DBG_FUNC_END("\r\n");

}

SYS_MODE gModeUpdFw = {
	"UPDFW",
	ModeUpdFw_Open,
	ModeUpdFw_Close,
	NULL,
	NULL,
	NULL,
	NULL
};

SYS_SUBMODE gSubModeUpdFw = {
	"SUBUPDFW",
	NULL,
	NULL,
};
