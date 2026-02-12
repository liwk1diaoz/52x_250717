#ifndef _FSLINUX_DEBUG_H_
#define _FSLINUX_DEBUG_H_

#include <stdio.h>
#include <sys/time.h>

#define DBG_COLOR_RED "^R"
#define DBG_COLOR_BLUE "^B"
#define DBG_COLOR_YELLOW "^Y"

#define CHKPNT      printf("\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__)
#define DBGD(x)     printf("\033[0;35m%s=%d\033[0m\r\n",#x,x)                  ///< Show a color sting of variable name and variable deciaml value
#define DBGH(x)     printf("\033[0;35m%s=0x%08X\033[0m\r\n",#x,x)
#define DBG_ERR(fmtstr, args...) printf("\033[0;31mERR:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_WRN(fmtstr, args...) printf("\033[0;33mWRN:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_DUMP(fmtstr, args...) printf(fmtstr, ##args)

#define TRACE_LOG 0
#define PERF_LOG 0

#if TRACE_LOG //debug
#define DBG_IND(fmtstr, args...) printf("%s(): " fmtstr, __func__, ##args)
#else //release
#define DBG_IND(...)
#endif

#if PERF_LOG
#define PERF_PRINT(fmtstr, args...) printf("\033[0;35mPERF:%s() \033[0m" fmtstr, __func__, ##args)
#define PERF_VAL(name) struct timeval name
#define PERF_MARK(p) gettimeofday(&p, NULL)
#define PERF_DIFF(from, to) ((to.tv_sec * 1000000 + to.tv_usec) - (from.tv_sec * 1000000 + from.tv_usec))
#else
#define PERF_PRINT(...)
#define PERF_VAL(name)
#define PERF_MARK(p)
#define PERF_DIFF(from, to)
#endif

#endif //_FSLINUX_DEBUG_H_