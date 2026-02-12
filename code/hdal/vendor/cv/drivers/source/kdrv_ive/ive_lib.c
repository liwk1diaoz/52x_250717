/*
    IVE module driver

    NT96680 IVE driver.

    @file       ive_lib.c
    @ingroup    mIDrvIPPIVE

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/

#include "ive_platform.h"
#include "ive_lib.h"
#include "ive_int.h"//header for IVE: internal enum/struct/api/variable

static IVE_STATE_MACHINE g_IveEngineState = IVE_ENGINE_IDLE;

static void (*g_pIveCBFunc)(UINT32 IntStatus, VOID *ive_trig_hdl) = NULL;

// interrupt flag
#define FLGPTN_IVE_FRAMEEND     FLGPTN_BIT(0)
#define FLGPTN_IVE_LLEND     	FLGPTN_BIT(8)
#define FLGPTN_IVE_LLERROR     	FLGPTN_BIT(9)

#define IVE_DMA_CACHE_HANDLE (1)

UINT32 g_uiIveFrmEndIntCnt = 0;
UINT32 _IVE_REG_BASE_ADDR;
UINT32 g_2sCompEdgeCoeff[EDGE_COEFF_NUM] = {0};
static KDRV_IVE_HANDLE *g_lib_ive_trig_hdl = 0;

/*
    IVE statemachine

    Operation & status check mechanism for IVE

    @param[in] IVE operation
    @param[in] State update option

    @return ER  Error code to check the status of the desired operation\n
    -@b E_OK  The desired operation is allowed.\n
    -@b E_OACV  The desired operation is not allowed.\n
*/
ER ive_stateMachine(IVE_ACTION_OP IveOp, BOOL bUpdate)
{
	ER erReturn = E_OK;
	switch (g_IveEngineState) {
	case IVE_ENGINE_IDLE :
		switch (IveOp) {
		case IVE_OP_OPEN:
			if (bUpdate) {
				g_IveEngineState = IVE_ENGINE_READY;
			}
			erReturn = E_OK;
			break;
		default:
			erReturn = E_OBJ;
			DBG_ERR("Operation %d illegal in IDLE state!\r\n", IveOp);
		}
		break;
	case IVE_ENGINE_READY:
		switch (IveOp) {
		case IVE_OP_CLOSE:
			if (bUpdate) {
				g_IveEngineState = IVE_ENGINE_IDLE;
			}
			erReturn = E_OK;
			break;
		case IVE_OP_SETPARAM:
			if (bUpdate) {
				g_IveEngineState = IVE_ENGINE_PAUSE;
			}
			erReturn = E_OK;
			break;
		default:
			erReturn = E_OBJ;
			DBG_ERR("Operation %d illegal in READY state!\r\n", IveOp);
		}
		break;
	case IVE_ENGINE_PAUSE:
		switch (IveOp) {
		case IVE_OP_CLOSE:
			if (bUpdate) {
				g_IveEngineState = IVE_ENGINE_IDLE;
			}
			erReturn = E_OK;
			break;
		case IVE_OP_SETPARAM:
			if (bUpdate) {
				g_IveEngineState = IVE_ENGINE_PAUSE;
			}
			erReturn = E_OK;
			break;
		case IVE_OP_START:
			if (bUpdate) {
				g_IveEngineState = IVE_ENGINE_RUN;
			}
			erReturn = E_OK;
			break;
		case IVE_OP_PAUSE:
			break;
		default:
			erReturn = E_OBJ;
			DBG_ERR("Operation %d illegal in PAUSE state!\r\n", IveOp);
		}
		break;
	case IVE_ENGINE_RUN  :
		switch (IveOp) {
		case IVE_OP_PAUSE:
			if (bUpdate) {
				g_IveEngineState = IVE_ENGINE_PAUSE;
			}
			erReturn = E_OK;
			break;
		default:
			erReturn = E_OBJ;
			DBG_ERR("Operation %d illegal in RUN state!\r\n", IveOp);
		}
		break;
	default:
		erReturn = E_OBJ;
	}
	return erReturn;
}

void ive_create_resource(void *parm, UINT32 clk_freq)
{
	ive_platform_create_resource((VOID *)parm, clk_freq);
}

void ive_release_resource(void *parm)
{
	ive_platform_release_resource((VOID *)parm);
}

void ive_set_base_addr(UINT32 uiAddr)
{
	_IVE_REG_BASE_ADDR = uiAddr;
}


/*
  IVE get interrupt status.

  Check IVE interrupt status.

  @return IVE interrupt status readout.
*/
static UINT32 ive_getIntStatus(VOID)
{
	return ive_getIntrStatusReg();
}

/*
  IVE enable interrupt driver

  IVE enable selected interrput

  @param[in] uiIntrEn Selection of interrupt

  @return
        - @b E_OK: Done with no error.
        - Others:  Error occured.
*/
static ER ive_enableInt(UINT32 uiIntrEn)
{
	ER erReturn;

 	erReturn = ive_platform_enableInt(uiIntrEn);

	return erReturn;
}

/*
  IVE clear all interrupt status.

  @param[in] uiIntrStatus Write 1 to clear the corresponding status of interrupt

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
static ER ive_clearIntStatus(UINT32 uiIntrStatus)
{
	ive_clrIntrStatusReg(uiIntrStatus);

	return E_OK;
}

/**
  IVE wait frame end

  Wait IVE Process done

  @return
        - @b E_OK: Done with no error
        - Others: Error occurs
*/
ER ive_waitFrameEnd(VOID)
{
	ER erReturn = E_OK;
	FLGPTN uiFlag;

	erReturn = ive_platform_flg_wait(&uiFlag, FLGPTN_IVE_FRAMEEND);
	if (erReturn != E_OK) {
        DBG_ERR("Error! %s_%d: ive_platform_flg_wait() %d\n", __FUNCTION__, __LINE__, erReturn);
		goto ret_end;
    }

ret_end:
	return erReturn;
}

/**
  IVE wait frame end with timeout

  Wait IVE Process done

  @return
        - @b E_OK: Done with no error
        - Others: Error occurs
*/
ER ive_waitFrameEnd_timeout(UINT32 timeout_ms)
{
	ER erReturn = E_OK;
	FLGPTN uiFlag;

	erReturn = ive_platform_flg_wait_timeout(&uiFlag, FLGPTN_IVE_FRAMEEND, timeout_ms);
	if (erReturn != E_OK) {
        DBG_IND("Error! %s_%d: ive_platform_flg_wait() %d\n", __FUNCTION__, __LINE__, erReturn);
		goto ret_end;
    }

ret_end:
	return erReturn;
}

void ive_waitFrameEnd_LL(BOOL is_clr_flag)
{
	FLGPTN uiFlag;
	ER erReturn = E_OK;

	if (is_clr_flag == TRUE) {
		erReturn = ive_platform_flg_clear((UINT32) FLGPTN_IVE_LLEND);
		if (erReturn != E_OK) {
			DBG_ERR("Error! %s_%d: ive_platform_flg_clear() %d\n", __FUNCTION__, __LINE__, erReturn);
			goto ret_end;
		}
	}

	erReturn = ive_platform_flg_wait(&uiFlag, FLGPTN_IVE_LLEND);
	if (erReturn != E_OK) {
        DBG_ERR("Error! %s_%d: ive_platform_flg_wait() %d\n", __FUNCTION__, __LINE__, erReturn);
		goto ret_end;
    }

ret_end:

	return;
}


/**
  IVE clear frame end flag

  clear fream end flag

  @return
      - @b E_OK: Done with no error
      - Others: Error occurs
*/
ER ive_clearFrameEndFlag(VOID)
{
	ER erReturn = E_OK;

	erReturn = ive_platform_flg_clear((UINT32) FLGPTN_IVE_FRAMEEND);

	return erReturn;
}

void ive_clearFrameEndFlag_LL(void)
{
	ER erReturn = E_OK;
	
	erReturn = ive_platform_flg_clear((UINT32) FLGPTN_IVE_LLEND);
	if (erReturn != E_OK) {
        DBG_ERR("Error! %s_%d: ive_platform_flg_clear() %d\n", __FUNCTION__, __LINE__, erReturn);
    }

	erReturn = ive_platform_flg_clear((UINT32) FLGPTN_IVE_LLERROR);
	if (erReturn != E_OK) {
        DBG_ERR("Error! %s_%d: ive_platform_flg_clear() %d\n", __FUNCTION__, __LINE__, erReturn);
    }
}

/**
    IVE get operation clock

    Get IVE clock selection.

    @param[in] clock setting.

    @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
UINT32 ive_get_clock_rate(VOID)
{
	UINT32 clk_freq_mhz;

	clk_freq_mhz = ive_platform_get_clock_rate();

	return clk_freq_mhz;
}

/*
    IVE set clock rate

    Map clock rate value to supported enum
*/
VOID ive_setClockRate(UINT32 uiIveClockRate)
{
	ER erReturn = E_OK;

	erReturn = ive_platform_set_clk_rate(uiIveClockRate);
	if (erReturn != E_OK) {
        DBG_ERR("Error! %s_%d: ive_platform_set_clk_rate(%d) %d\n", __FUNCTION__, __LINE__, uiIveClockRate, erReturn);
    }
}

/*
    IVE set function enabel driver

    set all functions' enable and options

    @param[in] FuncEn IVE output control structure pointer

    @return
        -@b E_OK: setting success
*/
static ER ive_setFuncEn(IVE_FUNC_EN *FuncEn)
{
	ER erReturn = E_OK;

	if(nvt_get_chip_id() == CHIP_NA51055) {
	} else {
		if (FuncEn->bGenFiltEn && FuncEn->bIrvEn) {
			DBG_ERR("Error! General filter & IRV can not both be enabled.\n");
			return E_PAR;
		}
	}

	if (FuncEn->bIntegralEn && (FuncEn->bNonMaxEn || FuncEn->bThresLutEn || FuncEn->bMorphFiltEn)) {
		DBG_ERR("Error! Integral image & (non-maximal/threshold LUT/morph filter) can not both be enabled.\n");
		return E_PAR;
	}

	ive_setGenFiltEnReg(FuncEn->bGenFiltEn);
	ive_setMednFiltEnReg(FuncEn->bMednFiltEn);
	ive_setEdgeFiltEnReg(FuncEn->bEdgeFiltEn);
	ive_setNonMaxSupEnReg(FuncEn->bNonMaxEn);
	ive_setThresLutEnReg(FuncEn->bThresLutEn);
	ive_setMorphFiltEnReg(FuncEn->bMorphFiltEn);
	ive_setIntegralEnReg(FuncEn->bIntegralEn);
	ive_setIrvEnReg(FuncEn->bIrvEn);
	return erReturn;
}

VOID ive_setSWReset(VOID)
{
	ive_setResetReg(TRUE);
	ive_setResetReg(FALSE);
	DBG_ERR("SW RESET!\r\n");
}

static ER ive_setMorphInSel(BOOL bMorphEn, UINT32 uiMorphInSel)
{
	if (bMorphEn == FALSE && uiMorphInSel != TH_LUT_IN) {
		DBG_ERR("Morphological filter is disabled, MORPH_IN_SEL should be %d!\r\n", TH_LUT_IN);
		return E_PAR;
	}
	ive_setMorphInSelReg(uiMorphInSel);

	return E_OK;
}

/**
    IVE chkDMAInInfo driver

    Check input dma info

    @param[in] uiInAddr DRAM address

    @return
        - @b E_OK: Info is correct
        - Others: Info is incorrect
*/
static ER ive_chkDMAInInfo(UINT32 uiInAddr, UINT32 uiInLofst)
{
	ER erReturn = E_OK;

	if (uiInAddr == 0) { // address = 0
		DBG_ERR("Input Address might be used before setting!!\r\n");
		erReturn = E_PAR;
		return erReturn;
	}
	if (uiInAddr % 4 != 0) { // address is not word align
		DBG_ERR("Input address is not word align!!\r\n");
		erReturn = E_PAR;
		return erReturn;
	}
	if (uiInLofst % 4 != 0) { // line offset is not word align
		DBG_ERR("Input line offset is not word align!!\r\n");
		erReturn = E_PAR;
		return erReturn;
	}

	return erReturn;
}

/**
    IVE chkDMAOutInfo driver

    Check output dma info

    @param[in] uiOutAddr DRAM output address

    @param[in] uiOutOfst DRAM output line offset

    @return
        - @b E_OK: Info is correct
        - Others: Info is incorrect
*/
static ER ive_chkDMAOutInfo(UINT32 uiOutAddr, UINT32 uiOutOfst)
{
	ER erReturn = E_OK;

	if (uiOutAddr == 0) { // address = 0
		DBG_ERR("Output Address = 0x%x, need to assign!!\r\n", uiOutAddr);
		erReturn = E_PAR;
		return erReturn;
	}
	if (uiOutAddr % 4 != 0) { // address is not word align
		DBG_ERR("Output address is not word align!!\r\n");
		erReturn = E_PAR;
		return erReturn;
	}
	if (uiOutOfst % 4 != 0) { // line offset is not word align
		DBG_ERR("Output line offset is not word align!!\r\n");
		erReturn = E_PAR;
		return erReturn;
	}

	return erReturn;
}

/**
    IVE get Dma input address

    Get input address

    @return
        - input address
*/
UINT32 ive_getDmaInAddr(VOID)
{
	return ive_getInAddrReg();
}

/**
    IVE get Dma output address

    Get output address

    @return
        - output address
*/
UINT32 ive_getDmaOutAddr(VOID)
{
	return ive_getOutAddrReg();
}

/**
    IVE open driver

    Open IVE engine

    @param[in] pIveObjCB IVE open object

    @return
        - @b E_OK: Done with no error
        - Others: Error occurs
*/
ER ive_open(IVE_OPENOBJ *pIveObjCB)
{
	ER erReturn = E_OK;

	// lock semaphore
	erReturn = ive_lock();
	if (erReturn != E_OK) {
		DBG_ERR("Error at %s_%d\r\n", __FUNCTION__, __LINE__);
		return erReturn;
	}

	// check state machine, no update
	erReturn = ive_stateMachine(IVE_OP_OPEN, FALSE);
	if (erReturn != E_OK) {
		DBG_ERR("Error at %s_%d\r\n", __FUNCTION__, __LINE__);
		return erReturn;
	}

	// Turn on power
	//pmc_turnonPower(PMC_MODULE_IVE);//no need for now

	// select IVE clock source
	ive_setClockRate(pIveObjCB->IVE_CLOCKSEL);//trans clock value to definition

	// enable engine clock
	erReturn = ive_attach();
	if (erReturn != E_OK) {
		DBG_ERR("Error at %s_%d\r\n", __FUNCTION__, __LINE__);
		return erReturn;
	}

	//Sram shutdown test
	ive_platform_disable_sram_shutdown();

	// enable interrupt
	ive_platform_int_enable();

	erReturn = ive_clearIntStatus(IVE_INT_ALL);
	if (erReturn != E_OK) {
		DBG_ERR("Error at %s_%d\r\n", __FUNCTION__, __LINE__);
		return erReturn;
	}

	// clear SW flag
	erReturn = ive_platform_flg_clear((UINT32) FLGPTN_IVE_FRAMEEND);
	if (erReturn != E_OK) {
		DBG_ERR("Error! %s_%d: ive_platform_flg_clear() %d\n", __FUNCTION__, __LINE__, erReturn);
		return erReturn;
	}

	// software reset
	erReturn = ive_setReset();
	if (erReturn != E_OK) {
		DBG_ERR("Error at %s_%d\r\n", __FUNCTION__, __LINE__);
		return erReturn;
	}

	//Hook call-back function
//#ifdef __KERNEL__
	if (pIveObjCB->FP_IVEISR_CB) {
		g_pIveCBFunc = pIveObjCB->FP_IVEISR_CB;
	}
//#else
//	pfIstCB_DCE = pIveObjCB->FP_IVEISR_CB;
//#endif

	// check state machine & update state
	erReturn = ive_stateMachine(IVE_OP_OPEN, TRUE);
	if (erReturn != E_OK) {
		DBG_ERR("Error at %s_%d\r\n", __FUNCTION__, __LINE__);
		return erReturn;
	}

	return erReturn;
}

static ER ive_check_outselection(IVE_PARAM *pIveParam){
	ER erReturn = E_OK;
	switch(pIveParam->uiOutDataSel) {
		case EDGE_1B:
		case EDGE_4B:
			if(pIveParam->FuncEn.bMorphFiltEn == FALSE && pIveParam->FuncEn.bThresLutEn == FALSE)
            {
                DBG_ERR("Out edge 1,4 bit: Morphological filter and threshold LUT can't both be disabled!\n");
                return E_PAR;
            }
            if(pIveParam->FuncEn.bMorphFiltEn == TRUE && pIveParam->uiMorphInSel == TH_LUT_IN && pIveParam->FuncEn.bThresLutEn == FALSE)
            {
                DBG_ERR("Out edge 1,4 bit: Morphological filter source is threshold LUT but LUT is disabled!\n");
                return E_PAR;
            }
			if(pIveParam->FuncEn.bIntegralEn == TRUE){
				DBG_ERR("Out edge 1,4 bit: integral image enable should be disabled\n");
                return E_PAR;
			}
            break;
		case IMG_MEDIAN_8B:	
			if(pIveParam->FuncEn.bEdgeFiltEn != FALSE ||
            pIveParam->FuncEn.bNonMaxEn != FALSE ||
            pIveParam->FuncEn.bThresLutEn != FALSE ||
            pIveParam->FuncEn.bMorphFiltEn != FALSE ||
            pIveParam->FuncEn.bIntegralEn != FALSE)
            {
                DBG_ERR("Out median 8 bit: functions which have no use are not all disabled!\n");
                return E_PAR;
            }
            break;
		case IMG_NONMAX_8B:	
			if(pIveParam->FuncEn.bThresLutEn != FALSE ||
            pIveParam->FuncEn.bMorphFiltEn != FALSE ||
            pIveParam->FuncEn.bIntegralEn != FALSE)
            {
                DBG_ERR("Out non-max 8 bit: functions which have no use are not all disabled!\n");
                return E_PAR;
            }
            break;
		case EDGE_MAG_THETA_12B:	
		case EDGE_X_Y_16B:	
			if(pIveParam->FuncEn.bNonMaxEn != FALSE ||
            pIveParam->FuncEn.bThresLutEn != FALSE ||
            pIveParam->FuncEn.bMorphFiltEn != FALSE ||
            pIveParam->FuncEn.bIntegralEn != FALSE)
            {
                DBG_ERR("Out edge 12,16 bit: functions which have no use are not all disabled!\n");
                return E_PAR;
            }
            if(pIveParam->FuncEn.bEdgeFiltEn == TRUE && pIveParam->EdgeF.uiEdgeMode == NO_DIR)
            {
                DBG_ERR("Out edge 12,16 bit: Edge filter mode can not be no direction!\n");
                return E_PAR;
            }
            break;
		case INTEGRAL_32B:	
			if(pIveParam->FuncEn.bNonMaxEn != FALSE ||
            pIveParam->FuncEn.bThresLutEn != FALSE ||
            pIveParam->FuncEn.bMorphFiltEn  != FALSE ||
			pIveParam->FuncEn.bEdgeFiltEn  != FALSE  )
            {
                DBG_ERR("Out integral 32 bit: functions which have no use are not all disabled!\n");
                return E_PAR;
            }
			if(pIveParam->FuncEn.bIntegralEn != TRUE){
				DBG_ERR("Out integral 32 bit: integral image enable should not be disabled!\n");
                return E_PAR;
			}
			break;
		 default:
            DBG_ERR("Unsupported output data type!\n");
			break;
	}
	return erReturn;
}
/**
    IVE setMode driver

    Set all writable register value

    @param[in] pIveParamObj IVE parameter object

    @return
        - @b E_OK: Register setting done
        - Others: Error occured.
*/
ER ive_setMode(IVE_PARAM *pIveParam)
{
	ER erReturn = E_OK;
	UINT32 uiInAddr = 0, uiInSize = 0;
	UINT32 uiOutAddr = 0, uiOutOfst = 0, uiOutSize = 0;
	UINT32  uiInAddr_pa = 0, uiOutAddr_pa = 0;
	UINT32 ret;

	erReturn = ive_stateMachine(IVE_OP_SETPARAM, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	erReturn = ive_check_outselection(pIveParam) ;
	if (erReturn != E_OK) return erReturn;	

	//disable all interrupt
	ive_setIntrEnableReg(0x00000000);

	

	//set function enable & output padding parameter
	erReturn = ive_setFuncEn(&(pIveParam->FuncEn));
	if (erReturn != E_OK) return erReturn;
	

	//set input data flow
	erReturn = ive_setMorphInSel(pIveParam->FuncEn.bMorphFiltEn, pIveParam->uiMorphInSel);
	if (erReturn != E_OK) return erReturn;
	

	//set output data flow
	ive_setOutDataReg(pIveParam->uiOutDataSel);

	

	//set input image size
	ive_setInImgWidthReg(pIveParam->InSize.uiWidth);
	ive_setInImgHeightReg(pIveParam->InSize.uiHeight);

	

	//set in/out burst length
	ive_setInBurstLengthReg((UINT32) pIveParam->InBurstSel);
	ive_setOutBurstLengthReg((UINT32) pIveParam->OutBurstSel);

	

	//set debug port
	ive_setDbgPortReg((UINT32) pIveParam->DbgPortSel);

	

	//input
	uiInSize = pIveParam->DmaIn.uiInLofst * pIveParam->InSize.uiHeight; // frame mode
#if IVE_IOREMAP_IN_KERNEL
	uiInAddr_pa = pIveParam->DmaIn.uiInSaddr;
	uiInAddr = ive_platform_pa2va_remap(uiInAddr_pa, uiInSize, 2);
#else
	uiInAddr = pIveParam->DmaIn.uiInSaddr;
	pIveParam->flowCT.dma_do_not_sync = 1 ;
#endif
	erReturn = ive_chkDMAInInfo(uiInAddr, pIveParam->DmaIn.uiInLofst);
	if (erReturn != E_OK) {
		DBG_ERR("Check input Dma info fail!\r\n");
#if IVE_IOREMAP_IN_KERNEL
		ive_platform_pa2va_unmap(uiInAddr, uiInAddr_pa);
#endif
		return erReturn;
	}
   
	if (pIveParam->flowCT.dma_do_not_sync == 0) {
		ret = ive_platform_dma_flush_mem2dev((UINT32) uiInAddr, uiInSize);
		if (ret != 0) {
			DBG_ERR("Error! %s_%d: ive_platform_dma_flush() %d\n", __FUNCTION__, __LINE__, ret);
			erReturn = E_PAR;
	#if IVE_IOREMAP_IN_KERNEL
			ive_platform_pa2va_unmap(uiInAddr, uiInAddr_pa);
	#endif
			return erReturn;
		}
	}
	//modify no remap
	/*
#if IVE_IOREMAP_IN_KERNEL
	ive_platform_pa2va_unmap(uiInAddr, uiInAddr_pa);
#else
	uiInAddr_pa = ive_platform_va2pa(uiInAddr);
#endif*/

	
    uiInAddr_pa = uiInAddr;
	ive_setInAddrReg(uiInAddr_pa);//pIveParam->DmaIn.uiInSaddr);
	ive_setInLofstReg(pIveParam->DmaIn.uiInLofst);

	// output
	uiOutOfst = pIveParam->DmaOut.uiOutLofst;
	uiOutSize = pIveParam->DmaOut.uiOutLofst * pIveParam->InSize.uiHeight;
#if IVE_IOREMAP_IN_KERNEL
	uiOutAddr_pa = pIveParam->DmaOut.uiOutSaddr;	
	uiOutAddr = ive_platform_pa2va_remap(uiOutAddr_pa, uiOutSize, 2);
#else
	uiOutAddr = pIveParam->DmaOut.uiOutSaddr;
#endif
	erReturn = ive_chkDMAOutInfo(uiOutAddr, uiOutOfst);
	if (erReturn != E_OK) {
		DBG_ERR("Check output Dma info fail!\r\n");
#if IVE_IOREMAP_IN_KERNEL
		ive_platform_pa2va_unmap(uiOutAddr, uiOutAddr_pa);
#endif
		return erReturn;
	}

	
	//New Dram data update to cache
	if (pIveParam->flowCT.dma_do_not_sync == 0) {
		ret = ive_platform_dma_flush((UINT32) uiOutAddr, uiOutSize);
		if (ret != 0) {
			DBG_ERR("Error! %s_%d: ive_platform_dma_flush() %d\n", __FUNCTION__, __LINE__, ret);
			erReturn = E_PAR;
	#if IVE_IOREMAP_IN_KERNEL
			ive_platform_pa2va_unmap(uiOutAddr, uiOutAddr_pa);
	#endif
			return erReturn;
		}
	}
	//modify  no remap
	/*
#if IVE_IOREMAP_IN_KERNEL
	ive_platform_pa2va_unmap(uiOutAddr, uiOutAddr_pa);
#else
	uiOutAddr_pa = ive_platform_va2pa(uiOutAddr);
#endif
	*/
	
	uiOutAddr_pa = uiOutAddr;
	ive_setOutAddrReg(uiOutAddr_pa);
	ive_setOutLofstReg(pIveParam->DmaOut.uiOutLofst);
    
	

	// set general filter parameters
	if (pIveParam->FuncEn.bGenFiltEn) {
		ive_setGenFiltCoeffReg(pIveParam->GenF.puiGenCoeff);
	}

	

	// set median filter parameters
	if (pIveParam->FuncEn.bMednFiltEn) {
		ive_setMednFiltModeReg(pIveParam->uiMednMode);
		
	}

	

	// set edge filter parameters
	if (pIveParam->FuncEn.bEdgeFiltEn) {
		INT32 i;
		if (pIveParam->EdgeF.uiEdgeMode == NO_DIR) {
			for (i = 0; i < EDGE_COEFF_NUM; i++) {
				//transform negative number to its 2's complement
				g_2sCompEdgeCoeff[i] = (UINT32)pIveParam->EdgeF.pEdgeCoeff1[i];
			}
			ive_setEdge1CoeffReg(g_2sCompEdgeCoeff);
		}
		ive_setEdgeFiltModeReg(pIveParam->EdgeF.uiEdgeMode);
		ive_setEdgeShiftReg(pIveParam->EdgeF.uiEdgeShiftBit);
		ive_setEdgeAngSlpFactReg(pIveParam->EdgeF.uiAngSlpFact);

		if (pIveParam->EdgeF.uiEdgeMode == BI_DIR) {
			for (i = 0; i < EDGE_COEFF_NUM; i++) {
				//transform negative number to its 2's complement
				g_2sCompEdgeCoeff[i] = (UINT32)pIveParam->EdgeF.pEdgeCoeff1[i];
			}
			ive_setEdge1CoeffReg(g_2sCompEdgeCoeff);
			for (i = 0; i < EDGE_COEFF_NUM; i++) {
				//transform negative number to its 2's complement
				g_2sCompEdgeCoeff[i] = (UINT32)pIveParam->EdgeF.pEdgeCoeff2[i];
			}
			ive_setEdge2CoeffReg(g_2sCompEdgeCoeff);
		}
	}

	

	if (pIveParam->FuncEn.bNonMaxEn) {
		ive_setEdgeMagThReg(pIveParam->uiEdgeMagTh);
	}

	

	// set threshold LUT parameters
	if (pIveParam->FuncEn.bThresLutEn) {
		ive_setThresLutReg(pIveParam->Thres.puiThresLut);
	}

	

	// set morphological filter parameters
	if (pIveParam->FuncEn.bMorphFiltEn) {
		ive_setMorphOpReg(pIveParam->MorphF.uiMorphOp);
		ive_setMorphNeighEnReg(pIveParam->MorphF.pbMorphNeighEn);
	}

	// set IRVA
	if(nvt_get_chip_id() == CHIP_NA51055) {
	} else {
		ive_setIrvMednInvalThReg(pIveParam->Irv.uiMednInvalTh);
		ive_setIrvEnReg(pIveParam->FuncEn.bIrvEn);
		ive_setIrvHistModeSelReg(pIveParam->Irv.uiHistMode);
		ive_setIrvInvalidValReg(pIveParam->Irv.uiInvalidVal);
		ive_setIrvThrSReg(pIveParam->Irv.uiThrS);
		ive_setIrvThrHReg(pIveParam->Irv.uiThrH);
	}

	//set interrupt
	ive_clearIntStatus(IVE_INT_ALL);
	ive_enableInt(IVE_INTE_ALL);

	

	erReturn = ive_stateMachine(IVE_OP_SETPARAM, TRUE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	erReturn = E_OK;

	return erReturn;
}

ER ive_ll_setmode(UINT32 ll_addr)
{
	UINT32 uiAddr6=0, uiPhyAddr6;
	UINT32 ret;

	ER erReturn;
	erReturn = ive_stateMachine(IVE_OP_SETPARAM, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}
	// Clear engine interrupt status
	uiAddr6 = ll_addr;
	if(uiAddr6!=0) {
		uiPhyAddr6 = ive_platform_va2pa(uiAddr6);
	}
	else uiPhyAddr6 = 0;
	if (uiAddr6 != 0) {
		ret = ive_platform_dma_flush_mem2dev((UINT32)uiAddr6, (UINT32)(8*28*50));
        if (ret != 0) {
            DBG_ERR("Error! %s_%d: ive_platform_dma_flush() %d\n", __FUNCTION__, __LINE__, ret);
            erReturn = E_PAR;
            return erReturn;
        }
	}
	ive_setLLAddrReg(uiPhyAddr6);

	ive_clearIntStatus(IVE_INT_ALL);
	ive_enableInt(IVE_INTE_ALL);

	erReturn = ive_ll_pause();
	if (erReturn != E_OK) {
		return erReturn;
	}

	erReturn = ive_stateMachine(IVE_OP_SETPARAM, TRUE);
	if (erReturn != E_OK) {
		return erReturn;
	}
	erReturn = E_OK;

	return erReturn;
}

/**
    IVE start driver

    Start IVE engine to start running
    Load settings at engine start

    @return
    -@b E_OK:  Done with no error.
    -Others:   Error occured.
*/
ER ive_start(VOID)
{
	ER erReturn = E_OK;

	erReturn = ive_stateMachine(IVE_OP_START, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	ive_enable(TRUE);

	erReturn = ive_stateMachine(IVE_OP_START, TRUE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	return erReturn;
}

ER ive_ll_start(void)
{
	ER erReturn;

	erReturn = ive_stateMachine(IVE_OP_START, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}
	// Clear SW flag
	ive_clearFrameEndFlag();
	ive_clearFrameEndFlag_LL();

	erReturn = ive_stateMachine(IVE_OP_START, TRUE);
	ive_ll_enable(ENABLE);

	return erReturn;
}

/**
    IVE pause driver

    Pause IVE engine to leave RUN state

    @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
ER ive_pause(VOID)
{
	ER erReturn = E_OK;

	erReturn = ive_stateMachine(IVE_OP_PAUSE, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	//ive_enable(FALSE);//mark to test HW auto clear

	erReturn = ive_stateMachine(IVE_OP_PAUSE, TRUE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	return erReturn;
}

ER ive_ll_pause(void)
{
	ER erReturn;

	erReturn = ive_stateMachine(IVE_OP_PAUSE, FALSE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	ive_ll_enable(DISABLE);
	ive_ll_terminate(DISABLE);

	ive_stateMachine(IVE_OP_PAUSE, TRUE);

	return E_OK;
}

/**
    Close IVE driver

    Release IVE driver

    @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
ER ive_close(VOID)
{
	ER erReturn;

	// check state machine, no update
	erReturn = ive_stateMachine(IVE_OP_CLOSE, FALSE);
	if (erReturn != E_OK) {
		DBG_ERR("[ive drv] close state fail\n");
		return erReturn;
	}

	// check semaphore lock status
	if (!ive_isOpen()) {
		DBG_ERR("[ive drv] close lock status fail\n");
		return E_OBJ;
	}

	ive_platform_int_disable();

	//Sram shutdown test
	ive_platform_enable_sram_shutdown();

	//disable engine clock
	erReturn = ive_detach();
	if (erReturn != E_OK) {
		DBG_ERR("[ive drv] close detach fail\n");
		return erReturn;
	}

	// check state machine, update state
	erReturn = ive_stateMachine(IVE_OP_CLOSE, TRUE);
	if (erReturn != E_OK) {
		DBG_ERR("[ive drv] close statemachine fail\n");
		return erReturn;
	}

	// release task semaphore
	erReturn = ive_unlock();//sc
	if (erReturn != E_OK) {
		DBG_ERR("[ive drv] close unlock fail\n");
		return erReturn;
	}
	DBG_IND("[ive drv] close success\n");
	return erReturn;
}

VOID ive_setTriHdl(VOID *ive_trig_hdl)
{
	g_lib_ive_trig_hdl = (KDRV_IVE_HANDLE *) ive_trig_hdl;
	return;
}

VOID * ive_getTriHdl(VOID)
{
	return (VOID *) g_lib_ive_trig_hdl;
}

UINT32 ive_getTriHdl_flagId(VOID)
{
	return g_lib_ive_trig_hdl->flag_id;
}

/*
    IVE interrupt handler

    IVE interrupt service routine.
*/
VOID ive_isr(VOID)
{
	UINT32 uiIveIntStatus;
	ER erReturn = E_OK;
	KDRV_IVE_HANDLE *ive_trig_hdl;


	uiIveIntStatus = ive_getIntStatus();
	// Only handle enabled interrupt
	uiIveIntStatus &= ive_getIntrEnableReg();

	ive_trig_hdl = (KDRV_IVE_HANDLE *) ive_getTriHdl();

	//nvt_dbg(ERR, "IVE: ive_isr() status=0x%x\r\n", uiIveIntStatus);

	if (uiIveIntStatus != 0) {
		ive_clearIntStatus(uiIveIntStatus);

		if (uiIveIntStatus & IVE_INT_FRM_END) {
			//debug_msg("Interrupt Status : 0x%08x\r\n", *((volatile UINT32 *)0xf0d90008));
			g_uiIveFrmEndIntCnt++;
			erReturn = ive_platform_flg_set((UINT32) FLGPTN_IVE_FRAMEEND);
			if (erReturn != E_OK) {
				DBG_ERR("Error! %s_%d: ive_platform_flg_set() %d\n", __FUNCTION__, __LINE__, erReturn);
			}
		}

		if (uiIveIntStatus & IVE_INT_LLEND) {
			//debug_msg("Interrupt Status : 0x%08x\r\n", *((volatile UINT32 *)0xf0d90008));
			g_uiIveFrmEndIntCnt++;
			erReturn = ive_platform_flg_set((UINT32) FLGPTN_IVE_LLEND);
			if (erReturn != E_OK) {
				DBG_ERR("Error! %s_%d: ive_platform_flg_set() %d\n", __FUNCTION__, __LINE__, erReturn);
			}
		}

		if (uiIveIntStatus & IVE_INTE_LLERROR) {
			DBG_ERR("Error! IVE LL command has error.\r\n");
		}
	}

	if (g_pIveCBFunc != NULL) {
		if (uiIveIntStatus) {
			g_pIveCBFunc(uiIveIntStatus, (VOID *)ive_trig_hdl);
		}
	}
}

EXPORT_SYMBOL(ive_create_resource);
EXPORT_SYMBOL(ive_release_resource);
EXPORT_SYMBOL(ive_set_base_addr);
