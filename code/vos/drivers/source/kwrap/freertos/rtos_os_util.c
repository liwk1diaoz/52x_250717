/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define __MODULE__    rtos_util
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/util.h>

#include <FreeRTOS.h>
#include <task.h> //FreeRTOS header file

#include "freertos_ext_kdrv.h"
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_UTIL_INITED_TAG       MAKEFOURCC('R', 'U', 'T', 'L') ///< a key value

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_util_debug_level = NVT_DBG_WRN;


/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void rtos_util_init(void *param)
{
	return;
}

void rtos_util_exit(void)
{
}

int vos_util_msec_to_tick(int msec)
{
	int tick;

	if (-1 == msec) {
		//Assume -1 msec to be used for timeout, return -1 without error messages
		return -1;
	} else if (msec < 0) {
		DBG_ERR("invalid msec %d\r\n", msec);
		return -1;
	}

#if configTICK_RATE_HZ < 1000
	tick = ((msec * configTICK_RATE_HZ + 999) / 1000); //round up
#else
	tick = ((msec * configTICK_RATE_HZ) / 1000);
#endif

	return tick;
}

void vos_util_delay_ms(int ms)
{
	if (VOS_UTIL_DELAY_INFINITELY == ms) {
		vTaskDelay(portMAX_DELAY); //wait infinitely (FreeRTOS Only)
	}

	if (ms < 0) {
		DBG_ERR("Invalid delay %d ms\r\n", ms);
		return;
	}

	if (ms > (0x1 << 22)) {
		DBG_ERR("ms %d too large\r\n", ms);
		return;
	}

	delay_us(1000*ms);
}

void vos_util_delay_us(int us)
{
	if (us < 0) {
		DBG_ERR("Invalid delay %d us\r\n", us);
		return;
	}

	delay_us(us);
}

void vos_util_delay_us_polling(int us)
{
	if (us < 0) {
		DBG_ERR("Invalid delay %d us\r\n", us);
		return;
	}

	delay_us_poll(us);
}

