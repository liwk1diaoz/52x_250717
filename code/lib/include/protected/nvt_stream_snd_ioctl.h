/**
    Structure definitions and commands used for IOCTL.

    @file nvt_stream_snd_ioctl.h
    Copyright Novatek Microelectronics Corp. 2014. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef _NVT_STREAM_IOCTL_H_
#define _NVT_STREAM_IOCTL_H_



#define NVT_STREAM_IOC_MAGIC 'S'
#include <asm/ioctl.h>
#include <linux/types.h>
typedef __u64 SSENDER_U64;
typedef __u32 SSENDER_U32;
typedef __u16 SSENDER_U16;
typedef __u8 SSENDER_U8;
typedef __s32 SSENDER_S32;

typedef struct {
	SSENDER_U64   tv_sec;     /* seconds */
	SSENDER_U64   tv_usec;    /* microseconds */
} NVT_STREAM_TIME;

typedef struct {
	SSENDER_U32   bIsAudio;
	SSENDER_U32   ChanId; //start from 0
	SSENDER_S32   Timeout_ms; //msec
} NVT_STREAM_IOC_IN_ARG;

typedef struct {
	SSENDER_U32   bIsAudio;
	SSENDER_U32   ChanId; //start from 0
} NVT_STREAM_IOC_IN_RELEASE;

typedef struct {
	SSENDER_U64   ItemSN; //the serial no. of each stream data
	SSENDER_U64   MaxSN; //the max serial no. of the current stream
	SSENDER_U32   phy_Addr;
	SSENDER_U32   Size;
	SSENDER_U8    SVCLayerId;
	SSENDER_U8    FrameType;
	SSENDER_U8    is_new;
	SSENDER_U8    reserved;
	NVT_STREAM_TIME TimeStamp;
} NVT_STREAM_IOC_OUT_ITEM;

typedef struct {
	SSENDER_U32   phy_Addr;
	SSENDER_U32   Size;
	SSENDER_U32   CodecType;
} NVT_STREAM_IOC_OUT_VINFO;

typedef struct{
	SSENDER_U32   CodecType;
	SSENDER_U32   ChannelCnt;
	SSENDER_U32   BitsPerSample;
	SSENDER_U32   SampePerSecond;
}NVT_STREAM_IOC_OUT_AINFO;

typedef struct {
	SSENDER_U32   phy_Addr;
	SSENDER_U32   Size;
	SSENDER_U32   bIsAutoRel;
} NVT_STREAM_IOC_OUT_IPC_PARAM;

typedef struct {
	NVT_STREAM_IOC_IN_ARG InArg;
	NVT_STREAM_IOC_OUT_ITEM OutItem;
} NVT_STREAM_IOCARG_GET_ITEM;

typedef struct {
	NVT_STREAM_IOC_IN_ARG InArg;
	NVT_STREAM_IOC_OUT_VINFO OutItem;
} NVT_STREAM_IOCARG_GET_VDOINFO;

typedef struct{
    NVT_STREAM_IOC_IN_ARG InArg;
    NVT_STREAM_IOC_OUT_AINFO OutItem;
}NVT_STREAM_IOCARG_GET_AINFO;

typedef struct {
	NVT_STREAM_IOC_OUT_IPC_PARAM OutItem;
} NVT_STREAM_IOCARG_GET_IPC_PARAM;

typedef struct {
	NVT_STREAM_IOC_IN_ARG InArg;
	NVT_STREAM_IOC_OUT_ITEM OutItem;
} NVT_STREAM_IOCARG_GETLOCK_ITEM;

typedef struct {
	NVT_STREAM_IOC_IN_RELEASE InArg;
} NVT_STREAM_IOCARG_RELEASE_ITEM;

typedef struct {
	SSENDER_U32   IsReady;
	SSENDER_U64   ItemSN; //the serial no. of each stream data
	SSENDER_U32   Size;
	SSENDER_U32   PhyAddr;
	NVT_STREAM_TIME TimeStamp;
} NVT_STREAM_IOC_OUT_POLL_RESULT;

typedef struct {
	NVT_STREAM_IOC_IN_ARG InArg;
	NVT_STREAM_IOC_OUT_POLL_RESULT OutPollResult;
} NVT_STREAM_IOCARG_POLL_ITEM;


//ioctl
#define NVT_STREAM_IOCCMD_GET_ITEM      _IOR(NVT_STREAM_IOC_MAGIC, 1, NVT_STREAM_IOCARG_GET_ITEM)
#define NVT_STREAM_IOCCMD_GET_VINFO     _IOR(NVT_STREAM_IOC_MAGIC, 2, NVT_STREAM_IOCARG_GET_VDOINFO)
#define NVT_STREAM_IOCCMD_GET_IPC_PARAM _IOR(NVT_STREAM_IOC_MAGIC, 3, NVT_STREAM_IOCARG_GET_IPC_PARAM)
#define NVT_STREAM_IOCCMD_POLL_ITEM     _IOR(NVT_STREAM_IOC_MAGIC, 4, NVT_STREAM_IOCARG_POLL_ITEM)
#define NVT_STREAM_IOCCMD_GET_AINFO     _IOR(NVT_STREAM_IOC_MAGIC, 5, NVT_STREAM_IOCARG_GET_AINFO)
#define NVT_STREAM_IOCCMD_GETLOCK_ITEM  _IOR(NVT_STREAM_IOC_MAGIC, 6, NVT_STREAM_IOCARG_GETLOCK_ITEM)
#define NVT_STREAM_IOCCMD_RELEASE_ITEM  _IOR(NVT_STREAM_IOC_MAGIC, 7, NVT_STREAM_IOCARG_RELEASE_ITEM)

#endif
