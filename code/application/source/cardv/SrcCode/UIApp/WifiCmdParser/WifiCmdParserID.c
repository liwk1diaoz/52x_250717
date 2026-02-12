//#include "SysKer.h"
#include "WifiCmdParser.h"
#include "WifiCmdParserInt.h"

static UINT32 is_wificmd_installed = 0;

ID FLG_ID_WIFICMD = 0;
SEM_HANDLE WIFICMD_SEM_ID = 0;
SEM_HANDLE WIFISTR_SEM_ID = 0;

void WifiCmd_InstallID(void)
{
	is_wificmd_installed ++;
	if (is_wificmd_installed > 1) {
		return;
	}

	OS_CONFIG_FLAG(FLG_ID_WIFICMD);
#if 0
	OS_CONFIG_SEMPHORE(WIFICMD_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(WIFISTR_SEM_ID, 0, 1, 1);
#else
	SEM_CREATE(WIFICMD_SEM_ID, 1);
	SEM_CREATE(WIFISTR_SEM_ID, 1);
#endif
}

void WifiCmd_UninstallID(void)
{
	if (is_wificmd_installed) {
		is_wificmd_installed--;
		if (is_wificmd_installed) {
			return;
		}
	} else {
		return;
	}

	rel_flg(FLG_ID_WIFICMD);
	//jira: NA51055-1243
	FLG_ID_WIFICMD=0;
#if 0
	OS_CONFIG_SEMPHORE(WIFICMD_SEM_ID, 0, 1, 1);
	OS_CONFIG_SEMPHORE(WIFISTR_SEM_ID, 0, 1, 1);
#else
	SEM_DESTROY(WIFICMD_SEM_ID);
	SEM_DESTROY(WIFISTR_SEM_ID);
#endif
}

