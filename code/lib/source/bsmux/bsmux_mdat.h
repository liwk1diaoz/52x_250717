/**
    @brief 		Header file of fileout library.

    @file 		bsmux_mdat.h

    @ingroup 	mBsMux

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _BSMUX_MDAT_H
#define _BSMUX_MDAT_H

/*-----------------------------------------------------------------------------*/
/* Specific  Functions                                                         */
/*-----------------------------------------------------------------------------*/
//1. bsmux_util_set_hdr_mem			=> NMR_Mdat_SetHeaderMemory
//2. bsmux_util_get_hdr_mem			=> NMR_Mdat_GetHeaderMemory
//3. bsmux_util_rel_hdr_mem			=> NMR_Mdat_ReleaseHeaderMemory
//4. bsmux_mdat_get_hdr_count
//6. bsmux_mdat_set_hdr_count3
//7. bsmux_mdat_get_max_entrytime
//8. bsmux_util_set_max_entrytime	=> NMR_Mdat_SetMaxEntryTime
//9. bsmux_mdat_nidx_make			=> NMR_BsMuxer_MakeNidx2Mdat
//10.bsmux_mdat_gps_make			=> NMR_BsMuxer_MakeGps2Mdat
//11.bsmux_mdat_calc_header_size	=> NMR_BsMuxer_CalcHeaderSize
extern ER     bsmux_mdat_set_hdr_mem(UINT32 type, UINT32 id, UINT32 addr, UINT32 size);
extern UINT32 bsmux_mdat_get_hdr_mem(UINT32 type, UINT32 id);
extern ER     bsmux_mdat_rel_hdr_mem(UINT32 type, UINT32 id, UINT32 addr);
extern BOOL   bsmux_mdat_chk_hdr_mem(UINT32 type, UINT32 id);
extern UINT32 bsmux_mdat_set_hdr_count(UINT32 value);
extern UINT32 bsmux_mdat_get_hdr_count(void);
extern ER     bsmux_mdat_set_max_entrytime(UINT32 id, UINT32 idx_size, UINT32 vfr);
extern ER     bsmux_mdat_get_max_entrytime(UINT32 id, UINT32 *value);
extern ER     bsmux_mdat_nidx_make(UINT32 id, UINT64 offset, UINT32 nidxid);
extern ER     bsmux_mdat_gps_make(UINT32 id, UINT64 offset);
extern UINT32 bsmux_mdat_calc_header_size(UINT32 id);

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
//common (15)
//0. bsmux_mdat_dbg
//1. bsmux_mdat_tskobj_init			=> NMR_BsMuxer_InitMdatobj
//3. bsmux_mdat_open
//4. bsmux_mdat_close
//5. bsmux_mdat_memcpy
//6. bsmux_mdat_get_need_size
//7. bsmux_mdat_set_need_size
//8. bsmux_mdat_clean
//9. bsmux_util_update_vidsize		=> NMR_BsMuxer_UpdateVidSize, NMR_BsMuxer_Update_h265VidSize
//10.bsmux_mdat_release_buf
//11.bsmux_mdat_gps_pad				=> NMR_BsMuxer_PadGps2MdatbyFskID
//12.bsmux_mdat_add_last
//13.bsmux_mdat_put_last
//sepcific (7)
//A. bsmux_mdat_save_entry			=> NMR_BsMuxer_SaveEntry_Vid & NMR_BsMuxer_SaveEntry_Aud & NMR_BsMuxer_SaveEntry_GPS
//B. bsmux_mdat_nidx_pad			=> NMR_BsMuxer_PadNidx2MdatbyFskID & NMR_BsMuxer_SetNidxInfo
//C. bsmux_mdat_make_header
//D. bsmux_mdat_update_header
extern ER bsmux_mdat_dbg(UINT32 value);
extern ER bsmux_mdat_tskobj_init(UINT32 id, UINT32 *action);
extern ER bsmux_mdat_open(void);
extern ER bsmux_mdat_close(void);
extern UINT32 bsmux_mdat_memcpy(VOID *uiDst, VOID *uiSrc, UINT32 uiSize, UINT32 method);
extern ER bsmux_mdat_get_need_size(UINT32 id, UINT32 *p_size);
extern ER bsmux_mdat_set_need_size(UINT32 id, UINT32 addr, UINT32 *p_size);
extern ER bsmux_mdat_clean(UINT32 id);
extern ER bsmux_mdat_update_vidsize(UINT32 id, UINT32 addr, void *p_bsq);
extern ER bsmux_mdat_release_buf(void *pBuf);
extern ER bsmux_mdat_gps_pad(UINT32 id, void *p_bsq);
extern ER bsmux_mdat_add_thumb(UINT32 id, void *p_bsq);
extern ER bsmux_mdat_add_last(UINT32 id);
extern ER bsmux_mdat_put_last(UINT32 id);
extern ER bsmux_mdat_add_meta(UINT32 id, void *p_bsq);
extern ER bsmux_mdat_save_entry(UINT32 id, void *p_bsq);
extern ER bsmux_mdat_nidx_pad(UINT32 id);
extern ER bsmux_mdat_make_header(UINT32 id, void *p_maker);
extern ER bsmux_mdat_update_header(UINT32 id, void *p_maker);
extern ER bsmux_mdat_make_front_moov(UINT32 id, void *p_maker, UINT32 minus1sec);

#endif //_BSMUX_MDAT_H
