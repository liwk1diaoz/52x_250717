/**
	@brief Debug menu for UVC lib.\n

	@file uvc_debug_menu.c

	@author Ben Wang

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "hdal.h"
#include "uvc_debug_menu.h"
#include "hd_util.h"

#define UVAC_DBG_CMD_NONE                    0

#define UVAC_DBG_ISO_VID            0x00000100
#define UVAC_DBG_ISO_AUD            0x00000400
#define UVAC_DBG_ISO_TX             0x00000001
#define UVAC_DBG_ISO_QUE_CNT        0x00000002

#define UVAC_DBG_VID_START          0x00000001
#define UVAC_DBG_VID_QUEUE          0x00000002
#define UVAC_DBG_VID_PAYLOAD        0x00000004
#define UVAC_DBG_VID_RCV_ALL        0x00000008
#define UVAC_DBG_VID_RCV_V1         0x00000010
#define UVAC_DBG_VID_RCV_V2         0x00000020
#define UVAC_DBG_VID_RCV_A1         0x00000040
#define UVAC_DBG_VID_RCV_A2         0x00000080
#define UVAC_DBG_VID_QUE_CNT        0x00000100

extern UINT32 m_uiU2UvacDbgIso;
extern UINT32 m_uiU2UvacDbgVid;

///////////////////////////////////////////////////////////////////////////////
/********************************************************************
	DEBUG MENU IMPLEMENTATION
********************************************************************/

static void uvc_debug_menu_print_p(UVC_DBG_MENU *p_menu, const char *p_title)
{
	printf("\n==============================");
	printf("\n %s", p_title);
	printf("\n------------------------------");

	while (p_menu->menu_id != UVC_DEBUG_MENU_ID_LAST) {
		if (p_menu->b_enable) {
			printf("\n %02d : %s", p_menu->menu_id, p_menu->p_name);
		}
		p_menu++;
	}

	printf("\n------------------------------");
	printf("\n %02d : %s", UVC_DEBUG_MENU_ID_QUIT, "Quit");
	printf("\n %02d : %s", UVC_DEBUG_MENU_ID_RETURN, "Return");
	printf("\n------------------------------\n");
}

static int uvc_debug_menu_exec_p(int menu_id, UVC_DBG_MENU *p_menu)
{
	if (menu_id == UVC_DEBUG_MENU_ID_RETURN) {
		return 0; // return 0 for return upper menu
	}

	if (menu_id == UVC_DEBUG_MENU_ID_QUIT) {
		return -1; // return -1 to notify upper layer to quit
	}

	while (p_menu->menu_id != UVC_DEBUG_MENU_ID_LAST) {
		if (p_menu->menu_id == menu_id && p_menu->b_enable) {
			printf("Run: %02d : %s\r\n", p_menu->menu_id, p_menu->p_name);
			if (p_menu->p_func) {
				return p_menu->p_func();
			} else {
				printf("ERR: null function for menu id = %d\n", menu_id);
				return 0; // just skip
			}
		}
		p_menu++;
	}

	printf("ERR: cannot find menu id = %d\n", menu_id);
	return 0;
}

int uvc_debug_menu_entry_p(UVC_DBG_MENU *p_menu, const char *p_title)
{
	int menu_id = 0;

	do {
		uvc_debug_menu_print_p(p_menu, p_title);
		menu_id = (int)hd_read_decimal_key_input("");
		if (uvc_debug_menu_exec_p(menu_id, p_menu) == -1) {
			return -1; //quit
		}
	} while (menu_id != UVC_DEBUG_MENU_ID_RETURN);

	return 0;
}
///////////////////////////////////////////////////////////////////////////////

static int uvac_dbg_none(void)
{
	m_uiU2UvacDbgVid = UVAC_DBG_CMD_NONE;
	m_uiU2UvacDbgIso = UVAC_DBG_CMD_NONE;
	return 0;
}
static int uvac_dbg_vid_start(void)
{
	m_uiU2UvacDbgVid |= UVAC_DBG_VID_START;
	return 0;
}
static int uvac_dbg_vid_queue_perf(void)
{
	m_uiU2UvacDbgVid |= UVAC_DBG_VID_QUEUE;
	return 0;
}
static int uvac_dbg_vid_rcv_all(void)
{
	m_uiU2UvacDbgVid |= UVAC_DBG_VID_RCV_ALL;
	return 0;
}
static int uvac_dbg_vid_rcv_v1(void)
{
	m_uiU2UvacDbgVid |= UVAC_DBG_VID_RCV_V1;
	return 0;
}
static int uvac_dbg_vid_rcv_v2(void)
{
	m_uiU2UvacDbgVid |= UVAC_DBG_VID_RCV_V2;
	return 0;
}
static int uvac_dbg_vid_rcv_a1(void)
{
	m_uiU2UvacDbgVid |= UVAC_DBG_VID_RCV_A1;
	return 0;
}
static int uvac_dbg_vid_rcv_a2(void)
{
	m_uiU2UvacDbgVid |= UVAC_DBG_VID_RCV_A2;
	return 0;
}
static int uvac_dbg_vid_queue_cnt(void)
{
	m_uiU2UvacDbgVid |= UVAC_DBG_VID_QUE_CNT;
	return 0;
}
static int uvac_dbg_iso_vid_tx(void)
{
	m_uiU2UvacDbgIso |= (UVAC_DBG_ISO_VID|UVAC_DBG_ISO_TX);
	return 0;
}
static int uvac_dbg_iso_vid_queue_cnt(void)
{
	m_uiU2UvacDbgIso |= (UVAC_DBG_ISO_VID|UVAC_DBG_ISO_QUE_CNT);
	return 0;
}
static int uvac_dbg_iso_aud_tx(void)
{
	m_uiU2UvacDbgIso |= (UVAC_DBG_ISO_AUD|UVAC_DBG_ISO_TX);
	return 0;
}
static int uvac_dbg_iso_aud_queue_cnt(void)
{
	m_uiU2UvacDbgIso |= (UVAC_DBG_ISO_AUD|UVAC_DBG_ISO_QUE_CNT);
	return 0;
}

UVC_DBG_MENU uvc_debug_menu[] = {
	{0x00, "DBG OFF",           uvac_dbg_none,       TRUE},
	{0x01, "DBG VID TSK START",  uvac_dbg_vid_start,      TRUE},
	{0x02, "DBG VID TSK PERF",  uvac_dbg_vid_queue_perf,      TRUE},
	{0x03, "DBG VID TSK QUEUE",  uvac_dbg_vid_queue_cnt,    TRUE},
	{0x04, "DBG VID RCV ALL",    uvac_dbg_vid_rcv_all,    TRUE},
	{0x05, "DBG VID RCV V1",    uvac_dbg_vid_rcv_v1,    TRUE},
	{0x06, "DBG VID RCV V2",    uvac_dbg_vid_rcv_v2,    TRUE},
	{0x07, "DBG VID RCV A1",    uvac_dbg_vid_rcv_a1,    TRUE},
	{0x08, "DBG VID RCV A2",    uvac_dbg_vid_rcv_a2,    TRUE},

	{0x09, "DBG ISO TSK VID TX",    uvac_dbg_iso_vid_tx,    TRUE},
	{0x0A, "DBG ISO TSK VID QUEUE",    uvac_dbg_iso_vid_queue_cnt,    TRUE},
	{0x0B, "DBG ISO TSK AUD TX",    uvac_dbg_iso_aud_tx,    TRUE},
	{0x0C, "DBG ISO TSK AUD QUEUE",    uvac_dbg_iso_aud_queue_cnt,    TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE}
};

