#ifndef __IPE_DBG_H_
#define __IPE_DBG_H_

#define __IPE_DBG_CMD__		0

#if 1
#define __MODULE__          ipe
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" // *=All, [mark]=CustomClass
#include "kwrap/debug.h"

#define IPE_DBG_NONE             0x00000000
#define IPE_DBG_VA               0x00000001
#define IPE_DBG_DEFOG            0x00000002
#define IPE_DBG_CCM              0x00000004
#define IPE_DBG_EEXT             0x00000008
#define IPE_DBG_DEFAULTON        0x10000000

#define PRINT_IPE(type, fmt, args...) {if (type) DBG_DUMP(fmt, ## args); }

extern UINT32 ipe_dbg_cmd;
#else

#if defined(__LINUX)
#define vk_pr_warn pr_warn
#define vk_printk printk
#elif defined(__FREERTOS)
#include <stdio.h>
#define vk_pr_warn printf
#define vk_printk printf

#ifndef unlikely
#define unlikely(x) (x)
#endif

#else
#error Not supported OS
#endif

#define NVT_DBG_FATAL     0
#define NVT_DBG_ERR       1
#define NVT_DBG_WRN       2
#define NVT_DBG_UNIT      3
#define NVT_DBG_FUNC      4
#define NVT_DBG_IND       5
#define NVT_DBG_MSG       6
#define NVT_DBG_VALUE     7
#define NVT_DBG_USER      8

#ifdef DEBUG

extern unsigned int ipe_debug_level;
#define nvt_dbg(level, fmt, args...)                \
	do {                                               \
		if (unlikely(NVT_DBG_##level <= ipe_debug_level))    \
			vk_pr_warn("%s:" fmt, __func__, ##args);   \
	} while (0)
#else
#define nvt_dbg(fmt, ...)
#endif

#define DBG_FATAL(fmt, args...) nvt_dbg(FATAL, fmt, ##args)
#define DBG_ERR(fmt, args...) nvt_dbg(ERR, fmt, ##args)
#define DBG_WRN(fmt, args...) nvt_dbg(WRN, fmt, ##args)
#define DBG_UNIT(fmt, args...) nvt_dbg(UNIT, fmt, ##args)
#define DBG_FUNC(fmt, args...) nvt_dbg(FUNC, fmt, ##args)
#define DBG_IND(fmt, args...) nvt_dbg(IND, fmt, ##args)
#define DBG_MSG(fmt, args...) nvt_dbg(MSG, fmt, ##args)
#define DBG_VALUE(fmt, args...) nvt_dbg(VALUE, fmt, ##args)
#define DBG_USER(fmt, args...) nvt_dbg(USER, fmt, ##args)
#define DBG_DUMP(fmt, args...) vk_pr_warn("%s:" fmt, __func__, ##args);

#endif

#endif



