/**
    @brief 		Header file of fileout library.

    @file 		fileout_port.h

    @ingroup 	mFileOut

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _FILEOUT_PORT_H
#define _FILEOUT_PORT_H

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
#include "FileSysTsk.h"

/*-----------------------------------------------------------------------------*/
/* Macro Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define FILEOUT_PATH_LEN            128
#define FILEOUT_OP_BUF_NUM          64
#define FILEOUT_QUEUE_NUM           6   //max port queue number at the same time
//notice: max path num 16 / max active port num 6

/*-----------------------------------------------------------------------------*/
/* Type Definitions                                                            */
/*-----------------------------------------------------------------------------*/
typedef struct {
    ISF_FILEOUT_BUF op_buf[FILEOUT_OP_BUF_NUM];
	UINT32 is_active;   ///< if not active, all data are invalid
	UINT32 need_to_rel;
    INT32  port_idx;    ///< port_idx 0 ~ port_idx 15
    UINT32 queue_head;  ///< queue head idx, the last added position, updated by fileout_addqueue()
    UINT32 queue_tail;  ///< queue tail idx, the last taken position, updated by fileout_dequeue()
	UINT64 size_total;  ///< updated by fileout_addqueue()
	UINT64 size_written; ///< updated by fileout_queue_written()
	UINT64 size_path; ///< cannot exceed FILEOUT_DEFAULT_MAX_FILE_SIZE
	CHAR *p_path;
	UINT32 path_size;
	FST_FILE filehdl;
} FILEOUT_QUEUE_INFO;

typedef struct {
	UINT32 buf_remain;
	UINT32 buf_pos;
    UINT64 size_remain;
	UINT64 size_total;
	UINT64 size_written;
} FILEOUT_QUEUE_SIZE_INFO;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT32 fileout_port_init(void);
INT32 fileout_port_start(INT32 port_idx);
INT32 fileout_port_close(INT32 port_idx);
INT32 fileout_port_get_queue(INT32 port_idx);//get a queue idx
INT32 fileout_port_cre_queue(INT32 port_idx);
INT32 fileout_port_rel_queue(INT32 port_idx);
INT32 fileout_port_is_queue_empty(void);
INT32 fileout_port_get_active(CHAR drive, UINT32 *port_num);
INT32 fileout_port_need_to_rel(CHAR drive);

INT32 fileout_queue_pick_idx(CHAR drive, INT32 idx);
INT32 fileout_queue_push(const ISF_FILEOUT_BUF *p_in_buf, INT32 queue_idx);
INT32 fileout_queue_pop(ISF_FILEOUT_BUF *p_ret_buf, INT32 queue_idx);
INT32 fileout_queue_written(INT32 queue_idx, UINT64 size);
INT32 fileout_queue_discard(INT32 queue_idx); //incomplete
FST_FILE fileout_queue_get_filehdl(INT32 queue_idx);
INT32 fileout_queue_set_filehdl(INT32 queue_idx, FST_FILE new_filehdl);
INT32 fileout_queue_get_port_idx(INT32 queue_idx);
INT32 fileout_queue_get_size_info(INT32 queue_idx, FILEOUT_QUEUE_SIZE_INFO *p_size_qinfo);
INT32 fileout_queue_get_buf_info(INT32 queue_idx, UINT32 buf_pos, ISF_FILEOUT_BUF *p_buf);
INT32 fileout_queue_get_remain_size(CHAR drive, UINT32 *p_size);
INT32 fileout_queue_set_max_pop_size(UINT64 size);
INT32 fileout_queue_get_max_pop_size(UINT64 *p_size);
INT32 fileout_queue_set_max_file_size(UINT64 size);
INT32 fileout_queue_get_max_file_size(UINT64 *p_size);

INT32 fileout_queue_set_need_size(INT32 queue_idx, UINT64 size);
INT32 fileout_queue_get_need_size(INT32 queue_idx, UINT64 *p_size);
#endif //_FILEOUT_PORT_H