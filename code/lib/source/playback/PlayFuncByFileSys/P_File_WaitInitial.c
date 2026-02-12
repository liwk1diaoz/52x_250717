#include "PlaybackTsk.h"
#include "PlaySysFunc.h"
#include "PBXFile.h"
#include "Utility/SwTimer.h"


/*
    Wait for file system initial.
    This is internal API.

    @return E_OK: OK, E_SYS: NG
*/
INT32 PB_WaitFileSystemInit(void)
{
	//FLGPTN uiFlag;
	UINT32 i;

	//PB_TimerStart(gMenuPlayInfo.PlayTimeOutID, 5); // 5ms
	for (i = 0; i < 1200; i++) { // 10s timeout
		// wait for filesystem has opened, flag don't clear
		if (PBXFile_WaitInit()) {
			//PB_TimerPause(gMenuPlayInfo.PlayTimeOutID);
			DBG_IND("Wait FileSys Init OK\r\n");

			return E_OK;
		}
		//wai_flg(&uiFlag, FLG_PB, FLGPB_TIMEOUT, TWF_ORW | TWF_CLR);
	}

	//PB_TimerPause(gMenuPlayInfo.PlayTimeOutID);
	DBG_ERR("Wait FileSys Init FAIL\r\n");

	return E_SYS;
}
