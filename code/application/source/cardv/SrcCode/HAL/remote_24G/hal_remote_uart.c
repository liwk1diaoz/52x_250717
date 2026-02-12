/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2017-2019. All rights reserved.
 * Description: product hal uart implemention
 * Author: HiMobileCam Reference Develop Team
 * Create: 2017-12-18
 * file    hal_uart.c
 */
#include <stdlib.h>
#include "hal_remote_uart.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <limits.h>
#include "hal_remote_uart.h"
#include <errno.h>
#include "kwrap/error_no.h"
#include <kwrap/debug.h>


#define VTIME_VALUE 50 /* Set timeout 50/10 seconds */

typedef struct {
    UART_BitRate bitRate;
    INT32 value;
} UART_Speed;

static UART_Speed g_uartSpeedTable[] = {
    {UART_BITRATE_115200, B115200},
    {UART_BITRATE_38400, B38400},
    {UART_BITRATE_19200, B19200},
    {UART_BITRATE_9600, B9600},
    {UART_BITRATE_4800, B4800},
    {UART_BITRATE_2400, B2400},
    {UART_BITRATE_1200, B1200},
    {UART_BITRATE_300, B300},
};

static INT32 UART_SetSpeed(INT32 fd, UART_BitRate speed)
{
    UINT32 i;
    INT32 status;

    for (i = 0; i < (sizeof(g_uartSpeedTable) / sizeof(g_uartSpeedTable[0])); i++) {
        if (speed == g_uartSpeedTable[i].bitRate) {
            struct termios opt;

            tcgetattr(fd, &opt);
            tcflush(fd, TCIOFLUSH);
            cfsetispeed(&opt, g_uartSpeedTable[i].value);
            cfsetospeed(&opt, g_uartSpeedTable[i].value);
            status = tcsetattr(fd, TCSANOW, &opt);
            if (status != 0) {
                perror("tcsetattr fd1");
            }
            tcflush(fd, TCIOFLUSH);
            break;
        }
    }

    if (i == (sizeof(g_uartSpeedTable) / sizeof(g_uartSpeedTable[0]))) {
        DBG_ERR("no this bitrate\n");
        return E_SYS;
    }

    return E_OK;
}

/* input :dataBits output:io */
static INT32 UART_GetDataBitsCtrlFlg(UART_DataBit dataBits, struct termios *io)
{
    switch (dataBits) { /* Set the number of data bits */
        case UART_DATABIT_7:
            io->c_cflag |= CS7;
            break;

        case UART_DATABIT_8:
            io->c_cflag |= CS8;
            break;
        default:
            fprintf(stderr, "Unsupported data size\n");
            return (E_SYS);
    }

    return E_OK;
}

/* input :stopBits output:io */
static INT32 UART_GetStopBitsCtrlFlg(UART_StopBit stopBits, struct termios *io)
{
    /* stop bit */
    switch (stopBits) {
        case UART_STOPBIT_1:
            io->c_cflag &= ~CSTOPB;
            break;
        case UART_STOPBIT_2:
            io->c_cflag |= CSTOPB;
            break;

        default:
            fprintf(stderr, "Unsupported stop bits\n");
            return (E_SYS);
    }

    return E_OK;
}

/* input :parity output:io */
static INT32 UART_GetParityCtrlFlg(UART_Parity parity, struct termios *io)
{
    /* parity */
    switch (parity) {
        case UART_PARITY_N:
            io->c_cflag &= ~PARENB; /* Clear parity enable */
            io->c_iflag &= ~INPCK;  /* Enable parity checking */
            break;

        case UART_PARITY_O:
            io->c_cflag |= (PARODD | PARENB); /* Set to odd effect */
            io->c_iflag |= INPCK;             /* Disnable parity checking */
            break;

        case UART_PARITY_E:
            io->c_cflag |= PARENB;  /* Enable parity */
            io->c_cflag &= ~PARODD; /* Conversion to even test */
            io->c_iflag |= INPCK;   /* Disnable parity checking */
            break;

        default:
            fprintf(stderr, "Unsupported parity\n");
            return (E_SYS);
    }

    return E_OK;
}

static INT32 UART_SetDataFormat(INT32 fd, UART_DataBit dataBits, UART_StopBit stopBits, UART_Parity parity)
{
    struct termios options = {0};
    INT32 ret;

    if (tcgetattr(fd, &options) != 0) {
        DBG_ERR("tcgetattr error");
        return (E_SYS);
    }

    ret = UART_GetDataBitsCtrlFlg(dataBits, &options);
    if (ret != E_OK) {
        DBG_ERR("input param error\n");
        return (E_SYS);
    }

    ret = UART_GetStopBitsCtrlFlg(stopBits, &options);
    if (ret != E_OK) {
        DBG_ERR("input param error\n");
        return (E_SYS);
    }
    ret = UART_GetParityCtrlFlg(parity, &options);
    if (ret != E_OK) {
        DBG_ERR("input param error\n");
        return (E_SYS);
    }

    tcflush(fd, TCIFLUSH);
    options.c_cc[VTIME] = VTIME_VALUE; /* Set timeout VTIME_VALUE/10 seconds */
    options.c_cc[VMIN] = 0;   /* Update the options and do it NOW */

    options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IXOFF | IXANY);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | IEXTEN);
    options.c_oflag &= ~OPOST;

    if (tcsetattr(fd, TCSANOW, &options) != 0) {
        DBG_ERR("SetupSerial 3");
        return (E_SYS);
    }

    return E_OK;
}

INT32 HAL_UART_Init(const char *uartDev, INT32 *uartFd)
{
    *uartFd = open(uartDev, O_RDWR | O_NOCTTY | O_NONBLOCK);

    if (*uartFd == -1) {
        DBG_ERR("uart dev open failed : %s <%s>\n",uartDev,strerror(errno));
        return E_SYS;
    }

    return E_OK;
}

INT32 HAL_UART_SetAttr(INT32 uartFd, const UART_Attr *uartAttr)
{
    INT32 ret = UART_SetSpeed(uartFd, uartAttr->bitRate);

    if (ret != E_OK) {
        DBG_ERR("Set bit rate Error\n");
        return E_SYS;
    }

    ret = UART_SetDataFormat(uartFd, uartAttr->dataBits, uartAttr->stopBits, uartAttr->parity);
    if (ret != E_OK) {
        DBG_ERR("Set Parity Error\n");
        return E_SYS;
    }

    return E_OK;
}

INT32 HAL_UART_Deinit(INT32 uartFd)
{
    close(uartFd);
    return E_OK;
}
