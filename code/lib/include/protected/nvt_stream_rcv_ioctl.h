/**
    Structure definitions and APIs used for nvt_stream_rcv io control.

    @file nvt_stream_rcv_ioctrl.h
    Copyright Novatek Microelectronics Corp. 2014. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/
#ifndef _NVT_STREAM_RCV_IOCTL_H_
#define _NVT_STREAM_RCV_IOCTL_H_

#include <asm/ioctl.h>

#define NVT_STREAM_RCV_IOC_MAGIC   'U'
#define NVT_STREAM_RCV_CMD_SIZE    12

/**
    @name nvt_stream_rcv error code.
*/
//@{
#define NVT_STREAM_RCV_RET_OK           0             ///<command OK
#define NVT_STREAM_RCV_RET_ERR          (-1)          ///<command fail
#define NVT_STREAM_RCV_RET_NO_FUNC      (-2)          ///<no operation
#define NVT_STREAM_RCV_RET_PAR_ERR      (-3)          ///<parameter error
#define NVT_STREAM_RCV_RET_IPC_ERR      (-4)          ///<IPC error
#define NVT_STREAM_RCV_RET_QUE_FULL     (-5)          ///<Queue full
#define NVT_STREAM_RCV_RET_BUF_ERR      (-99)         ///<buffer not enough
//@}

typedef enum {
	NVTSTREAM_RCV_DATA_TYPE_AUD  =   0x00000000,
	NVTSTREAM_RCV_DATA_TYPE_VID,
} NVTSTREAM_RCV_DATA_TYPE;

/**
    nvt stream_rcv data structure.
*/
typedef struct {
	unsigned int phy_Addr;                       ///< [in]data buffer
	unsigned int Size;                           ///< [in]data buffer size
	NVTSTREAM_RCV_DATA_TYPE Type;                ///< [in]data type
	unsigned int Reserved1;
} NVT_STREAM_RCV_DATA_ITEM;

/**
    nvt stream_rcv data structure.
*/
typedef struct {
	unsigned int CodecType;         ///< [in]video codec type
	unsigned int FrameRate;         ///< [in]video frame rate
	unsigned int Width;             ///< [in]video width
	unsigned int Height;            ///< [in]video height
} NVT_STREAM_RCV_VID_INFO_ITEM;

/**
    nvt stream_rcv command structure.
*/
typedef struct _NVT_STREAM_RCV_CMD {
	unsigned int cmd;                               ///< [in]command
	int          pid;                               ///< [in]PID
	NVT_STREAM_RCV_DATA_ITEM data;                  ///< [in/out]config data buffer
} NVT_STREAM_RCV_CMD, *PNVT_STREAM_RCV_CMD;

typedef struct _NVT_STREAM_RCV_VID_INFO {
	unsigned int cmd;                               ///< [in]command
	int          pid;                               ///< [in]PID
	NVT_STREAM_RCV_VID_INFO_ITEM info;              ///< [in/out]config video info
} NVT_STREAM_RCV_VID_INFO, *PNVT_STREAM_RCV_VID_INFO;

#define NVT_STREAM_RCV_SEND_DATA      _IOWR(NVT_STREAM_RCV_IOC_MAGIC, 45,NVT_STREAM_RCV_CMD)
#define NVT_STREAM_RCV_GET_CONFIG_CMD _IOWR(NVT_STREAM_RCV_IOC_MAGIC, 47,NVT_STREAM_RCV_CMD)
#define NVT_STREAM_RCV_SET_VID_CONFIG _IOWR(NVT_STREAM_RCV_IOC_MAGIC, 48,NVT_STREAM_RCV_VID_INFO)


#endif
