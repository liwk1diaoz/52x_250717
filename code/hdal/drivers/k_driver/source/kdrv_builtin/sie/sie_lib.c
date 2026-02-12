/*
    @file       sie_lib.c
    @ingroup    mIIPPSIE

    @brief      SIE integration control

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

// include files for FW
#include    "kwrap/type.h"	//header for basic variable type
#include    "sie_lib.h"		//header for SIE: external enum/struct/api/variable
#include    "sie_int.h"		//header for SIE: internal enum/struct/api/variable
#include    "sie_platform.h"
#ifdef __KERNEL__
#include    "sie_drv.h"
#endif
#include <kdrv_builtin/kdrv_builtin.h>
#include    "kwrap/perf.h"


#define DRV_SUPPORT_IST 0
#define Perf_GetCurrent(x) 0

#define SIE_CLK_API (0)
#define SIE_MFT_API (0) ///< SIE measure-frame-time API
#define SIE_FLIP_TEST (0) ///< This is for flip testing, only for emulation

#if (SIE_MFT_API==1)
static UINT32 uiMftTime[4], uiMftNote[4], uiMftCnt = 0;
#endif//(SIE_MFT_API==1)

static BOOL g_sie_act_en[SIE_ENGINE_ID_MAX] ={TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};
static BOOL sie_drv_first_start_skip[SIE_ENGINE_ID_MAX] ={TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE};
static ID g_SieLockStatus[SIE_ENGINE_ID_MAX] = {NO_TASK_LOCKED, NO_TASK_LOCKED, NO_TASK_LOCKED, NO_TASK_LOCKED, NO_TASK_LOCKED, NO_TASK_LOCKED, NO_TASK_LOCKED, NO_TASK_LOCKED};
static SIE_ISR_CB_TYPE pSieIntCbIdx[SIE_ENGINE_ID_MAX] = {SIE_ISR_CB_IMD, SIE_ISR_CB_IMD, SIE_ISR_CB_IMD, SIE_ISR_CB_IMD, SIE_ISR_CB_IMD, SIE_ISR_CB_IMD, SIE_ISR_CB_IMD, SIE_ISR_CB_IMD};
static BOOL g_use_vdlatch_cb[SIE_ENGINE_ID_MAX] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
static BOOL g_use_crp_end_sts[SIE_ENGINE_ID_MAX] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
static BOOL g_use_isr_sync_hdal[SIE_ENGINE_ID_MAX] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
static VOS_TICK bp1_stamp[SIE_ENGINE_ID_MAX] = {FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE}, bp2_stamp[SIE_ENGINE_ID_MAX]={FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE};
static UINT32 isr_cnt[SIE_ENGINE_ID_MAX];
VOS_TICK bp_time_diff[SIE_ENGINE_ID_MAX];

#define RGBIR_BINNUM 32
#define RGBIR_VALUPP 3840
#define RGBIR_VALLOW 80
#define RGBIR_A_LOWLIG_THR 224
#define RGBIR_IRDG_SUM_THR 128
static UINT32 rgb_ir_level_rst[SIE_ENGINE_ID_MAX] = {0};
static UINT32 ir_level_tk_last[SIE_ENGINE_ID_MAX] = {0}, ir_leve_lr_dg_last[SIE_ENGINE_ID_MAX] = {0};


#if (DRV_SUPPORT_IST == ENABLE)
#else
static void (*pfSie1IntCb)(UINT32 uiIntStatus, SIE_ENGINE_STATUS_INFO_CB *info);
static void (*pfSie2IntCb)(UINT32 uiIntStatus, SIE_ENGINE_STATUS_INFO_CB *info);
static void (*pfSie3IntCb)(UINT32 uiIntStatus, SIE_ENGINE_STATUS_INFO_CB *info);
static void (*pfSie4IntCb)(UINT32 uiIntStatus, SIE_ENGINE_STATUS_INFO_CB *info);
static void (*pfSie5IntCb)(UINT32 uiIntStatus, SIE_ENGINE_STATUS_INFO_CB *info);

static void (*pfSie1LatchIntCb)(UINT32 uiIntStatus, SIE_ENGINE_STATUS_INFO_CB *info);
static void (*pfSie2LatchIntCb)(UINT32 uiIntStatus, SIE_ENGINE_STATUS_INFO_CB *info);
static void (*pfSie3LatchIntCb)(UINT32 uiIntStatus, SIE_ENGINE_STATUS_INFO_CB *info);
static void (*pfSie4LatchIntCb)(UINT32 uiIntStatus, SIE_ENGINE_STATUS_INFO_CB *info);
static void (*pfSie5LatchIntCb)(UINT32 uiIntStatus, SIE_ENGINE_STATUS_INFO_CB *info);
#endif


//BCC verification only
//UINT32 g_uiSie1VdCnt = 0;
//UINT32 g_uiSie1Out0Cnt = 0;
//UINT32 g_uiSie1Out5Cnt = 0;
//UINT32 g_uiSie1BccOvflCnt = 0;
//UINT32 g_uiSie1ActStCnt = 0;

//BCC verification only
//UINT32 g_uiSie2VdCnt = 0;
//UINT32 g_uiSie2Out0Cnt = 0;
//UINT32 g_uiSie2Out5Cnt = 0;
//UINT32 g_uiSie2BccOvflCnt = 0;
//UINT32 g_uiSie2ActStCnt = 0;

//UINT32 g_uiSie3VdCnt = 0;
//UINT32 g_uiSie3Out0Cnt = 0;
//UINT32 g_uiSie3Out5Cnt = 0;
//UINT32 g_uiSie3BccOvflCnt = 0;
//UINT32 g_uiSie3ActStCnt = 0;

// BCC initial parameters
static SIE_BCC_PARAM_INFO  g_sieBccParamInfo = {0};
static UINT32 sie_lib_bp1[SIE_ENGINE_ID_MAX], sie_lib_bp2[SIE_ENGINE_ID_MAX];

/*
    SIE Lock Function

    Lock SIE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/

static ER sie_lock(SIE_ENGINE_ID eng_id)
{
	ER rt = E_OK;

	if (g_SieLockStatus[eng_id] == NO_TASK_LOCKED) {
		rt = sie_platform_sem_wait(eng_id);

		if (rt != E_OK) {
			return rt;
		}

		g_SieLockStatus[eng_id] = TASK_LOCKED;
		return rt;
	} else {
		return E_OBJ;
	}
}

/*
    SIE Unlock Function

    Unlock SIE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER sie_unlock(SIE_ENGINE_ID eng_id)
{
	if (g_SieLockStatus[eng_id] == TASK_LOCKED) {
		g_SieLockStatus[eng_id] = NO_TASK_LOCKED;
		return sie_platform_sem_signal(eng_id);
	} else {
		return E_OBJ;
	}
}

/*
    SIE Attach Function

    Pre-initialize driver required HW before driver open

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sie_attach(SIE_ENGINE_ID eng_id, BOOL do_enable)
{
	if (do_enable) {
		sie_platform_enable_clk(eng_id);
	}
	return E_OK;
}

/*
    SIE Detach Function

    De-initialize driver required HW after driver close

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sie_detach(SIE_ENGINE_ID eng_id, BOOL do_disable)
{
	if (do_disable) {
		sie_platform_disable_clk(eng_id);
		sie_platform_unprepare_clk(eng_id);
	}
	return E_OK;
}

/*
    SIE Interrupt Service Routine

    SIE interrupt service routine

    @return None.
*/
void sie_isr(SIE_ENGINE_ID eng_id)
{
	UINT32 inSieIntStatusRaw, inSieIntStatus, inSieIntEnable;
	UINT32 uiPtn = 0;
	SIE_ENGINE_STATUS_INFO_CB EngineStatus_cb = {0};

	inSieIntStatusRaw = sie_getIntrStatus(eng_id);
	inSieIntEnable = sie_getIntEnable(eng_id);
	inSieIntStatus = inSieIntStatusRaw & inSieIntEnable;
	if (inSieIntStatus == 0) {
		return;
	}
	sie_clrIntrStatus(eng_id, inSieIntStatus);
	uiPtn = 0;

	if (inSieIntStatus & SIE_INT_VD) {
		if((pSieIntCbIdx[eng_id] == SIE_ISR_CB_VDLATCH) && (g_use_vdlatch_cb[eng_id] == FALSE)) {
			g_use_vdlatch_cb[eng_id] = TRUE;
			//nvt_dbg(ERR, "g_use_vdlatch_cb == TRUE\r\n");
		}
		//nvt_dbg(IND, "VD\r\n");
		//g_uiSie1VdCnt++;

		uiPtn |= FLGPTN_SIE_FRAMEEND;
		//DBG_ERR("SIE_INT_VD == 1, uiPtn = %d\r\n",uiPtn);
		
		if (inSieIntStatusRaw & SIE_INT_DRAM_IN_OUT_ERR) {
			SIE_ENGINE_STATUS_INFO EngineStatus;
			sie_clrIntrStatus(eng_id, SIE_INT_DRAM_IN_OUT_ERR);
			sie_getEngineStatus(eng_id, &EngineStatus);
			sie_clrEngineStatus(eng_id, &EngineStatus);

			EngineStatus_cb.bDramIn1Udfl = EngineStatus.bDramIn1Udfl;
			EngineStatus_cb.bDramIn2Udfl = EngineStatus.bDramIn2Udfl;
			EngineStatus_cb.bDramOut0Ovfl= EngineStatus.bDramOut0Ovfl;	
			EngineStatus_cb.bDramOut1Ovfl= EngineStatus.bDramOut1Ovfl;	
			EngineStatus_cb.bDramOut2Ovfl= EngineStatus.bDramOut2Ovfl;	
			//nvt_dbg(ERR,"id %d, FE-CHK: SIE IO ERR:%d%d%d,%d%d%d%d%d%d!!\r\n", eng_id, EngineStatus.bBuffRheOvfl, EngineStatus.bDramIn1Udfl, EngineStatus.bDramIn2Udfl, EngineStatus.bDramOut0Ovfl, EngineStatus.bDramOut1Ovfl, EngineStatus.bDramOut2Ovfl, EngineStatus.bDramOut3Ovfl, EngineStatus.bDramOut4Ovfl, EngineStatus.bDramOut5Ovfl);
		}
	}
	if (inSieIntStatus & SIE_INT_MD_HIT) {
		sie_clrIntrStatus(eng_id, SIE_INT_MD_HIT);
		nvt_dbg(ERR,"id %d, MD HIT\r\n",eng_id);
		uiPtn |= FLGPTN_SIE_MD_HIT;
	}
	if (inSieIntStatus & SIE_INT_BP1) {
		uiPtn |= FLGPTN_SIE_BP1;
		if ((sie_lib_bp2[eng_id] > sie_lib_bp1[eng_id]) && (sie_lib_bp1[eng_id] > 0)) {
			vos_perf_mark(&bp1_stamp[eng_id]);
		}
	}
	if (inSieIntStatus & SIE_INT_BP2) {
		uiPtn |= FLGPTN_SIE_BP2;
		if ((sie_lib_bp2[eng_id] > sie_lib_bp1[eng_id]) && (sie_lib_bp1[eng_id] > 0)) {
			vos_perf_mark(&bp2_stamp[eng_id]);
			bp_time_diff[eng_id] = vos_perf_duration(bp1_stamp[eng_id], bp2_stamp[eng_id]);
			if (isr_cnt[eng_id] % 100 == 0) {
				nvt_dbg(ERR, "id %d, row time = %d ns \r\n", eng_id, bp_time_diff[eng_id] * 1000 /(sie_lib_bp2[eng_id] - sie_lib_bp1[eng_id]));
			}
			isr_cnt[eng_id]++;
		}
	}
	if (inSieIntStatus & SIE_INT_BP3) {
		uiPtn |= FLGPTN_SIE_BP3;
	}
	#if 0
	if (inSieIntStatus & SIE_INT_DRAM_IN_OUT_ERR) {
		SIE_ENGINE_STATUS_INFO EngineStatus;
		sie_getEngineStatus(eng_id, &EngineStatus);
		sie_clrEngineStatus(eng_id, &EngineStatus);
		nvt_dbg(ERR,"id %d, SIE IO ERR:%d%d%d,%d%d%d%d%d%d!!\r\n", eng_id, EngineStatus.bBuffRheOvfl, EngineStatus.bDramIn1Udfl, EngineStatus.bDramIn2Udfl, EngineStatus.bDramOut0Ovfl, EngineStatus.bDramOut1Ovfl, EngineStatus.bDramOut2Ovfl, EngineStatus.bDramOut3Ovfl, EngineStatus.bDramOut4Ovfl, EngineStatus.bDramOut5Ovfl);
	}
	#endif
	if (inSieIntStatus & SIE_INT_DPCF) {
		nvt_dbg(ERR,"id %d, DPCF !!\r\n",eng_id);
	}
	if (inSieIntStatus & SIE_INT_SIECLK_ERR) {
		nvt_dbg(ERR, "id %d, clk err, sie_clk < pxclk_clk !!\r\n", eng_id);
	}
	if (inSieIntStatus & SIE_INT_RAWENC_OUTOVFL) {
		//g_uiSie1BccOvflCnt++;
		nvt_dbg(ERR,"id %d, RAWENC_OUTOVFL !!\r\n",eng_id);
		//abort();
	}
	if (inSieIntStatus & SIE_INT_DRAM_OUT0_END) {
		//if (g_uiSie1VdCnt != 0) {
			//DBG_ERR("receive VD between active start and Dram out0 end!\r\n");
			//abort();
		//}
		//g_uiSie1Out0Cnt++;
		uiPtn |= FLGPTN_SIE_DO0_END;
		#if 0
		if (inSieIntStatusRaw & SIE_INT_DRAM_IN_OUT_ERR) {
			SIE_ENGINE_STATUS_INFO EngineStatus;
			sie_clrIntrStatus(eng_id, SIE_INT_DRAM_IN_OUT_ERR);
			sie_getEngineStatus(eng_id, &EngineStatus);
			sie_clrEngineStatus(eng_id, &EngineStatus);
			nvt_dbg(ERR,"id %d, O0E-CHK: SIE IO ERR:%d%d%d,%d%d%d%d%d!!\r\n", eng_id,EngineStatus.bBuffRheOvfl, EngineStatus.bDramIn1Udfl, EngineStatus.bDramIn2Udfl, EngineStatus.bDramOut0Ovfl, EngineStatus.bDramOut1Ovfl, EngineStatus.bDramOut2Ovfl, EngineStatus.bDramOut3Ovfl, EngineStatus.bDramOut4Ovfl);
		}
		if (inSieIntStatusRaw & SIE_INT_DPCF) {
			sie_clrIntrStatus(eng_id, SIE_INT_DPCF);
			nvt_dbg(ERR,"id %d, O0E-CHK: DPCF !!\r\n",eng_id);
		}
		if (inSieIntStatusRaw & SIE_INT_RAWENC_OUTOVFL) {
			sie_clrIntrStatus(eng_id, SIE_INT_RAWENC_OUTOVFL);
			nvt_dbg(ERR,"id %d, O0E-CHK: RAWENC_OUTOVFL !!\r\n",eng_id);
		}
		#endif
	}
	if (inSieIntStatus & SIE_INT_ACTST) {
		uiPtn |= FLGPTN_SIE_ACTST;
		//for verification
		//---------------------------------
		//g_uiSie1ActStCnt++;
		//g_uiSie1VdCnt = 0;//reset VD counter
		//---------------------------------
	}
	if (inSieIntStatus & SIE_INT_CRPST) {
		uiPtn |= FLGPTN_SIE_CRPST;
	}
	if (inSieIntStatus & SIE_INT_ACTEND) {
		uiPtn |= FLGPTN_SIE_ACTEND;
	}
	if (inSieIntStatus & SIE_INT_CROPEND) {
		if((pSieIntCbIdx[eng_id] == SIE_ISR_CB_VDLATCH) && (g_use_crp_end_sts[eng_id] == FALSE)) {
			g_use_crp_end_sts[eng_id] = TRUE;
			inSieIntStatus |= FLGPTN_SIE_CRPEND_VDLATISR;
//			nvt_dbg(ERR,"[%s][%d] %d ===============>\r\n", __func__, __LINE__, eng_id);
		}
		uiPtn |= FLGPTN_SIE_CRPEND;
	}
	if (inSieIntStatus & SIE_INT_BEHAVIOR_ERR) {
		nvt_dbg(ERR, "id %d, internal behavior err !!\r\n", eng_id);
	}
	if (inSieIntStatus & SIE_INT_DRAM_OUT1_END) {
		uiPtn |= FLGPTN_SIE_DO1_END;
	}
	if (inSieIntStatus & SIE_INT_DRAM_OUT2_END) {
		uiPtn |= FLGPTN_SIE_DO2_END;
	}
	if (inSieIntStatus & SIE_INT_DRAM_OUT3_END) {
		uiPtn |= FLGPTN_SIE_DO3_END;
	}
	if (inSieIntStatus & SIE_INT_DRAM_OUT4_END) {
		uiPtn |= FLGPTN_SIE_DO4_END;
	}
	if (inSieIntStatus & SIE_INT_DRAM_OUT5_END) {
		uiPtn |= FLGPTN_SIE_DO5_END;
		//if (g_uiSie1VdCnt != 0) {
			//DBG_ERR("receive VD between active start and Dram out5 end!\r\n");
		//	nvt_dbg(ERR,"receive VD between active start and Dram out5 end!\r\n");
			//abort();
		//}
		//g_uiSie1Out5Cnt++;
	}

#if (DRV_SUPPORT_IST == ENABLE)
	switch (eng_id) {
	case SIE_ENGINE_ID_1:
		if (pfIstCB_SIE != NULL) {
			drv_setIstEvent(DRV_IST_MODULE_SIE, inSieIntStatus);
		}
		break;

	case SIE_ENGINE_ID_2:
		if (pfIstCB_SIE2 != NULL) {
			drv_setIstEvent(DRV_IST_MODULE_SIE2, inSieIntStatus);
		}
		break;

	case SIE_ENGINE_ID_3:
		if (pfIstCB_SIE3 != NULL) {
			drv_setIstEvent(DRV_IST_MODULE_SIE3, inSieIntStatus);
		}
		break;

	case SIE_ENGINE_ID_4:
		if (pfIstCB_SIE4 != NULL) {
			drv_setIstEvent(DRV_IST_MODULE_SIE4, inSieIntStatus);
		}
		break;

	case SIE_ENGINE_ID_5:
		if (pfIstCB_SIE5 != NULL) {
			drv_setIstEvent(DRV_IST_MODULE_SIE5, inSieIntStatus);
		}
		break;

	default:
		break;
	}
#else

    if (g_use_vdlatch_cb[eng_id] && g_use_crp_end_sts[eng_id] && (pSieIntCbIdx[eng_id] == SIE_ISR_CB_VDLATCH)
		&& ((((inSieIntStatus & SIE_INT_VD) == SIE_INT_VD) && (g_use_isr_sync_hdal[eng_id] == 0)) || (g_use_isr_sync_hdal[eng_id] == 1))) {

		g_use_isr_sync_hdal[eng_id] = TRUE;
//        nvt_dbg(ERR,"[%s][%d] %d ===============>\r\n", __func__, __LINE__, eng_id);
		switch (eng_id) {
		case SIE_ENGINE_ID_1:
			if (pfSie1LatchIntCb) {
				pfSie1LatchIntCb(inSieIntStatus, &EngineStatus_cb);
			}
			break;

		case SIE_ENGINE_ID_2:
			if (pfSie2LatchIntCb) {
				pfSie2LatchIntCb(inSieIntStatus, &EngineStatus_cb);
			}
			break;

		case SIE_ENGINE_ID_3:
			if (pfSie3LatchIntCb) {
				pfSie3LatchIntCb(inSieIntStatus, &EngineStatus_cb);
			}
			break;

		case SIE_ENGINE_ID_4:
			if (pfSie4LatchIntCb) {
				pfSie4LatchIntCb(inSieIntStatus, &EngineStatus_cb);
			}
			break;

		case SIE_ENGINE_ID_5:
			if (pfSie5LatchIntCb) {
				pfSie5LatchIntCb(inSieIntStatus, &EngineStatus_cb);
			}
			break;

		default:
			break;
		}
    }else {
        //nvt_dbg(ERR,"else   isr_cb\r\n");
		switch (eng_id) {
		case SIE_ENGINE_ID_1:
			if (pfSie1IntCb) {
				pfSie1IntCb(inSieIntStatus, &EngineStatus_cb);
			}
			break;

		case SIE_ENGINE_ID_2:
			if (pfSie2IntCb) {
				pfSie2IntCb(inSieIntStatus, &EngineStatus_cb);
			}
			break;

		case SIE_ENGINE_ID_3:
			if (pfSie3IntCb) {
				pfSie3IntCb(inSieIntStatus, &EngineStatus_cb);
			}
			break;

		case SIE_ENGINE_ID_4:
			if (pfSie4IntCb) {
				pfSie4IntCb(inSieIntStatus, &EngineStatus_cb);
			}
			break;

		case SIE_ENGINE_ID_5:
			if (pfSie5IntCb) {
				pfSie5IntCb(inSieIntStatus, &EngineStatus_cb);
			}
			break;

		default:
			break;
		}
    }
#endif

	if (uiPtn != 0) {
		sie_platform_flg_set(eng_id, uiPtn);
	}
}

/**
    SIE Open Driver

    Open SIE engine

    @param[in] pObjCB SIE open object

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sie_open(SIE_ENGINE_ID eng_id, SIE_OPENOBJ *pObjCB)
{
	ER rt;
#if defined(__KERNEL__)
	int fastboot = kdrv_builtin_is_fastboot();
#else
	int fastboot = 0;
#endif

	// Lock semaphore
	rt = sie_lock(eng_id);
	if (rt != E_OK) {
		nvt_dbg(ERR,"Re-opened...\r\n");
		return rt;
	}

	// SW reset, "a special version" should be operated here
	// It has to be done before power on
	// It has to be done leaving reset=1

	if ((sie_drv_first_start_skip[eng_id] == FALSE) || !fastboot) {
		sie_setReset(eng_id, ENABLE);
		g_use_crp_end_sts[eng_id] = TRUE;
	}

	// change sie clk source
	sie_platform_set_clk_rate(eng_id, pObjCB);
	// enable sie clk (+ enable pll)
	sie_attach(eng_id, TRUE);
	// Disable Sram shut down
	sie_platform_disable_sram_shutdown(eng_id);

	if ((sie_drv_first_start_skip[eng_id] == FALSE) || !fastboot) {
		sie_clrIntrStatus(eng_id, SIE_INT_ALL);// Clear engine interrupt status, a special stage for SIE
		sie_setIntEnable(eng_id, 0);// Clear engine interrupt enable, a special stage for SIE
		sie_clrIntrStatus(eng_id, SIE_INT_ALL);// Clear engine interrupt status

		sie_platform_flg_clear(eng_id, FLGPTN_SIE_FRAMEEND);// Clear SW flag
	}

	// Hook call-back function, this is not used for new 520/528 flow.
	// Only emulation flow will use it.
	if (pObjCB->pfSieIsrCb != NULL) {
		switch (eng_id) {
		case SIE_ENGINE_ID_1:
			pfSie1IntCb   = pObjCB->pfSieIsrCb;
			break;

		case SIE_ENGINE_ID_2:
			pfSie2IntCb   = pObjCB->pfSieIsrCb;
			break;

		case SIE_ENGINE_ID_3:
			pfSie3IntCb   = pObjCB->pfSieIsrCb;
			break;

		case SIE_ENGINE_ID_4:
			pfSie4IntCb   = pObjCB->pfSieIsrCb;
			break;

		case SIE_ENGINE_ID_5:
			pfSie5IntCb   = pObjCB->pfSieIsrCb;
			break;

		default:
			break;
		}
	}

	// SW reset, "a special version" should be operated before
	if ((sie_drv_first_start_skip[eng_id] == FALSE) || !fastboot) {
		sie_setReset(eng_id, ENABLE);
	}

	return E_OK;
}

/**
    SIE Get Open Status

    Check if SIE engine is opened

    @return
        - @b FALSE: engine is not opened
        - @b TRUE: engine is opened
*/
BOOL sie_isOpened(SIE_ENGINE_ID eng_id)
{
	if (g_SieLockStatus[eng_id] == TASK_LOCKED) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
    SIE Close Driver

    Close SIE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sie_close(SIE_ENGINE_ID eng_id)
{
	if (g_SieLockStatus[eng_id] != TASK_LOCKED) {
		return E_SYS;
	}

	sie_setIntEnable(eng_id, 0);// Clear engine interrupt enable, a special stage for SIE
	sie_clrIntrStatus(eng_id, SIE_INT_ALL);// Clear engine interrupt status, a special stage for SIE

	sie_platform_enable_sram_shutdown(eng_id);// Enable Sram shut down
	sie_detach(eng_id, TRUE);// Detach(disable engine clock)
	sie_clr_buffer(eng_id);
	sie_unlock(eng_id);// Unlock semaphore

	return E_OK;
}

/**
    SIE Pause Operation

    Pause SIE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sie_pause(SIE_ENGINE_ID eng_id)
{
	UINT32 i=0;
	// disable ACT_EN
	sie_setActEn(eng_id,FALSE);
	sie_setLoad(eng_id);
	sie_setReset(eng_id, ENABLE);
	for (i=0;i<SIE_ENGINE_ID_MAX;i++) {
		sie_drv_first_start_skip[i] = FALSE; 
	}
	return E_OK;
}

/**
    SIE Start Operation

    Start SIE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sie_start(SIE_ENGINE_ID eng_id)
{
#if defined(__KERNEL__)
	int fastboot = kdrv_builtin_is_fastboot();
#else
	int fastboot = 0;
#endif

	sie_chgBurstLengthByMode(eng_id, BURST_MODE_REMAIN);
	if ((sie_drv_first_start_skip[eng_id] == FALSE) || !fastboot) {
		sie_clrIntrStatus(eng_id, SIE_INT_ALL); // need check
		// SW reset, "a special version" should be operated before for let reset=0
		sie_setReset(eng_id, DISABLE);//moved from setMode to start//
		//sie_drv_first_start_skip[eng_id] = FALSE; // move to sie_pause()
	}
	// enable ACT_EN
	sie_setActEn(eng_id,g_sie_act_en[eng_id]);
	sie_setLoad(eng_id);
	return E_OK;
}
/**
    SIE Set Mode

    Set SIE engine mode

    @param[in] pSieParam SIE configuration.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
#if (defined(_NVT_EMULATION_))

ER sie_setMode(SIE_ENGINE_ID eng_id, SIE_MODE_PARAM *pSieParam)
{
	// Configuration
	nvt_dbg(IND, "sie%d_setMode\r\n",eng_id+1);
	sie_setMainInput(eng_id, &(pSieParam->MainInInfo));
	//if (((pSieParam->uiFuncEn & SIE_RAWENC_EN) != 0) && ((pSieParam->uiFuncEn & SIE_RAWENC_GAMMA_EN) == 0)) {
	//	sie_setFunction(eng_id, pSieParam->uiFuncEn | SIE_RAWENC_GAMMA_EN);
	//} else {
	//	sie_setFunction(eng_id, pSieParam->uiFuncEn);
	//}
	sie_setFunction(eng_id, pSieParam->uiFuncEn);
	sie_setIntEnable(eng_id, 0);

	sie_setDramOutMode(eng_id, pSieParam->DramOutMode);
	sie_setVdHdInterval(eng_id, &(pSieParam->VdHdIntervalInfo));

    sie_setDVI(eng_id, &(pSieParam->DviInfo));
	sie_setSourceWindow(eng_id, &(pSieParam->SrcWinInfo));
	sie_setBreakPoint(eng_id, &(pSieParam->BreakPointInfo));
	sie_setActiveWindow(eng_id, &(pSieParam->ActWinInfo));
	sie_setCropWindow(eng_id, &(pSieParam->CrpWinInfo));

	sie_setDramIn0(eng_id, &(pSieParam->DramIn0Info));
	sie_setDramIn1(eng_id, &(pSieParam->DramIn1Info));
	sie_setDramIn2(eng_id, &(pSieParam->DramIn2Info));

    sie_setDramSingleOut(eng_id, &(pSieParam->DramSingleOutEn));

	sie_setDramOut0(eng_id, &(pSieParam->DramOut0Info));
	sie_setDramOut1(eng_id, &(pSieParam->DramOut1Info));


	sie_chgBurstLengthByMode(eng_id, BURST_MODE_REMAIN);

	sie_setPatGen(eng_id, &(pSieParam->PatGenInfo));
	sie_setObDt(eng_id, &(pSieParam->ObDtInfo));
	sie_setObOfs(eng_id, &(pSieParam->ObOfsInfo));
	sie_setDpc(eng_id, &(pSieParam->DefectInfo));
	sie_setECS(eng_id, &(pSieParam->EcsInfo));
	sie_setDGain(eng_id, &(pSieParam->DgainInfo));
	sie_setCGain(eng_id, &(pSieParam->CGainInfo));
	sie_setDecompanding(eng_id, &(pSieParam->DeCompandingInfo));
	sie_setCompanding(eng_id, &(pSieParam->CompandingInfo));

	//configure BCC parameters only when enabled
	sie_setBccParam(eng_id, &(pSieParam->BccParamInfo));


	sie_setStcsPath(eng_id, &(pSieParam->StcsPathInfo));
	sie_setVIG(eng_id, &(pSieParam->StcsVigInfo));
	sie_setCaCrp(eng_id, &(pSieParam->StcsCaLaSzGrpInfo.StcsCaCrpInfo));
	sie_setCaScl(eng_id, &(pSieParam->StcsCaLaSzGrpInfo.StcsCaSmplInfo));
	sie_setCaIrSub(eng_id, &(pSieParam->StcsCaLaSzGrpInfo.StcsCaIrSubInfo));
	sie_setLaIrSub(eng_id, &(pSieParam->StcsCaLaSzGrpInfo.StcsLaIrSubInfo));
	sie_setCaTh(eng_id, &(pSieParam->StcsCaThInfo));
	sie_setCaWin(eng_id, &(pSieParam->StcsCaLaSzGrpInfo.StcsCaWinInfo));
	sie_setLaCrp(eng_id, &(pSieParam->StcsCaLaSzGrpInfo.StcsLaCrpInfo));
	sie_setLaGma1(eng_id, &(pSieParam->StcsLaGma1Info));
	sie_setLaCg(eng_id, &(pSieParam->StcsLaCgInfo));
	sie_setLaTh(eng_id, &(pSieParam->StcsCaLaSzGrpInfo.StcsLaThInfo));
	sie_setLaWin(eng_id, &(pSieParam->StcsCaLaSzGrpInfo.StcsLaWinInfo));
	sie_setStcsOb(eng_id, &(pSieParam->StcsCaLaSzGrpInfo.StcsObInfo));
	sie_setmd(eng_id,&(pSieParam->mdInfo));

	sie_setDramOut2(eng_id, &(pSieParam->DramOut2Info));

	sie_clrIntrStatus(eng_id, SIE_INT_ALL);
	sie_setIntEnable(eng_id, pSieParam->uiIntrpEn);

	return E_OK;
}
#endif

ER sie_chgFuncEn(SIE_ENGINE_ID eng_id, SIE_FUNC_SWITCH_SEL FuncSwitch, UINT32 uiFuncSel)
{
	//if ((uiFuncSel & SIE_RAWENC_EN) != 0) {
	//	uiFuncSel |= SIE_RAWENC_GAMMA_EN;
	//}

	if (FuncSwitch == SIE_FUNC_SET) {
		sie_setFunction(eng_id, uiFuncSel);
	} else if (FuncSwitch == SIE_FUNC_ENABLE) {
		sie_enableFunction(eng_id, TRUE, uiFuncSel);
	} else if (FuncSwitch == SIE_FUNC_DISABLE) {
		sie_enableFunction(eng_id, FALSE, uiFuncSel);
	}
	sie_setLoad(eng_id);

	return E_OK;
}

/**
    Dynamic change SIE parameters

    @param[in] pParam Parameters.
    @param[in] FunSel Change function selection.

    @return
    -@b E_OK  Engine set mode and parameters without error.\n
    -@b E_PAR Egnine set parameters error.\n
*/
ER sie_chgParam(SIE_ENGINE_ID eng_id, void *pParam, SIE_CHANGE_FUN_PARAM_SEL FunSel)
{
	if (pParam == NULL) {
		nvt_dbg(ERR,"parameter NULL...\r\n");
		return E_PAR;
	}

	if (FunSel == SIE_CHG_MAIN_IN) {
		sie_setMainInput(eng_id, (SIE_MAIN_INPUT_INFO *)pParam);
	} else if (FunSel == SIE_CHG_SRC_WIN)      {
		sie_setSourceWindow(eng_id, (SIE_SRC_WIN_INFO *)pParam);
	} else if (FunSel == SIE_CHG_ACT_WIN)      {
		sie_setActiveWindow(eng_id, (SIE_ACT_WIN_INFO *)pParam);
	} else if (FunSel == SIE_CHG_PAT_GEN)      {
		sie_setPatGen(eng_id, (SIE_PATGEN_INFO *)pParam);
	} else if (FunSel == SIE_CHG_CROP_WIN)      {
		sie_setCropWindow(eng_id, (SIE_CRP_WIN_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_IN_ADDR)       {
		sie_setChgInAddr(eng_id, (SIE_CHG_IN_ADDR_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_OUT_ADDR)       {
		sie_setChgOutAddr(eng_id, (SIE_CHG_OUT_ADDR_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_OUT_ADDR_FLIP)  {
		sie_setChgOutAddr_Flip(eng_id, (SIE_CHG_OUT_ADDR_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_IO_LOFS)       {
		sie_setChgIOLofs(eng_id, (SIE_CHG_IO_LOFS_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_OUT_SRC)       {
		sie_setChgOutSrc(eng_id, (SIE_CHG_OUT_SRC_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_DVI)           {
		sie_setDVI(eng_id, (SIE_DVI_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_OBDT)          {
		sie_setObDt(eng_id, (SIE_OB_DT_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_OBOFS)         {
		sie_setObOfs(eng_id, (SIE_OB_OFS_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_FLIP)          {
		sie_setFlip(eng_id, (SIE_FLIP_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_DP) {
		sie_setDpc(eng_id, (SIE_DPC_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_ECS)           {
		sie_setECS(eng_id, (SIE_ECS_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_DGAIN)         {
		sie_setDGain(eng_id, (SIE_DGAIN_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_CG)            {
		sie_setCGain(eng_id, (SIE_CG_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_DRAM_BURST)    {
		sie_setDramBurst(eng_id, (SIE_DRAM_BURST_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_VIG)           {
		sie_setVIG(eng_id, (SIE_VIG_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_STCS_PATH)     {
		sie_setStcsPath(eng_id, (SIE_STCS_PATH_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_CALASIZE_GRP) {
		SIE_STCS_CALASIZE_GRP_INFO *pStcsCaLaSzGrpInfo;
		pStcsCaLaSzGrpInfo = (SIE_STCS_CALASIZE_GRP_INFO *)pParam;
		sie_setCaCrp(eng_id, &(pStcsCaLaSzGrpInfo->StcsCaCrpInfo));
		sie_setCaScl(eng_id, &(pStcsCaLaSzGrpInfo->StcsCaSmplInfo));
		sie_setCaWin(eng_id, &(pStcsCaLaSzGrpInfo->StcsCaWinInfo));
		sie_setLaCrp(eng_id, &(pStcsCaLaSzGrpInfo->StcsLaCrpInfo));
		sie_setLaWin(eng_id, &(pStcsCaLaSzGrpInfo->StcsLaWinInfo));
	} else if (FunSel ==  SIE_CHG_CA_TH)             {
		sie_setCaTh(eng_id, (SIE_CA_TH_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_CA_IRSUB)          {
		sie_setCaIrSub(eng_id, (SIE_CA_IRSUB_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_LA_CG)             {
		sie_setLaCg(eng_id, (SIE_LA_CG_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_LA_GMA)            {
		sie_setLaGma1(eng_id, (SIE_LA_GMA_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_LA_IRSUB)          {
		sie_setLaIrSub(eng_id, (SIE_LA_IRSUB_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_BCC_ADJ) {
		sie_setBccIniParm(eng_id, &g_sieBccParamInfo, (*(UINT32 *)pParam));
		sie_setBccParam(eng_id, &g_sieBccParamInfo);
	} else if (FunSel ==  SIE_CHG_ACT_ENG) {
		g_sie_act_en[eng_id] = (*(BOOL *)pParam);
	} else if (FunSel == SIE_CHG_ACT_EN) {
		sie_setActEn(eng_id, g_sie_act_en[eng_id]);
	} else if (FunSel ==  SIE_CHG_BP1) {
		sie_setBreakPoint1(eng_id, (SIE_BREAKPOINT_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_BP2) {
		sie_setBreakPoint2(eng_id, (SIE_BREAKPOINT_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_BP3) {
		sie_setBreakPoint3(eng_id, (SIE_BREAKPOINT_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_INTE) {
		sie_setIntEnable(eng_id, (*(UINT32 *)pParam));
	} else if (FunSel == SIE_CHG_OUT_BITDEPTH) {
		sie_setOutBitDepth(eng_id, (*(SIE_PACKBUS_SEL *)pParam));
	} else if (FunSel == SIE_CHG_SINGLEOUT) {
		sie_setDramSingleOut(eng_id, (SIE_DRAM_SINGLE_OUT *)pParam);
	} else if (FunSel == SIE_CHG_VDHD_INTERVAL) {
		sie_setVdHdInterval(eng_id, (SIE_VDHD_INTERVAL_INFO *)pParam);
	} else if (FunSel == SIE_CHG_DECOMPANDING) {
	    sie_setDecompanding(eng_id, (SIE_DECOMPANDING_INFO *)pParam);
	} else if (FunSel == SIE_CHG_COMPANDING) {
	    sie_setCompanding(eng_id, (SIE_COMPANDING_INFO *)pParam);
	} else if (FunSel == SIE_CHG_OUTMODE) {
		sie_setDramOutMode(eng_id, (*(SIE_DRAM_OUT_CTRL *)pParam));
	} else if (FunSel == SIE_CHG_RAWENC_RATE) {
	    sie_setBccSegBitNumReg(eng_id, (*(UINT32 *)pParam));
	} else if (FunSel == SIE_CHG_RINGBUF) {
		sie_setChgRingBuf(eng_id, (SIE_CHG_RINGBUF_INFO *)pParam);
	} else if (FunSel == SIE_CHG_STCS_OB) {
	    sie_setStcsOb(eng_id, (SIE_STCS_OB_INFO *)pParam);
	} else if (FunSel == SIE_CHG_MD) {
	    sie_setmd(eng_id, (SIE_MD_INFO *)pParam);
	}
#if 0
	  else if (FunSel == SIE_CHG_YOUT) {
		SIE_YOUT_INFO *pChgYoutInfo;
		pChgYoutInfo = (SIE_YOUT_INFO *)pParam;
		sie_setYoutCg(eng_id, &(pChgYoutInfo->YoutCGInfo));
		sie_setYoutScl(eng_id, &(pChgYoutInfo->YoutScalInfo));
		sie_setYoutWin(eng_id, &(pChgYoutInfo->YoutWinInfo));
		sie_setYoutAccm(eng_id, &(pChgYoutInfo->YoutAccmInfo));
	} else if (FunSel == SIE_CHG_GRIDLINE)       {
	    sie_setGridLine(eng_id, (SIE_GRIDLINE_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_VA_CROP)           {
		sie_setVaCrp(eng_id, (SIE_VA_CROP_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_VA_WIN)            {
		sie_setVaWin(eng_id,(SIE_VA_WIN_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_VA_CG)             {
		sie_setVaCg(eng_id, (SIE_VA_CG_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_VA_GMA)            {
		sie_setVaGma2(eng_id, (SIE_VA_GMA_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_VA_FLTR_G1)        {
		sie_setVaFltrG1(eng_id, (SIE_VA_FLTR_GROUP_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_VA_FLTR_G2)        {
		sie_setVaFltrG2(eng_id, (SIE_VA_FLTR_GROUP_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_VA_INDEP_WIN0)     {
		sie_setVaIndepWin(eng_id,(SIE_VA_INDEP_WIN_INFO *)pParam, 0);
	} else if (FunSel ==  SIE_CHG_VA_INDEP_WIN1)     {
		sie_setVaIndepWin(eng_id,(SIE_VA_INDEP_WIN_INFO *)pParam, 1);
	} else if (FunSel ==  SIE_CHG_VA_INDEP_WIN2)     {
		sie_setVaIndepWin(eng_id,(SIE_VA_INDEP_WIN_INFO *)pParam, 2);
	} else if (FunSel ==  SIE_CHG_VA_INDEP_WIN3)     {
		sie_setVaIndepWin(eng_id,(SIE_VA_INDEP_WIN_INFO *)pParam, 3);
	} else if (FunSel ==  SIE_CHG_VA_INDEP_WIN4)     {
		sie_setVaIndepWin(eng_id,(SIE_VA_INDEP_WIN_INFO *)pParam, 4);
	} else if (FunSel ==  SIE_CHG_ETH) {
		sie_setEth(eng_id, (SIE_ETH_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_BS_V)          {
		sie_setBSV(eng_id, (SIE_BS_V_INFO *)pParam);
	} else if (FunSel ==  SIE_CHG_BS_H)          {
		sie_setBSH(eng_id, (SIE_BS_H_INFO *)pParam);
	}
#endif
	sie_setLoad(eng_id);

	return E_OK;
}

/**
    SIE Wait event

    Wait for event.

    @param[in] uiVdCnt number of VD ends to wait after clearing flag.

    @return None.
*/
ER sie_waitEvent(SIE_ENGINE_ID eng_id, SIE_WAIT_EVENT_SEL WaitEvent, BOOL bClrFlag)
{
	UINT32 uiFLAG;
	FLGPTN uiFlag;

	switch (WaitEvent) {
		case SIE_WAIT_VD:
			uiFLAG = FLGPTN_SIE_FRAMEEND;
			break;
		case SIE_WAIT_BP1:
			uiFLAG = FLGPTN_SIE_BP1;
			break;
		case SIE_WAIT_BP2:
			uiFLAG = FLGPTN_SIE_BP2;
			break;
		case SIE_WAIT_BP3:
			uiFLAG = FLGPTN_SIE_BP3;
			break;
		case SIE_WAIT_ACTST:
			uiFLAG = FLGPTN_SIE_ACTST;
			break;
		case SIE_WAIT_CRPST:
			uiFLAG = FLGPTN_SIE_CRPST;
			break;
		case SIE_WAIT_ACTEND:
			uiFLAG = FLGPTN_SIE_ACTEND;
			break;
		case SIE_WAIT_CRPEND:
			uiFLAG = FLGPTN_SIE_CRPEND;
			break;
		case SIE_WAIT_DO0_END:
			uiFLAG = FLGPTN_SIE_DO0_END;
			break;
		case SIE_WAIT_DO1_END:
			uiFLAG = FLGPTN_SIE_DO1_END;
			break;
		case SIE_WAIT_DO2_END:
			uiFLAG = FLGPTN_SIE_DO2_END;
			break;
		case SIE_WAIT_DO3_END:
			uiFLAG = FLGPTN_SIE_DO3_END;
			break;
		case SIE_WAIT_DO4_END:
			uiFLAG = FLGPTN_SIE_DO4_END;
			break;
		case SIE_WAIT_DO5_END:
			uiFLAG = FLGPTN_SIE_DO5_END;
			break;
		default:
			//DBG_ERR("no such signal flag\r\n");
			nvt_dbg(ERR,"no such signal flag\r\n");
			return E_SYS;
			break;
	}

	if (bClrFlag == TRUE) {
		sie_platform_flg_clear(eng_id, uiFLAG);
	}

	if (sie_platform_flg_wait(eng_id, &uiFlag, uiFLAG))
	{
		return E_SYS;
	}

	return E_OK;
}

ER sie_chgBurstLengthByMode(SIE_ENGINE_ID eng_id, SIE_DRAM_BURST_MODE_SEL BurstMdSel)
{
	static SIE_DRAM_BURST_MODE_SEL gs_SieDramBurstMd = BURST_MODE_NORMAL;
	ER rt = E_OK;

	if (BurstMdSel == BURST_MODE_NORMAL) {
		gs_SieDramBurstMd = BURST_MODE_NORMAL;
	} else if (BurstMdSel == BURST_MODE_BURST_CAP) {
		gs_SieDramBurstMd = BURST_MODE_BURST_CAP;
	} else if (BurstMdSel == BURST_MODE_REMAIN) {
		// do nothing
	}

	if (gs_SieDramBurstMd == BURST_MODE_NORMAL) {
		SIE_BURST_LENGTH BurstLengthTmp;
		BurstLengthTmp.BurstLenOut0 = SIE_BURST_OUT0_128W; // 2
		BurstLengthTmp.BurstLenOut1 = SIE_BURST_64W; // 0: 64w
		BurstLengthTmp.BurstLenOut2 = SIE_BURST_64W; // 0: 32w
		BurstLengthTmp.BurstLenOut3 = SIE_BURST_64W; // don't care
		BurstLengthTmp.BurstLenOut4 = SIE_BURST_64W; // don't care
		BurstLengthTmp.BurstLenOut5 = SIE_BURST_64W; // don't care
		BurstLengthTmp.BurstLenIn0  = SIE_BURST_64W; // don't care
		BurstLengthTmp.BurstLenIn1  = SIE_BURST_48W; // 1: 48w
		BurstLengthTmp.BurstLenIn2  = SIE_BURST_48W; // 1: 48w
		BurstLengthTmp.uiBurstIntvlOut0 = 8;
		sie_chgBurstLength(eng_id, &BurstLengthTmp);
	} else if (gs_SieDramBurstMd == BURST_MODE_BURST_CAP) {
		SIE_BURST_LENGTH BurstLengthTmp;
		BurstLengthTmp.BurstLenOut0 = SIE_BURST_OUT0_128W; // 2
		BurstLengthTmp.BurstLenOut1 = SIE_BURST_64W; // 0: 64w
		BurstLengthTmp.BurstLenOut2 = SIE_BURST_64W; // 0: 32w
		BurstLengthTmp.BurstLenOut3 = SIE_BURST_64W; // don't care
		BurstLengthTmp.BurstLenOut4 = SIE_BURST_64W; // don't care
		BurstLengthTmp.BurstLenOut5 = SIE_BURST_64W; // don't care
		BurstLengthTmp.BurstLenIn0  = SIE_BURST_64W; // don't care
		BurstLengthTmp.BurstLenIn1  = SIE_BURST_48W; // 1: 48w
		BurstLengthTmp.BurstLenIn2  = SIE_BURST_48W; // 1: 48w
		BurstLengthTmp.uiBurstIntvlOut0 = 8;
		sie_chgBurstLength(eng_id, &BurstLengthTmp);
	} else {
		nvt_dbg(ERR,"gs_SieDramBurstMd=%d !!!", (int)gs_SieDramBurstMd);
		rt = E_PAR;
	}

	return rt;
}

/**

    SIE1 Get clock rate

    Get Engine clock

    @return None.
*/
UINT32 sie_getClockRate(SIE_ENGINE_ID id)
{
	UINT32 uiClkRate = 0;

//	if (linuxpll_getClockFreq(SIECLK_FREQ, &uiClkRate) == E_OK) {
	if (1) {
		return uiClkRate / 1000000;
	} else {
		nvt_dbg(ERR,"pll getClkFreq fail !!!");
		return 0;
	}
}

void sie_getLAGamma(SIE_ENGINE_ID eng_id, UINT32 *puiGammaLut)
{
	sie_getGammaLut_Reg(eng_id, puiGammaLut);
}

void sie_get_la_rslt(SIE_ENGINE_ID eng_id, UINT16 *puiBufLa1, UINT16 *puiBufLa2, SIE_LA_WIN_INFO *LaWinInfo, UINT32 uiBuffAddr)
{
	sie_getLAResultManual(eng_id, puiBufLa1, puiBufLa2, LaWinInfo, uiBuffAddr);
}

void sie_get_sat_gain_info(SIE_ENGINE_ID eng_id, UINT32 *gain)
{
	sie_getSatgainInfo(eng_id, gain);
}

void sie_get_ca_rslt(SIE_ENGINE_ID eng_id, SIE_STCS_CA_RSLT_INFO *CaRsltInfo, SIE_CA_WIN_INFO *CaWinInfo, UINT32 uiBuffAddr)
{
	sie_getCAResultManual(eng_id, CaRsltInfo, CaWinInfo, uiBuffAddr);
}

void sie_calc_ir_level(SIE_ENGINE_ID id, SIE_STCS_CA_RSLT_INFO *ca_rst, SIE_CA_WIN_INFO ca_win_info, SIE_CA_IRSUB_INFO ca_ir_sub_info)
{
	UINT32 db_dg_ratio = 0, b_g_value;
	UINT32 ir_range_step = 256/RGBIR_BINNUM;
	UINT16 ir_range[RGBIR_BINNUM+1], i, j;
	UINT16 ir_db_hist[RGBIR_BINNUM] = {0}, ir_dg_hist[RGBIR_BINNUM] = {0};
	UINT16 g_sel_bin[3] = {0}, g_sel_bin_num[3] = {0};
	UINT16 b_sel_max_bin = 0, b_sel_max_bin_num = 0;
	UINT32 ir_level_tk = 0, ir_level_lr_dg = 0;

	for (i = 0 ; i < RGBIR_BINNUM+1 ; i++) {
		ir_range[i] = i * ir_range_step;
	}
	ir_range[RGBIR_BINNUM] = 256+1;

	for (i = 0 ; i < ca_win_info.uiWinNmX * ca_win_info.uiWinNmY ; i++) {
		if (ca_rst->puiBufB[i] > 0 && ca_rst->puiBufG[i] > 0) {
			b_g_value = ca_rst->puiBufB[i] + ((ca_rst->puiBufIR[i] * ca_ir_sub_info.uiCaIrSubBWet)>>8);
			db_dg_ratio = (ca_rst->puiBufIR[i]<<8) / b_g_value;
			db_dg_ratio = (db_dg_ratio > 256)?256:db_dg_ratio;
			for (j = 1 ; j < RGBIR_BINNUM+1 ; j++) {
				if ((db_dg_ratio >= ir_range[j-1]) && (db_dg_ratio < ir_range[j]) && (b_g_value < RGBIR_VALUPP) && (b_g_value > RGBIR_VALLOW)) {
					ir_db_hist[j-1]++;
					break;
				}
			}

			b_g_value = ca_rst->puiBufG[i]+((ca_rst->puiBufIR[i]*ca_ir_sub_info.uiCaIrSubGWet)>>8);
			db_dg_ratio = (ca_rst->puiBufIR[i]<<8)/b_g_value;
			db_dg_ratio = (db_dg_ratio > 256)?256:db_dg_ratio;
			for (j = 1 ; j < RGBIR_BINNUM+1 ; j++) {
				if ((db_dg_ratio >= ir_range[j-1]) && (db_dg_ratio<ir_range[j]) && (b_g_value < RGBIR_VALUPP) && (b_g_value > RGBIR_VALLOW)) {
					ir_dg_hist[j-1]++;
					break;
				}
			}
		}
	}

    b_sel_max_bin = 0;
	b_sel_max_bin_num = 0;
	for (i = 0 ; i < RGBIR_BINNUM ; i++) {
		if (ir_db_hist[i] > b_sel_max_bin_num) {
			b_sel_max_bin = i;
			b_sel_max_bin_num = ir_db_hist[i];
		}

		if (ir_dg_hist[i] > g_sel_bin_num[0]) {
			g_sel_bin[2] = g_sel_bin[1];
			g_sel_bin_num[2] = g_sel_bin_num[1];
			g_sel_bin[1] = g_sel_bin[0];
			g_sel_bin_num[1] = g_sel_bin_num[0];
			g_sel_bin[0] = i;
			g_sel_bin_num[0] = ir_dg_hist[i];
		}else if (ir_dg_hist[i] > g_sel_bin_num[1]) {
			g_sel_bin[2] = g_sel_bin[1];
			g_sel_bin_num[2] = g_sel_bin_num[1];
			g_sel_bin[1] = i;
			g_sel_bin_num[1] = ir_dg_hist[i];
		}else if (ir_dg_hist[i] > g_sel_bin_num[2]) {
			g_sel_bin[2] = i;
			g_sel_bin_num[2] = ir_dg_hist[i];
		}
	}

	ir_level_tk = (b_sel_max_bin+1)<<3;
    if ((g_sel_bin_num[0]+g_sel_bin_num[1]+g_sel_bin_num[2]) == 0) {
		ir_level_lr_dg = 0;
	} else {
		if (g_sel_bin_num[0]+g_sel_bin_num[1]+g_sel_bin_num[2] < RGBIR_IRDG_SUM_THR) {
			ir_level_lr_dg = ir_leve_lr_dg_last[id];
		} else {
			ir_level_lr_dg = ((ir_range[g_sel_bin[0]]+4)*g_sel_bin_num[0] + (ir_range[g_sel_bin[1]]+4)*g_sel_bin_num[1] + (ir_range[g_sel_bin[2]]+4)*g_sel_bin_num[2])/(g_sel_bin_num[0]+g_sel_bin_num[1]+g_sel_bin_num[2]);
		}
	}

    ir_level_tk = (ir_level_tk_last[id]*3+ir_level_tk)>>2;
	ir_level_tk_last[id] = ir_level_tk;

	 // D or F light
    if (ir_level_tk < 160) {
		rgb_ir_level_rst[id] = ir_level_tk;
    } else {
		ir_level_lr_dg = (ir_leve_lr_dg_last[id]*3+ir_level_lr_dg)>>2;
		ir_leve_lr_dg_last[id] = ir_level_lr_dg;

		if (ir_level_lr_dg < 152) {
			rgb_ir_level_rst[id] = 152;
		} else {
			rgb_ir_level_rst[id] = ir_level_lr_dg;
		}
    }
}

UINT32 sie_get_ir_level(SIE_ENGINE_ID eng_id)
{
	return rgb_ir_level_rst[eng_id];
}

void sie_get_histo(SIE_ENGINE_ID eng_id, UINT16 *puiHisto)
{
	sie_getHisto(eng_id, puiHisto);
}

void sie_get_mdrslt(SIE_ENGINE_ID eng_id, SIE_MD_RESULT_INFO *pMdInfo)
{
	sie_getMdResult(eng_id, pMdInfo);
}

void sie_get_dram_single_out(SIE_ENGINE_ID eng_id, SIE_DRAM_SINGLE_OUT *pDOut)
{
	sie_getDramSingleOut(eng_id, pDOut);
}

void sie_get_sys_debug_info(SIE_ENGINE_ID eng_id, SIE_SYS_DEBUG_INFO *pDbugInfo)
{
	sie_getSysDbgInfo(eng_id, pDbugInfo);
}

void sie_set_checksum_en(SIE_ENGINE_ID eng_id, UINT32 uiEn)
{
	sie_setChecksumEn(eng_id, uiEn);
}
//@}

/**
SIE Calc API

@name   SIE_Calc
*/
//@{

void sie_calcECSScl(SIE_ECS_INFO *pEcsParam, SIE_ECS_ADJ_INFO *pEcsAdjParam)
{
#define SCES_HFRAC_MAX ((1<<15)-1)
#define SCES_VFRAC_MAX ((1<<15)-1)
	UINT32 uiHin, uiVin, uiHout, uiVout;
	UINT32 uiHfrac, uiVfrac;

	switch (pEcsParam->MapSizeSel) {
	case ECS_MAPSIZE_65X65:
		uiHin = uiVin = 65;
		break;
	case ECS_MAPSIZE_49X49:
		uiHin = uiVin = 49;
		break;
	case ECS_MAPSIZE_33X33:
		uiHin = uiVin = 33;
		break;
	default:
		uiHin = uiVin = 65;
		break;
	}

	uiHout = pEcsAdjParam->uiActSzX;
	uiVout = pEcsAdjParam->uiActSzY;
	if (uiHout <= 1 || uiVout <= 1) {
		nvt_dbg(ERR,"Act Sz ERR!\r\n");
		return;
	}

	uiHfrac = ((uiHin - 1) * 65536) / (uiHout - 1);
	if (uiHfrac > SCES_HFRAC_MAX) {
		nvt_dbg(WRN,"ECS Hscale error!\r\n");
		uiHfrac = SCES_HFRAC_MAX;
	}

	uiVfrac = ((uiVin - 1) * 65536) / (uiVout - 1);
	if (uiVfrac > SCES_VFRAC_MAX) {
		nvt_dbg(WRN,"ECS Vscale error!\r\n");
		uiVfrac = SCES_VFRAC_MAX;
	}

	pEcsParam->uiHSclFctr = uiHfrac;
	pEcsParam->uiVSclFctr = uiVfrac;
}


void sie_calcCaLaSize(SIE_ENGINE_ID eng_id, SIE_STCS_CALASIZE_GRP_INFO *pCaLaSzGrpInfo, SIE_STCS_CALASIZE_ADJ_INFO *pCaLaSzAdjInfo)
{
	UINT32 uiInSzH, uiInSzV;
	UINT32 uiBfSzH, uiBfSzV;
	UINT32 uiScFcH, uiScFcV;
	UINT32 uiCaWnSzH, uiCaWnSzV;
	UINT32 uiCaWnNmH, uiCaWnNmV;
	UINT32 uiLaCpStH, uiLaCpStV;
	UINT32 uiLaCpSzH, uiLaCpSzV;
	UINT32 uiLaWnSzH, uiLaWnSzV;
	UINT32 uiLaWnNmH, uiLaWnNmV;

	// stage-1, CA Crop
	uiInSzH = pCaLaSzAdjInfo->uiCaRoiSzX & 0xfffffffe;
	uiInSzV = pCaLaSzAdjInfo->uiCaRoiSzY & 0xfffffffe;
	if (uiInSzH == 0 || uiInSzV == 0) {
		nvt_dbg(ERR,"CA Sz ERR!, InSzH/V = (%d,%d)\r\n", (int)uiInSzH, (int)uiInSzV);
		return;
	}
	pCaLaSzGrpInfo->StcsCaCrpInfo.uiStX = pCaLaSzAdjInfo->uiCaRoiStX & 0xfffffffe;
	pCaLaSzGrpInfo->StcsCaCrpInfo.uiStY = pCaLaSzAdjInfo->uiCaRoiStY & 0xfffffffe;
	pCaLaSzGrpInfo->StcsCaCrpInfo.uiSzX = uiInSzH;//pCaLaSzAdjInfo->uiCaRoiSzX;
	pCaLaSzGrpInfo->StcsCaCrpInfo.uiSzY = uiInSzV;//pCaLaSzAdjInfo->uiCaRoiSzY;

	// stage-2, sub-sample
	//if (id<=1)
	//{
	//	uiBfSzH = (uiInSzH > 1280) ? (1280) : (uiInSzH);
	//	uiBfSzV = (uiInSzV > 960) ? (960) : (uiInSzV);
	//}else
	{
		uiBfSzH = (uiInSzH > 640) ? (640) : (uiInSzH);
		uiBfSzV = (uiInSzV > 512) ? (512) : (uiInSzV);
	}
	uiBfSzH = uiBfSzH / 2;
	uiBfSzV = uiBfSzV / 2;
	uiScFcH = (((uiInSzH / 2) - 1) << 10) / ((uiBfSzH/* /2 */) - 1);
	uiScFcV = (((uiInSzV / 2) - 1) << 10) / ((uiBfSzV/* /2 */) - 1);
	if (uiScFcH > 0x1fff) {
		nvt_dbg(ERR,"uiScFcH = 0x%x out of range\r\n", (int)uiScFcH);
	}
	if (uiScFcV > 0x1fff) {
		nvt_dbg(ERR,"uiScFcV = 0x%x out of range\r\n", (int)uiScFcV);
	}
	pCaLaSzGrpInfo->StcsCaSmplInfo.uiFctrX = uiScFcH;
	pCaLaSzGrpInfo->StcsCaSmplInfo.uiFctrY = uiScFcV;

	// stage-3, CA window
	uiCaWnNmH = pCaLaSzAdjInfo->uiCaWinNmX;
	uiCaWnNmV = pCaLaSzAdjInfo->uiCaWinNmY;
	if (uiCaWnNmH != 0 && uiCaWnNmV != 0) {
		uiCaWnSzH = uiBfSzH / uiCaWnNmH;
		uiCaWnSzV = uiBfSzV / uiCaWnNmV;
		if (uiCaWnSzH == 0) {
			nvt_dbg(ERR,"uiCaWnSzH = %d, forced to be 1\r\n", (int)uiCaWnSzH);
			uiCaWnSzH = 1;
			nvt_dbg(ERR,"uiCaWnNmH = %d, forced to be %d\r\n", (int)uiCaWnNmH, (int)uiBfSzH);
			uiCaWnNmH = uiBfSzH;
		}
		if (uiCaWnSzV == 0) {
			nvt_dbg(ERR,"uiCaWnSzV = %d, forced to be 1\r\n", (int)uiCaWnSzV);
			uiCaWnSzV = 1;
			nvt_dbg(ERR,"uiCaWnNmV = %d, forced to be %d\r\n", (int)uiCaWnNmV, (int)uiBfSzV);
			uiCaWnNmV = uiBfSzV;
		}
	} else { // if(uiCaWnNmH==0 || uiCaWnNmV==0)
		nvt_dbg(ERR,"CA Num ERR!\r\n");
		return;
	}
	pCaLaSzGrpInfo->StcsCaWinInfo.uiWinNmX = uiCaWnNmH;
	pCaLaSzGrpInfo->StcsCaWinInfo.uiWinNmY = uiCaWnNmV;
	pCaLaSzGrpInfo->StcsCaWinInfo.uiWinSzX = uiCaWnSzH;
	pCaLaSzGrpInfo->StcsCaWinInfo.uiWinSzY = uiCaWnSzV;

	// stage-4, LA Crop
	uiLaCpStH = pCaLaSzAdjInfo->uiLaRoiStX;// * uiBfSzH / uiInSzH;
	uiLaCpStV = pCaLaSzAdjInfo->uiLaRoiStY;// * uiBfSzV / uiInSzV;
	uiLaCpSzH = pCaLaSzAdjInfo->uiLaRoiSzX;// * uiBfSzH / uiInSzH;
	uiLaCpSzV = pCaLaSzAdjInfo->uiLaRoiSzY;// * uiBfSzV / uiInSzV;
	pCaLaSzGrpInfo->StcsLaCrpInfo.uiStX = uiLaCpStH;
	pCaLaSzGrpInfo->StcsLaCrpInfo.uiStY = uiLaCpStV;
	pCaLaSzGrpInfo->StcsLaCrpInfo.uiSzX = uiLaCpSzH;
	pCaLaSzGrpInfo->StcsLaCrpInfo.uiSzY = uiLaCpSzV;

	// stage-5, LA window
	uiLaWnNmH = pCaLaSzAdjInfo->uiLaWinNmX;
	uiLaWnNmV = pCaLaSzAdjInfo->uiLaWinNmY;
	if (uiLaWnNmH != 0 && uiLaWnNmV != 0) {
		uiLaWnSzH = uiLaCpSzH / uiLaWnNmH;
		uiLaWnSzV = uiLaCpSzV / uiLaWnNmV;
		if (uiLaWnSzH == 0) {
			nvt_dbg(ERR,"uiLaWnSzH = %d, forced to be 1\r\n", (int)uiLaWnSzH);
			uiLaWnSzH = 1;
			nvt_dbg(ERR,"uiLaWnNmH = %d, forced to be %d\r\n", (int)uiLaWnNmH, (int)uiLaCpSzH);
			uiLaWnNmH = uiLaCpSzH;
		}
		if (uiLaWnSzV == 0) {
			nvt_dbg(ERR,"uiLaWnSzV = %d, forced to be 1\r\n", (int)uiLaWnSzV);
			uiLaWnSzV = 1;
			nvt_dbg(ERR,"uiLaWnNmV = %d, forced to be %d\r\n", (int)uiLaWnNmV, (int)uiLaCpSzV);
			uiLaWnNmV = uiLaCpSzV;
		}
	} else if (uiLaWnNmH == 0 && uiLaWnNmV == 0) {
		uiLaWnSzH = 0;
		uiLaWnSzV = 0;
	} else {
		nvt_dbg(ERR,"LA Num ERR!\r\n");
		return;
	}
	pCaLaSzGrpInfo->StcsLaWinInfo.uiWinNmX = uiLaWnNmH;
	pCaLaSzGrpInfo->StcsLaWinInfo.uiWinNmY = uiLaWnNmV;
	pCaLaSzGrpInfo->StcsLaWinInfo.uiWinSzX = uiLaWnSzH;
	pCaLaSzGrpInfo->StcsLaWinInfo.uiWinSzY = uiLaWnSzV;

}

void sie_reg_isr_cb(SIE_ENGINE_ID id, SIE_DRV_ISR_FP fp)
{
	if (fp != NULL) {
		switch (id) {
		case SIE_ENGINE_ID_1:
			pfSie1IntCb = fp;
			pSieIntCbIdx[SIE_ENGINE_ID_1] = SIE_ISR_CB_IMD;
			break;

		case SIE_ENGINE_ID_2:
			pfSie2IntCb = fp;
			pSieIntCbIdx[SIE_ENGINE_ID_2] = SIE_ISR_CB_IMD;
			break;

		case SIE_ENGINE_ID_3:
			pfSie3IntCb = fp;
			pSieIntCbIdx[SIE_ENGINE_ID_3] = SIE_ISR_CB_IMD;
			break;

		case SIE_ENGINE_ID_4:
			pfSie4IntCb = fp;
			pSieIntCbIdx[SIE_ENGINE_ID_4] = SIE_ISR_CB_IMD;
			break;

		case SIE_ENGINE_ID_5:
			pfSie5IntCb = fp;
			pSieIntCbIdx[SIE_ENGINE_ID_5] = SIE_ISR_CB_IMD;
			break;

		default:
			break;
		}
	}
}

void sie_reg_vdlatch_isr_cb(SIE_ENGINE_ID id, SIE_DRV_ISR_FP fp)
{
	if (fp != NULL) {
		switch (id) {
		case SIE_ENGINE_ID_1:
			pfSie1LatchIntCb =
fp;
			pSieIntCbIdx[SIE_ENGINE_ID_1] = SIE_ISR_CB_VDLATCH;
			break;

		case SIE_ENGINE_ID_2:
			pfSie2LatchIntCb = fp;
			pSieIntCbIdx[SIE_ENGINE_ID_2] = SIE_ISR_CB_VDLATCH;
			break;

		case SIE_ENGINE_ID_3:
			pfSie3LatchIntCb = fp;
			pSieIntCbIdx[SIE_ENGINE_ID_3] = SIE_ISR_CB_VDLATCH;
			break;

		case SIE_ENGINE_ID_4:
			pfSie4LatchIntCb = fp;
			pSieIntCbIdx[SIE_ENGINE_ID_4] = SIE_ISR_CB_VDLATCH;
			break;

		case SIE_ENGINE_ID_5:
			pfSie5LatchIntCb = fp;
			pSieIntCbIdx[SIE_ENGINE_ID_5] = SIE_ISR_CB_VDLATCH;
			break;

		default:
			break;
		}
	}
}

void sie_calcBSHScl(SIE_BS_H_INFO *pBshParam, SIE_BS_H_ADJ_INFO *pBshAdjParam)
{
#define BSHCFB   (16)                // bayer scaling caculation fractional bit
#define BSHIFB   (4)                 // bayer scaling implementation fractional bit
#define BSHCU    (1<<BSHCFB)         // bayer scaling caculation unit
#define BSHTB    (BSHCFB-BSHIFB)     // bayer scaling truncation bit
#define BSHTU    (1<<BSHTB)          // bayer scaling truncation unit
#define BSHTM    (~(BSHTU-1))        // bayer scaling truncation mask
#define BSHDMB   (10)                // bayer scaling divM bit
#define BSHDIVS_BASIS   (11)
#define BSHLMX   (100)               // bayer scaling low-pass-filter max-value
#define BSHRM    (16)
#if 1
	UINT32 uiIv, uiSv, uiFv, uiBv1, uiBv2, uiDivM[2], uiDivS[2], uiDivIdx;
	UINT32 uiWtVal, uiWtBit, uiWtTmp;
	UINT32 uiInSz, uiOutSz, uiLpf, uiBinPwr;

	uiInSz  = pBshAdjParam->uiInSz;
	uiOutSz = pBshAdjParam->uiOutSz;
	uiLpf   = pBshAdjParam->uiLpf;
	uiBinPwr = pBshAdjParam->uiBinPwr;

	if (pBshAdjParam->bAdaptiveLpf) {
		if (uiLpf <= 100) {
			uiLpf = 20 + (uiLpf * 60 / 100);
		} else {
			uiLpf = 80;
		}
	}

	if ((uiOutSz * BSHRM) < uiInSz) {
		nvt_dbg(ERR,"Scaling Rate out of range!\r\n");
		return;
	} else if (((uiOutSz - 1)*BSHRM) < (uiInSz - 1)) {
		if (pBshAdjParam->uiLpf < 20) {
			nvt_dbg(WRN,"Scaling Rate near limitation, LPF would be raised to 20\r\n");
			uiLpf = 20;
		}
	}

	//fprintf(stderr,"H) %4d %4d %2d %d;", uiInSz, uiOutSz, uiLpf, uiBinPwr);

	// get Fv
	uiFv = ((BSHCU) << 1) * (uiInSz - 1) / (uiOutSz - 1); //((BSHCU)<<1)*(uiInSz-1)/(uiOutSz-1);
	// get Iv
	uiIv = uiFv * (BSHLMX + uiLpf) / (BSHLMX << 1);
	if (uiIv > ((BSHRM << (1 + BSHCFB)) - 1)) {
		uiIv = ((BSHRM << (1 + BSHCFB)) - 1);
	}
#if 0// no need to limit IV(low-pass) anymore
	uiIvMax = uiFv - (BSHCU);
	if (uiIv > uiIvMax) {
		//fprintf(stderr,"H uiIv = %d --> %d\n", uiIv, uiIvMax);
		//fprintf(stderr,"H LPF = %d --> %d\n", (uiIv-(uiFv>>1))*100/(uiFv>>1), (uiIvMax-(uiFv>>1))*100/(uiFv>>1));
		uiIv = uiIvMax;
	}
#endif
	uiIv = uiIv & BSHTM; //uiIv must be multiple of BSHTU
	// get Sv
	uiSv = uiFv - uiIv;
	while (uiSv >= (BSHRM << BSHCFB)) {
		uiSv -= (BSHTU);
		uiIv += (BSHTU);
	}
	// get Bv
	uiBv2 = (uiFv >> 1) + (BSHCU >> 1) - (uiIv >> 1); //(uiFv>>1)-(BSHCU>>1)-(uiIv>>1);
	uiBv1 = uiBv2 + (uiFv >> 1);

	//fprintf(stderr,"%6x %5x %6x %6x %x;", uiIv, uiSv, uiBv1, uiBv2, 0);

	{
		// only for checking, print out the scaling ratio
		//UINT32 uiTmp, uiTmp2;
		//uiTmp = BSHCU+(BSHCU>>1)+uiBv1+(uiIv>>1);
		//uiTmp2= BSHCU+(BSHCU>>1)+uiBv2+(uiIv>>1);
		//fprintf(stderr,"%2d.%03d %2d.%03d:", uiTmp/BSHCU, (uiTmp%BSHCU)*1000/(BSHCU), uiTmp2/BSHCU, (uiTmp2%BSHCU)*1000/(BSHCU));
	}

	//fprintf(stderr,"\r\n");
	// get DivM/S for normal case
	uiWtVal = uiIv >> BSHTB;
	uiWtBit = 0;
	uiWtTmp = 1 << (uiWtBit);
	while (uiWtTmp < uiWtVal) {
		uiWtBit ++;
		uiWtTmp = 1 << (uiWtBit);
	}
	uiDivIdx = uiWtVal % 2;
	uiDivS[uiDivIdx] = uiWtBit + BSHDMB - 1;
	uiDivM[uiDivIdx] = (1 << uiDivS[uiDivIdx]) / (uiWtVal);
	uiDivS[uiDivIdx] = uiDivS[uiDivIdx] - uiBinPwr;
	//fprintf(stderr,"%4d=%4d,%2d=%4d.%02d\r\n", uiWtVal, uiDivM[uiDivIdx], uiDivS[uiDivIdx], (1<<uiDivS[uiDivIdx])/uiDivM[uiDivIdx], ((100<<uiDivS[uiDivIdx])/uiDivM[uiDivIdx])%100);
	// get DivM/S for the +1 case
	uiWtVal = (uiIv >> BSHTB) + 1;
	uiWtBit = 0;
	uiWtTmp = 1 << (uiWtBit);
	while (uiWtTmp < uiWtVal) {
		uiWtBit ++;
		uiWtTmp = 1 << (uiWtBit);
	}
	uiDivIdx = uiWtVal % 2;
	uiDivS[uiDivIdx] = uiWtBit + BSHDMB - 1;
	uiDivM[uiDivIdx] = (1 << uiDivS[uiDivIdx]) / (uiWtVal);
	uiDivS[uiDivIdx] = uiDivS[uiDivIdx] - uiBinPwr;
	//fprintf(stderr,"%4d=%4d,%2d=%4d.%02d\r\n", uiWtVal, uiDivM[uiDivIdx], uiDivS[uiDivIdx], (1<<uiDivS[uiDivIdx])/uiDivM[uiDivIdx], ((100<<uiDivS[uiDivIdx])/uiDivM[uiDivIdx])%100);
	if (uiDivS[0] < BSHDIVS_BASIS || uiDivS[0] > (BSHDIVS_BASIS + 7)) {
		//fprintf(stderr,"ERROR, uiDivS out of range !!!\r\nERROR, uiDivS out of range !!!\r\nERROR, uiDivS out of range !!!\r\n");
		return;
	}
	if (uiDivS[1] < BSHDIVS_BASIS || uiDivS[1] > (BSHDIVS_BASIS + 7)) {
		//fprintf(stderr,"ERROR, uiDivS out of range !!!\r\nERROR, uiDivS out of range !!!\r\nERROR, uiDivS out of range !!!\r\n");
		return;
	}

	pBshParam->bSrcIntpV   = (uiInSz <= 3264) ? (1) : (0);
	pBshParam->uiIv        =    uiIv;
	pBshParam->uiSv        =    uiSv;
	pBshParam->uiBvR       =    uiBv1;
	pBshParam->uiBvB       =    uiBv2;
	pBshParam->uiOutSize   =    uiOutSz;
	pBshParam->uiDivM[0]   =    uiDivM[0];
	pBshParam->uiDivS[0]   =    uiDivS[0] - BSHDIVS_BASIS;
	pBshParam->uiDivM[1]   =    uiDivM[1];
	pBshParam->uiDivS[1]   =    uiDivS[1] - BSHDIVS_BASIS;
#endif
}


void sie_calcBSVScl(SIE_BS_V_INFO *pBsvParam, SIE_BS_V_ADJ_INFO *pBsvAdjParam)
{
#define BSVCFB   (16)                // bayer scaling caculation fractional bit
#define BSVIFB   (4)                 // bayer scaling implementation fractional bit
#define BSVCU    (1<<BSVCFB)         // bayer scaling caculation unit
#define BSVTB    (BSVCFB-BSVIFB)     // bayer scaling truncation bit
#define BSVTU    (1<<BSVTB)          // bayer scaling truncation unit
#define BSVTM    (~(BSVTU-1))        // bayer scaling truncation mask
#define BSVDMB   (10)                // bayer scaling divM bit
#define BSVDIVS_BASIS   (11)
#define BSVDIVS_BASIS_RB   (7)
#define BSVLMX   (100)               // bayer scaling low-pass-filter max-value
#define BSVIU    (1<<BSVIFB)         // bayer scaling implementation unit
#define BSVRM    (8)
#if 1
	UINT32 uiIv, uiSv, uiFv, uiBv1, uiBv2, uiDivM[2], uiDivS[2], uiDivIdx;
	UINT32 uiWtVal, uiWtBit, uiWtTmp;
	UINT32 uiWtValMin, /*uiWtValMax, */ uiWtValIdx;
	UINT32 uiDivMRB[BSVIU], uiDivSRB[BSVIU];
	UINT32 uiInShft;
	UINT32 uiInSz, uiOutSz, uiLpf, uiBinPwr;

	uiInSz  = pBsvAdjParam->uiInSz;
	uiOutSz = pBsvAdjParam->uiOutSz;
	uiLpf   = pBsvAdjParam->uiLpf;
	uiBinPwr = pBsvAdjParam->uiBinPwr;

	if (pBsvAdjParam->bAdaptiveLpf) {
		if (uiLpf <= 100) {
			uiLpf = 20 + (uiLpf * 60 / 100);
		} else {
			uiLpf = 80;
		}
	}

	if ((uiOutSz * BSVRM) < uiInSz) {
		nvt_dbg(ERR,"Scaling Rate out of range!\r\n");
		return;
	} else if (((uiOutSz - 1)*BSVRM) < (uiInSz - 1)) {
		if (pBsvAdjParam->uiLpf < 20) {
			nvt_dbg(WRN,"Scaling Rate near limitation, LPF would be raised to 20\r\n");
			uiLpf = 20;
		}
	}

	//fprintf(stderr,"V) %4d %4d %2d %d;", uiInSz, uiOutSz, uiLpf, uiBinPwr);

	// get Fv
	if (1) { //(pBsvAdjParam->uiBsHOutSz&0x3F)
		// temporal patch for "BS+BCC last line muting"
		//DBG_ERR("IN\r\n");
		uiFv = ((BSVCU) << 1) * (uiInSz - 2) / (uiOutSz - 1); //((BSVCU)<<1)*(uiInSz-1)/(uiOutSz-1);
	} else {
		uiFv = ((BSVCU) << 1) * (uiInSz - 1) / (uiOutSz - 1); //((BSVCU)<<1)*(uiInSz-1)/(uiOutSz-1);
	}
	// get Iv
	uiIv = uiFv * (BSVLMX + uiLpf) / (BSVLMX << 1);
	if (uiIv > ((BSVRM << (1 + BSVCFB)) - 1)) {
		uiIv = ((BSVRM << (1 + BSVCFB)) - 1);
	}
#if 0// no need to limit IV(low-pass) anymore
	uiIvMax = uiFv - (BSVCU);
	if (uiIv > uiIvMax) {
		//fprintf(stderr,"V uiIv = %d --> %d\n", uiIv, uiIvMax);
		//fprintf(stderr,"V LPF = %d --> %d\n", (uiIv-(uiFv>>1))*100/(uiFv>>1), (uiIvMax-(uiFv>>1))*100/(uiFv>>1));
		uiIv = uiIvMax;
	}
#endif
	uiIv = uiIv & BSVTM; //uiIv must be multiple of BSVTU
	// get InShift & refine Iv
	uiWtVal = uiIv >> BSVTB;
	uiWtBit = 0;
	uiWtTmp = 1 << (uiWtBit);
	while (uiWtTmp < uiWtVal) {
		uiWtBit ++;
		uiWtTmp = 1 << (uiWtBit);
	}
	uiInShft = (uiWtBit > 6) ? (uiWtBit - 6) : (0);
	uiIv = uiIv & (BSVTM << uiInShft); //uiIv must be multiple of (BSVTU<<uiInShft)
	// refine Iv for RB's sake
	if ((uiIv % ((UINT32)BSVCU << uiInShft)) == 0 && uiIv > ((UINT32)BSVCU << uiInShft)) {
		if (((uiIv / (BSVCU << uiInShft)) % 2) == 1) {
			uiIv = uiIv - (BSVTU << uiInShft); //uiIv must be multiple of (BSVTU<<uiInShft)
		}
	}
	// refine Iv for DivM/S[1]'s sake
	if (uiIv == ((UINT32)1 << (2 + BSVCFB + uiInShft))) {
		uiIv = uiIv - (BSVTU << uiInShft); //uiIv must be multiple of (BSVTU<<uiInShft)
	}
	// get Sv
	uiSv = uiFv - uiIv;
	while (uiSv >= ((UINT32)BSVRM << (BSVCFB + uiInShft))) {
		uiSv -= (BSVTU << uiInShft);
		uiIv += (BSVTU << uiInShft);
	}
	// get Bv
	uiBv2 = (uiFv >> 1) + (BSVCU >> 1) - (uiIv >> 1); //(uiFv>>1)-(BSVCU>>1)-(uiIv>>1);
	uiBv1 = uiBv2 + (uiFv >> 1);

	//fprintf(stderr,"%6x %5x %6x %6x %x;", uiIv, uiSv, uiBv1, uiBv2, uiInShft);

	{
		// only for checking, print out the scaling ratio
		//UINT32 uiTmp, uiTmp2;
		//uiTmp = BSVCU+(BSVCU>>1)+uiBv1+(uiIv>>1);
		//uiTmp2= BSVCU+(BSVCU>>1)+uiBv2+(uiIv>>1);
		//fprintf(stderr,"%2d.%03d %2d.%03d:", uiTmp/BSVCU, (uiTmp%BSVCU)*1000/(BSVCU), uiTmp2/BSVCU, (uiTmp2%BSVCU)*1000/(BSVCU));
	}


	//fprintf(stderr,"\r\n");
	// get DivM/S for normal case
	uiWtVal = uiIv >> (BSVTB + uiInShft);
	uiWtBit = 0;
	uiWtTmp = 1 << (uiWtBit);
	while (uiWtTmp < uiWtVal) {
		uiWtBit ++;
		uiWtTmp = 1 << (uiWtBit);
	}
	uiDivIdx = uiWtVal % 2;
	uiDivS[uiDivIdx] = uiWtBit + BSVDMB - 1;
	uiDivM[uiDivIdx] = (1 << uiDivS[uiDivIdx]) / (uiWtVal);
	uiDivS[uiDivIdx] = uiDivS[uiDivIdx] - uiBinPwr;
	//fprintf(stderr,"%4d=%4d,%2d=%4d.%02d\r\n", uiWtVal, uiDivM[uiDivIdx], uiDivS[uiDivIdx], (1<<uiDivS[uiDivIdx])/uiDivM[uiDivIdx], ((100<<uiDivS[uiDivIdx])/uiDivM[uiDivIdx])%100);
	// get DivM/S for the +1 case
	uiWtVal = (uiIv >> (BSVTB + uiInShft)) + 1;
	uiWtBit = 0;
	uiWtTmp = 1 << (uiWtBit);
	while (uiWtTmp < uiWtVal) {
		uiWtBit ++;
		uiWtTmp = 1 << (uiWtBit);
	}
	uiDivIdx = uiWtVal % 2;
	uiDivS[uiDivIdx] = uiWtBit + BSVDMB - 1;
	uiDivM[uiDivIdx] = (1 << uiDivS[uiDivIdx]) / (uiWtVal);
	uiDivS[uiDivIdx] = uiDivS[uiDivIdx] - uiBinPwr;
	//fprintf(stderr,"%4d=%4d,%2d=%4d.%02d\r\n", uiWtVal, uiDivM[uiDivIdx], uiDivS[uiDivIdx], (1<<uiDivS[uiDivIdx])/uiDivM[uiDivIdx], ((100<<uiDivS[uiDivIdx])/uiDivM[uiDivIdx])%100);
	if (uiDivS[0] < BSVDIVS_BASIS || uiDivS[0] > (BSVDIVS_BASIS + 4)) {
		//fprintf(stderr,"ERROR, uiDivS out of range !!!\r\nERROR, uiDivS out of range !!!\r\nERROR, uiDivS out of range !!!\r\n");
		return;
	}
	if (uiDivS[1] < BSVDIVS_BASIS || uiDivS[1] > (BSVDIVS_BASIS + 4)) {
		//fprintf(stderr,"ERROR, uiDivS out of range !!!\r\nERROR, uiDivS out of range !!!\r\nERROR, uiDivS out of range !!!\r\n");
		return;
	}


	pBsvParam->uiIv        =    uiIv;
	pBsvParam->uiSv        =    uiSv;
	pBsvParam->uiBvR       =    uiBv1;
	pBsvParam->uiBvB       =    uiBv2;
	pBsvParam->uiOutSize   =    uiOutSz;
	pBsvParam->uiDivM[0]   =    uiDivM[0];
	pBsvParam->uiDivS[0]   =    uiDivS[0] - BSVDIVS_BASIS;
	pBsvParam->uiDivM[1]   =    uiDivM[1];
	pBsvParam->uiDivS[1]   =    uiDivS[1] - BSVDIVS_BASIS;
	pBsvParam->uiInShft    =    uiInShft;


	{
		// get the DivM/S for the partial-integration R and B
		UINT32 uiCnt, uiRest, i, iUnit;
		uiWtVal = uiIv >> (BSVTB + uiInShft);
		iUnit = BSVIU >> uiInShft;
		uiCnt = uiWtVal / iUnit;
		uiRest = uiWtVal % iUnit;
		if (uiCnt % 2 == 0) {
			uiWtValMin = (uiCnt / 2) * iUnit;
			//uiWtValMax = (uiCnt/2)*iUnit + uiRest;
		} else {
			uiWtValMin = (uiCnt / 2) * iUnit + uiRest;
			//uiWtValMax = (uiCnt/2)*iUnit + iUnit;
			if (uiRest == 0 && iUnit >= 16) {
				//fprintf(stderr,"ERROR, 16 sets would not be enought !!!\r\nERROR, 16 sets would not be enought !!!\r\nERROR, 16 sets would not be enought !!!\r\n");
				if (uiWtVal > iUnit) {
					//fprintf(stderr,"weight(IV) should be adjsuted\r\n");
				} else {
					//fprintf(stderr,"weight(IV) should be increased\r\n");
				}
				return;
			}
		}
		//fprintf(stderr,"from %3d to %3d\r\n", uiWtValMin, uiWtValMax);
		if (uiWtValMin == 0) {
			//fprintf(stderr,"WARNING: Vertical IV might be zero, source-interpolation must be ON !!!\r\n");
			for (i = 0; i < BSVIU; i++) {
				uiDivSRB[i] = BSVDIVS_BASIS_RB;
				uiDivMRB[i] =  0;
				pBsvParam->uiDivMRb[i] =    uiDivMRB[i];
				pBsvParam->uiDivSRb[i] =    uiDivSRB[i] - BSVDIVS_BASIS_RB;
			}
		} else {
			for (i = 0; i < BSVIU; i++) {
				uiWtValIdx = (BSVIU * ((uiWtValMin + BSVIU - 1 - i) / BSVIU)) + i;
				//fprintf(stderr,"%3d ", uiWtValIdx);

				// get DivM/S
				uiWtVal = uiWtValIdx;
				uiWtBit = 0;
				uiWtTmp = 1 << (uiWtBit);
				while (uiWtTmp < uiWtVal) {
					uiWtBit ++;
					uiWtTmp = 1 << (uiWtBit);
				}
				uiDivSRB[i] = uiWtBit + BSVDMB - 1;
				uiDivMRB[i] = (1 << uiDivSRB[i]) / (uiWtVal);
				uiDivSRB[i] = uiDivSRB[i] - uiBinPwr;

#if 0 // Pin: this is for BS broken line issue, don't clear M & S value for R/B
				if (uiWtValIdx > uiWtValMax) {
					uiDivSRB[i] = BSVDIVS_BASIS_RB;
					uiDivMRB[i] =  0;
				}
#endif

				if (uiDivSRB[i] < BSVDIVS_BASIS_RB || uiDivSRB[i] > (BSVDIVS_BASIS_RB + 10)) {
					//fprintf(stderr,"ERROR, uiDivSRB out of range !!!\r\nERROR, uiDivSRB out of range !!!\r\nERROR, uiDivSRB out of range !!!\r\n");
					return;
				}

				if (uiDivMRB[i] != 0) {
					//fprintf(stderr,"%4d=%4d,%2d=%4d.%02d\r\n", uiWtVal, uiDivMRB[i], uiDivSRB[i], (1<<uiDivSRB[i])/uiDivMRB[i], ((100<<uiDivSRB[i])/uiDivMRB[i])%100);
				} else {
					//fprintf(stderr,"%4d=%4d,%2d=----.--\r\n", uiWtValIdx, uiDivMRB[i], uiDivSRB[i]);
				}

				pBsvParam->uiDivMRb[i] =    uiDivMRB[i];
				pBsvParam->uiDivSRb[i] =    uiDivSRB[i] - BSVDIVS_BASIS_RB;
			}
		}
	}
#endif
}

void sie_lib_get_bp(UINT32 id, UINT32 *bp1, UINT32 *bp2)
{
	if (id < SIE_ENGINE_ID_MAX) {
		*bp1 = sie_lib_bp1[id];
		*bp2 = sie_lib_bp2[id];
	}
}

void sie_lib_calc_row_time(UINT32 id, UINT32 calc_start_line, UINT32 calc_end_line)
{
	if (id < SIE_ENGINE_ID_MAX) {
		sie_lib_bp1[id] = calc_start_line;
		sie_lib_bp2[id] = calc_end_line;
	}
}

#if 0 // unused API or function
void sie_calcYoutInfo(SIE_YOUT_GRP_INFO *pSzGrpInfo, SIE_YOUT_ADJ_INFO *pSzAdjInfo)
{
	UINT32 uiInSzH, uiInSzV;
	UINT32 uiScaOutSzX, uiScaOutSzY;
	UINT32 uiScFcH, uiScFcV;
	UINT32 uiWinX, uiWinY, uiWinSzX, uiWinSzY;
	UINT32 uiTotalPxl, uiTmp, uiBit = 0, uiShift, uiGain;
	UINT32 i;

	uiInSzH = pSzAdjInfo->uiCropWinSzX & 0xfffffffc;
	uiInSzV = pSzAdjInfo->uiCropWinSzY & 0xfffffffe;
	if (uiInSzH == 0 || uiInSzV == 0) {
		nvt_dbg(ERR,"Crop Sz ERR!\r\n");
	}

	//if (pSzAdjInfo->uiYoutWinNmX < 32 || pSzAdjInfo->uiYoutWinNmY < 32) {
	//	DBG_ERR("Win Num < 32!\r\n");
	//}

	uiWinX = pSzAdjInfo->uiYoutWinNmX;
	uiWinY = pSzAdjInfo->uiYoutWinNmY;

	if (uiWinX == 0) {
		uiWinX = 1;
		nvt_dbg(ERR, "Win NumX ==0, force to 1\r\n");
	}

	if (uiWinY == 0) {
		uiWinY = 1;
		nvt_dbg(ERR, "Win NumY ==0, force to 1\r\n");
	}


	uiWinSzX = uiInSzH / uiWinX;
	uiWinSzY = uiInSzV / uiWinY;

	uiScaOutSzX = uiWinX * uiWinSzX;
	uiScaOutSzY = uiWinY * uiWinSzY;

	uiScFcH = (((uiInSzH - 1) << 16) / (uiScaOutSzX - 1)) - 65536;
	uiScFcV = (((uiInSzV - 1) << 16) / (uiScaOutSzY - 1)) - 65536;

	if (uiScFcH > 0xffff) {
		nvt_dbg(ERR,"uiScFcH = 0x%x out of range\r\n", (int)uiScFcH);
	}
	if (uiScFcV > 0xffff) {
		nvt_dbg(ERR,"uiScFcV = 0x%x out of range\r\n", (int)uiScFcV);
	}

	pSzGrpInfo->YoutScalInfo.uiFctrX = uiScFcH;
	pSzGrpInfo->YoutScalInfo.uiFctrY = uiScFcV;


	pSzGrpInfo->YoutWinInfo.uiWinNumX = uiWinX;
	pSzGrpInfo->YoutWinInfo.uiWinNumY = uiWinY;
	pSzGrpInfo->YoutWinInfo.uiWinSzX = uiWinSzX;
	pSzGrpInfo->YoutWinInfo.uiWinSzY = uiWinSzY;

	uiTotalPxl = uiWinSzX * uiWinSzY;

	uiTmp = uiTotalPxl;
	for (i = 0; i < 32; i++) {
		if (uiTmp == 0) {
			break;
		} else {
			uiTmp = uiTmp >> 1;
			uiBit++;
		}
	}

	uiShift = (uiBit + 12) - 16;
	if (uiShift > 16)
	{
		nvt_dbg(ERR, "Shift %d > 16, out of range, force to 16\r\n", uiShift);
		uiShift = 16;
	}

	if (uiTotalPxl == 0) {
		nvt_dbg(ERR, "Yout total pixel == 0, force to 1\r\n");
		uiTotalPxl = 1;
		uiShift = 0;
	}

	// gain = 1024/((WinSzX*WinSzY)>>shift)
	uiGain = 1024 / (uiTotalPxl >> uiShift);

	if (uiGain > 255) {
		nvt_dbg(ERR,"Gain over range, set to 0xFF\r\n");
		uiGain = 255;
	}

	pSzGrpInfo->YoutAccmInfo.uiAccmShft = uiShift;
	pSzGrpInfo->YoutAccmInfo.uiAccmGain = uiGain;



#if 0
	DBG_ERR("uiWinX = %d\r\n", uiWinX);
	DBG_ERR("uiWinY = %d\r\n", uiWinY);
	DBG_ERR("uiWinSzX = %d\r\n", uiWinSzX);
	DBG_ERR("uiWinSzY = %d\r\n", uiWinSzY);
	DBG_ERR("uiScFcH = %d\r\n", uiScFcH);
	DBG_ERR("uiScFcV = %d\r\n", uiScFcV);
	DBG_ERR("uiScaOutSzX = %d\r\n", uiScaOutSzX);
	DBG_ERR("uiScaOutSzY = %d\r\n", uiScaOutSzY);
	DBG_ERR("uiShift = %d\r\n", uiShift);
	DBG_ERR("uiGain = %d\r\n", uiGain);
	DBG_ERR("uiBit = %d\r\n", uiBit);
#endif

}

void sie_calcObPlnScl(SIE_OB_PLANE_INFO *pObpParam, SIE_OB_PLANE_ADJ_INFO *pObpAdjParam)
{
#define SCOS_HFRAC_MAX ((1<<14)-1)
#define SCOS_VFRAC_MAX ((1<<14)-1)
	UINT32 uiHin, uiVin, uiHout, uiVout;
	UINT32 uiHfrac, uiVfrac;

	uiHin = uiVin = 17;

	uiHout = pObpAdjParam->uiActSzX;
	uiVout = pObpAdjParam->uiActSzY;
	if (uiHout <= 1 || uiVout <= 1) {
		nvt_dbg(ERR,"Act Sz ERR!\r\n");
		return;
	}


	uiHfrac = ((uiHin - 1) * 65536) / (uiHout - 1);
	if (uiHfrac > SCOS_HFRAC_MAX) {
		nvt_dbg(WRN,"OBP Hscale error!\r\n");
		uiHfrac = SCOS_HFRAC_MAX;
	}

	uiVfrac = ((uiVin - 1) * 65536) / (uiVout - 1);
	if (uiVfrac > SCOS_VFRAC_MAX) {
		nvt_dbg(WRN,"OBP Vscale error!\r\n");
		uiVfrac = SCOS_VFRAC_MAX;
	}

	pObpParam->uiHSclFctr = uiHfrac;
	pObpParam->uiVSclFctr = uiVfrac;
}
#endif
#if (SIE_CLK_API==1)
ER sie1_SIE_CLK_openClock(void)
{
	//linuxpll_enableClock(SIE_CLK);
	//SIE_CLK APIs do not change state//erReturn = sie1_stateMachine(SIE_CLKOP_OPEN,TRUE);
	return E_OK;
}

ER sie1_SIE_CLK_closeClock(void)
{
	//linuxpll_disableClock(SIE_CLK);
	//SIE_CLK APIs do not change state//erReturn = sie1_stateMachine(SIE_CLKOP_CLOSE,TRUE);
	return E_OK;
}

BOOL sie1_SIE_CLK_isClockOpened(void)
{
	return 0;//linuxpll_isClockEnabled(SIE_CLK);
}

ER sie1_SIE_CLK_chgClockSource(SIE_CLKSRC_SEL ClkSrc)
{
	if (ClkSrc == SIE_CLKSRC_CURR) {
	} else if (ClkSrc == SIE_CLKSRC_480) {
		//linuxpll_setClockRate(PLL_CLKSEL_SIE_CLKSRC, PLL_CLKSEL_SIE_CLKSRC_480);
	} else if (ClkSrc == SIE_CLKSRC_PLL5) {
		//linuxpll_setClockRate(PLL_CLKSEL_SIE_CLKSRC, PLL_CLKSEL_SIE_CLKSRC_PLL5);
	} else if (ClkSrc == SIE_CLKSRC_PLL13) {
		//linuxpll_setClockRate(PLL_CLKSEL_SIE_CLKSRC, PLL_CLKSEL_SIE_CLKSRC_PLL6);
	} else if (ClkSrc == SIE_CLKSRC_PLL12) {
		//linuxpll_setClockRate(PLL_CLKSEL_SIE_CLKSRC, PLL_CLKSEL_SIE_CLKSRC_PLL7);
	} else {
		//linuxpll_setClockRate(PLL_CLKSEL_SIE_CLKSRC, PLL_CLKSEL_SIE_CLKSRC_480);
	}
	//SIE_CLK APIs do not change state//erReturn = sie1_stateMachine(SIE_CLKOP_CHG,TRUE);
	return E_OK;
}

ER sie1_SIE_CLK_chgClockRate(UINT32 uiClkRate)
{
	//linux pll_setClockFreq(SIECLK_FREQ, uiClkRate);
	//SIE_CLK APIs do not change state//erReturn = sie1_stateMachine(SIE_CLKOP_CHG,TRUE);
	return E_OK;
}
#endif// (SIE_CLK_API==1)


#ifdef __KERNEL__
//for kdrv
EXPORT_SYMBOL(sie_attach);
EXPORT_SYMBOL(sie_detach);
EXPORT_SYMBOL(sie_open);
EXPORT_SYMBOL(sie_start);
EXPORT_SYMBOL(sie_pause);
EXPORT_SYMBOL(sie_close);
EXPORT_SYMBOL(sie_chgFuncEn);
EXPORT_SYMBOL(sie_chgParam);
EXPORT_SYMBOL(sie_waitEvent);
EXPORT_SYMBOL(sie_reg_isr_cb);
EXPORT_SYMBOL(sie_reg_vdlatch_isr_cb);
EXPORT_SYMBOL(sie_get_la_rslt);
EXPORT_SYMBOL(sie_get_ca_rslt);
EXPORT_SYMBOL(sie_calc_ir_level);
EXPORT_SYMBOL(sie_get_ir_level);
EXPORT_SYMBOL(sie_get_mdrslt);
EXPORT_SYMBOL(sie_get_histo);
EXPORT_SYMBOL(sie_get_sat_gain_info);
EXPORT_SYMBOL(sie_get_dram_single_out);
EXPORT_SYMBOL(sie_get_sys_debug_info);
EXPORT_SYMBOL(sie_set_checksum_en);
//sie drv internal using
EXPORT_SYMBOL(sie_isOpened);
EXPORT_SYMBOL(sie_isr);
EXPORT_SYMBOL(sie_chgBurstLengthByMode);
EXPORT_SYMBOL(sie_getLAGamma);
EXPORT_SYMBOL(sie_getClockRate);
EXPORT_SYMBOL(sie_lib_calc_row_time);

#endif
//@}
