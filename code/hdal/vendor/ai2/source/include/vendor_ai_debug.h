#ifndef _VENDOR_AI_DEBUG_H_
#define _VENDOR_AI_DEBUG_H_

#include <stdarg.h>
#include <kwrap/type.h>


#if defined(__LINUX) && defined(__KERNEL__)
#include <linux/printk.h>
#define vk_pr_warn pr_warn
#define vk_printk printk
#define vk_print_isr printk
#define vos_dump_stack dump_stack

#elif defined(__FREERTOS)
#define vk_pr_warn debug_msg_isr
#define vk_printk debug_msg_isr
#define vk_print_isr debug_msg_isr

#ifndef unlikely
#define unlikely(x) (x)
#endif

#ifndef KERN_EMERG
#define KERN_EMERG ""
#endif

#else
#include <stdio.h>
#define vk_pr_warn printf
#define vk_printk printf
#define vk_print_isr printf

#ifndef unlikely
#define unlikely(x) (x)
#endif

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

extern unsigned int vendor_ai_debug_level;

#define nvt_dbg(level, fmt, args...) do { \
	if (unlikely(NVT_DBG_##level <= vendor_ai_debug_level)) { \
		vk_pr_warn(fmt, ##args); \
	} \
} while (0)

#define DBG_FATAL(fmt, args...)         nvt_dbg(FATAL, __CLASS__" FATAL:%s() " fmt, __func__, ##args)
#define DBG_ERR(fmt, args...)           nvt_dbg(ERR,   __CLASS__" ERR:%s() "   fmt, __func__, ##args)
#define DBG_WRN(fmt, args...)           nvt_dbg(WRN,   __CLASS__" WRN:%s() "   fmt, __func__, ##args)
#define DBG_UNIT(fmt, args...)          nvt_dbg(UNIT,  __CLASS__" UNIT:%s() "  fmt, __func__, ##args)
#define DBG_FUNC_BEGIN(fmt, args...)    nvt_dbg(FUNC,  __CLASS__" %s():begin " fmt, __func__, ##args)
#define DBG_FUNC(fmt, args...)          nvt_dbg(FUNC,  __CLASS__" %s(): "      fmt, __func__, ##args)
#define DBG_FUNC_END(fmt, args...)      nvt_dbg(FUNC,  __CLASS__" %s():end "   fmt, __func__, ##args)
#define DBG_IND(fmt, args...)           nvt_dbg(IND,   __CLASS__" %s() "       fmt, __func__, ##args)
#define DBG_MSG(fmt, args...)           nvt_dbg(MSG,   __CLASS__" " fmt, ##args)
#define DBG_VALUE(fmt, args...)         nvt_dbg(VALUE, __CLASS__" " fmt, ##args)
#define DBG_USER(fmt, args...)          nvt_dbg(USER,  __CLASS__" " fmt, ##args)
#define DBG_DUMP(fmt, args...)          vk_pr_warn(fmt, ##args)

#define CHKPNT    vk_pr_warn("AI:CHK: %d, %s\r\n", __LINE__, __func__)
#define DBGD(x)   vk_pr_warn("AI:DBG: %s=%d\r\n", #x, x)
#define DBGH(x)   vk_pr_warn("AI:DBG: %s=0x%08X\r\n", #x, x)


#define PROF                DISABLE
#if PROF
	static struct timeval tstart, tend;
	#define PROF_START()    do_gettimeofday(&tstart);
	#define PROF_END(msg)   do_gettimeofday(&tend);     \
			DBG_DUMP("%s time (us): %lu\r\n", msg,        \
					(tend.tv_sec - tstart.tv_sec) * 1000000 + (tend.tv_usec - tstart.tv_usec));
#else
	#define PROF_START()
	#define PROF_END(msg)
#endif


#define ASSERT(x)                                                       \
	do {    if (x) break;                                               \
		vk_printk(KERN_EMERG "### ASSERTION FAILED %s: %s: %d: %s\n",      \
			   __FILE__, __func__, __LINE__, #x); vos_dump_stack(); vos_debug_halt();  \
	} while (0)


#if defined(__FREERTOS)
int debug_msg_isr(const char *fmtstr, ...);
#endif

void debug_dumpmem(ULONG addr, ULONG length); ///< dump memory region to console output


#endif //_VENDOR_AI_DEBUG_H_


