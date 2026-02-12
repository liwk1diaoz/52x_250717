

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

static  VK_DEFINE_SPINLOCK(my_lock);
#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#define FLGPTN_IST_1	FLGPTN_BIT(0)
#define FLGPTN_IST_2	FLGPTN_BIT(1)
#define FLGPTN_IST_25	FLGPTN_BIT(2)

extern unsigned int		rtos_interrupt_debug_level;

static irq_bh_handler_t bh_interrupt_tbl[INT_ID_MAX + 1] = {NULL};
static UINT32			bh_priority[INT_ID_MAX + 1];
static UINT32			bh_event[INT_ID_MAX + 1];
static UINT32			bh_data[INT_ID_MAX + 1];

static ID flag_ist;

#if 1

static void Ist_task1(void *pvParameters)
{
	FLGPTN			flgptn = 0;
	unsigned long	irq,event;
	unsigned long	flag = 0;


	DBG_MSG("run\r\n");

	//coverity[no_escape]
	while (1) {

		wai_flg(&flgptn, flag_ist, FLGPTN_IST_1, TWF_ORW | TWF_CLR);

		for(irq = 0; irq <= INT_ID_MAX; irq++) {
			event = bh_event[irq];
			if((bh_priority[irq] & IRQF_BH_PRI_HIGH) && (event != 0) && (bh_interrupt_tbl[irq] != NULL)) {

				bh_interrupt_tbl[irq](irq, event, (void *)bh_data[irq]);

				loc_cpu(flag);
				bh_event[irq] &= ~event;
				unl_cpu(flag);
			}
		}
	}
}

static void Ist_task2(void *pvParameters)
{
	FLGPTN			flgptn = 0;
	unsigned long	irq,event;
	unsigned long	flag = 0;

	DBG_MSG("run\r\n");

	//coverity[no_escape]
	while (1) {
		wai_flg(&flgptn, flag_ist, FLGPTN_IST_2, TWF_ORW | TWF_CLR);

		for(irq = 0; irq <= INT_ID_MAX; irq++) {
			event = bh_event[irq];
			if((bh_priority[irq] & IRQF_BH_PRI_MIDDLE) && (event != 0) && (bh_interrupt_tbl[irq] != NULL)) {

				bh_interrupt_tbl[irq](irq, event, (void *)bh_data[irq]);

				loc_cpu(flag);
				bh_event[irq] &= ~event;
				unl_cpu(flag);
			}
		}
	}
}

static void Ist_task25(void *pvParameters)
{
	FLGPTN			flgptn = 0;
	unsigned long	irq,event;
	unsigned long	flag = 0;

	DBG_MSG("run\r\n");

	//coverity[no_escape]
	while (1) {
		wai_flg(&flgptn, flag_ist, FLGPTN_IST_25, TWF_ORW | TWF_CLR);

		for(irq = 0; irq <= INT_ID_MAX; irq++) {
			event = bh_event[irq];
			if((bh_priority[irq] & IRQF_BH_PRI_LOW) && (event != 0) && (bh_interrupt_tbl[irq] != NULL)) {

				bh_interrupt_tbl[irq](irq, event, (void *)bh_data[irq]);

				loc_cpu(flag);
				bh_event[irq] &= ~event;
				unl_cpu(flag);
			}
		}

	}
}

#endif


/**

*/
void irq_init(void)
{
	static BOOL init_done = 0;
	THREAD_HANDLE  TSK_IST1;
	THREAD_HANDLE  TSK_IST2;
	THREAD_HANDLE  TSK_IST25;

	if(!init_done) {

		cre_flg(&flag_ist, NULL, "flag_ist");

		TSK_IST1 = vos_task_create(Ist_task1,  0, "Ist1",   1, 4096);
		vos_task_resume(TSK_IST1);
		TSK_IST2 = vos_task_create(Ist_task2,  0, "Ist2",   2, 2048);
		vos_task_resume(TSK_IST2);
		TSK_IST25 = vos_task_create(Ist_task25, 0, "Ist25", 25, 2048);
		vos_task_resume(TSK_IST25);

		init_done = 1;
	}
}

/**
	Request IRQ bottom half service
*/
int request_irq_bh(unsigned int irq, irq_bh_handler_t bh_handler, IRQF_BH flags)
{
	unsigned long flag = 0;

	if((irq < INT_GIC_SPI_START_ID) || (irq >= INT_ID_CNT)) {
		DBG_ERR("irq %d",irq);
		return -1;
	}

	loc_cpu(flag);

	bh_interrupt_tbl[irq-31] = bh_handler;

	if (flags & IRQF_BH_PRI_HIGH) {
		bh_priority[irq-31] = IRQF_BH_PRI_HIGH;
	} else if (flags & IRQF_BH_PRI_MIDDLE) {
		bh_priority[irq-31] = IRQF_BH_PRI_MIDDLE;
	} else if (flags & IRQF_BH_PRI_LOW) {
		bh_priority[irq-31] = IRQF_BH_PRI_LOW;
	}

	unl_cpu(flag);

	return 0;
}

/**
	Free IRQ Bottom Half

	This would disable IRQ bottom halfservice.
*/
void free_irq_bh(unsigned int irq, void *dev)
{
	unsigned long flag = 0;

	loc_cpu(flag);
	bh_interrupt_tbl[irq-31] = NULL;
	bh_priority[irq-31] = 0;
	unl_cpu(flag);
}

/**
	Kick bottom half service

*/
int kick_bh(unsigned int irq, unsigned long event, void *data)
{
	unsigned long flag = 0;
	UINT32 losing_evt;

	losing_evt = bh_event[irq-31] & event;

	loc_cpu(flag);
	bh_event[irq-31] |= event;
	bh_data[irq-31] = (UINT32)data;
	unl_cpu(flag);

	if (losing_evt) {
		DBG_ERR("\r\nIRQ(%d) losing event [0x%08X]\r\n", irq, (UINT)event);
	}

	if (bh_priority[irq-31] & IRQF_BH_PRI_HIGH) {
		iset_flg(flag_ist, FLGPTN_IST_1);
	} else if (bh_priority[irq-31] & IRQF_BH_PRI_MIDDLE) {
		iset_flg(flag_ist, FLGPTN_IST_2);
	} else if (bh_priority[irq-31] & IRQF_BH_PRI_LOW) {
		iset_flg(flag_ist, FLGPTN_IST_25);
	}

	return 0;
}








