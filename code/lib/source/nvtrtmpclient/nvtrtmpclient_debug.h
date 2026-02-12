/**
    Header file of NvtRtmpClient (internal)

    @file       nvtrtmpclient_debug.h
    @ingroup    NVTRTMPCLIENT
    @version    V1.00.000
    @date       2018/03/14

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _NVTRTMPCLIENT_DEBUG_H_
#define _NVTRTMPCLIENT_DEBUG_H_

#include <stdio.h>
#include <sys/prctl.h>
#ifdef __cplusplus
extern "C" {
#endif

#define NAMED_THREAD()       \
        {   \
            char thread_name[16];   \
            snprintf(thread_name, sizeof(thread_name), "%s", __FUNCTION__); \
            prctl(PR_SET_NAME, (unsigned long)thread_name); \
        }


#define DBG_COLOR_RED "^R"
#define DBG_COLOR_BLUE "^B"
#define DBG_COLOR_YELLOW "^Y"

#define CHKPNT      printf("\033[37mCHK: %d, %s\033[0m\r\n",__LINE__,__func__)
#define DBGD(x)     printf("\033[0;35m%s=%d\033[0m\r\n",#x,x)                  ///< Show a color sting of variable name and variable deciaml value
#define DBGH(x)     printf("\033[0;35m%s=0x%08X\033[0m\r\n",#x,x)
#define DBG_ERR(fmtstr, args...) printf("\033[0;31mERR:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_WRN(fmtstr, args...) printf("\033[0;33mWRN:%s() \033[0m" fmtstr, __func__, ##args)
#define DBG_DUMP(fmtstr, args...) printf(fmtstr, ##args)

#if 0 //debug
#define DBG_IND(fmtstr, args...) printf("%s(): " fmtstr, __func__, ##args)
#else //release
#define DBG_IND(...)
#endif

#ifdef __cplusplus
}
#endif
/* ----------------------------------------------------------------- */
#endif /* _NVTRTMPCLIENT_DEBUG_H_  */
