/**
    @brief 		Header file of fileout library.

    @file 		bsmux_ctrl.h

    @ingroup 	mBsMux

    @note		Nothing.

    Copyright Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _BSMUX_CTRL_H
#define _BSMUX_CTRL_H

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
//>> mem ctrl mapping (use hd common mem)
//1. bsmux_ctrl_mem_alloc 			=> _ISF_BsMux_AllocMem
//2. bsmux_ctrl_mem_free			=> _ISF_BsMux_FreeMem
//3. bsmux_ctrl_mem_init			=> NMR_BsMuxer_InitQ
//4. bsmux_ctrl_mem_set_range 		=> _NMR_BsMuxer_SetSaveBuf
//5. bsmux_ctrl_mem_set_buffer		=> NMR_BsMuxer_SetBuffer (copybuf), NMediaRecTS_SetBuffer
//6. bsmux_ctrl_mem_get_bufinfo		=> NMR_BsMuxer_GetReal2Card, NMR_BsMuxer_GetTotal2Card
//7. bsmux_ctrl_mem_set_bufinfo 	=> NMR_BsMuxer_AddReal2Card
//9. bsmux_ctrl_mem_dbg
//>> saveq ctrl mapping (bsq)
//1. bsmux_ctrl_bs_getnum 			=> NMR_BsMuxer_HowManySavefileQueue & NMediaRecTS_HowManyBSinQueue
//2. bsmux_ctrl_bs_initqueue		=> NMR_BsMuxer_InitQ
//3. bsmux_ctrl_bs_enqueue			=> NMR_BsMuxer_AddQWithFstskid, NMediaRecTS_AddPesQ
//4. bsmux_ctrl_bs_dequeue			=> NMR_BsMuxer_GetQ & NMediaRecTS_GetQ
//5. bsmux_ctrl_bs_rollback			=> NMR_BsMuxer_RollbackOneSec & NMediaRecTS_RollbackPesQ
//6. bsmux_ctrl_bs_copy2buffer		=> NMR_BsMuxer_Copy2BSBuffer (copybuf)
//7. bsmux_ctrl_bs_putall2card		=> NMR_BsMuxer_PutAllBSQ2Card (copybuf)
//8. bsmux_ctrl_bs_init_fileinfo	=> NMR_BsMuxer_InitQ
//9. bsmux_ctrl_bs_init_iframeinfo
//10.bsmux_ctrl_bs_get_fileinfo		=> NMR_BsMuxer_GetNowFrames
//11.bsmux_ctrl_bs_set_fileinfo
//12.bsmux_ctrl_bs_add_lastI		=> NMediaRecTS_Add_LastI
//13.bsmux_ctrl_bs_get_lastI		=> NMediaRecTS_Get_LastI
//14.bsmux_ctrl_bs_dbg
//>> main ctrl mapping (interface api)
//1. bsmux_ctrl_init
//2. bsmux_ctrl_open 				=> _ISF_BsMux_Open, NMI_BsMuxer_Open, NMR_BsMuxer_MdatOpen
//3. bsmux_ctrl_start				=> NMI_BsMuxer_Action
//4. bsmux_ctrl_stop 				=> _NMI_BsMuxer_Action & NMR_BsMuxer_Stop, NMediaRecTS_Stop
//5. bsmux_ctrl_close				=> NMR_BsMuxer_MdatClose
//6. bsmux_ctrl_in					=> NMI_BsMuxer_In & _NMR_BsMuxer_PutData
//7. bsmux_ctrl_trig_emr			=> NMediaRec_TrigEMR & NMediaRec_TrigEMRPause
//8. bsmux_ctrl_trig_cutnow			=> NMediaRec_SetCutNOW, NMR_BsMuxer_SetCutNOW, NMediaRecTS_SetCutNow

/* =========================================================================== */
/* =                           MEM Ctrl Interface                            = */
/* =========================================================================== */
extern ER bsmux_ctrl_mem_alloc(UINT32 id);
extern ER bsmux_ctrl_mem_free(UINT32 id);
extern ER bsmux_ctrl_mem_init(UINT32 id);
extern ER bsmux_ctrl_mem_cal_range(UINT32 id, UINT32 *p_size);
extern ER bsmux_ctrl_mem_set_range(UINT32 id, UINT32 value);
extern ER bsmux_ctrl_mem_det_range(UINT32 id);
extern ER bsmux_ctrl_mem_cal_buffer(UINT32 id, UINT32 *p_size);
extern ER bsmux_ctrl_mem_set_buffer(UINT32 id, void *p_buf);
extern ER bsmux_ctrl_mem_get_bufinfo(UINT32 id, UINT32 type, UINT32 *value);
extern ER bsmux_ctrl_mem_set_bufinfo(UINT32 id, UINT32 type, UINT32 value);
extern ER bsmux_ctrl_mem_dbg(UINT32 value);

/* =========================================================================== */
/* =                           BSQ Ctrl Interface                            = */
/* =========================================================================== */
extern UINT32 bsmux_ctrl_bs_getnum(UINT32 id, UINT32 lock);
extern UINT32 bsmux_ctrl_bs_active(UINT32 id, UINT32 lock);
extern ER bsmux_ctrl_bs_calcgopnum(UINT32 id, UINT32 *p_num, UINT32 *p_sec);
extern ER bsmux_ctrl_bs_calcqueue(UINT32 id, UINT32 *p_size);
extern ER bsmux_ctrl_bs_initqueue(UINT32 id, UINT32 addr, UINT32 size);
extern ER bsmux_ctrl_bs_enqueue(UINT32 id, void *p_bsq);
extern ER bsmux_ctrl_bs_dequeue(UINT32 id, void *p_bsq);
extern ER bsmux_ctrl_bs_predequeue(UINT32 id, void *p_bsq);
extern ER bsmux_ctrl_bs_rollback(UINT32 id, UINT32 sec, UINT32 check);
extern ER bsmux_ctrl_bs_reorder(UINT32 id, UINT32 sec);
extern ER bsmux_ctrl_bs_copy2strgbuf(UINT32 id, void *p_bsq);
extern ER bsmux_ctrl_bs_copy2buffer(UINT32 id, void *p_bsq);
extern ER bsmux_ctrl_bs_putall2card(UINT32 id);
extern ER bsmux_ctrl_bs_init_fileinfo(UINT32 id);
extern ER bsmux_ctrl_bs_get_fileinfo(UINT32 id, UINT32 type, UINT32 *value);
extern ER bsmux_ctrl_bs_set_fileinfo(UINT32 id, UINT32 type, UINT32 value);
extern ER bsmux_ctrl_bs_add_lastI(UINT32 id, UINT32 bsqnum);
extern UINT32 bsmux_ctrl_bs_get_lastI(UINT32 id, UINT32 backSec);
extern ER bsmux_ctrl_bs_dbg(UINT32 value);

/* =========================================================================== */
/* =                          Main Ctrl Interface                            = */
/* =========================================================================== */
extern ER bsmux_ctrl_init(void);
extern ER bsmux_ctrl_open(UINT32 id);
extern ER bsmux_ctrl_start(UINT32 id);
extern ER bsmux_ctrl_stop(UINT32 id);
extern ER bsmux_ctrl_close(UINT32 id);
extern ER bsmux_ctrl_uninit(void);
extern ER bsmux_ctrl_in(UINT32 id, void *p_data);
extern ER bsmux_ctrl_extend(UINT32 id);
extern ER bsmux_ctrl_pause(UINT32 id);
extern ER bsmux_ctrl_resume(UINT32 id);
extern ER bsmux_ctrl_emergency(UINT32 id);
extern ER bsmux_ctrl_trig_emr(UINT32 id, UINT32 pause_id);
extern ER bsmux_ctrl_trig_cutnow(UINT32 id);

#endif //_BSMUX_CTRL_H
