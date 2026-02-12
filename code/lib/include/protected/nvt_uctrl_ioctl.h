/**
    Structure definitions and APIs used for nvt_uctrl io control.

    @file nvt_uctrl_ioctrl.h
    Copyright Novatek Microelectronics Corp. 2014. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#ifndef _NVT_UCTRL_IOCTL_H_
#define _NVT_UCTRL_IOCTL_H_

#include <asm/ioctl.h>
#include <nvt_type.h>

#define NVT_UCTRL_IOC_MAGIC        'U'
#define NVT_UCTRL_CMD_SIZE         256           ///<maximum command length
#define NVT_UCTRL_CONFIG_SIZE      40960         ///<maximum config length
#define NVT_UCTRL_RAW_SIZE         (64*1024)     ///<maximum raw size

/**
    @name nvt_uctrl error code.
*/
//@{
#define NVT_UCTRL_RET_OK           0             ///<command OK
#define NVT_UCTRL_RET_ERR          (-1)          ///<command fail
#define NVT_UCTRL_RET_NO_FUNC      (-2)          ///<no operation
#define NVT_UCTRL_RET_PAR_ERR      (-3)          ///<parameter error
#define NVT_UCTRL_RET_IPC_ERR      (-4)          ///<IPC error
#define NVT_UCTRL_RET_BUF_ERR      (-99)         ///<buffer not enough
//@}


/**
    nvt uctrl command structure.
*/
typedef struct _NVT_UCTRL_CMD {
	UINT32       cmd;                               ///< [in]command
	UINT32       cmd_size;                          ///< [in]command size
	UINT32       config_data;                       ///< [in/out]config data buffer
	int          config_size;                       ///< [in/out]config data buffer size
	unsigned int ret_value;                         ///< [out]command result
	UINT32       buf;                               ///< [out]result string buffer
	unsigned int buf_size;                          ///< [out]result string buffer size
} NVT_UCTRL_CMD, *PNVT_UCTRL_CMD;

/**
    nvt uctrl notify structure.
*/
typedef struct _NVT_UCTRL_NOTIFY {
	unsigned int ret_value;                         ///< [out]notify result
	UINT32       buf;                               ///< [out]notify string buffer
	unsigned int buf_size;                          ///< [out]notify string buffer size
} NVT_UCTRL_NOTIFY, *PNVT_UCTRL_NOTIFY;


//misc from 40
#define NVT_UCTRL_SET_CMD        _IOWR(NVT_UCTRL_IOC_MAGIC, 45,NVT_UCTRL_CMD)
#define NVT_UCTRL_WAIT_NOTIFY    _IOWR(NVT_UCTRL_IOC_MAGIC, 46,NVT_UCTRL_NOTIFY)
#define NVT_UCTRL_GET_CONFIG_CMD _IOWR(NVT_UCTRL_IOC_MAGIC, 47,NVT_UCTRL_CMD)
#define NVT_UCTRL_SET_CONFIG_CMD _IOWR(NVT_UCTRL_IOC_MAGIC, 48,NVT_UCTRL_CMD)
#define NVT_UCTRL_WAIT_NOTIFY_RAW _IOWR(NVT_UCTRL_IOC_MAGIC,49,NVT_UCTRL_NOTIFY)

// exproted APIs for kernel driver (e.g: nvt_status)
int nvt_uctrl_cmdsnd(char *cmd, char *ret_addr, int ret_size);
int nvt_uctrl_cmdrcv(char *cmd, int ret_size);


#endif
