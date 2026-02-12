/*
    SIE event install id

    This file is the API of the SIE control.

    @file       ctl_sie_event_id.c
    @ingroup    mISYSAlg
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/
#include "ctl_sie_event_int.h"
#include "ctl_sie_event_id_int.h"

ID sie_event_flg_id[SIE_EVENT_FLAG_MAX];

void sie_event_install_id(void)
{
	UINT32 i;

	for (i = 0; i < SIE_EVENT_FLAG_MAX; i++) {
		OS_CONFIG_FLAG(sie_event_flg_id[i]);
	}
}

void sie_event_uninstall_id(void)
{
	UINT32 i;

	for (i = 0; i < SIE_EVENT_FLAG_MAX; i++) {
		rel_flg(sie_event_flg_id[i]);
	}
}

