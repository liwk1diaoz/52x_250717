//global debug level: PRJ_DBG_LVL
#include "PrjInc.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include <kwrap/stdio.h>
#include "GxPower.h"
#include "PowerDef.h"

//local debug level: THIS_DBGLVL
#define THIS_DBGLVL         2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          SysPwrCmd
#define __DBGLVL__          ((THIS_DBGLVL>=PRJ_DBG_LVL)?THIS_DBGLVL:PRJ_DBG_LVL)
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include <kwrap/debug.h>
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// power input
///////////////////////////////////////////////////////////////////////////////

static BOOL Cmd_power_src(unsigned char argc, char **argv)
{
	UINT32 src = System_GetPowerOnSource(); //get src
	DBG_DUMP("=> power on src = %d\r\n", src);
	return TRUE;
}
static BOOL Cmd_power_boot(unsigned char argc, char **argv)
{
	DBG_DUMP("=> Boot flow\r\n");
	System_PowerOnFinish(); //boot start
	return TRUE;
}
static BOOL Cmd_power_off(unsigned char argc, char **argv)
{
	DBG_DUMP("=> Shutdown flow + Power Off\r\n");
	//GxPower_PowerOff(); //shutdown start
	Ux_PostEvent(NVTEVT_SYSTEM_SHUTDOWN, 1, 0); //shutdown start
	return TRUE;
}
static BOOL Cmd_power_reset(unsigned char argc, char **argv)
{
	DBG_DUMP("=> Shutdown flow + H/W Reset\r\n");
	System_EnableHWReset(0);
	//System_EnableHWResetByAlarmTime(0, 0, 0, 10);
	Ux_PostEvent(NVTEVT_SYSTEM_SHUTDOWN, 1, 0); //shutdown start
	return TRUE;
}
static BOOL Cmd_power_swreset(unsigned char argc, char **argv)
{
	DBG_DUMP("=> Shutdown flow + S/W Reset\r\n");
	System_EnableSWReset(0);
	Ux_PostEvent(NVTEVT_SYSTEM_SHUTDOWN, 1, 0); //shutdown start
	return TRUE;
}
static BOOL Cmd_power_sleep(unsigned char argc, char **argv)
{
	DBG_DUMP("=> Sleep 1\r\n");
	Ux_PostEvent(NVTEVT_SYSTEM_SLEEP, 1, 1);
	Delay_DelayMs(2*1000);
	DBG_DUMP("=> Sleep 2\r\n");
	Ux_PostEvent(NVTEVT_SYSTEM_SLEEP, 1, 2);
	Delay_DelayMs(2*1000);
	DBG_DUMP("=> Sleep 3\r\n");
	Ux_PostEvent(NVTEVT_SYSTEM_SLEEP, 1, 3);
	return TRUE;
}
static BOOL Cmd_power_wakeup(unsigned char argc, char **argv)
{
	DBG_DUMP("=> Wakeup 0\r\n");
	Ux_PostEvent(NVTEVT_SYSTEM_SLEEP, 1, 0);
	return TRUE;
}

static SXCMD_BEGIN(power_cmd_tbl, "power command")
SXCMD_ITEM("src", Cmd_power_src, "src")
SXCMD_ITEM("boot", Cmd_power_boot, "boot")
SXCMD_ITEM("off", Cmd_power_off, "off")
SXCMD_ITEM("reset", Cmd_power_reset, "reset")
SXCMD_ITEM("swreset", Cmd_power_swreset, "swreset")
SXCMD_ITEM("sleep", Cmd_power_sleep, "sleep")
SXCMD_ITEM("wakeup", Cmd_power_wakeup, "wakeup")
SXCMD_END()

static int power_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(power_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "ker");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", power_cmd_tbl[loop].p_name, power_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(power, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(power_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		power_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], power_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = power_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}

	return 0;
}



