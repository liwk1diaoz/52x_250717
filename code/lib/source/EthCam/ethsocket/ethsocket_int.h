#ifndef _ETHSOCKET_INT_H_
#define _ETHSOCKET_INT_H_

#if defined(__ECOS)
#include <cyg/infra/diag.h>
#endif

#ifndef __USE_IPC
#define __USE_IPC              0
#endif

#if 0 //debug
#if defined(__ECOS)
#define ETHSOCKET_PRINT(id,fmtstr, args...) diag_printf("[%s:%d]"fmtstr"", __func__,id, ##args)
#else
#define ETHSOCKET_PRINT(id,fmtstr, args...) printf("[ethsocket:%s:%d]"fmtstr"", __func__,id, ##args)
#endif
#else //release
#define ETHSOCKET_PRINT(...)
#endif

#if defined(__ECOS)
#define ETHSOCKET_ERR(id,fmtstr, args...) diag_printf("[ERR:%s:%d]"fmtstr"", __func__,id, ##args)
#else
#define ETHSOCKET_ERR(id,fmtstr, args...) printf("[ethsocket:ERR:%s:%d]"fmtstr"", __func__,id, ##args)
#endif


/**
     ethsocket ID
*/
typedef enum _ETHSOCKIPC_ID {
	ETHSOCKIPC_ID_0 = 0,       ///< ethsocket 0 //data1, record
	ETHSOCKIPC_ID_1 = 1,       ///< ethsocket 1 //data2, display
	ETHSOCKIPC_ID_2 = 2,       ///< ethsocket 2 //cmd
	ETHSOCKIPC_MAX_NUM,        ///< Number of ethsocket
} ETHSOCKIPC_ID;

#endif //_ETHSOCKET_INT_H_
