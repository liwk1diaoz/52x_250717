/*
    MDBC module driver

    NT96520 MDBC driver.

    @file       MDBC.c
    @ingroup    mIDrvIPP_MDBC

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#include    "mdbc_lib.h"
#include    "mdbc_reg.h"
#include    "mdbc_int.h"
#include    "md_platform.h"

#define DRV_SUPPORT_IST 0
/**
    @addtogroup mIDrvIPP_MDBC
*/
//@{
static UINT32 guiMdbcStatus = MDBC_STATUS_IDLE;
static ID gMdbcLockStatus = NO_TASK_LOCKED;
static BOOL g_mdbc_clk_en          = FALSE;


#if 0//(DRV_SUPPORT_IST == ENABLE)

#else
static void (*pMDBCIntCB)(UINT32 intstatus);
#endif

//ISR
void mdbc_isr(void);
//control
static ER mdbc_chkStateMachine(UINT32 Operation, BOOL Update);
static void mdbc_swRst(BOOL bReset);
static void mdbc_enableInt(BOOL bEnable, UINT32 uiIntr);
static ER   mdbc_lock(void);
static ER   mdbc_unlock(void);
static void mdbc_attach(void);
static void mdbc_detach(void);
static void mdbc_enable(BOOL bStart);
static void mdbc_ll_enable(BOOL bStart);
static void mdbc_ll_terminate(BOOL isterminate);
static void mdbc_setMdbcMode(MDBC_MODE mode);
//input/output
static void mdbc_setDmaInDataAddr(UINT32 uiSai0, UINT32 uiSai1, UINT32 uiSai2,UINT32 uiSai3, UINT32 uiSai4, UINT32 uiSai5);
static void mdbc_set_dmain_lladdr(UINT32 uiSaiLL);
static void mdbc_setDmaInOffset(UINT32 uiOfsi0,UINT32 uiOfsi1);
static void mdbc_setDmaOutAddr(UINT32 uiSao0,UINT32 uiSao1,UINT32 uiSao2,UINT32 uiSao3);
static void mdbc_setInputSize(MDBC_IN_SIZE *mdbcInSize);
//set parameters
static void mdbc_setMdbcControl(MDBC_CONTROL_EN controlEn);
static void mdbc_setMdmatchParam(MDBC_MDMATCH_PARAM *pMdmatchParam);
static void mdbc_setMorphParam(MDBC_MOR_PARAM *pMorphParam);
static void mdbc_setUpdateParam(MDBC_UPD_PARAM *pUpdateParam);
static void mdbc_setROI0Param(MDBC_ROI_PARAM *pROIParam);
static void mdbc_setROI1Param(MDBC_ROI_PARAM *pROIParam);
static void mdbc_setROI2Param(MDBC_ROI_PARAM *pROIParam);
static void mdbc_setROI3Param(MDBC_ROI_PARAM *pROIParam);
static void mdbc_setROI4Param(MDBC_ROI_PARAM *pROIParam);
static void mdbc_setROI5Param(MDBC_ROI_PARAM *pROIParam);
static void mdbc_setROI6Param(MDBC_ROI_PARAM *pROIParam);
static void mdbc_setROI7Param(MDBC_ROI_PARAM *pROIParam);
//get info

//for FPGA verification only
/*
#ifdef __KERNEL__

VOID mdbc_setBaseAddr(UINT32 uiAddr)
{
	_MDBC_REG_BASE_ADDR = uiAddr;
}

VOID mdbc_create_resource(VOID)
{
	OS_CONFIG_FLAG(FLG_ID_MDBC);
	SEM_CREATE(SEMID_MDBC, 1);
}

VOID mdbc_release_resource(VOID)
{
	rel_flg(FLG_ID_MDBC);
	SEM_DESTROY(SEMID_MDBC);
}
#endif
*/
/*
    MDBC State Machine

    MDBC state machine

    @param[in] Operation the operation to execute
    @param[in] Update if we will update state machine status

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER mdbc_chkStateMachine(UINT32 Operation, BOOL Update)
{
	ER ER_Code;

	switch (guiMdbcStatus) {
	case MDBC_STATUS_IDLE:
		switch (Operation) {
		case MDBC_OP_ATTACH:
			if (Update) {
				guiMdbcStatus = MDBC_STATUS_IDLE;
			}
			ER_Code = E_OK;
			break;
		case MDBC_OP_DETACH:
			if (Update) {
				guiMdbcStatus = MDBC_STATUS_IDLE;
			}
			ER_Code = E_OK;
			break;
		case MDBC_OP_OPEN:
			if (Update) {
				guiMdbcStatus = MDBC_STATUS_READY;
			}
			ER_Code = E_OK;
			break;
		default:
			ER_Code = E_OBJ;
			break;
		}
		break;

	case MDBC_STATUS_READY:
		switch (Operation) {
		case MDBC_OP_CLOSE:
			if (Update) {
				guiMdbcStatus = MDBC_STATUS_IDLE;
			}
			ER_Code = E_OK;
			break;
		case MDBC_OP_SETMODE:
			if (Update) {
				guiMdbcStatus = MDBC_STATUS_PAUSE;    //PAUSE first, and set MDBC_OP_RUN to start
			}
			ER_Code = E_OK;
			break;
		case MDBC_OP_PAUSE:
			if (Update) {
				guiMdbcStatus = MDBC_STATUS_PAUSE;
			}
			ER_Code = E_OK;
			break;
		default:
			ER_Code = E_OBJ;
			break;
		}
		break;

	case MDBC_STATUS_PAUSE:
		switch (Operation) {
		case MDBC_OP_SETMODE:
			if (Update) {
				guiMdbcStatus = MDBC_STATUS_PAUSE;
			}
			ER_Code = E_OK;
			break;
		case MDBC_OP_RUN:
			if (Update) {
				guiMdbcStatus = MDBC_STATUS_RUN;
			}
			ER_Code = E_OK;
			break;
		case MDBC_OP_CLOSE:
			if (Update) {
				guiMdbcStatus = MDBC_STATUS_IDLE;
			}
			ER_Code = E_OK;
			break;
		case MDBC_OP_PAUSE:
			DBG_WRN("Multiple pause operations!\r\n");
			ER_Code = E_OK;
			break;
		default :
			ER_Code = E_OBJ;
			break;
		}
		break;


	case MDBC_STATUS_RUN:
		switch (Operation) {
		case MDBC_OP_PAUSE:
			if (Update) {
				guiMdbcStatus = MDBC_STATUS_PAUSE;
			}
			ER_Code = E_OK;
			break;
		default :
			ER_Code = E_OBJ;
			break;
		}
		break;
	default:
		ER_Code = E_OBJ;
	}
	if (ER_Code != E_OK) {
		DBG_ERR("State machine error! st %d op %d\r\n", guiMdbcStatus, Operation);
	}

	return ER_Code;

}

/**
    Enable/Disable MDBC Interrupt

    MDBC interrupt enable selection.

    @param[in] bEnable Decide selected funtions are to be enabled or disabled.
        \n-@b ENABLE: enable selected functions.
        \n-@b DISABLE: disable selected functions.
    @param[in] uiIntr Indicate the function(s)

    @return None.
*/
static void mdbc_enableInt(BOOL bEnable, UINT32 uiIntr)
{
	T_MDBC_INTERRUPT_ENABLE_REGISTER reg1;
	reg1.reg = MDBC_GETREG(MDBC_INTERRUPT_ENABLE_REGISTER_OFS);

	if (bEnable) {
		reg1.reg |= uiIntr;
	} else {
		reg1.reg &= (~uiIntr);
	}

	MDBC_SETREG(MDBC_INTERRUPT_ENABLE_REGISTER_OFS, reg1.reg);
}

/**
    MDBC Get Interrupt Enable Status

    Get current MDBC interrupt Enable status

    @return MDBC interrupt Enable status.
*/
UINT32 mdbc_getIntEnable(void)
{
	T_MDBC_INTERRUPT_ENABLE_REGISTER reg1;
	reg1.reg = MDBC_GETREG(MDBC_INTERRUPT_ENABLE_REGISTER_OFS);

	return reg1.reg;
}

/**
    MDBC Get Interrupt Status

    Get current MDBC interrupt status

    @return MDBC interrupt status readout.
*/
UINT32 mdbc_getIntrStatus(void)
{
	T_MDBC_INTERRUPT_STATUS_REGISTER reg1;
	reg1.reg = MDBC_GETREG(MDBC_INTERRUPT_STATUS_REGISTER_OFS);

	return reg1.reg;
}

/**
    MDBC Clear Interrupt Status

    Clear MDBC interrupt status.

    @param[in] uiStatus 1's in this word will clear corresponding interrupt status.

    @return None.
*/
void mdbc_clrIntrStatus(UINT32 uiStatus)
{
	T_MDBC_INTERRUPT_STATUS_REGISTER reg1;
	reg1.reg = uiStatus;
	MDBC_SETREG(MDBC_INTERRUPT_STATUS_REGISTER_OFS, reg1.reg);
}

//@}

/**
    MDBC Reset

    Enable/disable MDBC SW reset

    @param[in] bReset
        \n-@b TRUE: enable reset,
        \n-@b FALSE: disable reset.

    @return None.
*/
void mdbc_swRst(BOOL bReset)
{
	T_MDBC_CONTROL_REGISTER reg1;
	reg1.reg = MDBC_GETREG(MDBC_CONTROL_REGISTER_OFS);

	reg1.bit.MDBC_SW_RST = bReset;

	MDBC_SETREG(MDBC_CONTROL_REGISTER_OFS, reg1.reg);
}

/**
    MDBC Enable

    Trigger MDBC HW start

    @param[in] bStart
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
void mdbc_enable(BOOL bStart)
{
	T_MDBC_CONTROL_REGISTER reg1;
	reg1.reg = MDBC_GETREG(MDBC_CONTROL_REGISTER_OFS);

	reg1.bit.MDBC_START = bStart;

	MDBC_SETREG(MDBC_CONTROL_REGISTER_OFS, reg1.reg);
}

void mdbc_ll_enable(BOOL bStart)
{
	T_MDBC_CONTROL_REGISTER reg1;
	reg1.reg = MDBC_GETREG(MDBC_CONTROL_REGISTER_OFS);
	reg1.bit.LL_FIRE = bStart;

	MDBC_SETREG(MDBC_CONTROL_REGISTER_OFS, reg1.reg);
}

void mdbc_ll_terminate(BOOL isterminate)
{
	T_LL_CONTROL_REGISTER reg1;
	reg1.reg = MDBC_GETREG(LL_CONTROL_REGISTER_OFS);
	reg1.bit.LL_TERMINATE = isterminate;
	if (isterminate) {
		reg1.bit.LL_TERMINATE = 0;
		reg1.bit.LL_TERMINATE = isterminate;
	}
	MDBC_SETREG(LL_CONTROL_REGISTER_OFS, reg1.reg);
}

/*
    MDBC Interrupt Service Routine

    MDBC interrupt service routine

    @return None.
*/
void mdbc_isr(void)
{
	UINT32 IntStatus;
	IntStatus = mdbc_getIntrStatus();
	IntStatus = IntStatus & mdbc_getIntEnable();

	if (IntStatus == 0) {
		return;
	}
	mdbc_clrIntrStatus(IntStatus);

	//DBG_DUMP("[ISR] IntStatus: 0x%08x\r\n",IntStatus);
	if (IntStatus & MDBC_INTE_LLEND) {
		//iset_flg(FLG_ID_MDBC, FLGPTN_MDBC_LLEND);
		md_platform_flg_set(FLGPTN_MDBC_LLEND);
	}
	if (IntStatus & MDBC_INTE_LLERROR) {
		nvt_dbg(ERR, "Illegal linked list setting\r\n");
		//iset_flg(FLG_ID_MDBC, FLGPTN_MDBC_LLERROR);
		md_platform_flg_set(FLGPTN_MDBC_LLERROR);
	}
	if (IntStatus & MDBC_INTE_FRM) {
		//iset_flg(FLG_ID_MDBC, FLGPTN_MDBC_FRAMEEND);
		md_platform_flg_set(FLGPTN_MDBC_FRAMEEND);
	}

#if 0//(DRV_SUPPORT_IST == ENABLE)
	if (pfIstCB_CNN != NULL) {
		drv_setIstEvent(DRV_IST_MODULE_CNN, IntStatus);
	}
#else
	if (pMDBCIntCB) {
		pMDBCIntCB(IntStatus);
	}
#endif
}

/**
    MDBC Lock Function

    Lock MDBC engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER mdbc_lock(void)
{
	ER erReturn;

	erReturn = md_platform_sem_wait();

	if (erReturn != E_OK) {
		return erReturn;
	}

	gMdbcLockStatus = TASK_LOCKED;

	return E_OK;
}

/**
    MDBC Unlock Function

    Unlock MDBC engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER mdbc_unlock(void)
{
	if (gMdbcLockStatus == TASK_LOCKED) {
		gMdbcLockStatus = NO_TASK_LOCKED;
        md_platform_sem_signal();
		return E_OK;
	} else {
		return E_OBJ;
	}
}

/**
    MDBC Attach Function

    Pre-initialize driver required HW before driver open
    \n This function should be called before calling any other functions

    @return None
*/
static void mdbc_attach(void)
{
	if (g_mdbc_clk_en == FALSE){
        md_platform_enable_clk();
        g_mdbc_clk_en = TRUE;
    }
}

/**
    MDBC Detach Function

    De-initialize driver required HW after driver close
    \n This function should be called before calling any other functions

    @return None
*/
static void mdbc_detach(void)
{
	if (g_mdbc_clk_en == TRUE){
        md_platform_disable_clk();
        g_mdbc_clk_en = FALSE;
    }
}

/**
    MDBC Open Driver

    Open MDBC engine

    @param[in] pObjCB MDBC open object

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER mdbc_open(MDBC_OPENOBJ *pObjCB)
{
	ER erReturn;
	UINT32 clk_freq_mhz;
	// Lock semaphore
	erReturn = mdbc_lock();
	if (erReturn != E_OK) {
		return erReturn;
	}

	// Check state-machine
	erReturn = mdbc_chkStateMachine(MDBC_OP_OPEN, FALSE);
	if (erReturn != E_OK) {
		// release resource when stateMachine fail
		mdbc_unlock();
		return erReturn;
	}

	// Power on module
	//pmc_turnonPower(PMC_MODULE_CNN);

	// Select PLL clock source & enable PLL clock source
	clk_freq_mhz = md_platform_get_clk_rate();
	if (clk_freq_mhz == 0) {
		md_platform_set_clk_rate(360);
	} else {
		md_platform_set_clk_rate(clk_freq_mhz);
	}
	//md_platform_set_clk_rate(pObjCB->uiMdbcClockSel);

	// Attach
	mdbc_attach();

	// Disable Sram shut down
	md_platform_disable_sram_shutdown();

	erReturn = md_platform_flg_clear(FLGPTN_MDBC_FRAMEEND | FLGPTN_MDBC_LLEND | FLGPTN_MDBC_LLERROR);
	if (erReturn != E_OK) {
		return erReturn;
	}

	md_platform_int_enable();

	// Clear engine interrupt status
	mdbc_clrIntrStatus(MDBC_INT_ALL);
    mdbc_enableInt(ENABLE, MDBC_INT_ALL);

	// Hook call-back function
	pMDBCIntCB   = pObjCB->FP_MDBCISR_CB;

	// SW reset
	mdbc_swRst(1);
	mdbc_swRst(0);

	// Update state-machine
	mdbc_chkStateMachine(MDBC_OP_OPEN, TRUE);

	return E_OK;
}

/*
    MDBC Get Open Status

    Check if MDBC engine is opened

    @return
        - @b FALSE: engine is not opened
        - @b TRUE: engine is opened
*/
BOOL mdbc_isOpened(void)
{
	if (gMdbcLockStatus == TASK_LOCKED) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
    MDBC Close Driver

    Close MDBC engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER mdbc_close(void)
{
	ER erReturn;

	// Check state-machine
	erReturn = mdbc_chkStateMachine(MDBC_OP_CLOSE, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	// Check semaphore
	if (gMdbcLockStatus != TASK_LOCKED) {
		return E_SYS;
	}

	md_platform_int_disable();
	md_platform_enable_sram_shutdown();

	// Detach
	mdbc_detach();

#if 0
	// Unhook call-back function
#if (DRV_SUPPORT_IST == ENABLE)
	pfIstCB_CNN = NULL;
#else
	pMDBCIntCB   = NULL;
#endif
#endif

	// Update state-machine
	mdbc_chkStateMachine(MDBC_OP_CLOSE, TRUE);

	// Unlock semaphore
	erReturn = mdbc_unlock();
	if (erReturn != E_OK) {
		return erReturn;
	}

	return E_OK;
}

/**
    MDBC Pause Operation

    Pause MDBC engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER mdbc_pause(void)
{
	ER erReturn;

	erReturn = mdbc_chkStateMachine(MDBC_OP_PAUSE, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	mdbc_enable(DISABLE);

	mdbc_chkStateMachine(MDBC_OP_PAUSE, TRUE);

	return E_OK;
}

ER mdbc_ll_pause(void)
{
	ER erReturn;

	erReturn = mdbc_chkStateMachine(MDBC_OP_PAUSE, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	mdbc_ll_enable(DISABLE);
	mdbc_ll_terminate(DISABLE);

	mdbc_chkStateMachine(MDBC_OP_PAUSE, TRUE);

	return E_OK;
}

/**
    MDBC Start Operation

    Start MDBC engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER mdbc_start(void)
{
	ER erReturn;

	erReturn = mdbc_chkStateMachine(MDBC_OP_RUN, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	md_platform_flg_clear(FLGPTN_MDBC_FRAMEEND);

	erReturn = mdbc_chkStateMachine(MDBC_OP_RUN, TRUE);
    mdbc_enable(ENABLE);
	return erReturn;
}

ER mdbc_ll_start(void)
{
	ER erReturn;

	erReturn = mdbc_chkStateMachine(MDBC_OP_RUN, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}
	// Clear SW flag
	md_platform_flg_clear(FLGPTN_MDBC_FRAMEEND);
	mdbc_clr_ll_frameend();

	erReturn = mdbc_chkStateMachine(MDBC_OP_RUN, TRUE);
	mdbc_ll_enable(ENABLE);

	return erReturn;
}

/**
    MDBC Clear Frame End

    Clear MDBC frame end flag.

    @return None.
*/
void mdbc_clrFrameEnd(void)
{
	//clr_flg(FLG_ID_MDBC, FLGPTN_MDBC_FRAMEEND);
	md_platform_flg_clear(FLGPTN_MDBC_FRAMEEND);
}

/**
    MDBC Wait Frame End

    Wait for MDBC frame end flag.

    @param[in] bClrFlag
        \n-@b TRUE: clear flag and wait for next flag.
        \n-@b FALSE: wait for flag.

    @return None.
*/
void mdbc_waitFrameEnd(BOOL bClrFlag)
{
	FLGPTN uiFlag;
	if (bClrFlag == TRUE) {
		md_platform_flg_clear(FLGPTN_MDBC_FRAMEEND);
		//clr_flg(FLG_ID_MDBC, FLGPTN_MDBC_FRAMEEND);
	}
	md_platform_flg_wait(&uiFlag, FLGPTN_MDBC_FRAMEEND);
	//wai_flg(&uiFlag, FLG_ID_MDBC, FLGPTN_MDBC_FRAMEEND, TWF_CLR | TWF_ORW);
}
//@}

void mdbc_clr_ll_frameend(void)
{
	md_platform_flg_clear(FLGPTN_MDBC_LLEND);
	md_platform_flg_clear(FLGPTN_MDBC_LLERROR);
	//clr_flg(FLG_ID_MDBC, FLGPTN_MDBC_LLEND);
	//clr_flg(FLG_ID_MDBC, FLGPTN_MDBC_LLERROR);
}

void mdbc_wait_ll_frameend(BOOL is_clr_flag)
{
	FLGPTN uiFlag;
	if (is_clr_flag == TRUE) {
		md_platform_flg_clear(FLGPTN_MDBC_LLEND);
		//clr_flg(FLG_ID_MDBC, FLGPTN_MDBC_LLEND);
	}
	md_platform_flg_wait(&uiFlag, FLGPTN_MDBC_LLEND);
	//wai_flg(&uiFlag, FLG_ID_MDBC, FLGPTN_MDBC_LLEND, TWF_CLR | TWF_ORW);
}

/**
    MDBC Set Mode

    Set MDBC engine mode

    @param pDisAddrSize DIS configuration.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER mdbc_setMode(MDBC_PARAM *pMdbcPara)
{
	ER ER_Code;
#if (MDBC_DMA_CACHE_HANDLE == 1)
#if MD_IOREMAP_IN_KERNEL
	UINT32 uiAddr0, uiAddr1, uiAddr2;
#else
	UINT32 uiAddr0, uiAddr1, uiAddr2, uiAddr3, uiAddr4, uiAddr5;//, uiAddr6=0;
#endif
	UINT32 uiPhyAddr0, uiPhyAddr1, uiPhyAddr2, uiPhyAddr3, uiPhyAddr4, uiPhyAddr5;//, uiPhyAddr6;
	UINT32 uiImgYSize,uiImgUVSize;//,uiSize;
#endif

	ER_Code = mdbc_chkStateMachine(MDBC_OP_SETMODE, FALSE);
	if (ER_Code != E_OK) {
		return ER_Code;
	}

	// MdMatch
	if (pMdbcPara->MdmatchPara.lbsp_th >= 256) {
		DBG_ERR("lbsp_th must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.d_colour >= 16) {
		DBG_ERR("d_colour must be < 16! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.r_colour >= 256) {
		DBG_ERR("r_colour must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.d_lbsp >= 16) {
		DBG_ERR("d_lbsp must be < 16! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.r_lbsp >= 16) {
		DBG_ERR("r_lbsp must be < 16! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.model_num % 4) {
		DBG_ERR("Background model number must be 4*N! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.t_alpha >= 256) {
		DBG_ERR("t_alpha must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.dw_shift >= 5) {
		DBG_ERR("dw_shift must be <= 4! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.dlast_alpha >= 1024) {
		DBG_ERR("dlast_alpha must be < 1024! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.min_match > pMdbcPara->MdmatchPara.model_num) {
		DBG_ERR("min_match must be < model_num! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.dlt_alpha >= 1024) {
		DBG_ERR("dlt_alpha must be < 1024! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.dst_alpha >= 1024) {
		DBG_ERR("dst_alpha must be < 1024! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.uv_thres >= 256) {
		DBG_ERR("uv_thres must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.s_alpha >= 1024) {
		DBG_ERR("s_alpha must be < 1024! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MdmatchPara.dbg_lumDiff >= (1<<28)) {
		DBG_ERR("dbg_lumDiff must be < (1<<28)! Abort!\r\n");
		return E_SYS;
	}
	mdbc_setMdmatchParam(&(pMdbcPara->MdmatchPara));

	// Morph
	if (pMdbcPara->MorPara.th_ero > 8 || pMdbcPara->MorPara.th_dil > 8 ) {
		DBG_ERR("Erode/Dilate threshold must <= 8! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->MorPara.mor_sel0 > Bypass || pMdbcPara->MorPara.mor_sel1 > Bypass
	 || pMdbcPara->MorPara.mor_sel2 > Bypass || pMdbcPara->MorPara.mor_sel3 > Bypass) {
		DBG_ERR("mor_sel command error! Abort!\r\n");
		return E_SYS;
	}
	mdbc_setMorphParam(&(pMdbcPara->MorPara));

	// Update
	if (pMdbcPara->UpdPara.maxT >= 256) {
		DBG_ERR("maxT must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->UpdPara.minT > pMdbcPara->UpdPara.maxT) {
		DBG_ERR("minT must be <= maxT! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->UpdPara.maxFgFrm >= 256) {
		DBG_ERR("maxFgFrm must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->UpdPara.deghost_dth > 256) {
		DBG_ERR("deghost_dth must be <= 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->UpdPara.deghost_sth >= 256) {
		DBG_ERR("deghost_sth must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->UpdPara.stable_frm >= 256) {
		DBG_ERR("stable_frm must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->UpdPara.update_dyn >= 256) {
		DBG_ERR("update_dyn must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->UpdPara.va_distth >= 256) {
		DBG_ERR("va_distth must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->UpdPara.t_distth >= 256) {
		DBG_ERR("t_distth must be < 256! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->UpdPara.dbg_frmID >= (1<<8)) {
		DBG_ERR("dbg_frmID must be < (1<<8)! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->UpdPara.dbg_rnd >= (1<<15)) {
		DBG_ERR("dbg_rnd must be < (1<<15)! Abort!\r\n");
		return E_SYS;
	}
	mdbc_setUpdateParam(&(pMdbcPara->UpdPara));

	// ROI0
	if(pMdbcPara->controlEn.roi_en0){
		if (pMdbcPara->ROIPara0.roi_x >= 1024) {
			DBG_ERR("roi_x must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_y >= 1024) {
			DBG_ERR("roi_y must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_w >= 1024) {
			DBG_ERR("roi_w must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_h >= 1024) {
			DBG_ERR("roi_h must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_x >= pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_x should be < width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara0.roi_y >= pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_y should be < height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara0.roi_w > pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_w should be <= width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara0.roi_h > pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_h should be <= height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara0.roi_uv_thres >= 256) {
			DBG_ERR("roi_uv_thres must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_lbsp_th >= 256) {
			DBG_ERR("roi_lbsp_th must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_d_colour >= 16) {
			DBG_ERR("roi_d_colour must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_r_colour >= 256) {
			DBG_ERR("roi_r_colour must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_d_lbsp >= 16) {
			DBG_ERR("roi_d_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_r_lbsp >= 16) {
			DBG_ERR("roi_r_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_maxT >= 256) {
			DBG_ERR("roi_maxT must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara0.roi_minT > pMdbcPara->ROIPara0.roi_maxT) {
			DBG_ERR("roi_minT must be <= roi_maxT! Abort!\r\n");
			return E_SYS;
		}
		mdbc_setROI0Param(&(pMdbcPara->ROIPara0));
	}

	// ROI1
	if(pMdbcPara->controlEn.roi_en1){
		if (pMdbcPara->ROIPara1.roi_x >= 1024) {
			DBG_ERR("roi_x must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_y >= 1024) {
			DBG_ERR("roi_y must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_w >= 1024) {
			DBG_ERR("roi_w must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_h >= 1024) {
			DBG_ERR("roi_h must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_x >= pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_x should be < width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara1.roi_y >= pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_y should be < height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara1.roi_w > pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_w should be <= width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara1.roi_h > pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_h should be <= height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara1.roi_uv_thres >= 256) {
			DBG_ERR("roi_uv_thres must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_lbsp_th >= 256) {
			DBG_ERR("roi_lbsp_th must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_d_colour >= 16) {
			DBG_ERR("roi_d_colour must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_r_colour >= 256) {
			DBG_ERR("roi_r_colour must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_d_lbsp >= 16) {
			DBG_ERR("roi_d_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_r_lbsp >= 16) {
			DBG_ERR("roi_r_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_maxT >= 256) {
			DBG_ERR("roi_maxT must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara1.roi_minT > pMdbcPara->ROIPara1.roi_maxT) {
			DBG_ERR("roi_minT must be <= roi_maxT! Abort!\r\n");
			return E_SYS;
		}
		mdbc_setROI1Param(&(pMdbcPara->ROIPara1));
	}

	// ROI2
	if(pMdbcPara->controlEn.roi_en2){
		if (pMdbcPara->ROIPara2.roi_x >= 1024) {
			DBG_ERR("roi_x must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_y >= 1024) {
			DBG_ERR("roi_y must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_w >= 1024) {
			DBG_ERR("roi_w must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_h >= 1024) {
			DBG_ERR("roi_h must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_x >= pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_x should be < width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara2.roi_y >= pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_y should be < height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara2.roi_w > pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_w should be <= width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara2.roi_h > pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_h should be <= height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara2.roi_uv_thres >= 256) {
			DBG_ERR("roi_uv_thres must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_lbsp_th >= 256) {
			DBG_ERR("roi_lbsp_th must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_d_colour >= 16) {
			DBG_ERR("roi_d_colour must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_r_colour >= 256) {
			DBG_ERR("roi_r_colour must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_d_lbsp >= 16) {
			DBG_ERR("roi_d_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_r_lbsp >= 16) {
			DBG_ERR("roi_r_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_maxT >= 256) {
			DBG_ERR("roi_maxT must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara2.roi_minT > pMdbcPara->ROIPara2.roi_maxT) {
			DBG_ERR("roi_minT must be <= roi_maxT! Abort!\r\n");
			return E_SYS;
		}
		mdbc_setROI2Param(&(pMdbcPara->ROIPara2));
	}

	// ROI3
	if(pMdbcPara->controlEn.roi_en3){
		if (pMdbcPara->ROIPara3.roi_x >= 1024) {
			DBG_ERR("roi_x must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_y >= 1024) {
			DBG_ERR("roi_y must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_w >= 1024) {
			DBG_ERR("roi_w must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_h >= 1024) {
			DBG_ERR("roi_h must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_x >= pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_x should be < width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara3.roi_y >= pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_y should be < height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara3.roi_w > pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_w should be <= width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara3.roi_h > pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_h should be <= height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara3.roi_uv_thres >= 256) {
			DBG_ERR("roi_uv_thres must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_lbsp_th >= 256) {
			DBG_ERR("roi_lbsp_th must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_d_colour >= 16) {
			DBG_ERR("roi_d_colour must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_r_colour >= 256) {
			DBG_ERR("roi_r_colour must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_d_lbsp >= 16) {
			DBG_ERR("roi_d_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_r_lbsp >= 16) {
			DBG_ERR("roi_r_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_maxT >= 256) {
			DBG_ERR("roi_maxT must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara3.roi_minT > pMdbcPara->ROIPara3.roi_maxT) {
			DBG_ERR("roi_minT must be <= roi_maxT! Abort!\r\n");
			return E_SYS;
		}
		mdbc_setROI3Param(&(pMdbcPara->ROIPara3));
	}

	// ROI4
	if(pMdbcPara->controlEn.roi_en4){
		if (pMdbcPara->ROIPara4.roi_x >= 1024) {
			DBG_ERR("roi_x must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_y >= 1024) {
			DBG_ERR("roi_y must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_w >= 1024) {
			DBG_ERR("roi_w must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_h >= 1024) {
			DBG_ERR("roi_h must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_x >= pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_x should be < width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara4.roi_y >= pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_y should be < height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara4.roi_w > pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_w should be <= width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara4.roi_h > pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_h should be <= height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara4.roi_uv_thres >= 256) {
			DBG_ERR("roi_uv_thres must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_lbsp_th >= 256) {
			DBG_ERR("roi_lbsp_th must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_d_colour >= 16) {
			DBG_ERR("roi_d_colour must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_r_colour >= 256) {
			DBG_ERR("roi_r_colour must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_d_lbsp >= 16) {
			DBG_ERR("roi_d_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_r_lbsp >= 16) {
			DBG_ERR("roi_r_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_maxT >= 256) {
			DBG_ERR("roi_maxT must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara4.roi_minT > pMdbcPara->ROIPara4.roi_maxT) {
			DBG_ERR("roi_minT must be <= roi_maxT! Abort!\r\n");
			return E_SYS;
		}
		mdbc_setROI4Param(&(pMdbcPara->ROIPara4));
	}

	// ROI5
	if(pMdbcPara->controlEn.roi_en5){
		if (pMdbcPara->ROIPara5.roi_x >= 1024) {
			DBG_ERR("roi_x must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_y >= 1024) {
			DBG_ERR("roi_y must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_w >= 1024) {
			DBG_ERR("roi_w must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_h >= 1024) {
			DBG_ERR("roi_h must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_x >= pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_x should be < width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara5.roi_y >= pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_y should be < height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara5.roi_w > pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_w should be <= width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara5.roi_h > pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_h should be <= height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara5.roi_uv_thres >= 256) {
			DBG_ERR("roi_uv_thres must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_lbsp_th >= 256) {
			DBG_ERR("roi_lbsp_th must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_d_colour >= 16) {
			DBG_ERR("roi_d_colour must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_r_colour >= 256) {
			DBG_ERR("roi_r_colour must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_d_lbsp >= 16) {
			DBG_ERR("roi_d_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_r_lbsp >= 16) {
			DBG_ERR("roi_r_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_maxT >= 256) {
			DBG_ERR("roi_maxT must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara5.roi_minT > pMdbcPara->ROIPara5.roi_maxT) {
			DBG_ERR("roi_minT must be <= roi_maxT! Abort!\r\n");
			return E_SYS;
		}
		mdbc_setROI5Param(&(pMdbcPara->ROIPara5));
	}

	// ROI6
	if(pMdbcPara->controlEn.roi_en6){
		if (pMdbcPara->ROIPara6.roi_x >= 1024) {
			DBG_ERR("roi_x must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_y >= 1024) {
			DBG_ERR("roi_y must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_w >= 1024) {
			DBG_ERR("roi_w must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_h >= 1024) {
			DBG_ERR("roi_h must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_x >= pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_x should be < width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara6.roi_y >= pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_y should be < height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara6.roi_w > pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_w should be <= width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara6.roi_h > pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_h should be <= height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara6.roi_uv_thres >= 256) {
			DBG_ERR("roi_uv_thres must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_lbsp_th >= 256) {
			DBG_ERR("roi_lbsp_th must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_d_colour >= 16) {
			DBG_ERR("roi_d_colour must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_r_colour >= 256) {
			DBG_ERR("roi_r_colour must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_d_lbsp >= 16) {
			DBG_ERR("roi_d_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_r_lbsp >= 16) {
			DBG_ERR("roi_r_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_maxT >= 256) {
			DBG_ERR("roi_maxT must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara6.roi_minT > pMdbcPara->ROIPara6.roi_maxT) {
			DBG_ERR("roi_minT must be <= roi_maxT! Abort!\r\n");
			return E_SYS;
		}
		mdbc_setROI6Param(&(pMdbcPara->ROIPara6));
	}

	// ROI7
	if(pMdbcPara->controlEn.roi_en7){
		if (pMdbcPara->ROIPara7.roi_x >= 1024) {
			DBG_ERR("roi_x must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_y >= 1024) {
			DBG_ERR("roi_y must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_w >= 1024) {
			DBG_ERR("roi_w must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_h >= 1024) {
			DBG_ERR("roi_h must be < 1024! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_x >= pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_x should be < width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara7.roi_y >= pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_y should be < height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara7.roi_w > pMdbcPara->Size.uiMdbcWidth) {
			DBG_WRN("roi_w should be <= width! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara7.roi_h > pMdbcPara->Size.uiMdbcHeight) {
			DBG_WRN("roi_h should be <= height! Abort!\r\n");
		}
		if (pMdbcPara->ROIPara7.roi_uv_thres >= 256) {
			DBG_ERR("roi_uv_thres must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_lbsp_th >= 256) {
			DBG_ERR("roi_lbsp_th must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_d_colour >= 16) {
			DBG_ERR("roi_d_colour must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_r_colour >= 256) {
			DBG_ERR("roi_r_colour must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_d_lbsp >= 16) {
			DBG_ERR("roi_d_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_r_lbsp >= 16) {
			DBG_ERR("roi_r_lbsp must be < 16! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_maxT >= 256) {
			DBG_ERR("roi_maxT must be < 256! Abort!\r\n");
			return E_SYS;
		}
		if (pMdbcPara->ROIPara7.roi_minT > pMdbcPara->ROIPara7.roi_maxT) {
			DBG_ERR("roi_minT must be <= roi_maxT! Abort!\r\n");
			return E_SYS;
		}
		mdbc_setROI7Param(&(pMdbcPara->ROIPara7));
	}


#if (MDBC_DMA_CACHE_HANDLE == 1)
	// Input DMA/Cache Handle
	uiImgYSize = pMdbcPara->InInfo.uiLofs0 * pMdbcPara->Size.uiMdbcHeight;
	uiImgUVSize = pMdbcPara->InInfo.uiLofs1 * (pMdbcPara->Size.uiMdbcHeight>>1);
	//uiSize = pMdbcPara->Size.uiMdbcWidth * pMdbcPara->Size.uiMdbcHeight;
#if MD_IOREMAP_IN_KERNEL
	uiAddr0 = md_platform_pa2va_remap(pMdbcPara->InInfo.uiInAddr0,uiImgYSize);
	uiAddr1 = md_platform_pa2va_remap(pMdbcPara->InInfo.uiInAddr1,uiImgUVSize);
	uiAddr2 = md_platform_pa2va_remap(pMdbcPara->InInfo.uiInAddr2,uiImgUVSize);

	uiPhyAddr0 = pMdbcPara->InInfo.uiInAddr0;//fmem_lookup_pa(uiAddr0);//dma_getPhyAddr(uiAddr0);
	uiPhyAddr1 = pMdbcPara->InInfo.uiInAddr1;//fmem_lookup_pa(uiAddr1);//dma_getPhyAddr(uiAddr1);
	uiPhyAddr2 = pMdbcPara->InInfo.uiInAddr2;//fmem_lookup_pa(uiAddr2);//dma_getPhyAddr(uiAddr2);
	uiPhyAddr3 = pMdbcPara->InInfo.uiInAddr3;//fmem_lookup_pa(uiAddr3);//dma_getPhyAddr(uiAddr3);
	uiPhyAddr4 = pMdbcPara->InInfo.uiInAddr4;//fmem_lookup_pa(uiAddr4);//dma_getPhyAddr(uiAddr4);
	uiPhyAddr5 = pMdbcPara->InInfo.uiInAddr5;//fmem_lookup_pa(uiAddr5);//dma_getPhyAddr(uiAddr5);

#else
	uiAddr0 = pMdbcPara->InInfo.uiInAddr0;
	uiAddr1 = pMdbcPara->InInfo.uiInAddr1;
	uiAddr2 = pMdbcPara->InInfo.uiInAddr2;
	uiAddr3 = pMdbcPara->InInfo.uiInAddr3;
	uiAddr4 = pMdbcPara->InInfo.uiInAddr4;
	uiAddr5 = pMdbcPara->InInfo.uiInAddr5;
	//uiAddr6 = pMdbcPara->uiLLAddr;
	uiPhyAddr0 = md_platform_va2pa(uiAddr0);//fmem_lookup_pa(uiAddr0);//dma_getPhyAddr(uiAddr0);
	uiPhyAddr1 = md_platform_va2pa(uiAddr1);//fmem_lookup_pa(uiAddr1);//dma_getPhyAddr(uiAddr1);
	uiPhyAddr2 = md_platform_va2pa(uiAddr2);//fmem_lookup_pa(uiAddr2);//dma_getPhyAddr(uiAddr2);
	uiPhyAddr3 = md_platform_va2pa(uiAddr3);//fmem_lookup_pa(uiAddr3);//dma_getPhyAddr(uiAddr3);
	uiPhyAddr4 = md_platform_va2pa(uiAddr4);//fmem_lookup_pa(uiAddr4);//dma_getPhyAddr(uiAddr4);
	uiPhyAddr5 = md_platform_va2pa(uiAddr5);//fmem_lookup_pa(uiAddr5);//dma_getPhyAddr(uiAddr5);
	//if(uiAddr6!=0)uiPhyAddr6 = fmem_lookup_pa(uiAddr6);//dma_getPhyAddr(uiAddr6);
	//else uiPhyAddr6 = 0;
#endif
	
	if (uiAddr0 != 0) {
		md_platform_dma_flush_mem2dev(uiAddr0, uiImgYSize);
	}
	if (uiAddr1 != 0) {
		md_platform_dma_flush_mem2dev(uiAddr1, uiImgUVSize);
	}
	if (uiAddr2 != 0) {
		md_platform_dma_flush_mem2dev(uiAddr2, uiImgUVSize);
	}
	//if (uiAddr3 != 0) {
	//	md_platform_dma_flush_mem2dev(uiAddr3, uiSize*5*pMdbcPara->MdmatchPara.model_num);
	//}
	//if (uiAddr4 != 0) {
	//	md_platform_dma_flush_mem2dev(uiAddr4, uiSize*6);
	//}
	//if (uiAddr5 != 0) {
	//	md_platform_dma_flush_mem2dev(uiAddr5, ((uiSize+15)/16)*16*12);
	//}
	mdbc_setDmaInDataAddr(uiPhyAddr0, uiPhyAddr1, uiPhyAddr2, uiPhyAddr3, uiPhyAddr4, uiPhyAddr5);
	//mdbc_set_dmain_lladdr(uiPhyAddr6);
#if MD_IOREMAP_IN_KERNEL
	md_platform_pa2va_unmap(uiAddr0,uiPhyAddr0);
	md_platform_pa2va_unmap(uiAddr1,uiPhyAddr1);
	md_platform_pa2va_unmap(uiAddr2,uiPhyAddr2);
#endif

	// Output DMA/Cache Handle
#if MD_IOREMAP_IN_KERNEL
	uiPhyAddr0 = pMdbcPara->OutInfo.uiOutAddr0;//fmem_lookup_pa(uiAddr0);//dma_getPhyAddr(uiAddr0);
	uiPhyAddr1 = pMdbcPara->OutInfo.uiOutAddr1;//fmem_lookup_pa(uiAddr1);//dma_getPhyAddr(uiAddr1);
	uiPhyAddr2 = pMdbcPara->OutInfo.uiOutAddr2;//fmem_lookup_pa(uiAddr2);//dma_getPhyAddr(uiAddr2);
	uiPhyAddr3 = pMdbcPara->OutInfo.uiOutAddr3;//fmem_lookup_pa(uiAddr3);//dma_getPhyAddr(uiAddr3);
#else
	uiAddr0 = pMdbcPara->OutInfo.uiOutAddr0;
	uiAddr1 = pMdbcPara->OutInfo.uiOutAddr1;
	uiAddr2 = pMdbcPara->OutInfo.uiOutAddr2;
	uiAddr3 = pMdbcPara->OutInfo.uiOutAddr3;
	uiPhyAddr0 = md_platform_va2pa(uiAddr0);//fmem_lookup_pa(uiAddr0);//dma_getPhyAddr(uiAddr0);
	uiPhyAddr1 = md_platform_va2pa(uiAddr1);//fmem_lookup_pa(uiAddr1);//dma_getPhyAddr(uiAddr1);
	uiPhyAddr2 = md_platform_va2pa(uiAddr2);//fmem_lookup_pa(uiAddr2);//dma_getPhyAddr(uiAddr2);
	uiPhyAddr3 = md_platform_va2pa(uiAddr3);//fmem_lookup_pa(uiAddr3);//dma_getPhyAddr(uiAddr3);
#endif
	
	//if (uiAddr0 != 0) {
	//	md_platform_dma_flush_mem2dev(uiAddr0, ((uiSize+7)/8));
	//}
	//if (uiAddr1 != 0) {
	//	md_platform_dma_flush_mem2dev(uiAddr1, uiSize*5*pMdbcPara->MdmatchPara.model_num);
	//}
	//if (uiAddr2 != 0) {
	//	md_platform_dma_flush_mem2dev(uiAddr2, uiSize*6);
	//}
	//if (uiAddr3 != 0) {
	//	md_platform_dma_flush_mem2dev(uiAddr3, ((uiSize+15)/16)*16*12);
	//}
	mdbc_setDmaOutAddr(uiPhyAddr0, uiPhyAddr1, uiPhyAddr2, uiPhyAddr3);
#else
	// DRAM addresses of input fetures
	mdbc_setDmaInDataAddr(pMdbcPara->InInfo.uiInAddr0, pMdbcPara->InInfo.uiInAddr1, pMdbcPara->InInfo.uiInAddr2,
	                      pMdbcPara->InInfo.uiInAddr3, pMdbcPara->InInfo.uiInAddr4, pMdbcPara->InInfo.uiInAddr5);
	// Output DRAM addresses
	mdbc_setDmaOutAddr(pMdbcPara->OutInfo.uiOutAddr0, pMdbcPara->OutInfo.uiOutAddr1, pMdbcPara->OutInfo.uiOutAddr2, pMdbcPara->OutInfo.uiOutAddr3);
	//mdbc_set_dmain_lladdr(pMdbcPara->uiLLAddr);
#endif

	// Mode
	mdbc_setMdbcMode(pMdbcPara->mode);
	// Control
	mdbc_setMdbcControl(pMdbcPara->controlEn);
	// Size
	if (pMdbcPara->Size.uiMdbcWidth > 960 || pMdbcPara->Size.uiMdbcWidth < 60) {
		DBG_ERR("width must be 60 <= width <= 960! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->Size.uiMdbcWidth%2 == 1) {
		DBG_ERR("width must be even! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->Size.uiMdbcHeight > 1022 || pMdbcPara->Size.uiMdbcHeight < 60) {
		DBG_ERR("height must be 60 <= height <= 1022! Abort!\r\n");
		return E_SYS;
	}
	if (pMdbcPara->Size.uiMdbcHeight%2 == 1) {
		DBG_ERR("height must be even! Abort!\r\n");
		return E_SYS;
	}
	mdbc_setInputSize(&(pMdbcPara->Size));
	// Lineoffsets
	mdbc_setDmaInOffset(pMdbcPara->InInfo.uiLofs0,pMdbcPara->InInfo.uiLofs1);
	// Interrupt enable
	mdbc_enableInt(ENABLE, pMdbcPara->uiIntEn);

	mdbc_chkStateMachine(MDBC_OP_SETMODE, TRUE);

	return E_OK;
}

ER mdbc_ll_setmode(UINT32 ll_addr)
{
#if (MDBC_DMA_CACHE_HANDLE == 1)
	UINT32 uiAddr6=0, uiPhyAddr6;
#endif
	ER ER_Code;
	ER_Code = mdbc_chkStateMachine(MDBC_OP_SETMODE, FALSE);
	if (ER_Code != E_OK) {
		return ER_Code;
	}
	// Clear engine interrupt status
	mdbc_enableInt(DISABLE, MDBC_INTE_ALL);
	mdbc_clrIntrStatus(MDBC_INT_ALL);
#if (MDBC_DMA_CACHE_HANDLE == 1)
	uiAddr6 = ll_addr;
	if(uiAddr6!=0)uiPhyAddr6 = md_platform_va2pa(uiAddr6);//fmem_lookup_pa(uiAddr6);//dma_getPhyAddr(uiAddr6);
	else uiPhyAddr6 = 0;
	if (uiAddr6 != 0) {
		md_platform_dma_flush_mem2dev(uiAddr6, 8*68*20);
	}

	mdbc_set_dmain_lladdr(uiPhyAddr6);
#else
	mdbc_set_dmain_lladdr(ll_addr);
#endif
	mdbc_enableInt(ENABLE, MDBC_INTE_ALL);

	ER_Code = mdbc_ll_pause();
	if (ER_Code != E_OK) {
		return ER_Code;
	}

	ER_Code = mdbc_chkStateMachine(MDBC_OP_SETMODE, TRUE);
	return ER_Code;
}

/**
    MDBC set operation mode

    Configure MDBC operation mode.

    @return None.
*/
void mdbc_setMdbcMode(MDBC_MODE mode)
{
	T_MDBC_MODE_REGISTER0 reg1;
	reg1.reg = MDBC_GETREG(MDBC_MODE_REGISTER0_OFS);

	reg1.bit.MDBC_MODE = mode;

	MDBC_SETREG(MDBC_MODE_REGISTER0_OFS, reg1.reg);
}

/**
    MDBC set Control

    Configure MDBC Control.

    @return None.
*/
void mdbc_setMdbcControl(MDBC_CONTROL_EN controlEn)
{
	T_MDBC_MODE_REGISTER0 reg1;
	reg1.reg = MDBC_GETREG(MDBC_MODE_REGISTER0_OFS);

	reg1.bit.BC_UPDATE_NEI_EN = controlEn.update_nei_en;
	reg1.bit.BC_DEGHOST_EN = controlEn.deghost_en;
	reg1.bit.ROI_EN0 = controlEn.roi_en0;
	reg1.bit.ROI_EN1 = controlEn.roi_en1;
	reg1.bit.ROI_EN2 = controlEn.roi_en2;
	reg1.bit.ROI_EN3 = controlEn.roi_en3;
	reg1.bit.ROI_EN4 = controlEn.roi_en4;
	reg1.bit.ROI_EN5 = controlEn.roi_en5;
	reg1.bit.ROI_EN6 = controlEn.roi_en6;
	reg1.bit.ROI_EN7 = controlEn.roi_en7;
	reg1.bit.CHKSUM_EN = controlEn.chksum_en;
	reg1.bit.BGMW_SAVE_BW_EN = controlEn.bgmw_save_bw_en;

	MDBC_SETREG(MDBC_MODE_REGISTER0_OFS, reg1.reg);
}

/**
    MDBC set DMA input address of data

    Configure DMA input starting addresses of data.

    @param uiSai0 DMA input channel (DMA to MDBC) starting address 0.
    @param uiSai1 DMA input channel (DMA to MDBC) starting address 1.
    @param uiSai2 DMA input channel (DMA to MDBC) starting address 2.

    @return None.
*/
void mdbc_setDmaInDataAddr(UINT32 uiSai0, UINT32 uiSai1, UINT32 uiSai2,UINT32 uiSai3, UINT32 uiSai4, UINT32 uiSai5)
{
	T_DMA_TO_MDBC_REGISTER0 reg0;
	T_DMA_TO_MDBC_REGISTER1 reg1;
	T_DMA_TO_MDBC_REGISTER4 reg2;
	T_DMA_TO_MDBC_REGISTER5 reg3;
	T_DMA_TO_MDBC_REGISTER6 reg4;
	T_DMA_TO_MDBC_REGISTER7 reg5;

    if ((uiSai0 == 0) || (uiSai1 == 0) || (uiSai2 == 0) ||
	    (uiSai3 == 0) || (uiSai4 == 0) || (uiSai5 == 0)) {
		DBG_ERR("Input addresses cannot be 0x0!\r\n");
		return;
	}

	reg0.reg = MDBC_GETREG(DMA_TO_MDBC_REGISTER0_OFS);
	reg0.bit.DRAM_SAI0 = uiSai0 >> 2;

	reg1.reg = MDBC_GETREG(DMA_TO_MDBC_REGISTER1_OFS);
	reg1.bit.DRAM_SAI1 = uiSai1 >> 2;

	reg2.reg = MDBC_GETREG(DMA_TO_MDBC_REGISTER4_OFS);
	reg2.bit.DRAM_SAI2 = uiSai2 >> 2;

	reg3.reg = MDBC_GETREG(DMA_TO_MDBC_REGISTER5_OFS);
	reg3.bit.DRAM_SAI3 = uiSai3 >> 2;

	reg4.reg = MDBC_GETREG(DMA_TO_MDBC_REGISTER6_OFS);
	reg4.bit.DRAM_SAI4 = uiSai4 >> 2;

	reg5.reg = MDBC_GETREG(DMA_TO_MDBC_REGISTER7_OFS);
	reg5.bit.DRAM_SAI5 = uiSai5 >> 2;

	MDBC_SETREG(DMA_TO_MDBC_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(DMA_TO_MDBC_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(DMA_TO_MDBC_REGISTER4_OFS, reg2.reg);
	MDBC_SETREG(DMA_TO_MDBC_REGISTER5_OFS, reg3.reg);
	MDBC_SETREG(DMA_TO_MDBC_REGISTER6_OFS, reg4.reg);
	MDBC_SETREG(DMA_TO_MDBC_REGISTER7_OFS, reg5.reg);
}

void mdbc_set_dmain_lladdr(UINT32 uiSaiLL)
{
	T_DMA_TO_MDBC_LINKED_LIST_REGISTER reg0;
	reg0.reg = MDBC_GETREG(DMA_TO_MDBC_LINKED_LIST_REGISTER_OFS);
	reg0.bit.DRAM_SAI_LL = uiSaiLL >> 2;
	MDBC_SETREG(DMA_TO_MDBC_LINKED_LIST_REGISTER_OFS, reg0.reg);
}

/**
    MDBC set DMA input line offset

    Configure DMA input line offset.

    @param uiOfsi0 input line offset(word) 0 for all input channels.

    @return None.
*/
void mdbc_setDmaInOffset(UINT32 uiOfsi0,UINT32 uiOfsi1)
{
	T_DMA_TO_MDBC_REGISTER2 reg0;
	T_DMA_TO_MDBC_REGISTER3 reg1;

	reg0.reg = MDBC_GETREG(DMA_TO_MDBC_REGISTER2_OFS);
	reg0.bit.DRAM_OFSI0 = uiOfsi0 >> 2;
	MDBC_SETREG(DMA_TO_MDBC_REGISTER2_OFS, reg0.reg);

	reg1.reg = MDBC_GETREG(DMA_TO_MDBC_REGISTER3_OFS);
	reg1.bit.DRAM_OFSI1 = uiOfsi1 >> 2;
	MDBC_SETREG(DMA_TO_MDBC_REGISTER3_OFS, reg1.reg);
}

/**
    MDBC set DMA output address

    Configure DMA output starting addresses.

    @param uiSai DMA output channel (MDBC to DMA) starting address.

    @return None.
*/
void mdbc_setDmaOutAddr(UINT32 uiSao0,UINT32 uiSao1,UINT32 uiSao2,UINT32 uiSao3)
{
	T_MDBC_TO_DMA_REGISTER0 reg0;
	T_MDBC_TO_DMA_REGISTER1 reg1;
	T_MDBC_TO_DMA_REGISTER2 reg2;
	T_MDBC_TO_DMA_REGISTER3 reg3;

    if ((uiSao0 == 0)||(uiSao1 == 0)||(uiSao2 == 0)||(uiSao3 == 0)) {
		DBG_ERR("Output addresses cannot be 0x0!\r\n");
		return;
	}

	reg0.reg = MDBC_GETREG(MDBC_TO_DMA_REGISTER0_OFS);
	reg0.bit.DRAM_SAO0 = uiSao0>> 2;
	MDBC_SETREG(MDBC_TO_DMA_REGISTER0_OFS, reg0.reg);

	reg1.reg = MDBC_GETREG(MDBC_TO_DMA_REGISTER1_OFS);
	reg1.bit.DRAM_SAO1 = uiSao1>> 2;
	MDBC_SETREG(MDBC_TO_DMA_REGISTER1_OFS, reg1.reg);

	reg2.reg = MDBC_GETREG(MDBC_TO_DMA_REGISTER2_OFS);
	reg2.bit.DRAM_SAO2 = uiSao2>> 2;
	MDBC_SETREG(MDBC_TO_DMA_REGISTER2_OFS, reg2.reg);

	reg3.reg = MDBC_GETREG(MDBC_TO_DMA_REGISTER3_OFS);
	reg3.bit.DRAM_SAO3 = uiSao3>> 2;
	MDBC_SETREG(MDBC_TO_DMA_REGISTER3_OFS, reg3.reg);
}

/**
    MDBC set input size

    Configure input size information.

    @param cnnInSize MDBC input size information.

    @return None.
*/
void mdbc_setInputSize(MDBC_IN_SIZE *pMdbcInSize)
{
	T_INPUT_SIZE_REGISTER reg0;

	reg0.reg = MDBC_GETREG(INPUT_SIZE_REGISTER_OFS);
	reg0.bit.WIDTH = pMdbcInSize->uiMdbcWidth>>1;
	reg0.bit.HEIGHT = pMdbcInSize->uiMdbcHeight>>1;

	MDBC_SETREG(INPUT_SIZE_REGISTER_OFS, reg0.reg);
}

/**
    MDBC set Model Match parameters

    @param cnnParam MDBC Model Match parameters.

    @return None.
*/
void mdbc_setMdmatchParam(MDBC_MDMATCH_PARAM *pMdmatchParam)
{
	T_MODEL_MATCH_REGISTER0 reg0;
	T_MODEL_MATCH_REGISTER1 reg1;
	T_MODEL_MATCH_REGISTER2 reg2;
	T_MODEL_MATCH_REGISTER3 reg3;
	T_MODEL_MATCH_REGISTER4 reg4;

	reg0.reg = MDBC_GETREG(MODEL_MATCH_REGISTER0_OFS);
	reg1.reg = MDBC_GETREG(MODEL_MATCH_REGISTER1_OFS);
	reg2.reg = MDBC_GETREG(MODEL_MATCH_REGISTER2_OFS);
	reg3.reg = MDBC_GETREG(MODEL_MATCH_REGISTER3_OFS);
	reg4.reg = MDBC_GETREG(MODEL_MATCH_REGISTER4_OFS);

	reg0.bit.LBSP_TH  = pMdmatchParam->lbsp_th;
	reg0.bit.D_COLOUR = pMdmatchParam->d_colour;
	reg0.bit.R_COLOUR = pMdmatchParam->r_colour;
	reg0.bit.D_LBSP   = pMdmatchParam->d_lbsp;
	reg0.bit.R_LBSP   = pMdmatchParam->r_lbsp;

	reg1.bit.BG_MODEL_NUM = pMdmatchParam->model_num;
	reg1.bit.T_ALPHA = pMdmatchParam->t_alpha;
	reg1.bit.DW_SHIFT = pMdmatchParam->dw_shift;
	reg1.bit.D_LAST_ALPHA = pMdmatchParam->dlast_alpha;

	reg2.bit.BC_MIN_MATCH = pMdmatchParam->min_match;
	reg2.bit.DLT_ALPHA = pMdmatchParam->dlt_alpha;
	reg2.bit.DST_ALPHA = pMdmatchParam->dst_alpha;

	reg3.bit.BC_UV_THRES = pMdmatchParam->uv_thres;
	reg3.bit.S_ALPHA = pMdmatchParam->s_alpha;

	reg4.bit.DBG_LUM_DIFF = pMdmatchParam->dbg_lumDiff;
	reg4.bit.DBG_LUM_DIFF_EN = pMdmatchParam->dbg_lumDiff_en;

	MDBC_SETREG(MODEL_MATCH_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(MODEL_MATCH_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(MODEL_MATCH_REGISTER2_OFS, reg2.reg);
	MDBC_SETREG(MODEL_MATCH_REGISTER3_OFS, reg3.reg);
	MDBC_SETREG(MODEL_MATCH_REGISTER4_OFS, reg4.reg);
}

/**
    MDBC set morph parameters

    @param poolParam MDBC morph parameters.

    @return None.
*/
void mdbc_setMorphParam(MDBC_MOR_PARAM *pMorphParam)
{
	T_MORPHOLOGICAL_PROCESS_REGISTER reg0;

	reg0.reg = MDBC_GETREG(MORPHOLOGICAL_PROCESS_REGISTER_OFS);
	reg0.bit.TH_ERO = pMorphParam->th_ero;
	reg0.bit.TH_DIL = pMorphParam->th_dil;
	reg0.bit.MOR_SEL0 = pMorphParam->mor_sel0;
	reg0.bit.MOR_SEL1 = pMorphParam->mor_sel1;
	reg0.bit.MOR_SEL2 = pMorphParam->mor_sel2;
	reg0.bit.MOR_SEL3 = pMorphParam->mor_sel3;

	MDBC_SETREG(MORPHOLOGICAL_PROCESS_REGISTER_OFS, reg0.reg);
}

/**
    MDBC set Update parameters

    @param LRNParam MDBC Update parameters.

    @return None.
*/
void mdbc_setUpdateParam(MDBC_UPD_PARAM *pUpdateParam)
{
	T_UPDATE_REGISTER0 reg0;
	T_UPDATE_REGISTER1 reg1;
	T_UPDATE_REGISTER2 reg2;
	T_UPDATE_REGISTER3 reg3;
	T_UPDATE_REGISTER4 reg4;

	reg0.reg = MDBC_GETREG(UPDATE_REGISTER0_OFS);
	reg1.reg = MDBC_GETREG(UPDATE_REGISTER1_OFS);
	reg2.reg = MDBC_GETREG(UPDATE_REGISTER2_OFS);
	reg3.reg = MDBC_GETREG(UPDATE_REGISTER3_OFS);
	reg4.reg = MDBC_GETREG(UPDATE_REGISTER4_OFS);

	reg0.bit.BC_MIN_T = pUpdateParam->minT;
	reg0.bit.BC_MAX_T = pUpdateParam->maxT;
	reg0.bit.BC_MAX_FG_FRM = pUpdateParam->maxFgFrm;

	reg1.bit.BC_DEGHOST_DTH = pUpdateParam->deghost_dth;
	reg1.bit.BC_DEGHOST_STH = pUpdateParam->deghost_sth;

	reg2.bit.BC_STABLE_FRAME = pUpdateParam->stable_frm;
	reg2.bit.BC_UPDATE_DYN = pUpdateParam->update_dyn;
	reg2.bit.BC_VA_DISTTH = pUpdateParam->va_distth;
	reg2.bit.BC_T_DISTTH = pUpdateParam->t_distth;

	reg3.bit.DBG_FRM_ID = pUpdateParam->dbg_frmID;
	reg3.bit.DBG_FRM_ID_EN = pUpdateParam->dbg_frmID_en;

	reg4.bit.DBG_RND = pUpdateParam->dbg_rnd;
	reg4.bit.DBG_RND_EN = pUpdateParam->dbg_rnd_en;

	MDBC_SETREG(UPDATE_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(UPDATE_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(UPDATE_REGISTER2_OFS, reg2.reg);
	MDBC_SETREG(UPDATE_REGISTER3_OFS, reg3.reg);
	MDBC_SETREG(UPDATE_REGISTER4_OFS, reg4.reg);
}

UINT32 mdbc_getLumDiff(void)
{
	T_MODEL_MATCH_REGISTER5 reg1;
	reg1.reg = MDBC_GETREG(MODEL_MATCH_REGISTER5_OFS);

	return reg1.reg;
}

UINT32 mdbc_getFrmID(void)
{
	T_UPDATE_REGISTER3 reg1;
	reg1.reg = MDBC_GETREG(UPDATE_REGISTER3_OFS);

	return (reg1.reg>>12);
}

UINT32 mdbc_getRND(void)
{
	T_UPDATE_REGISTER4 reg1;
	reg1.reg = MDBC_GETREG(UPDATE_REGISTER4_OFS);

	return (reg1.reg>>16);
}

/**
    MDBC set ROI0 parameters

    @param MDBC ROI0 parameters.

    @return None.
*/
void mdbc_setROI0Param(MDBC_ROI_PARAM *pROIParam)
{
	T_ROI0_REGISTER0 reg0;
	T_ROI0_REGISTER1 reg1;
	T_ROI0_REGISTER2 reg2;
	T_ROI0_REGISTER3 reg3;

	reg0.reg = MDBC_GETREG(ROI0_REGISTER0_OFS);
	reg1.reg = MDBC_GETREG(ROI0_REGISTER1_OFS);
	reg2.reg = MDBC_GETREG(ROI0_REGISTER2_OFS);
	reg3.reg = MDBC_GETREG(ROI0_REGISTER3_OFS);

	reg0.bit.ROI_X0   		= pROIParam->roi_x;
	reg0.bit.ROI_Y0   		= pROIParam->roi_y;

	reg1.bit.ROI_W0 		= pROIParam->roi_w;
	reg1.bit.ROI_H0 		= pROIParam->roi_h;
	reg1.bit.ROI_UV_THRES0 	= pROIParam->roi_uv_thres;

	reg2.bit.ROI_LBSP_TH0 	= pROIParam->roi_lbsp_th;
	reg2.bit.ROI_D_COLOUR0 	= pROIParam->roi_d_colour;
	reg2.bit.ROI_R_COLOUR0 	= pROIParam->roi_r_colour;
	reg2.bit.ROI_D_LBSP0 	= pROIParam->roi_d_lbsp;
	reg2.bit.ROI_R_LBSP0	= pROIParam->roi_r_lbsp;

	reg3.bit.ROI_MORPH_EN0 	= pROIParam->roi_morph_en;
	reg3.bit.ROI_MIN_T0 	= pROIParam->roi_minT;
	reg3.bit.ROI_MAX_T0 	= pROIParam->roi_maxT;

	MDBC_SETREG(ROI0_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(ROI0_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(ROI0_REGISTER2_OFS, reg2.reg);
	MDBC_SETREG(ROI0_REGISTER3_OFS, reg3.reg);
}

/**
    MDBC set ROI1 parameters

    @param MDBC ROI1 parameters.

    @return None.
*/
void mdbc_setROI1Param(MDBC_ROI_PARAM *pROIParam)
{
	T_ROI1_REGISTER0 reg0;
	T_ROI1_REGISTER1 reg1;
	T_ROI1_REGISTER2 reg2;
	T_ROI1_REGISTER3 reg3;

	reg0.reg = MDBC_GETREG(ROI1_REGISTER0_OFS);
	reg1.reg = MDBC_GETREG(ROI1_REGISTER1_OFS);
	reg2.reg = MDBC_GETREG(ROI1_REGISTER2_OFS);
	reg3.reg = MDBC_GETREG(ROI1_REGISTER3_OFS);

	reg0.bit.ROI_X1   		= pROIParam->roi_x;
	reg0.bit.ROI_Y1   		= pROIParam->roi_y;

	reg1.bit.ROI_W1 		= pROIParam->roi_w;
	reg1.bit.ROI_H1 		= pROIParam->roi_h;
	reg1.bit.ROI_UV_THRES1 	= pROIParam->roi_uv_thres;

	reg2.bit.ROI_LBSP_TH1 	= pROIParam->roi_lbsp_th;
	reg2.bit.ROI_D_COLOUR1 	= pROIParam->roi_d_colour;
	reg2.bit.ROI_R_COLOUR1 	= pROIParam->roi_r_colour;
	reg2.bit.ROI_D_LBSP1 	= pROIParam->roi_d_lbsp;
	reg2.bit.ROI_R_LBSP1	= pROIParam->roi_r_lbsp;

	reg3.bit.ROI_MORPH_EN1 	= pROIParam->roi_morph_en;
	reg3.bit.ROI_MIN_T1 	= pROIParam->roi_minT;
	reg3.bit.ROI_MAX_T1 	= pROIParam->roi_maxT;

	MDBC_SETREG(ROI1_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(ROI1_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(ROI1_REGISTER2_OFS, reg2.reg);
	MDBC_SETREG(ROI1_REGISTER3_OFS, reg3.reg);
}

/**
    MDBC set ROI2 parameters

    @param MDBC ROI2 parameters.

    @return None.
*/
void mdbc_setROI2Param(MDBC_ROI_PARAM *pROIParam)
{
	T_ROI2_REGISTER0 reg0;
	T_ROI2_REGISTER1 reg1;
	T_ROI2_REGISTER2 reg2;
	T_ROI2_REGISTER3 reg3;

	reg0.reg = MDBC_GETREG(ROI2_REGISTER0_OFS);
	reg1.reg = MDBC_GETREG(ROI2_REGISTER1_OFS);
	reg2.reg = MDBC_GETREG(ROI2_REGISTER2_OFS);
	reg3.reg = MDBC_GETREG(ROI2_REGISTER3_OFS);

	reg0.bit.ROI_X2   		= pROIParam->roi_x;
	reg0.bit.ROI_Y2   		= pROIParam->roi_y;

	reg1.bit.ROI_W2 		= pROIParam->roi_w;
	reg1.bit.ROI_H2 		= pROIParam->roi_h;
	reg1.bit.ROI_UV_THRES2 	= pROIParam->roi_uv_thres;

	reg2.bit.ROI_LBSP_TH2 	= pROIParam->roi_lbsp_th;
	reg2.bit.ROI_D_COLOUR2 	= pROIParam->roi_d_colour;
	reg2.bit.ROI_R_COLOUR2 	= pROIParam->roi_r_colour;
	reg2.bit.ROI_D_LBSP2 	= pROIParam->roi_d_lbsp;
	reg2.bit.ROI_R_LBSP2	= pROIParam->roi_r_lbsp;

	reg3.bit.ROI_MORPH_EN2 	= pROIParam->roi_morph_en;
	reg3.bit.ROI_MIN_T2 	= pROIParam->roi_minT;
	reg3.bit.ROI_MAX_T2 	= pROIParam->roi_maxT;

	MDBC_SETREG(ROI2_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(ROI2_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(ROI2_REGISTER2_OFS, reg2.reg);
	MDBC_SETREG(ROI2_REGISTER3_OFS, reg3.reg);
}

/**
    MDBC set ROI3 parameters

    @param MDBC ROI3 parameters.

    @return None.
*/
void mdbc_setROI3Param(MDBC_ROI_PARAM *pROIParam)
{
	T_ROI3_REGISTER0 reg0;
	T_ROI3_REGISTER1 reg1;
	T_ROI3_REGISTER2 reg2;
	T_ROI3_REGISTER3 reg3;

	reg0.reg = MDBC_GETREG(ROI3_REGISTER0_OFS);
	reg1.reg = MDBC_GETREG(ROI3_REGISTER1_OFS);
	reg2.reg = MDBC_GETREG(ROI3_REGISTER2_OFS);
	reg3.reg = MDBC_GETREG(ROI3_REGISTER3_OFS);

	reg0.bit.ROI_X3   		= pROIParam->roi_x;
	reg0.bit.ROI_Y3   		= pROIParam->roi_y;

	reg1.bit.ROI_W3 		= pROIParam->roi_w;
	reg1.bit.ROI_H3 		= pROIParam->roi_h;
	reg1.bit.ROI_UV_THRES3 	= pROIParam->roi_uv_thres;

	reg2.bit.ROI_LBSP_TH3 	= pROIParam->roi_lbsp_th;
	reg2.bit.ROI_D_COLOUR3 	= pROIParam->roi_d_colour;
	reg2.bit.ROI_R_COLOUR3 	= pROIParam->roi_r_colour;
	reg2.bit.ROI_D_LBSP3 	= pROIParam->roi_d_lbsp;
	reg2.bit.ROI_R_LBSP3	= pROIParam->roi_r_lbsp;

	reg3.bit.ROI_MORPH_EN3 	= pROIParam->roi_morph_en;
	reg3.bit.ROI_MIN_T3 	= pROIParam->roi_minT;
	reg3.bit.ROI_MAX_T3 	= pROIParam->roi_maxT;

	MDBC_SETREG(ROI3_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(ROI3_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(ROI3_REGISTER2_OFS, reg2.reg);
	MDBC_SETREG(ROI3_REGISTER3_OFS, reg3.reg);
}

/**
    MDBC set ROI4 parameters

    @param MDBC ROI4 parameters.

    @return None.
*/
void mdbc_setROI4Param(MDBC_ROI_PARAM *pROIParam)
{
	T_ROI4_REGISTER0 reg0;
	T_ROI4_REGISTER1 reg1;
	T_ROI4_REGISTER2 reg2;
	T_ROI4_REGISTER3 reg3;

	reg0.reg = MDBC_GETREG(ROI4_REGISTER0_OFS);
	reg1.reg = MDBC_GETREG(ROI4_REGISTER1_OFS);
	reg2.reg = MDBC_GETREG(ROI4_REGISTER2_OFS);
	reg3.reg = MDBC_GETREG(ROI4_REGISTER3_OFS);

	reg0.bit.ROI_X4   		= pROIParam->roi_x;
	reg0.bit.ROI_Y4   		= pROIParam->roi_y;

	reg1.bit.ROI_W4 		= pROIParam->roi_w;
	reg1.bit.ROI_H4 		= pROIParam->roi_h;
	reg1.bit.ROI_UV_THRES4 	= pROIParam->roi_uv_thres;

	reg2.bit.ROI_LBSP_TH4 	= pROIParam->roi_lbsp_th;
	reg2.bit.ROI_D_COLOUR4 	= pROIParam->roi_d_colour;
	reg2.bit.ROI_R_COLOUR4 	= pROIParam->roi_r_colour;
	reg2.bit.ROI_D_LBSP4	= pROIParam->roi_d_lbsp;
	reg2.bit.ROI_R_LBSP4	= pROIParam->roi_r_lbsp;

	reg3.bit.ROI_MORPH_EN4 	= pROIParam->roi_morph_en;
	reg3.bit.ROI_MIN_T4 	= pROIParam->roi_minT;
	reg3.bit.ROI_MAX_T4 	= pROIParam->roi_maxT;

	MDBC_SETREG(ROI4_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(ROI4_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(ROI4_REGISTER2_OFS, reg2.reg);
	MDBC_SETREG(ROI4_REGISTER3_OFS, reg3.reg);
}

/**
    MDBC set ROI5 parameters

    @param MDBC ROI5 parameters.

    @return None.
*/
void mdbc_setROI5Param(MDBC_ROI_PARAM *pROIParam)
{
	T_ROI5_REGISTER0 reg0;
	T_ROI5_REGISTER1 reg1;
	T_ROI5_REGISTER2 reg2;
	T_ROI5_REGISTER3 reg3;

	reg0.reg = MDBC_GETREG(ROI5_REGISTER0_OFS);
	reg1.reg = MDBC_GETREG(ROI5_REGISTER1_OFS);
	reg2.reg = MDBC_GETREG(ROI5_REGISTER2_OFS);
	reg3.reg = MDBC_GETREG(ROI5_REGISTER3_OFS);

	reg0.bit.ROI_X5   		= pROIParam->roi_x;
	reg0.bit.ROI_Y5   		= pROIParam->roi_y;

	reg1.bit.ROI_W5 		= pROIParam->roi_w;
	reg1.bit.ROI_H5 		= pROIParam->roi_h;
	reg1.bit.ROI_UV_THRES5 	= pROIParam->roi_uv_thres;

	reg2.bit.ROI_LBSP_TH5	= pROIParam->roi_lbsp_th;
	reg2.bit.ROI_D_COLOUR5 	= pROIParam->roi_d_colour;
	reg2.bit.ROI_R_COLOUR5 	= pROIParam->roi_r_colour;
	reg2.bit.ROI_D_LBSP5	= pROIParam->roi_d_lbsp;
	reg2.bit.ROI_R_LBSP5	= pROIParam->roi_r_lbsp;

	reg3.bit.ROI_MORPH_EN5 	= pROIParam->roi_morph_en;
	reg3.bit.ROI_MIN_T5 	= pROIParam->roi_minT;
	reg3.bit.ROI_MAX_T5 	= pROIParam->roi_maxT;

	MDBC_SETREG(ROI5_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(ROI5_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(ROI5_REGISTER2_OFS, reg2.reg);
	MDBC_SETREG(ROI5_REGISTER3_OFS, reg3.reg);
}

/**
    MDBC set ROI6 parameters

    @param MDBC ROI6 parameters.

    @return None.
*/
void mdbc_setROI6Param(MDBC_ROI_PARAM *pROIParam)
{
	T_ROI6_REGISTER0 reg0;
	T_ROI6_REGISTER1 reg1;
	T_ROI6_REGISTER2 reg2;
	T_ROI6_REGISTER3 reg3;

	reg0.reg = MDBC_GETREG(ROI6_REGISTER0_OFS);
	reg1.reg = MDBC_GETREG(ROI6_REGISTER1_OFS);
	reg2.reg = MDBC_GETREG(ROI6_REGISTER2_OFS);
	reg3.reg = MDBC_GETREG(ROI6_REGISTER3_OFS);

	reg0.bit.ROI_X6   		= pROIParam->roi_x;
	reg0.bit.ROI_Y6   		= pROIParam->roi_y;

	reg1.bit.ROI_W6 		= pROIParam->roi_w;
	reg1.bit.ROI_H6 		= pROIParam->roi_h;
	reg1.bit.ROI_UV_THRES6 	= pROIParam->roi_uv_thres;

	reg2.bit.ROI_LBSP_TH6	= pROIParam->roi_lbsp_th;
	reg2.bit.ROI_D_COLOUR6 	= pROIParam->roi_d_colour;
	reg2.bit.ROI_R_COLOUR6 	= pROIParam->roi_r_colour;
	reg2.bit.ROI_D_LBSP6	= pROIParam->roi_d_lbsp;
	reg2.bit.ROI_R_LBSP6	= pROIParam->roi_r_lbsp;

	reg3.bit.ROI_MORPH_EN6 	= pROIParam->roi_morph_en;
	reg3.bit.ROI_MIN_T6 	= pROIParam->roi_minT;
	reg3.bit.ROI_MAX_T6 	= pROIParam->roi_maxT;

	MDBC_SETREG(ROI6_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(ROI6_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(ROI6_REGISTER2_OFS, reg2.reg);
	MDBC_SETREG(ROI6_REGISTER3_OFS, reg3.reg);
}

/**
    MDBC set ROI7 parameters

    @param MDBC ROI7 parameters.

    @return None.
*/
void mdbc_setROI7Param(MDBC_ROI_PARAM *pROIParam)
{
	T_ROI7_REGISTER0 reg0;
	T_ROI7_REGISTER1 reg1;
	T_ROI7_REGISTER2 reg2;
	T_ROI7_REGISTER3 reg3;

	reg0.reg = MDBC_GETREG(ROI7_REGISTER0_OFS);
	reg1.reg = MDBC_GETREG(ROI7_REGISTER1_OFS);
	reg2.reg = MDBC_GETREG(ROI7_REGISTER2_OFS);
	reg3.reg = MDBC_GETREG(ROI7_REGISTER3_OFS);

	reg0.bit.ROI_X7   		= pROIParam->roi_x;
	reg0.bit.ROI_Y7   		= pROIParam->roi_y;

	reg1.bit.ROI_W7 		= pROIParam->roi_w;
	reg1.bit.ROI_H7 		= pROIParam->roi_h;
	reg1.bit.ROI_UV_THRES7 	= pROIParam->roi_uv_thres;

	reg2.bit.ROI_LBSP_TH7	= pROIParam->roi_lbsp_th;
	reg2.bit.ROI_D_COLOUR7 	= pROIParam->roi_d_colour;
	reg2.bit.ROI_R_COLOUR7 	= pROIParam->roi_r_colour;
	reg2.bit.ROI_D_LBSP7	= pROIParam->roi_d_lbsp;
	reg2.bit.ROI_R_LBSP7	= pROIParam->roi_r_lbsp;

	reg3.bit.ROI_MORPH_EN7 	= pROIParam->roi_morph_en;
	reg3.bit.ROI_MIN_T7 	= pROIParam->roi_minT;
	reg3.bit.ROI_MAX_T7 	= pROIParam->roi_maxT;

	MDBC_SETREG(ROI7_REGISTER0_OFS, reg0.reg);
	MDBC_SETREG(ROI7_REGISTER1_OFS, reg1.reg);
	MDBC_SETREG(ROI7_REGISTER2_OFS, reg2.reg);
	MDBC_SETREG(ROI7_REGISTER3_OFS, reg3.reg);
}

/*
    table index :   [0x0, 0xff]         bits : 7_0
    mode        :   [0x0]               bits : 63_61
*/
UINT64 mdbc_ll_null_cmd(UINT8 tbl_idx)
{
	return (UINT64)tbl_idx;
}

/*
    regVal      :   [0x0, 0xffffffff]   bits : 31_0
    regOfs      :   [0x0, 0xfff]        bits : 43_32
    ByteEn      :   [0x0, 0xf]          bits : 47_44
    mode        :   [0x0]               bits : 63_61
*/
UINT64 mdbc_ll_upd_cmd(UINT8 byte_en, UINT16 reg_ofs, UINT32 reg_val)
{
	return (UINT64)(((UINT64)8 << 60) + ((UINT64)(byte_en & 0xf) << 44) + ((UINT64)(reg_ofs & 0xfff) << 32) + (UINT64)reg_val);
}

/*
    table index :   [0x0, 0xff]         bits : 7_0
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 mdbc_ll_nextll_cmd(UINT32 addr, UINT8 tbl_idx)
{
	return (UINT64)(((UINT64)2 << 60) + ((UINT64)addr << 8) + (UINT64)tbl_idx);
}

/*
    addr        :   [0x0, 0xffffffff]   bits : 39_8
    mode        :   [0x0]               bits : 63_61
*/
UINT64 mdbc_ll_nextupd_cmd(UINT32 addr)
{
	return (UINT64)(((UINT64)4 << 60) + ((UINT64)addr << 8));
}