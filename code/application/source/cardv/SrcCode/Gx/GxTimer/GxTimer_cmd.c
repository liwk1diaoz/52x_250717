#include "GxTimer.h"
#include "kwrap/sxcmd.h"
#include "GxTimer_int.h"
#if 0
BOOL Cmd_gxtimer_dump(unsigned char argc, char **pargv);
static SXCMD_BEGIN(gxtimer, "GxTimer")
SXCMD_ITEM("dump", Cmd_gxtimer_dump, "Dump GxTimer status")
SXCMD_END()
BOOL Cmd_gxtimer_dump(unsigned char argc, char **pargv)
{
	GxTimer_Dump();
	return TRUE;
}
void xGxTimer_InstallCmd(void)
{
	sxcmd_addtable(gxtimer);
}
#endif
