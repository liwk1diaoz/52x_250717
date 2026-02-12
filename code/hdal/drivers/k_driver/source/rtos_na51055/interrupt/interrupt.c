
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <io_address.h>
#include <interrupt.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/task.h>
#include <kwrap/spinlock.h>
#define __MODULE__    rtos_interrupt
#include <kwrap/debug.h>
unsigned int rtos_interrupt_debug_level = NVT_DBG_WRN;

int		nvt_request_irq(unsigned int irq_num, irq_handler_t isr, void *priv);
void	nvt_enable_irq(int number);
void	nvt_disable_irq(int number);

#if defined(_CPU2_RTOS_)
extern 	void nvt_enable_irq_target(int number, int target);
#endif
/**
	Request IRQ service
*/
int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev)
{
	DBG_IND("irq(%d)<%s> handler(0x%08X) flags(0x%08X) dev(0x%08X)\r\n",
		(INT)irq, (CHAR *)name, (UINT)handler, (UINT)flags, (UINT)dev);

	if(handler != NULL) {
		nvt_request_irq(irq, handler, dev);
	}
	nvt_enable_irq(irq);

	return 0;
}

#if defined(_CPU2_RTOS_)
/**
	Request IRQ service
*/
int request_irq_core2(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev)
{
	DBG_IND("irq(%d)<%s> handler(0x%08X) flags(0x%08X) dev(0x%08X)\r\n",
		(INT)irq, (CHAR *)name, (UINT)handler, (UINT)flags, (UINT)dev);

	if(handler != NULL) {
		nvt_request_irq(irq, handler, dev);
	}

	nvt_enable_irq_target(irq, 2);
	return 0;
}
#endif

/**
	Free IRQ

	This would disable IRQ service.
*/
void free_irq(unsigned int irq, void *dev)
{
	DBG_IND("irq(%d)\r\n", (INT)irq);

	nvt_disable_irq(irq);
}

