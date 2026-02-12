/*
    Copyright   Novatek Microelectronics Corp. 2005~2018.  All rights reserved.

    @file       hd_filein_lib.h
    @ingroup    mFileIn

    @note       Nothing.

    @date       2019/07/17
*/

#ifndef HD_FILEIN_LIB_H
#define HD_FILEIN_LIB_H

#include "kwrap/debug.h"
#include "kwrap/error_no.h"
#include "kflow_common/isf_flow_def.h"
#include "hd_common.h"

#define ISF_FILEIN_IN_NUM 2
#define ISF_FILEIN_OUT_NUM 2

typedef enum {
	HD_FILEIN_PARAM_FILEHDL,           ///< file handle
	HD_FILEIN_PARAM_FILESIZE,          ///< total file size
	HD_FILEIN_PARAM_FILEDUR,           ///< total file duration (sec)
	HD_FILEIN_PARAM_FILEFMT,           ///< file format
	HD_FILEIN_PARAM_BLK_TIME,          ///< play time per one block (e.g. 1 sec per block)
	HD_FILEIN_PARAM_TT_BLKNUM,         ///< total block number
	HD_FILEIN_PARAM_PL_BLKNUM,         ///< preload block number
	HD_FILEIN_PARAM_RSV_BLKNUM,        ///< reserve block number (as buffer area)
	HD_FILEIN_PARAM_NEED_MEMSIZE,      ///< get internal need mem size
	HD_FILEIN_PARAM_BLK_INFO,          ///< file block starting addr
	HD_FILEIN_PARAM_MEM_RANGE,         ///< allocated mem addr & size
	HD_FILEIN_PARAM_CUR_VDOBS,         ///< current used video bs addr & size
	HD_FILEIN_PARAM_EVENT_CB,          ///< callback event
	HD_FILEIN_PARAM_CONTAINER,         ///< media container
	HD_FILEIN_PARAM_VDO_FR,            ///< video frame rate (fps)
	HD_FILEIN_PARAM_AUD_FR,            ///< audio frame rate (fps)
	HD_FILEIN_PARAM_VDO_TTFRM,         ///< video total frame
	HD_FILEIN_PARAM_AUD_TTFRM,         ///< audio total frame
	HD_FILEIN_PARAM_USER_BLK_DIRECTLY, ///< user define filein block size directly
	HD_FILEIN_PARAM_BLK_SIZE,          ///< size per block
} HD_FILEIN_PARAM;

//  NMediaPlay FileIn Buffer Info
typedef struct {
	UINT32                  start_addr;                             ///< start address
	UINT32                  blk_size;                               ///< buffer size per unit time
	UINT32                  tt_blknum;
} HD_FILEIN_BLKINFO, *PHD_FILEIN_BLKINFO;

typedef struct {
	UINT32 addr;
	UINT32 size;
	UINT32 read_state;
} HD_FILEIN_READBUF_INFO, *PHD_FILEIN_READBUF_INFO;

/********************************************************************
    EXTERN VARIABLES & FUNCTION PROTOTYPES DECLARATIONS
********************************************************************/
HD_RESULT hd_filein_open(HD_PATH_ID path_id);
HD_RESULT hd_filein_start(HD_PATH_ID path_id);
HD_RESULT hd_filein_stop(HD_PATH_ID path_id);
HD_RESULT hd_filein_close(HD_PATH_ID path_id);
HD_RESULT hd_filein_get(HD_PATH_ID path_id, UINT32 param, UINT32 *value);
HD_RESULT hd_filein_set(HD_PATH_ID path_id, UINT32 param, UINT32 value);
HD_RESULT hd_filein_push_in_buf(HD_PATH_ID path_id, UINT32 *pBuf, INT32 wait_ms);
HD_RESULT hd_filein_pull_out_buf(HD_PATH_ID path_id, UINT32 *pBuf, INT32 wait_ms);
HD_RESULT hd_filein_uninit(VOID);

#endif //HD_FILEIN_LIB_H

