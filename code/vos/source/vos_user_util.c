/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define __MODULE__    vos_user_util
#define __DBGLVL__    2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/util.h>

#include <unistd.h>

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_UTIL_INITED_TAG       MAKEFOURCC('R', 'U', 'T', 'L') ///< a key value

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/

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
	if (-1 == msec) {
		//Assume -1 msec to be used for timeout, return -1 without error messages
		return -1;
	} else if (msec < 0) {
		DBG_ERR("invalid msec %d\r\n", msec);
		return -1;
	}

	return msec; //return the same msec as tick (Linux user-space Only)
}

void vos_util_delay_ms(int ms)
{
	if (ms < 0) {
		DBG_ERR("Invalid delay %d ms\r\n", ms);
		return;
	}

	usleep(ms*1000);
}

void vos_util_delay_us(int us)
{
	if (us < 0) {
		DBG_ERR("Invalid delay %d us\r\n", us);
		return;
	}

	usleep(us);
}

void vos_util_delay_us_polling(int us)
{
	DBG_WRN("not supported\r\n");
	usleep(us);
}