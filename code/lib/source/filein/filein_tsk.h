/**
    Header file of Novatek Media File In Task

    Exported header file of novatek media file in task.

    @file       NMediaFileInTsk.h
    @ingroup    mIAPPMEDIAPLAY

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#ifndef _NMEDIAFILEINTSK_H
#define _NMEDIAFILEINTSK_H

#if defined (__UITRON) || defined (__ECOS)
#include <stdio.h>
#include <string.h>
#include "Type.h"
#include "kernel.h"
#include "SxCmd.h"
#include "FileSysTsk.h"
#else
#include <stdio.h>
#include <string.h>
#include "kwrap/type.h"
#include "FileSysTsk.h"
#include "filein.h"
#include "avfile/MediaReadLib.h"
#endif

#define SIZE_32X(a)                 (((a + 31)>>5)<<5)

#define FILEIN_JOBQ_MAX         10                              ///< 10 for one path
#define FILEIN_PRI              8                               ///< priority
#define FILEIN_STKSIZE          1024                            ///< stack size
#define FILEIN_MAX_PATH         2

// Read file block default setting
#define FILEIN_BLK_TIME         1                               ///< play time per one block (seconds)
#define FILEIN_PL_BLKNUM        3                               ///< preload block number
#define FILEIN_RSV_BLKNUM       1                               ///< reserve block number

// Flags for NMediaPlay FileIn
//#define FLG_FILEIN_IDLE         FLGPTN_BIT(0)
//#define FLG_FILEIN_READBS       FLGPTN_BIT(1)                   ///< read bitstream

/**
    @name File Read Reserved Buffer Size

    File read reserved buffer size.
*/
//@{
#define FILEIN_BUFBLK_RESERVESIZE        0x200000         ///< to avoid next buffer corrupted: 2M bytes (max I-frame size)
//@}

/**
    @name File Read Max Block Size
*/
//@{
#define NMP_FILEIN_BUFBLK_MAXSIZE            0x200000         ///< restrict max block size is 2MB
//@}

// NMediaPlay FileIn Read Block Information
typedef enum {
	FILEIN_BUFBLK_0 = 0,
	FILEIN_BUFBLK_1,
	FILEIN_BUFBLK_2,
	FILEIN_BUFBLK_3,
	FILEIN_BUFBLK_4,
	FILEIN_BUFBLK_5,
	FILEIN_BUFBLK_6,
	FILEIN_BUFBLK_CNT,
	ENUM_DUMMY4WORD(FILEIN_BUFBLK)
} FILEIN_BUFBLK;

// NMediaPlay FileIn Job Queue
typedef struct {
	UINT32                  front;                                  ///< front pointer
	UINT32                  rear;                                   ///< rear pointer
	BOOL                    bfull;                                  ///< full flag
	UINT32                  queue[FILEIN_JOBQ_MAX];
} FILEIN_JOBQ, *PFILEIN_JOBQ;

//  NMediaPlay FileIn Object
typedef struct {
    FST_FILE                file_handle;                            ///< file handle
    UINT64                  file_ttlsize;                           ///< file total size
    UINT32                  file_ttldur;                            ///< file total duration (second)
    UINT32                  file_fmt;                               ///< file format
    UINT32                  vdo_fr;                                 ///< video frame rate
    FILEIN_MEM_RANGE        mem;                                    ///< memory range
    CONTAINERPARSER         *file_container;                        ///< file container
    UINT32                  aud_fr;                                 ///< audio frame rate
    UINT32                  vdo_ttfrm;                              ///< video total frame
    UINT32                  aud_ttfrm;                              ///< audio total frame
} FILEIN_OBJ, *PFILEIN_OBJ;

// NMediaPlay FileIn Read Block Addr and Offset
typedef struct {
	UINT32                  addr_in_mem;                            ///< buffer block address in memory
	UINT64                  file_offset;                            ///< file offset
} FILEIN_READBLK, *PFILEIN_READBLK;

//  NMediaPlay FileIn Buffer Info
typedef struct {
	UINT32                  start_addr;                             ///< start address
	UINT32                  end_addr;                               ///< end address
	UINT32                  blk_size;                               ///< buffer size per unit time
	UINT32                  next_blk_idx;                           ///< next read block index
	UINT64                  next_file_pos;                          ///< next read file position
	UINT64                  read_accum_offset;                      ///< accumulation of read file data
	UINT64                  demux_offset;                           ///< current bs demux offset
	MEM_RANGE               vdodec_bs;                              ///< current video decode bs addr & size
	FILEIN_READBLK      ReadBlk[FILEIN_BUFBLK_CNT];         ///< file read block
	BOOL                    read_finish;                            ///< read file finished
} FILEIN_FILEREAD_INFO, *PFILEIN_FILEREAD_INFO;

typedef struct {
	UINT32                  start_addr;                             ///< start address
	UINT32                  blk_size;                               ///< buffer size per unit time
	UINT32                  tt_blknum;                              ///< total block number
} FILEIN_BLK_INFO, *PFILEIN_BLK_INFO;

/*extern void     FileIn_InstallID(void) _SECTION(".kercfg_text");
extern UINT32 _SECTION(".kercfg_data") FILEIN_TSK_ID;
extern UINT32 _SECTION(".kercfg_data") FILEIN_FLG_ID;
extern UINT32 _SECTION(".kercfg_data") FILEIN_SEM_ID[];*/

typedef void (FileInEventCb)(CHAR *Name, UINT32 event_id, UINT32 value);

#endif
