/**
	@brief Source code of debug function.\n
	This file contains the debug function, and debug menu entry point.

	@file uvc_debug_menu.h

	@ingroup mhdal

	@note Nothing.

	Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#ifndef _UVC_DEBUG_MENU_H_
#define _UVC_DEBUG_MENU_H_

#define UVC_DEBUG_MENU_ID_QUIT 0xFE
#define UVC_DEBUG_MENU_ID_RETURN 0xFF
#define UVC_DEBUG_MENU_ID_LAST (-1)

typedef struct _UVC_DBG_MENU {
	int            menu_id;          ///< user command value
	const char     *p_name;          ///< command string
	int            (*p_func)(void);  ///< command function
	BOOL           b_enable;         ///< execution option
} UVC_DBG_MENU;

extern UVC_DBG_MENU uvc_debug_menu[];

extern int uvc_debug_menu_entry_p(UVC_DBG_MENU *p_menu, const char *p_title);

#endif
