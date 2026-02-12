#include "timer_int.h"
#include "emu_timer.h"

#define AUTOTEST_TIMER        ENABLE

static volatile UINT32 cb_count;
static volatile UINT32 emu_tmr_id;

static void emu_timer_callback(UINT32 event)
{
	cb_count++;
	if (cb_count & 0x01) {
		timer_reload(emu_tmr_id, 500000);
	} else {
		timer_reload(emu_tmr_id, 1000000);
	}
	printk("%s: tic: %d\r\n", __func__,
                timer_get_current_count(timer_get_sys_timer_id()));
}

AUTOTEST_RESULT emu_timer_auto(void)
{
	TIMER_ID timer_id;
	UINT32 tick_s, tick_e;

	printk("%s: sys tic: %d\r\n", __func__,
		timer_get_current_count(timer_get_sys_timer_id()));
	printk("%s: sys tic: %d\r\n", __func__,
		timer_get_current_count(timer_get_sys_timer_id()));
	printk("%s: sys tic: %d\r\n", __func__,
		timer_get_current_count(timer_get_sys_timer_id()));

	//
	// one shot delay
	//
	if (timer_open(&timer_id, emu_timer_callback) != E_OK) {
		printk("%s: timer open fail\r\n", __func__);
		return AUTOTEST_RESULT_FAIL;
	}
	emu_tmr_id = timer_id;
	cb_count = 0;

	printk("%s: get timer %d\r\n", __func__, timer_id);
	tick_s =  timer_get_current_count(timer_get_sys_timer_id());
	timer_cfg(timer_id, 800000, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	timer_wait_timeup(timer_id);
	tick_e =  timer_get_current_count(timer_get_sys_timer_id());
	while (cb_count == 0) {
		cond_resched();
	}

	timer_close(timer_id);
	printk("%s: end tick %d\r\n", __func__, tick_e);
	printk("%s: one shot timer %d elapse %d us\r\n", __func__,
			timer_id,
			tick_e-tick_s);

	//
	// free-run delay
	//
	if (timer_open(&timer_id, emu_timer_callback) != E_OK) {
                printk("%s: timer open fail\r\n", __func__);
                return AUTOTEST_RESULT_FAIL;
        }
	emu_tmr_id = timer_id;

        printk("%s: get timer %d\r\n", __func__, timer_id);
	cb_count = 0;
        tick_s =  timer_get_current_count(timer_get_sys_timer_id());
        timer_cfg(timer_id, 1000000, TIMER_MODE_FREE_RUN | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
	while (cb_count < 4) {
		cond_resched();
	}
        tick_e =  timer_get_current_count(timer_get_sys_timer_id());

        timer_close(timer_id);
	printk("%s: free run timer %d elapse %d us\r\n", __func__,
                        timer_id,
                        tick_e-tick_s);


	printk("%s: HZ %d\r\n", __func__, HZ);
	printk("%s: enter\r\n", __func__);

	return AUTOTEST_RESULT_OK;
}
