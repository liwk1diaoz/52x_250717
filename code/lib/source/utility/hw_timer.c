/*
    Timer module driver

    This file is the driver of timer module

    @file       timer.c
    @ingroup    miDrvTimer_Timer
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/
/****************************************************************************/
/*                                                                          */
/*  Todo: Modify register accessing code to the macro in RCWMacro.h         */
/*                                                                          */
/****************************************************************************/

//--- Linux header file
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#include "timer.h"
#include "comm/timer_ioctl.h"
#include <kwrap/error_no.h>
#include <kwrap/task.h>
#include <kwrap/semaphore.h>


///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          utimer
#define __DBGLVL__          2 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define _TIMER_LIMIT_		20
#define _TIMER_             "/dev/nvt_timer_module0"
/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Function Protype                                                      */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct {
	int                          fd;
	TIMER_CB                     timer_cb[_TIMER_LIMIT_];
	UINT32                       timer_id_all;   //
	SEM_HANDLE                   semid;
	THREAD_HANDLE                tsk_hdl;
} timer_ctrl;
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static timer_ctrl g_timer_ctrl;


ER timer_rcv_event(UINT32 *event)
{
	int ret;
	TIMER_IOC_RCV_EVENT  ioc_event;

	ioc_event.timer_id_all = g_timer_ctrl.timer_id_all;
	ret = ioctl(g_timer_ctrl.fd, IOCTL_TIMER_RCV_EVENT, &ioc_event);
	if(ret < 0) {
		printf("wait err %d\n", ret);
	}
	*event = ioc_event.event;
	return E_OK;
}


THREAD_RETTYPE hwtimer_task(void *pvParameters)
{
	int idx =0, ret;
	UINT32 event, c_event;

	while (g_timer_ctrl.fd != -1) {
		//printf("timer_rcv_event b = 0x%x \r\n", event);
		ret = timer_rcv_event(&event);
		//printf("timer_rcv_event e = 0x%x \r\n", event);
		if (E_OK == ret) {
			c_event = event;
			idx = 0;
			//printf("event = 0x%x \r\n", event);
			while (c_event > 0)
			{
				//printf("c_event = 0x%x g_timer_ctrl.timer_cb = 0x%x\r\n", c_event, g_timer_ctrl.timer_cb[idx]);
				if ((c_event & 1) && g_timer_ctrl.timer_cb[idx]) {
					(g_timer_ctrl.timer_cb[idx])(idx);
					//printf("hwtimer event id = %d cb\r\n", idx);
				}
				c_event = c_event >> 1;
				idx++;
			}
		} else {
			printf("Fails, ret = %d, g_timer_ctrl.fd = %d\r\n", ret, g_timer_ctrl.fd);
			sleep(1);
		}
	}
	THREAD_RETURN(0);
}

ER timer_init(void)
{
	ER stReslut = E_SYS;

	g_timer_ctrl.fd = -1;

	SEM_CREATE(g_timer_ctrl.semid, 1);
  	g_timer_ctrl.fd = open(_TIMER_, O_RDWR);
	if (-1 == g_timer_ctrl.fd) {
		printf("[IO]open() failed with error\n");
		goto END_INIT;
	}
	g_timer_ctrl.timer_id_all = 0;
	g_timer_ctrl.tsk_hdl = vos_task_create(hwtimer_task, NULL, "utimer_task", 2, 4*1024);
	if (0 == g_timer_ctrl.tsk_hdl) {
		DBG_ERR("create utimer_task failed\r\n");
		return -1;
	}
	vos_task_resume(g_timer_ctrl.tsk_hdl);
	stReslut = E_OK;

END_INIT:
    return stReslut;
}

ER timer_exit(void)
{
	ER stReslut = E_OK;

	if (g_timer_ctrl.fd != -1) close(g_timer_ctrl.fd);

	return stReslut;
}

ER timer_open(PTIMER_ID pTimerID, TIMER_CB EventHandler)
{
    int ret;
    TIMER_ID id;

    ret = ioctl(g_timer_ctrl.fd, IOCTL_TIMER_OPEN, &id);
    *pTimerID = id;
	//printf("timer_open id %d pTimerID %d\n", id, *pTimerID);
	if(ret < 0) {
        printf("open err %d\n", ret);
		return E_SYS;
    }
	if (EventHandler) {
		SEM_WAIT(g_timer_ctrl.semid);
    	g_timer_ctrl.timer_cb[*pTimerID] = EventHandler;
		g_timer_ctrl.timer_id_all |= (1 << id);
		SEM_SIGNAL(g_timer_ctrl.semid);
		printf("timer_id_all = 0x%x\r\n", g_timer_ctrl.timer_id_all);

	}
    return E_OK;


}

/**
    Check if the UART Driver is opened

    Check if the UART Driver is opened.

    @return
     - @b TRUE:  UART driver is already opened.
     - @b FALSE: UART driver has not opened yet.
*/

ER timer_close(TIMER_ID TimerID)
{
    int ret;
    TIMER_ID id = TimerID;

    ret = ioctl(g_timer_ctrl.fd, IOCTL_TIMER_CLOSE, &id);
    if(ret < 0) {
        printf("close err %d\n", ret);
		return E_SYS;
    }
	SEM_WAIT(g_timer_ctrl.semid);
    g_timer_ctrl.timer_cb[TimerID] = NULL;
	g_timer_ctrl.timer_id_all &= (~(1 << id));
	SEM_SIGNAL(g_timer_ctrl.semid);
    return E_OK;

}

ER timer_cfg(TIMER_ID TimerID, UINT32 uiIntval, TIMER_MODE TimerMode, TIMER_STATE TimerState)
{
    int ret;
    TIMER_IOC_CONFIG config;
    config.expires = uiIntval;
    config.id = TimerID;
    config.mode = TimerMode;
    config.state = TimerState;

    ret = ioctl(g_timer_ctrl.fd, IOCTL_TIMER_CONFIG, &config);
    if(ret < 0) {
        printf("config err %d\n", ret);
		return E_SYS;
    }
    return E_OK;
}


TIMER_ID timer_get_sys_timer_id(void)
{
    return TIMER_SYSTIMER_ID;
}

/**
    Read current timer count.

    When timer is paused, current timer count is zero. The timer count will feedback
    faster than the enabled status.

    @param[in] TimerID      The timer ID whose count value you would like to get.

    @return Return unsigned 32-bit timer tick count value
*/
UINT32 timer_get_current_count(TIMER_ID TimerID)
{
    unsigned long ret = 0;
    unsigned long count = 0;
    count = (unsigned long) TimerID;

    if (TimerID >= TIMER_NUM) {
		printf("timer_getCurrentCount TimerID %d\n", TimerID);
		return 0;
    }
    ret = ioctl(g_timer_ctrl.fd, IOCTL_TIMER_GET_CURCOUNT, &count);
    if(ret < 0) {
        printf("count err %d\n", ret);
    }
    return count;
}

ER timer_wait_timeup(TIMER_ID TimerID)
{
    int ret;
    TIMER_ID id = TimerID;

    ret = ioctl(g_timer_ctrl.fd, IOCTL_TIMER_WAITTIMEUP, &id);
    if(ret < 0) {
        printf("wait err %d\n", ret);
    }
    return E_OK;
}



