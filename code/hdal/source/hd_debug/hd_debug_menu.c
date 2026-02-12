/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file hd_debug_menu.c

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include "hd_debug_int.h"
#include "hd_util.h"
#include <string.h>
#include <kwrap/examsys.h>

#define HD_DEBUG_MENU_ID_QUIT 0xFE
#define HD_DEBUG_MENU_ID_RETURN 0xFF
#define HD_DEBUG_MENU_ID_LAST (-1)

extern int hd_audiocap_menu(void);
extern int hd_audioout_menu(void);
extern int hd_audioenc_menu(void);
extern int hd_audiodec_menu(void);
extern int hd_videocap_menu(void);
extern int hd_videoout_menu(void);
extern int hd_videopout_menu(void);
extern int hd_videoproc_menu(void);
extern int hd_videoenc_menu(void);
extern int hd_videodec_menu(void);
extern int hd_osg_menu(void);
extern int hd_common_menu(void);
extern int hd_util_menu(void);
extern int hd_debug_menu(void);

// _TODO: plug hdal modules here
// register your module here
static HD_DBG_MENU root_menu[] = {
	{0x01, "AUDIOCAPTURE",   hd_audiocap_menu,   TRUE},
	{0x02, "AUDIOOUT",       hd_audioout_menu,   TRUE},
	{0x03, "AUDIOENC",       hd_audioenc_menu,   TRUE},
	{0x04, "AUDIODEC",       hd_audiodec_menu,   TRUE},
	{0x05, "VIDEOCAPTURE",   hd_videocap_menu,   TRUE},
	{0x06, "VIDEOOUT",       hd_videoout_menu,   TRUE},
	{0x07, "VIDEOPROCESS",   hd_videoproc_menu,  TRUE},
	{0x08, "VIDEOENC",       hd_videoenc_menu,   TRUE},
	{0x09, "VIDEODEC",       hd_videodec_menu,   TRUE},
	{0x0A, "OSG",            hd_osg_menu,        TRUE},
	{0x0B, "COMMON",         hd_common_menu,     TRUE},
	{0x0C, "UTIL",           hd_util_menu,       TRUE},
	{0x0D, "DEBUG",          hd_debug_menu,      TRUE},
	// escape must be last
	{HD_DEBUG_MENU_ID_LAST,  "", NULL,           FALSE},
};

HD_RESULT hd_debug_init(void)
{
	return HD_OK;
}

HD_RESULT hd_debug_uninit(void)
{
	return HD_OK;
}

/********************************************************************
	DEBUG MENU IMPLEMENTATION
********************************************************************/
void hd_debug_menu_print_p(HD_DBG_MENU *p_menu, const char *p_title)
{
	DBG_DUMP("\n==============================");
	DBG_DUMP("\n %s", p_title);
	DBG_DUMP("\n------------------------------");

	while (p_menu->menu_id != HD_DEBUG_MENU_ID_LAST) {
		if (p_menu->b_enable) {
			DBG_DUMP("\n %02d : %s", p_menu->menu_id, p_menu->p_name);
		}
		p_menu++;
	}

	DBG_DUMP("\n------------------------------");
	DBG_DUMP("\n %02d : %s", HD_DEBUG_MENU_ID_QUIT, "Quit");
	DBG_DUMP("\n %02d : %s", HD_DEBUG_MENU_ID_RETURN, "Return");
	DBG_DUMP("\n------------------------------\n");
}

int hd_debug_menu_exec_p(int menu_id, HD_DBG_MENU *p_menu)
{
	if (menu_id == HD_DEBUG_MENU_ID_RETURN) {
		return 0; // return 0 for return upper menu
	}

	if (menu_id == HD_DEBUG_MENU_ID_QUIT) {
		return -1; // return -1 to notify upper layer to quit
	}

	while (p_menu->menu_id != HD_DEBUG_MENU_ID_LAST) {
		if (p_menu->menu_id == menu_id && p_menu->b_enable) {
			DBG_DUMP("Run: %02d : %s\r\n", p_menu->menu_id, p_menu->p_name);
			if (p_menu->p_func) {
				return p_menu->p_func();
			} else {
				DBG_ERR("null function for menu id = %d\n", menu_id);
				return 0; // just skip
			}
		}
		p_menu++;
	}

	DBG_ERR("cannot find menu id = %d\n", menu_id);
	return 0;
}

int hd_debug_menu_entry_p(HD_DBG_MENU *p_menu, const char *p_title)
{
	int menu_id = 0;

	do {
		hd_debug_menu_print_p(p_menu, p_title);
		menu_id = (int)hd_read_decimal_key_input("");
		if (hd_debug_menu_exec_p(menu_id, p_menu) == -1) {
			return -1; //quit
		}
	} while (menu_id != HD_DEBUG_MENU_ID_RETURN);

	return 0;
}

static int debug_menu_err_enable_p(void)
{
	g_hd_mask_err = (unsigned int)-1;
	return 0;
}

static int debug_menu_err_disable_p(void)
{
	g_hd_mask_err = (unsigned int)0;
	return 0;
}

static int debug_menu_wrn_enable_p(void)
{
	g_hd_mask_wrn = (unsigned int)-1;
	return 0;
}

static int debug_menu_wrn_disable_p(void)
{
	g_hd_mask_wrn = (unsigned int)0;
	return 0;
}

static int debug_menu_ind_enable_p(void)
{
	g_hd_mask_ind = (unsigned int)-1;
	return 0;
}

static int debug_menu_ind_disable_p(void)
{
	g_hd_mask_ind = (unsigned int)0;
	return 0;
}

static int debug_menu_msg_enable_p(void)
{
	g_hd_mask_msg = (unsigned int)-1;
	return 0;
}

static int debug_menu_msg_disable_p(void)
{
	g_hd_mask_msg = (unsigned int)0 | HD_LOG_MASK_DEBUG;
	return 0;
}

static int debug_menu_func_enable_p(void)
{
	g_hd_mask_func = (unsigned int)-1;
	return 0;
}

static int debug_menu_func_disable_p(void)
{
	g_hd_mask_func = (unsigned int)0;
	return 0;
}

static int debug_menu_module_enable_p(unsigned int mask)
{
	g_hd_mask_err |= mask;
	g_hd_mask_wrn |= mask;
	g_hd_mask_ind |= mask;
	g_hd_mask_msg |= mask;
	g_hd_mask_func |= mask;
	return 0;
}

static int debug_menu_module_disable_p(unsigned int mask)
{
	g_hd_mask_err &= ~mask;
	g_hd_mask_wrn &= ~mask;
	g_hd_mask_ind &= ~mask;
	g_hd_mask_msg &= ~mask;
	g_hd_mask_func &= ~mask;
	return 0;
}

static int debug_menu_audiocapture_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_AUDIOCAPTURE);
}

static int debug_menu_audiocapture_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_AUDIOCAPTURE);
}

static int debug_menu_audioout_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_AUDIOOUT);
}

static int debug_menu_audioout_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_AUDIOOUT);
}

static int debug_menu_audioenc_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_AUDIOENC);
}

static int debug_menu_audioenc_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_AUDIOENC);
}

static int debug_menu_audiodec_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_AUDIODEC);
}

static int debug_menu_audiodec_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_AUDIODEC);
}

static int debug_videocapture_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_VIDEOCAPTURE);
}

static int debug_videocapture_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_VIDEOCAPTURE);
}

static int debug_menu_videoout_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_VIDEOOUT);
}

static int debug_menu_videoout_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_VIDEOOUT);
}

static int debug_menu_videoprocess_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_VIDEOPROCESS);
}

static int debug_menu_videoprocess_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_VIDEOPROCESS);
}

static int debug_menu_videoenc_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_VIDEOENC);
}

static int debug_menu_videoenc_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_VIDEOENC);
}

static int debug_menu_videodec_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_VIDEODEC);
}

static int debug_menu_videodec_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_VIDEODEC);
}

static int debug_menu_gfx_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_GFX);
}

static int debug_menu_gfx_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_GFX);
}

static int debug_menu_common_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_COMMON);
}

static int debug_menu_common_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_COMMON);
}

static int debug_menu_util_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_UTIL);
}

static int debug_menu_util_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_UTIL);
}

static int debug_menu_debug_enable_p(void)
{
	return debug_menu_module_enable_p(HD_LOG_MASK_UTIL);
}

static int debug_menu_debug_disable_p(void)
{
	return debug_menu_module_disable_p(HD_LOG_MASK_UTIL);
}

static HD_DBG_MENU debug_menu[] = {
	{0x01, "all ERR mask enable",            debug_menu_err_enable_p,              TRUE},
	{0x02, "all ERR mask disable",           debug_menu_err_disable_p,             TRUE},
	{0x03, "all WRN mask enable",            debug_menu_wrn_enable_p,              TRUE},
	{0x04, "all WRN mask disable",           debug_menu_wrn_disable_p,             TRUE},
	{0x05, "all IND mask enable",            debug_menu_ind_enable_p,              TRUE},
	{0x06, "all IND mask disable",           debug_menu_ind_disable_p,             TRUE},
	{0x07, "all MSG mask enable",            debug_menu_msg_enable_p,              TRUE},
	{0x08, "all MSG mask disable",           debug_menu_msg_disable_p,             TRUE},
	{0x09, "all FUNC mask enable",           debug_menu_func_enable_p,             TRUE},
	{0x0A, "all FUNC mask disable",          debug_menu_func_disable_p,            TRUE},
	{0x0B, "all AUDIOCAPTURE mask enable",   debug_menu_audiocapture_enable_p,     TRUE},
	{0x0C, "all AUDIOCAPTURE mask disable",  debug_menu_audiocapture_disable_p,    TRUE},
	{0x0D, "all AUDIOOUT mask enable",       debug_menu_audioout_enable_p,         TRUE},
	{0x0E, "all AUDIOOUT mask disable",      debug_menu_audioout_disable_p,        TRUE},
	{0x0F, "all AUDIOENC mask enable",       debug_menu_audioenc_enable_p,         TRUE},
	{0x10, "all AUDIOENC mask disable",      debug_menu_audioenc_disable_p,        TRUE},
	{0x11, "all AUDIODEC mask enable",       debug_menu_audiodec_enable_p,         TRUE},
	{0x12, "all AUDIODEC mask disable",      debug_menu_audiodec_disable_p,        TRUE},
	{0x13, "all VIDEOCAPTURE mask enable",   debug_videocapture_enable_p,          TRUE},
	{0x14, "all VIDEOCAPTURE mask disable",  debug_videocapture_disable_p,         TRUE},
	{0x15, "all VIDEOOUT mask enable",       debug_menu_videoout_enable_p,         TRUE},
	{0x16, "all VIDEOOUT mask disable",      debug_menu_videoout_disable_p,        TRUE},
	{0x17, "all VIDEOPROCESS  mask enable",  debug_menu_videoprocess_enable_p,     TRUE},
	{0x18, "all VIDEOPROCESS  mask disable", debug_menu_videoprocess_disable_p,    TRUE},
	{0x19, "all VIDEOENC mask enable",       debug_menu_videoenc_enable_p,         TRUE},
	{0x1A, "all VIDEOENC mask disable",      debug_menu_videoenc_disable_p,        TRUE},
	{0x1B, "all VIDEODEC mask enable",       debug_menu_videodec_enable_p,         TRUE},
	{0x1C, "all VIDEODEC mask disable",      debug_menu_videodec_disable_p,        TRUE},
	{0x1D, "all GFX mask enable",            debug_menu_gfx_enable_p,              TRUE},
	{0x1E, "all GFX mask disable",           debug_menu_gfx_disable_p,             TRUE},
	{0x1F, "all COMMON mask enable",         debug_menu_common_enable_p,           TRUE},
	{0x20, "all COMMON mask disable",        debug_menu_common_disable_p,          TRUE},
	{0x21, "all UTIL mask enable",           debug_menu_util_enable_p,             TRUE},
	{0x22, "all UTIL mask disable",          debug_menu_util_disable_p,            TRUE},
	{0x23, "all DEBUG mask enable",          debug_menu_debug_enable_p,            TRUE},
	{0x24, "all DEBUG mask disable",         debug_menu_debug_disable_p,           TRUE},
	// escape muse be last
	{-1,   "",               NULL,               FALSE},
};

int hd_debug_menu(void)
{
	return hd_debug_menu_entry_p(debug_menu, "DEBUG");
}

HD_RESULT hd_debug_run_menu(void)
{
	hd_debug_menu_entry_p(root_menu, "HDAL");
	return HD_OK;
}

