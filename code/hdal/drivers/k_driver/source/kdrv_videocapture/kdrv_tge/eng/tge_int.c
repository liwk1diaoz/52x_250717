/**
    TGE module driver

    NT96460 TGE driver.

    @file       TGE.c
    @ingroup    mIIPPTGE

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/

// include files for FW
#ifdef __KERNEL__
#include    "mach/fmem.h"
#endif
#include    "kwrap/type.h"
#include    "tge_lib.h"//header for TGE: external enum/struct/api/variable
#include    "tge_int.h"//header for TGE: internal enum/struct/api/variable
#include    "tge_reg.h"//header for TGE: reg struct
#include    "tge_platform.h"

#if 0 // Note: platform combination
UINT32 _TGE_REG_BASE_ADDR;

void tge_setBaseAddr(UINT32 uiAddr)
{
	_TGE_REG_BASE_ADDR = uiAddr;
}
#endif
void tge_setReset(BOOL bReset)
{
	T_R0_ENGINE_CONTROL regCtrl;
	regCtrl.reg = TGE_GETREG(R0_ENGINE_CONTROL_OFS);
	regCtrl.bit.TGE_RST = bReset;
	TGE_SETREG(R0_ENGINE_CONTROL_OFS, regCtrl.reg);
}
/*
void tge_setVDReset(BOOL bReset)
{
    T_R0_ENGINE_CONTROL regCtrl;
    regCtrl.reg = TGE_GETREG(R0_ENGINE_CONTROL_OFS);
    regCtrl.bit.VD_RST = bReset;
    TGE_SETREG(R0_ENGINE_CONTROL_OFS,regCtrl.reg);
}
*/
void tge_setLoad(void)
{
	T_R0_ENGINE_CONTROL regCtrl;
	regCtrl.reg = TGE_GETREG(R0_ENGINE_CONTROL_OFS);
	regCtrl.bit.VD_LOAD  = 1;
	regCtrl.bit.VD2_LOAD = 1;
	TGE_SETREG(R0_ENGINE_CONTROL_OFS, regCtrl.reg);
}


void tge_setBasicSet(TGE_BASIC_SET_INFO *pBasic)
{
	tge_setVdPhase(&(pBasic->VdPhaseData.VdPhaseData));
	tge_setVd2Phase(&(pBasic->VdPhaseData.Vd2PhaseData));

	tge_setVdInv(&(pBasic->VdInvData.VdInvData));
	tge_setVd2Inv(&(pBasic->VdInvData.Vd2InvData));
}

void tge_setVdPhase(TGE_VD_PHASE *pPhaseSel)
{
	T_R8_ENGINE_CONTROL regCtrl8;
	regCtrl8.reg = TGE_GETREG(R8_ENGINE_CONTROL_OFS);

	regCtrl8.bit.VD_PHASE             = pPhaseSel->VdPhase;
	regCtrl8.bit.HD_PHASE             = pPhaseSel->HdPhase;

	TGE_SETREG(R8_ENGINE_CONTROL_OFS, regCtrl8.reg);
}
void tge_setVd2Phase(TGE_VD_PHASE *pPhaseSel)
{
	T_R8_ENGINE_CONTROL regCtrl8;
	regCtrl8.reg = TGE_GETREG(R8_ENGINE_CONTROL_OFS);

	regCtrl8.bit.VD2_PHASE            = pPhaseSel->VdPhase;
	regCtrl8.bit.HD2_PHASE            = pPhaseSel->HdPhase;

	TGE_SETREG(R8_ENGINE_CONTROL_OFS, regCtrl8.reg);
}

void tge_setVdInv(TGE_VD_INV *pInvSel)
{
	T_R8_ENGINE_CONTROL regCtrl8;
	regCtrl8.reg = TGE_GETREG(R8_ENGINE_CONTROL_OFS);

	regCtrl8.bit.VD_INV             = pInvSel->bVdInv;
	regCtrl8.bit.HD_INV             = pInvSel->bHdInv;

	TGE_SETREG(R8_ENGINE_CONTROL_OFS, regCtrl8.reg);
}
void tge_setVd2Inv(TGE_VD_INV *pInvSel)
{
	T_R8_ENGINE_CONTROL regCtrl8;
	regCtrl8.reg = TGE_GETREG(R8_ENGINE_CONTROL_OFS);

	regCtrl8.bit.VD2_INV             = pInvSel->bVdInv;
	regCtrl8.bit.HD2_INV             = pInvSel->bHdInv;

	TGE_SETREG(R8_ENGINE_CONTROL_OFS, regCtrl8.reg);
}
void tge_setModeSel(TGE_MODE_SEL_INFO *pMode)
{
	tge_setVdMode(pMode->ModeSel);
	tge_setVd2Mode(pMode->Mode2Sel);
}
void tge_setVdMode(TGE_MODE_SEL ModeSel)
{
	T_R0_ENGINE_CONTROL regCtrl;
	T_R4_ENGINE_CONTROL regCtrl4;

	regCtrl.reg  = TGE_GETREG(R0_ENGINE_CONTROL_OFS);
	regCtrl4.reg = TGE_GETREG(R4_ENGINE_CONTROL_OFS);


	if (ModeSel == MODE_MASTER) {
		regCtrl.bit.MODE_SEL    = 1;
		regCtrl4.bit.VD_HD_IN_SEL = 0/*don't care*/;
	} else if (ModeSel == MODE_SLAVE_TO_PAD) {
		regCtrl.bit.MODE_SEL    = 0;
		regCtrl4.bit.VD_HD_IN_SEL = 0;
	} else if (ModeSel == MODE_SLAVE_TO_CSI) {
		regCtrl.bit.MODE_SEL    = 0;
		regCtrl4.bit.VD_HD_IN_SEL = 1;
	}

	TGE_SETREG(R0_ENGINE_CONTROL_OFS, regCtrl.reg);
	TGE_SETREG(R4_ENGINE_CONTROL_OFS, regCtrl4.reg);
}
void tge_setVd2Mode(TGE_MODE_SEL ModeSel)
{
	T_R0_ENGINE_CONTROL regCtrl;
	T_R4_ENGINE_CONTROL regCtrl4;

	regCtrl.reg  = TGE_GETREG(R0_ENGINE_CONTROL_OFS);
	regCtrl4.reg = TGE_GETREG(R4_ENGINE_CONTROL_OFS);


	if (ModeSel == MODE_MASTER) {
		regCtrl.bit.MODE2_SEL    = 1;
		regCtrl4.bit.VD2_HD2_IN_SEL = 0/*don't care*/;
	} else if (ModeSel == MODE_SLAVE_TO_PAD) {
		regCtrl.bit.MODE2_SEL    = 0;
		regCtrl4.bit.VD2_HD2_IN_SEL = 0;
	} else if (ModeSel == MODE_SLAVE_TO_CSI) {
		regCtrl.bit.MODE2_SEL    = 0;
		regCtrl4.bit.VD2_HD2_IN_SEL = 1;
	}

	TGE_SETREG(R0_ENGINE_CONTROL_OFS, regCtrl.reg);
	TGE_SETREG(R4_ENGINE_CONTROL_OFS, regCtrl4.reg);
}
void tge_setVdTimingPause(TGE_TIMING_PAUSE_INFO *pTimingPauseInfo)
{
	T_R4_ENGINE_CONTROL regCtrl4;
	regCtrl4.reg = TGE_GETREG(R4_ENGINE_CONTROL_OFS);

	if (pTimingPauseInfo->bVdPause) {
		regCtrl4.bit.VD_PAUSE = 1;
	} else {
		regCtrl4.bit.VD_PAUSE = 0;
	}
	if (pTimingPauseInfo->bHdPause) {
		regCtrl4.bit.HD_PAUSE = 1;
	} else {
		regCtrl4.bit.HD_PAUSE = 0;
	}

	TGE_SETREG(R4_ENGINE_CONTROL_OFS, regCtrl4.reg);
}

void tge_setVd2TimingPause(TGE_TIMING_PAUSE_INFO *pTimingPauseInfo)
{
	T_R4_ENGINE_CONTROL regCtrl4;
	regCtrl4.reg = TGE_GETREG(R4_ENGINE_CONTROL_OFS);

	if (pTimingPauseInfo->bVdPause) {
		regCtrl4.bit.VD2_PAUSE = 1;
	} else {
		regCtrl4.bit.VD2_PAUSE = 0;
	}
	if (pTimingPauseInfo->bHdPause) {
		regCtrl4.bit.HD2_PAUSE = 1;
	} else {
		regCtrl4.bit.HD2_PAUSE = 0;
	}

	TGE_SETREG(R4_ENGINE_CONTROL_OFS, regCtrl4.reg);
}


void tge_enableIntEnable(BOOL bEnable, UINT32 uiIntrp)
{
	T_R14_INTERRUPT_EN regInte14;
	regInte14.reg = TGE_GETREG(R14_INTERRUPT_EN_OFS);
	if (bEnable) {
		regInte14.reg |= uiIntrp;
	} else {
		regInte14.reg &= (~uiIntrp);
	}
	TGE_SETREG(R14_INTERRUPT_EN_OFS, regInte14.reg);
}

void tge_setIntEnable(UINT32 uiIntrp)
{
	T_R14_INTERRUPT_EN regInte14;
	regInte14.reg = uiIntrp;
	TGE_SETREG(R14_INTERRUPT_EN_OFS, regInte14.reg);
}

UINT32 tge_getIntEnable(void)
{
	T_R14_INTERRUPT_EN regInte14;
	regInte14.reg = TGE_GETREG(R14_INTERRUPT_EN_OFS);
	return regInte14.reg;
}


UINT32 tge_getIntrStatus(void)
{
	T_R18_INTERRUPT regInt18;
	regInt18.reg = TGE_GETREG(R18_INTERRUPT_OFS);
	return regInt18.reg;
}

void tge_clrIntrStatus(UINT32 uiIntrpStatus)
{
	T_R18_INTERRUPT regInt18;
	regInt18.reg = uiIntrpStatus;
	TGE_SETREG(R18_INTERRUPT_OFS, regInt18.reg);
}

void tge_setAllVdRst(TGE_VD_RST_INFO *pVdRst)
{
	tge_setVdRst(pVdRst->VdRst);
	tge_setVd2Rst(pVdRst->Vd2Rst);
}
void tge_setRst(TGE_VD_RST_INFO *pParam, TGE_CHANGE_RST_SEL FunSel)
{
	T_R0_ENGINE_CONTROL     reg0;
	reg0.reg = TGE_GETREG(R0_ENGINE_CONTROL_OFS);

	if (FunSel & TGE_CHGRST_VD_RST)  {
		reg0.bit.VD_RST  = ~(pParam->VdRst);
	}
	if (FunSel & TGE_CHGRST_VD2_RST) {
		reg0.bit.VD2_RST = ~(pParam->Vd2Rst);
	}


	TGE_SETREG(R0_ENGINE_CONTROL_OFS, reg0.reg);
}
void tge_getVdRstInfo(TGE_VD_RST_INFO *pParam)
{
	T_R0_ENGINE_CONTROL     reg0;
	reg0.reg = TGE_GETREG(R0_ENGINE_CONTROL_OFS);

	pParam->VdRst = (reg0.bit.VD_RST == 0) ? TGE_VD_ENABLE : TGE_VD_DISABLE;
	pParam->Vd2Rst = (reg0.bit.VD2_RST == 0) ? TGE_VD_ENABLE : TGE_VD_DISABLE;
}
void tge_setVdRst(TGE_VD_RST_SEL VdRst)
{
	T_R0_ENGINE_CONTROL     reg0;
	reg0.reg = TGE_GETREG(R0_ENGINE_CONTROL_OFS);

	if (VdRst == TGE_VD_ENABLE) {
		reg0.bit.VD_RST    = 0;
	} else if (VdRst == TGE_VD_DISABLE) {
		reg0.bit.VD_RST    = 1;
	}

	TGE_SETREG(R0_ENGINE_CONTROL_OFS, reg0.reg);
}

void tge_setVd2Rst(TGE_VD_RST_SEL VdRst)
{
	T_R0_ENGINE_CONTROL     reg0;
	reg0.reg = TGE_GETREG(R0_ENGINE_CONTROL_OFS);

	if (VdRst == TGE_VD_ENABLE) {
		reg0.bit.VD2_RST    = 0;
	} else if (VdRst == TGE_VD_DISABLE) {
		reg0.bit.VD2_RST    = 1;
	}

	TGE_SETREG(R0_ENGINE_CONTROL_OFS, reg0.reg);
}

void tge_setVdHdInfo(TGE_VD_RST_INFO *pVdRst, TGE_VDHD_SET *pVdHdInfo)
{
	if (pVdRst->VdRst == TGE_VD_ENABLE) {
		tge_setVdHd(&(pVdHdInfo->VdInfo));
	}
	if (pVdRst->Vd2Rst == TGE_VD_ENABLE) {
		tge_setVd2Hd2(&(pVdHdInfo->Vd2Info));
	}
}

void tge_setVdHd(TGE_VDHD_INFO *pVdHd)
{
	T_R28_VERTICAL_SYNC     reg28;
	T_R2C_VERTICAL_SYNC     reg2C;
	T_R30_VERTICAL_SYNC     reg30;
	T_R34_HORIZONTAL_SYNC   reg34;
	T_R38_HORIZONTAL_SYNC   reg38;
	T_R0_ENGINE_CONTROL     reg0;

	reg0.reg = TGE_GETREG(R0_ENGINE_CONTROL_OFS);

	if ((reg0.bit.VD_RST == 0) && (pVdHd->uiVdPeriod == 0) && (pVdHd->uiVdAssert == 0) && (pVdHd->uiHdPeriod == 0) && (pVdHd->uiHdAssert == 0)) {
		DBG_ERR("VD Period && Assert == 0, HD Period && Assert == 0, ERR !!");
	} else {
		reg28.bit.VD_PERIOD = pVdHd->uiVdPeriod;
		reg2C.bit.VD_ASSERT = pVdHd->uiVdAssert;
		reg30.bit.VD_FRONTBLANK = pVdHd->uiVdFrontBlnk;
		reg34.bit.HD_PERIOD = pVdHd->uiHdPeriod;
		reg34.bit.HD_ASSERT = pVdHd->uiHdAssert;
		reg38.bit.HD_COUNT = pVdHd->uiHdCnt;

		TGE_SETREG(R28_VERTICAL_SYNC_OFS, reg28.reg);
		TGE_SETREG(R2C_VERTICAL_SYNC_OFS, reg2C.reg);
		TGE_SETREG(R30_VERTICAL_SYNC_OFS, reg30.reg);
		TGE_SETREG(R34_HORIZONTAL_SYNC_OFS, reg34.reg);
		TGE_SETREG(R38_HORIZONTAL_SYNC_OFS, reg38.reg);
	}
}

void tge_setVd2Hd2(TGE_VDHD_INFO *pVdHd)
{
	T_R44_VERTICAL_SYNC     reg44;
	T_R48_VERTICAL_SYNC     reg48;
	T_R4C_VERTICAL_SYNC     reg4C;
	T_R50_HORIZONTAL_SYNC   reg50;
	T_R54_HORIZONTAL_SYNC   reg54;
	T_R0_ENGINE_CONTROL     reg0;

	reg0.reg = TGE_GETREG(R0_ENGINE_CONTROL_OFS);

	if ((reg0.bit.VD2_RST == 0) && (pVdHd->uiVdPeriod == 0) && (pVdHd->uiVdAssert == 0) && (pVdHd->uiHdPeriod == 0) && (pVdHd->uiHdAssert == 0)) {
		DBG_ERR("VD2 Period && Assert == 0, HD2 Period && Assert == 0, ERR !!");
	} else {
		reg44.bit.VD2_PERIOD = pVdHd->uiVdPeriod;
		reg48.bit.VD2_ASSERT = pVdHd->uiVdAssert;
		reg4C.bit.VD2_FRONTBLANK = pVdHd->uiVdFrontBlnk;
		reg50.bit.HD2_PERIOD = pVdHd->uiHdPeriod;
		reg50.bit.HD2_ASSERT = pVdHd->uiHdAssert;
		reg54.bit.HD2_COUNT = pVdHd->uiHdCnt;

		TGE_SETREG(R44_VERTICAL_SYNC_OFS, reg44.reg);
		TGE_SETREG(R48_VERTICAL_SYNC_OFS, reg48.reg);
		TGE_SETREG(R4C_VERTICAL_SYNC_OFS, reg4C.reg);
		TGE_SETREG(R50_HORIZONTAL_SYNC_OFS, reg50.reg);
		TGE_SETREG(R54_HORIZONTAL_SYNC_OFS, reg54.reg);
	}
}

void tge_setBreadPoint(TGE_VD_RST_INFO *pVdRst, TGE_BREAKPOINT_INFO *pBP)
{
	if (pVdRst->VdRst == TGE_VD_ENABLE) {
		tge_setVdBreadPoint(pBP->uiVdBp);
	}
	if (pVdRst->Vd2Rst == TGE_VD_ENABLE) {
		tge_setVd2BreadPoint(pBP->uiVd2Bp);
	}
}

void tge_setVdBreadPoint(UINT32 uiBp)
{
	T_R3C_BREAKPOINT regBP_A;
	regBP_A.bit.VD_BP   = uiBp;
	TGE_SETREG(R3C_BREAKPOINT_OFS, regBP_A.reg);
}

void tge_setVd2BreadPoint(UINT32 uiBp)
{
	T_R58_BREAKPOINT regBP_A;
	regBP_A.bit.VD2_BP   = uiBp;
	TGE_SETREG(R58_BREAKPOINT_OFS, regBP_A.reg);
}

void tge_getEngineStatus(TGE_ENGINE_STATUS_INFO *pEngineStatus)
{
	T_R108_STATUS regInt;
	regInt.reg = TGE_GETREG(R108_STATUS_OFS);
	pEngineStatus->uiClkCnt = regInt.bit.CLK_CNT;
	pEngineStatus->bVD = regInt.bit.VD;
	pEngineStatus->bHD = regInt.bit.HD;
	pEngineStatus->bFlshExtTrg = regInt.bit.FLSH_EXT_TRG;
	pEngineStatus->bMshExtTrg = regInt.bit.MSH_EXT_TRG;
	pEngineStatus->bFlshCtrl = regInt.bit.FLSH_CTRL;
	pEngineStatus->bMshCtrlA0 = regInt.bit.MES_CTRL_A0;
	pEngineStatus->bMshCtrlA1 = regInt.bit.MES_CTRL_A1;
	pEngineStatus->bMshCtrlB0 = regInt.bit.MES_CTRL_B0;
	pEngineStatus->bMshCtrlB1 = regInt.bit.MES_CTRL_B1;
}

//#endif
//@}
