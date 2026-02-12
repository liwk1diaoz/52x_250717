/*
   LVDS/HiSPi module driver Object File

    @file       lvdsobj.c
    @ingroup    mIDrvIO_LVDS
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "kwrap/type.h"
#include "../lvds.h"
#include "lvds_int.h"

#ifndef __KERNEL__
#define __MODULE__    rtos_lvds
#include <kwrap/debug.h>
extern unsigned int rtos_lvds_debug_level;
#endif

static LVDSOBJ lvds_object[LVDS_ID_MAX] = {
	{
		LVDS_ID_LVDS,  lvds_open, lvds_close,   lvds_is_opened,    lvds_set_enable,   lvds_get_enable,      lvds_enable_streaming,  lvds_set_config,  lvds_get_config,
		lvds_set_channel_config,     lvds_get_channel_config, lvds_set_padlane_config, lvds_set_sync_word, lvds_wait_interrupt,  lvds_dump_debug_info
	},
	{
		LVDS_ID_LVDS2, lvds2_open, lvds2_close, lvds2_is_opened,   lvds2_set_enable,  lvds2_get_enable,     lvds2_enable_streaming, lvds2_set_config, lvds2_get_config,
		lvds2_set_channel_config,    lvds2_get_channel_config, lvds2_set_padlane_config, lvds2_set_sync_word, lvds2_wait_interrupt, lvds2_dump_debug_info
	}
};


/**
    @addtogroup mIDrvIO_LVDS
*/
//@{

#ifdef __KERNEL__

UINT32 lvds_errlog[LVDS_ID_MAX];


void lvds_errchk_timer_cb(unsigned long ui_event)
{
	UINT32 sts;
	struct timer_list *tick = (struct timer_list *)ui_event;

	mod_timer(tick, jiffies + msecs_to_jiffies(500));

	if (lvds_get_enable()) {
		sts = LVDS_R_REGISTER(LVDS_INTSTS_REG_OFS);
		LVDS_W_REGISTER(LVDS_INTSTS_REG_OFS, LVDS_ERR_CHK_INT);

		lvds_errlog[LVDS_ID_LVDS] = 0;
		if (lvds_get_config(LVDS_CONFIG_ID_CHID) != 0xF) {
			lvds_errlog[LVDS_ID_LVDS] += (sts & (LVDS_INTERRUPT_FIFO_ER|LVDS_INTERRUPT_PIXCNT_ER|LVDS_INTERRUPT_FIFO1_OV));
		}
		if (lvds_get_config(LVDS_CONFIG_ID_CHID2) != 0xF) {
			lvds_errlog[LVDS_ID_LVDS] += (sts & (LVDS_INTERRUPT_FIFO2_ER|LVDS_INTERRUPT_PIXCNT_ER2|LVDS_INTERRUPT_FIFO2_OV));
		}
		if (lvds_get_config(LVDS_CONFIG_ID_CHID3) != 0xF) {
			lvds_errlog[LVDS_ID_LVDS] += (sts & (LVDS_INTERRUPT_FIFO3_ER|LVDS_INTERRUPT_PIXCNT_ER3|LVDS_INTERRUPT_FIFO3_OV));
		}
		if (lvds_get_config(LVDS_CONFIG_ID_CHID4) != 0xF) {
			lvds_errlog[LVDS_ID_LVDS] += (sts & (LVDS_INTERRUPT_FIFO4_ER|LVDS_INTERRUPT_PIXCNT_ER4|LVDS_INTERRUPT_FIFO4_OV));
		}
	}

	if (lvds2_get_enable()) {
		sts = LVDS2_R_REGISTER(LVDS_INTSTS_REG_OFS);
		LVDS2_W_REGISTER(LVDS_INTSTS_REG_OFS, LVDS_ERR_CHK_INT);

		lvds_errlog[LVDS_ID_LVDS2] = 0;
		if (lvds2_get_config(LVDS_CONFIG_ID_CHID) != 0xF) {
			lvds_errlog[LVDS_ID_LVDS2] += (sts & (LVDS_INTERRUPT_FIFO_ER|LVDS_INTERRUPT_PIXCNT_ER|LVDS_INTERRUPT_FIFO1_OV));
		}
		if (lvds2_get_config(LVDS_CONFIG_ID_CHID2) != 0xF) {
			lvds_errlog[LVDS_ID_LVDS2] += (sts & (LVDS_INTERRUPT_FIFO2_ER|LVDS_INTERRUPT_PIXCNT_ER2|LVDS_INTERRUPT_FIFO2_OV));
		}
	}

}

void lvds_enable_errchk_timer(LVDS_ID id, BOOL enable)
{
/*
	static struct timer_list lvds_errchk_timer;
	static UINT32 opened,enabled;

	if (enable) {
		enabled |= (0x1 << (UINT32)id);
	} else {
		enabled &= ~(0x1 << (UINT32)id);
	}

	if ((enabled > 0) && (opened == 0)) {
		opened = 1;

		init_timer(&lvds_errchk_timer);
		lvds_errchk_timer.function  = lvds_errchk_timer_cb;
		lvds_errchk_timer.expires   = jiffies + msecs_to_jiffies(500);
		lvds_errchk_timer.data      = (unsigned long)&lvds_errchk_timer;
		add_timer(&lvds_errchk_timer);

	} else if ((enabled == 0) && (opened == 1)) {
		opened = 0;

		del_timer_sync(&lvds_errchk_timer);
	}
*/
}
#endif


PLVDSOBJ lvds_get_drv_object(LVDS_ID lvds_identity)
{
	if (lvds_identity >= LVDS_ID_MAX) {
		DBG_ERR("No such module.%d\r\n", lvds_identity);
		return NULL;
	}

	return &lvds_object[lvds_identity];
}
#ifdef __KERNEL__
EXPORT_SYMBOL(lvds_get_drv_object);
#endif

//@}


//@}
