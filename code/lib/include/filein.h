#ifndef _FILEIN_H
#define _FILEIN_H

#include "kwrap/debug.h"
#include "kwrap/error_no.h"

// for TS format
typedef enum {
	//FILEIN_TS_BLK_SIZE  = 0x200000, ///< one block size
	FILEIN_TS_BLK_SIZE  = 10000 * 188, ///< one block size
	FILEIN_TS_PL_BLKNUM = 3,        ///< preload block number
} FILEIN_TS_BUFINFO;

typedef enum {
	FILEIN_PARAM_FILEHDL,           ///< file handle
	FILEIN_PARAM_FILESIZE,          ///< total file size
	FILEIN_PARAM_FILEDUR,           ///< total file duration (sec)
	FILEIN_PARAM_FILEFMT,           ///< file format
	FILEIN_PARAM_BLK_TIME,          ///< play time per one block (e.g. 1 sec per block)
	FILEIN_PARAM_TT_BLKNUM,         ///< total block number
	FILEIN_PARAM_PL_BLKNUM,         ///< preload block number
	FILEIN_PARAM_RSV_BLKNUM,        ///< reserve block number (as buffer area)
	FILEIN_PARAM_NEED_MEMSIZE,      ///< get internal need mem size
	FILEIN_PARAM_BLK_INFO,          ///< file block starting addr
	FILEIN_PARAM_MEM_RANGE,         ///< allocated mem addr & size
	FILEIN_PARAM_CUR_VDOBS,         ///< current used video bs addr & size
	FILEIN_PARAM_EVENT_CB,          ///< callback event
	FILEIN_PARAM_CONTAINER,         ///< media container
	FILEIN_PARAM_VDO_FR,            ///< video frame rate (fps)
	FILEIN_PARAM_AUD_FR,            ///< audio frame rate (fps)
	FILEIN_PARAM_VDO_TTFRM,         ///< video total frame
	FILEIN_PARAM_AUD_TTFRM,         ///< audio total frame
	FILEIN_PARAM_USER_BLK_DIRECTLY, ///< user define filein block size directly
	FILEIN_PARAM_BLK_SIZE,          ///< size per one block (e.g. 1 sec per block)
} FILEIN_PARAM;

typedef enum {
	FILEIN_ACTION_PRELOAD,            ///< reload file block for initialization
	FILEIN_ACTION_MAX,
} FILEIN_ACTION;

typedef enum {
	FILEIN_STATE_PRELOAD,            ///< preload state
	FILEIN_STATE_NORMAL,             ///< normal state
} FILEIN_STATE;

typedef struct {
	UINT32 Addr;    ///< Memory buffer starting address
	UINT32 Size;    ///< Memory buffer size
} FILEIN_MEM_RANGE, *PFILEIN_MEM_RANGE;

typedef struct {
	UINT32 blk_time;                    ///< block time for one block (ms)
	UINT32 tt_blknum;                   ///< total block number
	UINT32 pl_blknum;                   ///< preload block number
	UINT32 rsv_blknum;                  ///< reserve block number (as buffer area)
	BOOL   user_define_blk;             ///< user define filein block size directly
	UINT32 blk_size;                    ///< size for one block
} FILEIN_USERBUF_INFO, *PFILEIN_USERBUF_INFO;

typedef enum {
	FILEIN_IN_TYPE_CHK_FILEBLK = 0,                 ///< checking is it the target bs in valid block range
	FILEIN_IN_TYPE_READ_ONEFRM,                     ///< read one frame only (for ff/fr, step play)
	FILEIN_IN_TYPE_RELOAD_BUF,                      ///< re-init buffer info and read one file block
	FILEIN_IN_TYPE_READ_ONEBLK,                     ///< read one block directly (for TS format)
	FILEIN_IN_TYPE_READ_TSBLK,						///< read FileReadInfo block 1 and block 2 directly (for TS format)
	FILEIN_IN_TYPE_CONFIG_BUFINFO,
	FILEIN_IN_TYPE_GET_BUFINFO,
	ENUM_DUMMY4WORD(FILEIN_IN_TYPE)
} FILEIN_IN_TYPE;

typedef enum {
	ISF_FILEIN_EVENT_MEM_RANGE = 0,                 ///< FileIn memory range
	ISF_FILEIN_EVENT_READ_ONE_BLK,
	ENUM_DUMMY4WORD(ISF_FILEIN_CB_EVENT)
} ISF_FILEIN_CB_EVENT;

typedef struct {
	UINT64 read_accum_offset;                        ///< accumulation of file reading data
	UINT64 last_used_offset;                         ///< accumulation of last used data
	UINT64 bs_offset;                                ///< target bs offset
	UINT32 bs_size;                                  ///< target bs size
	UINT32 buf_start_addr;                           ///< read file buffer start addr
	UINT32 buf_end_addr;                             ///< read file buffer end addr
	UINT64 blk_size;                                 ///< file block size
	UINT32 tt_blknum;                                ///< total file block number
	UINT32 uiThisAddr;
	UINT32 uiDemuxAddr;
	UINT64 uiNextFilePos;
	UINT32 event;                                    ///< read input event type (refer to FILEIN_IN_TYPE)
} FILEIN_CB_INFO, *PFILEIN_CB_INFO;

typedef struct {
	UINT32 addr;
	UINT32 size;
	UINT32 read_state;
} FILEIN_READBUF_INFO, *PFILEIN_READBUF_INFO;

extern ER FileIn_Open(UINT32 Id);
extern ER FileIn_Close(UINT32 Id);
extern ER FileIn_SetParam(UINT32 Id, UINT32 Param, UINT32 Value);
extern ER FileIn_GetParam(UINT32 Id, UINT32 Param, UINT32 *pValue);
extern ER FileIn_Action(UINT32 Id, UINT32 Action);
extern ER FileIn_In(UINT32 Id, UINT32 *pBuf);

#endif //_FILEIN_H

