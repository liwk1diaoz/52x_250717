/**
	IPL KDF Layer

    .

    @file       kdf_ipp_id_int.h
    @ingroup    mISYSAlg
    @note       Nothing (or anything need to be mentioned).

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _KDF_IPP_ID_INT_H_
#define _KDF_IPP_ID_INT_H_

#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/task.h"
#include "kdf_ipp_int.h"

/* define */
#define KDF_IPP_TSK_PRI           3
#define KDF_IPP_TSK_STKSIZE       4096

/**********************************************************/
/*              kdf_ipl task flag pattern	              */
/**********************************************************/
#define KDF_IPP_TASK_PAUSE             FLGPTN_BIT(0)
#define KDF_IPP_TASK_PAUSE_END         FLGPTN_BIT(1)
#define KDF_IPP_TASK_RESUME            FLGPTN_BIT(2)
#define KDF_IPP_TASK_RESUME_END        FLGPTN_BIT(3)
#define KDF_IPP_TASK_IDLE              FLGPTN_BIT(4)
#define KDF_IPP_TASK_RESTART           FLGPTN_BIT(5)
#define KDF_IPP_TASK_FLUSH             FLGPTN_BIT(6)
#define KDF_IPP_TASK_RES               FLGPTN_BIT(7)
#define KDF_IPP_TASK_CHK               FLGPTN_BIT(8)

#define KDF_IPP_TASK_TRIG			  FLGPTN_BIT(9)
#define KDF_IPP_TASK_TRIG_END		  FLGPTN_BIT(10)
#define KDF_IPP_TASK_TIMEOUT		  FLGPTN_BIT(11)

#define KDF_IPP_ENG_IDLE_OFFSET		  (12)
#define KDF_IPP_ENG_RHE_IDLE			  FLGPTN_BIT(12)
#define KDF_IPP_ENG_IFE_IDLE			  FLGPTN_BIT(13)
#define KDF_IPP_ENG_DCE_IDLE			  FLGPTN_BIT(14)
#define KDF_IPP_ENG_IPE_IDLE			  FLGPTN_BIT(15)
#define KDF_IPP_ENG_IME_IDLE			  FLGPTN_BIT(16)
#define KDF_IPP_ENG_IFE2_IDLE		  FLGPTN_BIT(17)
#define KDF_IPP_ENG_TIMEOUT			  FLGPTN_BIT(18)

#define KDF_IPP_QUEUE_PROC			  FLGPTN_BIT(19)

#define KDF_IPP_DIR_PAUSE_END		  FLGPTN_BIT(20)
#define KDF_IPP_DIR_TIMEOUT		  	  FLGPTN_BIT(21)

#define KDF_IPP_TASK_EXIT_END         FLGPTN_BIT(22)

#define KDF_IPP_PROC_INIT       (KDF_IPP_TASK_PAUSE_END|KDF_IPP_TASK_TRIG_END| \
								KDF_IPP_ENG_RHE_IDLE|KDF_IPP_ENG_IFE_IDLE|KDF_IPP_ENG_DCE_IDLE| \
								KDF_IPP_ENG_IPE_IDLE|KDF_IPP_ENG_IME_IDLE|KDF_IPP_ENG_IFE2_IDLE)

/**********************************************************/
/*              kdf_ipl hadnle flag pattern	              */
/**********************************************************/
#define KDF_IPP_HANDLE_POOL_LOCK	 FLGPTN_BIT(0)
#define KDF_IPP_HANDLE_LOCK_TIMEOUT	 FLGPTN_BIT(31)

/*************************************************/
extern THREAD_DECLARE(kdf_ipp_tsk, p1);
extern THREAD_HANDLE g_kdf_ipp_tsk_id;
extern ID g_kdf_ipp_flg_id;
extern void kdf_ipp_install_id(void);
extern void kdf_ipp_uninstall_id(void);



#endif //_KDF_IPP_ID_INT_H_

