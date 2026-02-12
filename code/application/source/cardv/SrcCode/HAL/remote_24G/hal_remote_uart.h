/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2017-2019. All rights reserved.
 * Description: product inner hal uart interface
 * Author: HiMobileCam Reference Develop Team
 * Create: 2017-12-18
 */

#ifndef HAL_REMOTE_UART_H
#define HAL_REMOTE_UART_H
#include <kwrap/type.h>

typedef enum {
    UART_BITRATE_300,
    UART_BITRATE_1200,
    UART_BITRATE_2400,
    UART_BITRATE_4800,
    UART_BITRATE_9600,
    UART_BITRATE_19200,
    UART_BITRATE_38400,
    UART_BITRATE_115200,
    UART_BITRATE_BUTT,
} UART_BitRate;

typedef enum {
    UART_DATABIT_7,
    UART_DATABIT_8,
    UART_DATABIT_BUTT,
} UART_DataBit;

typedef enum {
    UART_STOPBIT_1,
    UART_STOPBIT_2,
    UART_STOPBIT_BUTT,
} UART_StopBit;

typedef enum {
    UART_PARITY_N,
    UART_PARITY_O,
    UART_PARITY_E,
    UART_PARITY_BUTT,
} UART_Parity;

typedef struct {
    UART_DataBit bitRate;
    UART_DataBit dataBits;
    UART_StopBit stopBits;
    UART_Parity parity; /* Even -> E, odd ->O, no ->N */
} UART_Attr;

INT32 HAL_UART_Init(const char *uartDev, INT32 *uartFd);

INT32 HAL_UART_SetAttr(INT32 uartFd, const UART_Attr *uartAttr);

INT32 HAL_UART_Deinit(INT32 uartFd);

#endif /* End of HAL_REMOTE_UART_H */

