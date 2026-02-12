#ifndef __MODULE_DBG_H_
#define __MODULE_DBG_H_

#define __MODULE__	ctl_ipp
#define __DBGLVL__	8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#include "kwrap/debug.h"
#include "kwrap/perf.h"

// for CTL_IPP_DBG_TRC_PART log type
typedef enum {
	CTL_IPP_DBG_TRC_NONE,	// dont use this when call CTL_IPP_DBG_TRC_PART(). use CTL_IPP_DBG_TRC() instead
	CTL_IPP_DBG_TRC_FLOW,	// print when g_ctl_ipp_dbg_trc_type == (CTL_IPP_DBG_TRC_XXX or CTL_IPP_DBG_TRC_ALL)
	CTL_IPP_DBG_TRC_BUF,
	CTL_IPP_DBG_TRC_ISP,
	CTL_IPP_DBG_TRC_ISE,
	CTL_IPP_DBG_TRC_EXAM,
	CTL_IPP_DBG_TRC_DBG,
	CTL_IPP_DBG_TRC_ALL,	// dont use this when call CTL_IPP_DBG_TRC_PART(). add new CTL_IPP_DBG_TRC_TYPE item instead
} CTL_IPP_DBG_TRC_TYPE;

/* for debug.h */
extern unsigned int ctl_ipp_debug_level;
extern UINT32 g_ctl_ipp_dbg_base_frame_rate; // default is 30 frames
extern CTL_IPP_DBG_TRC_TYPE g_ctl_ipp_dbg_trc_type; // default is CTL_IPP_DBG_TRC_NONE

/**
	debug log
	lv=1(ERR     ): print fatal error that only happen one time (ex. open/close/...)
	lv=2(WRN     ): print warning msg that only happen one time (ex. open/close/...)
	lv=3(ERR_FREQ): print fatal error that happen every frame
	lv=4(ERR_FREQ): print warning msg that happen every frame
	lv=5(IND     ): print tracing code that only happen one time (ex. open/close/...)
	lv=6(TRC     ): print tracing code that happen every frame
	lv=6(TRC_PART): print partial tracing code that happen every frame (defined by CTL_IPP_DBG_TRC_TYPE)
*/
#define CTL_IPP_DBG_ERR(fmt, args...)	DBG_ERR(fmt, ##args)
#define CTL_IPP_DBG_WRN(fmt, args...)	DBG_WRN(fmt, ##args)
#define CTL_IPP_DBG_IND(fmt, args...)	DBG_IND(fmt, ##args)
#define CTL_IPP_DBG_TRC(fmt, args...)	DBG_MSG("%s:" fmt, __func__, ##args)
#define CTL_IPP_DBG_DUMP(fmt, args...)	DBG_DUMP("%s:" fmt, __func__, ##args);

#define CTL_IPP_DBG_FREQ_SECOND(second) ((second) * g_ctl_ipp_dbg_base_frame_rate)
#define CTL_IPP_DBG_FREQ_NORMAL CTL_IPP_DBG_FREQ_SECOND(4)

#define CTL_IPP_DBG_ERR_FREQ(freq, args...) 								\
	do { 																	\
		static UINT8 __cnt = 0; 											\
		UINT8 __freq = (ctl_ipp_debug_level >= 3) ? 0 :  /* always print */ \
						(((freq) > 255) ? 255 : (freq)); /* print by freq */\
		if (__cnt == 0) { DBG_ERR(args); if (__freq != 0) __cnt++; } 		\
		else if (__cnt == __freq) __cnt = 0; 								\
		else __cnt++; 														\
	} while(0)

#define CTL_IPP_DBG_WRN_FREQ(freq, args...) 								\
	do { 																	\
		static UINT8 __cnt = 0; 											\
		UINT8 __freq = (ctl_ipp_debug_level >= 4) ? 0 :  /* always print */ \
						(((freq) > 255) ? 255 : (freq)); /* print by freq */\
		if (__cnt == 0) { DBG_WRN(args); if (__freq != 0) __cnt++; } 		\
		else if (__cnt == __freq) __cnt = 0; 								\
		else __cnt++; 														\
	} while(0)

#define CTL_IPP_DBG_DUMP_FREQ(freq, args...) 								\
	do { 																	\
		static UINT8 __cnt = 0; 											\
		UINT8 __freq = (ctl_ipp_debug_level >= 6) ? 0 :  /* always print */ \
						(((freq) > 255) ? 255 : (freq)); /* print by freq */\
		if (__cnt == 0) { DBG_DUMP(args); if (__freq != 0) __cnt++; } 		\
		else if (__cnt == __freq) __cnt = 0; 								\
		else __cnt++; 														\
	} while(0)

#define CTL_IPP_DBG_TRC_PART(type, args...)									\
	do {																	\
		if ((g_ctl_ipp_dbg_trc_type == type) || 							\
			(g_ctl_ipp_dbg_trc_type == CTL_IPP_DBG_TRC_ALL)) {				\
			CTL_IPP_DBG_TRC(args);											\
		}																	\
	} while(0)

#endif


