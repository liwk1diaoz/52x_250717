#if defined(__LINUX)
#include <linux/ctype.h>
#include <linux/string.h>
#else
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#endif
#include "kwrap/spinlock.h"
#include "kwrap/task.h"
#include "kwrap/util.h"
#include "kwrap/error_no.h"
#include "kwrap/cmdsys.h"
#include "kwrap/sxcmd.h"
#include <plat/top.h>
#include "comm/timer.h"
#include "heavyload.h"

#define __MODULE__          heavyload
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"

#define THREAD_COUNT         3

typedef struct _HEAVY_LOAD_CTRL {
	UINT32                   init;
	UINT32                   start;
	THREAD_HANDLE            thread[THREAD_COUNT];
	UINT32                   thread_busy_loop_cnt;
	UINT32                   heavy_load_type;
	UINT32                   trig_sleep;
	TIMER_ID                 timerid;
} HEAVY_LOAD_CTRL;

static HEAVY_LOAD_CTRL  heavy_load_ctrl;

static VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags)   vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags)   vk_spin_unlock_irqrestore(&my_lock, flags)

#if defined(__LINUX)
#define THREAD_BUSY_LOOP_CNT    3
#define THREAD_BUSY_LOOP_US     150000
#define TRIG_SLEEP_MS           20
#define TIMER_PERIOD_US         200
#define TIMER_BUSY_LOOP_US      180
#else
#define THREAD_BUSY_LOOP_CNT    1
#define THREAD_BUSY_LOOP_US     150000
#define TRIG_SLEEP_MS           100
#define TIMER_PERIOD_US         500
#define TIMER_BUSY_LOOP_US      300
#endif

static THREAD_DECLARE(heavyload_func, arglist)
{
	HEAVY_LOAD_CTRL        *p_ctrl;
	UINT32                 interrupt_count = 0;
	unsigned long          flags = 0, i;

	p_ctrl = arglist;
	while (!THREAD_SHOULD_STOP) {
		if (!p_ctrl->start) {
			break;
		}
		loc_cpu(flags);
		for (i = 0;i < p_ctrl->thread_busy_loop_cnt; i++) {
			vos_util_delay_us_polling(THREAD_BUSY_LOOP_US);
		}
		unl_cpu(flags);
		vos_util_delay_ms(p_ctrl->trig_sleep);
		for (i = 0;i < p_ctrl->thread_busy_loop_cnt; i++) {
			vos_util_delay_us_polling(THREAD_BUSY_LOOP_US);
		}
		if (0 == (interrupt_count & 0xFF)) {
			DBG_DUMP("heavyload interrupt_count %d\r\n", (int)interrupt_count);
		}
		interrupt_count++;
	}
	THREAD_RETURN(0);
}

static void heavyload_timer_cb_p(UINT32 event)
{
	vos_util_delay_us_polling(TIMER_BUSY_LOOP_US);
	timer_cfg(heavy_load_ctrl.timerid, TIMER_PERIOD_US, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
}

static void heavyload_init(HEAVY_LOAD_CTRL  *p_ctrl)
{
	int ret;

	if (0 == p_ctrl->init) {
		memset(p_ctrl, 0x00, sizeof(HEAVY_LOAD_CTRL));
		p_ctrl->heavy_load_type = 0;
		p_ctrl->init = 0;
		if ((ret = timer_open(&p_ctrl->timerid, heavyload_timer_cb_p)) == E_OK) {
			timer_cfg(p_ctrl->timerid, TIMER_PERIOD_US, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		} else {
			DBG_ERR("timer_open fail\r\n");
		}
	}
}

#if 0
static void heavyload_exit(HEAVY_LOAD_CTRL  *p_ctrl)
{
	memset(p_ctrl, 0x00, sizeof(HEAVY_LOAD_CTRL));
}
#endif

static BOOL heavyload_cmd_start(unsigned char argc, char **argv)
{
	UINT32               is_start = 1;
	int                  thread_pri = 1;
	int                  trig_sleep = TRIG_SLEEP_MS;
	HEAVY_LOAD_CTRL      *p_ctrl;
	int                  i, thread_count;

	p_ctrl = &heavy_load_ctrl;

	// read parameter from command
	//sscanf_s(str_cmd, "%d %d %d", &is_start, &p_ctrl->trig_sleep, &thread_pri);
	DBG_DUMP("heavyload_cmd argc = %d\r\n", argc);
	if (argc >= 1) {
		sscanf(argv[0], "%d", (int *)&is_start);
	}
	if (argc >= 2) {
		sscanf(argv[1], "%d", (int *)&trig_sleep);
	}
	if (argc >= 3) {
		sscanf(argv[2], "%d", (int *)&thread_pri);
	}

	// start heavy load
	if (is_start) {
		if (1 == p_ctrl->start) {
			DBG_WRN("heavyload already started\r\n");
			return FALSE;
		}
		DBG_DUMP("start heavyload\r\n");
		DBG_DUMP("trig_sleep = %d ms, thread_pri = %d\r\n", (int)trig_sleep, thread_pri);
		heavyload_init(p_ctrl);
		p_ctrl->start = 1;
		p_ctrl->trig_sleep = trig_sleep;
		if( nvt_get_chip_id() == CHIP_NA51084) {
			thread_count = 3;
			p_ctrl->thread_busy_loop_cnt = 3;
		} else {
			thread_count = 1;
			p_ctrl->thread_busy_loop_cnt = 1;
		}
		for (i = 0; i < thread_count; i++) {
			THREAD_CREATE(p_ctrl->thread[i], heavyload_func, p_ctrl, "heavyload_thread");
			if (0 == p_ctrl->thread[i]) {
				DBG_ERR("creating heavyload_tsk fail.\r\n");
				return FALSE;
			}
			THREAD_RESUME(p_ctrl->thread[i]);
			if (thread_pri > 0) {
				THREAD_SET_PRIORITY(p_ctrl->thread[i], thread_pri);
			}
		}
	} else {
	// stop heavy load
		if (1 != p_ctrl->start) {
			DBG_WRN("heavyload not started\r\n");
			return FALSE;
		}
		DBG_DUMP("stop heavyload \r\n");
		p_ctrl->start = 0;
		//THREAD_DESTROY(p_ctrl->thread);
		DBG_DUMP("heavyload exit\r\n");
		//heavyload_exit(p_ctrl);

	}
	return TRUE;
}

static SXCMD_BEGIN(heavyload_cmd_tbl, "heavyload")
SXCMD_ITEM("start",        heavyload_cmd_start,         "start heavyload")
SXCMD_END()

int heavyload_cmd_showhelp(int (*dump)(const char *fmt, ...))
{
	UINT32 cmd_num = SXCMD_NUM(heavyload_cmd_tbl);
	UINT32 loop = 1;

	dump("---------------------------------------------------------------------\r\n");
	dump("  %s\n", "heavyload");
	dump("---------------------------------------------------------------------\r\n");

	for (loop = 1 ; loop <= cmd_num ; loop++) {
		dump("%15s : %s\r\n", heavyload_cmd_tbl[loop].p_name, heavyload_cmd_tbl[loop].p_desc);
	}
	return 0;
}

#if defined(__LINUX)
int heavyload_cmd_execute(unsigned char argc, char **argv)
#else
MAINFUNC_ENTRY(heavyload, argc, argv)
#endif
{
	UINT32 cmd_num = SXCMD_NUM(heavyload_cmd_tbl);
	UINT32 loop;
	int    ret;

	if (argc < 2) {
		return -1;
	}
	if (strncmp(argv[1], "?", 2) == 0) {
		heavyload_cmd_showhelp(vk_printk);
		return 0;
	}
	for (loop = 1 ; loop <= cmd_num ; loop++) {
		if (strncmp(argv[1], heavyload_cmd_tbl[loop].p_name, strlen(argv[1])) == 0) {
			ret = heavyload_cmd_tbl[loop].p_func(argc-2, &argv[2]);
			return ret;
		}
	}
	if (loop > cmd_num) {
		DBG_ERR("Invalid CMD !!\r\n");
		heavyload_cmd_showhelp(vk_printk);
		return -1;
	}
	return 0;
}

