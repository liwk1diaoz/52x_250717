#ifndef HAL_REMOTE_OP_H
#define HAL_REMOTE_OP_H
#include <kwrap/type.h>
#include "hal_remote_uart.h"

#define HAL_FD_INITIALIZATION_VAL (-1)

#define REMOTE_DATA_SIZE 1
#define REMOTE_MODE_LEN        30

typedef struct {
    UINT32 wantReadLen;   /* want read length */
    UINT32 actualReadLen; /* actual read length */
    UINT8  rawData[REMOTE_DATA_SIZE];
} REMOTE_RAW_DATA;

typedef struct {
    char uartDev[16]; /* valid only when remote data transmitter on uart interface */
} REMOTE_OBJ_DATA;

typedef struct {
    INT32 (*init)(const REMOTE_OBJ_DATA *data);
    INT32 (*deinit)(void);
    INT32 (*getRawData)(REMOTE_RAW_DATA *pRemoteData, INT32 timeoutMsec);
    INT32 (*SendTestString)(void);
} REMOTE_OP;

typedef struct {
    REMOTE_OBJ_DATA data;
    REMOTE_OP op;
}REMOTE_OBJ;

extern INT32 HAL_Remote_GetOp(REMOTE_OP *op);

#endif
