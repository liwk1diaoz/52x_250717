#ifdef __KERNEL__
#include <linux/interrupt.h>
#include <linux/signal.h>
#include <linux/kernel.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/ctype.h>
#endif
#include "timer_int.h"
#include "comm/hwclock.h"

#include <kwrap/spinlock.h>
static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#define HWCLOCK_INIT_TAG       MAKEFOURCC('H', 'W', 'C', 'K') ///< a key value
#define LONG_COUNTER_PERIOD    1000000000                     ///< 1000 sec

typedef struct {
	UINT32         long_counter_init;
	TIMER_ID       long_counter_timerid;
	UINT64         long_counter_sample_cnt;
	UINT32         long_counter_offset;
	UINT32         last_sys_timer_count;
} HWCLOCK_CTRL;

static HWCLOCK_CTRL  hwclock_ctrl;
static BOOL _long_counter_ready = FALSE;

static void hwclock_timer_cb_p(UINT32 event)
{
	unsigned long      flags;
	HWCLOCK_CTRL      *pctrl = &hwclock_ctrl;

	loc_cpu(flags);
	pctrl->last_sys_timer_count = timer_get_current_count(TIMER_SYSTIMER_ID);
	pctrl->long_counter_sample_cnt++;
	unl_cpu(flags);
}


void hwclock_init(void)
{
	ER             ret;
	HWCLOCK_CTRL   *pctrl = &hwclock_ctrl;

	if (HWCLOCK_INIT_TAG == pctrl->long_counter_init)
		return;

	// Due to timer's HW restriction, the max interval value is 0xFFFFFFFF us
	// Init long counter
	if ((ret = timer_open(&pctrl->long_counter_timerid, hwclock_timer_cb_p)) == E_OK) {
		timer_cfg(pctrl->long_counter_timerid, LONG_COUNTER_PERIOD, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		pctrl->long_counter_offset = pctrl->last_sys_timer_count = timer_get_current_count(TIMER_SYSTIMER_ID);
		pctrl->long_counter_sample_cnt = pctrl->long_counter_offset/LONG_COUNTER_PERIOD;
		pctrl->long_counter_offset = pctrl->long_counter_offset % LONG_COUNTER_PERIOD;
		pctrl->long_counter_init = HWCLOCK_INIT_TAG;
		_long_counter_ready = TRUE;
	} else {
		DBG_ERR("init fail!(ret=%d)\r\n", ret);
	}
}

UINT32 hwclock_get_counter(void)
{
	return timer_get_current_count(TIMER_SYSTIMER_ID);
}

UINT64 hwclock_get_longcounter(void)
{
	UINT64             count = 0;
	unsigned long      flags;
	UINT32             curr_long_counter;
	HWCLOCK_CTRL       *pctrl = &hwclock_ctrl;

	if (HWCLOCK_INIT_TAG != pctrl->long_counter_init) {
		return timer_get_current_count(TIMER_SYSTIMER_ID);
	}
	loc_cpu(flags);
	curr_long_counter = timer_get_current_count(pctrl->long_counter_timerid);
	// check if timer timeout , but cb still not running
	if(timer_get_current_count(TIMER_SYSTIMER_ID) - pctrl->last_sys_timer_count >= LONG_COUNTER_PERIOD/10*9 && curr_long_counter < LONG_COUNTER_PERIOD/10*5 ) {
		count = (pctrl->long_counter_sample_cnt+1)*LONG_COUNTER_PERIOD + curr_long_counter + pctrl->long_counter_offset;
	} else {
		count = (pctrl->long_counter_sample_cnt)*LONG_COUNTER_PERIOD + curr_long_counter + pctrl->long_counter_offset;
	}
	unl_cpu(flags);
	return count;
}

UINT64 hwclock_diff_longcounter(UINT64 time_start, UINT64 time_end)
{
	UINT32 time_start_sec = 0;
	UINT32 time_start_usec = 0;
	UINT32 time_end_sec=0;
	UINT32 time_end_usec =0;
	INT32  diff_time_sec =0 ;
	INT32  diff_time_usec = 0;
	UINT64 diff_time;

	time_start_sec = (time_start >> 32) & 0xFFFFFFFF;
	time_start_usec = time_start & 0xFFFFFFFF;
	time_end_sec = (time_end >> 32) & 0xFFFFFFFF;
	time_end_usec = time_end & 0xFFFFFFFF;
	diff_time_sec = (INT32)time_end_sec - (INT32)time_start_sec;
	diff_time_usec = (INT32)time_end_usec - (INT32)time_start_usec;
	diff_time = (INT64)diff_time_sec * 1000000 + diff_time_usec;
	return diff_time;
}

BOOL hwclock_is_longcounter_ready(void)
{
	return _long_counter_ready;
}


/**
     Un-initialize system timer.

*/
void hwclock_exit(void)
{
	HWCLOCK_CTRL       *pctrl = &hwclock_ctrl;

	if (HWCLOCK_INIT_TAG != pctrl->long_counter_init)
		return;
	timer_close(pctrl->long_counter_timerid);
	pctrl->long_counter_timerid = TIMER_INVALID;
	pctrl->long_counter_init = 0;
}

#ifdef __KERNEL__
/*-----------------------------------------------------------------------------*/
/* Kernel Mode Definiton                                                       */
/*-----------------------------------------------------------------------------*/
EXPORT_SYMBOL(hwclock_get_counter);
EXPORT_SYMBOL(hwclock_get_longcounter);
EXPORT_SYMBOL(hwclock_diff_longcounter);
EXPORT_SYMBOL(hwclock_is_longcounter_ready);
#endif
