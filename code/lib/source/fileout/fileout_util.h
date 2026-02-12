/**
    @brief 		Header file of fileout library.

    @file 		fileout_util.h

    @ingroup 	mFileOut

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _FILEOUT_UTIL_H
#define _FILEOUT_UTIL_H

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT32 fileout_util_init(void);
void fileout_util_time_delay_ms(void);
void fileout_util_time_mark(UINT32 *p_time);
void fileout_util_time_dur_ms(UINT32 old_time, UINT32 new_time, UINT32 *p_time);
INT32 fileout_util_is_slowcard(void);
INT32 fileout_util_is_fallocate(void);
INT32 fileout_util_is_formatfree(void);
INT32 fileout_util_is_wait_ready(void);
INT32 fileout_util_is_close_on_exec(void);
INT32 fileout_util_is_usememblk(void);
INT32 fileout_util_is_append(void);
UINT32 fileout_util_get_append_size(void);
INT32 fileout_util_is_skip_ops(INT32 queue_idx);
INT32 fileout_util_is_null_term(CHAR *p_path, UINT32 max_size);
void fileout_util_show_quesize(void);
void fileout_util_show_queinfo(INT32 queue_idx);
void fileout_util_show_fdbinfo(void);
void fileout_util_show_opsinfo(UINT32 dms, UINT32 type, ISF_FILEOUT_BUF *p_buf, INT32 qidx);
void fileout_util_show_bufinfo(const ISF_FILEOUT_BUF *p_buf);
void fileout_util_set_log(INT32 log_type, INT32 is_on);
void fileout_util_set_slowcard(INT32 dur_ms, INT32 is_on);
void fileout_util_set_fallocate(INT32 is_on);
void fileout_util_set_formatfree(INT32 is_on);
void fileout_util_set_wait_ready(INT32 is_on);
void fileout_util_set_close_on_exec(INT32 is_on);
void fileout_util_set_usememblk(INT32 is_on);
void fileout_util_set_append(INT32 is_on);
void fileout_util_set_append_size(UINT32 size);
void fileout_util_set_skip_ops(INT32 queue_idx, INT32 is_on);

INT32 fileout_util_cre_mem_blk(UINT32 blk_size);
INT32 fileout_util_rel_mem_blk(void);
INT32 fileout_util_get_blk_addr(void);
INT32 fileout_util_get_blk_size(void);
#endif //_FILEOUT_UTIL_H