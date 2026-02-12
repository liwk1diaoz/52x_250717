/*
    IVE module driver

    NT96680 IVE driver.

    @file       ive_int.c
    @ingroup    mIDrvIPPIVE

    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.
*/
#include "ive_int.h"
#include "ive_platform.h"

/*--------- driver default value ---------*/
/*----------------------------------------*/
static volatile ID g_IveLockStatus = NO_TASK_LOCKED;

/*-----------Internal function API definition ------------*/
// engine control API
VOID ive_setResetReg(BOOL bReset)
{
	T_ENGINE_CONTROL_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(ENGINE_CONTROL_REGISTER_OFS);
	LocalReg.bit.IVE_RST = bReset;
	IVE_SETREG(ENGINE_CONTROL_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setStartReg(BOOL bStart)
{
	T_ENGINE_CONTROL_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(ENGINE_CONTROL_REGISTER_OFS);
	LocalReg.bit.IVE_START = bStart;
	IVE_SETREG(ENGINE_CONTROL_REGISTER_OFS, LocalReg.reg);
}

// flag related API
VOID ive_setIntrEnableReg(UINT32 uiIntrOp)
{
	T_IVE_INTERRUPT_ENABLE_REGISTER LocalReg;
	LocalReg.reg = uiIntrOp;
	IVE_SETREG(IVE_INTERRUPT_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_clrIntrStatusReg(UINT32 uiIntrStatus)
{
	T_IVE_INTERRUPT_STATUS_REGISTER LocalReg;
	LocalReg.reg = uiIntrStatus;
	IVE_SETREG(IVE_INTERRUPT_STATUS_REGISTER_OFS, LocalReg.reg);
}

// function control API
VOID ive_setGenFiltEnReg(BOOL bEn)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.GEN_FILT_EN = bEn;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setMednFiltEnReg(BOOL bEn)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.MEDN_FILT_EN = bEn;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setMednFiltModeReg(UINT32 uiFiltMode)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.MEDN_FILT_MODE = uiFiltMode;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setEdgeFiltEnReg(BOOL bEn)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.EDGE_FILT_EN = bEn;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setEdgeFiltModeReg(UINT32 uiFiltMode)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.EDGE_MODE = uiFiltMode;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

//IRV
VOID ive_setIrvEnReg(BOOL bEn)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.IRV_EN = bEn;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setIrvHistModeSelReg(BOOL bHistModeSel)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.IRV_HIST_MODE_SEL = bHistModeSel;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setIrvInvalidValReg(BOOL bInvalidVal)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.IRV_INVALID_VAL = bInvalidVal;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setIrvThrSReg(UINT32 uiIrvThrS)
{
	T_IVE_IRV_SETTING_REGISTER0_528 LocalReg_528;

	LocalReg_528.reg = IVE_GETREG(IVE_IRV_SETTING_REGISTER0_OFS_528);
	LocalReg_528.bit.IRV_THR_S = uiIrvThrS;
	IVE_SETREG(IVE_IRV_SETTING_REGISTER0_OFS_528, LocalReg_528.reg);
}

VOID ive_setIrvThrHReg(UINT32 uiIrvThrH)
{
	T_IVE_IRV_SETTING_REGISTER0_528 LocalReg_528;

	LocalReg_528.reg = IVE_GETREG(IVE_IRV_SETTING_REGISTER0_OFS_528);
	LocalReg_528.bit.IRV_THR_H = uiIrvThrH;
	IVE_SETREG(IVE_IRV_SETTING_REGISTER0_OFS_528, LocalReg_528.reg);
}

VOID ive_setIrvMednInvalThReg(UINT32 uiIrvMednInvalTh)
{
	T_IVE_IRV_SETTING_REGISTER0_528 LocalReg_528;

	LocalReg_528.reg = IVE_GETREG(IVE_IRV_SETTING_REGISTER0_OFS_528);
	LocalReg_528.bit.MEDN_INVAL_TH = uiIrvMednInvalTh;
	IVE_SETREG(IVE_IRV_SETTING_REGISTER0_OFS_528, LocalReg_528.reg);
}
//End IRV

VOID ive_setEdgeAngSlpFactReg(UINT32 uiSlpFact)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.ANGLE_SLP_FACT = uiSlpFact;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setEdgeShiftReg(UINT32 uiShiftBit)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.EDGE_SHIFT_BIT = uiShiftBit;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setNonMaxSupEnReg(BOOL bEn)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.NON_MAX_SUP_EN = bEn;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setThresLutEnReg(BOOL bEn)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.THRES_LUT_EN = bEn;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setMorphFiltEnReg(BOOL bEn)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.MORPH_FILT_EN = bEn;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setMorphInSelReg(UINT32 uiMorphInSel)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.MORPH_IN_SEL = uiMorphInSel;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setMorphOpReg(UINT32 uiMorphOp)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.MORPH_OP = uiMorphOp;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setIntegralEnReg(BOOL bEn)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.INTEGRAL_EN = bEn;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}



VOID ive_setOutDataReg(UINT32 uiOutDataSel)
{
	T_FUNCTION_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(FUNCTION_ENABLE_REGISTER_OFS);
	LocalReg.bit.OUT_DATA_SEL = uiOutDataSel;
	IVE_SETREG(FUNCTION_ENABLE_REGISTER_OFS, LocalReg.reg);
}

// dram in API
VOID ive_setInAddrReg(UINT32 uiInAddr)
{
	T_IVE_DMA_INPUT_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_DMA_INPUT_REGISTER_OFS);
	LocalReg.bit.DRAM_IN_SADDR = uiInAddr >> 2;
	IVE_SETREG(IVE_DMA_INPUT_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setLLAddrReg(UINT32 uiInAddr)
{
	T_IVE_LL_ADDRESS_RESISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_LL_ADDRESS_RESISTER_OFS);
	LocalReg.bit.DRAM_LL_SAI = uiInAddr >> 2;
	IVE_SETREG(IVE_LL_ADDRESS_RESISTER_OFS, LocalReg.reg);
}

VOID ive_setInLofstReg(UINT32 uiInLofst)
{
	T_IVE_DMA_INPUT_LINE_OFFSET_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_DMA_INPUT_LINE_OFFSET_REGISTER_OFS);
	LocalReg.bit.DRAM_IN_LOFST = uiInLofst >> 2;
	IVE_SETREG(IVE_DMA_INPUT_LINE_OFFSET_REGISTER_OFS, LocalReg.reg);
}

// dram out API
VOID ive_setOutAddrReg(UINT32 uiOutAddr)
{
	T_IVE_DMA_OUTPUT_RESULT_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_DMA_OUTPUT_RESULT_REGISTER_OFS);
	LocalReg.bit.DRAM_OUT_SADDR = uiOutAddr >> 2;
	IVE_SETREG(IVE_DMA_OUTPUT_RESULT_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setOutLofstReg(UINT32 uiOutLofst)
{
	T_IVE_DMA_OUTPUT_LINE_OFFSET_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_DMA_OUTPUT_LINE_OFFSET_REGISTER_OFS);
	LocalReg.bit.DRAM_OUT_LOFST = uiOutLofst >> 2;
	IVE_SETREG(IVE_DMA_OUTPUT_LINE_OFFSET_REGISTER_OFS, LocalReg.reg);
}

// output image data size API
VOID ive_setInImgWidthReg(UINT32 uiWidth)
{
	T_INPUT_IMAGE_SIZE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(INPUT_IMAGE_SIZE_REGISTER_OFS);
	LocalReg.bit.IMG_WIDTH = uiWidth;
	IVE_SETREG(INPUT_IMAGE_SIZE_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setInImgHeightReg(UINT32 uiHeight)
{
	T_INPUT_IMAGE_SIZE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(INPUT_IMAGE_SIZE_REGISTER_OFS);
	LocalReg.bit.IMG_HEIGHT = uiHeight;
	IVE_SETREG(INPUT_IMAGE_SIZE_REGISTER_OFS, LocalReg.reg);
}

//set general filter parameters
VOID ive_setGenFiltCoeffReg(UINT32 *puiCoeff)
{
	T_GENERAL_FILTER_REGISTER_0 LocalReg;
	T_GENERAL_FILTER_REGISTER_1 LocalReg1;

	LocalReg.reg = IVE_GETREG(GENERAL_FILTER_REGISTER_0_OFS);
	LocalReg.bit.GEN_FILT_COEFF0 = puiCoeff[0];
	LocalReg.bit.GEN_FILT_COEFF1 = puiCoeff[1];
	LocalReg.bit.GEN_FILT_COEFF2 = puiCoeff[2];
	LocalReg.bit.GEN_FILT_COEFF3 = puiCoeff[3];
	LocalReg.bit.GEN_FILT_COEFF4 = puiCoeff[4];
	LocalReg.bit.GEN_FILT_COEFF5 = puiCoeff[5];
	LocalReg.bit.GEN_FILT_COEFF6 = puiCoeff[6];
	LocalReg.bit.GEN_FILT_COEFF7 = puiCoeff[7];
	IVE_SETREG(GENERAL_FILTER_REGISTER_0_OFS, LocalReg.reg);

	LocalReg1.reg = IVE_GETREG(GENERAL_FILTER_REGISTER_1_OFS);
	LocalReg1.bit.GEN_FILT_COEFF8 = puiCoeff[8];
	LocalReg1.bit.GEN_FILT_COEFF9 = puiCoeff[9];
	IVE_SETREG(GENERAL_FILTER_REGISTER_1_OFS, LocalReg1.reg);
}

//set edge filter parameters
VOID ive_setEdge1CoeffReg(UINT32 *puiEdgeCoeff)
{
	T_EDGE_FILTER_REGISTER_0 LocalReg;
	T_EDGE_FILTER_REGISTER_1 LocalReg1;

	LocalReg.reg = IVE_GETREG(EDGE_FILTER_REGISTER_0_OFS);
	LocalReg.bit.EDGE_FILT1_COEFF0 = puiEdgeCoeff[0];
	LocalReg.bit.EDGE_FILT1_COEFF1 = puiEdgeCoeff[1];
	LocalReg.bit.EDGE_FILT1_COEFF2 = puiEdgeCoeff[2];
	LocalReg.bit.EDGE_FILT1_COEFF3 = puiEdgeCoeff[3];
	LocalReg.bit.EDGE_FILT1_COEFF4 = puiEdgeCoeff[4];
	LocalReg.bit.EDGE_FILT1_COEFF5 = puiEdgeCoeff[5];
	IVE_SETREG(EDGE_FILTER_REGISTER_0_OFS, LocalReg.reg);

	LocalReg1.reg = IVE_GETREG(EDGE_FILTER_REGISTER_1_OFS);
	LocalReg1.bit.EDGE_FILT1_COEFF6 = puiEdgeCoeff[6];
	LocalReg1.bit.EDGE_FILT1_COEFF7 = puiEdgeCoeff[7];
	LocalReg1.bit.EDGE_FILT1_COEFF8 = puiEdgeCoeff[8];
	IVE_SETREG(EDGE_FILTER_REGISTER_1_OFS, LocalReg1.reg);
}

VOID ive_setEdge2CoeffReg(UINT32 *puiEdgeCoeff)
{
	T_EDGE_FILTER_REGISTER_1 LocalReg;
	T_EDGE_FILTER_REGISTER_2 LocalReg1;

	LocalReg.reg = IVE_GETREG(EDGE_FILTER_REGISTER_1_OFS);
	LocalReg.bit.EDGE_FILT2_COEFF0 = puiEdgeCoeff[0];
	LocalReg.bit.EDGE_FILT2_COEFF1 = puiEdgeCoeff[1];
	LocalReg.bit.EDGE_FILT2_COEFF2 = puiEdgeCoeff[2];
	IVE_SETREG(EDGE_FILTER_REGISTER_1_OFS, LocalReg.reg);

	LocalReg1.reg = IVE_GETREG(EDGE_FILTER_REGISTER_2_OFS);
	LocalReg1.bit.EDGE_FILT2_COEFF3 = puiEdgeCoeff[3];
	LocalReg1.bit.EDGE_FILT2_COEFF4 = puiEdgeCoeff[4];
	LocalReg1.bit.EDGE_FILT2_COEFF5 = puiEdgeCoeff[5];
	LocalReg1.bit.EDGE_FILT2_COEFF6 = puiEdgeCoeff[6];
	LocalReg1.bit.EDGE_FILT2_COEFF7 = puiEdgeCoeff[7];
	LocalReg1.bit.EDGE_FILT2_COEFF8 = puiEdgeCoeff[8];
	IVE_SETREG(EDGE_FILTER_REGISTER_2_OFS, LocalReg1.reg);
}

//set threshold LUT parameters
VOID ive_setThresLutReg(UINT32 *puiThresLut)
{
	T_EDGE_THRESHOLD_LUT_REGISTER_0 LocalReg;
	T_EDGE_THRESHOLD_LUT_REGISTER_1 LocalReg1;
	T_EDGE_THRESHOLD_LUT_REGISTER_2 LocalReg2;
	T_EDGE_THRESHOLD_LUT_REGISTER_3 LocalReg3;

	LocalReg.reg = IVE_GETREG(EDGE_THRESHOLD_LUT_REGISTER_0_OFS);
	LocalReg.bit.EDGE_THRES_LUT0 = puiThresLut[0];
	LocalReg.bit.EDGE_THRES_LUT1 = puiThresLut[1];
	LocalReg.bit.EDGE_THRES_LUT2 = puiThresLut[2];
	LocalReg.bit.EDGE_THRES_LUT3 = puiThresLut[3];
	IVE_SETREG(EDGE_THRESHOLD_LUT_REGISTER_0_OFS, LocalReg.reg);

	LocalReg1.reg = IVE_GETREG(EDGE_THRESHOLD_LUT_REGISTER_1_OFS);
	LocalReg1.bit.EDGE_THRES_LUT4 = puiThresLut[4];
	LocalReg1.bit.EDGE_THRES_LUT5 = puiThresLut[5];
	LocalReg1.bit.EDGE_THRES_LUT6 = puiThresLut[6];
	LocalReg1.bit.EDGE_THRES_LUT7 = puiThresLut[7];
	IVE_SETREG(EDGE_THRESHOLD_LUT_REGISTER_1_OFS, LocalReg1.reg);

	LocalReg2.reg = IVE_GETREG(EDGE_THRESHOLD_LUT_REGISTER_2_OFS);
	LocalReg2.bit.EDGE_THRES_LUT8  = puiThresLut[8];
	LocalReg2.bit.EDGE_THRES_LUT9  = puiThresLut[9];
	LocalReg2.bit.EDGE_THRES_LUT10 = puiThresLut[10];
	LocalReg2.bit.EDGE_THRES_LUT11 = puiThresLut[11];
	IVE_SETREG(EDGE_THRESHOLD_LUT_REGISTER_2_OFS, LocalReg2.reg);

	LocalReg3.reg = IVE_GETREG(EDGE_THRESHOLD_LUT_REGISTER_3_OFS);
	LocalReg3.bit.EDGE_THRES_LUT12 = puiThresLut[12];
	LocalReg3.bit.EDGE_THRES_LUT13 = puiThresLut[13];
	LocalReg3.bit.EDGE_THRES_LUT14 = puiThresLut[14];
	IVE_SETREG(EDGE_THRESHOLD_LUT_REGISTER_3_OFS, LocalReg3.reg);
}

VOID ive_setEdgeMagThReg(UINT32 uiEdgeMagTh)
{
	T_EDGE_THRESHOLD_LUT_REGISTER_3 LocalReg;
	LocalReg.reg = IVE_GETREG(EDGE_THRESHOLD_LUT_REGISTER_3_OFS);
	LocalReg.bit.EDGE_MAG_TH = uiEdgeMagTh;
	IVE_SETREG(EDGE_THRESHOLD_LUT_REGISTER_3_OFS, LocalReg.reg);
}

//set threshold LUT parameters
VOID ive_setMorphNeighEnReg(BOOL *pbMorphNeighEn)
{
	T_MORPHOLOGICAL_FILTER_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(MORPHOLOGICAL_FILTER_REGISTER_OFS);
	LocalReg.bit.NEIGH0_EN  = pbMorphNeighEn[0];
	LocalReg.bit.NEIGH1_EN  = pbMorphNeighEn[1];
	LocalReg.bit.NEIGH2_EN  = pbMorphNeighEn[2];
	LocalReg.bit.NEIGH3_EN  = pbMorphNeighEn[3];
	LocalReg.bit.NEIGH4_EN  = pbMorphNeighEn[4];
	LocalReg.bit.NEIGH5_EN  = pbMorphNeighEn[5];
	LocalReg.bit.NEIGH6_EN  = pbMorphNeighEn[6];
	LocalReg.bit.NEIGH7_EN  = pbMorphNeighEn[7];
	LocalReg.bit.NEIGH8_EN  = pbMorphNeighEn[8];
	LocalReg.bit.NEIGH9_EN  = pbMorphNeighEn[9];
	LocalReg.bit.NEIGH10_EN = pbMorphNeighEn[10];
	LocalReg.bit.NEIGH11_EN = pbMorphNeighEn[11];
	LocalReg.bit.NEIGH12_EN = pbMorphNeighEn[12];
	LocalReg.bit.NEIGH13_EN = pbMorphNeighEn[13];
	LocalReg.bit.NEIGH14_EN = pbMorphNeighEn[14];
	LocalReg.bit.NEIGH15_EN = pbMorphNeighEn[15];
	LocalReg.bit.NEIGH16_EN = pbMorphNeighEn[16];
	LocalReg.bit.NEIGH17_EN = pbMorphNeighEn[17];
	LocalReg.bit.NEIGH18_EN = pbMorphNeighEn[18];
	LocalReg.bit.NEIGH19_EN = pbMorphNeighEn[19];
	LocalReg.bit.NEIGH20_EN = pbMorphNeighEn[20];
	LocalReg.bit.NEIGH21_EN = pbMorphNeighEn[21];
	LocalReg.bit.NEIGH22_EN = pbMorphNeighEn[22];
	LocalReg.bit.NEIGH23_EN = pbMorphNeighEn[23];
	IVE_SETREG(MORPHOLOGICAL_FILTER_REGISTER_OFS, LocalReg.reg);
}


VOID ive_setInBurstLengthReg(UINT32 InBurstSel)
{
	T_IVE_BURST_LENGTH_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_BURST_LENGTH_REGISTER_OFS);
	LocalReg.bit.IVE_IN_BURST_LEN_SEL = InBurstSel;
	IVE_SETREG(IVE_BURST_LENGTH_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setOutBurstLengthReg(UINT32 OutBurstSel)
{
	T_IVE_BURST_LENGTH_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_BURST_LENGTH_REGISTER_OFS);
	LocalReg.bit.IVE_OUT_BURST_LEN_SEL = OutBurstSel;
	IVE_SETREG(IVE_BURST_LENGTH_REGISTER_OFS, LocalReg.reg);
}

VOID ive_setDbgPortReg(UINT32 DbgPortSel)
{
	T_IVE_BURST_LENGTH_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_BURST_LENGTH_REGISTER_OFS);
	//LocalReg.bit.IVE_DBG_PORT_SEL = DbgPortSel;
	IVE_SETREG(IVE_BURST_LENGTH_REGISTER_OFS, LocalReg.reg);
}

//get reg value APIs
UINT32 ive_getIntrEnableReg(VOID)
{
	T_IVE_INTERRUPT_ENABLE_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_INTERRUPT_ENABLE_REGISTER_OFS);
	return LocalReg.reg;
}

UINT32 ive_getIntrStatusReg(VOID)
{
	//T_IVE_INTERRUPT_ENABLE_REGISTER LocalReg;
	//LocalReg.reg = IVE_GETREG(IVE_INTERRUPT_ENABLE_REGISTER_OFS); //sc
	T_IVE_INTERRUPT_STATUS_REGISTER LocalReg; //sc
	LocalReg.reg = IVE_GETREG(IVE_INTERRUPT_STATUS_REGISTER_OFS);
	return LocalReg.reg;
}
/*
UINT32 ive_getIrvModCntReg(VOID)
{
	T_IRV_DEBUG_STATUS_REGISTER_0 LocalReg;
	LocalReg.reg = IVE_GETREG(IRV_DEBUG_STATUS_REGISTER_0_OFS);
	return LocalReg.bit.IRV_MOD_CNT;
}

UINT32 ive_getIrvNmodCntReg(VOID)
{
	T_IRV_DEBUG_STATUS_REGISTER_1 LocalReg;
	LocalReg.reg = IVE_GETREG(IRV_DEBUG_STATUS_REGISTER_1_OFS);
	return LocalReg.bit.IRV_NMOD_CNT;
}
*/
// dram in API
UINT32 ive_getInAddrReg(VOID)
{
	T_IVE_DMA_INPUT_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_DMA_INPUT_REGISTER_OFS);
	return LocalReg.bit.DRAM_IN_SADDR << 2;
}

UINT32 ive_getInLofstReg(VOID)
{
	T_IVE_DMA_INPUT_LINE_OFFSET_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_DMA_INPUT_LINE_OFFSET_REGISTER_OFS);
	return LocalReg.bit.DRAM_IN_LOFST << 2;
}

// dram out API
UINT32 ive_getOutAddrReg(VOID)
{
	T_IVE_DMA_OUTPUT_RESULT_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_DMA_OUTPUT_RESULT_REGISTER_OFS);
	return LocalReg.bit.DRAM_OUT_SADDR << 2;
}

UINT32 ive_getOutLofstReg(VOID)
{
	T_IVE_DMA_OUTPUT_LINE_OFFSET_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_DMA_OUTPUT_LINE_OFFSET_REGISTER_OFS);
	return LocalReg.bit.DRAM_OUT_LOFST << 2;
}
/*------------ engine operation API --------------*/

/*
    IVE attach driver

    IVE engine clock enable

    @return
    -@b E_OK : Enable clock done.
*/
ER ive_attach()
{
	ive_platform_enable_clk();
	return E_OK;
}

/*
    IVE detach driver

    IVE engine clock disable

    @return
    -@b E_OK : Disable clock done.
*/
ER ive_detach()
{
	ive_platform_disable_clk();
	return E_OK;
}

/*
    IVE start driver

    start engine to run or stop engine

    @return
    -@b E_OK : start/stop engine done.
*/
ER ive_enable(BOOL bStartOp)
{
	ive_setStartReg(bStartOp);
	return E_OK;
}

void ive_ll_enable(BOOL bStart)
{
	T_ENGINE_CONTROL_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(ENGINE_CONTROL_REGISTER_OFS);
	LocalReg.bit.LL_FIRE = bStart;
	IVE_SETREG(ENGINE_CONTROL_REGISTER_OFS, LocalReg.reg);
}

void ive_ll_terminate(BOOL isterminate)
{
	T_IVE_TERMINATE_REGISTER reg1;
	reg1.reg = IVE_GETREG(IVE_TERMINATE_REGISTER_OFS);
	reg1.bit.LL_TERMINATE = isterminate;
	if (isterminate) {
		reg1.bit.LL_TERMINATE = 0;
		reg1.bit.LL_TERMINATE = isterminate;
	}
	IVE_SETREG(IVE_TERMINATE_REGISTER_OFS, reg1.reg);
}

/*
    IVE lock driver

    IVE engine task lock

    @return
    -@b E_OK: Done with no error
    -@b others: Error occurs
*/
ER ive_lock(VOID)
{
	ER erReturn = E_OK;

	if (g_IveLockStatus == NO_TASK_LOCKED) {

		erReturn = ive_platform_sem_wait();
		if (erReturn != E_OK) {
			return erReturn;
		}

		g_IveLockStatus = TASK_LOCKED;
		erReturn = E_OK;
	} else {
		erReturn = E_OBJ;
	}

	return erReturn;
}

/*
    IVE unlock driver

    IVE engine task unlock

    @return
    -@b E_OK: Done with no error
    -@b others: Error occurs
*/
ER ive_unlock(VOID)
{
	ER erReturn = E_OK;

	if (g_IveLockStatus == TASK_LOCKED) {
		g_IveLockStatus = NO_TASK_LOCKED;
		erReturn = ive_platform_sem_signal();
	} else {
		erReturn = E_OBJ;
	}

	return erReturn;
}

/*
    IVE check open

    Check if IVE is opened or not

    @return
    -@b TRUE: engine is opened
    -@b FALSE: engine is closed
*/
BOOL ive_isOpen(VOID)
{
	BOOL bStatus = TRUE;
	if (g_IveLockStatus == TASK_LOCKED) {
		bStatus = TRUE;
	} else {
		bStatus = FALSE;
		DBG_ERR("IVE is closed!\r\n");
	}

	return bStatus;
}

/*
    IVE software reset driver
*/
ER ive_setReset(VOID)
{
	ive_setResetReg(FALSE);
	ive_setResetReg(TRUE);
	ive_setResetReg(FALSE);
	return E_OK;
}

// dram out API
UINT32 ive_getFrmEndRdyReg(VOID)
{
	T_IVE_INTERRUPT_STATUS_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_INTERRUPT_STATUS_REGISTER_OFS);
	return LocalReg.bit.INT_FRM_END;
}

// dram out API
UINT32 ive_getLLFrmEndRdyReg(VOID)
{
	T_IVE_INTERRUPT_STATUS_REGISTER LocalReg;
	LocalReg.reg = IVE_GETREG(IVE_INTERRUPT_STATUS_REGISTER_OFS);
	return LocalReg.bit.INT_LLEND;
}
