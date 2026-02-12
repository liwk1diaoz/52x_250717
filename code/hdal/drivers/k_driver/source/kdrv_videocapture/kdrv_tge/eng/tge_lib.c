/*
    @file       tge_lib.c
    @ingroup    mIIPPTGE

    @brief      TGE integration control

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

//#include <string.h>
#include    "kwrap/type.h"//a header for basic variable type
#include    "tge_lib.h"//header for SIE: external enum/struct/api/variable
#include    "tge_int.h"//header for SIE: internal enum/struct/api/variable
#include    "tge_platform.h"
#ifdef __KERNEL__
#include    "tge_drv.h"
#endif

#define DRV_SUPPORT_IST 0

/**
    @addtogroup mIIPPTGE
*/
//@{

//---------------------------------------------------------------------------------------------
// internal variable/enum/struct

//#define pll_setClockFreq(x)
//#define pll_enableClock(x)
//#define pll_disableClock(x)
//#define pll_setClockRate(x,y)
//#define pll_getPLLEn(x,y)
//#define pll_setPLLEn(x)
//#define Perf_GetCurrent(x) 0
//#define pll_getClockFreq(x)

#if 0 // Notes: platform combination
static SEM_HANDLE SEMID_TGE;
static FLGPTN     FLG_ID_TGE;

#define FLGPTN_TGE_FRAMEEND      FLGPTN_BIT(0)
#define FLGPTN_TGE_FRAMEEND2     FLGPTN_BIT(1)
#define FLGPTN_TGE_FRAMEEND3     FLGPTN_BIT(2)
#define FLGPTN_TGE_FRAMEEND4     FLGPTN_BIT(3)
#define FLGPTN_TGE_FRAMEEND5     FLGPTN_BIT(4)
#define FLGPTN_TGE_FRAMEEND6     FLGPTN_BIT(5)
#define FLGPTN_TGE_FRAMEEND7     FLGPTN_BIT(6)
#define FLGPTN_TGE_FRAMEEND8     FLGPTN_BIT(7)
#define FLGPTN_TGE_VD_BP1        FLGPTN_BIT(8)
#define FLGPTN_TGE_VD2_BP1       FLGPTN_BIT(9)
#define FLGPTN_TGE_VD3_BP1       FLGPTN_BIT(10)
#define FLGPTN_TGE_VD4_BP1       FLGPTN_BIT(11)
#define FLGPTN_TGE_VD5_BP1       FLGPTN_BIT(12)
#define FLGPTN_TGE_VD6_BP1       FLGPTN_BIT(13)
#define FLGPTN_TGE_VD7_BP1       FLGPTN_BIT(14)
#define FLGPTN_TGE_VD8_BP1       FLGPTN_BIT(15)
#endif

#if 0//not yet
//#define SEMID_TGE 0xffffffff
//#define TGE_CLK   0xffffffff
//void *pfIstCB_TGE=NULL;
//#define DRV_IST_MODULE_TGE 0xffffffff
//#define FLG_ID_TGE 0xffffffff
#define PMC_MODULE_TGE 0xffffffff
#endif

//static ID g_TgeLockStatus = NO_TASK_LOCKED;
#if (DRV_SUPPORT_IST == ENABLE)
#else
static void (*pfTgeIntCb)(UINT32 uiIntStatus);

#endif
//static UINT32 g_uiTgeClockRate = 0;

typedef enum {
	TGE_ACTOP_OPEN          = 0,    ///< Open engine
	TGE_ACTOP_CLOSE,                ///< Close engine
	TGE_ACTOP_START,             ///< Start engine
	TGE_ACTOP_PARAM,             ///< Set parameter
	TGE_ACTOP_PAUSE,             ///< Pause engine
	TGE_ACTOP_CHGPARAM,      ///< Change dynamic parameter
	ENUM_DUMMY4WORD(TGE_ACTION_OP)
} TGE_ACTION_OP;
//@}

/*
    TGE Engine Status.

    Set TGE status.
*/
//@{
typedef enum {
	TGE_ENGINE_IDLE  = 0, ///< engine is idle
	TGE_ENGINE_READY = 1, ///< engine is ready
	TGE_ENGINE_PAUSE = 2, ///< engine is paused(ready to Run)
	TGE_ENGINE_RUN   = 3, ///< engine is running
	ENUM_DUMMY4WORD(TGE_ENGINE_STATUS)
} TGE_ENGINE_STATUS;
//@}

//@}

/**
TGE Internal API

@name   TGE_Internal
*/
//@{

//---------------------------------------------------------------------------------------------
// internal API
#if 0
void tge_create_resource(void)
{
	OS_CONFIG_FLAG(FLG_ID_TGE);
	SEM_CREATE(SEMID_TGE,1);
}
EXPORT_SYMBOL(tge_create_resource);

void tge_release_resource(void)
{
	rel_flg(FLG_ID_TGE);
	SEM_DESTROY(SEMID_TGE);
}
EXPORT_SYMBOL(tge_release_resource);
#endif

/*
    TGE Attach Function

    Pre-initialize driver required HW before driver open

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER tge_attach(void)
{
	//pll_enableClock(0);
	tge_platform_enable_clk();
	return E_OK;
}

/*
    TGE Detach Function

    De-initialize driver required HW after driver close

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
static ER tge_detach(void)
{
	//pll_disableClock(0);
	tge_platform_disable_clk();
	return E_OK;
}
#if 0
ER tge_setClock(TGE_CLKSRC_SEL ClkSrc)
{
	struct clk *parent_clk = NULL;

	// for CLK
	if (ClkSrc == CLKSRC_PXCLK) {
		//uiTempAddr = ioremap_nocache(0xf0020000, 0x160);
	    //WRITE_REG(((READ_REG(uiTempAddr + 0x18))&0xFF3FFFFF) | 0x00000000, uiTempAddr + 0x18);
		parent_clk = clk_get(NULL, "sie_io_pxclk");
	} else if (ClkSrc == CLKSRC_MCLK) {
		parent_clk = clk_get(NULL, "f0c00000.siemck");
	} else if (ClkSrc == CLKSRC_MCLK2) {
		nvt_dbg(ERR, "TGE CLK1 is not support MCLK2 !!\r\n");
	}

	clk_enable(tge_clk[0]);

	if (parent_clk == NULL){
		nvt_dbg(IND, "tge_setClock  fail!\r\n");
		return E_SYS;
	}

	if (!IS_ERR(parent_clk))
		clk_set_parent(tge_clk[0], parent_clk);

	clk_put(parent_clk);

	return E_OK;
}
ER tge_setClock2(TGE_CLKSRC_SEL ClkSrc2)
{
	struct clk *parent_clk = NULL;

	// for CLK2
	if (ClkSrc2 == CLKSRC_PXCLK) {
		//uiTempAddr = ioremap_nocache(0xf0020000, 0x160);
	    //WRITE_REG(((READ_REG(uiTempAddr + 0x18))&0x3FFFFFFF) | 0x00000000, uiTempAddr + 0x18);
		parent_clk = clk_get(NULL, "sie2_io_pxclk");
	} else if (ClkSrc2 == CLKSRC_MCLK) {
		nvt_dbg(ERR, "TGE CLK2 is not support MCLK !!\r\n");
	} else if (ClkSrc2 == CLKSRC_MCLK2) {
		parent_clk = clk_get(NULL, "f0c00000.siemk2");
	}

	clk_enable(tge_clk[1]);

	if (parent_clk == NULL){
		nvt_dbg(IND, "tge_setClock2  fail!\r\n");
		return E_SYS;
	}

	if (!IS_ERR(parent_clk))
		clk_set_parent(tge_clk[1], parent_clk);

	clk_put(parent_clk);

	return E_OK;
}
#endif

/*
    TGE Interrupt Service Routine

    TGE interrupt service routine

    @return None.
*/
void tge_isr(void)
{
	UINT32 inTgeIntStatus, inTgeIntEnable;
	UINT32 uiPtn;

	inTgeIntStatus = tge_getIntrStatus();
    inTgeIntEnable = tge_getIntEnable();
	inTgeIntStatus = inTgeIntStatus & inTgeIntEnable;
	if (inTgeIntStatus == 0) {
		return;
	}
	tge_clrIntrStatus(inTgeIntStatus);

	uiPtn = 0;

	if (inTgeIntStatus & TGE_INT_VD) {
		uiPtn |= FLGPTN_TGE_FRAMEEND;
	}
	if (inTgeIntStatus & TGE_INT_VD2) {
		uiPtn |= FLGPTN_TGE_FRAMEEND2;
	}
	if (inTgeIntStatus & TGE_INT_VD3) {
		uiPtn |= FLGPTN_TGE_FRAMEEND3;
	}
	if (inTgeIntStatus & TGE_INT_VD4) {
		uiPtn |= FLGPTN_TGE_FRAMEEND4;
	}
	if (inTgeIntStatus & TGE_INT_VD5) {
		uiPtn |= FLGPTN_TGE_FRAMEEND5;
	}
	if (inTgeIntStatus & TGE_INT_VD6) {
		uiPtn |= FLGPTN_TGE_FRAMEEND6;
	}
	if (inTgeIntStatus & TGE_INT_VD7) {
		uiPtn |= FLGPTN_TGE_FRAMEEND7;
	}
	if (inTgeIntStatus & TGE_INT_VD8) {
		uiPtn |= FLGPTN_TGE_FRAMEEND8;
	}
	if (inTgeIntStatus & TGE_INT_VD_BP) {
		uiPtn |= FLGPTN_TGE_VD_BP1;
	}
	if (inTgeIntStatus & TGE_INT_VD2_BP) {
		uiPtn |= FLGPTN_TGE_VD2_BP1;
	}
	if (inTgeIntStatus & TGE_INT_VD3_BP) {
		uiPtn |= FLGPTN_TGE_VD3_BP1;
	}
	if (inTgeIntStatus & TGE_INT_VD4_BP) {
		uiPtn |= FLGPTN_TGE_VD4_BP1;
	}
	if (inTgeIntStatus & TGE_INT_VD5_BP) {
		uiPtn |= FLGPTN_TGE_VD5_BP1;
	}
	if (inTgeIntStatus & TGE_INT_VD6_BP) {
		uiPtn |= FLGPTN_TGE_VD6_BP1;
	}
	if (inTgeIntStatus & TGE_INT_VD7_BP) {
		uiPtn |= FLGPTN_TGE_VD7_BP1;
	}
	if (inTgeIntStatus & TGE_INT_VD8_BP) {
		uiPtn |= FLGPTN_TGE_VD8_BP1;
	}
	//NOT_YET//if (inTgeIntStatus & TGE_INT_BP2                ) {  uiPtn |= FLGPTN_TGE_BP2;     }
	//NOT_YET//if (inTgeIntStatus & TGE_INT_ACTST              ) {     }
	//NOT_YET//if (inTgeIntStatus & TGE_INT_DRAM_IN_OUT_ERR    ) {  DBG_WRN("DRAM IO ERR\r\n");  }
	//NOT_YET// TBD. . .

#if (DRV_SUPPORT_IST == ENABLE)
	if (pfIstCB_TGE != NULL) {
		drv_setIstEvent(DRV_IST_MODULE_TGE, inTgeIntStatus);
	}
#else
	if (pfTgeIntCb) {
		pfTgeIntCb(inTgeIntStatus);
	}
#endif

	if (uiPtn != 0) {
		#if 0
		iset_flg(FLG_ID_TGE, uiPtn);
		#else
		tge_platform_flg_set(uiPtn);
		#endif
	}
}


//@}

/**
TGE External API

@name   TGE_External
*/
//@{

//---------------------------------------------------------------------------------------------
// external API



#if 0// driver policy: software-reset is limited
/*
    TGE Reset Function

    Reset TGE HW before

    @return None
*/
ER tge_reset(void)
{
	ER erReturn;

	erReturn = tge_stateMachine(TGE_ACTOP_SWRESET, FALSE);
	if (erReturn != E_OK) {
		DBG_ERR("reset is not allowed...\r\n");
		return erReturn;
	}

	tge_setReset(ENABLE);
	tge_setReset(DISABLE);

	tge_stateMachine(TGE_ACTOP_SWRESET, TRUE);

	return E_OK;
}
#endif

/**
    TGE Open Driver

    Open TGE engine

    @param[in] pObjCB TGE open object

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER tge_open(TGE_OPENOBJ *pObjCB)
{
	TGE_VD_RST_INFO TgeRst = {0};

	// SW reset, "a special version" should be operated here
	// It has to be done before power on
	// It has to be done leaving reset=1
	tge_setReset(ENABLE);

	// Power on module(power domain on)
	//pmc_turnonPower(PMC_MODULE_TGE);

	// Select PLL clock source
	#if 0
	tge_setClock(pObjCB->TgeClkSel);
	tge_setClock2(pObjCB->TgeClkSel2);
	#else
	tge_platform_set_clk_rate(pObjCB);
	#endif

	// Enable PLL clock source
	//pll_setPLLEn(PLL_ID_5, TRUE);

	// Attach(enable engine clock)
	tge_attach();

	// Enable engine interrupt
	//drv_enableInt(DRV_INT_TGE);

	// Clear engine interrupt status
	tge_clrIntrStatus(TGE_INT_ALL);

	// Set initial value "disalbe" to all VD/HD
	tge_setAllVdRst(&TgeRst);

	// Clear SW flag
	#if 0
	clr_flg(FLG_ID_TGE, FLGPTN_TGE_FRAMEEND);
	#else
	tge_platform_flg_clear(FLGPTN_TGE_FRAMEEND);
	#endif

	// Hook call-back function
	if (pObjCB->pfTgeIsrCb != NULL) {
#if (DRV_SUPPORT_IST == ENABLE)
		pfIstCB_TGE = pObjCB->pfTgeIsrCb;
#else
		pfTgeIntCb   = pObjCB->pfTgeIsrCb;
#endif
	}

	// SW reset, "a special version" should be operated before
	tge_setReset(ENABLE);
	//moved to setMode()//tge_setReset(DISABLE);

	return E_OK;
}
#if 0
/**
    TGE Get Open Status

    Check if TGE engine is opened

    @return
        - @b FALSE: engine is not opened
        - @b TRUE: engine is opened
*/
BOOL tge_isOpened(void)
{
	if (g_TgeLockStatus == TASK_LOCKED) {
		return TRUE;
	} else {
		return FALSE;
	}
}
#endif
/**
    TGE Close Driver

    Close TGE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER tge_close(void)
{
	// Disable engine interrupt
	//drv_disableInt(DRV_INT_TGE);

	// Detach(disable engine clock)
	tge_detach();

	return E_OK;
}

/**
    TGE Pause Operation

    Pause TGE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER tge_pause(void)
{
	// disable ACT_EN
	tge_setReset(ENABLE);//tge_enableFunction(DISABLE,TGE_ACT_EN);//tge_setStart(FALSE);
	tge_setLoad();

	return E_OK;
}

/**
    TGE Start Operation

    Start TGE engine

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER tge_start(void)
{
	// enable
	//tge_setVDReset(DISABLE); // VD enable
	tge_setReset(DISABLE);//tge_enableFunction(ENABLE,TGE_ACT_EN);//tge_setStart(TRUE);
	tge_setLoad();

	return E_OK;
}

/**
    TGE Set Mode

    Set TGE engine mode

    @param[in] pTgeParam TGE configuration.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER tge_setMode(TGE_MODE_PARAM *pTgeParam)
{
	// Configuration
	tge_setIntEnable(pTgeParam->uiIntrpEn);
	tge_setBasicSet(&(pTgeParam->BasicSetInfo));
	tge_setModeSel(&(pTgeParam->ModeSelInfo));
	tge_setAllVdRst(&(pTgeParam->VdRstInfo));
	tge_setVdHdInfo(&(pTgeParam->VdRstInfo), &(pTgeParam->VdHdInfo));
	tge_setBreadPoint(&(pTgeParam->VdRstInfo), &(pTgeParam->BreakPointInfo));

	// SW reset, "a special version" should be operated before for let reset=0
	//tge_setVDReset(ENABLE); // VD enable
	tge_setReset(ENABLE);//keep it//    tge_setReset(DISABLE);

	return E_OK;
}

/**
    Dynamic change TGE parameters

    @param[in] pParam Parameters.
    @param[in] FunSel Change function selection.

    @return
    -@b E_OK  Engine set mode and parameters without error.\n
    -@b E_PAR Egnine set parameters error.\n
*/
ER tge_chgParam(void *pParam, TGE_CHANGE_FUN_PARAM_SEL FunSel)
{
	ER erReturn=E_OK;

	if (pParam == NULL) {
		DBG_ERR("parameter NULL...\r\n");
		return E_PAR;
	}

	if (FunSel == TGE_CHG_VD_RST)            {
		tge_setVdRst(*((TGE_VD_RST_SEL *)pParam));
	}
	if (FunSel == TGE_CHG_VD2_RST)           {
		tge_setVd2Rst(*((TGE_VD_RST_SEL *)pParam));
	}


	if (FunSel == TGE_CHG_VD_PHASE)          {
		tge_setVdPhase((TGE_VD_PHASE *)pParam);
	}
	if (FunSel == TGE_CHG_VD2_PHASE)         {
		tge_setVd2Phase((TGE_VD_PHASE *)pParam);
	}


	if (FunSel == TGE_CHG_VD_INV)            {
		tge_setVdInv((TGE_VD_INV *)pParam);
	}
	if (FunSel == TGE_CHG_VD2_INV)           {
		tge_setVd2Inv((TGE_VD_INV *)pParam);
	}


	if (FunSel == TGE_CHG_VD_MODE)           {
		tge_setVdMode(*((TGE_MODE_SEL *)pParam));
	}
	if (FunSel == TGE_CHG_VD2_MODE)          {
		tge_setVd2Mode(*((TGE_MODE_SEL *)pParam));
	}

	if (FunSel == TGE_CHG_VDHD)              {
		tge_setVdHd((TGE_VDHD_INFO *)pParam);
	}
	if (FunSel == TGE_CHG_VD2HD2)            {
		tge_setVd2Hd2((TGE_VDHD_INFO *)pParam);
	}

	if (FunSel == TGE_CHG_TIMING_VD_PAUSE)   {
		tge_setVdTimingPause((TGE_TIMING_PAUSE_INFO *)pParam);
	}
	if (FunSel == TGE_CHG_TIMING_VD2_PAUSE)  {
		tge_setVd2TimingPause((TGE_TIMING_PAUSE_INFO *)pParam);
	}

	if (FunSel == TGE_CHG_VD_BP)             {
		tge_setVdBreadPoint(*((UINT32 *)pParam));
	}
	if (FunSel == TGE_CHG_VD2_BP)            {
		tge_setVd2BreadPoint(*((UINT32 *)pParam));
	}

	if (FunSel == TGE_CHG_CLK1)    {
		tge_setClock(*((TGE_CLKSRC_SEL *)pParam));
	}
	if (FunSel == TGE_CHG_CLK2)    {
		tge_setClock2(*((TGE_CLKSRC_SEL *)pParam));
	}

	tge_setLoad();

	return erReturn;
}

/**
    Dynamic change TGE Rst (Multiple)

    @param[in] pParam Parameters.
    @param[in] FunSel Change function selection.

    @return
    -@b E_OK  Engine set mode and parameters without error.\n
    -@b E_PAR Egnine set parameters error.\n
*/
ER tge_chgRst(TGE_VD_RST_INFO *pParam, TGE_CHANGE_RST_SEL FunSel)
{
	ER erReturn=E_OK;

	if (pParam == NULL) {
		DBG_ERR("parameter NULL...\r\n");
		return E_PAR;
	}

	tge_setRst(pParam, FunSel);
	tge_setLoad();

	return erReturn;

}

/**
    Dynamic change TGE VD/HD Info (Multiple)

    @param[in] pParam Parameters.
    @param[in] FunSel Change function selection.

    @return
    -@b E_OK  Engine set mode and parameters without error.\n
    -@b E_PAR Egnine set parameters error.\n
*/
ER tge_chgVdHd(TGE_VDHD_SET *pParam, TGE_CHANGE_VDHD_SEL FunSel)
{
	ER erReturn=E_OK;

	if (pParam == NULL) {
		DBG_ERR("parameter NULL...\r\n");
		return E_PAR;
	}

	if (FunSel & TGE_CHG_VDINFO)   {
		tge_setVdHd(&(pParam->VdInfo));
	}
	if (FunSel & TGE_CHG_VD2INFO)  {
		tge_setVd2Hd2(&(pParam->Vd2Info));
	}

	tge_setLoad();

	return erReturn;

}
/**
    TGE Wait VD End

    Wait for VD end flag.

    @param[in] uiVdCnt number of VD ends to wait after clearing flag.

    @return None.
*/
ER tge_waitEvent(TGE_WAIT_EVENT_SEL WaitEvent, BOOL bClrFlag)
{
	UINT32 uiFLAG;
	FLGPTN uiFlag;

	switch (WaitEvent) {
	case TGE_WAIT_VD:
		uiFLAG = FLGPTN_TGE_FRAMEEND;
		break;
	case TGE_WAIT_VD2:
		uiFLAG = FLGPTN_TGE_FRAMEEND2;
		break;
	case TGE_WAIT_VD3:
		uiFLAG = FLGPTN_TGE_FRAMEEND3;
		break;
	case TGE_WAIT_VD4:
		uiFLAG = FLGPTN_TGE_FRAMEEND4;
		break;
	case TGE_WAIT_VD5:
		uiFLAG = FLGPTN_TGE_FRAMEEND5;
		break;
	case TGE_WAIT_VD6:
		uiFLAG = FLGPTN_TGE_FRAMEEND6;
		break;
	case TGE_WAIT_VD7:
		uiFLAG = FLGPTN_TGE_FRAMEEND7;
		break;
	case TGE_WAIT_VD8:
		uiFLAG = FLGPTN_TGE_FRAMEEND8;
		break;
	case TGE_WAIT_VD_BP1:
		uiFLAG = FLGPTN_TGE_VD_BP1;
		break;
	case TGE_WAIT_VD2_BP1:
		uiFLAG = FLGPTN_TGE_VD2_BP1;
		break;
	case TGE_WAIT_VD3_BP1:
		uiFLAG = FLGPTN_TGE_VD3_BP1;
		break;
	case TGE_WAIT_VD4_BP1:
		uiFLAG = FLGPTN_TGE_VD4_BP1;
		break;
	case TGE_WAIT_VD5_BP1:
		uiFLAG = FLGPTN_TGE_VD5_BP1;
		break;
	case TGE_WAIT_VD6_BP1:
		uiFLAG = FLGPTN_TGE_VD6_BP1;
		break;
	case TGE_WAIT_VD7_BP1:
		uiFLAG = FLGPTN_TGE_VD7_BP1;
		break;
	case TGE_WAIT_VD8_BP1:
		uiFLAG = FLGPTN_TGE_VD8_BP1;
		break;
	default:
		DBG_ERR("no such signal flag\r\n");
		return E_SYS;
		break;
	}

	if (bClrFlag == TRUE) {
		#if 0
		clr_flg(FLG_ID_TGE, uiFLAG);
		#else
		tge_platform_flg_clear(uiFLAG);
		#endif
	}
	#if 0
	wai_flg(&uiFlag, FLG_ID_TGE, uiFLAG, TWF_CLR | TWF_ORW);
	#else
	tge_platform_flg_wait(&uiFlag, uiFLAG);
	#endif
	return E_OK;
}

#if 0
/**

    TGE Get clock rate

    Get Engine clock

    @return None.
*/
UINT32 tge_getClockRate(void)
{
	return g_uiTgeClockRate;

}
#endif

void tge_reg_isr_cb(TGE_DRV_ISR_FP fp)
{
	if (fp != NULL) {
		pfTgeIntCb = fp;
	}
}

//@}

//@}


