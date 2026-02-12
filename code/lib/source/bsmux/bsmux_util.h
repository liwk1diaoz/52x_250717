/**
    @brief 		Header file of fileout library.

    @file 		bsmux_util.h

    @ingroup 	mBsMux

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _BSMUX_UTIL_H
#define _BSMUX_UTIL_H

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
//>> common util mapping
//1. bsmux_util_get_mid   (makerobj)=> NMR_MakerObj_GetMakerid
//2. bsmux_util_set_minfo (makerobj)=> NMediaRecSetInfo & NMR_MakerObj_SetInfo
//3. bsmux_util_get_minfo (makerobj)=> NMediaRecGetInfo & NMR_MakerObj_GetInfo
//4. bsmux_util_set_param			=> NMI_BsMuxer_SetParam
//5. bsmux_util_get_param			=> NMI_BsMuxer_GetParam, NMR_BsMuxer_get_info
//9. bsmux_util_wait_ready			=> NMR_BsMuxer_WaitFSrelease, NMR_BsMuxer_MP4_WaitFSrelease & NMediaRecTS_WaitFSrelease
//10.bsmux_util_prepare_buf
//11.bsmux_util_mem_dbg
//a. bsmux_util_put_version 		=> NMR_BsMuxer_PutVersionString
//b. bsmux_util_get_version
//c. bsmux_util_show_setting		=> NMR_BsMuxer_ShowSetting
//d. bsmux_util_set_result			=> NMR_BsMuxer_PauseAudioAndStop
//e. bsmux_util_chk_frm_sync
//A. bsmux_util_is_invalid_id
//B. bsmux_util_is_null_obj
//C. bsmux_util_is_not_normal
//D. bsmux_util_set_writeblock_size
//E. bsmux_util_get_buf_pa
//F. bsmux_util_get_buf_va
//G. bsmux_util_calc_align			=> NMR_BsMuxer_CalcAlign & NMediaRecTS_CalcAlign
//H. bsmux_util_calc_sec			=> NMediaRec_CalcSecBySetting & NMediaRec_CalcSecBySettingPath
//J. bsmux_util_memcpy
//>> plugin util mapping
//0. bsmux_util_plugin	  (makerobj)=> include NMR_MakerObj_Create & NMR_MakerObjUti_InitMov & NMediaRec_GetFSType
//1. bsmux_util_tskobj_init
//2. bsmux_util_check_tag			=> mdat check startcode & NMediaRecTS_CheckTag
//3. bsmux_util_engine_open
//4. bsmux_util_engine_close
//5. bsmux_util_engcpy
//6. bsmux_util_plugin_get_size
//7. bsmux_util_plugin_set_size
//8. bsmux_util_plugin_clean
//9. bsmux_util_update_vidinfo
//10.bsmux_util_release_buf			=> NMR_BsMuxer_ReleaseBuf & NMR_BsMuxer_MP4_ReleaseBuf / NMediaRecTS_ReleaseBuf
//11.bsmux_util_add_gps				=> NMR_BsMuxer_PadGps2MdatbyFskID
//A. bsmux_util_save_entry			=> NMR_BsMuxer_SaveEntry_Vid & NMR_BsMuxer_SaveEntry_Aud & NMR_BsMuxer_SaveEntry_GPS
//B. bsmux_util_nidx_pad			=> NMR_BsMuxer_PadNidx2MdatbyFskID & NMR_BsMuxer_SetNidxInfo
//C. bsmux_util_make_header 		=> NMediaRec_MakeHeader & NMR_MakerObj_MakeHeader
//D. bsmux_util_update_header		=> NMR_BsMuxer_MovUpdateHeader & NMR_MakerObj_UpdateHeader

/* =========================================================================== */
/* =                           Common Utility API                            = */
/* =========================================================================== */
extern ER bsmux_util_get_mid(UINT32 id, UINT32 *makerId);
extern ER bsmux_util_set_minfo(UINT32 type, UINT32 p1, UINT32 p2, UINT32 p3);
extern ER bsmux_util_get_minfo(UINT32 id, UINT32 type, UINT32 *p1, UINT32 *p2, UINT32 *p3);
extern ER bsmux_util_set_param(UINT32 id, UINT32 param, UINT32 value);
extern ER bsmux_util_get_param(UINT32 id, UINT32 param, UINT32 *p_value);
extern ER bsmux_util_wait_ready(UINT32 id);
extern ER bsmux_util_prepare_buf(UINT32 id, UINT32 type, UINT32 addr, UINT64 size, UINT64 pos);
extern ER bsmux_util_mem_dbg(UINT32 value);
extern ER bsmux_util_strgbuf_init(UINT32 id);
extern ER bsmux_util_strgbuf_handle_buffer(UINT32 id, void *p_bsq);
extern ER bsmux_util_strgbuf_handle_space(UINT32 id, void *p_buf);
extern ER bsmux_util_strgbuf_handle_header(UINT32 id);
extern VOID bsmux_util_put_version(UINT32 date, UINT32 ver1, UINT32 ver2);
extern VOID bsmux_util_get_version(UINT32 *p_version);
extern VOID bsmux_util_show_setting(UINT32 id);
extern VOID bsmux_util_set_result(UINT32 result);
extern VOID bsmux_util_chk_frm_sync(UINT32 id, void *p_data);
extern VOID bsmux_util_dump_info(UINT32 id);
extern VOID bsmux_util_dump_buffer(char *pBuf, UINT32 length);
extern UINT32 bsmux_util_is_invalid_id(UINT32 id);
extern UINT32 bsmux_util_is_null_obj(void *p_obj);
extern UINT32 bsmux_util_is_not_normal(void);
extern UINT32 bsmux_util_set_writeblock_size(UINT32 id);
extern UINT32 bsmux_util_get_buf_pa(UINT32 id, UINT32 va);
extern UINT32 bsmux_util_get_buf_va(UINT32 id, UINT32 pa);
extern UINT32 bsmux_util_calc_align(UINT32 input, UINT32 alignsize, UINT32 type);
extern UINT32 bsmux_util_calc_sec(BSMUX_CALC_SEC *p_setting);
extern UINT32 bsmux_util_calc_entry_per_sec(UINT32 id);
extern UINT32 bsmux_util_calc_frm_num(UINT32 id, UINT32 type);
extern UINT32 bsmux_util_pset_padding_size(UINT32 id, UINT32 size); //fast put flow ver3
extern UINT32 bsmux_util_calc_padding_size(UINT32 id); //fast put flow ver3
extern UINT32 bsmux_util_calc_reserved_size(UINT32 id);
extern UINT32 bsmux_util_memcpy(VOID *uiDst, VOID *uiSrc, UINT32 uiSize, UINT32 method);
extern UINT32 bsmux_util_muxalign(UINT32 id, UINT32 src);
extern UINT64 bsmux_util_calc_timestamp(UINT32 id, UINT64 time_start);
extern BOOL bsmux_util_check_tag(UINT32 id, void *p_bsq);
extern BOOL bsmux_util_check_firstI(UINT32 id, void *p_bsq);
extern BOOL bsmux_util_check_frmidx(UINT32 id, void *p_bsq);
extern BOOL bsmux_util_check_bufuse(UINT32 id, void *p_bsq);
extern BOOL bsmux_util_check_duration(UINT32 id, void *p_bsq);
extern BOOL bsmux_util_check_buflocksts(UINT32 id, void *p_bsq);
extern BOOL bsmux_util_check_buflockdur(UINT32 id, void *p_bsq);
/* =========================================================================== */
/* =                           Plugin Utility API                            = */
/* =========================================================================== */
extern ER bsmux_util_plugin(UINT32 id);
//common (15)
extern ER bsmux_util_plugin_dbg(UINT32 value);
extern ER bsmux_util_tskobj_init(UINT32 id, UINT32 *action);
extern ER bsmux_util_engine_open(void);
extern ER bsmux_util_engine_close(void);
extern UINT32 bsmux_util_engcpy(VOID *uiDst, VOID *uiSrc, UINT32 uiSize, UINT32 method);
extern ER bsmux_util_plugin_get_size(UINT32 id, UINT32 *p_size);
extern ER bsmux_util_plugin_set_size(UINT32 id, UINT32 addr, UINT32 *p_size);
extern ER bsmux_util_plugin_clean(UINT32 id);
extern ER bsmux_util_update_vidinfo(UINT32 id, UINT32 addr, void *p_bsq);
extern ER bsmux_util_release_buf(BSMUXER_FILE_BUF *p_buf);
extern ER bsmux_util_add_gps(UINT32 id, void *p_bsq);
extern ER bsmux_util_add_thumb(UINT32 id, void *p_bsq);
extern ER bsmux_util_add_lasting(UINT32 id);
extern ER bsmux_util_put_lasting(UINT32 id);
extern ER bsmux_util_add_meta(UINT32 id, void *p_bsq);
//sepcific (7)
extern ER bsmux_util_save_entry(UINT32 id, void *p_bsq);
extern ER bsmux_util_nidx_pad(UINT32 id);
extern ER bsmux_util_make_header(UINT32 id);
extern ER bsmux_util_update_header(UINT32 id); //@note BSM:updateHeader status Waiting_Hit, donothing
extern ER bsmux_util_make_pes(UINT32 id, void *p_bsq);
extern ER bsmux_util_make_pat(UINT32 id);
extern ER bsmux_util_make_moov(UINT32 id, UINT32 minus1sec);

#endif //_BSMUX_UTIL_H
