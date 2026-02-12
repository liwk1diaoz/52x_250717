/**
    Header file of Novatek Media TS (Transport Stream) Demux Task

    Exported header file of novatek media TS (Transport Stream) demux task.

    @file       NMediaTsDemuxTsk.h
    @ingroup    mIAPPMEDIAPLAY

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _NMEDIATSDEMUXTSK_H
#define _NMEDIATSDEMUXTSK_H

#define NMP_TSDEMUX_PRI              8                                 ///< priority
#define NMP_TSDEMUX_STKSIZE          1024                              ///< stack size

/**
	File read block size.
*/
#define         NMP_TSDEMUX_BUFBLK_UNIT_SIZE           0x200000        ///< buffer block unit size: 2M bytes

/**
    Flags for NMediaPlay BsDemux
*/
#define FLG_NMP_TSDEMUX_IDLE         FLGPTN_BIT(0)
#define FLG_NMP_TSDEMUX_DEMUX        FLGPTN_BIT(1)
#define FLG_NMP_TSDEMUX_DONE         FLGPTN_BIT(2)
#define FLG_NMP_TSDEMUX_STOP         FLGPTN_BIT(3)

/**
    NMedia Play TS Demux Buffer Block information
*/
typedef enum _NMP_TSDEMUX_BUFBLK
{
    NMP_TSDEMUX_BUFBLK_0 = 0,
    NMP_TSDEMUX_BUFBLK_1,
    NMP_TSDEMUX_BUFBLK_2,
	NMP_TSDEMUX_BUFBLK_3,
	NMP_TSDEMUX_BUFBLK_4,
	NMP_TSDEMUX_BUFBLK_5,
	NMP_TSDEMUX_BUFBLK_6,
    NMP_TSDEMUX_BUFBLK_CNT,
    ENUM_DUMMY4WORD(_NMP_TSDEMUX_BUFBLK)
} NMP_TSDEMUX_BUFBLK;

typedef enum {
	NMP_TSDEMUX_SETINFO_SRC_POS_SIZE = 0,                             ///< source position and size, p1: read mem addr(in), p2: read size(in)
	NMP_TSDEMUX_SETINFO_CLEAR_FRAME_ENTRY,                            ///< clear frame entry, p1: demux dst addr(in), p2: size(in)
} NMP_TSDEMUX_SETINFO;

/**
    NMediaPlay TsDemux Object
*/
typedef struct _NMP_TSDEMUX_OBJ {
	UINT32      read_pos;
	UINT32      read_size;
	UINT32      dst_addr;
	UINT32      used_size;
	UINT32      width;
	UINT32      height;
	UINT32      bufblk_num;         ///< buffer block number (default use NMP_TSDEMUX_BUFBLK_CNT)
	UINT32		need_read_oneblk;
} NMP_TSDEMUX_OBJ, *PNMP_TSDEMUX_OBJ;

/**
    NMediaPlay TsDemux Buffer Information
*/
typedef struct
{
	UINT32      start_addr;         ///< start address
	UINT32      end_addr;           ///< end address
	UINT32      cur_addr;           ///< current used address
	UINT64      read_accum_size;
	UINT64      demux_accum_size;
	UINT64      use_accum_size;
	UINT64      file_remain_size;
	UINT64      file_ttlsize;       ///< total file size
	BOOL        b_read_finished;
} NMP_TSDEMUX_BUFINFO, *PNMP_TSDEMUX_BUFINFO;

extern UINT32 _NMP_TsDemux_GetBufSize(UINT32 pathID);
extern ER     _NMP_TSDemux_ConfigBufInfo(UINT32 buf_addr, UINT32 buf_size);
extern ER     _NMP_TsDemux_TskStart(void);
extern void   _NMP_TsDemux_Open(void);
extern ER     _NMP_TsDemux_Close(void);
extern ER     _NMP_TSDemux_SetInfo(UINT32 type, UINT32 param1, UINT32 param2);
// test command
extern BOOL   Cmd_TsDemux_SetDebug(CHAR *strCmd);

#endif
