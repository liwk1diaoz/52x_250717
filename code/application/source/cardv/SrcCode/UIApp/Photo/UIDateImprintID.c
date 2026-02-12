//#include "SysKer.h"
#include "UIDateImprint.h"
#include "UIDateImprintID.h"

ID UI_DATEIMPRINT_FLG_ID = 0;

void UiDateImprint_InstallID(void)
{
	OS_CONFIG_FLAG(UI_DATEIMPRINT_FLG_ID);
}
void UiDateImprint_UnInstallID(void)
{
	vos_flag_destroy(UI_DATEIMPRINT_FLG_ID);
}

