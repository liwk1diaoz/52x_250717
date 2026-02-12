/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2018-2019. All rights reserved.
 * Description: product gps uart interface
 * Author: HiMobileCam Reference Develop Team
 * Create: 2018-12-19
 * file    hi_gps_uart.c
 */
#include "hal_remote_op.h"
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "kwrap/error_no.h"
#include <kwrap/debug.h>


static INT32 g_RemoteUartFd = HAL_FD_INITIALIZATION_VAL;

static INT32 HAL_Remote_UartInit(const REMOTE_OBJ_DATA *pData)
{
    INT32 ret;

    if (g_RemoteUartFd != HAL_FD_INITIALIZATION_VAL) {
        DBG_ERR("Remote already init\n");
        return E_OK;
    }

    ret = HAL_UART_Init(pData->uartDev, &g_RemoteUartFd);
    if (ret != E_OK) {
        DBG_ERR("uart init failed\n");
        return ret;
    }

    UART_Attr uartAttr = { 0 };
    uartAttr.bitRate = UART_BITRATE_19200;
    uartAttr.dataBits = UART_DATABIT_8;
    uartAttr.stopBits = UART_STOPBIT_1;
    uartAttr.parity = UART_PARITY_N;

    ret = HAL_UART_SetAttr(g_RemoteUartFd, &uartAttr);
    if (ret != E_OK) {
        DBG_ERR("uart set attr failed\n");
        return ret;
    }

    return E_OK;
}

static INT32 HAL_Remote_UartDeinit(void)
{
    INT32 ret;
    if (g_RemoteUartFd == HAL_FD_INITIALIZATION_VAL) {
        DBG_ERR("Uart device has already deinit!\n");
        return E_SYS;
    }

    ret = HAL_UART_Deinit(g_RemoteUartFd);
    if (ret != E_OK) {
        DBG_ERR("uart Deinit failed\n");
        return ret;
    }

    g_RemoteUartFd = HAL_FD_INITIALIZATION_VAL;

    return E_OK;
}

static INT32 HAL_Remote_GetUartRawData(REMOTE_RAW_DATA *pRemoteData, INT32 timeoutMsec)
{
    INT32 ret;
    fd_set readFds;
    struct timeval time;

    if ((pRemoteData == NULL) || (pRemoteData->wantReadLen > REMOTE_DATA_SIZE)) {
        DBG_ERR("pRemoteData is NULL pointer or Too much data acquired at one time len%d !\n", pRemoteData->wantReadLen);
        return E_SYS;
    }

    if (g_RemoteUartFd == HAL_FD_INITIALIZATION_VAL) {
        DBG_ERR("g_RemoteUartFd == HAL_FD_INITIALIZATION_VAL!\n");
        return E_SYS;
    }

    FD_ZERO(&readFds);
    FD_SET(g_RemoteUartFd, &readFds);
    time.tv_sec = timeoutMsec / 1000U;
    time.tv_usec = (timeoutMsec % 1000U) * 1000U;
    if (timeoutMsec == 0) {
        ret = select(g_RemoteUartFd + 1, &readFds, NULL, NULL, NULL); //no timeout
    } else {
        ret = select(g_RemoteUartFd + 1, &readFds, NULL, NULL, &time);
    }

    if (ret < 0) {
        if (errno == EAGAIN) {
            return E_SYS;
        } else {
            DBG_ERR("select failed!\n");
            return E_SYS;
        }
    } else if (ret == 0) {
        //MLOGE("get gps data timeout!\n");
        pRemoteData->actualReadLen = 0;
        return E_SYS;
    }
    if (FD_ISSET(g_RemoteUartFd, &readFds)) {
        ret = read(g_RemoteUartFd, pRemoteData->rawData, 1);
        if (ret < 0) {
            if (errno == EAGAIN) {
                DBG_ERR("No data readable!\n");
                pRemoteData->actualReadLen = 0;
                return E_SYS;
            } else {
                DBG_ERR("read data faild!\n");
                return E_SYS;
            }
        }
    }

    pRemoteData->actualReadLen = ret;

    return E_OK;
}

static INT32 HAL_Remote_SendTestString(void)
{
    INT32 ret;
    char remote_mode[REMOTE_MODE_LEN] = {0};

    if (g_RemoteUartFd == HAL_FD_INITIALIZATION_VAL) {
        DBG_ERR("g_RemoteUartFd == HAL_FD_INITIALIZATION_VAL!\n");
        return E_SYS;
    }

    snprintf(remote_mode,REMOTE_MODE_LEN,"%s","test string");
    ret = write(g_RemoteUartFd, remote_mode, strlen(remote_mode));
    if (ret == -1)
    {
        DBG_ERR("set test string failed = %d\n",ret);
        return E_SYS;
    }

    return E_OK;
}


INT32 HAL_Remote_GetOp(REMOTE_OP *op)
{
    if (op == NULL) {
        DBG_ERR("[Errot]ops is null,error\n");
        return E_SYS;
    }

    op->init = HAL_Remote_UartInit;
    op->deinit = HAL_Remote_UartDeinit;
    op->getRawData = HAL_Remote_GetUartRawData;
    op->SendTestString = HAL_Remote_SendTestString;

    return E_OK;
}

