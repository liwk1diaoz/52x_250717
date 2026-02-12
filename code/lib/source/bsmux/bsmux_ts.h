/**
    @brief 		Header file of fileout library.

    @file 		bsmux_ts.h

    @ingroup 	mBsMux

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _BSMUX_TS_H
#define _BSMUX_TS_H

/*-----------------------------------------------------------------------------*/
/* Specific  Functions                                                         */
/*-----------------------------------------------------------------------------*/
//.. bsmux_ts_util_make_pmt			=> NMediaRecTS_MakePMT
//.. bsmux_ts_util_make_pcr			=> NMediaRecTS_MakePCR
//.. bsmux_ts_add_thumb				=> NMediaRecTS_AddThumb
//.. bsmux_ts_util_get_pts			=> NMediaRecTS_GetPts
extern ER bsmux_ts_make_pmt(UINT32 id, UINT32 pmtAddr);
extern ER bsmux_ts_make_pcr(UINT32 id, UINT32 pcrAddr, UINT64 pcrValue);
extern UINT64 bsmux_ts_get_pts(UINT32 pesHeaderLen, UINT32 ptsLen, UINT32 bsAddr);

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
//common (15)
//0. bsmux_ts_dbg
//1. bsmux_ts_tskobj_init
//3. bsmux_ts_open
//4. bsmux_ts_close
//5. bsmux_ts_mux
//6. bsmux_ts_get_need_size
//7. bsmux_ts_set_need_size
//8. bsmux_ts_clean
//9. bsmux_ts_update_vidinfo
//10.bsmux_ts_release_buf
//11.bsmux_ts_add_gps_data			=> NMediaRecTS_AddGPSData
//12.bsmux_ts_add_last
//13.bsmux_ts_put_last
//sepcific (6)
//A. bsmux_ts_save_entry
//B. bsmux_ts_nidx_pad
//C. bsmux_ts_make_header
//D. bsmux_ts_update_header
//E. bsmux_ts_make_pes				=> NMediaRecTS_MakePesHeader
//F. bsmux_ts_make_pat				=> NMediaRecTS_MakePAT
extern ER bsmux_ts_dbg(UINT32 value);
extern ER bsmux_ts_tskobj_init(UINT32 id, UINT32 *action);
extern ER bsmux_ts_open(void);
extern ER bsmux_ts_close(void);
extern UINT32 bsmux_ts_mux(VOID *uiDst, VOID *uiSrc, UINT32 uiSize, UINT32 method);
extern ER bsmux_ts_get_need_size(UINT32 id, UINT32 *p_size);
extern ER bsmux_ts_set_need_size(UINT32 id, UINT32 addr, UINT32 *p_size);
extern ER bsmux_ts_clean(UINT32 id);
extern ER bsmux_ts_update_vidinfo(UINT32 id, UINT32 addr, void *p_bsq);
extern ER bsmux_ts_release_buf(void *pBuf);
extern ER bsmux_ts_add_gps_data(UINT32 id, void *p_bsq);
extern ER bsmux_ts_add_thumb(UINT32 id, void *p_bsq);
extern ER bsmux_ts_add_last(UINT32 id);
extern ER bsmux_ts_put_last(UINT32 id);
extern ER bsmux_ts_add_meta(UINT32 id, void *p_bsq);
extern ER bsmux_ts_save_entry(UINT32 id, void *p_bsq);
extern ER bsmux_ts_nidx_pad(UINT32 id, UINT32 nowVfn, UINT32 nowAfn);
extern ER bsmux_ts_make_header(UINT32 id, void *p_maker);
extern ER bsmux_ts_update_header(UINT32 id, void *p_maker);
extern ER bsmux_ts_make_pes(UINT32 id, void *p_src);
extern ER bsmux_ts_make_pat(UINT32 id, UINT32 patAddr);

#endif //_BSMUX_TS_H
