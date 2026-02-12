/*
    Internal header file for SIE module.

    @file       sie_init_int.h
    @ingroup    mIIPPSIE

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

#ifndef _SIE_INIT_INT_H
#define _SIE_INIT_INT_H
#if defined(__KERNEL__)
#include "kwrap/error_no.h"
#include "kwrap/type.h"
#include "kwrap/debug.h"
#include "sie_init.h"

typedef enum _SIE_FB_DBG_LVL {
	SIE_FB_DBG_LVL_NONE = 0,
	SIE_FB_DBG_LVL_ERR,
	SIE_FB_DBG_LVL_WRN,
	SIE_FB_DBG_LVL_IND,
	SIE_FB_DBG_LVL_FUNC,
	SIE_FB_DBG_LVL_MAX,
	ENUM_DUMMY4WORD(SIE_FB_DBG_LVL)
} SIE_FB_DBG_LVL;

extern SIE_FB_DBG_LVL sie_fb_dbg_lvl;
#define sie_fb_dbg_err(fmt, args...)     { if (sie_fb_dbg_lvl >= SIE_FB_DBG_LVL_ERR) { DBG_ERR(fmt, ##args);  }}
#define sie_fb_dbg_wrn(fmt, args...)     { if (sie_fb_dbg_lvl >= SIE_FB_DBG_LVL_WRN) { DBG_WRN(fmt, ##args);  }}
#define sie_fb_dbg_ind(fmt, args...)     { if (sie_fb_dbg_lvl >= SIE_FB_DBG_LVL_IND) { DBG_DUMP(fmt, ##args); }}
#define sie_fb_dbg_func(fmt, args...)    { if (sie_fb_dbg_lvl >= SIE_FB_DBG_LVL_FUNC){ DBG_DUMP(fmt, ##args); }}

INT32 sie_init_plat_chk_node(CHAR *node);
INT32 sie_init_plat_read_dtsi_array(CHAR *node, CHAR *tag, UINT32 *buf, UINT32 num);
void *kdrv_sie_builtin_plat_malloc(UINT32 size);
void kdrv_sie_builtin_plat_free(void *p_buf);

#endif// _SIE_INIT_INT_H
#endif
