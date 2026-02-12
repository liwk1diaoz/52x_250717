/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>

#define __MODULE__    rtos_spinlock
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/spinlock.h>

#include <FreeRTOS.h>
#include <task.h> //FreeRTOS header file

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_spinlock_debug_level = NVT_DBG_WRN;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void vk_spin_lock_init(vk_spinlock_t *lock)
{
	unsigned long flags;

	flags = (unsigned long)taskENTER_CRITICAL_FROM_ISR();

	lock->init_tag = RTOS_SPINLOCK_INITED_TAG;
	lock->is_lock = 0;

	taskEXIT_CRITICAL_FROM_ISR((UBaseType_t)flags);
}

unsigned long _vk_raw_spin_lock_irqsave(vk_spinlock_t *lock)
{
	unsigned long flags;

	flags = (unsigned long)taskENTER_CRITICAL_FROM_ISR();

	if (RTOS_SPINLOCK_INITED_TAG != lock->init_tag) {
		DBG_ERR("not inited, tag = 0x%X\r\n", (unsigned int)lock->init_tag);
		taskEXIT_CRITICAL_FROM_ISR((UBaseType_t)flags);
		return flags;
	}

	//the same spinlock should not be locked twice
	if (lock->is_lock == 1) {
		DBG_ERR("recursive lock\r\n");
	}

	lock->is_lock = 1;

	return flags;
}

void vk_spin_unlock_irqrestore(vk_spinlock_t *lock, unsigned long flags)
{
	if (RTOS_SPINLOCK_INITED_TAG != lock->init_tag) {
		DBG_ERR("not inited, tag = 0x%X\r\n", (unsigned int)lock->init_tag);
		return;
	}

	if (lock->is_lock == 0) {
		DBG_ERR("unlock a non-locked spinlock\r\n");
	}

	lock->is_lock = 0;
	taskEXIT_CRITICAL_FROM_ISR((UBaseType_t)flags);
}
