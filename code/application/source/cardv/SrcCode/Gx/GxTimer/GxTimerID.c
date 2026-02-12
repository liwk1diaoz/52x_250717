
#include "GxTimer.h"
#include "GxTimer_int.h"
#include <kwrap/verinfo.h>

ID SEMID_GXTIMER = 0;

void GxTimer_InstallID(void)
{
	OS_CONFIG_SEMPHORE(SEMID_GXTIMER, 0, 1, 1);
}

