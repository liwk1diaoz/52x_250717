#include "sde_platform.h"
#include    "sde_int.h"
#include    "sde_lib.h"
#include    "sde_reg.h"

#if defined (__KERNEL__)
#include    "sde_proc.h"
#endif


#include <kwrap/verinfo.h>
//=============================================================================
// version
//=============================================================================
VOS_MODULE_VERSION(SDE, 1, 00, 000, 00);


BOOL g_sde_fw_power_saving_en = 1;
BOOL g_sde_hw_power_saving_en = 1;

#if DRV_SUPPORT_IST
#else
static void (*g_pfSdeIntHdlCB)(UINT32 intstatus);
#endif


VOID sde_isr(VOID);

static UINT32   g_uiSDEngineStatus  = SDE_ENGINE_IDLE;
static BOOL     g_bSdePolling = FALSE;//TRUE;




/*
#ifdef __KERNEL__
#else
static UINT32   g_uiSdeTrueClock    = 0;//PLL_CLKSEL_SDE_PLL6;//alex
#endif
*/
//static UINT32   g_uiSdeClock;

static BOOL     g_sde_clk_en = FALSE;

/*

static ER sde_setInSize(UINT32 uiHsize, UINT32 uiVsize);
static ER sde_chgBurstLength(SDE_BURST_SEL *pBurstLen);
static ER sde_setScanSelBit(SDE_OUTVOL_SEL *pOutVolSel);
static ER sde_setInvValSelBit(SDE_INV_SEL *pInvSel);
static ER sde_setLumaPara(SDE_LUMA_PARA *pLumaPara);
static ER sde_setCostPara(SDE_COST_PARA *pCostPara);
static ER sde_setPenaltyPara(SDE_PENALTY_PARA *pPenaltyPara);
static ER sde_setSmoothPara(SDE_SMOOTH_PARA *pSmoothPara);
static ER sde_setDiagThresh(SDE_DIAG_THRESH *pThDiag);
*/
static ER sde_stop(VOID);
//static ER sde_setClockRate(UINT32 uiClock);
static ER sde_chgBurstLength(SDE_BURST_SEL *pBurstLen);
static ER sde_setInSize(UINT32 uiHsize, UINT32 uiVsize);
static ER sde_setScanSelBit(SDE_OUTVOL_SEL *pOutVolSel);
static ER sde_setInvValSelBit(SDE_INV_SEL *pInvSel);
static ER sde_setLumaPara(SDE_LUMA_PARA *pLumaPara);
static ER sde_setCostPara(SDE_COST_PARA *pCostPara);
static ER sde_setPenaltyPara(SDE_PENALTY_PARA *pPenaltyPara);
static ER sde_setSmoothPara(SDE_SMOOTH_PARA *pSmoothPara);
static ER sde_setDiagThresh(SDE_DIAG_THRESH *pThDiag);
static ER sde_setConfidenceTh(UINT8 confid_th);


static ER sde_unlock(VOID);
static ER sde_lock(VOID);
static ER sde_attach(VOID);
static ER sde_detach(VOID);
static ER sde_chkStateMachine(SDEOPERATION Op, SDESTATUSUPDATE Update);

// for FPGA use
ER sde_clr(VOID);
ER sde_setDmaInAddr0(UINT32 sai, UINT32 ofsi);
ER sde_setDmaInAddr1(UINT32 sai, UINT32 ofsi);
ER sde_setDmaInAddr2(UINT32 sai, UINT32 ofsi);
ER sde_setDmaOutAddr(UINT32 sai, UINT32 ofsi);
ER sde_setDmaOutAddr2(UINT32 sai, UINT32 ofsi);


void sde_setBaseAddr(UINT32 uiAddr)
{
//Alex  TODO:
#if defined (__KERNEL__)
	_SDE_REG_BASE_ADDR = uiAddr;
	
#elif defined (__FREERTOS)

#elif defined (__UITRON) || defined (__ECOS)

#else

#endif
}

/*
    SDE interrupt handler

    SDE interrupt service routine.
*/
VOID sde_isr(VOID)
{
	UINT32 uiSdeIntStatusRaw, uiSdeIntStatus, uiSdeIntEnable;
	UINT32 CBStatus;

	uiSdeIntStatusRaw = sde_getIntStatus();
	uiSdeIntEnable = sde_getIntEnable();
	uiSdeIntStatus = uiSdeIntStatusRaw & uiSdeIntEnable;

	if (uiSdeIntStatus == 0) {
		return;
	}
	sde_clearInt(uiSdeIntStatus);

	CBStatus = 0;

	if (uiSdeIntStatus & SDE_INT_FRMEND) {
		//iset_flg(FLG_ID_SDE, FLGPTN_SDE_FRAMEEND);
		sde_platform_flg_set(FLGPTN_SDE_FRAMEEND);
		if (sde_getIntEnable() & SDE_INTE_FRMEND) {
			CBStatus |= SDE_INT_FRMEND;
		}
		//DBG_WRN("SDE_INT_FRMEND!!\r\n");
	}

	if (uiSdeIntStatus & SDE_INT_LL_END) {
		sde_platform_flg_set(FLGPTN_SDE_LLEND);
		if (sde_getIntEnable() & SDE_INTE_LL_END) {
			CBStatus |= SDE_INT_LL_END;
		}
		//DBG_WRN("SDE_INT_LL_END!!\r\n");
	}

	if (uiSdeIntStatus & SDE_INT_LL_ERR) {
		sde_platform_flg_set(FLGPTN_SDE_LL_ERROR);
		if (sde_getIntEnable() & SDE_INTE_LL_ERROR) {
			CBStatus |= SDE_INT_LL_ERR;
		}
		//DBG_WRN("SDE_INT_LL_ERR!!\r\n");
	}

	if (uiSdeIntStatus & SDE_INT_LL_JOB_END) {
		sde_platform_flg_set(FLGPTN_SDE_JOB_END);
		if (sde_getIntEnable() & SDE_INTE_LL_JOB_END) {
			CBStatus |= SDE_INT_LL_JOB_END;
		}
		//DBG_WRN("SDE_INT_LL_JOB_END!!\r\n");
	}


#if DRV_SUPPORT_IST
	if (pfIstCB_SDE != NULL) {
		if (CBStatus) {
			drv_setIstEvent(DRV_IST_MODULE_IFE, CBStatus);
		}
	}
#else
	if (g_pfSdeIntHdlCB != NULL) {
		if (CBStatus) {
			g_pfSdeIntHdlCB(CBStatus);
		}
	}
#endif
}

/*
  Reset SDE
  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sde_clr(VOID)
{
	volatile T_ENGINE_CONTROL_REGISTER    LocalReg;

	LocalReg.reg = SDE_GETREG(ENGINE_CONTROL_REGISTER_OFS);
	LocalReg.bit.SDE_SW_RST = 1;
	SDE_SETREG(ENGINE_CONTROL_REGISTER_OFS, LocalReg.reg);
	while (1) {
		LocalReg.reg = SDE_GETREG(ENGINE_CONTROL_REGISTER_OFS);
		if (!LocalReg.bit.SDE_SW_RST) {
			break;
		}
	}
	return E_OK;
}

/*
  Set input 0 address and lineoffset
  @param[in] uiInAddr    input address
  @param[in] uiLineOfs   input lineoffset
  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/

ER sde_setDmaInAddr0(UINT32 uiInAddr, UINT32 uiLineOfs)
{
	T_DMA_TO_SDE_INPUT_IMAGE_LEFT               LocalReg1;
	T_DMA_TO_SDE_INPUT_IMAGE_LEFT_LINE_OFFSET   LocalReg2;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_INPUT_IMAGE_LEFT_OFS);
	LocalReg2.reg = SDE_GETREG(DMA_TO_SDE_INPUT_IMAGE_LEFT_LINE_OFFSET_OFS);

	if (((uiInAddr & 0x3) != 0) || ((uiLineOfs & 0x3) != 0)) {
		DBG_WRN("Input Address and lineoffset must be word align!!\r\n");
	}

	LocalReg1.bit.DRAM_IN_SADDR0 = uiInAddr >> 2;
	LocalReg2.bit.DRAM_IN_LOFST0 = uiLineOfs >> 2;
	SDE_SETREG(DMA_TO_SDE_INPUT_IMAGE_LEFT_OFS, LocalReg1.reg);
	SDE_SETREG(DMA_TO_SDE_INPUT_IMAGE_LEFT_LINE_OFFSET_OFS, LocalReg2.reg);

	return E_OK;
}

/*
  Set input 1 address and lineoffset

  @param[in] uiInAddr    input address
  @param[in] uiLineOfs   input lineoffset

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sde_setDmaInAddr1(UINT32 uiInAddr, UINT32 uiLineOfs)
{
	T_DMA_TO_SDE_INPUT_IMAGE_RIGHT               LocalReg1;
	T_DMA_TO_SDE_INPUT_IMAGE_RIGHT_LINE_OFFSET   LocalReg2;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_INPUT_IMAGE_RIGHT_OFS);
	LocalReg2.reg = SDE_GETREG(DMA_TO_SDE_INPUT_IMAGE_RIGHT_LINE_OFFSET_OFS);
	if (((uiInAddr & 0x3) != 0) || ((uiLineOfs & 0x3) != 0)) {
		DBG_WRN("Input Address and lineoffset must be word align!!\r\n");
	}
	LocalReg1.bit.DRAM_IN_SADDR1 = uiInAddr >> 2;
	LocalReg2.bit.DRAM_IN_LOFST1 = uiLineOfs >> 2;
	SDE_SETREG(DMA_TO_SDE_INPUT_IMAGE_RIGHT_OFS, LocalReg1.reg);
	SDE_SETREG(DMA_TO_SDE_INPUT_IMAGE_RIGHT_LINE_OFFSET_OFS, LocalReg2.reg);

	return E_OK;
}


/*
  Set input 2 address and lineoffset

  Set input 2 address and lineoffset

  @param[in] uiInAddr    input address
  @param[in] uiLineOfs   input lineoffset
  // note line offset would be a slice of cost

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sde_setDmaInAddr2(UINT32 uiInAddr, UINT32 uiLineOfs)
{
	T_DMA_TO_SDE_SCAN_SLICE_INVERTED_OUTPUT_RESULT               LocalReg1;
	T_DMA_TO_SDE_SCAN_SLICE_INVERTED_OUTPUT_RESULT_LINE_OFFSET   LocalReg2;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_SCAN_SLICE_INVERTED_OUTPUT_RESULT_OFS);
	LocalReg2.reg = SDE_GETREG(DMA_TO_SDE_SCAN_SLICE_INVERTED_OUTPUT_RESULT_LINE_OFFSET_OFS);
	if (((uiInAddr & 0x3) != 0) || ((uiLineOfs & 0x3) != 0)) {
		DBG_WRN("Input Address and lineoffset must be word align!!\r\n");
	}
	LocalReg1.bit.DRAM_IN_SADDR2 = uiInAddr >> 2;
	LocalReg2.bit.DRAM_IN_LOFST2 = uiLineOfs >> 2;
	SDE_SETREG(DMA_TO_SDE_SCAN_SLICE_INVERTED_OUTPUT_RESULT_OFS, LocalReg1.reg);
	SDE_SETREG(DMA_TO_SDE_SCAN_SLICE_INVERTED_OUTPUT_RESULT_LINE_OFFSET_OFS, LocalReg2.reg);

	return E_OK;
}


/*
  Set output address and lineoffset

  Set output address and lineoffset

  @param[in] uiOutAddr    output address
  @param[in] uiLineOfs    output lineoffset

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sde_setDmaOutAddr(UINT32 uiOutAddr, UINT32 uiLineOfs)
{
	T_DMA_TO_SDE_OUTPUT_DISPARITY_MAP LocalReg1;
	T_DMA_TO_SDE_OUTPUT_DISPARITY_MAP_LINE_OFFSET LocalReg2;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_OUTPUT_DISPARITY_MAP_OFS);
	LocalReg2.reg = SDE_GETREG(DMA_TO_SDE_OUTPUT_DISPARITY_MAP_LINE_OFFSET_OFS);
	if (uiOutAddr == 0) {
		//uiOutAddr = 0x800000;
		DBG_ERR("Output Address might be used before setting!!\r\n");
		return E_PAR;
	}
	if (((uiOutAddr & 0x3) != 0) || ((uiLineOfs & 0x3) != 0)) {
		DBG_WRN("Output Address and lineoffset must be word align!!\r\n");
	}
	LocalReg1.bit.DRAM_OUT_SADDR = uiOutAddr >> 2;
	LocalReg2.bit.DRAM_OUT_LOFST = uiLineOfs >> 2;
	SDE_SETREG(DMA_TO_SDE_OUTPUT_DISPARITY_MAP_OFS, LocalReg1.reg);
	SDE_SETREG(DMA_TO_SDE_OUTPUT_DISPARITY_MAP_LINE_OFFSET_OFS, LocalReg2.reg);

	return E_OK;
}

/*
  Set output address and lineoffset

  Set output address and lineoffset

  @param[in] uiOutAddr    output address
  @param[in] uiLineOfs    output lineoffset

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sde_setDmaOutAddr2(UINT32 uiOutAddr, UINT32 uiLineOfs)
{
    T_DMA_TO_SDE_OUTPUT_CONFIDENCE LocalReg1;
    T_DMA_TO_SDE_OUTPUT_CONFIDENCE_LINE_OFFSET LocalReg2;

    LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_OUTPUT_CONFIDENCE_OFS);
    LocalReg2.reg = SDE_GETREG(DMA_TO_SDE_OUTPUT_CONFIDENCE_LINE_OFFSET_OFS);
	
    if (uiOutAddr == 0) {
        //uiOutAddr = 0x800000;
        DBG_ERR("Output Address 2 might be used before setting!!\r\n");
        return E_PAR;
    }
	
    if (((uiOutAddr & 0x3) != 0) || ((uiLineOfs & 0x3) != 0)) {
        DBG_WRN("Output Address and lineoffset must be word align!!\r\n");
    }
    LocalReg1.bit.DRAM_OUT2_SADDR = uiOutAddr >> 2;
    LocalReg2.bit.DRAM_OUT2_LOFST = uiLineOfs >> 2;
    SDE_SETREG(DMA_TO_SDE_OUTPUT_CONFIDENCE_OFS, LocalReg1.reg);
    SDE_SETREG(DMA_TO_SDE_OUTPUT_CONFIDENCE_LINE_OFFSET_OFS, LocalReg2.reg);

    return E_OK;
}



UINT32 sde_getDmaInAddr0(VOID)
{
	T_DMA_TO_SDE_INPUT_IMAGE_LEFT       LocalReg1;
	UINT32 uiAddr, uiPhyAddr;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_INPUT_IMAGE_LEFT_OFS);
	uiPhyAddr = LocalReg1.bit.DRAM_IN_SADDR0 << 2;

//#if (SDE_DMA_CACHE_HANDLE == 1)
//	uiAddr = dma_getNonCacheAddr(uiPhyAddr);
//#else
	uiAddr = uiPhyAddr;
//#endif

	return uiAddr;
}

UINT32 sde_getDmaInAddr1(VOID)
{
	T_DMA_TO_SDE_INPUT_IMAGE_RIGHT       LocalReg1;
	UINT32 uiAddr, uiPhyAddr;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_INPUT_IMAGE_RIGHT_OFS);
	uiPhyAddr = LocalReg1.bit.DRAM_IN_SADDR1 << 2;

//#if (SDE_DMA_CACHE_HANDLE == 1)
//	uiAddr = dma_getNonCacheAddr(uiPhyAddr);
//#else
	uiAddr = uiPhyAddr;
//#endif

	return uiAddr;
}


UINT32 sde_getDmaInAddr2(VOID)
{
	T_DMA_TO_SDE_SCAN_SLICE_INVERTED_OUTPUT_RESULT       LocalReg1;
	UINT32 uiAddr, uiPhyAddr;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_SCAN_SLICE_INVERTED_OUTPUT_RESULT_OFS);
	uiPhyAddr = LocalReg1.bit.DRAM_IN_SADDR2 << 2;

//#if (SDE_DMA_CACHE_HANDLE == 1)
//	uiAddr = dma_getNonCacheAddr(uiPhyAddr);
//#else
	uiAddr = uiPhyAddr;
//#endif

	return uiAddr;
}


UINT32 sde_getDmaInLofs0(VOID)
{
	T_DMA_TO_SDE_INPUT_IMAGE_LEFT_LINE_OFFSET       LocalReg1;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_INPUT_IMAGE_LEFT_LINE_OFFSET_OFS);
	return (LocalReg1.bit.DRAM_IN_LOFST0 << 2);
}

UINT32 sde_getDmaInLofs1(VOID)
{
	T_DMA_TO_SDE_INPUT_IMAGE_RIGHT_LINE_OFFSET       LocalReg1;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_INPUT_IMAGE_RIGHT_LINE_OFFSET_OFS);
	return (LocalReg1.bit.DRAM_IN_LOFST1 << 2);
}

UINT32 sde_getDmaInLofs2(VOID)
{
	T_DMA_TO_SDE_SCAN_SLICE_INVERTED_OUTPUT_RESULT_LINE_OFFSET       LocalReg1;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_SCAN_SLICE_INVERTED_OUTPUT_RESULT_LINE_OFFSET_OFS);
	return (LocalReg1.bit.DRAM_IN_LOFST2 << 2);
}

UINT32  sde_getDmaOutAddr(VOID)
{
	T_DMA_TO_SDE_OUTPUT_DISPARITY_MAP       LocalReg1;
	UINT32 uiAddr, uiPhyAddr;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_OUTPUT_DISPARITY_MAP_OFS);
	uiPhyAddr = LocalReg1.bit.DRAM_OUT_SADDR << 2;

//#if (SDE_DMA_CACHE_HANDLE == 1)
//	uiAddr = dma_getNonCacheAddr(uiPhyAddr);//va
//#else
	uiAddr = uiPhyAddr; //pa
//#endif
	return uiAddr;
}

UINT32  sde_getDmaOutAddr2(VOID)
{
	T_DMA_TO_SDE_OUTPUT_CONFIDENCE       LocalReg1;
	UINT32 uiAddr, uiPhyAddr;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_OUTPUT_CONFIDENCE_OFS);
	uiPhyAddr = LocalReg1.bit.DRAM_OUT2_SADDR << 2;

//#if (SDE_DMA_CACHE_HANDLE == 1)
//	uiAddr = dma_getNonCacheAddr(uiPhyAddr);//va
//#else
	uiAddr = uiPhyAddr; //pa
//#endif
	return uiAddr;
}



UINT32  sde_getDmaOutLofs(VOID)
{
	T_DMA_TO_SDE_OUTPUT_DISPARITY_MAP_LINE_OFFSET       LocalReg1;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_OUTPUT_DISPARITY_MAP_LINE_OFFSET_OFS);
	return (LocalReg1.bit.DRAM_OUT_LOFST << 2);
}

UINT32  sde_getDmaOutLofs2(VOID)
{
	T_DMA_TO_SDE_OUTPUT_CONFIDENCE_LINE_OFFSET       LocalReg1;

	LocalReg1.reg = SDE_GETREG(DMA_TO_SDE_OUTPUT_CONFIDENCE_LINE_OFFSET_OFS);
	return (LocalReg1.bit.DRAM_OUT2_LOFST << 2);
}

UINT32  sde_getInvalidCount(VOID)
{
	T_SDE_INFORMATION_REGISTER       LocalReg;

	LocalReg.reg = SDE_GETREG(SDE_INFORMATION_REGISTER_OFS);
	return LocalReg.bit.INVALID_COUNT;
}

UINT32 sde_getInVsize(VOID)
{
	T_INPUT_IMAGE_SIZE_REGISTER    LocalReg;

	LocalReg.reg = SDE_GETREG(INPUT_IMAGE_SIZE_REGISTER_OFS);
	return (LocalReg.bit.IMG_HEIGHT);
}

UINT32 sde_getInHsize(VOID)
{
	T_INPUT_IMAGE_SIZE_REGISTER    LocalReg;

	LocalReg.reg = SDE_GETREG(INPUT_IMAGE_SIZE_REGISTER_OFS);
	return (LocalReg.bit.IMG_WIDTH);
}


/*
  Set Control Signals

  Set 1-bit control signals of SDE

  @param[in] bEnable choose to enable or disable specified functions.
        \n-@b ENABLE: to enable.
        \n-@b DISABLE: to disable.
    @param[in] uiFunc indicate the function(s)

    @return None.
*/

ER  sde_enableCtrlBit(BOOL bEnable, UINT32 uiFunc)
{
	T_ENGINE_CONTROL_REGISTER   LocalReg;
	LocalReg.reg = SDE_GETREG(ENGINE_CONTROL_REGISTER_OFS);

	if (bEnable) {
		LocalReg.reg |= uiFunc;
	} else {
		LocalReg.reg &= (~uiFunc);
	}

	SDE_SETREG(ENGINE_CONTROL_REGISTER_OFS, LocalReg.reg);

	return E_OK;
}

/**
    SDE Enable Functions

    Enable/Disable selected SDE functions

    @param[in] bEnable choose to enable or disable specified functions.
        \n-@b ENABLE: to enable.
        \n-@b DISABLE: to disable.
    @param[in] uiFunc indicate the function(s)

    @return None.
*/
ER sde_enableFunction(BOOL bEnable, UINT32 uiFunc)
{
	T_SDE_FUNCTION_CONTROL_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS);

	uiFunc &= (SDE_FUNC_ALL);

	if (bEnable) {
		LocalReg.reg |= uiFunc;
	} else {
		LocalReg.reg &= (~uiFunc);
	}

	SDE_SETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS, LocalReg.reg);
	return E_OK;
}

/**
    SDE Check Function Enable/Disable Status

    Check if the one specified SDE funtion is enabled or not

    @param[in] uiFunc indicate the function to check

    @return
        \n-@b ENABLE: the function is enabled.
        \n-@b DISABLE: the function is disabled.
*/
BOOL sde_checkFunctionEnable(UINT32 uiFunc)
{
	T_SDE_FUNCTION_CONTROL_REGISTER LocalReg;
	UINT32 uiFuncSt;

	LocalReg.reg = SDE_GETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS);

	uiFuncSt = LocalReg.reg;

	return (BOOL)((uiFuncSt & uiFunc) != 0);
}


/**
  Wait SDE Processing done

  Wait SDE Processing done

  @return VOID
*/
VOID sde_waitFlagFrameEnd(VOID)
{
	FLGPTN uiFlag;
	T_SDE_INTERRUPT_STATUS_REGISTER  LocalReg;

	if (g_bSdePolling) {
		LocalReg.reg = 0;
		while (LocalReg.bit.INTS_FRM_END != 1) {
			LocalReg.reg = SDE_GETREG(SDE_INTERRUPT_STATUS_REGISTER_OFS);
		}
		sde_clearInt(SDE_INT_FRMEND);

	} else {
		sde_platform_flg_wait(&uiFlag, FLGPTN_SDE_FRAMEEND);
		//wai_flg(&uiFlag, FLG_ID_SDE, FLGPTN_SDE_FRAMEEND, TWF_ORW | TWF_CLR);
		//g_uiIFEngineStatus = IFE_ENGINE_READY;
	}
}

VOID sde_waitFlagLinkListEnd(VOID)
{
	FLGPTN uiFlag;
	T_SDE_INTERRUPT_STATUS_REGISTER  LocalReg;

	if (g_bSdePolling) {
		LocalReg.reg = 0;
		while (LocalReg.bit.INTS_LL_END != 1) {
			LocalReg.reg = SDE_GETREG(SDE_INTERRUPT_STATUS_REGISTER_OFS);
		}
		sde_clearInt(SDE_INT_LL_END);

	} else {
		sde_platform_flg_wait(&uiFlag, FLGPTN_SDE_LLEND);
		//wai_flg(&uiFlag, FLG_ID_SDE, FLGPTN_SDE_FRAMEEND, TWF_ORW | TWF_CLR);
		//g_uiIFEngineStatus = IFE_ENGINE_READY;
	}

}




/**
  clear SDE frameend flag

  @return VOID
*/
VOID sde_clrFrameEnd(VOID)
{
	sde_platform_flg_clear(FLGPTN_SDE_FRAMEEND);
	//clr_flg(FLG_ID_SDE, FLGPTN_SDE_FRAMEEND);
}

VOID sde_clr_LL_End(VOID)
{
	sde_platform_flg_clear(FLGPTN_SDE_LLEND);
}

// driver

/**
  Open SDE driver

  This function should be called before calling any other functions

  @param[in] pObjCB ISR callback function

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
ER sde_open(SDE_OPENOBJ *pObjCB)
{
	ER erReturn;
	//_BURST_LENGTH BurstLen;
	SDE_BURST_SEL BurstLen;

	erReturn = sde_lock();
	if (erReturn != E_OK) {
		DBG_ERR("erReturn fail\r\n");
		return erReturn;
	}

	
	erReturn = sde_chkStateMachine(SDE_OP_OPEN, NOTUPDATE);
	if (erReturn != E_OK) {
		// release resource when stateMachine fail
		sde_unlock();
		return erReturn;
	}

	//HW auto gating. 
#if defined (__KERNEL__)
	if (g_sde_hw_power_saving_en == 1) {
		clk_set_phase((pdrv_info_data->module_info.pclk[0]), 1);
		clk_set_phase((pdrv_info_data->module_info.p_pclk[0]), 1);
		//DBG_WRN("SDE HW clock gating enable!\r\n");
	} else {
		clk_set_phase((pdrv_info_data->module_info.pclk[0]), 0);
		clk_set_phase((pdrv_info_data->module_info.p_pclk[0]), 0);
		//DBG_WRN("SDE HW clock gating disable!\r\n");
	}
#endif

	// Turn on power
	sde_platform_set_clk_rate(pObjCB->uiSdeClockSel);

	erReturn = sde_attach();
	if (erReturn != E_OK) {
		return erReturn;
	}
	
	//Enable SRAM, sde
	sde_platform_disable_sram_shutdown();

	// Pre-set the interrupt flag
	sde_platform_flg_clear(FLGPTN_SDE_FRAMEEND);
	/*
	if (erReturn != E_OK) {
		return erReturn;
	}
	*/

	//enable interrupt
	sde_platform_int_enable();

	sde_enableInt(TRUE);
	
	sde_clearInt(/*SDE_INT_FRMEND*/SDE_INT_ALL);
	
	sde_clr();

	g_pfSdeIntHdlCB = pObjCB->FP_SDEISR_CB;

	BurstLen = SDE_BURST_64W;  
	sde_chgBurstLength(&BurstLen);

	
	erReturn = sde_chkStateMachine(SDE_OP_OPEN, UPDATE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	return E_OK;
}

/**
    check SDE is opened.

    check if SDE module is opened.

    @return
        -@b TRUE: SDE is open.
        -@b FALSE: SDE is closed.
*/
BOOL sde_isOpened(VOID)
{
	return g_SdeLockStatus;
}


/**
  Close SDE driver

  Release SDE driver

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
ER sde_close(VOID)
{
	ER erReturn;

	erReturn = sde_chkStateMachine(SDE_OP_CLOSE, NOTUPDATE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	// Check lock status
	if (g_SdeLockStatus != TASK_LOCKED) {
		return E_SYS;
	}

	sde_platform_int_disable();

	//Disable SRAM
	sde_platform_enable_sram_shutdown();
	
	erReturn = sde_detach();
	if (erReturn != E_OK) {
		return erReturn;
	}

	erReturn = sde_chkStateMachine(SDE_OP_CLOSE, UPDATE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	sde_unlock();

	return E_OK;
}


ER sde_setMode(SDE_PARAM *pSdeInfo)
{
	ER erReturn;
	SDE_BURST_SEL BurstLen;

#if (SDE_DMA_CACHE_HANDLE == 1)
	UINT32 uiAddr, uiSize, uiPhyAddr;
#endif

	if (pSdeInfo->opMode == SDE_OPMODE_UNKNOWN) {
		DBG_ERR("Invalid operation!! sde_setMode()\r\n");
		return E_PAR;
	}

	erReturn = sde_chkStateMachine(SDE_OP_SETMODE, NOTUPDATE);
	if (erReturn != E_OK) {
		return erReturn;
	}
	
#if _EMULATION_
	DBG_ERR("SDE  _EMULATION_ = 1;   Bypass all set to driver related API \r\n");
	goto SDE_BY_PASS;
#endif 
	//---------------------------------------
	//Enable all SDE Interrupt
	//---------------------------------------
	sde_enableInt(pSdeInfo->uiIntrEn);

	//---------------------------------------
	//Set scan mode. Search range selection
	//---------------------------------------
	if (pSdeInfo->opMode == SDE_MAX_DISP_63) {
		sde_enableFunction(ENABLE, SDE_S_SCAN_MODE);
	} else {
		sde_enableFunction(DISABLE, SDE_S_SCAN_MODE);
	}

	//---------------------------------------
	//Set confidence related parameter.
	//---------------------------------------
	sde_set_disp_val_mode(pSdeInfo->disp_val_mode);
	sde_set_conditional_disp_en(pSdeInfo->cond_disp_en);
	sde_set_conf_out2_en(pSdeInfo->conf_out2_en);
	sde_set_conf_out2_mode_sel(pSdeInfo->conf_out2_mode);
	sde_set_conf_min2_sel(pSdeInfo->conf_min2_sel);

	//---------------------------------------
	//Set input Path 2 enable switch. round 1 or round 2. 
	//---------------------------------------
	if (pSdeInfo->bInput2En) {
		sde_enableFunction(ENABLE, SDE_S_PATH2_INPUT_EN);
	} else {
		sde_enableFunction(DISABLE, SDE_S_PATH2_INPUT_EN);
	}

	//---------------------------------------
	//Set output data format selection. Disparity or cost.
	//---------------------------------------
	if (pSdeInfo->outSel) {
		sde_enableFunction(ENABLE, SDE_S_OUT_SEL);
	} else {
		sde_enableFunction(DISABLE, SDE_S_OUT_SEL);
	}

	
	if (pSdeInfo->bCostCompMode) {
		sde_enableFunction(ENABLE, SDE_S_COST_COMP_MODE);
	} else {
		sde_enableFunction(DISABLE, SDE_S_COST_COMP_MODE);
	}


	//---------------------------------------
	//Scan function enable switch.
	//---------------------------------------
	if (pSdeInfo->bScanEn) {
		sde_enableFunction(ENABLE, SDE_S_SCAN_FUN_EN);
	} else {
		sde_enableFunction(DISABLE, SDE_S_SCAN_FUN_EN);
	}

	//---------------------------------------
	//Diadonal search enable switch.
	//---------------------------------------
	if (pSdeInfo->bDiagEn) {
		sde_enableFunction(ENABLE, SDE_S_DIAG_FUN_EN);
	} else {
		sde_enableFunction(DISABLE, SDE_S_DIAG_FUN_EN);
	}
	
	//---------------------------------------
	// Input 01, mirror/flip control here
	//---------------------------------------
	if (pSdeInfo->bHflip01) {
		sde_enableFunction(ENABLE, SDE_S_H_FLIP01);
	} else {
		sde_enableFunction(DISABLE, SDE_S_H_FLIP01);
	}
	if (pSdeInfo->bVflip01) {
		sde_enableFunction(ENABLE, SDE_S_V_FLIP01);
	} else {
		sde_enableFunction(DISABLE, SDE_S_V_FLIP01);
	}

	//---------------------------------------
	// Input 2, mirror/flip control here
	//---------------------------------------
	if (pSdeInfo->bHflip2) {
		sde_enableFunction(ENABLE, SDE_S_H_FLIP2);
	} else {
		sde_enableFunction(DISABLE, SDE_S_H_FLIP2);
	}
	if (pSdeInfo->bVflip2) {
		sde_enableFunction(ENABLE, SDE_S_V_FLIP2);
	} else {
		sde_enableFunction(DISABLE, SDE_S_V_FLIP2);
	}

	

	//---------------------------------------
	//Set Burst length
	//---------------------------------------
	BurstLen = pSdeInfo->burstSel;//SDE_BURST_64W;
	sde_chgBurstLength(&BurstLen);
	
	//-----------------------------------------------------
	// scan function selection and invalid value selection
	//-----------------------------------------------------
	
	sde_setScanSelBit(&pSdeInfo->outVolSel);
	sde_setInvValSelBit(&pSdeInfo->invSel);
	
	//---------------------------------------
	// input size H V
	//---------------------------------------
	sde_setInSize(pSdeInfo->ioPara.Size.uiWidth, pSdeInfo->ioPara.Size.uiHeight);
	
	//---------------------------------------
	// set parameters
	//---------------------------------------
	sde_setCostPara(&pSdeInfo->costPara);
	sde_setLumaPara(&pSdeInfo->lumaPara);
	sde_setPenaltyPara(&pSdeInfo->penaltyValues);
	sde_setSmoothPara(&pSdeInfo->thSmooth);
	sde_setDiagThresh(&pSdeInfo->thDiag);
	//---------------------------------------
	//Confidence threshold, new in 528 
	//---------------------------------------
	sde_setConfidenceTh(pSdeInfo->confidence_th);

	//---------------------------------------
	//Link list start addr
	//---------------------------------------
	uiAddr = pSdeInfo->ioPara.uiInAddr_link_list;
	if (uiAddr !=0){
		uiPhyAddr = sde_platform_va2pa(uiAddr);;
		sde_set_LL_start_addr(uiPhyAddr);
	}
	

#if (SDE_DMA_CACHE_HANDLE == 1)
	//----------------------------------------------
	// input 0
	//----------------------------------------------
	uiAddr = pSdeInfo->ioPara.uiInAddr0;
	uiSize = pSdeInfo->ioPara.Size.uiOfsi0 * pSdeInfo->ioPara.Size.uiHeight;
	if (uiAddr != 0) {
		sde_platform_dma_flush_mem2dev(uiAddr, uiSize);
		uiPhyAddr = sde_platform_va2pa(uiAddr);
		sde_setDmaInAddr0(uiPhyAddr, pSdeInfo->ioPara.Size.uiOfsi0);
	}else{
		DBG_WRN("SDE Input 0 address is 0 !!\r\n");	
	}
	
	
	//----------------------------------------------
	// input 1
	//----------------------------------------------
	uiAddr = pSdeInfo->ioPara.uiInAddr1;
	uiSize = pSdeInfo->ioPara.Size.uiOfsi1 * pSdeInfo->ioPara.Size.uiHeight;
	if (uiAddr != 0) {
		sde_platform_dma_flush_mem2dev(uiAddr, uiSize);
		uiPhyAddr = sde_platform_va2pa(uiAddr);
		sde_setDmaInAddr1(uiPhyAddr, pSdeInfo->ioPara.Size.uiOfsi1);
	}else{
		DBG_WRN("SDE Input 1 address is 0 !!\r\n");	
	}
	

	//----------------------------------------------
	// input 2
	//----------------------------------------------
	if (pSdeInfo->bInput2En) { // if we have input two, get the memory space then
		uiAddr = pSdeInfo->ioPara.uiInAddr2;
		//uiSize = pSdeInfo->ioPara.Size.uiOfsi2 * pSdeInfo->ioPara.Size.uiHeight;

		if (uiAddr != 0) {
			
			//sde_platform_dma_flush_mem2dev(uiAddr, uiSize);
			uiPhyAddr = sde_platform_va2pa(uiAddr);
			sde_setDmaInAddr2(uiPhyAddr, pSdeInfo->ioPara.Size.uiOfsi2);
		}else{
			DBG_WRN("SDE Input 2 address is 0 while input2 enabled !!\r\n");	
		}
		
	}

	//----------------------------------------------
	// output, disparity slice or disparity map
	//----------------------------------------------
	uiAddr = pSdeInfo->ioPara.uiOutAddr;
	//uiSize = pSdeInfo->ioPara.Size.uiOfso * pSdeInfo->ioPara.Size.uiHeight;
	if (uiAddr != 0) {
		//vos_cpu_dcache_sync(uiAddr, uiSize, VOS_DMA_BIDIRECTIONAL );

		sde_platform_dma_flush_dev2mem(uiAddr, 32);//for ca9 cpu.

		/*
		if (pSdeInfo->ioPara.Size.uiOfso == pSdeInfo->ioPara.Size.uiWidth) {			
			//sde_platform_dma_flush_mem2dev(uiAddr, uiSize);
			sde_platform_dma_flush_dev2mem(uiAddr, uiSize);//for ca9 cpu.
		} else {
			//sde_platform_dma_flush_mem2dev(uiAddr, uiSize);
			
		}
		*/
		uiPhyAddr = sde_platform_va2pa(uiAddr);
		erReturn = sde_setDmaOutAddr(uiPhyAddr, pSdeInfo->ioPara.Size.uiOfso);
	}else{
		DBG_WRN("SDE output 1 address is 0!!\r\n");	
	}
	
	if (erReturn != E_OK) {
		return erReturn;
	}

	//----------------------------------------------
	//output 2 addr / offset, confidence  level
	//----------------------------------------------
	if (pSdeInfo->conf_out2_en){
		uiAddr = pSdeInfo->ioPara.uiOutAddr2;
		//uiSize = pSdeInfo->ioPara.Size.uiOfso2 * pSdeInfo->ioPara.Size.uiHeight;
		if (uiAddr != 0) {
			//sde_platform_dma_flush_mem2dev(uiAddr, uiSize);

			//vos_cpu_dcache_sync(uiAddr, uiSize, VOS_DMA_BIDIRECTIONAL );

			sde_platform_dma_flush_dev2mem(uiAddr, 32);//for ca9 cpu.
			
			uiPhyAddr = sde_platform_va2pa(uiAddr);
			erReturn = sde_setDmaOutAddr2(uiPhyAddr, pSdeInfo->ioPara.Size.uiOfso2);
		}else{
			DBG_WRN("SDE output 2 address is 0!!\r\n");	
		}
		
		if (erReturn != E_OK) {
		    return erReturn;
		}
	}
	
	

#else // NON CATCH HANDLE MODE
	//input 0
	sde_setDmaInAddr0(pSdeInfo->ioPara.uiInAddr0, pSdeInfo->ioPara.Size.uiOfsi0);
	//input 1
	sde_setDmaInAddr1(pSdeInfo->ioPara.uiInAddr1, pSdeInfo->ioPara.Size.uiOfsi1);
	//input 2
	if (pSdeInfo->bInput2En) {
		sde_setDmaInAddr2(pSdeInfo->ioPara.uiInAddr2, pSdeInfo->ioPara.Size.uiOfsi2);
	}
	
	// output 1
	erReturn = sde_setDmaOutAddr(pSdeInfo->ioPara.uiOutAddr, pSdeInfo->ioPara.Size.uiOfso);
	if (erReturn != E_OK) {
		return erReturn;
	}
	// output 2
	if (pSdeInfo->conf_out2_en){
		erReturn = sde_setDmaOutAddr2(pSdeInfo->ioPara.uiOutAddr2, pSdeInfo->ioPara.Size.uiOfso2);
		if (erReturn != E_OK) {
		    return erReturn;
		}
	}
	
#endif

#if _EMULATION_
SDE_BY_PASS:
#endif

	erReturn = sde_chkStateMachine(SDE_OP_SETMODE, UPDATE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	return erReturn;
}


/*
  Start SDE

  Set SDE start bit to 1.

  @param None.

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/
ER sde_start(VOID)
{
	ER erReturn;
	T_ENGINE_CONTROL_REGISTER    LocalReg;

	erReturn = sde_chkStateMachine(SDE_OP_START, NOTUPDATE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	//if (g_sde_fw_power_saving_en){
		//Enable SRAM, sde
	sde_platform_disable_sram_shutdown();
	erReturn = sde_attach();
	if (erReturn != E_OK) {
		return erReturn;
	}
	//}
	
	sde_clrFrameEnd();
	sde_clr_LL_End();
	
	sde_chkStateMachine(SDE_OP_START, UPDATE);
	LocalReg.reg = SDE_GETREG(ENGINE_CONTROL_REGISTER_OFS);
	LocalReg.bit.SDE_START = 1;
	SDE_SETREG(ENGINE_CONTROL_REGISTER_OFS, LocalReg.reg);

	return E_OK;
}

/**
    SDE pause

    Pause SDE operation

    @param None.

    @return ER SDE pause status.
*/
ER sde_pause(VOID)
{
	ER      erReturn;

	erReturn = sde_chkStateMachine(SDE_OP_PAUSE, NOTUPDATE);
	if (erReturn != E_OK) {
		return erReturn;
	}

	sde_stop();

	if (g_sde_fw_power_saving_en){
		//Enable SRAM, sde
		sde_platform_enable_sram_shutdown();
		erReturn = sde_detach();
		if (erReturn != E_OK) {
			return erReturn;
		}
	}

	erReturn = sde_chkStateMachine(SDE_OP_PAUSE, UPDATE);
	if (erReturn != E_OK) {
		return erReturn;
	}
	return E_OK;
}

ER sde_getBurstLength(SDE_BURST_SEL *pBurstLen)
{
	T_ENGINE_CONTROL_REGISTER    LocalReg1;
	LocalReg1.reg = SDE_GETREG(ENGINE_CONTROL_REGISTER_OFS);
	*pBurstLen = (SDE_BURST_SEL)LocalReg1.bit.SDE_BURST_LENGTH_SEL;
	return E_OK;
}

/*
  Check SDE State Machine

  This function check SDE state machine

  @param[in] Op:        operation mode
  @param[in] Update:    SDE Operation update state machine

  @return
        - @b E_OK:      Done with no error.
        - Others:       Error occured.
*/

static ER sde_chkStateMachine(SDEOPERATION Op, SDESTATUSUPDATE Update)
{
	switch (Op) {
	case SDE_OP_OPEN:
		switch (g_uiSDEngineStatus) {
		case SDE_ENGINE_IDLE:
			if (Update) {
				g_uiSDEngineStatus = SDE_ENGINE_READY;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("SDE Operation ERROR!!Operate %d from State %d\r\n", Op, g_uiSDEngineStatus);
			return E_SYS;
			break;
		}
		break;
	case SDE_OP_CLOSE:
		switch (g_uiSDEngineStatus) {
		case SDE_ENGINE_PAUSE:
		case SDE_ENGINE_READY:
			if (Update) {
				g_uiSDEngineStatus = SDE_ENGINE_IDLE;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("SDE Operation ERROR!!Operate %d from State %d\r\n", Op, g_uiSDEngineStatus);
			return E_SYS;
			break;
		}
		break;
	case SDE_OP_SETMODE:
		switch (g_uiSDEngineStatus) {
		case SDE_ENGINE_READY:
		case SDE_ENGINE_PAUSE:
			if (Update) {
				g_uiSDEngineStatus = SDE_ENGINE_PAUSE;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("SDE Operation ERROR!!Operate %d from State %d\r\n", Op, g_uiSDEngineStatus);
			return E_SYS;
			break;
		}
		break;
	case SDE_OP_START:
		switch (g_uiSDEngineStatus) {
		case SDE_ENGINE_PAUSE:
			if (Update) {
				g_uiSDEngineStatus = SDE_ENGINE_RUN;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("SDE Operation ERROR!!Operate %d from State %d\r\n", Op, g_uiSDEngineStatus);
			return E_SYS;
			break;
		}
		break;
	case SDE_OP_CHGPARAM:
		switch (g_uiSDEngineStatus) {
		case SDE_ENGINE_READY:
		case SDE_ENGINE_PAUSE:
		case SDE_ENGINE_RUN:
			return E_OK;
			break;
		default:
			DBG_ERR("SDE Operation ERROR!!Operate %d from State %d\r\n", Op, g_uiSDEngineStatus);
			return E_SYS;
			break;
		}
		break;
	case SDE_OP_PAUSE:
		switch (g_uiSDEngineStatus) {
		case SDE_ENGINE_RUN:
		case SDE_ENGINE_PAUSE:
			if (Update) {
				g_uiSDEngineStatus = SDE_ENGINE_PAUSE;
			}
			return E_OK;
			break;
		default:
			DBG_ERR("SDE Operation ERROR!!Operate %d from State %d\r\n", Op, g_uiSDEngineStatus);
			return E_SYS;
			break;
		}
		break;
	default :
		DBG_ERR("SDE Operation ERROR!!\r\n");
		return E_SYS;
		break;
	}
	return E_OK;
}

/*
    SDE attach.

    attach SDE.

    @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
static ER sde_attach(VOID)
{
	// Enable SDE Clock
	if (g_sde_clk_en == FALSE) {
		sde_platform_enable_clk();
		g_sde_clk_en = TRUE;
	}
	return E_OK;
}

/*
    SDE detach.

    detach SDE.

    @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
static ER sde_detach(VOID)
{
	// Disable SDE Clock
	if (g_sde_clk_en == TRUE) {
		sde_platform_disable_clk();
		g_sde_clk_en = FALSE;
	}
	return E_OK;
}


/**
    SDE stop

    set sde_start bit to 0;

    @param None.

    @return ER SDE pause status.
*/
static ER sde_stop(VOID)
{
	T_ENGINE_CONTROL_REGISTER    LocalReg;

	LocalReg.reg = SDE_GETREG(ENGINE_CONTROL_REGISTER_OFS);
	LocalReg.bit.SDE_START = 0;
	SDE_SETREG(ENGINE_CONTROL_REGISTER_OFS, LocalReg.reg);
	return E_OK;
}




/*
  Lock SDE

  This function lock SDE module

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
static ER sde_lock(VOID)
{
	return sde_platform_sem_wait();
}


/*
  Unlock SDE

  This function unlock SDE module

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
static ER sde_unlock(VOID)
{
	return sde_platform_sem_signal();
}



/**
  SDE interrupt enable selection.

  SDE interrupt enable selection.

  @param[in] uiIntr 1's in this word will activate corresponding interrupt.

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
ER sde_enableInt(BOOL Intr_en) // no get need
{
	T_SDE_INTERRUPT_ENABLE_REGISTER LocalReg;

	LocalReg.reg = 0;//uiIntr;

	if (g_bSdePolling) {
		LocalReg.reg &= (~SDE_INTE_FRMEND);
		LocalReg.reg &= (~SDE_INTE_LL_END);
		LocalReg.reg &= (~SDE_INTE_LL_ERROR);
		LocalReg.reg &= (~SDE_INTE_LL_JOB_END);
	} else {
		LocalReg.reg |= SDE_INTE_FRMEND;
		LocalReg.reg |= SDE_INTE_LL_END;
		LocalReg.reg |= SDE_INTE_LL_ERROR;
		LocalReg.reg |= SDE_INTE_LL_JOB_END;	
	}
	
	if (Intr_en == TRUE){
		SDE_SETREG(SDE_INTERRUPT_ENABLE_REGISTER_OFS, LocalReg.reg);
	}else{
		//disable all interrupt enable.
		LocalReg.reg = 0;
		SDE_SETREG(SDE_INTERRUPT_ENABLE_REGISTER_OFS, LocalReg.reg);
	}

	return E_OK;
}


/**
  SDE interrupt enable status.

  Check SDE interrupt enable status.

  @return SDE interrupt enable status.
*/
UINT32 sde_getIntEnable(VOID)
{
	T_SDE_INTERRUPT_ENABLE_REGISTER  LocalReg;

	LocalReg.reg = SDE_GETREG(SDE_INTERRUPT_ENABLE_REGISTER_OFS);
	return LocalReg.reg;
}

/**
  SDE interrupt status.

  Check SDE interrupt status.

  @return SDE interrupt status readout.
*/
UINT32 sde_getIntStatus(VOID)
{
	T_SDE_INTERRUPT_STATUS_REGISTER  LocalReg;

	LocalReg.reg = SDE_GETREG(SDE_INTERRUPT_STATUS_REGISTER_OFS);
	return LocalReg.reg;
}


/**
  SDE clear interrupt status.

  SDE clear interrupt status.

  @param[in] uiIntr 1's in this word will clear corresponding interrupt

  @return
        - @b E_OK:  Done with no error.
        - Others:   Error occured.
*/
ER sde_clearInt(UINT32 uiIntr)
{
	T_SDE_INTERRUPT_STATUS_REGISTER  LocalReg;

	LocalReg.reg = uiIntr;
	SDE_SETREG(SDE_INTERRUPT_STATUS_REGISTER_OFS, LocalReg.reg);

	return E_OK;
}

/*
  Set SDE burst length

  @param[in] BurstLen    input & output burst length (the same)

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/

static ER sde_chgBurstLength(SDE_BURST_SEL *pBurstLen)
{
	T_ENGINE_CONTROL_REGISTER   LocalReg;
	LocalReg.reg = SDE_GETREG(ENGINE_CONTROL_REGISTER_OFS);
	switch (*pBurstLen) {
	case SDE_BURST_64W:
		LocalReg.bit.SDE_BURST_LENGTH_SEL = 0;
		break;
	case SDE_BURST_48W:
		LocalReg.bit.SDE_BURST_LENGTH_SEL = 1;
		break;
	case SDE_BURST_32W:
		LocalReg.bit.SDE_BURST_LENGTH_SEL = 2;
		break;
	default:
		LocalReg.bit.SDE_BURST_LENGTH_SEL = 0;
		break;
	}

	SDE_SETREG(ENGINE_CONTROL_REGISTER_OFS, LocalReg.reg);

	return E_OK;
}


/*
  Set input size

  Set input size

  @param[in] hsize  input horizontal size, should be 4x
  @param[in] vsize  input vertical size, should be 2x

  @return
        - @b E_OK: Done with no error.
        - Others: Error occured.
*/

static ER sde_setInSize(UINT32 uiHsize, UINT32 uiVsize)
{
	T_INPUT_IMAGE_SIZE_REGISTER    LocalReg;

	LocalReg.reg = SDE_GETREG(INPUT_IMAGE_SIZE_REGISTER_OFS);
	if ((uiHsize & 0x3) != 0) {
		DBG_ERR("SDE Input width should be 4x!!\r\n");
		return E_PAR;
	}

	LocalReg.bit.IMG_HEIGHT = uiVsize;
	LocalReg.bit.IMG_WIDTH = uiHsize;
	SDE_SETREG(INPUT_IMAGE_SIZE_REGISTER_OFS, LocalReg.reg);

	return E_OK;
}


static ER sde_setScanSelBit(SDE_OUTVOL_SEL *pOutVolSel)
{
	T_SDE_FUNCTION_CONTROL_REGISTER    LocalReg;

	LocalReg.reg = SDE_GETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS);
	// scan function selection
	switch (*pOutVolSel) {
	case SDE_OUT_SCAN_VOL:
		LocalReg.bit.SDE_SCAN_FUN_SEL = 0;
		break;
	case SDE_OUT_SCAN_DIR_1:
		LocalReg.bit.SDE_SCAN_FUN_SEL = 1;
		break;
	case SDE_OUT_SCAN_DIR_3:
		LocalReg.bit.SDE_SCAN_FUN_SEL = 2;
		break;
	case SDE_OUT_SCAN_DIR_5:
		LocalReg.bit.SDE_SCAN_FUN_SEL = 3;
		break;
	case SDE_OUT_SCAN_DIR_7:
		LocalReg.bit.SDE_SCAN_FUN_SEL = 4;
		break;
	default:
		//no change
		break;
	}

	SDE_SETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS, LocalReg.reg);

	return E_OK;

}


static ER sde_setInvValSelBit(SDE_INV_SEL *pInvSel)
{
	T_SDE_FUNCTION_CONTROL_REGISTER    LocalReg;

	LocalReg.reg = SDE_GETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS);
	switch (*pInvSel) {
	case SDE_INV_0:
		LocalReg.bit.DISP_INVALID_VAL_SEL = 0;
		break;
	case SDE_INV_63:
		LocalReg.bit.DISP_INVALID_VAL_SEL = 1;
		break;
	case SDE_INV_255:
		LocalReg.bit.DISP_INVALID_VAL_SEL = 2;
		break;
	default:
		//no change
		break;
	}

	SDE_SETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS, LocalReg.reg);

	return E_OK;

}


static ER sde_setLumaPara(SDE_LUMA_PARA *pLumaPara)
{
	T_LUMA_RELATED_THRESHOLD_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(LUMA_RELATED_THRESHOLD_REGISTER_OFS);
	LocalReg.bit.LUMA_THRESH_SATURATED = pLumaPara->uiLumaThreshSaturated;
	LocalReg.bit.COST_SATURATED_MATCH = pLumaPara->uiCostSaturatedMatch;
	LocalReg.bit.DELTA_LUMA_SMOOTH_THRESH = pLumaPara->uiDeltaLumaSmoothThresh;

	SDE_SETREG(LUMA_RELATED_THRESHOLD_REGISTER_OFS, LocalReg.reg);
	return E_OK;
}


static ER sde_setCostPara(SDE_COST_PARA *pCostPara)
{
	T_DIFF_CLAMP_LUT_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(DIFF_CLAMP_LUT_REGISTER_OFS);
	LocalReg.bit.AD_BOUND_UPPER = pCostPara->uiAdBoundUpper;
	LocalReg.bit.AD_BOUND_LOWER = pCostPara->uiAdBoundLower;
	LocalReg.bit.CENSUS_BOUND_UPPER = pCostPara->uiCensusBoundUpper;
	LocalReg.bit.CENSUS_BOUND_LOWER = pCostPara->uiCensusBoundLower;
	LocalReg.bit.CENSUS_AD_RATIO = pCostPara->uiCensusAdRatio;

	SDE_SETREG(DIFF_CLAMP_LUT_REGISTER_OFS, LocalReg.reg);

	return E_OK;
}


static ER sde_setPenaltyPara(SDE_PENALTY_PARA *pPenaltyPara)
{
	T_PENALTY_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(PENALTY_REGISTER_OFS);
	LocalReg.bit.SCAN_PENALTY_NONSMOOTH = pPenaltyPara->uiScanPenaltyNonsmooth;
	LocalReg.bit.SCAN_PENALTY_SMOOTH0 = pPenaltyPara->uiScanPenaltySmooth0;
	LocalReg.bit.SCAN_PENALTY_SMOOTH1 = pPenaltyPara->uiScanPenaltySmooth1;
	LocalReg.bit.SCAN_PENALTY_SMOOTH2 = pPenaltyPara->uiScanPenaltySmooth2;

	SDE_SETREG(PENALTY_REGISTER_OFS, LocalReg.reg);
	return E_OK;
}

static ER sde_setSmoothPara(SDE_SMOOTH_PARA *pSmoothPara)
{
	T_PENALTY_THRESHOLD_INDEX_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(PENALTY_THRESHOLD_INDEX_REGISTER_OFS);
	LocalReg.bit.DELTA_DISP_LUT0 = pSmoothPara->uiDeltaDispLut0;
	LocalReg.bit.DELTA_DISP_LUT1 = pSmoothPara->uiDeltaDispLut1;

	if (pSmoothPara->uiDeltaDispLut1 < 3) {
		DBG_WRN("SDE DispLut1 should not be smaller than 3, set to 3 instead!\r\n");
		LocalReg.bit.DELTA_DISP_LUT1 = 3;
	}

	if (pSmoothPara->uiDeltaDispLut1 > 15) {
		DBG_WRN("SDE DispLut1 should not be bigger than 15, set to 15 instead!\r\n");
		LocalReg.bit.DELTA_DISP_LUT1 = 15;
	}

	if (pSmoothPara->uiDeltaDispLut1 % 2 == 0) {
		DBG_ERR("SDE parameter error, DispLut1 should be odd numbers in range of 3-15!!\r\n");
		return E_PAR;
	}

	SDE_SETREG(PENALTY_THRESHOLD_INDEX_REGISTER_OFS, LocalReg.reg);
	return E_OK;
}

static ER sde_setDiagThresh(SDE_DIAG_THRESH *pThDiag)
{
	T_DIAGONAL_SEARCH_THRESHOLD_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(DIAGONAL_SEARCH_THRESHOLD_REGISTER_OFS);
	LocalReg.bit.DIAG_THRESH_LUT0 = pThDiag->uiDiagThreshLut0;
	LocalReg.bit.DIAG_THRESH_LUT1 = pThDiag->uiDiagThreshLut1;
	LocalReg.bit.DIAG_THRESH_LUT2 = pThDiag->uiDiagThreshLut2;
	LocalReg.bit.DIAG_THRESH_LUT3 = pThDiag->uiDiagThreshLut3;
	LocalReg.bit.DIAG_THRESH_LUT4 = pThDiag->uiDiagThreshLut4;
	LocalReg.bit.DIAG_THRESH_LUT5 = pThDiag->uiDiagThreshLut5;
	LocalReg.bit.DIAG_THRESH_LUT6 = pThDiag->uiDiagThreshLut6;

	SDE_SETREG(DIAGONAL_SEARCH_THRESHOLD_REGISTER_OFS, LocalReg.reg);

	return E_OK;
}

static ER sde_setConfidenceTh(UINT8 confid_th)
{
	T_CONFIDENCE_REGISTER  LocalReg;
	
	LocalReg.reg = SDE_GETREG(CONFIDENCE_REGISTER_OFS);
	LocalReg.bit.CONFIDENCE_TH = confid_th;
	//LocalReg.bit.CONFIDENCE_VAL = 0;//not exist, be removed. 
	
	SDE_SETREG(CONFIDENCE_REGISTER_OFS, LocalReg.reg);
	return E_OK;
}

void sde_linked_list_fire(void)
{
	T_ENGINE_CONTROL_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(ENGINE_CONTROL_REGISTER_OFS);
	LocalReg.bit.LL_FIRE = 0x1;
	SDE_SETREG(ENGINE_CONTROL_REGISTER_OFS, LocalReg.reg);
}

void sde_set_link_list_terminate(void)
{
	T_LL_REGISTER0 LocalReg;
	LocalReg.reg = SDE_GETREG(LL_REGISTER0_OFS);
	LocalReg.bit.LL_TERMINATE = 0x1;	
	SDE_SETREG(LL_REGISTER0_OFS, LocalReg.reg);
}

void sde_set_disp_val_mode(BOOL mode_opt)
{
	T_SDE_FUNCTION_CONTROL_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS);
	LocalReg.bit.SDE_DISP_VAL_MODE = mode_opt;
	SDE_SETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS, LocalReg.reg);	
}

void sde_set_conditional_disp_en(BOOL enable)
{
	T_SDE_FUNCTION_CONTROL_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS);
	LocalReg.bit.SDE_CONDITIONAL_DISP_EN = enable;
	SDE_SETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS, LocalReg.reg);
}

void sde_set_conf_out2_en(BOOL enable)
{
	T_SDE_FUNCTION_CONTROL_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS);
	LocalReg.bit.SDE_CONF_OUT2_EN = enable;
	SDE_SETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS, LocalReg.reg);
}

void sde_set_conf_out2_mode_sel(BOOL mode_opt)
{
	T_SDE_FUNCTION_CONTROL_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS);
	LocalReg.bit.SDE_CONF_OUT2_MODE_SEL= mode_opt;
	SDE_SETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS, LocalReg.reg);
}

void sde_set_conf_min2_sel(BOOL sel_opt)
{
	T_SDE_FUNCTION_CONTROL_REGISTER LocalReg;
	LocalReg.reg = SDE_GETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS);
	LocalReg.bit.SDE_CONF_MIN2_SEL = sel_opt;
	SDE_SETREG(SDE_FUNCTION_CONTROL_REGISTER_OFS, LocalReg.reg);	
}

void sde_set_LL_start_addr(UINT32 st_addr)
{
	T_LL_REGISTER1 LocalReg;
	//LocalReg.reg = SDE_GETREG(LL_REGISTER1_OFS);
	LocalReg.reg = st_addr;
	SDE_SETREG(LL_REGISTER1_OFS, LocalReg.reg);

}



#ifdef __KERNEL__
EXPORT_SYMBOL(sde_open);
EXPORT_SYMBOL(sde_start);
EXPORT_SYMBOL(sde_waitFlagFrameEnd);
EXPORT_SYMBOL(sde_clrFrameEnd);
EXPORT_SYMBOL(sde_close);
EXPORT_SYMBOL(sde_setMode);
EXPORT_SYMBOL(sde_pause);



#endif






