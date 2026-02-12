#ifndef _ETHSOCKETCLI_INT_H_
#define _ETHSOCKETCLI_INT_H_

#if defined(__ECOS)
#include <cyg/infra/diag.h>
#endif

#ifndef __USE_IPC
#define __USE_IPC              0
#endif

#if 0//debug
#if defined(__ECOS)
#define ETHSOCKETCLI_PRINT(path_id, id, fmtstr, args...) diag_printf("eCos: %s():%d,%d " fmtstr, __func__,path_id,id, ##args)
#else
#define ETHSOCKETCLI_PRINT(path_id, id, fmtstr, args...) printf("ethsocket: %s():%d,%d " fmtstr, __func__,path_id,id, ##args)
#endif
#else //release
#define ETHSOCKETCLI_PRINT(...)
//#define ETHSOCKETCLI_PRINT(path_id, id, fmtstr, args...)  ((path_id==1) ? diag_printf("eCos: %s():%d,%d " fmtstr, __func__,path_id,id, ##args) : diag_printf(""))

#endif

#if defined(__ECOS)
#define ETHSOCKETCLI_ERR(path_id, id, fmtstr, args...) diag_printf("eCos:ERR:%s:%d,%d " fmtstr, __func__,path_id,id, ##args)
#else
#define ETHSOCKETCLI_ERR(path_id, id, fmtstr, args...) printf("ethsocket:ERR:%s:%d,%d " fmtstr, __func__,path_id,id, ##args)
#endif


typedef enum _ETHSOCKIPCCLI_ID {
	ETHSOCKIPCCLI_ID_0 = 0,       ///< ethsocket cli 0
	ETHSOCKIPCCLI_ID_1 = 1,          ///< ethsocket cli 1
	ETHSOCKIPCCLI_ID_2 = 2,          ///< ethsocket cli 2
	ETHSOCKIPCCLI_MAX_NUM,     ///< Number of ethsocket
	ENUM_DUMMY4WORD(ETHSOCKIPCCLI_ID)
} ETHSOCKIPCCLI_ID;

typedef enum _ETHCAM_PATH_ID {
	ETHCAM_PATH_ID_1,   //Tx 1
	ETHCAM_PATH_ID_2,   //Tx 2
	ETHCAM_PATH_ID_MAX,
	ENUM_DUMMY4WORD(ETHCAM_PATH_ID)
} ETHCAM_PATH_ID;

#endif //_ETHSOCKETCLI_INT_H_

