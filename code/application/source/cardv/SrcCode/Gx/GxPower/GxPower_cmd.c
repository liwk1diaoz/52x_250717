#include <stdio.h>
#include <string.h>
#include "GxPower.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include <kwrap/stdio.h>
#include "kwrap/debug.h"

extern GX_CALLBACK_PTR   g_fpPowerCB;


static BOOL Cmd_gxpower_autooffen(unsigned char argc, char **argv)
{
	UINT32 value = FALSE;
	if (argc < 1) return 0;
	sscanf_s(argv[0], "%d", &value);
	GxPower_SetControl(GXPWR_CTRL_AUTOPOWEROFF_EN, value);
	return TRUE;
}


static BOOL Cmd_gxpower_setofftime(unsigned char argc, char **argv)
{
	UINT32 time = 30;
	if (argc < 1) return 0;
	sscanf_s(argv[0], "%d", &time);
	GxPower_SetControl(GXPWR_CTRL_AUTOPOWEROFF_TIME, time);
	return TRUE;
}

static BOOL Cmd_gxpower_autopwroff(unsigned char argc, char **argv)
{
	if (g_fpPowerCB) {
		g_fpPowerCB(POWER_CB_POWEROFF, 0, 0);
	}
	return TRUE;
}


static BOOL Cmd_gxpower_battempty(unsigned char argc, char **argv)
{
	if (g_fpPowerCB) {
		g_fpPowerCB(POWER_CB_BATT_EMPTY, 0, 0);
	}
	return TRUE;
}

static BOOL Cmd_gxpower_battchange(unsigned char argc, char **argv)
{
	UINT32 value = 3;
	if (argc < 1) return 0;
	sscanf_s(argv[0], "%d", &value);
	if (g_fpPowerCB) {
		g_fpPowerCB(POWER_CB_BATT_CHG, 1, value);
	}
	return TRUE;
}

static BOOL Cmd_gxpower_chargecurrent(unsigned char argc, char **argv)
{
	UINT32 value = 1;
	if (argc < 1) return 0;
	sscanf_s(argv[0], "%d", &value);
	GxPower_SetControl(GXPWR_CTRL_BATTERY_CHARGE_CURRENT, value);
	return TRUE;
}


static BOOL Cmd_gxpower_chargeen(unsigned char argc, char **argv)
{
	UINT32 value = TRUE;
	if (argc < 1) return 0;
	sscanf_s(argv[0], "%d", &value);
	GxPower_SetControl(GXPWR_CTRL_BATTERY_CHARGE_EN, value);
	return TRUE;
}


#if 0
BOOL Cmd_gxpower_adc(unsigned char argc, char **argv)
{
	extern UINT32 gAdcValue;
	if (argc < 1) return 0;
	sscanf(argv[0], "%d", &gAdcValue);
	return TRUE;
}
#endif


static SXCMD_BEGIN(gxpower_cmd_tbl, "GxPower")
SXCMD_ITEM("autooffen %", Cmd_gxpower_autooffen, "enable auto power off, param value is 1 or 0")
SXCMD_ITEM("setofftime %", Cmd_gxpower_setofftime, "set auto power off time, param is auto off time(s)")
SXCMD_ITEM("autopwroff", Cmd_gxpower_autopwroff, "callback auto power off event")
SXCMD_ITEM("battempty", Cmd_gxpower_battempty, "callback battery emtpy event")
SXCMD_ITEM("battchange %", Cmd_gxpower_battchange, "callback battery level changed event, param value is battery level")
SXCMD_ITEM("chargecurrent %", Cmd_gxpower_chargecurrent, "set charge current, 0 is low, 1 is medium, 2 is high")
SXCMD_ITEM("chargeen %", Cmd_gxpower_chargeen, "enable battery charge, param value is 1 or 0")
SXCMD_END()

static int gxpower_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(gxpower_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "ker");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", gxpower_cmd_tbl[loop].p_name, gxpower_cmd_tbl[loop].p_desc);
	}
	return 0;
}

MAINFUNC_ENTRY(gxpower, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(gxpower_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		gxpower_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], gxpower_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = gxpower_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}

	return 0;
}

