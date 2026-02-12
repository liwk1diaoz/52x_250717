/**
    @brief 		Header file of bsmux library.

    @file 		bsmux_cb.h

    @ingroup 	mBsMux

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _BSMUX_CB_H
#define _BSMUX_CB_H

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
extern void bsmux_cb_register(BSMUX_EVENT_CB new_cb);
extern ER bsmux_cb_buf(BSMUXER_FILE_BUF *p_buf, UINT32 id);
extern ER bsmux_cb_event(BSMUXER_CBEVENT event, void *p_data, UINT32 id);
extern ER bsmux_cb_result(BSMUXER_RESULT result, UINT32 id);
extern ER bsmux_cb_dbg(UINT32 value);

#endif //_BSMUX_CB_H
