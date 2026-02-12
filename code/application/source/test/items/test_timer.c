#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "kwrap/type.h"
#include "kwrap/examsys.h"
#include "kwrap/sxcmd.h"
#include "timer.h"
#include "Utility/SwTimer.h"

#define __MODULE__          test_timer
#define __DBGLVL__          6 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"

SWTIMER_ID   SwTimerID1, SwTimerID2;
TIMER_ID     timer_id1, timer_id2, timer_id3;

void hw_cb1(UINT32 uiEvent)
{
	DBG_DUMP("hw_cb1 uiEvent = 0x%x\r\n", uiEvent);
}
void hw_cb2(UINT32 uiEvent)
{
	DBG_DUMP("hw_cb2 uiEvent = 0x%x\r\n", uiEvent);
}
void hw_cb3(UINT32 uiEvent)
{
	DBG_DUMP("hw_cb3 uiEvent = 0x%x\r\n", uiEvent);
}


void sw_cb1(UINT32 uiEvent)
{
	DBG_DUMP("sw_cb1 uiEvent = 0x%x\r\n", uiEvent);
}

void sw_cb2(UINT32 uiEvent)
{
	DBG_DUMP("sw_cb2 uiEvent = 0x%x\r\n", uiEvent);
}


static BOOL test_timer_start(unsigned char argc, char **argv)
{
	DBG_DUMP("test_timer start\r\n");
	timer_open(&timer_id1, hw_cb1);
	timer_cfg(timer_id1, 1000000, TIMER_MODE_CLKSRC_DIV0 | TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	timer_open(&timer_id2, hw_cb2);
	timer_cfg(timer_id2, 500000, TIMER_MODE_CLKSRC_DIV0 | TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	timer_open(&timer_id3, hw_cb3);
	timer_cfg(timer_id3, 390000, TIMER_MODE_CLKSRC_DIV0 | TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);



	SwTimer_Open(&SwTimerID1, sw_cb1);
	SwTimer_Cfg(SwTimerID1, 500, SWTIMER_MODE_FREE_RUN);
	SwTimer_Start(SwTimerID1);
	SwTimer_Open(&SwTimerID2, sw_cb2);
	SwTimer_Cfg(SwTimerID2, 1000, SWTIMER_MODE_FREE_RUN);
	SwTimer_Start(SwTimerID2);
	return TRUE;
}


static BOOL test_timer_stop(unsigned char argc, char **argv)
{
	timer_close(timer_id1);
	timer_close(timer_id2);
	timer_close(timer_id3);
	SwTimer_Close(SwTimerID1);
	SwTimer_Close(SwTimerID2);
	return TRUE;
}


static SXCMD_BEGIN(timer_cmd_tbl, "test_timer")
SXCMD_ITEM("start", test_timer_start, "")
SXCMD_ITEM("stop", test_timer_stop, "")
SXCMD_END()


static int timer_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(timer_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "hfs");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", timer_cmd_tbl[loop].p_name, timer_cmd_tbl[loop].p_desc);
	}
	return 0;
}

EXAMFUNC_ENTRY(test_timer, argc, argv)
{
	UINT32 cmd_num = SXCMD_NUM(timer_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		timer_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], timer_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = timer_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		timer_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}

