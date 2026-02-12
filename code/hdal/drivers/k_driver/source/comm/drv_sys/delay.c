
#include "kwrap/type.h"
#include "kwrap/error_no.h"

#include "comm/timer.h"

#ifdef _NVT_FPGA_
#define POLLING_THRESHOLD	2000
#else
#define POLLING_THRESHOLD	40
#endif


/**
    Delay in microsecond via polling.

    Delay in microsecond via polling. The actual delay time might be greater than
    expected. The caller won't sleep but polling until timer is expired.

    @param[in] micro_sec     Number of microsecond to delay. 1 ~ 4,294,967,295 (0xFFFF_FFFF).
    @return Void.
*/
void delay_us_poll(UINT32 micro_sec)
{
	TIMER_ID    timer_id;
	UINT32      uiBase;

	// The number of microsecond must be greater than 0
	if (micro_sec == 0) {
		return;
	}

	timer_id     = timer_get_sys_timer_id();
	uiBase      = timer_get_current_count(timer_id);

	// The resolution of timer tick is 1 us, to make sure the delay period is not shorter
	// than expected, we have to add 1 to delay time
	micro_sec++;

	while (1) {
		if ((timer_get_current_count(timer_id) - uiBase) >= micro_sec) {
			break;
		}
	}
}


/**
    Delay in microsecond

    Delay in microsecond. The actual delay time might be greater than expected.
    The caller will sleep and release CPU until delay timer is expired. If the
    number of microsecond is <= the threshold (default is 40, can be configured
    via Delay_SetUsThreshold()), the caller won't sleep but polling until timer
    is expired.

    @note   This function can't be called under ISR.

    @param[in] micro_sec     Number of microsecond to delay. 1 ~ 4,294,967,295 (0xFFFF_FFFF).
    @return Void.
*/
void delay_us(UINT32 micro_sec)
{
	TIMER_ID    timer_id;

	// The number of microsecond must be greater than 0
	if (micro_sec == 0) {
		return;
	}

	// Check if the expected delay <= threshold
	if (micro_sec <= POLLING_THRESHOLD) {
		return delay_us_poll(micro_sec);
	}

	// Open timer
	// There is no available HW timer, polling system timer
	if (timer_open(&timer_id, NULL) != E_OK) {
		delay_us_poll(micro_sec);
	}
	// Configure timer and wait for timer expired
	else {
		timer_cfg(timer_id, micro_sec, TIMER_MODE_ONE_SHOT | TIMER_MODE_ENABLE_INT, TIMER_STATE_PLAY);
		timer_wait_timeup(timer_id);
		timer_close(timer_id);
	}

}



