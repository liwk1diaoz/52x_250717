#ifndef _NVTWIFI_INT_H_
#define _NVTWIFI_INT_H_

#if 0 //debug
#define NVTWIFI_PRINT(fmtstr, args...) diag_printf("eCos: %s() " fmtstr, __func__, ##args)
#else //release
#define NVTWIFI_PRINT(...)
#endif

#define NVTWIFI_ERR(fmtstr, args...) diag_printf("eCoooooos: ERR %s() " fmtstr, __func__, ##args)

#endif //_NVTWIFI_DEBUG_H_



#define ipc_isCacheAddr(addr)       NvtIPC_IsCacheAddr(addr)
#define ipc_getPhyAddr(addr)        NvtIPC_GetPhyAddr(addr)
#define ipc_getNonCacheAddr(addr)   NvtIPC_GetNonCacheAddr(addr)


