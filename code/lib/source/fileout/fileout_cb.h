/**
    @brief 		Header file of fileout library.

    @file 		fileout_cb.h

    @ingroup 	mFileOut

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _FILEOUT_CB_H
#define _FILEOUT_CB_H

/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
INT32 fileout_cb_init(void);
void fileout_cb_register(FILEOUT_EVENT_CB new_cb);
INT32 fileout_cb_store_path(ISF_FILEOUT_BUF *p_buf, const CHAR *p_req_path);

INT32 fileout_cb_call_naming(INT32 queue_idx, ISF_FILEOUT_BUF *p_buf);
INT32 fileout_cb_call_opened(INT32 queue_idx, ISF_FILEOUT_BUF *p_buf);
INT32 fileout_cb_call_closed(INT32 queue_idx, ISF_FILEOUT_BUF *p_buf);
INT32 fileout_cb_call_delete(INT32 queue_idx, ISF_FILEOUT_BUF *p_buf);
INT32 fileout_cb_call_fs_err(INT32 queue_idx, ISF_FILEOUT_BUF *p_buf, INT32 err_code);
#endif //_FILEOUT_CB_H