/**
    SIE module driver

    SIE driver.

    @file       sie_int.c
    @ingroup    mIIPPSIE

    Copyright   Novatek Microelectronics Corp. 2010.  All rights reserved.
*/


// include files for FW


//#include    <linux/dma-mapping.h> // header file Dma(cache handle)

#ifdef __KERNEL__
#include    "mach/fmem.h"
#endif

#include    "kwrap/type.h"
#include    "sie_lib.h"//header for SIE: external enum/struct/api/variable
#include    "sie_int.h"//header for SIE: internal enum/struct/api/variable
#include    "sie_reg.h"//header for SIE: reg struct
#include    "sie_platform.h"
#include    <plat/top.h>


#define sqrt(parm)  (parm)
#if (defined __UITRON || defined __ECOS || defined __KERNEL__)
#define dma_getNonCacheAddr(parm) 0
//#define dma_getPhyAddr(parm) 0
#endif

#define CLAMP(x, min, max)    (((x) >= (max)) ? (max) : (((x) <= (min)) ? (min) : (x)))

#define SIE_INT_FLIP_TEST   (1)
static UINT32 g_uiDramOut1VirAddr[8] = {0}, g_uiDramOut2VirAddr[8] = {0};
static UINT32 g_uiDramOut0VirAddr[8] = {0};

static UINT32 irsub_r[8]={0}, irsub_g[8]={0}, irsub_b[8]={0};
static UINT32 satgen_g_last[8] = {0}, satgen_g_cur[8]={0};

//BCC global parameters
UINT32 g_uiBccGammaTblDef[GAM_TBL_TAP] = {
	0, 512, 1024, 1254, 1448, 1619, 1773, 1915, 2048, 2172,
	2289, 2401, 2508, 2610, 2709, 2804, 2896, 2985, 3072, 3156,
	3238, 3318, 3396, 3472, 3547, 3620, 3692, 3762, 3831, 3899,
	3965, 4031, 4095
};

void sie_setReset(UINT32 id, BOOL bReset)
{
	T_R0_ENGINE_CONTROL regCtrl;
	unsigned long spin_flags;

	spin_flags = sie_platform_spin_lock(0);
	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R0_ENGINE_CONTROL_OFS);
	regCtrl.bit.SIE_RST = bReset;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS, regCtrl.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

BOOL sie_getReset(UINT32 id)
{
	T_R0_ENGINE_CONTROL regCtrl;

	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS);
	if (regCtrl.bit.SIE_RST == 1) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void sie_setLoad(UINT32 id)
{
	T_R0_ENGINE_CONTROL regCtrl;
	unsigned long spin_flags;

	spin_flags = sie_platform_spin_lock(0);
	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS);
	regCtrl.bit.SIE_LOAD = 1;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS, regCtrl.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setActEn(UINT32 id, BOOL bActEn)
{
	T_R0_ENGINE_CONTROL regCtrl;
	unsigned long spin_flags;

	spin_flags = sie_platform_spin_lock(0);
	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS);
	regCtrl.bit.SIE_ACT_EN = bActEn;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS, regCtrl.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_setDramOutMode(UINT32 id, SIE_DRAM_OUT_CTRL OutMode)
{
	T_R0_ENGINE_CONTROL regCtrl;
	unsigned long spin_flags;

	spin_flags = sie_platform_spin_lock(0);
	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R0_ENGINE_CONTROL_OFS);
	if (OutMode.out0mode== DRAM_OUT_MODE_SINGLE) {
		regCtrl.bit.DRAM_OUT0_MODE = 1;
	} else {
		regCtrl.bit.DRAM_OUT0_MODE = 0;
	}
	if (OutMode.out1mode == DRAM_OUT_MODE_SINGLE) {
		regCtrl.bit.DRAM_OUT1_MODE = 1;
	} else {
		regCtrl.bit.DRAM_OUT1_MODE = 0;
	}
	if (OutMode.out2mode == DRAM_OUT_MODE_SINGLE) {
		regCtrl.bit.DRAM_OUT2_MODE = 1;
	} else {
		regCtrl.bit.DRAM_OUT2_MODE = 0;
	}
	if (nvt_get_chip_id() == CHIP_NA51055) { 
		if ((OutMode.out0mode != OutMode.out1mode) || (OutMode.out0mode != OutMode.out2mode) || (OutMode.out1mode != OutMode.out2mode))
		{
			nvt_dbg(ERR,"single outmode error out0/1/2 = %d/%d/%d, force to single-out mode\r\n",OutMode.out0mode,OutMode.out1mode,OutMode.out2mode);
			regCtrl.bit.DRAM_OUT0_MODE = 1;
			regCtrl.bit.DRAM_OUT1_MODE = 1;
			regCtrl.bit.DRAM_OUT2_MODE = 1;
		}
	}
	
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R0_ENGINE_CONTROL_OFS, regCtrl.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_setVdHdInterval(UINT32 id, SIE_VDHD_INTERVAL_INFO *IntervalInfo)
{
	T_R13C_VD_INTEV vdIntev;
	T_R140_VD_INTEV vdIntev2;
	T_R144_HD_INTEV hdIntev;
	T_R148_HD_INTEV hdIntev2;
	unsigned long spin_flags;

	spin_flags = sie_platform_spin_lock(0);

	vdIntev.bit.VD_INTERVAL_COUNT_LOWER_THR = IntervalInfo->VdIntervLowerThr;
	vdIntev2.bit.VD_INTERVAL_COUNT_UPPER_THR = IntervalInfo->VdIntervUpperThr;
	hdIntev.bit.HD_INTERVAL_COUNT_LOWER_THR = IntervalInfo->HdIntervLowerThr;
	hdIntev2.bit.HD_INTERVAL_COUNT_UPPER_THR = IntervalInfo->HdIntervUpperThr;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R13C_VD_INTEV_OFS, vdIntev.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R140_VD_INTEV_OFS, vdIntev2.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R144_HD_INTEV_OFS, hdIntev.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R148_HD_INTEV_OFS, hdIntev2.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setMainInput(UINT32 id, SIE_MAIN_INPUT_INFO *pMainInInfo)
{
	T_R0_ENGINE_CONTROL regCtrl;
	unsigned long spin_flags;

	spin_flags = sie_platform_spin_lock(0);

	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS);

	switch (id) {
	case SIE_ENGINE_ID_1:
		if (pMainInInfo->MainInSrc == MAIN_IN_PARA_MSTR_SNR) { // from SIE1 PAD
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 0;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_PARA_SLAV_SNR) { // from TGE VD
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 1; 
		} else if (pMainInInfo->MainInSrc == MAIN_IN_SELF_PATGEN) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 2;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_CSI_1) {
			regCtrl.bit.MAIN_IN_SEL   = 1;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_DRAM) {
			regCtrl.bit.MAIN_IN_SEL   = 5;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_2PPATGEN) { /*2 pixel pattern gen, Debug mode*/
			regCtrl.bit.MAIN_IN_SEL   = 7;
			regCtrl.bit.PARAPL_VD_SEL = 2/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_AHD) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 4;
		} else {
			nvt_dbg(ERR, "pMainInInfo->MainInSrc %d ERR !!\n", pMainInInfo->MainInSrc);
		}
		break;
	case SIE_ENGINE_ID_2:
		if (pMainInInfo->MainInSrc == MAIN_IN_PARA_MSTR_SNR) { // from SIE2 PAD
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 0;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_PARA_SLAV_SNR) { // from TGE VD2
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 1;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_SELF_PATGEN) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 2;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_CSI_1) {
			regCtrl.bit.MAIN_IN_SEL   = 1;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_CSI_2) {
			regCtrl.bit.MAIN_IN_SEL   = 2;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_DRAM) {
			regCtrl.bit.MAIN_IN_SEL   = 5;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_2PPATGEN) { /*2 pixel pattern gen, Debug mode*/
			regCtrl.bit.MAIN_IN_SEL   = 7;
			regCtrl.bit.PARAPL_VD_SEL = 2/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_AHD) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 4;
		} else {
			nvt_dbg(ERR, "id: %d, pMainInInfo->MainInSrc %d ERR !!\n", id, pMainInInfo->MainInSrc);
		}
		break;

	case SIE_ENGINE_ID_3:
		if (pMainInInfo->MainInSrc == MAIN_IN_PARA_MSTR_SNR) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 0;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_PARA_SLAV_SNR) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 1;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_SELF_PATGEN) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 2;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_DRAM) {
			regCtrl.bit.MAIN_IN_SEL   = 5;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_2PPATGEN) { /*2 pixel pattern gen, Debug mode*/
			regCtrl.bit.MAIN_IN_SEL   = 7;
			regCtrl.bit.PARAPL_VD_SEL = 2/*don't care*/;
		} else {
			nvt_dbg(ERR, "pMainInInfo->MainInSrc %d ERR !!\n", pMainInInfo->MainInSrc);
		}
		break;
	case SIE_ENGINE_ID_4:
		if (pMainInInfo->MainInSrc == MAIN_IN_PARA_MSTR_SNR) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 0;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_PARA_SLAV_SNR) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 1;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_SELF_PATGEN) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 2;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_CSI_1) {
			regCtrl.bit.MAIN_IN_SEL   = 1;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_CSI_2) {
			regCtrl.bit.MAIN_IN_SEL   = 2;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_DRAM) {
			regCtrl.bit.MAIN_IN_SEL   = 5;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_2PPATGEN) { /*2 pixel pattern gen, Debug mode*/
			regCtrl.bit.MAIN_IN_SEL   = 7;
			regCtrl.bit.PARAPL_VD_SEL = 2/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_AHD) {
			//nvt_dbg(ERR, "in MAIN_IN_AHD\r\n");
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 4;
		} else {
			nvt_dbg(ERR, "pMainInInfo->MainInSrc %d ERR !!\n", pMainInInfo->MainInSrc);
		}
		break;
	case SIE_ENGINE_ID_5:
		if (pMainInInfo->MainInSrc == MAIN_IN_PARA_MSTR_SNR) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 0;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_PARA_SLAV_SNR) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 1;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_SELF_PATGEN) {
			regCtrl.bit.MAIN_IN_SEL   = 0;
			regCtrl.bit.PARAPL_VD_SEL = 2;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_CSI_1) {
			regCtrl.bit.MAIN_IN_SEL   = 1;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_CSI_2) {
			regCtrl.bit.MAIN_IN_SEL   = 2;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_DRAM) {
			regCtrl.bit.MAIN_IN_SEL   = 5;
			regCtrl.bit.PARAPL_VD_SEL = 0/*don't care*/;
		} else if (pMainInInfo->MainInSrc == MAIN_IN_2PPATGEN) { /*2 pixel pattern gen, Debug mode*/
			regCtrl.bit.MAIN_IN_SEL   = 7;
			regCtrl.bit.PARAPL_VD_SEL = 2/*don't care*/;
		} else {
			nvt_dbg(ERR, "pMainInInfo->MainInSrc %d ERR !!\n", pMainInInfo->MainInSrc);
		}
		break;
	}

	regCtrl.bit.VD_PHASE = pMainInInfo->VdPhase;
	regCtrl.bit.HD_PHASE = pMainInInfo->HdPhase;
	regCtrl.bit.DATA_PHASE = pMainInInfo->DataPhase;
	regCtrl.bit.VD_INV = pMainInInfo->bVdInv;
	regCtrl.bit.HD_INV = pMainInInfo->bHdInv;
	regCtrl.bit.DIRECT_TO_RHE = pMainInInfo->bDirect2RHE;
	regCtrl.bit.SIE_RAW_SEL = pMainInInfo->bRawbitdepSel;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS, regCtrl.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getMainInput(UINT32 id, SIE_MAIN_INPUT_INFO *pMainInInfo)
{
	T_R0_ENGINE_CONTROL regCtrl;
	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS);

    switch (id) {
	case SIE_ENGINE_ID_1:
		if (regCtrl.bit.MAIN_IN_SEL == 0) {
			if (regCtrl.bit.PARAPL_VD_SEL == 0) {
				pMainInInfo->MainInSrc = MAIN_IN_PARA_MSTR_SNR;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 1) {
				pMainInInfo->MainInSrc = MAIN_IN_PARA_SLAV_SNR;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 2) {
				pMainInInfo->MainInSrc = MAIN_IN_SELF_PATGEN;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 4) {
				pMainInInfo->MainInSrc = MAIN_IN_AHD;
			}
		} else if (regCtrl.bit.MAIN_IN_SEL == 1) {
			pMainInInfo->MainInSrc = MAIN_IN_CSI_1;
		} else if (regCtrl.bit.MAIN_IN_SEL == 5) {
			pMainInInfo->MainInSrc = MAIN_IN_DRAM;
		} else if (regCtrl.bit.MAIN_IN_SEL == 7) {
			pMainInInfo->MainInSrc = MAIN_IN_2PPATGEN;
		}
		break;
	case SIE_ENGINE_ID_2:
		if (regCtrl.bit.MAIN_IN_SEL == 0) {
			if (regCtrl.bit.PARAPL_VD_SEL == 0) {
				pMainInInfo->MainInSrc = MAIN_IN_PARA_MSTR_SNR;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 1) {
				pMainInInfo->MainInSrc = MAIN_IN_PARA_SLAV_SNR;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 2) {
				pMainInInfo->MainInSrc = MAIN_IN_SELF_PATGEN;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 4) {
				pMainInInfo->MainInSrc = MAIN_IN_AHD;
			}
		} else if (regCtrl.bit.MAIN_IN_SEL == 1) {
			pMainInInfo->MainInSrc = MAIN_IN_CSI_1;
		} else if (regCtrl.bit.MAIN_IN_SEL == 2) {
			pMainInInfo->MainInSrc = MAIN_IN_CSI_2;
		} else if (regCtrl.bit.MAIN_IN_SEL == 5) {
			pMainInInfo->MainInSrc = MAIN_IN_DRAM;
		} else if (regCtrl.bit.MAIN_IN_SEL == 7) {
			pMainInInfo->MainInSrc = MAIN_IN_2PPATGEN;
		}
		break;
	case SIE_ENGINE_ID_3:
		if (regCtrl.bit.MAIN_IN_SEL == 0) {
			if (regCtrl.bit.PARAPL_VD_SEL == 0) {
				pMainInInfo->MainInSrc = MAIN_IN_PARA_MSTR_SNR;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 1) {
				pMainInInfo->MainInSrc = MAIN_IN_PARA_SLAV_SNR;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 2) {
				pMainInInfo->MainInSrc = MAIN_IN_SELF_PATGEN;
			} 
		} else if (regCtrl.bit.MAIN_IN_SEL == 5) {
			pMainInInfo->MainInSrc = MAIN_IN_DRAM;
		} else if (regCtrl.bit.MAIN_IN_SEL == 7) {
			pMainInInfo->MainInSrc = MAIN_IN_2PPATGEN;
		}
		break;
	case SIE_ENGINE_ID_4:
		if (regCtrl.bit.MAIN_IN_SEL == 0) {
			if (regCtrl.bit.PARAPL_VD_SEL == 0) {
				pMainInInfo->MainInSrc = MAIN_IN_PARA_MSTR_SNR;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 1) {
				pMainInInfo->MainInSrc = MAIN_IN_PARA_SLAV_SNR;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 2) {
				pMainInInfo->MainInSrc = MAIN_IN_SELF_PATGEN;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 4) {
				pMainInInfo->MainInSrc = MAIN_IN_AHD;
			}
		} else if (regCtrl.bit.MAIN_IN_SEL == 1) {
			pMainInInfo->MainInSrc = MAIN_IN_CSI_1;
		} else if (regCtrl.bit.MAIN_IN_SEL == 2) {
			pMainInInfo->MainInSrc = MAIN_IN_CSI_2;
		} else if (regCtrl.bit.MAIN_IN_SEL == 5) {
			pMainInInfo->MainInSrc = MAIN_IN_DRAM;
		} else if (regCtrl.bit.MAIN_IN_SEL == 7) {
			pMainInInfo->MainInSrc = MAIN_IN_2PPATGEN;
		}
		break;
	case SIE_ENGINE_ID_5:
		if (regCtrl.bit.MAIN_IN_SEL == 0) {
			if (regCtrl.bit.PARAPL_VD_SEL == 0) {
				pMainInInfo->MainInSrc = MAIN_IN_PARA_MSTR_SNR;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 1) {
				pMainInInfo->MainInSrc = MAIN_IN_PARA_SLAV_SNR;
			} else if (regCtrl.bit.PARAPL_VD_SEL == 2) {
				pMainInInfo->MainInSrc = MAIN_IN_SELF_PATGEN;
			} 
		} else if (regCtrl.bit.MAIN_IN_SEL == 1) {
			pMainInInfo->MainInSrc = MAIN_IN_CSI_1;
		} else if (regCtrl.bit.MAIN_IN_SEL == 2) {
			pMainInInfo->MainInSrc = MAIN_IN_CSI_2;
		} else if (regCtrl.bit.MAIN_IN_SEL == 5) {
			pMainInInfo->MainInSrc = MAIN_IN_DRAM;
		} else if (regCtrl.bit.MAIN_IN_SEL == 7) {
			pMainInInfo->MainInSrc = MAIN_IN_2PPATGEN;
		}
		break;
    }

	pMainInInfo->VdPhase = regCtrl.bit.VD_PHASE;
	pMainInInfo->HdPhase = regCtrl.bit.HD_PHASE;
	pMainInInfo->DataPhase = regCtrl.bit.DATA_PHASE;
	pMainInInfo->bVdInv = regCtrl.bit.VD_INV;
	pMainInInfo->bHdInv = regCtrl.bit.HD_INV;
	pMainInInfo->bDirect2RHE = regCtrl.bit.DIRECT_TO_RHE;
}

void sie_setDramInStart(UINT32 id)
{
	T_R0_ENGINE_CONTROL regCtrl;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS);
	regCtrl.bit.DRAM_IN_START = 1;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS, regCtrl.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

BOOL sie_getDramInStart(UINT32 id)
{
	T_R0_ENGINE_CONTROL regCtrl;
	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS);
	return (BOOL)regCtrl.bit.DRAM_IN_START;
}

void sie_setRwOBP(UINT32 id, BOOL bEnable)
{
	T_R0_ENGINE_CONTROL regCtrl;
	unsigned long spin_flags;
	spin_flags=sie_platform_spin_lock(0);

	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS);
	regCtrl.bit.RW_OBP = bEnable;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS, regCtrl.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

ER sie_verifyFunction(UINT32 uiFunction)
{
	ER erReturn = E_OK;

	if ((uiFunction & SIE_BS_V_EN) != 0 && (uiFunction & SIE_BS_H_EN) == 0) {
		nvt_dbg(ERR,"BS_H should be enabled along with BS_V\r\n");
		erReturn = E_PAR;
	}
	return erReturn;
}

void sie_enableFunction(UINT32 id, BOOL bEnable, UINT32 uiFunction)
{
	T_R4_ENGINE_FUNCTION regEn;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regEn.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS);
	if (bEnable) {
		regEn.reg |= uiFunction;
	} else {
		regEn.reg &= (~uiFunction);
	}
	sie_verifyFunction(regEn.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS, regEn.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setFunction(UINT32 id, UINT32 uiFunction)
{
	T_R4_ENGINE_FUNCTION regEn;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	//regEn.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS);
	regEn.reg = uiFunction;
	sie_verifyFunction(regEn.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS, regEn.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

UINT32 sie_getFunction(UINT32 id)
{
	T_R4_ENGINE_FUNCTION regEn;

	regEn.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS);
	return regEn.reg;
}

void sie_setSramPwrSave(UINT32 id, BOOL bBsAuto, BOOL bVaAuto)
{
	T_R0_ENGINE_CONTROL regCtrl;
	unsigned long spin_flags;
	spin_flags=sie_platform_spin_lock(0);
	regCtrl.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS);
	if (bBsAuto)     {
		regCtrl.bit.SRAM_PWR_SAVE_BS = 1;
	} else            {
		regCtrl.bit.SRAM_PWR_SAVE_BS = 0;
	}
	if (bVaAuto)     {
		regCtrl.bit.SRAM_PWR_SAVE_VA = 1;
	} else            {
		regCtrl.bit.SRAM_PWR_SAVE_VA = 0;
	}
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R0_ENGINE_CONTROL_OFS, regCtrl.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_enableIntEnable(UINT32 id, BOOL bEnable, UINT32 uiIntrp)
{
	T_R8_ENGINE_INTERRUPT regInte;
	regInte.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R8_ENGINE_INTERRUPT_OFS);
	if (bEnable) {
		regInte.reg |= uiIntrp;
	} else {
		regInte.reg &= (~uiIntrp);
	}
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R8_ENGINE_INTERRUPT_OFS, regInte.reg);
}

void sie_setIntEnable(UINT32 id, UINT32 uiIntrp)
{
	T_R8_ENGINE_INTERRUPT regInte;
	unsigned long spin_flags;
	T_R20_ENGINE_TIMING regBP_A;
	UINT32 bp1 = 0, bp2 = 0;

	spin_flags=sie_platform_spin_lock(0);

	regBP_A.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R20_ENGINE_TIMING_OFS);
	sie_lib_get_bp(id, &bp1, &bp2);
	if ((bp2 > bp1) && (bp1 > 0)) {
		regBP_A.bit.BP1 = bp1;
		regBP_A.bit.BP2 = bp2;
		regInte.reg = uiIntrp | SIE_INT_CROPEND|SIE_INT_BP1|SIE_INT_BP2;
	} else {
		regInte.reg = uiIntrp | SIE_INT_CROPEND;
	}
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R8_ENGINE_INTERRUPT_OFS, regInte.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R20_ENGINE_TIMING_OFS, regBP_A.reg);

	sie_platform_spin_unlock(0,spin_flags);
}

UINT32 sie_getIntEnable(UINT32 id)
{
	T_R8_ENGINE_INTERRUPT regInte;

	regInte.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R8_ENGINE_INTERRUPT_OFS);
	return regInte.reg;
}

UINT32 sie_getIntrStatus(UINT32 id)
{
	T_RC_ENGINE_INTERRUPT regInt;

	regInt.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RC_ENGINE_INTERRUPT_OFS);
	return regInt.reg;
}

void sie_clrIntrStatus(UINT32 id, UINT32 uiIntrpStatus)
{
	T_RC_ENGINE_INTERRUPT regInt;

	regInt.reg = uiIntrpStatus;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RC_ENGINE_INTERRUPT_OFS, regInt.reg);
}


void sie_getEngineStatus(UINT32 id, SIE_ENGINE_STATUS_INFO *pEngineStatus)
{
	T_R10_ENGINE_STATUS regInt;

	regInt.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R10_ENGINE_STATUS_OFS);

	pEngineStatus->uiLineCnt   = regInt.bit.LN16_CNT;
	pEngineStatus->bBuffRheOvfl = regInt.bit.BUFF_RHE_OVFL;
	pEngineStatus->bDramIn1Udfl = regInt.bit.DRAM_IN1_UDFL;
	pEngineStatus->bDramIn2Udfl = regInt.bit.DRAM_IN2_UDFL;
	pEngineStatus->bDramOut0Ovfl = regInt.bit.DRAM_OUT0_OVFL;
	pEngineStatus->bDramOut1Ovfl = regInt.bit.DRAM_OUT1_OVFL;
	pEngineStatus->bDramOut2Ovfl = regInt.bit.DRAM_OUT2_OVFL;
	pEngineStatus->bDramOut3Ovfl = regInt.bit.DRAM_OUT3_OVFL;
	pEngineStatus->bDramOut4Ovfl = regInt.bit.DRAM_OUT4_OVFL;
	pEngineStatus->bDramOut5Ovfl = regInt.bit.DRAM_OUT5_OVFL;
}

void sie_clrEngineStatus(UINT32 id, SIE_ENGINE_STATUS_INFO *pEngineStatus)
{
	T_R10_ENGINE_STATUS regInt;

	regInt.reg = 0;
	regInt.bit.BUFF_RHE_OVFL = pEngineStatus->bBuffRheOvfl;
	regInt.bit.DRAM_IN1_UDFL = pEngineStatus->bDramIn1Udfl;
	regInt.bit.DRAM_IN2_UDFL = pEngineStatus->bDramIn2Udfl;
	regInt.bit.DRAM_OUT0_OVFL = pEngineStatus->bDramOut0Ovfl;
	regInt.bit.DRAM_OUT1_OVFL = pEngineStatus->bDramOut1Ovfl;
	regInt.bit.DRAM_OUT2_OVFL = pEngineStatus->bDramOut2Ovfl;
	regInt.bit.DRAM_OUT3_OVFL = pEngineStatus->bDramOut3Ovfl;
	regInt.bit.DRAM_OUT4_OVFL = pEngineStatus->bDramOut4Ovfl;
	regInt.bit.DRAM_OUT5_OVFL = pEngineStatus->bDramOut5Ovfl;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R10_ENGINE_STATUS_OFS, regInt.reg);
}

void sie_setSourceWindow(UINT32 id, SIE_SRC_WIN_INFO *pSrcWin)
{
	T_R1C_ENGINE_TIMING regTiming;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regTiming.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1C_ENGINE_TIMING_OFS);//no need//
	regTiming.bit.SRC_WIDTH   = pSrcWin->uiSzX;
	regTiming.bit.SRC_HEIGHT  = pSrcWin->uiSzY;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1C_ENGINE_TIMING_OFS, regTiming.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getSourceWindow(UINT32 id, SIE_SRC_WIN_INFO *pSrcWin)
{
	T_R1C_ENGINE_TIMING regTiming;
	regTiming.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1C_ENGINE_TIMING_OFS);
	pSrcWin->uiSzX = regTiming.bit.SRC_WIDTH;
	pSrcWin->uiSzY = regTiming.bit.SRC_HEIGHT;
}

void sie_setBreakPoint(UINT32 id, SIE_BREAKPOINT_INFO *pBP)
{
	T_R20_ENGINE_TIMING regBP_A;
	T_R24_ENGINE_TIMING regBP_B;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regBP_A.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R20_ENGINE_TIMING_OFS);
	regBP_B.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R24_ENGINE_TIMING_OFS);//no need//
	regBP_A.bit.BP1   = pBP->uiBp1;
	regBP_A.bit.BP2   = pBP->uiBp2;
	regBP_B.bit.BP3   = pBP->uiBp3;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R20_ENGINE_TIMING_OFS, regBP_A.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R24_ENGINE_TIMING_OFS, regBP_B.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setBreakPoint1(UINT32 id, SIE_BREAKPOINT_INFO *pBP)
{
	T_R20_ENGINE_TIMING regBP_A;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
    regBP_A.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R20_ENGINE_TIMING_OFS);
	regBP_A.bit.BP1   = pBP->uiBp1;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R20_ENGINE_TIMING_OFS, regBP_A.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setBreakPoint2(UINT32 id, SIE_BREAKPOINT_INFO *pBP)
{
	T_R20_ENGINE_TIMING regBP_A;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regBP_A.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R20_ENGINE_TIMING_OFS);
	regBP_A.bit.BP2   = pBP->uiBp2;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R20_ENGINE_TIMING_OFS, regBP_A.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setBreakPoint3(UINT32 id, SIE_BREAKPOINT_INFO *pBP)
{
	T_R24_ENGINE_TIMING regBP_B;
	UINT32 spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regBP_B.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R24_ENGINE_TIMING_OFS);//no need//
	regBP_B.bit.BP3   = pBP->uiBp3;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R24_ENGINE_TIMING_OFS, regBP_B.reg);
	sie_platform_spin_unlock(0,spin_flags);
}


void sie_setActiveWindow(UINT32 id, SIE_ACT_WIN_INFO *pActWin)
{
	T_R28_ENGINE_TIMING reg28;
	T_R2C_ENGINE_TIMING reg2C;
	T_R30_ENGINE_CFA_PATTERN reg30;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	if (pActWin->uiSzX & 0x1)    {
		nvt_dbg(ERR,"SzX = %d, not 2*N\r\n", (int)pActWin->uiSzX);
		pActWin->uiSzX &= 0xfffffffe;
	}
	//no need//if(pActWin->uiSzY&0x1)    {        DBG_ERR("SzY = %d, not 2*N\r\n", pActWin->uiSzY);    pActWin->uiSzY &= 0xfffffffe;    }
	reg28.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R28_ENGINE_TIMING_OFS);//no need//
	reg2C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2C_ENGINE_TIMING_OFS);//no need//
	reg30.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS);
	reg28.bit.ACT_STX       = pActWin->uiStX;
	reg30.bit.ACT_CFAPAT    = pActWin->CfaPat;
	reg28.bit.ACT_STY       = pActWin->uiStY;
	reg2C.bit.ACT_SZX       = pActWin->uiSzX;
	reg2C.bit.ACT_SZY       = pActWin->uiSzY;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R28_ENGINE_TIMING_OFS, reg28.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R2C_ENGINE_TIMING_OFS, reg2C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS, reg30.reg);
	sie_platform_spin_unlock(0,spin_flags);
}


void sie_getActiveWindow(UINT32 id, SIE_ACT_WIN_INFO *pActWin)
{
	T_R28_ENGINE_TIMING reg28;
	T_R2C_ENGINE_TIMING reg2C;
	T_R30_ENGINE_CFA_PATTERN reg30;

	reg28.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R28_ENGINE_TIMING_OFS);
	reg2C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2C_ENGINE_TIMING_OFS);
	reg30.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS);
	pActWin->uiStX  = reg28.bit.ACT_STX ;
	pActWin->CfaPat = reg30.bit.ACT_CFAPAT;
	pActWin->uiStY  = reg28.bit.ACT_STY;
	pActWin->uiSzX  = reg2C.bit.ACT_SZX;
	pActWin->uiSzY  = reg2C.bit.ACT_SZY;
}


void sie_setCropWindow(UINT32 id, SIE_CRP_WIN_INFO *pCrpWin)
{
	T_R30_ENGINE_CFA_PATTERN reg30;
	T_R34_ENGINE_TIMING reg34;
	T_R38_ENGINE_TIMING reg38;
	//SIE_ACT_WIN_INFO ActWin = {0};
	//SIE_DVI_INFO DviParam = {0};
	UINT32 uiStX;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	if (pCrpWin->uiSzX & 0x1)    {
		nvt_dbg(ERR,"SzX = %d, not 2*N\r\n", (int)pCrpWin->uiSzX);
		pCrpWin->uiSzX &= 0xfffffffe;
	}

	// check crop function is avaiable?
	uiStX = pCrpWin->uiStX;
	#if 0
	if (sie_getFunction(id) & SIE_DVI_EN){
		sie_getDVI(id, &DviParam);
		if (DviParam.DviFormat == DVI_FORMAT_CCIR656_EAV){
			sie_getActiveWindow(id, &ActWin);
			if ((ActWin.uiSzX == pCrpWin->uiSzX) && (ActWin.uiSzY == pCrpWin->uiSzY) && (pCrpWin->uiStX==0) && (pCrpWin->uiStY==0)){
				// No Crop, do nothing
			}else{
				if ((ActWin.uiSzX == pCrpWin->uiSzX) && (pCrpWin->uiStX==0)){
					// Crop Y only, Crop function fail !!
					//DBG_ERR("CCIR56 EAV mode fail: Crop Y only!!!\r\n");
					nvt_dbg(ERR,"CCIR56 EAV mode fail: Crop Y only!!!\r\n");
				}else{
					// Crop X, check Stx
					if (pCrpWin->uiStX >= 8) {
						uiStX = pCrpWin->uiStX - 4;
					} else
						//DBG_ERR("CCIR656 EAV mode fail: Crop StX < 8 !!\r\n");
					{
						nvt_dbg(ERR, "CCIR656 EAV mode fail: Crop StX < 8 !!\r\n");
					}
				}
			}
		}
	}
	#endif


	//no need//if(pCrpWin->uiSzY&0x1)    {        DBG_ERR("SzY = %d, not 2*N\r\n", pCrpWin->uiSzY);    pCrpWin->uiSzY &= 0xfffffffe;    }
	reg34.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R34_ENGINE_TIMING_OFS);//no need//
	reg38.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R38_ENGINE_TIMING_OFS);//no need//
	reg30.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS);
	reg34.bit.CRP_STX       = uiStX;//pCrpWin->uiStX;
	reg30.bit.CROP_CFAPAT    = pCrpWin->CfaPat;
	reg34.bit.CRP_STY       = pCrpWin->uiStY;
	reg38.bit.CRP_SZX       = pCrpWin->uiSzX;
	reg38.bit.CRP_SZY       = pCrpWin->uiSzY;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS, reg30.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R34_ENGINE_TIMING_OFS, reg34.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R38_ENGINE_TIMING_OFS, reg38.reg);

	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getCropWindow(UINT32 id, SIE_CRP_WIN_INFO *pCrpWin)
{
	T_R30_ENGINE_CFA_PATTERN reg30;
	T_R34_ENGINE_TIMING reg34;
	T_R38_ENGINE_TIMING reg38;

	reg34.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R34_ENGINE_TIMING_OFS);
	reg38.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R38_ENGINE_TIMING_OFS);
	reg30.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS);
	pCrpWin->uiStX = reg34.bit.CRP_STX ;
	pCrpWin->CfaPat = reg30.bit.CROP_CFAPAT;
	pCrpWin->uiStY = reg34.bit.CRP_STY;
	pCrpWin->uiSzX = reg38.bit.CRP_SZX;
	pCrpWin->uiSzY = reg38.bit.CRP_SZY;
}

void sie_getActBlankingInfo(UINT32 id, SIE_PIX_RATE_INFO *pPixRateInfo)
{
	T_R3C_ENGINE_STATUS reg3c;
	reg3c.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R3C_ENGINE_STATUS_OFS);
	pPixRateInfo->uiPixRateCnt = reg3c.bit.PIX_PATE_CNT;
	pPixRateInfo->uiPixRateMax = reg3c.bit.PIX_PATE_MAX;
}
void sie_setDramIn0(UINT32 id, SIE_DRAM_IN0_INFO *pDIn0)
{
	T_R48_ENGINE_DRAM reg48;
	T_R4C_ENGINE_DRAM reg4C;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg48.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R48_ENGINE_DRAM_OFS);//no need//
	reg4C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4C_ENGINE_DRAM_OFS);//no need//
	reg48.bit.DRAM_IN1_SAI      = pDIn0->uiAddr >> 2;
	reg4C.bit.DRAM_IN1_PACK_BUS = pDIn0->PackBus;
	reg4C.bit.DRAM_IN1_OFSO     = pDIn0->uiLofs >> 2;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R48_ENGINE_DRAM_OFS, reg48.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R4C_ENGINE_DRAM_OFS, reg4C.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getDramIn0(UINT32 id, SIE_DRAM_IN0_INFO *pDIn0)
{
	T_R48_ENGINE_DRAM reg48;
	T_R4C_ENGINE_DRAM reg4C;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg48.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R48_ENGINE_DRAM_OFS);
	reg4C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4C_ENGINE_DRAM_OFS);
	pDIn0->uiAddr   = dma_getNonCacheAddr(reg48.bit.DRAM_IN1_SAI << 2);
	pDIn0->PackBus  = reg4C.bit.DRAM_IN1_PACK_BUS;
	pDIn0->uiLofs   = reg4C.bit.DRAM_IN1_OFSO << 2;
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setDramIn1(UINT32 id, SIE_DRAM_IN1_INFO *pDIn1)
{
	T_R48_ENGINE_DRAM reg48;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg48.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R48_ENGINE_DRAM_OFS);//no need//
    if (pDIn1->uiAddr != 0) {
		sie_platform_dma_flush_mem2dev(pDIn1->uiAddr,pDIn1->uiSize);
		reg48.bit.DRAM_IN1_SAI      = sie_platform_va2pa(pDIn1->uiAddr) >> 2;//pDIn1->uiAddr >> 2
    }
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R48_ENGINE_DRAM_OFS, reg48.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setDramIn2(UINT32 id, SIE_DRAM_IN2_INFO *pDIn2)
{
	T_R50_ENGINE_DRAM reg50;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg50.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R50_ENGINE_DRAM_OFS);//no need//
	if (pDIn2->uiAddr != 0)
	{
		#if 0
		fmem_dcache_sync((void *)pDIn2->uiAddr, ECS_MAX_MAP_SZ*ECS_MAX_MAP_SZ*4, DMA_BIDIRECTIONAL);
		#else
		sie_platform_dma_flush_mem2dev(pDIn2->uiAddr,ECS_MAX_MAP_BUF_SZ);
		#endif
		reg50.bit.DRAM_IN2_SAI      = sie_platform_va2pa(pDIn2->uiAddr) >> 2;//pDIn2->uiAddr >> 2;
	}
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R50_ENGINE_DRAM_OFS, reg50.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setDramSingleOut(UINT32 id, SIE_DRAM_SINGLE_OUT *pDOut)
{
	T_RA4_ENGINE_DRAM regA4;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regA4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RA4_ENGINE_DRAM_OFS);
    regA4.bit.DRAM_OUT0_SINGLE_EN = pDOut->SingleOut0En;
	regA4.bit.DRAM_OUT1_SINGLE_EN = pDOut->SingleOut1En;
	regA4.bit.DRAM_OUT2_SINGLE_EN = pDOut->SingleOut2En;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RA4_ENGINE_DRAM_OFS, regA4.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getDramSingleOut(UINT32 id, SIE_DRAM_SINGLE_OUT *pDOut)
{
	T_RA4_ENGINE_DRAM regA4;

	regA4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RA4_ENGINE_DRAM_OFS);
    pDOut->SingleOut0En = regA4.bit.DRAM_OUT0_SINGLE_EN;
	pDOut->SingleOut1En = regA4.bit.DRAM_OUT1_SINGLE_EN;
	pDOut->SingleOut2En = regA4.bit.DRAM_OUT2_SINGLE_EN;
}

void sie_getSysDbgInfo(UINT32 id, SIE_SYS_DEBUG_INFO *pDbugInfo)
{ 
    T_R10_ENGINE_STATUS reg10;
    T_R18_ENGINE_STATUS reg18;
	T_R160_DEBUG reg160;
	T_R180_DEBUG reg180;
	T_R184_DEBUG reg184;
	T_R180_DEBUG reg190;
	T_R184_DEBUG reg194;

    reg10.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R10_ENGINE_STATUS_OFS);
	reg18.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R18_ENGINE_STATUS_OFS);
	reg160.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R160_DEBUG_OFS);
    reg180.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R180_DEBUG_OFS);
	reg184.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R184_DEBUG_OFS);
	reg190.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R190_DEBUG_OFS);
	reg194.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R194_DEBUG_OFS);
	pDbugInfo->pxclkIn = reg18.bit.PXCLK_IN;
	pDbugInfo->sieclkIn = reg18.bit.SIECLK_IN;

	if (nvt_get_chip_id() == CHIP_NA51055) { //520
		pDbugInfo->uiLineCnt = (reg10.bit.LN16_CNT<<4)+1;
		pDbugInfo->uiCsiHdCnt = reg180.bit.CSI_HD_CNT;
		pDbugInfo->uiCsiLineEdCnt = reg180.bit.CSI_LN_END_CNT;
		pDbugInfo->uiCsiPixCnt = reg184.bit.CSI_PXL_CNT<<1;
		pDbugInfo->uiMaxPixelCnt = reg10.bit.PX_CNT;
	} else { //528
		pDbugInfo->uiLineCnt = (reg160.bit.LN_CNT_MAX+1);
		pDbugInfo->uiCsiHdCnt = reg190.bit.CSI_HD_CNT;
		pDbugInfo->uiCsiLineEdCnt = reg190.bit.CSI_LN_END_CNT;
		pDbugInfo->uiCsiPixCnt = reg194.bit.CSI_PXL_CNT<<1;
		pDbugInfo->uiMaxPixelCnt = reg10.reg&0x1fff;
	}
	//nvt_dbg(ERR,"%d/%d %d/%d/%d/%d\r\n", pDbugInfo->pxclkIn,pDbugInfo->sieclkIn,pDbugInfo->uiLineCnt,pDbugInfo->uiCsiHdCnt,pDbugInfo->uiCsiLineEdCnt,pDbugInfo->uiCsiPixCnt);
}

void sie_setChecksumEn(UINT32 id, UINT32 uiEn)
{
	T_R18_ENGINE_STATUS reg18;
	unsigned long spin_flags;
	
	spin_flags=sie_platform_spin_lock(0);
    reg18.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R18_ENGINE_STATUS_OFS);
	reg18.bit.CHECKSUM_EN = uiEn;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R18_ENGINE_STATUS_OFS, reg18.reg);
	sie_platform_spin_unlock(0,spin_flags);
}


void sie_setDramOut0(UINT32 id, SIE_DRAM_OUT0_INFO *pDOut0)
{
	T_R38_ENGINE_TIMING reg38;
	T_R5C_ENGINE_DRAM   reg5C;
	T_R60_ENGINE_DRAM   reg60;
	T_R64_ENGINE_DRAM   reg64;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
    reg38.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R38_ENGINE_TIMING_OFS);//no need//
	reg5C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R5C_ENGINE_DRAM_OFS);//no need//
	reg60.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R60_ENGINE_DRAM_OFS);//no need//
	reg64.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R64_ENGINE_DRAM_OFS);//no need//


	//reg60.bit.DRAM_OUT0_SOURCE          = pDOut0->OutSrc;
	reg60.bit.DRAM_OUT0_OFSO            = pDOut0->uiLofs >> 2;
	reg60.bit.DRAM_OUT0_PACK_BUS        = pDOut0->PackBus;
//#if (SIE_INT_FLIP_TEST==1)
#if 1 //(defined(_NVT_EMULATION_))
	reg60.bit.DRAM_OUT0_HFLIP           = pDOut0->bHFlip;
	reg60.bit.DRAM_OUT0_VFLIP           = pDOut0->bVFlip;
#else
	if (sie_getFunction(id)&SIE_DVI_EN) {
		reg60.bit.DRAM_OUT0_HFLIP           = pDOut0->bHFlip;
		reg60.bit.DRAM_OUT0_VFLIP           = pDOut0->bVFlip;
	} else {
		reg60.bit.DRAM_OUT0_HFLIP           = 0;
		reg60.bit.DRAM_OUT0_VFLIP           = 0;
	}
#endif
	reg60.bit.DRAM_OUT0_RINGBUF_EN         = pDOut0->bRingBufEn;
	reg64.bit.DRAM_OUT0_RINGBUF_LEN         = pDOut0->uiRingBufLen;

	if (pDOut0->uiAddr != 0) {
		sie_platform_dma_flush_dev2mem(pDOut0->uiAddr,pDOut0->uiLofs*reg38.bit.CRP_SZY, 1);
		g_uiDramOut0VirAddr[id]             = pDOut0->uiAddr;
		reg5C.bit.DRAM_OUT0_SAO         = sie_platform_va2pa(pDOut0->uiAddr) >> 2;//virt_to_phys(pDOut0->uiAddr) >> 2;
    }

//	nvt_dbg(INFO,"0x%08x/0x%08x/0x%08x\r\n",pDOut0->uiAddr, frm_va2pa(pDOut0->uiAddr), reg5C.bit.DRAM_OUT0_SAO);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R5C_ENGINE_DRAM_OFS, reg5C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R60_ENGINE_DRAM_OFS, reg60.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R64_ENGINE_DRAM_OFS, reg64.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setDramOut0Flip(UINT32 id, SIE_DRAM_OUT0_INFO *pDOut0)
{
	UINT32 uiOut0Addr0;
	//T_R4_ENGINE_FUNCTION reg04;
	T_R38_ENGINE_TIMING  reg38;
	T_R5C_ENGINE_DRAM    reg5C;
	T_R60_ENGINE_DRAM    reg60;
	T_R64_ENGINE_DRAM    reg64;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	//reg04.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS);//no need//
	reg38.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R38_ENGINE_TIMING_OFS);//no need//
	reg5C.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R5C_ENGINE_DRAM_OFS);//no need//
	reg60.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R60_ENGINE_DRAM_OFS);//no need//
	reg64.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R64_ENGINE_DRAM_OFS);//no need//


	//reg60.bit.DRAM_OUT0_SOURCE          = pDOut0->OutSrc;
	reg60.bit.DRAM_OUT0_OFSO            = pDOut0->uiLofs >> 2;
	reg60.bit.DRAM_OUT0_PACK_BUS        = pDOut0->PackBus;

//#if (SIE_INT_FLIP_TEST==1)
#if 1// (defined(_NVT_EMULATION_))
	reg60.bit.DRAM_OUT0_HFLIP           = pDOut0->bHFlip;
	reg60.bit.DRAM_OUT0_VFLIP           = pDOut0->bVFlip;
#else
	if (reg04.bit.DVI_EN == 1) {
		reg60.bit.DRAM_OUT0_HFLIP           = pDOut0->bHFlip;
		reg60.bit.DRAM_OUT0_VFLIP           = pDOut0->bVFlip;
	} else {
		reg60.bit.DRAM_OUT0_HFLIP           = 0;
		reg60.bit.DRAM_OUT0_VFLIP           = 0;
	}
#endif

	uiOut0Addr0                          = pDOut0->uiAddr;

	//DBG_ERR("0: HFlip = %d,VFlip = %d, srcAddr0 =0x%08x, srcAddr1 = 0x%08x, lineoff = %d\r\n",pDOut0->bHFlip,pDOut0->bVFlip, uiOut0Addr0, uiOut0Addr1, pDOut0->uiLofs);
	//if (reg04.bit.DVI_EN == 1) {
		if (pDOut0->bHFlip) {
			uiOut0Addr0                  = uiOut0Addr0 + pDOut0->uiLofs;
		}

		if (pDOut0->bVFlip) {
			uiOut0Addr0                  = uiOut0Addr0 + (reg38.bit.CRP_SZY - 1) * pDOut0->uiLofs;
		}
	//}

	if (pDOut0->uiAddr != 0) {
		sie_platform_dma_flush_dev2mem(pDOut0->uiAddr,pDOut0->uiLofs*reg38.bit.CRP_SZY,1);
		g_uiDramOut0VirAddr[id]                  = uiOut0Addr0;
		reg5C.bit.DRAM_OUT0_SAO              = sie_platform_va2pa(uiOut0Addr0) >> 2;
    }

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R5C_ENGINE_DRAM_OFS, reg5C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R60_ENGINE_DRAM_OFS, reg60.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R64_ENGINE_DRAM_OFS, reg64.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setDramOut1(UINT32 id, SIE_DRAM_OUT1_INFO *pDOut1)
{
    T_R4_ENGINE_FUNCTION reg04;
	T_R38_ENGINE_TIMING reg38;
	T_R68_ENGINE_DRAM reg68;
	T_R6C_ENGINE_DRAM reg6C;
	T_R70_ENGINE_DRAM reg70;
	T_RAC_BASIC_DVI regDVI;
	T_R240_STCS_CA    reg240;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
    reg04.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS);//no need//
    reg38.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R38_ENGINE_TIMING_OFS);//no need//
	reg68.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R68_ENGINE_DRAM_OFS);//no need//
	reg6C.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R6C_ENGINE_DRAM_OFS);//no need//
	reg70.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R70_ENGINE_DRAM_OFS);//no need//
	regDVI.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RAC_BASIC_DVI_OFS);//no need//
	reg240.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R240_STCS_CA_OFS);

	if (pDOut1->uiAddr != 0) {
		if (reg04.bit.DVI_EN == 1) {
			if (regDVI.bit.OUT_YUV420 == 1)
				sie_platform_dma_flush_dev2mem(pDOut1->uiAddr,pDOut1->uiLofs*(reg38.bit.CRP_SZY>>1),1);
			else
				sie_platform_dma_flush_dev2mem(pDOut1->uiAddr,pDOut1->uiLofs*reg38.bit.CRP_SZY,1);
		}else
		{
			sie_platform_dma_flush_dev2mem(pDOut1->uiAddr, (((reg240.bit.CA_WIN_NUMX+1)*16*5+31)/32)*4*(reg240.bit.CA_WIN_NUMY+1), 0);
		}
		g_uiDramOut1VirAddr[id]             = pDOut1->uiAddr;
		reg68.bit.DRAM_OUT1_SAO             = sie_platform_va2pa(pDOut1->uiAddr) >> 2;
	}
	reg6C.bit.DRAM_OUT1_OFSO            = pDOut1->uiLofs >> 2;
//#if (SIE_INT_FLIP_TEST==1)
#if 1 //(defined(_NVT_EMULATION_))
	reg6C.bit.DRAM_OUT1_HFLIP           = pDOut1->bHFlip;
	reg6C.bit.DRAM_OUT1_VFLIP           = pDOut1->bVFlip;
#else
	if (sie_getFunction(id)&SIE_DVI_EN) {
		reg6C.bit.DRAM_OUT1_HFLIP           = pDOut1->bHFlip;
		reg6C.bit.DRAM_OUT1_VFLIP           = pDOut1->bVFlip;
	} else {
		reg6C.bit.DRAM_OUT1_HFLIP           = 0;
		reg6C.bit.DRAM_OUT1_VFLIP           = 0;
	}
#endif

	//nvt_dbg(INFO,"0x%08x/0x%08x/0x%08x\r\n",pDOut1->uiAddr,frm_va2pa(pDOut1->uiAddr), reg68.bit.DRAM_OUT1_SAO);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R68_ENGINE_DRAM_OFS, reg68.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R6C_ENGINE_DRAM_OFS, reg6C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R70_ENGINE_DRAM_OFS, reg70.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setDramOut1Flip(UINT32 id, SIE_DRAM_OUT1_INFO *pDOut1)
{
	UINT32 uiOut1Addr0;
	T_R4_ENGINE_FUNCTION reg04;
	T_R38_ENGINE_TIMING  reg38;
	T_R68_ENGINE_DRAM    reg68;
	T_R6C_ENGINE_DRAM    reg6C;
	T_R70_ENGINE_DRAM    reg70;
	//T_R1E8_STCS          reg1E8;
	T_R240_STCS_CA       reg240;
	T_RAC_BASIC_DVI regDVI;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg04.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS);//no need//
	reg38.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R38_ENGINE_TIMING_OFS);//no need//
	reg68.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R68_ENGINE_DRAM_OFS);//no need//
	reg6C.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R6C_ENGINE_DRAM_OFS);//no need//
	reg70.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R70_ENGINE_DRAM_OFS);//no need//
	//reg1E8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1E8_STCS_OFS);//no need//
	reg240.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R240_STCS_CA_OFS);
	regDVI.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RAC_BASIC_DVI_OFS);//no need//



	reg6C.bit.DRAM_OUT1_OFSO            = pDOut1->uiLofs >> 2;
//#if (SIE_INT_FLIP_TEST==1)
#if 1 //(defined(_NVT_EMULATION_))
	reg6C.bit.DRAM_OUT1_HFLIP           = pDOut1->bHFlip;
	reg6C.bit.DRAM_OUT1_VFLIP           = pDOut1->bVFlip;
#else
	if (reg04.bit.DVI_EN) {
		reg6C.bit.DRAM_OUT1_HFLIP           = pDOut1->bHFlip;
		reg6C.bit.DRAM_OUT1_VFLIP           = pDOut1->bVFlip;
	} else {
		reg6C.bit.DRAM_OUT1_HFLIP           = 0;
		reg6C.bit.DRAM_OUT1_VFLIP           = 0;
	}
#endif


	uiOut1Addr0                         = pDOut1->uiAddr;

	//if (reg04.bit.DVI_EN == 1) {
		if (pDOut1->bHFlip) {
			uiOut1Addr0                 = uiOut1Addr0 + pDOut1->uiLofs;
		}

		if (pDOut1->bVFlip) {
			if (regDVI.bit.OUT_YUV420 == 1)
				uiOut1Addr0                 = uiOut1Addr0 + (reg38.bit.CRP_SZY/2 - 1) * pDOut1->uiLofs;
			else
				uiOut1Addr0                 = uiOut1Addr0 + (reg38.bit.CRP_SZY - 1) * pDOut1->uiLofs;
		}
	//}

    if (pDOut1->uiAddr != 0) {
		if (reg04.bit.DVI_EN == 1) {
			if (regDVI.bit.OUT_YUV420 == 1)
				sie_platform_dma_flush_dev2mem(pDOut1->uiAddr,pDOut1->uiLofs*(reg38.bit.CRP_SZY>>1),1);
			else
				sie_platform_dma_flush_dev2mem(pDOut1->uiAddr,pDOut1->uiLofs*reg38.bit.CRP_SZY,1);
		}else
		{
			sie_platform_dma_flush_dev2mem(pDOut1->uiAddr, (((reg240.bit.CA_WIN_NUMX+1)*16*5+31)/32)*4*(reg240.bit.CA_WIN_NUMY+1), 0);
		}
		
		g_uiDramOut1VirAddr[id]             = uiOut1Addr0;
		reg68.bit.DRAM_OUT1_SAO            = sie_platform_va2pa(uiOut1Addr0) >> 2;
    }

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R68_ENGINE_DRAM_OFS, reg68.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R6C_ENGINE_DRAM_OFS, reg6C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R70_ENGINE_DRAM_OFS, reg70.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_setDramOut2(UINT32 id, SIE_DRAM_OUT2_INFO *pDOut2)
{
	T_R74_ENGINE_DRAM reg74;
	T_R78_ENGINE_DRAM reg78;
	T_R7C_ENGINE_DRAM reg7C;
	T_R2B0_STCS_LA    reg2B0;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg74.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R74_ENGINE_DRAM_OFS);//no need//
	reg78.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R78_ENGINE_DRAM_OFS);//no need//
	reg7C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R7C_ENGINE_DRAM_OFS);//no need//
	reg2B0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2B0_STCS_LA_OFS);

	if (pDOut2->uiAddr != 0) {
		sie_platform_dma_flush_dev2mem(pDOut2->uiAddr,(((reg2B0.bit.LA_WIN_NUMX+1)*16*2+31)/32)*4*(reg2B0.bit.LA_WIN_NUMY+1),0);
		g_uiDramOut2VirAddr[id]             = pDOut2->uiAddr;
		reg74.bit.DRAM_OUT2_SAO             = sie_platform_va2pa(pDOut2->uiAddr) >> 2;
	}
	reg78.bit.DRAM_OUT2_OFSO            = pDOut2->uiLofs >> 2;

	//nvt_dbg(INFO,"0x%08x/0x%08x/0x%08x\r\n",pDOut2->uiAddr,frm_va2pa(pDOut2->uiAddr), reg74.bit.DRAM_OUT2_SAO);

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R74_ENGINE_DRAM_OFS, reg74.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R78_ENGINE_DRAM_OFS, reg78.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R7C_ENGINE_DRAM_OFS, reg7C.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getDramOut0(UINT32 id, SIE_DRAM_OUT0_INFO *pDOut0)
{
	T_R60_ENGINE_DRAM reg60;
	T_R64_ENGINE_DRAM reg64;

	reg60.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R60_ENGINE_DRAM_OFS);
	reg64.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R64_ENGINE_DRAM_OFS);
	pDOut0->uiAddr = g_uiDramOut0VirAddr[id];//dma_getNonCacheAddr(reg5C.bit.DRAM_OUT0_SAO << 2);
	//pDOut0->OutSrc = reg60.bit.DRAM_OUT0_SOURCE;
	pDOut0->uiLofs = reg60.bit.DRAM_OUT0_OFSO << 2;
	pDOut0->PackBus = reg60.bit.DRAM_OUT0_PACK_BUS;
//#if (SIE_INT_FLIP_TEST==1)
#if 1 //(defined(_NVT_EMULATION_))
	pDOut0->bHFlip = reg60.bit.DRAM_OUT0_HFLIP;
	pDOut0->bVFlip = reg60.bit.DRAM_OUT0_VFLIP;
#else
	if (sie_getFunction(id)&SIE_DVI_EN) {
		pDOut0->bHFlip = reg60.bit.DRAM_OUT0_HFLIP;
		pDOut0->bVFlip = reg60.bit.DRAM_OUT0_VFLIP;
	} else {
		pDOut0->bHFlip = 0;
		pDOut0->bVFlip = 0;
	}
#endif
	pDOut0->bRingBufEn = reg60.bit.DRAM_OUT0_RINGBUF_EN;
	pDOut0->uiRingBufLen = reg64.bit.DRAM_OUT0_RINGBUF_LEN;
}

void sie_getDramOut0Flip(UINT32 id, SIE_DRAM_OUT0_INFO *pDOut0)
{
	UINT32 uiOut0Addr0;
	T_R4_ENGINE_FUNCTION reg04;
	T_R38_ENGINE_TIMING  reg38;
	T_R60_ENGINE_DRAM reg60;
	T_R64_ENGINE_DRAM reg64;

	reg04.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS);//no need//
	reg38.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R38_ENGINE_TIMING_OFS);//no need//
	reg60.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R60_ENGINE_DRAM_OFS);
	reg64.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R64_ENGINE_DRAM_OFS);

	//pDOut0->OutSrc = reg60.bit.DRAM_OUT0_SOURCE;
	pDOut0->uiLofs = reg60.bit.DRAM_OUT0_OFSO << 2;
	pDOut0->PackBus = reg60.bit.DRAM_OUT0_PACK_BUS;
//#if (SIE_INT_FLIP_TEST==1)
#if 1 //(defined(_NVT_EMULATION_))
	pDOut0->bHFlip = reg60.bit.DRAM_OUT0_HFLIP;
	pDOut0->bVFlip = reg60.bit.DRAM_OUT0_VFLIP;
#else
	if (reg04.bit.DVI_EN == 1) {
		pDOut0->bHFlip = reg60.bit.DRAM_OUT0_HFLIP;
		pDOut0->bVFlip = reg60.bit.DRAM_OUT0_VFLIP;
	} else {
		pDOut0->bHFlip = 0;
		pDOut0->bVFlip = 0;
	}
#endif


	uiOut0Addr0                          = g_uiDramOut0VirAddr[id];//dma_getNonCacheAddr(reg5C.bit.DRAM_OUT0_SAO << 2);


	if (reg04.bit.DVI_EN == 1) {
		if (pDOut0->bHFlip) {
			uiOut0Addr0                  = uiOut0Addr0 - pDOut0->uiLofs;
		}
		if (pDOut0->bVFlip) {
			uiOut0Addr0                 = uiOut0Addr0 - (reg38.bit.CRP_SZY - 1) * pDOut0->uiLofs;
		}
	}

	pDOut0->uiAddr                    = uiOut0Addr0;
	pDOut0->bRingBufEn = reg60.bit.DRAM_OUT0_RINGBUF_EN;
	pDOut0->uiRingBufLen = reg64.bit.DRAM_OUT0_RINGBUF_LEN;
}

void sie_getDramOut1(UINT32 id, SIE_DRAM_OUT1_INFO *pDOut1)
{
	T_R6C_ENGINE_DRAM reg6C;

	reg6C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R6C_ENGINE_DRAM_OFS);
	pDOut1->uiAddr = g_uiDramOut1VirAddr[id];//dma_getNonCacheAddr(reg68.bit.DRAM_OUT1_SAO << 2);
	pDOut1->uiLofs = reg6C.bit.DRAM_OUT1_OFSO << 2;
//#if (SIE_INT_FLIP_TEST==1)
#if 1 //(defined(_NVT_EMULATION_))
	pDOut1->bHFlip = reg6C.bit.DRAM_OUT1_HFLIP;
	pDOut1->bVFlip = reg6C.bit.DRAM_OUT1_VFLIP;
#else
	if (sie_getFunction(id)&SIE_DVI_EN) {
		pDOut1->bHFlip = reg6C.bit.DRAM_OUT1_HFLIP;
		pDOut1->bVFlip = reg6C.bit.DRAM_OUT1_VFLIP;
	} else {
		pDOut1->bHFlip = 0;
		pDOut1->bVFlip = 0;
	}
#endif
}

void sie_getDramOut1Flip(UINT32 id, SIE_DRAM_OUT1_INFO *pDOut1)
{
	UINT32 uiOut1Addr0;
	T_R4_ENGINE_FUNCTION reg04;
	T_R38_ENGINE_TIMING  reg38;
	T_R6C_ENGINE_DRAM reg6C;

	reg04.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS);//no need//
	reg38.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R38_ENGINE_TIMING_OFS);//no need//
	reg6C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R6C_ENGINE_DRAM_OFS);


	pDOut1->uiLofs = reg6C.bit.DRAM_OUT1_OFSO << 2;
//#if (SIE_INT_FLIP_TEST==1)
#if 1//(defined(_NVT_EMULATION_))
	pDOut1->bHFlip = reg6C.bit.DRAM_OUT1_HFLIP;
	pDOut1->bVFlip = reg6C.bit.DRAM_OUT1_VFLIP;
#else
	if (reg04.bit.DVI_EN == 1) {
		pDOut1->bHFlip = reg6C.bit.DRAM_OUT1_HFLIP;
		pDOut1->bVFlip = reg6C.bit.DRAM_OUT1_VFLIP;
	} else {
		pDOut1->bHFlip = 0;
		pDOut1->bVFlip = 0;
	}
#endif



	uiOut1Addr0 = g_uiDramOut1VirAddr[id];//dma_getNonCacheAddr(reg68.bit.DRAM_OUT1_SAO << 2);

	if (reg04.bit.DVI_EN) {
		if (pDOut1->bHFlip) {
			uiOut1Addr0                 = uiOut1Addr0 - pDOut1->uiLofs;
		}

		if (pDOut1->bVFlip) {
			uiOut1Addr0                 = uiOut1Addr0 - (reg38.bit.CRP_SZY - 1) * pDOut1->uiLofs;
		}
	}

	pDOut1->uiAddr                   = uiOut1Addr0;
}

void sie_getDramOut2(UINT32 id, SIE_DRAM_OUT2_INFO *pDOut2)
{
	T_R78_ENGINE_DRAM reg78;

	reg78.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R78_ENGINE_DRAM_OFS);
	pDOut2->uiAddr = g_uiDramOut2VirAddr[id];//dma_getNonCacheAddr(reg74.bit.DRAM_OUT2_SAO << 2);
	pDOut2->uiLofs = reg78.bit.DRAM_OUT2_OFSO << 2;
}

void sie_clr_buffer(UINT32 id)
{
	g_uiDramOut0VirAddr[id] = 0;
	g_uiDramOut1VirAddr[id] = 0;
	g_uiDramOut2VirAddr[id] = 0;
}
//NA//
#if 0
void sie1_getDramOut2Flip(SIE_DRAM_OUT2_INFO *pDOut2)
{
	UINT32 uiOut2Addr0;
	//UINT32 uiOut2Addr1;
	T_R4_ENGINE_FUNCTION reg04;
	T_R2C_ENGINE_TIMING  reg2C;
	T_R74_ENGINE_DRAM reg74;
	T_R78_ENGINE_DRAM reg78;
	//T_R7C_ENGINE_DRAM reg7C;
	T_RAC_BASIC_DVI   regAC;
	reg04.reg = SIE_GETREG(R4_ENGINE_FUNCTION_OFS);//no need//
	reg2C.reg = SIE_GETREG(R2C_ENGINE_TIMING_OFS);//no need//
	reg74.reg = SIE_GETREG(R74_ENGINE_DRAM_OFS);
	reg78.reg = SIE_GETREG(R78_ENGINE_DRAM_OFS);
	//reg7C.reg = SIE_GETREG(R7C_ENGINE_DRAM_OFS);
	regAC.reg = SIE_GETREG(RAC_BASIC_DVI_OFS);//no need//


	//pDOut2->uiAddr[0] = dma_getNonCacheAddr(reg74.bit.DRAM_OUT2_SAO0<<2);
	//pDOut2->OutSrc = reg74.bit.DRAM_OUT2_SOURCE;
	pDOut2->uiLofs = reg78.bit.DRAM_OUT2_OFSO << 2;
	//pDOut2->PackBus = reg78.bit.DRAM_OUT2_PACK_BUS;
	//pDOut2->bHFlip = reg78.bit.DRAM_OUT2_HFLIP;
	//pDOut2->bVFlip = reg78.bit.DRAM_OUT2_VFLIP;
	//pDOut2->uiAddr[1] = dma_getNonCacheAddr(reg7C.bit.DRAM_OUT2_SAO1<<2);
	pDOut2->uiPingPongBufNum = reg78.bit.DRAM_OUT2_BUFNUM;

	uiOut2Addr0                         = dma_getNonCacheAddr(reg74.bit.DRAM_OUT2_SAO << 2);
	//uiOut2Addr1                         = dma_getNonCacheAddr(reg7C.bit.DRAM_OUT2_SAO1<<2);

	//DBG_ERR("0: HFlip = %d,VFlip = %d, srcAddr0 =0x%08x, srcAddr1 = 0x%08x, lineoff = %d, DVI_EN = %d, Split = %d\r\n",pDOut2->bHFlip,pDOut2->bVFlip, uiOut2Addr0, uiOut2Addr1, pDOut2->uiLofs, reg04.bit.DVI_EN, regAC.bit.OUT_SPLIT);


	if ((reg04.bit.DVI_EN == 1) && (regAC.bit.OUT_SPLIT == 1)) {
		if (0) { //(pDOut2->bHFlip)
			uiOut2Addr0                 = uiOut2Addr0 - pDOut2->uiLofs;
			//uiOut2Addr1                 = uiOut2Addr1 - pDOut2->uiLofs;
		}

		//DBG_ERR("1: HFlip = %d,VFlip = %d, srcAddr0 =0x%08x, srcAddr1 = 0x%08x\r\n",pDOut2->bHFlip,pDOut2->bVFlip, uiOut2Addr0, uiOut2Addr1);

		if (0) { //(pDOut2->bVFlip)
			uiOut2Addr0                 = uiOut2Addr0 - (reg2C.bit.ACT_SZY - 1) * pDOut2->uiLofs;
			//uiOut2Addr1                 = uiOut2Addr1 - (reg2C.bit.ACT_SZY - 1)*pDOut2->uiLofs;
		}

		//DBG_ERR("2: uiOutAddr0 = 0x%08x, uiOutAddr1 = 0x%08x, ACTSzY = %d\r\n", uiOut2Addr0, uiOut2Addr1, reg2C.bit.ACT_SZY);

	}

	pDOut2->uiAddr                   = uiOut2Addr0;
	//pDOut2->uiAddr[1]                   = uiOut2Addr1;
}
#endif
//NA//

void sie_chgBurstLength(UINT32 id, SIE_BURST_LENGTH *pBurstLen)
{
	T_RA8_ENGINE_DRAM regA8;
	unsigned long spin_flags;
	
	spin_flags=sie_platform_spin_lock(0);

	regA8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RA8_ENGINE_DRAM_OFS);//no need//
	regA8.bit.DRAM_BURST_LENGTH_OUT0            = pBurstLen->BurstLenOut0;
	regA8.bit.DRAM_BURST_LENGTH_OUT1            = pBurstLen->BurstLenOut1;
	regA8.bit.DRAM_BURST_LENGTH_OUT2            = pBurstLen->BurstLenOut2;
	regA8.bit.DRAM_BURST_LENGTH_IN1             = pBurstLen->BurstLenIn1;
	regA8.bit.DRAM_BURST_LENGTH_IN2             = pBurstLen->BurstLenIn2;
	regA8.bit.DRAM_OUT0_BURST_MIN_INTERVAL      = pBurstLen->uiBurstIntvlOut0;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RA8_ENGINE_DRAM_OFS, regA8.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getBurstLength(UINT32 id, SIE_BURST_LENGTH *pBurstLen)
{
	T_RA8_ENGINE_DRAM regA8;

	regA8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RA8_ENGINE_DRAM_OFS);
	pBurstLen->BurstLenOut0 = regA8.bit.DRAM_BURST_LENGTH_OUT0;
	pBurstLen->BurstLenOut1 = regA8.bit.DRAM_BURST_LENGTH_OUT1;
	pBurstLen->BurstLenOut2 = regA8.bit.DRAM_BURST_LENGTH_OUT2;
	pBurstLen->BurstLenIn1 = regA8.bit.DRAM_BURST_LENGTH_IN1;
	pBurstLen->BurstLenIn2 = regA8.bit.DRAM_BURST_LENGTH_IN2;
	regA8.bit.DRAM_OUT0_BURST_MIN_INTERVAL      = pBurstLen->uiBurstIntvlOut0;
}

void sie_setDramBurst(UINT32 id, SIE_DRAM_BURST_INFO *pDramBurst)// would be removed soon
{
	T_RA8_ENGINE_DRAM regA8;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regA8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RA8_ENGINE_DRAM_OFS);//no need//
	regA8.bit.DRAM_BURST_LENGTH_OUT0            = pDramBurst->DramOut0Burst;
	regA8.bit.DRAM_BURST_LENGTH_OUT1            = pDramBurst->DramOut1Burst;
	regA8.bit.DRAM_BURST_LENGTH_OUT2            = pDramBurst->DramOut2Burst;
	regA8.bit.DRAM_BURST_LENGTH_IN1             = pDramBurst->DramIn1Burst;
	regA8.bit.DRAM_BURST_LENGTH_IN2             = pDramBurst->DramIn2Burst;
	regA8.bit.DRAM_OUT0_BURST_MIN_INTERVAL      = pDramBurst->uiDramOut0BurstIntvl;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RA8_ENGINE_DRAM_OFS, regA8.reg);
	sie_platform_spin_unlock(0,spin_flags);
}// would be removed soon

void sie_setFlip(UINT32 id, SIE_FLIP_INFO *pFlipParm)
{
	T_R60_ENGINE_DRAM    reg60;
	T_R6C_ENGINE_DRAM    reg6C;
	//no need//T_R78_ENGINE_DRAM    reg78;
	//no need//T_R84_ENGINE_DRAM    reg84;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg60.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R60_ENGINE_DRAM_OFS);//no need//
	reg6C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R6C_ENGINE_DRAM_OFS);//no need//
	//no need//reg78.reg = SIE_GETREG(R78_ENGINE_DRAM_OFS);//no need//
	//no need//reg84.reg = SIE_GETREG(R84_ENGINE_DRAM_OFS);//no need//

	//if (sie_getFunction(id)&SIE_DVI_EN) {
		reg60.bit.DRAM_OUT0_HFLIP           = pFlipParm->bOut0HFlip;
		reg60.bit.DRAM_OUT0_VFLIP           = pFlipParm->bOut0VFlip;
		reg6C.bit.DRAM_OUT1_HFLIP           = pFlipParm->bOut1HFlip;
		reg6C.bit.DRAM_OUT1_VFLIP           = pFlipParm->bOut1VFlip;

	//} else {
	//	reg60.bit.DRAM_OUT0_HFLIP           = 0;
	//	reg60.bit.DRAM_OUT0_VFLIP           = 0;
	//	reg6C.bit.DRAM_OUT1_HFLIP           = 0;
	//	reg6C.bit.DRAM_OUT1_VFLIP           = 0;
	//}
	//NA//reg78.bit.DRAM_OUT2_HFLIP           = pFlipParm->bOut2HFlip;
	//NA//reg78.bit.DRAM_OUT2_VFLIP           = pFlipParm->bOut2VFlip;
	//NA//reg84.bit.DRAM_OUT3_HFLIP           = pFlipParm->bOut3HFlip;
	//NA//reg84.bit.DRAM_OUT3_VFLIP           = pFlipParm->bOut3VFlip;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R60_ENGINE_DRAM_OFS, reg60.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R6C_ENGINE_DRAM_OFS, reg6C.reg);
	//NA//SIE_SETREG(R78_ENGINE_DRAM_OFS,reg78.reg);
	//NA//SIE_SETREG(R84_ENGINE_DRAM_OFS,reg84.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getFlip(UINT32 id, SIE_FLIP_INFO *pFlipParm)
{
	T_R60_ENGINE_DRAM    reg60;
	T_R6C_ENGINE_DRAM    reg6C;
	//no need//T_R78_ENGINE_DRAM    reg78;
	//no need//T_R84_ENGINE_DRAM    reg84;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg60.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R60_ENGINE_DRAM_OFS);//no need//
	reg6C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R6C_ENGINE_DRAM_OFS);//no need//
	//no need//reg78.reg = SIE_GETREG(R78_ENGINE_DRAM_OFS);//no need//
	//no need//reg84.reg = SIE_GETREG(R84_ENGINE_DRAM_OFS);//no need//

	//if (sie_getFunction(id)&SIE_DVI_EN) {
		pFlipParm->bOut0HFlip = reg60.bit.DRAM_OUT0_HFLIP;
		pFlipParm->bOut0VFlip = reg60.bit.DRAM_OUT0_VFLIP;
		pFlipParm->bOut1HFlip = reg6C.bit.DRAM_OUT1_HFLIP;
		pFlipParm->bOut1VFlip = reg6C.bit.DRAM_OUT1_VFLIP;
	//} else {
	//	pFlipParm->bOut0HFlip = 0;
	//	pFlipParm->bOut0VFlip = 0;
	//	pFlipParm->bOut1HFlip = 0;
	//	pFlipParm->bOut1VFlip = 0;
	//}
	//no need//pFlipParm->bOut2HFlip = reg78.bit.DRAM_OUT2_HFLIP;
	//no need//pFlipParm->bOut2VFlip = reg78.bit.DRAM_OUT2_VFLIP;
	//no need//pFlipParm->bOut3HFlip = reg84.bit.DRAM_OUT3_HFLIP;
	//no need//pFlipParm->bOut3VFlip = reg84.bit.DRAM_OUT3_VFLIP;
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setDVI(UINT32 id, SIE_DVI_INFO *pDviParam)
{
	T_RAC_BASIC_DVI regDVI;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regDVI.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RAC_BASIC_DVI_OFS);//no need//

	if (pDviParam->DviFormat == DVI_FORMAT_CCIR601) {
		regDVI.bit.DVI_FORMAT       = 0;
		regDVI.bit.CCIR656_ACT_SEL  = 0/*don't care*/;
	} else if (pDviParam->DviFormat == DVI_FORMAT_CCIR656_EAV) {
		regDVI.bit.DVI_FORMAT       = 1;
		regDVI.bit.CCIR656_ACT_SEL  = 0;
	} else if (pDviParam->DviFormat == DVI_FORMAT_CCIR656_ACT) {
		regDVI.bit.DVI_FORMAT       = 1;
		regDVI.bit.CCIR656_ACT_SEL  = 1;
	}

	if (pDviParam->DviInMode == DVI_MODE_SD) {
		regDVI.bit.DVI_MODE     = 0;
		regDVI.bit.HD_IN_SWAP   = 0/*don't care*/;
	} else if (pDviParam->DviInMode == DVI_MODE_HD) {
		regDVI.bit.DVI_MODE     = 1;
		regDVI.bit.HD_IN_SWAP   = 0;
	} else if (pDviParam->DviInMode == DVI_MODE_HD_INV) {
		regDVI.bit.DVI_MODE     = 1;
		regDVI.bit.HD_IN_SWAP   = 1;
	}
	regDVI.bit.OUT_SWAP         = pDviParam->DviOutSwap;
	regDVI.bit.OUT_SPLIT        = pDviParam->bOutSplit;
	regDVI.bit.FIELD_EN         = pDviParam->bFieldEn;
	regDVI.bit.FIELD_SEL        = pDviParam->bFieldSel;
	regDVI.bit.CCIR656_VD_MODE  = pDviParam->bCCIR656VdSel;
	regDVI.bit.PARAL_DATA_PERIOD = pDviParam->uiDataPeriod;
	regDVI.bit.PARAL_DATA_INDEX = pDviParam->uiDataIdx;
	regDVI.bit.CCIR656_AUTO_ALIGN = pDviParam->bAutoAlign;
	regDVI.bit.OUT_YUV420         = pDviParam->bOutYUV420;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RAC_BASIC_DVI_OFS, regDVI.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getDVI(UINT32 id, SIE_DVI_INFO *pDviParam)
{
	T_RAC_BASIC_DVI regDVI;

	regDVI.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RAC_BASIC_DVI_OFS);

	if (regDVI.bit.DVI_FORMAT == 0) {
		pDviParam->DviFormat = DVI_FORMAT_CCIR601;
	} else { // ifregDVI.bit.DVI_FORMAT==1)
		if (regDVI.bit.CCIR656_ACT_SEL == 0) {
			pDviParam->DviFormat = DVI_FORMAT_CCIR656_EAV;
		} else { // if(regDVI.bit.CCIR656_ACT_SEL==1)
			pDviParam->DviFormat = DVI_FORMAT_CCIR656_ACT;
		}
	}
	if (regDVI.bit.DVI_MODE == 0) {
		pDviParam->DviInMode = DVI_MODE_SD;
	} else { // ifregDVI.bit.DVI_FORMAT==1)
		if (regDVI.bit.HD_IN_SWAP == 0) {
			pDviParam->DviInMode = DVI_MODE_HD;
		} else { // if(regDVI.bit.HD_IN_SWAP==1)
			pDviParam->DviInMode = DVI_MODE_HD_INV;
		}
	}
	pDviParam->DviOutSwap   = regDVI.bit.OUT_SWAP;
	pDviParam->bOutSplit    = regDVI.bit.OUT_SPLIT;
	pDviParam->bOutYUV420   = regDVI.bit.OUT_YUV420;
	pDviParam->bFieldEn     = regDVI.bit.FIELD_EN;
	pDviParam->bFieldSel    = regDVI.bit.FIELD_SEL;
	pDviParam->bCCIR656VdSel = regDVI.bit.CCIR656_VD_MODE;
	pDviParam->uiDataPeriod = regDVI.bit.PARAL_DATA_PERIOD;
	pDviParam->uiDataIdx    = regDVI.bit.PARAL_DATA_INDEX;
	pDviParam->bAutoAlign   = regDVI.bit.CCIR656_AUTO_ALIGN;
}


void sie_setPatGen(UINT32 id, SIE_PATGEN_INFO *pPatGen)
{
	T_RB0_BASIC_PATGEN regPG;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regPG.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RB0_BASIC_PATGEN_OFS);//no need//

	regPG.bit.PATGEN_MODE   = pPatGen->PatGenMode;
	regPG.bit.PATGEN_VAL    = pPatGen->uiPatGenVal;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RB0_BASIC_PATGEN_OFS, regPG.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_getPatGen(UINT32 id, SIE_PATGEN_INFO *pPatGen)
{
	T_RB0_BASIC_PATGEN regPG;
	regPG.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RB0_BASIC_PATGEN_OFS);//no need//

	pPatGen->PatGenMode = regPG.bit.PATGEN_MODE;
	pPatGen->uiPatGenVal = regPG.bit.PATGEN_VAL;
}

void sie_setObDt(UINT32 id, SIE_OB_DT_INFO *pObDt)
{
	T_RB4_BASIC_OB regB4;
	T_RB8_BASIC_OB regB8;
	T_RBC_BASIC_OB regBC;
	T_RC0_BASIC_OB regC0;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regB4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RB4_BASIC_OB_OFS);//no need//
	regB8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RB8_BASIC_OB_OFS);//no need//
	regBC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RBC_BASIC_OB_OFS);
	regC0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RC0_BASIC_OB_OFS);//no need//

	regB4.bit.OBDT_X   = pObDt->uiStX;
	regB4.bit.OBDT_Y   = pObDt->uiStY;
	regB8.bit.OBDT_WIDTH   = pObDt->uiSzX;
	regB8.bit.OBDT_HEIGHT  = pObDt->uiSzY;
	regBC.bit.OBDT_GAIN   = pObDt->uiSubRatio;
	regC0.bit.OBDT_THRES   = pObDt->uiThres;
	regC0.bit.OBDT_XDIV   = pObDt->uiDivX;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RB4_BASIC_OB_OFS, regB4.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RB8_BASIC_OB_OFS, regB8.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RBC_BASIC_OB_OFS, regBC.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RC0_BASIC_OB_OFS, regC0.reg);
	sie_platform_spin_unlock(0,spin_flags);
}


void sie_getObDtRslt(UINT32 id, SIE_OB_DT_RSLT_INFO *pObDtRslt)
{
	T_RC4_BASIC_OB regC4;

	regC4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RC4_BASIC_OB_OFS);

	pObDtRslt->uiAvg = regC4.bit.OBDT_AVER;
	pObDtRslt->uiCnt = regC4.bit.OBDT_SCNT;
}

void sie_setObOfs(UINT32 id, SIE_OB_OFS_INFO *pObDtOfs)
{
	T_RBC_BASIC_OB regBC;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regBC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RBC_BASIC_OB_OFS);
	regBC.bit.OB_OFS   = pObDtOfs->uiObOfs;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RBC_BASIC_OB_OFS, regBC.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getObOfs(UINT32 id, SIE_OB_OFS_INFO *pObDtOfs)
{
    T_RBC_BASIC_OB regBC;
    regBC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RBC_BASIC_OB_OFS);
    pObDtOfs->uiObOfs = regBC.bit.OB_OFS;
}

void sie_setDpc(UINT32 id, SIE_DPC_INFO *pDpc)
{
    T_R30_ENGINE_CFA_PATTERN reg30;
	T_RCC_BASIC_DEFECT regCC;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regCC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],RCC_BASIC_DEFECT_OFS);//no need//
	reg30.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS);
	regCC.bit.DEF_FACT   = pDpc->DefFact;
	reg30.bit.DPC_CFAPAT = pDpc->DpcCfapat;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],RCC_BASIC_DEFECT_OFS, regCC.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS, reg30.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_setDecompanding(UINT32 id, SIE_DECOMPANDING_INFO *pDecomp)
{
	T_RD0_DECOMP regD0;
	T_RD4_DECOMP regD4;
	T_RD8_DECOMP regD8;
	T_RDC_DECOMP regDC;
	T_RE0_DECOMP regE0;
	T_RE4_DECOMP regE4;
	T_RE8_DECOMP regE8;
	T_REC_DECOMP regEC;
	T_RF0_DECOMP regF0;
	T_RF4_DECOMP regF4;
	T_RF8_DECOMP regF8;
	T_RFC_DECOMP regFC;
	T_R100_DECOMP reg100;
	T_R104_DECOMP reg104;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regD0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RD0_DECOMP_OFS);
	regD4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RD4_DECOMP_OFS);
	regD8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RD8_DECOMP_OFS);
	regDC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RDC_DECOMP_OFS);
	regE0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RE0_DECOMP_OFS);
	regE4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RE4_DECOMP_OFS);
	regE8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RE8_DECOMP_OFS);
	regEC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], REC_DECOMP_OFS);
	regF0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RF0_DECOMP_OFS);
	regF4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RF4_DECOMP_OFS);
	regF8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RF8_DECOMP_OFS);
	regFC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], RFC_DECOMP_OFS);
	reg100.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R100_DECOMP_OFS);
	reg104.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R104_DECOMP_OFS);
	regD0.bit.DECOMP_KP0   = pDecomp->uiDecompKp[0];
	regD0.bit.DECOMP_KP1   = pDecomp->uiDecompKp[1];
	regD4.bit.DECOMP_KP2   = pDecomp->uiDecompKp[2];
	regD4.bit.DECOMP_KP3   = pDecomp->uiDecompKp[3];
	regD8.bit.DECOMP_KP4   = pDecomp->uiDecompKp[4];
	regD8.bit.DECOMP_KP5   = pDecomp->uiDecompKp[5];
	regDC.bit.DECOMP_KP6   = pDecomp->uiDecompKp[6];
	regDC.bit.DECOMP_KP7   = pDecomp->uiDecompKp[7];
	regE0.bit.DECOMP_KP8   = pDecomp->uiDecompKp[8];
	regE0.bit.DECOMP_KP9   = pDecomp->uiDecompKp[9];
	regE4.bit.DECOMP_KP10   = pDecomp->uiDecompKp[10];

    //sub-point
	regE8.bit.DECOMP_SP0   = pDecomp->uiDecompSp[0];
	regE8.bit.DECOMP_SP1   = pDecomp->uiDecompSp[1];
	regEC.bit.DECOMP_SP2   = pDecomp->uiDecompSp[2];
	regEC.bit.DECOMP_SP3   = pDecomp->uiDecompSp[3];
	regF0.bit.DECOMP_SP4   = pDecomp->uiDecompSp[4];
	regF0.bit.DECOMP_SP5   = pDecomp->uiDecompSp[5];
	regF4.bit.DECOMP_SP6   = pDecomp->uiDecompSp[6];
	regF4.bit.DECOMP_SP7   = pDecomp->uiDecompSp[7];
	regF8.bit.DECOMP_SP8   = pDecomp->uiDecompSp[8];
	regF8.bit.DECOMP_SP9   = pDecomp->uiDecompSp[9];
	regFC.bit.DECOMP_SP10   = pDecomp->uiDecompSp[10];
	regFC.bit.DECOMP_SP11   = pDecomp->uiDecompSp[11];

    // shift
	reg100.bit.DECOMP_SB0   = pDecomp->uiDecompSb[0];
	reg100.bit.DECOMP_SB1   = pDecomp->uiDecompSb[1];
	reg100.bit.DECOMP_SB2   = pDecomp->uiDecompSb[2];
	reg100.bit.DECOMP_SB3   = pDecomp->uiDecompSb[3];
	reg100.bit.DECOMP_SB4   = pDecomp->uiDecompSb[4];
	reg100.bit.DECOMP_SB5   = pDecomp->uiDecompSb[5];
	reg100.bit.DECOMP_SB6   = pDecomp->uiDecompSb[6];
	reg100.bit.DECOMP_SB7   = pDecomp->uiDecompSb[7];
	reg104.bit.DECOMP_SB8   = pDecomp->uiDecompSb[8];
	reg104.bit.DECOMP_SB9   = pDecomp->uiDecompSb[9];
	reg104.bit.DECOMP_SB10   = pDecomp->uiDecompSb[10];
	reg104.bit.DECOMP_SB11   = pDecomp->uiDecompSb[11];

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RD0_DECOMP_OFS, regD0.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RD4_DECOMP_OFS, regD4.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RD8_DECOMP_OFS, regD8.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RDC_DECOMP_OFS, regDC.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RE0_DECOMP_OFS, regE0.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RE4_DECOMP_OFS, regE4.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RE8_DECOMP_OFS, regE8.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], REC_DECOMP_OFS, regEC.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RF0_DECOMP_OFS, regF0.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RF4_DECOMP_OFS, regF4.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RF8_DECOMP_OFS, regF8.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], RFC_DECOMP_OFS, regFC.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R100_DECOMP_OFS, reg100.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R104_DECOMP_OFS, reg104.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_setCompanding(UINT32 id, SIE_COMPANDING_INFO * pComp)
{
	T_R108_COMP reg108;
	T_R10C_COMP reg10C;
	T_R110_COMP reg110;
	T_R114_COMP reg114;
	T_R118_COMP reg118;
	T_R11C_COMP reg11C;
	T_R120_COMP reg120;
	T_R124_COMP reg124;
	T_R128_COMP reg128;
	T_R12C_COMP reg12C;
	T_R130_COMP reg130;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg108.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R108_COMP_OFS);
	reg10C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R10C_COMP_OFS);
	reg110.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R110_COMP_OFS);
	reg114.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R114_COMP_OFS);
	reg118.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R118_COMP_OFS);
	reg11C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R11C_COMP_OFS);
	reg120.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R120_COMP_OFS);
	reg124.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R124_COMP_OFS);
	reg128.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R128_COMP_OFS);
	reg12C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R12C_COMP_OFS);
	reg130.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R130_COMP_OFS);
	reg108.bit.COMP_KP0   = pComp->uiCompKp[0];
	reg108.bit.COMP_KP1   = pComp->uiCompKp[1];
	reg108.bit.COMP_KP2   = pComp->uiCompKp[2];
	reg108.bit.COMP_KP3   = pComp->uiCompKp[3];
	reg10C.bit.COMP_KP4   = pComp->uiCompKp[4];
	reg10C.bit.COMP_KP5   = pComp->uiCompKp[5];
	reg10C.bit.COMP_KP6   = pComp->uiCompKp[6];
	reg10C.bit.COMP_KP7   = pComp->uiCompKp[7];
	reg110.bit.COMP_KP8   = pComp->uiCompKp[8];
	reg110.bit.COMP_KP9   = pComp->uiCompKp[9];
	reg110.bit.COMP_KP10   = pComp->uiCompKp[10];

	reg114.bit.COMP_SP0   = pComp->uiCompSp[0];
	reg114.bit.COMP_SP1   = pComp->uiCompSp[1];
	reg118.bit.COMP_SP2   = pComp->uiCompSp[2];
	reg118.bit.COMP_SP3   = pComp->uiCompSp[3];
	reg11C.bit.COMP_SP4   = pComp->uiCompSp[4];
	reg11C.bit.COMP_SP5   = pComp->uiCompSp[5];
	reg120.bit.COMP_SP6   = pComp->uiCompSp[6];
	reg120.bit.COMP_SP7   = pComp->uiCompSp[7];
	reg124.bit.COMP_SP8   = pComp->uiCompSp[8];
	reg124.bit.COMP_SP9   = pComp->uiCompSp[9];
	reg128.bit.COMP_SP10   = pComp->uiCompSp[10];
	reg128.bit.COMP_SP11   = pComp->uiCompSp[11];

	reg12C.bit.COMP_SB0   = pComp->uiCompSb[0];
	reg12C.bit.COMP_SB1   = pComp->uiCompSb[1];
	reg12C.bit.COMP_SB2   = pComp->uiCompSb[2];
	reg12C.bit.COMP_SB3   = pComp->uiCompSb[3];
	reg12C.bit.COMP_SB4   = pComp->uiCompSb[4];
	reg12C.bit.COMP_SB5   = pComp->uiCompSb[5];
	reg12C.bit.COMP_SB6   = pComp->uiCompSb[6];
	reg12C.bit.COMP_SB7   = pComp->uiCompSb[7];
	reg130.bit.COMP_SB8   = pComp->uiCompSb[8];
	reg130.bit.COMP_SB9   = pComp->uiCompSb[9];
	reg130.bit.COMP_SB10   = pComp->uiCompSb[10];
	reg130.bit.COMP_SB11   = pComp->uiCompSb[11];

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R108_COMP_OFS, reg108.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R10C_COMP_OFS, reg10C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R110_COMP_OFS, reg110.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R114_COMP_OFS, reg114.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R118_COMP_OFS, reg118.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R11C_COMP_OFS, reg11C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R120_COMP_OFS, reg120.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R124_COMP_OFS, reg124.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R128_COMP_OFS, reg128.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R12C_COMP_OFS, reg12C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R130_COMP_OFS, reg130.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_setECS(UINT32 id, SIE_ECS_INFO *pEcs)
{
	T_R154_BASIC_ECS regEcs0;
	T_R158_BASIC_ECS regEcs1;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regEcs0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R154_BASIC_ECS_OFS);//no need//
	regEcs1.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R158_BASIC_ECS_OFS);//no need//

	regEcs0.bit.MAP_SIZESEL     = pEcs->MapSizeSel;
	regEcs0.bit.MAP_DTHR_EN     = pEcs->bDthrEn;
	regEcs0.bit.MAP_DTHR_RST    = pEcs->bDthrRst;
	regEcs0.bit.MAP_DTHR_LEVEL  = pEcs->uiDthrLvl;
	regEcs0.bit.MAP_SHIFT       = pEcs->uiMapShift;

	regEcs1.bit.MAP_HSCL    = pEcs->uiHSclFctr;
	regEcs1.bit.MAP_VSCL    = pEcs->uiVSclFctr;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R154_BASIC_ECS_OFS, regEcs0.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R158_BASIC_ECS_OFS, regEcs1.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getECS(UINT32 id, SIE_ECS_INFO *pEcs)
{
	T_R154_BASIC_ECS regEcs0;
	T_R158_BASIC_ECS regEcs1;

	regEcs0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R154_BASIC_ECS_OFS);//no need//
	regEcs1.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R158_BASIC_ECS_OFS);//no need//

	pEcs->MapSizeSel = regEcs0.bit.MAP_SIZESEL;
	pEcs->bDthrEn = regEcs0.bit.MAP_DTHR_EN;
	pEcs->bDthrRst = regEcs0.bit.MAP_DTHR_RST;
	pEcs->uiDthrLvl = regEcs0.bit.MAP_DTHR_LEVEL;
	pEcs->uiMapShift = regEcs0.bit.MAP_SHIFT;

	pEcs->uiHSclFctr = regEcs1.bit.MAP_HSCL;
	pEcs->uiVSclFctr = regEcs1.bit.MAP_VSCL;
}


void sie_setDGain(UINT32 id, SIE_DGAIN_INFO *pDGain)
{
	T_R15C_BASIC_DGAIN regDGn;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	regDGn.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R15C_BASIC_DGAIN_OFS);//no need//
	regDGn.bit.DGAIN_GAIN     = pDGain->uiGainIn8P8Bit;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R15C_BASIC_DGAIN_OFS, regDGn.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setCGain(UINT32 id, SIE_CG_INFO *pCGain)
{
	T_R1B4_BASIC_CG reg1b4;
	T_R1B8_BASIC_CG reg1b8;
	T_R1BC_BASIC_CG reg1bc;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg1b4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1B4_BASIC_CG_OFS);
	reg1b8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1B8_BASIC_CG_OFS);
	reg1bc.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1BC_BASIC_CG_OFS);
	reg1b4.bit.CGAIN_RGAIN     = pCGain->uiRGain;
	reg1b4.bit.CGAIN_GRGAIN     = pCGain->uiGrGain;
	reg1b8.bit.CGAIN_GBGAIN     = pCGain->uiGbGain;
	reg1b8.bit.CGAIN_BGAIN     = pCGain->uiBGain;
	reg1bc.bit.CGAIN_IRGAIN     = pCGain->uiIrGain;
	reg1bc.bit.CGAIN_LEVEL_SEL = pCGain->bGainSel;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1B4_BASIC_CG_OFS, reg1b4.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1B8_BASIC_CG_OFS, reg1b8.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1BC_BASIC_CG_OFS, reg1bc.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_getCGain(UINT32 id, SIE_CG_INFO *pCGain)
{
	T_R1B4_BASIC_CG reg1b4;
	T_R1B8_BASIC_CG reg1b8;
	T_R1BC_BASIC_CG reg1bc;

	reg1b4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1B4_BASIC_CG_OFS);
	reg1b8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1B8_BASIC_CG_OFS);
	reg1bc.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1BC_BASIC_CG_OFS);
	pCGain->uiRGain  = reg1b4.bit.CGAIN_RGAIN;
	pCGain->uiGrGain = reg1b4.bit.CGAIN_GRGAIN;
	pCGain->uiGbGain = reg1b8.bit.CGAIN_GBGAIN;
	pCGain->uiBGain = reg1b8.bit.CGAIN_BGAIN;
	pCGain->uiIrGain = reg1bc.bit.CGAIN_IRGAIN;
	pCGain->bGainSel = reg1bc.bit.CGAIN_LEVEL_SEL;
}

void sie_setStcsPath(UINT32 id, SIE_STCS_PATH_INFO *pSetting)
{
	T_R1E8_STCS reg1E8;
	T_R350_STCS_VA reg350;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg1E8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1E8_STCS_OFS);//no need//
	reg350.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R350_STCS_VA_OFS);//no need//

	reg1E8.bit.STCS_VIG_EN    = pSetting->bVigEn;
	reg1E8.bit.STCS_LA_CG_EN    = pSetting->bLaCgEn;
	reg1E8.bit.STCS_GAMMA_1_EN    = pSetting->bLaGma1En;
	reg1E8.bit.STCS_GAMMA_2_EN    = pSetting->bVaGma2En;
	reg1E8.bit.STCS_HISTO_Y_SEL    = pSetting->HistoSrcSel;
	reg1E8.bit.STCS_CA_TH_EN    = pSetting->bCaThEn;
	reg1E8.bit.STCS_VA_CG_EN    = pSetting->bVaCgEn;
	reg1E8.bit.STCS_LA_RGB2Y_SEL    = pSetting->La1SrcSel;
	reg1E8.bit.STCS_LA_RGB2Y1_MOD = pSetting->LaRgb2Y1Mod;
	reg1E8.bit.STCS_LA_RGB2Y2_MOD = pSetting->LaRgb2Y2Mod;
	reg1E8.bit.STCS_LA_TH_EN      = pSetting->bLaThEn;
	reg1E8.bit.STCS_CA_ACCM_SRC      = pSetting->bCaAccmSrc;
	reg1E8.bit.STCS_COMPANDING_SHIFT      = pSetting->Companding_shift;
	reg350.bit.VACC_OUTSEL    = pSetting->VaOutSel;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1E8_STCS_OFS, reg1E8.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R350_STCS_VA_OFS, reg350.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getStcsPath(UINT32 id, SIE_STCS_PATH_INFO *pSetting)
{
	T_R1E8_STCS reg1E8;
	T_R350_STCS_VA reg350;

	reg1E8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1E8_STCS_OFS);
	reg350.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R350_STCS_VA_OFS);

	pSetting->bVigEn = reg1E8.bit.STCS_VIG_EN;
	pSetting->bLaCgEn = reg1E8.bit.STCS_LA_CG_EN;
	pSetting->bLaGma1En = reg1E8.bit.STCS_GAMMA_1_EN;
	pSetting->bVaGma2En = reg1E8.bit.STCS_GAMMA_2_EN;
	pSetting->HistoSrcSel = reg1E8.bit.STCS_HISTO_Y_SEL;
	pSetting->bCaThEn = reg1E8.bit.STCS_CA_TH_EN;
	pSetting->bVaCgEn = reg1E8.bit.STCS_VA_CG_EN;
	pSetting->La1SrcSel = reg1E8.bit.STCS_LA_RGB2Y_SEL;

	pSetting->VaOutSel = reg350.bit.VACC_OUTSEL;
}


void sie_setVIG(UINT32 id, SIE_VIG_INFO *pVig)
{
	UINT32 i, uiBuf;
	T_R1FC_STCS_VIG reg1FC;
	T_R200_STCS_VIG reg200;
	T_R204_STCS_VIG reg204;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg1FC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1FC_STCS_VIG_OFS);//no need//
	reg200.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R200_STCS_VIG_OFS);//no need//
	reg204.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R204_STCS_VIG_OFS);//no need//

	reg1FC.bit.VIG_X    = pVig->uiVigX;
	reg1FC.bit.VIG_Y    = pVig->uiVigY;
	reg1FC.bit.VIG_TAB_GAIN    = pVig->uiVigTabGain;

	reg200.bit.VIG_T    = pVig->uiVigT;

	reg204.bit.VIG_XDIV    = pVig->uiVigXDiv;
	reg204.bit.VIG_YDIV    = pVig->uiVigYDiv;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1FC_STCS_VIG_OFS, reg1FC.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R200_STCS_VIG_OFS, reg200.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R204_STCS_VIG_OFS, reg204.reg);

	if (pVig->puiVigLut == NULL) {
		sie_platform_spin_unlock(0,spin_flags);
		return;
	}
	for (i = 0; i < 15; i += 3) {
		uiBuf  = (pVig->puiVigLut[i + 0] & 0x3ff) << 0;
		uiBuf |= (pVig->puiVigLut[i + 1] & 0x3ff) << 10;
		uiBuf |= (pVig->puiVigLut[i + 2] & 0x3ff) << 20;
		SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R208_STCS_VIG_OFS + (i * 4 / 3), uiBuf);
	}
	uiBuf  = (pVig->puiVigLut[15] & 0x3ff) << 0;
	uiBuf |= (pVig->puiVigLut[16] & 0x3ff) << 10;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R208_STCS_VIG_OFS + (20), uiBuf);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setStcsOb(UINT32 id, SIE_STCS_OB_INFO *pSetting)
{
	T_R2A8_STCS_OB reg2A8;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg2A8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R2A8_STCS_OB_OFS);//no need//

	reg2A8.bit.STCS_CA_OB_OFS    = pSetting->uiCaObOfs;
	reg2A8.bit.STCS_LA_OB_OFS    = pSetting->uiLaObOfs;

	//DBG_ERR("ob %d/%d/%d/%d\r\n",pSetting->uiCaObOfs,pSetting->uiLaObOfs,reg2A8.bit.STCS_CA_OB_OFS,reg2A8.bit.STCS_LA_OB_OFS);

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R2A8_STCS_OB_OFS, reg2A8.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setmd(UINT32 id, SIE_MD_INFO *pSetting)
{
	T_R3C0_STCS_MD reg3c0;
	T_R3C4_STCS_MD reg3c4;
	T_R3C8_STCS_MD reg3c8;
	T_R3D0_STCS_MD reg3d0;
	T_R3D4_STCS_MD reg3d4;
	T_R3D8_STCS_MD reg3d8;
	unsigned long spin_flags;
	
	spin_flags=sie_platform_spin_lock(0);

	reg3c0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R3C0_STCS_MD_OFS);//no need//
	reg3c4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R3C4_STCS_MD_OFS);//no need//
	reg3c8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R3C8_STCS_MD_OFS);//no need//
	reg3d0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R3D0_STCS_MD_OFS);//no need//
	reg3d4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R3D4_STCS_MD_OFS);//no need//
	reg3d8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R3D8_STCS_MD_OFS);//no need//

	reg3c0.bit.MD_Y_SEL       = pSetting->md_src;
	reg3c0.bit.MD_SUM_FRMS    = pSetting->sum_frms-1;
	reg3c0.bit.MD_BLKDIFF_THR = pSetting->blkdiff_thr;
	reg3c4.bit.MD_TOTAL_BLKDIFF_THR = pSetting->total_blkdiff_thr;
	reg3c8.bit.MD_BLKDIFF_CNT_THR = pSetting->blkdiff_cnt_thr;
	reg3d0.bit.MD_VALID_REG_MODE = pSetting->mask_mode;
	reg3d4.bit.MD_VALID_REG0_31 = pSetting->mask0;
	reg3d8.bit.MD_VALID_REG32_63 = pSetting->mask1;
	
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R3C0_STCS_MD_OFS, reg3c0.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R3C4_STCS_MD_OFS, reg3c4.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R3C8_STCS_MD_OFS, reg3c8.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R3D0_STCS_MD_OFS, reg3d0.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R3D4_STCS_MD_OFS, reg3d4.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R3D8_STCS_MD_OFS, reg3d8.reg);
	
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_getStcsOb(UINT32 id, SIE_STCS_OB_INFO *pSetting)
{
	T_R2A8_STCS_OB reg2A8;

	reg2A8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R2A8_STCS_OB_OFS);//no need//

	pSetting->uiCaObOfs = reg2A8.bit.STCS_CA_OB_OFS;
	pSetting->uiLaObOfs = reg2A8.bit.STCS_LA_OB_OFS;
}
void sie_setCaCrp(UINT32 id, SIE_CA_CROP_INFO *pCaCrp)
{
	T_R220_STCS_CA reg220;
	T_R228_STCS_CA reg228;
	T_R30_ENGINE_CFA_PATTERN reg30;
	//static UINT32 flag = 0;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg220.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R220_STCS_CA_OFS);
	reg228.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R228_STCS_CA_OFS);
	reg30.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS);

	reg220.bit.CA_CROP_STX    = pCaCrp->uiStX;
	reg220.bit.CA_CROP_STY    = pCaCrp->uiStY;

	reg228.bit.CA_CROP_SZX    = pCaCrp->uiSzX;
	reg228.bit.CA_CROP_SZY    = pCaCrp->uiSzY;

	reg30.bit.CA_CFAPAT    = pCaCrp->CfaPat;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R220_STCS_CA_OFS, reg220.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R228_STCS_CA_OFS, reg228.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS, reg30.reg);
	sie_platform_spin_unlock(0,spin_flags);
#if 0
    if (flag == 0)
    {
		nvt_dbg(IND, "drv ca crp %d/%d\r\n",pCaCrp->uiSzX,pCaCrp->uiSzY);
		flag = 1;
    }
#endif
}


void sie_setCaScl(UINT32 id, SIE_CA_SCAL_INFO *pCaScl)
{
	T_R224_STCS_CA reg224;
	unsigned long spin_flags;
	//static UINT32  flag=0;

	spin_flags=sie_platform_spin_lock(0);
	reg224.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R224_STCS_CA_OFS);

	reg224.bit.SMPL_X_FACT    = pCaScl->uiFctrX;
	reg224.bit.SMPL_Y_FACT    = pCaScl->uiFctrY;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R224_STCS_CA_OFS, reg224.reg);
	sie_platform_spin_unlock(0,spin_flags);
#if 0
	if (flag == 0)
    {
		nvt_dbg(IND, "drv ca scl %d/%d\r\n",pCaScl->uiFctrX,pCaScl->uiFctrY);
		flag = 1;
    }
	#endif
}
void sie_setCaIrSub(UINT32 id, SIE_CA_IRSUB_INFO *pCaIrSubInfo)
{
	T_R244_STCS_CA_IR reg244;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg244.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R244_STCS_CA_IR_OFS);
#if (defined(_NVT_EMULATION_))
	reg244.bit.CA_IRSUB_RWET    = pCaIrSubInfo->uiCaIrSubRWet;
	reg244.bit.CA_IRSUB_GWET    = pCaIrSubInfo->uiCaIrSubGWet;
	reg244.bit.CA_IRSUB_BWET    = pCaIrSubInfo->uiCaIrSubBWet;
#else
	reg244.bit.CA_IRSUB_RWET    = 0;//pCaIrSubInfo->uiCaIrSubRWet;
	reg244.bit.CA_IRSUB_GWET    = 0;//pCaIrSubInfo->uiCaIrSubGWet;
	reg244.bit.CA_IRSUB_BWET    = 0;//pCaIrSubInfo->uiCaIrSubBWet;
#endif

	irsub_r[id] = pCaIrSubInfo->uiCaIrSubRWet;
	irsub_g[id] = pCaIrSubInfo->uiCaIrSubGWet;
	irsub_b[id] = pCaIrSubInfo->uiCaIrSubBWet;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R244_STCS_CA_IR_OFS, reg244.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_setLaIrSub(UINT32 id, SIE_LA_IRSUB_INFO *pLaIrSubInfo)
{
	T_R1F4_STCS_LA_IR reg1f4;
	UINT32 spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg1f4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R1F4_STCS_LA_IR_OFS);

	reg1f4.bit.LA_IRSUB_RWET    = pLaIrSubInfo->uiLaIrSubRWet;
	reg1f4.bit.LA_IRSUB_GWET    = pLaIrSubInfo->uiLaIrSubGWet;
	reg1f4.bit.LA_IRSUB_BWET    = pLaIrSubInfo->uiLaIrSubBWet;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R1F4_STCS_LA_IR_OFS, reg1f4.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_setCaTh(UINT32 id, SIE_CA_TH_INFO *pCaTh)
{
	T_R22C_STCS_CA reg22C;
	T_R230_STCS_CA reg230;
	T_R234_STCS_CA reg234;
	T_R238_STCS_CA reg238;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg22C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R22C_STCS_CA_OFS);
	reg230.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R230_STCS_CA_OFS);
	reg234.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R234_STCS_CA_OFS);
	reg238.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R238_STCS_CA_OFS);

	reg22C.bit.CA_TH_G_LOWER    = pCaTh->uiGThLower;
	reg22C.bit.CA_TH_G_UPPER    = pCaTh->uiGThUpper;

	reg230.bit.CA_TH_RG_LOWER    = pCaTh->uiRGThLower;
	reg230.bit.CA_TH_RG_UPPER    = pCaTh->uiRGThUpper;

	reg234.bit.CA_TH_BG_LOWER    = pCaTh->uiBGThLower;
	reg234.bit.CA_TH_BG_UPPER    = pCaTh->uiBGThUpper;

	reg238.bit.CA_TH_PG_LOWER    = pCaTh->uiPGThLower;
	reg238.bit.CA_TH_PG_UPPER    = pCaTh->uiPGThUpper;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R22C_STCS_CA_OFS, reg22C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R230_STCS_CA_OFS, reg230.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R234_STCS_CA_OFS, reg234.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R238_STCS_CA_OFS, reg238.reg);
	sie_platform_spin_unlock(0,spin_flags);
}


void sie_setCaWin(UINT32 id, SIE_CA_WIN_INFO *pSetting)
{
	T_R23C_STCS_CA reg23C;
	T_R240_STCS_CA reg240;
	//static UINT32 flag=0;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	//nvt_dbg(IND,"%d/%d/%d/%d\r\n",g_uiCaWinSzfifo[id][4],g_uiCaWinSzfifo[id][5],g_uiCaWinSzfifo[id][6],g_uiCaWinSzfifo[id][7]);
	reg23C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R23C_STCS_CA_OFS);
	reg240.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R240_STCS_CA_OFS);

	reg23C.bit.CA_WIN_SZX    = pSetting->uiWinSzX;
	reg23C.bit.CA_WIN_SZY    = pSetting->uiWinSzY;

	reg240.bit.CA_WIN_NUMX    = pSetting->uiWinNmX - 1;
	reg240.bit.CA_WIN_NUMY    = pSetting->uiWinNmY - 1;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R23C_STCS_CA_OFS, reg23C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R240_STCS_CA_OFS, reg240.reg);
	sie_platform_spin_unlock(0,spin_flags);
#if 0
	if (flag == 0)
    {
		nvt_dbg(IND,"drv ca win sz %d/%d\r\n",pSetting->uiWinSzX,pSetting->uiWinSzY);
		flag = 1;
    }
#endif
}


void sie_getCaWin(UINT32 id, SIE_CA_WIN_INFO *pSetting)
{
	T_R23C_STCS_CA reg23C;
	T_R240_STCS_CA reg240;

	reg23C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R23C_STCS_CA_OFS);
	reg240.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R240_STCS_CA_OFS);

	pSetting->uiWinSzX = reg23C.bit.CA_WIN_SZX;
	pSetting->uiWinSzY = reg23C.bit.CA_WIN_SZY;

	pSetting->uiWinNmX = reg240.bit.CA_WIN_NUMX + 1;
	pSetting->uiWinNmY = reg240.bit.CA_WIN_NUMY + 1;
}

void sie_setLaCrp(UINT32 id, SIE_LA_CROP_INFO *pSetting)
{
	T_R248_STCS_LA reg248;
	T_R24C_STCS_LA reg24C;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg248.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R248_STCS_LA_OFS);
	reg24C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R24C_STCS_LA_OFS);

	reg248.bit.LA_CROP_STX    = pSetting->uiStX;
	reg248.bit.LA_CROP_STY    = pSetting->uiStY;

	reg24C.bit.LA_CROP_SZX    = pSetting->uiSzX;
	reg24C.bit.LA_CROP_SZY    = pSetting->uiSzY;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R248_STCS_LA_OFS, reg248.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R24C_STCS_LA_OFS, reg24C.reg);
	sie_platform_spin_unlock(0,spin_flags);
}


void sie_setLaGma1(UINT32 id, SIE_LA_GMA_INFO *pSetting)
{
	UINT32 i, uiBuf;
	unsigned long spin_flags;

	//unsigned long myflags;
	if (pSetting->puiGmaTbl == NULL) {
		return;
	}

	spin_flags=sie_platform_spin_lock(0);
	for (i = 0; i < 63; i += 3) {
		uiBuf  = (pSetting->puiGmaTbl[i + 0] & 0x3ff) << 0;
		uiBuf |= (pSetting->puiGmaTbl[i + 1] & 0x3ff) << 10;
		uiBuf |= (pSetting->puiGmaTbl[i + 2] & 0x3ff) << 20;
		SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R250_STCS_GAMMA_OFS + (i * 4 / 3), uiBuf);
	}
	// the last one
	uiBuf  = (pSetting->puiGmaTbl[63] & 0x3ff) << 0;
	uiBuf |= (pSetting->puiGmaTbl[64] & 0x3ff) << 10;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R250_STCS_GAMMA_OFS + (i * 4 / 3), uiBuf);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setLaCg(UINT32 id, SIE_LA_CG_INFO *pSetting)
{
	T_R1EC_STCS_LA_CG reg1EC;
	T_R1F0_STCS_LA_CG reg1F0;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg1EC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1EC_STCS_LA_CG_OFS);
	reg1F0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1F0_STCS_LA_CG_OFS);
	reg1EC.bit.LA_CG_RGAIN = pSetting->uiRGain;
	reg1EC.bit.LA_CG_GGAIN = pSetting->uiGGain;
	reg1F0.bit.LA_CG_BGAIN = pSetting->uiBGain;
	reg1F0.bit.LA_CG_IRGAIN = pSetting->uiIRGain;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1EC_STCS_LA_CG_OFS, reg1EC.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1F0_STCS_LA_CG_OFS, reg1F0.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
void sie_setLaTh(UINT32 id, SIE_LA_TH *pSetting)
{
	T_R2B4_STCS_LA reg2b4;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg2b4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R2B4_STCS_LA_OFS);
	reg2b4.bit.LA_TH_Y1_LOWER = pSetting->uiLaThY1Lower;
	reg2b4.bit.LA_TH_Y1_UPPER = pSetting->uiLaThY1Upper;
	reg2b4.bit.LA_TH_Y2_LOWER = pSetting->uiLaThY2Lower;
	reg2b4.bit.LA_TH_Y2_UPPER = pSetting->uiLaThY2Upper;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R2B4_STCS_LA_OFS, reg2b4.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

void sie_setLaWin(UINT32 id, SIE_LA_WIN_INFO *pSetting)
{
	T_R2AC_STCS_LA reg2AC;
	T_R2B0_STCS_LA reg2B0;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	reg2AC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2AC_STCS_LA_OFS);//no need//
	reg2B0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2B0_STCS_LA_OFS);//no need//

	reg2AC.bit.LA_WIN_SZX    = pSetting->uiWinSzX;
	reg2AC.bit.LA_WIN_SZY    = pSetting->uiWinSzY;

	reg2B0.bit.LA_WIN_NUMX    = pSetting->uiWinNmX - 1;
	reg2B0.bit.LA_WIN_NUMY    = pSetting->uiWinNmY - 1;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R2AC_STCS_LA_OFS, reg2AC.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R2B0_STCS_LA_OFS, reg2B0.reg);
	sie_platform_spin_unlock(0,spin_flags);
}


void sie_getLaWin(UINT32 id, SIE_LA_WIN_INFO *pSetting)
{
	T_R2AC_STCS_LA reg2AC;
	T_R2B0_STCS_LA reg2B0;

	reg2AC.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2AC_STCS_LA_OFS);
	reg2B0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2B0_STCS_LA_OFS);

	pSetting->uiWinSzX = reg2AC.bit.LA_WIN_SZX;
	pSetting->uiWinSzY =  reg2AC.bit.LA_WIN_SZY;

	pSetting->uiWinNmX = reg2B0.bit.LA_WIN_NUMX + 1;
	pSetting->uiWinNmY = reg2B0.bit.LA_WIN_NUMY + 1;
}

UINT32 sie_getLaWinSum(UINT32 id)
{
	UINT32 uiExp, uiDat;
	T_R2B0_STCS_LA reg2B0;

	reg2B0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2B0_STCS_LA_OFS);

	uiExp = reg2B0.bit.LA1_WIN_SUM & 0x3f;
	uiDat = (reg2B0.bit.LA1_WIN_SUM >> 6) & 0xfff;

	return uiDat << uiExp;
}

UINT32 sie_getOutAdd(UINT32 id, UINT32 uiOutChIdx, SIE_PINGPONG_BUFF_SEL uiBufSel)
{
	UINT32 uiPPBAddr = 0, uiOutAddr = 0;

	if (uiOutChIdx == 0) {
		SIE_DRAM_OUT0_INFO DOut0;
		sie_getDramOut0(id, &DOut0);
		uiPPBAddr = DOut0.uiAddr;
	} else if (uiOutChIdx == 1) {
		SIE_DRAM_OUT1_INFO DOut1;
		sie_getDramOut1(id, &DOut1);
		uiPPBAddr = DOut1.uiAddr;
	} else if (uiOutChIdx == 2) {
		SIE_DRAM_OUT2_INFO DOut2;
		sie_getDramOut2(id, &DOut2);
		uiPPBAddr = DOut2.uiAddr;
	} else {
		nvt_dbg(ERR,"uiOutChIdx %d out of range, set to Ch0 !!\r\n", (int)uiOutChIdx);
	}

	uiOutAddr = uiPPBAddr;// + uiPPBIdx * uiFramOfSet;
	return uiOutAddr;
}

UINT32 sie_getOutAdd_Flip(UINT32 id, UINT32 uiOutChIdx, SIE_PINGPONG_BUFF_SEL uiBufSel)
{
	UINT32 uiPPBAddr[3] = {0}; //, uiPPBNum=0, uiPPBIdx=0;

	if (uiOutChIdx == 0) {
		SIE_DRAM_OUT0_INFO DOut0;
		sie_getDramOut0Flip(id, &DOut0);
		uiPPBAddr[0] = DOut0.uiAddr;
	} else if (uiOutChIdx == 1) {
		SIE_DRAM_OUT1_INFO DOut1;
		sie_getDramOut1Flip(id, &DOut1);
		uiPPBAddr[0] = DOut1.uiAddr;
	} else if (uiOutChIdx == 2) {
		SIE_DRAM_OUT2_INFO DOut2;
		sie_getDramOut2(id, &DOut2);
		uiPPBAddr[0] = DOut2.uiAddr;
	} 

	return uiPPBAddr[0];
}

void sie_getCAResult(UINT32 id, SIE_STCS_CA_RSLT_INFO *CaRsltInfo)
{
	SIE_CA_WIN_INFO CaWinInfo = {0};
	UINT32 uiBuffAddr;
	//static UINT32 cnt;

	sie_getCaWin(id, &CaWinInfo);
	uiBuffAddr = sie_getOutAdd(id, 1, PINGPONG_BUFF_LAST);
	#if 0
	if (cnt<5)
	{
		nvt_dbg(IND, "cnt = %d, uiBufaddr = 0x%08x\r\n",cnt,uiBuffAddr);
		cnt++;
	}
	#endif
	if (uiBuffAddr != 0) {
		if (CaWinInfo.uiWinSzX !=0 && CaWinInfo.uiWinSzY!=0)
			sie_getCAResultManual(id, CaRsltInfo, &CaWinInfo, uiBuffAddr);
		else
			nvt_dbg(WRN, "WinSzX or WinSzY = 0\r\n");
	} else {
	}
}

//NA//
#if 0
void sie1_getCAFlipResult(UINT16 *puiBufR, UINT16 *puiBufG, UINT16 *puiBufB, UINT16 *puiAccCnt, SIE_FLIP_SEL pFlipSel)
{
	SIE_CA_WIN_INFO CaWinInfo = {0};
	UINT32 uiBuffAddr;
	sie1_getCaWin(&CaWinInfo);
	uiBuffAddr = sie1_getOutAdd_Flip(1, PINGPONG_BUFF_LAST);
	sie1_getCAFlipResultManual(puiBufR, puiBufG, puiBufB, puiAccCnt, &CaWinInfo, uiBuffAddr, pFlipSel);
}
#endif
//NA//

void sie_getSatgainInfo(UINT32 id, UINT32 *g_satgain)
{
    if (sie_getFunction(id)&SIE_RGBIR_FORMAT_SEL)
		*g_satgain = satgen_g_cur[id];
	else
		*g_satgain = 256;//1x for rggb sensor
}

///////// OUTPUT FORMAT /////////
//
// CA_TH_EN == 0
//    BAYER_FORMAT == BAYER
//      => HW output: R/G/B/0/CNT
//      => SW output: *puiBufR = R, *puiBufG = G, *puiBufB = B, *puiBufIR = 0, *puiAccCnt = CNT
//    BAYER_FORMAT == RGB-IR
//      => HW output: R/G/B/IR/CNT
//      => SW output: *puiBufR = R, *puiBufG = G, *puiBufB = B, *puiBufIR = IR, *puiAccCnt = CNT
// CA_TH_EN == 1
//    BAYER_FORMAT == BAYER
//      => HW output: Rth/Gth/Bth/0/CNT
//      => SW output: *puiBufR = Rth, *puiBufG = Gth, *puiBufB = Bth, *puiBufIR = 0, *puiAccCnt = CNT
//    BAYER_FORMAT == RGB-IR
//      => HW output: Rth/Gth/Bth/IRth/CNT
//      => SW output: *puiBufR = Rth, *puiBufG = Gth, *puiBufB = Bth, *puiAccCnt = CNT
//
////////////////////////////////////
void sie_getCAResultManual(UINT32 id, SIE_STCS_CA_RSLT_INFO *CaRsltInfo, SIE_CA_WIN_INFO *CaWinInfo, UINT32 uiBuffAddr)
{
	UINT32 i, uiBuf;
	UINT32 uiWinNum;
	UINT32 uiR, uiG, uiB, uiN, uiIR = 0;
	//UINT32 uiRth = 0, uiGth = 0, uiBth = 0;
	UINT32 uiOutChNum;
	BOOL bIrFmt = FALSE;//, bCaTh = FALSE;
	SIE_STCS_PATH_INFO StcsPathInfo = {0};
	SIE_STCS_OB_INFO StcsObInfo = {0};
	SIE_CG_INFO cgainInfo = {0};
	UINT32 uiCaSubOb=0;
	UINT32 RoundBase=0, shift = 0;
	UINT16 tmpr=0,tmpr2=0,tmpg=0,tmpg2=0,tmpb=0,tmpb2=0,tmpir=0;
	UINT32 sum_g=0,sum_g_sub=0;

	sie_getStcsPath(id, &StcsPathInfo);
	sie_getStcsOb(id, &StcsObInfo);
	
	uiOutChNum = 10; // HW output format: Rth/Gth/Bth/0/Cnt or Rth/Gth/Bth/Irth/Cnt, total 5 channel * 2 bytes

	sie_platform_dma_flush_dev2mem(uiBuffAddr, ((CaWinInfo->uiWinNmX*16*5+31)/32)*4*CaWinInfo->uiWinNmY,0);

	
	if (sie_getFunction(id)&SIE_RGBIR_FORMAT_SEL) {
		bIrFmt = TRUE;
	}

	uiWinNum = CaWinInfo->uiWinNmX * CaWinInfo->uiWinNmY;
	//uiWinSz = g_uiCaWinSzfifo[id][0]*g_uiCaWinSzfifo[id][1];//CaWinInfo->uiWinSzX * CaWinInfo->uiWinSzY;

	//uiN = uiWinSz;

	//if ((StcsObInfo.uiCaObOfs == 0) && (StcsObInfo.uiLaObOfs!=0)) // it's no need after IQ change flow
	uiCaSubOb = StcsObInfo.uiLaObOfs;

	sie_getCGain(id, &cgainInfo);

	if (cgainInfo.bGainSel) { // iColorGainSel = 1;
		RoundBase = 64;
		shift = 7;
    }else {             // iColorGainSel= 0
		RoundBase = 128;
		shift = 8;
    }

	for (i = 0; i < uiWinNum; i++) {
		uiBuf = *(volatile UINT32 *)(uiBuffAddr + (i * uiOutChNum));
		uiR = ((uiBuf >> 3) & 0x1fff) << ((uiBuf) & 0x7);
		uiG = ((uiBuf >>19) & 0x1fff) << ((uiBuf >> 16) & 0x7);
		uiBuf = *(volatile UINT32 *)(uiBuffAddr + (i * uiOutChNum) + 4);
		uiB = ((uiBuf >> 3) & 0x1fff) << ((uiBuf) & 0x7);
		if (bIrFmt) {
			uiIR = ((uiBuf >> 19) & 0x1fff) << ((uiBuf >> 16) & 0x7); // Irth or Ir
		} else {
			uiIR = 0;
		}
		uiBuf = *(volatile UINT32 *)(uiBuffAddr + (i * uiOutChNum) + 8);
		uiN = (uiBuf & 0x7f);

		*(CaRsltInfo->puiAccCnt + i) = uiN;

        //nvt_dbg(IND, "%d/%d/%d/%d/%d\r\n",uiR, uiG, uiB, uiIR, uiN);
		//nvt_dbg(IND, "R/G/B addr: 0x%08x/0x%08x/0x%08x\r\n",(UINT32)CaRsltInfo->puiBufR,(UINT32)CaRsltInfo->puiBufG,(UINT32)CaRsltInfo->puiBufB);

        if (uiN!=0) {
			if (nvt_get_chip_id() == CHIP_NA51055) { //520
				if (sie_getFunction(id)&SIE_COMPANDING_EN) {
					tmpr  = CLAMP((uiR  / uiN),uiCaSubOb,4095)-uiCaSubOb;
					tmpg  = CLAMP((uiG  / uiN),uiCaSubOb,4095)-uiCaSubOb;
					tmpb  = CLAMP((uiB  / uiN),uiCaSubOb,4095)-uiCaSubOb;
					tmpir = CLAMP((uiIR / uiN),uiCaSubOb,4095)-uiCaSubOb;
				}else {
					tmpr  = CLAMP((((CLAMP((uiR  / uiN),uiCaSubOb,4095)-uiCaSubOb)*cgainInfo.uiRGain + RoundBase)>>shift),0,4095);
					tmpg  = CLAMP((((CLAMP((uiG  / uiN),uiCaSubOb,4095)-uiCaSubOb)*cgainInfo.uiRGain + RoundBase)>>shift),0,4095);
					tmpb  = CLAMP((((CLAMP((uiB  / uiN),uiCaSubOb,4095)-uiCaSubOb)*cgainInfo.uiRGain + RoundBase)>>shift),0,4095);
					tmpir = CLAMP((((CLAMP((uiIR  / uiN),uiCaSubOb,4095)-uiCaSubOb)*cgainInfo.uiRGain + RoundBase)>>shift),0,4095);
				}
			}else {//528
				tmpr  = CLAMP((uiR  / uiN),uiCaSubOb,4095)-uiCaSubOb;
				tmpg  = CLAMP((uiG  / uiN),uiCaSubOb,4095)-uiCaSubOb;
				tmpb  = CLAMP((uiB  / uiN),uiCaSubOb,4095)-uiCaSubOb;
				tmpir = CLAMP((uiIR / uiN),uiCaSubOb,4095)-uiCaSubOb;
			}

			*(CaRsltInfo->puiBufR + i) = tmpr;
			*(CaRsltInfo->puiBufG + i) = tmpg;
			*(CaRsltInfo->puiBufB + i) = tmpb;
			*(CaRsltInfo->puiBufIR + i)= tmpir;

			if(bIrFmt)
			{
				sum_g+= tmpg;
				tmpr2 = (irsub_r[id]*tmpir+128)>>8;
				tmpg2 = (irsub_g[id]*tmpir+128)>>8;
				tmpb2 = (irsub_b[id]*tmpir+128)>>8;
				if (tmpr>tmpr2)
					*(CaRsltInfo->puiBufR + i) = CLAMP(tmpr - tmpr2,0,4095);
				else
					*(CaRsltInfo->puiBufR + i) = 0;
				if (tmpg>tmpg2)
					*(CaRsltInfo->puiBufG + i) = CLAMP(tmpg - tmpg2,0,4095);
				else
					*(CaRsltInfo->puiBufG + i) = 0;
				if (tmpb>tmpb2)
					*(CaRsltInfo->puiBufB + i) = CLAMP(tmpb - tmpb2,0,4095);
				else
					*(CaRsltInfo->puiBufB + i) = 0;

				*(CaRsltInfo->puiBufIR + i)= tmpir;

				sum_g_sub+= (*(CaRsltInfo->puiBufG + i));
			}

        } else {
            *(CaRsltInfo->puiBufR + i) = 0;
			*(CaRsltInfo->puiBufG + i) = 0;
			*(CaRsltInfo->puiBufB + i) = 0;
			*(CaRsltInfo->puiBufIR + i) = 0;
        }
	}

    if ((uiWinNum>0) && (bIrFmt==1) && (sum_g_sub>0))
    {
		sum_g = sum_g/uiWinNum; // average
		sum_g_sub = sum_g_sub/uiWinNum; // average
		if (sum_g_sub>0)
			satgen_g_cur[id] = (sum_g<<8)/sum_g_sub;
		else
			satgen_g_cur[id] = 0;
		satgen_g_cur[id] = (satgen_g_last[id]*15+satgen_g_cur[id])>>4;
		//nvt_dbg(ERR, "gain_ratio_cur = %d\r\n",gain_ratio_cur);
		//gain_ratio_cur = gain_ratio_cur&0x3FF;
		if (satgen_g_cur[id] > 1023) {
			satgen_g_cur[id] = 1023;
		}
		//nvt_dbg(ERR, "gain_ratio_cur.= %d\r\n",gain_ratio_cur);
		satgen_g_last[id] = satgen_g_cur[id];
    }
}
//NA//
#if 0
void sie1_getCAFlipResultManual(UINT16 *puiBufR, UINT16 *puiBufG, UINT16 *puiBufB, UINT16 *puiAccCnt, SIE_CA_WIN_INFO *CaWinInfo, UINT32 uiBuffAddr, SIE_FLIP_SEL pFlipSel)
{
	UINT32 i, j, uiBuf;
	//UINT32 uiWinNum;
	UINT32 uiR, uiG, uiB, uiN;
	UINT32 Outidx = 0;
	//uiWinNum = CaWinInfo->uiWinNmX*CaWinInfo->uiWinNmY;

	for (j = 0; j < CaWinInfo->uiWinNmY; j++) {
		for (i = 0; i < CaWinInfo->uiWinNmX; i++) {
			uiBuf = *(volatile UINT32 *)(uiBuffAddr + ((j * CaWinInfo->uiWinNmX + i) * 8));
			uiR = ((uiBuf >> 3) & 0x1fff) << ((uiBuf) & 0x7);
			uiG = ((uiBuf >> 19) & 0x1fff) << ((uiBuf >> 16) & 0x7);
			uiBuf = *(volatile UINT32 *)(uiBuffAddr + ((j * CaWinInfo->uiWinNmX + i) * 8) + 4);
			uiB = ((uiBuf >> 3) & 0x1fff) << ((uiBuf) & 0x7);
			uiN = ((uiBuf >> 16) &  0xff);

			if (pFlipSel == SIE_NO_FLIP) {
				Outidx = j                        * CaWinInfo->uiWinNmX + i;
			}
			if (pFlipSel == SIE_H_FLIP) {
				Outidx = j                        * CaWinInfo->uiWinNmX + (CaWinInfo->uiWinNmX - i - 1);
			}
			if (pFlipSel == SIE_V_FLIP) {
				Outidx = (CaWinInfo->uiWinNmY - j - 1) * CaWinInfo->uiWinNmX + i;
			}
			if (pFlipSel == SIE_HV_FLIP) {
				Outidx = (CaWinInfo->uiWinNmY - j - 1) * CaWinInfo->uiWinNmX + (CaWinInfo->uiWinNmX - i - 1);
			}

			*(puiAccCnt + Outidx) = uiN;
			if (uiN != 0) {
				*(puiBufR + Outidx) = uiR / uiN;
				*(puiBufG + Outidx) = uiG / uiN;
				*(puiBufB + Outidx) = uiB / uiN;
			} else {
				*(puiBufR + Outidx) = 0;
				*(puiBufG + Outidx) = 0;
				*(puiBufB + Outidx) = 0;
			}
		}
	}
}
#endif
//NA//

void sie_getHisto(UINT32 id, UINT16 *puiHisto)
{
#define LA_HIST_BIN_NUM (128)
#define LA_HIST_OUT_BASE_BIT (16)
	UINT32 uiTotalCnt = 0, uiCnt;
	UINT16 uiCntArray[128];
	UINT32 i, uiBuf, uiDat;
	UINT32 cnt = 0/*, cnt2 = 0*/;
	UINT32 uiMaxBin=0,uiMaxVal=0;
	
	if (puiHisto == NULL) {
		return;
	}
	//nvt_dbg(ERR,"%d/0x%08x/0x%08x\r\n",id,_SIE_REG_BASE_ADDR_SET[id],SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],0x330));

    if ((sie_getFunction(id)&SIE_STCS_HISTO_Y_EN) == 0) {
		return;
    }
	uiTotalCnt = 0;
	for (i = 0; i < 128; i += 2) {
		uiBuf = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2B8_STCS_HISTO_OFS + (i << 1));
		uiCnt = (uiBuf) & 0xffff;
		uiTotalCnt += uiCnt;
		uiCntArray[i + 0] = uiCnt;
		uiCnt = (uiBuf >> 16) & 0xffff;
		uiTotalCnt += uiCnt;
		uiCntArray[i + 1] = uiCnt;
	}
	

	if (uiTotalCnt == 0) {
//		nvt_dbg(ERR,"Total Count = 0 !!");
		uiTotalCnt = 1;
	}

	uiMaxBin = 0;
	uiMaxVal = 0;
	for (i = 0; i < 128; i++) {
		uiDat = ((uiCntArray[i] << LA_HIST_OUT_BASE_BIT) + 32768) / uiTotalCnt;
		if (uiDat > 0xffff) {
			uiDat = 0xffff;
		}
		*(puiHisto + i) = uiDat;
		cnt+= uiDat;
		if (uiDat > uiMaxVal) {
			uiMaxVal = uiDat;
			uiMaxBin = i;
		}
	}
	if (cnt!= 65535) {
		if (cnt > 65535)
			*(puiHisto + uiMaxBin) = uiMaxVal - (cnt - 65535);
		else
			*(puiHisto + uiMaxBin) = uiMaxVal + (65535 - cnt);
	}
	#if 0
	if (cnt!= 65535) {
		nvt_dbg(ERR, "11 cnt = %d, uiMaxVal = %d\r\n",cnt, uiMaxVal);
		cnt2 = 0;
		for (i = 0; i < 128; i++) {
			cnt2+= *(puiHisto + i);
		}
		if (cnt2!= 65535)
			nvt_dbg(ERR, "22 cnt = %d, cnt2 = %d\r\n",cnt, cnt2);
	}
	#endif
	
}
void sie_getMdResult(UINT32 id, SIE_MD_RESULT_INFO *pMdInfo)
{
	T_R3C8_STCS_MD reg3c8;
	T_R3CC_STCS_MD reg3cc;
	UINT32 tmp;
	UINT32 i,j;

	reg3c8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R3C8_STCS_MD_OFS);//no need//
	reg3cc.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R3CC_STCS_MD_OFS);//no need//

	pMdInfo->uiBlkDiffCnt   = reg3c8.bit.MD_BLKDIFF_CNT;
	pMdInfo->uiTotalBlkDiff = reg3cc.bit.MD_TOTAL_BLKDIFF;

	for (i = 0; i < 32; i++) {
		tmp = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R3E4_MD_OFS + i * 4);
		for (j = 0; j < 32; j++) {
			pMdInfo->pbMdThresRslt[i*32+j] = (tmp>>j)&0x1;
		}
	}
	#if 0
	for (i = 0; i < 4; i++) {
		tmp = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R3E4_MD_OFS + i * 4);
		for (j = 0; j < 32; j++) {
			nvt_dbg(ERR, "%d ",pMdInfo->pbMdThresRslt[i*32+j]);
		}
		nvt_dbg(ERR, "\r\n");
	}
	nvt_dbg(ERR, "\r\n");
	nvt_dbg(ERR, "\r\n");
	#endif
}

void sie_getLAResult(UINT32 id, UINT16 *puiBufLa1, UINT16 *puiBufLa2)
{
	SIE_LA_WIN_INFO LaWinInfo = {0};
	UINT32 uiBuffAddr;
	sie_getLaWin(id, &LaWinInfo);
	uiBuffAddr = sie_getOutAdd(id, 2, PINGPONG_BUFF_LAST);
	if (uiBuffAddr != 0) {
		if (LaWinInfo.uiWinSzX !=0 && LaWinInfo.uiWinSzY!=0)
			sie_getLAResultManual(id, puiBufLa1, puiBufLa2, &LaWinInfo, uiBuffAddr);
		else
			nvt_dbg(WRN, "WinSzX or WinSzY = 0\r\n");
	} else {
	}
}

//NA//
#if 0
void sie1_getLAFlipResult(UINT16 *puiBufLa1, UINT16 *puiBufLa2, SIE_FLIP_SEL pFlipSel)
{
	SIE_LA_WIN_INFO LaWinInfo = {0};
	UINT32 uiBuffAddr;
	sie1_getLaWin(&LaWinInfo);
	uiBuffAddr = sie1_getOutAdd_Flip(2, PINGPONG_BUFF_LAST);
	sie1_getLAFlipResultManual(puiBufLa1, puiBufLa2, &LaWinInfo, uiBuffAddr, pFlipSel);
}
#endif
//NA//

void sie_getLAResultManual(UINT32 id, UINT16 *puiBufLa1, UINT16 *puiBufLa2, SIE_LA_WIN_INFO *LaWinInfo, UINT32 uiBuffAddr)
{
	UINT32 i, uiBuf;
	UINT32 uiWinNum, uiWinBas;
	UINT32 uiLa1, uiLa2;

	uiWinNum = LaWinInfo->uiWinNmX * LaWinInfo->uiWinNmY;
	uiWinBas = LaWinInfo->uiWinSzX * LaWinInfo->uiWinSzY;
	sie_platform_dma_flush_dev2mem(uiBuffAddr,((LaWinInfo->uiWinNmX*16*2+31)/32)*4*LaWinInfo->uiWinNmY,0);
	for (i = 0; i < uiWinNum; i++) {
		uiBuf = *(volatile UINT32 *)(uiBuffAddr + (i << 2));
		uiLa1 = ((uiBuf >> 3) & 0x1fff) << ((uiBuf) & 0x7);
		if (puiBufLa1!=NULL)
			*(puiBufLa1 + i)  = uiLa1 / uiWinBas;
		uiLa2 = ((uiBuf >> 19) & 0x1fff) << ((uiBuf >> 16) & 0x7);
		if (puiBufLa2!=NULL)
			*(puiBufLa2 + i)  = uiLa2 / uiWinBas;
	}
}

//NA//
#if 0
void sie1_getLAFlipResultManual(UINT16 *puiBufLa1, UINT16 *puiBufLa2, SIE_LA_WIN_INFO *LaWinInfo, UINT32 uiBuffAddr, SIE_FLIP_SEL pFlipSel)
{
	UINT32 i, j, uiBuf;
	//UINT32 uiWinNum;
	UINT32 uiWinBas;
	UINT32 uiLa1, uiLa2;
	INT32 Outidx = 0, OutidxX = 0;
	INT32  Outidx_yofs = 0, Outidx_xofs = 0;


	//uiWinNum = LaWinInfo->uiWinNmX*LaWinInfo->uiWinNmY;
	uiWinBas = LaWinInfo->uiWinSzX * LaWinInfo->uiWinSzY;

	if (pFlipSel == SIE_NO_FLIP) {
		Outidx = 0; // start pointer location
		Outidx_yofs = LaWinInfo->uiWinNmX;
		Outidx_xofs = 1;
	} else if (pFlipSel == SIE_H_FLIP) {
		Outidx = LaWinInfo->uiWinNmX - 1;
		Outidx_yofs = LaWinInfo->uiWinNmX;
		Outidx_xofs = -1;
	} else if (pFlipSel == SIE_V_FLIP) {
		Outidx = (LaWinInfo->uiWinNmY - 1) * LaWinInfo->uiWinNmX;
		Outidx_yofs = -LaWinInfo->uiWinNmX;
		Outidx_xofs = 1;
	} else if (pFlipSel == SIE_HV_FLIP) {
		Outidx = (LaWinInfo->uiWinNmY - 1) * LaWinInfo->uiWinNmX + (LaWinInfo->uiWinNmX - 1);
		Outidx_yofs = -LaWinInfo->uiWinNmX;
		Outidx_xofs = -1;
	}

	for (j = 0; j < LaWinInfo->uiWinNmY; j++) {
		OutidxX = 0;
		for (i = 0; i < LaWinInfo->uiWinNmX; i++) {
			uiBuf = *(volatile UINT32 *)(uiBuffAddr + ((j * LaWinInfo->uiWinNmX + i) << 2));

			//if ((puiBufLa1+(LaWinInfo->uiWinNmY-j-1)*LaWinInfo->uiWinNmX+(LaWinInfo->uiWinNmX-i-1))!=(puiBufLa1+Outidx+OutidxX))
			//{
			//    DBG_DUMP("MISMATCH!! 0x%08x 0x%08x\r\n",puiBufLa1+(LaWinInfo->uiWinNmY-j-1)*LaWinInfo->uiWinNmX+(LaWinInfo->uiWinNmX-i-1), puiBufLa1+Outidx+OutidxX);
			//}

			uiLa1 = ((uiBuf >> 3) & 0x1fff) << ((uiBuf) & 0x7);
			*(puiBufLa1 + Outidx + OutidxX)  = uiLa1 / uiWinBas;
			uiLa2 = ((uiBuf >> 19) & 0x1fff) << ((uiBuf >> 16) & 0x7);
			*(puiBufLa2 + Outidx + OutidxX)  = uiLa2 / uiWinBas;

			OutidxX += Outidx_xofs;
		}
		Outidx += Outidx_yofs;
	}
}
#endif

void sie_setChgRingBuf(UINT32 id, SIE_CHG_RINGBUF_INFO *pChgRingBuf)
{
	SIE_DRAM_OUT0_INFO DOut0Info = {0};
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(1);
	sie_getDramOut0(id, &DOut0Info);
	DOut0Info.bRingBufEn= pChgRingBuf->ringbufEn;
	DOut0Info.uiRingBufLen= pChgRingBuf->ringbufLen;
	sie_setDramOut0(id, &DOut0Info);
	sie_platform_spin_unlock(1,spin_flags);
}

void sie_setChgInAddr(UINT32 id, SIE_CHG_IN_ADDR_INFO *pChgIoAddrInfo)
{
	unsigned long spin_flags;
	spin_flags=sie_platform_spin_lock(1);
	if (pChgIoAddrInfo->uiIn0Addr != 0) {
		SIE_DRAM_IN0_INFO DIn0Info = {0};
		sie_getDramIn0(id, &DIn0Info);
		DIn0Info.uiAddr = pChgIoAddrInfo->uiIn0Addr;
		sie_setDramIn0(id, &DIn0Info);
	}
	if (pChgIoAddrInfo->uiIn1Addr != 0) {
		SIE_DRAM_IN1_INFO DIn1Info = {0};
		DIn1Info.uiAddr = pChgIoAddrInfo->uiIn1Addr;
		sie_setDramIn1(id, &DIn1Info);
	}
	if (pChgIoAddrInfo->uiIn2Addr != 0) {
		SIE_DRAM_IN2_INFO DIn2Info = {0};
		DIn2Info.uiAddr = pChgIoAddrInfo->uiIn2Addr;
		sie_setDramIn2(id, &DIn2Info);
	}
	sie_platform_spin_unlock(1,spin_flags);
}

void sie_setChgOutAddr(UINT32 id, SIE_CHG_OUT_ADDR_INFO *pChgIoAddrInfo)
{
	unsigned long spin_flags;
	spin_flags=sie_platform_spin_lock(1);
	if (pChgIoAddrInfo->uiOut0Addr != 0) {
		SIE_DRAM_OUT0_INFO DOut0Info = {0};
		sie_getDramOut0(id, &DOut0Info);
		DOut0Info.uiAddr = pChgIoAddrInfo->uiOut0Addr;
		sie_setDramOut0(id, &DOut0Info);
	}
	if (pChgIoAddrInfo->uiOut1Addr != 0) {
		SIE_DRAM_OUT1_INFO DOut1Info = {0};
		sie_getDramOut1(id, &DOut1Info);
		DOut1Info.uiAddr = pChgIoAddrInfo->uiOut1Addr;
		sie_setDramOut1(id, &DOut1Info);
	}
	if (pChgIoAddrInfo->uiOut2Addr != 0) {
		SIE_DRAM_OUT2_INFO DOut2Info = {0};
		sie_getDramOut2(id, &DOut2Info);
		DOut2Info.uiAddr = pChgIoAddrInfo->uiOut2Addr;
		sie_setDramOut2(id, &DOut2Info);
	}
	sie_platform_spin_unlock(1,spin_flags);
}

void sie_setChgOutAddr_Flip(UINT32 id, SIE_CHG_OUT_ADDR_INFO *pChgIoAddrInfo)
{
	unsigned long spin_flags;
	spin_flags=sie_platform_spin_lock(1);
	if (pChgIoAddrInfo->uiOut0Addr != 0) {
		SIE_DRAM_OUT0_INFO DOut0Info = {0};
		sie_getDramOut0(id, &DOut0Info);
		DOut0Info.uiAddr = pChgIoAddrInfo->uiOut0Addr;
		sie_setDramOut0Flip(id, &DOut0Info);
	}
	if (pChgIoAddrInfo->uiOut1Addr != 0) {
		SIE_DRAM_OUT1_INFO DOut1Info = {0};
		sie_getDramOut1(id, &DOut1Info);
		DOut1Info.uiAddr = pChgIoAddrInfo->uiOut1Addr;
		sie_setDramOut1Flip(id, &DOut1Info);
	}
	if (pChgIoAddrInfo->uiOut2Addr != 0) {
		SIE_DRAM_OUT2_INFO DOut2Info = {0};
		sie_getDramOut2(id, &DOut2Info);
		DOut2Info.uiAddr = pChgIoAddrInfo->uiOut2Addr;
		sie_setDramOut2(id, &DOut2Info);
	}
	sie_platform_spin_unlock(1,spin_flags);
}

void sie_setChgIOLofs(UINT32 id, SIE_CHG_IO_LOFS_INFO *pChgIoLofsInfo)
{
	unsigned long spin_flags;
	
	spin_flags=sie_platform_spin_lock(1);

	if (pChgIoLofsInfo->uiOut0Lofs != 0) {
		SIE_DRAM_OUT0_INFO DOut0Info = {0};
		sie_getDramOut0(id, &DOut0Info);
		DOut0Info.uiLofs = pChgIoLofsInfo->uiOut0Lofs;
		sie_setDramOut0(id, &DOut0Info);
	}
	if (pChgIoLofsInfo->uiOut1Lofs != 0) {
		SIE_DRAM_OUT1_INFO DOut1Info = {0};
		sie_getDramOut1(id, &DOut1Info);
		DOut1Info.uiLofs = pChgIoLofsInfo->uiOut1Lofs;
		sie_setDramOut1(id, &DOut1Info);
	} 
	if (pChgIoLofsInfo->uiOut2Lofs != 0) {
		SIE_DRAM_OUT2_INFO DOut2Info = {0};
		sie_getDramOut2(id, &DOut2Info);
		DOut2Info.uiLofs = pChgIoLofsInfo->uiOut2Lofs;
		sie_setDramOut2(id, &DOut2Info);
	}
	sie_platform_spin_unlock(1,spin_flags);
}

void sie_setOutBitDepth(UINT32 id, SIE_PACKBUS_SEL ChgOut0PackBus)
{
	SIE_DRAM_OUT0_INFO DOut0Info = {0};
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(1);
	sie_getDramOut0(id, &DOut0Info);
	DOut0Info.PackBus = ChgOut0PackBus;
	sie_setDramOut0(id, &DOut0Info);
	sie_platform_spin_unlock(1,spin_flags);
}

void sie_setChgOutSrc(UINT32 id, SIE_CHG_OUT_SRC_INFO *pChgIoSrcInfo)
{
	unsigned long spin_flags;
	spin_flags=sie_platform_spin_lock(1);
	if (pChgIoSrcInfo->bOut0ChgEn) {
		SIE_DRAM_OUT0_INFO DOut0Info = {0};
		sie_getDramOut0(id, &DOut0Info);
		DOut0Info.OutSrc = pChgIoSrcInfo->Out0Src;
		sie_setDramOut0(id, &DOut0Info);
	}
	sie_platform_spin_unlock(1,spin_flags);
}

void sie_getGammaLut_Reg(UINT32 id, UINT32 *puiGmaLut)
{
	T_R250_STCS_GAMMA GmaReg0;
	T_R254_STCS_GAMMA GmaReg1;
	T_R258_STCS_GAMMA GmaReg2;
	T_R25C_STCS_GAMMA GmaReg3;

	T_R260_STCS_GAMMA GmaReg4;
	T_R264_STCS_GAMMA GmaReg5;
	T_R268_STCS_GAMMA GmaReg6;
	T_R26C_STCS_GAMMA GmaReg7;

	T_R270_STCS_GAMMA GmaReg8;
	T_R274_STCS_GAMMA GmaReg9;
	T_R278_STCS_GAMMA GmaReg10;
	T_R27C_STCS_GAMMA GmaReg11;

	T_R280_STCS_GAMMA GmaReg12;
	T_R284_STCS_GAMMA GmaReg13;
	T_R288_STCS_GAMMA GmaReg14;
	T_R28C_STCS_GAMMA GmaReg15;

	T_R290_STCS_GAMMA GmaReg16;
	T_R294_STCS_GAMMA GmaReg17;
	T_R298_STCS_GAMMA GmaReg18;
	T_R29C_STCS_GAMMA GmaReg19;

	T_R2A0_STCS_GAMMA GmaReg20;
	T_R2A4_STCS_GAMMA GmaReg21;

	GmaReg0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R250_STCS_GAMMA_OFS);
	GmaReg1.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R254_STCS_GAMMA_OFS);
	GmaReg2.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R258_STCS_GAMMA_OFS);
	GmaReg3.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R25C_STCS_GAMMA_OFS);
	puiGmaLut[0] = GmaReg0.bit.STCS_GAMMA_1_TBL0;
	puiGmaLut[1] = GmaReg0.bit.STCS_GAMMA_1_TBL1;
	puiGmaLut[2] = GmaReg0.bit.STCS_GAMMA_1_TBL2;

	puiGmaLut[3] = GmaReg1.bit.STCS_GAMMA_1_TBL3;
	puiGmaLut[4] = GmaReg1.bit.STCS_GAMMA_1_TBL4;
	puiGmaLut[5] = GmaReg1.bit.STCS_GAMMA_1_TBL5;

	puiGmaLut[6] = GmaReg2.bit.STCS_GAMMA_1_TBL6;
	puiGmaLut[7] = GmaReg2.bit.STCS_GAMMA_1_TBL7;
	puiGmaLut[8] = GmaReg2.bit.STCS_GAMMA_1_TBL8;

	puiGmaLut[9]  = GmaReg3.bit.STCS_GAMMA_1_TBL9;
	puiGmaLut[10] = GmaReg3.bit.STCS_GAMMA_1_TBL10;
	puiGmaLut[11] = GmaReg3.bit.STCS_GAMMA_1_TBL11;


	GmaReg4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R260_STCS_GAMMA_OFS);
	GmaReg5.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R264_STCS_GAMMA_OFS);
	GmaReg6.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R268_STCS_GAMMA_OFS);
	GmaReg7.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R26C_STCS_GAMMA_OFS);
	puiGmaLut[12] = GmaReg4.bit.STCS_GAMMA_1_TBL12;
	puiGmaLut[13] = GmaReg4.bit.STCS_GAMMA_1_TBL13;
	puiGmaLut[14] = GmaReg4.bit.STCS_GAMMA_1_TBL14;

	puiGmaLut[15] = GmaReg5.bit.STCS_GAMMA_1_TBL15;
	puiGmaLut[16] = GmaReg5.bit.STCS_GAMMA_1_TBL16;
	puiGmaLut[17] = GmaReg5.bit.STCS_GAMMA_1_TBL17;

	puiGmaLut[18] = GmaReg6.bit.STCS_GAMMA_1_TBL18;
	puiGmaLut[19] = GmaReg6.bit.STCS_GAMMA_1_TBL19;
	puiGmaLut[20] = GmaReg6.bit.STCS_GAMMA_1_TBL20;

	puiGmaLut[21] = GmaReg7.bit.STCS_GAMMA_1_TBL21;
	puiGmaLut[22] = GmaReg7.bit.STCS_GAMMA_1_TBL22;
	puiGmaLut[23] = GmaReg7.bit.STCS_GAMMA_1_TBL23;

	GmaReg8.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R270_STCS_GAMMA_OFS);
	GmaReg9.reg  = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R274_STCS_GAMMA_OFS);
	GmaReg10.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R278_STCS_GAMMA_OFS);
	GmaReg11.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R27C_STCS_GAMMA_OFS);
	puiGmaLut[24] = GmaReg8.bit.STCS_GAMMA_1_TBL24;
	puiGmaLut[25] = GmaReg8.bit.STCS_GAMMA_1_TBL25;
	puiGmaLut[26] = GmaReg8.bit.STCS_GAMMA_1_TBL26;

	puiGmaLut[27] = GmaReg9.bit.STCS_GAMMA_1_TBL27;
	puiGmaLut[28] = GmaReg9.bit.STCS_GAMMA_1_TBL28;
	puiGmaLut[29] = GmaReg9.bit.STCS_GAMMA_1_TBL29;

	puiGmaLut[30] = GmaReg10.bit.STCS_GAMMA_1_TBL30;
	puiGmaLut[31] = GmaReg10.bit.STCS_GAMMA_1_TBL31;
	puiGmaLut[32] = GmaReg10.bit.STCS_GAMMA_1_TBL32;

	puiGmaLut[33] = GmaReg11.bit.STCS_GAMMA_1_TBL33;
	puiGmaLut[34] = GmaReg11.bit.STCS_GAMMA_1_TBL34;
	puiGmaLut[35] = GmaReg11.bit.STCS_GAMMA_1_TBL35;

	GmaReg12.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R280_STCS_GAMMA_OFS);
	GmaReg13.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R284_STCS_GAMMA_OFS);
	GmaReg14.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R288_STCS_GAMMA_OFS);
	GmaReg15.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R28C_STCS_GAMMA_OFS);
	puiGmaLut[36] = GmaReg12.bit.STCS_GAMMA_1_TBL36;
	puiGmaLut[37] = GmaReg12.bit.STCS_GAMMA_1_TBL37;
	puiGmaLut[38] = GmaReg12.bit.STCS_GAMMA_1_TBL38;

	puiGmaLut[39] = GmaReg13.bit.STCS_GAMMA_1_TBL39;
	puiGmaLut[40] = GmaReg13.bit.STCS_GAMMA_1_TBL40;
	puiGmaLut[41] = GmaReg13.bit.STCS_GAMMA_1_TBL41;

	puiGmaLut[42] = GmaReg14.bit.STCS_GAMMA_1_TBL42;
	puiGmaLut[43] = GmaReg14.bit.STCS_GAMMA_1_TBL43;
	puiGmaLut[44] = GmaReg14.bit.STCS_GAMMA_1_TBL44;

	puiGmaLut[45] = GmaReg15.bit.STCS_GAMMA_1_TBL45;
	puiGmaLut[46] = GmaReg15.bit.STCS_GAMMA_1_TBL46;
	puiGmaLut[47] = GmaReg15.bit.STCS_GAMMA_1_TBL47;

	GmaReg16.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R290_STCS_GAMMA_OFS);
	GmaReg17.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R294_STCS_GAMMA_OFS);
	GmaReg18.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R298_STCS_GAMMA_OFS);
	GmaReg19.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R29C_STCS_GAMMA_OFS);
	puiGmaLut[48] = GmaReg16.bit.STCS_GAMMA_1_TBL48;
	puiGmaLut[49] = GmaReg16.bit.STCS_GAMMA_1_TBL49;
	puiGmaLut[50] = GmaReg16.bit.STCS_GAMMA_1_TBL50;

	puiGmaLut[51] = GmaReg17.bit.STCS_GAMMA_1_TBL51;
	puiGmaLut[52] = GmaReg17.bit.STCS_GAMMA_1_TBL52;
	puiGmaLut[53] = GmaReg17.bit.STCS_GAMMA_1_TBL53;

	puiGmaLut[54] = GmaReg18.bit.STCS_GAMMA_1_TBL54;
	puiGmaLut[55] = GmaReg18.bit.STCS_GAMMA_1_TBL55;
	puiGmaLut[56] = GmaReg18.bit.STCS_GAMMA_1_TBL56;

	puiGmaLut[57] = GmaReg19.bit.STCS_GAMMA_1_TBL57;
	puiGmaLut[58] = GmaReg19.bit.STCS_GAMMA_1_TBL58;
	puiGmaLut[59] = GmaReg19.bit.STCS_GAMMA_1_TBL59;

	GmaReg20.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2A0_STCS_GAMMA_OFS);
	GmaReg21.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R2A4_STCS_GAMMA_OFS);
	puiGmaLut[60] = GmaReg20.bit.STCS_GAMMA_1_TBL60;
	puiGmaLut[61] = GmaReg20.bit.STCS_GAMMA_1_TBL61;
	puiGmaLut[62] = GmaReg20.bit.STCS_GAMMA_1_TBL62;

	puiGmaLut[63] = GmaReg21.bit.STCS_GAMMA_1_TBL63;
	puiGmaLut[64] = GmaReg21.bit.STCS_GAMMA_1_TBL64;

}

//----- BCC set reg -----
VOID sie_setBccEn(UINT32 id, BOOL bBccEn)
{
	T_R4_ENGINE_FUNCTION LocalReg;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	LocalReg.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS);
	LocalReg.bit.RAWENC_EN = bBccEn;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R4_ENGINE_FUNCTION_OFS, LocalReg.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

VOID sie_setBccGammaEnReg(UINT32 id, BOOL bGammaEn)
{
	T_R804_RAWENC_CTRL LocalReg;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	LocalReg.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R804_RAWENC_CTRL_OFS);
	LocalReg.bit.GAMMA_EN = bGammaEn;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R804_RAWENC_CTRL_OFS, LocalReg.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

VOID sie_setBccSegBitNumReg(UINT32 id, UINT32 uiSegBitNum)
{
	T_R814_RAWENC_BRC LocalReg;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	LocalReg.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R814_RAWENC_BRC_OFS);
	LocalReg.bit.BRC_SEGBITNO = uiSegBitNum;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R814_RAWENC_BRC_OFS, LocalReg.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

VOID sie_setBccGammaTblReg(UINT32 id, UINT32 *p_uiBccGammaTbl)
{
	INT32 i;
	UINT32 uiBuf;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	for (i = 0; i < 16; i++) {
		//*((volatile UINT32 *)(_SIE_REG_BASE_ADDR_SET[id] + R824_RAWENC_GAMMA_OFS + i * 4)) =
		//	(p_uiBccGammaTbl[i * 2] & 0xfff) + ((p_uiBccGammaTbl[i * 2 + 1] & 0xfff) << 16);
		uiBuf = (p_uiBccGammaTbl[i * 2] & 0xfff) + ((p_uiBccGammaTbl[i * 2 + 1] & 0xfff) << 16);
		SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R824_RAWENC_GAMMA_OFS + i * 4, uiBuf);
	}
	//*((volatile UINT32 *)(_SIE_REG_BASE_ADDR_SET[id] + R864_RAWENC_GAMMA_OFS)) =
	//	(p_uiBccGammaTbl[32] & 0xfff);
	uiBuf = (p_uiBccGammaTbl[32] & 0xfff);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R864_RAWENC_GAMMA_OFS, uiBuf);
	sie_platform_spin_unlock(0,spin_flags);
}

VOID sie_setBccDctQtblIdx(UINT32 id, UINT32 *pDctTblIdx)
{
	T_R868_RAWENC_DCT_QTBL_IDX LocalReg;
	T_R86C_RAWENC_DCT_QTBL_IDX LocalReg2;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	LocalReg.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R868_RAWENC_DCT_QTBL_IDX_OFS);
	LocalReg2.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R86C_RAWENC_DCT_QTBL_IDX_OFS);
	LocalReg.bit.DCT_QTBL0_IDX = pDctTblIdx[0];
	LocalReg.bit.DCT_QTBL1_IDX = pDctTblIdx[1];
	LocalReg.bit.DCT_QTBL2_IDX = pDctTblIdx[2];
	LocalReg.bit.DCT_QTBL3_IDX = pDctTblIdx[3];
	LocalReg2.bit.DCT_QTBL4_IDX = pDctTblIdx[4];
	LocalReg2.bit.DCT_QTBL5_IDX = pDctTblIdx[5];
	LocalReg2.bit.DCT_QTBL6_IDX = pDctTblIdx[6];
	LocalReg2.bit.DCT_QTBL7_IDX = pDctTblIdx[7];
	//DBG_ERR("%d/%d/%d/%d\r\n",pDctTblIdx[0],pDctTblIdx[1],pDctTblIdx[2],pDctTblIdx[3]);
	//DBG_ERR("%d/%d/%d/%d\r\n",pDctTblIdx[4],pDctTblIdx[5],pDctTblIdx[6],pDctTblIdx[7]);
	//DBG_ERR("%d/%d/%d/%d\r\n",LocalReg.bit.DCT_QTBL0_IDX,LocalReg.bit.DCT_QTBL1_IDX,LocalReg.bit.DCT_QTBL2_IDX,LocalReg.bit.DCT_QTBL3_IDX);

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R868_RAWENC_DCT_QTBL_IDX_OFS, LocalReg.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R86C_RAWENC_DCT_QTBL_IDX_OFS, LocalReg2.reg);
	sie_platform_spin_unlock(0,spin_flags);
}
VOID sie_setBccDctThr(UINT32 id, UINT32 *pDctThr)
{
	T_R870_RAWENC_DCT_LEVEL_TH LocalReg;
	unsigned long spin_flags;

	spin_flags=sie_platform_spin_lock(0);
	LocalReg.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R870_RAWENC_DCT_LEVEL_TH_OFS);
	LocalReg.bit.DCT_LEVEL_TH0 = pDctThr[0];
	LocalReg.bit.DCT_LEVEL_TH1 = pDctThr[1];
	LocalReg.bit.DCT_LEVEL_TH2 = pDctThr[2];
	LocalReg.bit.DCT_LEVEL_TH3 = pDctThr[3];
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id], R870_RAWENC_DCT_LEVEL_TH_OFS, LocalReg.reg);
	sie_platform_spin_unlock(0,spin_flags);
}

VOID sie_setBccParam(UINT32 id, SIE_BCC_PARAM_INFO *pBccParam)
{
	sie_setBccGammaEnReg(id, pBccParam->bBccGammaEn);
	sie_setBccSegBitNumReg(id, pBccParam->uiBrcSegBitNo[id]);

	if (pBccParam->bBccGammaEn && pBccParam->p_uiBccGammaTbl != NULL) {
		sie_setBccGammaTblReg(id, pBccParam->p_uiBccGammaTbl);
	}

    sie_setBccDctQtblIdx(id, pBccParam->uiDctTblIdx);
	sie_setBccDctThr(id, pBccParam->uiDctThr);

	//RAWENC_EN must be set at the end to make random rounding initial effective
	//if ((sie_getFunction(id)&SIE_RAWENC_EN) != 0)
	//{
	//	sie_setBccEn(id, FALSE);
	//	sie_setBccEn(id, TRUE);
	//}
}

VOID sie_setBccIniParm(UINT32 id, SIE_BCC_PARAM_INFO *pBccParam, UINT32 rate)
{
	pBccParam->bBccGammaEn = 1;
	pBccParam->uiBrcSegBitNo[id] = rate;

	pBccParam->p_uiBccGammaTbl = g_uiBccGammaTblDef;

	pBccParam->uiDctTblIdx[0] = 0;
	pBccParam->uiDctTblIdx[1] = 4;
	pBccParam->uiDctTblIdx[2] = 8;
	pBccParam->uiDctTblIdx[3] = 0xc;
	pBccParam->uiDctTblIdx[4] = 0x10;
	pBccParam->uiDctTblIdx[5] = 0x14;
	pBccParam->uiDctTblIdx[6] = 0x18;
	pBccParam->uiDctTblIdx[7] = 0x1c;

	pBccParam->uiDctThr[0] = 19;
	pBccParam->uiDctThr[1] = 43;
	pBccParam->uiDctThr[2] = 68;
	pBccParam->uiDctThr[3] = 97;
}

#if 0 // unused API or function
void sie_setBSH(UINT32 id, SIE_BS_H_INFO *pBsh)
{
	T_R160_BASIC_BSH reg160;
	T_R164_BASIC_BSH reg164;
	T_R168_BASIC_BSH reg168;
	T_R16C_BASIC_BSH reg16C;
	T_R170_BASIC_BSH reg170;
	T_R1A8_BASIC_BSV reg1A8;
	sie_platform_spin_lock(0);

	reg160.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R160_BASIC_BSH_OFS);//no need//
	reg164.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R164_BASIC_BSH_OFS);//no need//
	reg168.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R168_BASIC_BSH_OFS);//no need//
	reg16C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R16C_BASIC_BSH_OFS);//no need//
	reg170.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R170_BASIC_BSH_OFS);//no need//
	reg1A8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1A8_BASIC_BSV_OFS);

	reg160.bit.BS_H_IV    = pBsh->uiIv;
	reg160.bit.BS_SRC_INTP = pBsh->bSrcIntpV;

	reg164.bit.BS_H_SV    = pBsh->uiSv;

	reg168.bit.BS_H_BV_R    = pBsh->uiBvR;

	reg16C.bit.BS_H_BV_B    = pBsh->uiBvB;

	reg170.bit.BS_H_OUTSIZE    = pBsh->uiOutSize;
	reg170.bit.BS_H_ACC_DIV_M0 = pBsh->uiDivM[0];
	reg170.bit.BS_H_ACC_DIV_S0 = pBsh->uiDivS[0];

	reg1A8.bit.BS_H_ACC_DIV_M1 = pBsh->uiDivM[1];
	reg1A8.bit.BS_H_ACC_DIV_S1 = pBsh->uiDivS[1];

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R160_BASIC_BSH_OFS, reg160.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R164_BASIC_BSH_OFS, reg164.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R168_BASIC_BSH_OFS, reg168.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R16C_BASIC_BSH_OFS, reg16C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R170_BASIC_BSH_OFS, reg170.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1A8_BASIC_BSV_OFS, reg1A8.reg);
	sie_platform_spin_unlock(0);
}

void sie_getBSH(UINT32 id, SIE_BS_H_INFO *pBsh)
{
	T_R160_BASIC_BSH reg160;
	T_R164_BASIC_BSH reg164;
	T_R168_BASIC_BSH reg168;
	T_R16C_BASIC_BSH reg16C;
	T_R170_BASIC_BSH reg170;
	T_R1A8_BASIC_BSV reg1A8;
	sie_platform_spin_lock(0);

	reg160.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R160_BASIC_BSH_OFS);//no need//
	reg164.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R164_BASIC_BSH_OFS);//no need//
	reg168.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R168_BASIC_BSH_OFS);//no need//
	reg16C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R16C_BASIC_BSH_OFS);//no need//
	reg170.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R170_BASIC_BSH_OFS);//no need//
	reg1A8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1A8_BASIC_BSV_OFS);

	pBsh->uiIv = reg160.bit.BS_H_IV;
	pBsh->bSrcIntpV = reg160.bit.BS_SRC_INTP;
	pBsh->uiSv = reg164.bit.BS_H_SV;
	pBsh->uiBvR = reg168.bit.BS_H_BV_R;
	pBsh->uiBvB = reg16C.bit.BS_H_BV_B;
	pBsh->uiOutSize = reg170.bit.BS_H_OUTSIZE;
	pBsh->uiDivM[0] = reg170.bit.BS_H_ACC_DIV_M0;
	pBsh->uiDivS[0] = reg170.bit.BS_H_ACC_DIV_S0;
	pBsh->uiDivM[1] = reg1A8.bit.BS_H_ACC_DIV_M1;
	pBsh->uiDivS[1] = reg1A8.bit.BS_H_ACC_DIV_S1;
	sie_platform_spin_unlock(0);
}

void sie_setBSV(UINT32 id, SIE_BS_V_INFO *pBsv)
{
	UINT32 i, uiBuf;
	T_R174_BASIC_BSV reg174;
	T_R178_BASIC_BSV reg178;
	T_R17C_BASIC_BSV reg17C;
	T_R180_BASIC_BSV reg180;
	T_R184_BASIC_BSV reg184;
	T_R1A8_BASIC_BSV reg1A8;

	sie_platform_spin_lock(0);
	reg174.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R174_BASIC_BSV_OFS);//no need//
	reg178.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R178_BASIC_BSV_OFS);//no need//
	reg17C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R17C_BASIC_BSV_OFS);//no need//
	reg180.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R180_BASIC_BSV_OFS);//no need//
	reg184.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R184_BASIC_BSV_OFS);//no need//
	reg1A8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1A8_BASIC_BSV_OFS);

	reg174.bit.BS_V_IV    = pBsv->uiIv;

	reg178.bit.BS_V_SV    = pBsv->uiSv;

	reg17C.bit.BS_V_BV_R    = pBsv->uiBvR;

	reg180.bit.BS_V_BV_B    = pBsv->uiBvB;

	reg184.bit.BS_V_OUTSIZE    = pBsv->uiOutSize;
	reg184.bit.BS_V_ACC_IN_S    = pBsv->uiInShft;
	reg184.bit.BS_V_ACC_DIV_M0   = pBsv->uiDivM[0];
	reg184.bit.BS_V_ACC_DIV_S0   = pBsv->uiDivS[0];

	reg1A8.bit.BS_V_ACC_DIV_M1 = pBsv->uiDivM[1];
	reg1A8.bit.BS_V_ACC_DIV_S1 = pBsv->uiDivS[1];

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R174_BASIC_BSV_OFS, reg174.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R178_BASIC_BSV_OFS, reg178.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R17C_BASIC_BSV_OFS, reg17C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R180_BASIC_BSV_OFS, reg180.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R184_BASIC_BSV_OFS, reg184.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1A8_BASIC_BSV_OFS, reg1A8.reg);

	for (i = 0; i < 16; i += 2) {
		uiBuf  = ((pBsv->uiDivMRb[i + 0]) & 0x3ff) << 0;
		uiBuf |= ((pBsv->uiDivSRb[i + 0]) & 0x00f) << 12;
		uiBuf |= ((pBsv->uiDivMRb[i + 1]) & 0x3ff) << 16;
		uiBuf |= ((pBsv->uiDivSRb[i + 1]) & 0x00f) << 28;
		SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R188_BASIC_BSV_OFS + (i * 2), uiBuf);
	}
	sie_platform_spin_unlock(0);
}

void sie_getBSV(UINT32 id, SIE_BS_V_INFO *pBsv)
{
	UINT32 i, uiBuf;
	T_R174_BASIC_BSV reg174;
	T_R178_BASIC_BSV reg178;
	T_R17C_BASIC_BSV reg17C;
	T_R180_BASIC_BSV reg180;
	T_R184_BASIC_BSV reg184;
	T_R1A8_BASIC_BSV reg1A8;

	reg174.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R174_BASIC_BSV_OFS);
	reg178.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R178_BASIC_BSV_OFS);
	reg17C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R17C_BASIC_BSV_OFS);
	reg180.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R180_BASIC_BSV_OFS);
	reg184.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R184_BASIC_BSV_OFS);
	reg1A8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1A8_BASIC_BSV_OFS);

	pBsv->uiIv = reg174.bit.BS_V_IV;

	pBsv->uiSv = reg178.bit.BS_V_SV;

	pBsv->uiBvR = reg17C.bit.BS_V_BV_R;

	pBsv->uiBvB = reg180.bit.BS_V_BV_B;

	pBsv->uiOutSize = reg184.bit.BS_V_OUTSIZE;
	pBsv->uiInShft = reg184.bit.BS_V_ACC_IN_S;
	pBsv->uiDivM[0] = reg184.bit.BS_V_ACC_DIV_M0;
	pBsv->uiDivS[0] = reg184.bit.BS_V_ACC_DIV_S0;
	pBsv->uiDivM[1] = reg1A8.bit.BS_V_ACC_DIV_M1;
	pBsv->uiDivS[1] = reg1A8.bit.BS_V_ACC_DIV_S1;


	for (i = 0; i < 16; i += 2) {
		uiBuf = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R188_BASIC_BSV_OFS + (i * 2));
		pBsv->uiDivMRb[i + 0] = (uiBuf >> 0) & 0x3ff;
		pBsv->uiDivSRb[i + 0] = (uiBuf >> 12) & 0x00f;
		pBsv->uiDivMRb[i + 1] = (uiBuf >> 16) & 0x3ff;
		pBsv->uiDivSRb[i + 1] = (uiBuf >> 28) & 0x00f;
	}
}

void sie_setGridLine(UINT32 id, SIE_GRIDLINE_INFO *pGridLine)
{
	//T_R1BC_BASIC_GRIDLINE reg1bc;
	T_R1C0_BASIC_GRIDLINE reg1c0;

	sie_platform_spin_lock(0);
	//reg1bc.Reg = SIE_GETREG(R1BC_BASIC_GRIDLINE_OFS);
	reg1c0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R1C0_BASIC_GRIDLINE_OFS);

	//reg1bc.Bit.GRID_LINE_VAL0     = pGridLine->uiGridVal0;
	//reg1bc.Bit.GRID_LINE_VAL1     = pGridLine->uiGridVal1;
	reg1c0.bit.GRID_LINE_VAL2     = pGridLine->uiGridVal2;
	reg1c0.bit.GRID_LINE_VAL3     = pGridLine->uiGridVal3;
	reg1c0.bit.GRID_LINE_SHFTBS   = pGridLine->uiShftBs;

	//SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1BC_BASIC_GRIDLINE_OFS, reg1bc.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1C0_BASIC_GRIDLINE_OFS, reg1c0.reg);
	sie_platform_spin_unlock(0);
}

void sie_setYoutScl(UINT32 id, SIE_YOUT_SCAL_INFO *pYoutScl)
{
	T_R1AC_BASIC_YOUT reg1ac;

	sie_platform_spin_lock(0);
	reg1ac.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1AC_BASIC_YOUT_OFS);

	reg1ac.bit.YOUT_SMPL_X_SFACT    = pYoutScl->uiFctrX;
	reg1ac.bit.YOUT_SMPL_Y_SFACT    = pYoutScl->uiFctrY;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1AC_BASIC_YOUT_OFS, reg1ac.reg);
	sie_platform_spin_unlock(0);
}
void sie_setYoutWin(UINT32 id, SIE_YOUT_WIN_INFO *pYoutWin)
{
	T_R1B0_BASIC_YOUT reg1b0;
	T_R1C8_BASIC_YOUT reg1c8;

	sie_platform_spin_lock(0);
	reg1b0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1B0_BASIC_YOUT_OFS);
	reg1c8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1C8_BASIC_YOUT_OFS);

	reg1b0.bit.YOUT_WIN_SZX    = pYoutWin->uiWinSzX;
	reg1b0.bit.YOUT_WIN_SZY    = pYoutWin->uiWinSzY;

	if (pYoutWin->uiWinNumX > 128) {
		nvt_dbg(ERR,"YOUT WIN NUMX out of range !!");
	}
	if (pYoutWin->uiWinNumY > 128) {
		nvt_dbg(ERR,"YOUT WIN NUMY out of range !!");
	}

	reg1c8.bit.YOUT_WIN_NUMX    = pYoutWin->uiWinNumX - 1;
	reg1c8.bit.YOUT_WIN_NUMY    = pYoutWin->uiWinNumY - 1;


	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1B0_BASIC_YOUT_OFS, reg1b0.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1C8_BASIC_YOUT_OFS, reg1c8.reg);
	sie_platform_spin_unlock(0);
}
void sie_setYoutAccm(UINT32 id, SIE_YOUT_ACCM_INFO *pYoutAccm)
{
	T_R1B0_BASIC_YOUT reg1b0;

	sie_platform_spin_lock(0);
	reg1b0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1B0_BASIC_YOUT_OFS);

	reg1b0.bit.YOUT_ACCUM_SHIFT = pYoutAccm->uiAccmShft;
	reg1b0.bit.YOUT_ACCUM_GAIN = pYoutAccm->uiAccmGain;


	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1B0_BASIC_YOUT_OFS, reg1b0.reg);
	sie_platform_spin_unlock(0);
}
void sie_getYoutWin(UINT32 id, SIE_YOUT_WIN_INFO *pSetting)
{
	T_R1B0_BASIC_YOUT reg1b0;
	T_R1C8_BASIC_YOUT reg1c8;

	reg1b0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1B0_BASIC_YOUT_OFS);
	reg1c8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1C8_BASIC_YOUT_OFS);

	pSetting->uiWinSzX   = reg1b0.bit.YOUT_WIN_SZX;
	pSetting->uiWinSzY   = reg1b0.bit.YOUT_WIN_SZY;

	pSetting->uiWinNumX  = reg1c8.bit.YOUT_WIN_NUMX + 1;
	pSetting->uiWinNumY  = reg1c8.bit.YOUT_WIN_NUMY + 1;
}
void sie_setYoutCg(UINT32 id, SIE_YOUT_CG_INFO *pYoutCg)
{
	T_R1C4_BASIC_YOUT reg1c4;

	sie_platform_spin_lock(0);
	reg1c4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R1C4_BASIC_YOUT_OFS);

	reg1c4.bit.YOUT_CGAIN_RGAIN    = pYoutCg->uiRGain;
	reg1c4.bit.YOUT_CGAIN_GGAIN    = pYoutCg->uiGGain;
	reg1c4.bit.YOUT_CGAIN_BGAIN    = pYoutCg->uiBGain;
	reg1c4.bit.YOUT_CGAIN_SEL      = pYoutCg->uiGainSel;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1C4_BASIC_YOUT_OFS, reg1c4.reg);
	sie_platform_spin_unlock(0);
}

void sie_setVaCg(UINT32 id, SIE_VA_CG_INFO *pSetting)
{
	//T_R1F4_STCS_VA_CG reg1F4;
	T_R1F8_STCS_VA_CG reg1F8;

	sie_platform_spin_lock(0);
	//reg1F4.Reg = SIE_GETREG(R1F4_STCS_VA_CG_OFS);
	reg1F8.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id], R1F8_STCS_VA_CG_OFS);

	//reg1F4.Bit.VA_CG_GAIN0 = pSetting->uiGain0;
	//reg1F4.Bit.VA_CG_GAIN1 = pSetting->uiGain1;
	reg1F8.bit.VA_CG_GAIN2 = pSetting->uiGain2;
	reg1F8.bit.VA_CG_GAIN3 = pSetting->uiGain3;

	//SIE_SETREG(R1F4_STCS_VA_CG_OFS, reg1F4.Reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R1F8_STCS_VA_CG_OFS, reg1F8.reg);
	sie_platform_spin_unlock(0);
}


void sie_setVaCrp(UINT32 id, SIE_VA_CROP_INFO *pSetting)
{
	T_R338_STCS_VA reg338;
	T_R33C_STCS_VA reg33C;
	T_R30_ENGINE_CFA_PATTERN reg30;

	sie_platform_spin_lock(0);
	{
		// VA-Crop should not include the last 4 line
		UINT32 uiStcsInSzV = 0;
		if (sie_getFunction(id)&SIE_BS_V_EN) {
			SIE_BS_V_INFO Bsv;
			sie_getBSV(id, &Bsv);
			uiStcsInSzV = Bsv.uiOutSize;
		} else {
			SIE_CRP_WIN_INFO CrpWinInfo;
			sie_getCropWindow(id, &CrpWinInfo);
			uiStcsInSzV = CrpWinInfo.uiSzY;
		}
		if ((pSetting->uiStY + pSetting->uiSzY) > (uiStcsInSzV - 4)) {
			//DBG_ERR("VA Crop out of range !!");
			nvt_dbg(ERR,"VA Crop out of range !!");
		}
	}

	reg338.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R338_STCS_VA_OFS);
	reg33C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R33C_STCS_VA_OFS);
	reg30.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS);

	reg338.bit.VA_CROP_STX    = pSetting->uiStX;
	reg338.bit.VA_CROP_STY    = pSetting->uiStY;

	reg33C.bit.VA_CROP_SZX    = pSetting->uiSzX;
	reg33C.bit.VA_CROP_SZY    = pSetting->uiSzY;

	//reg30.bit.VA_CFAPAT    = pSetting->CfaPat;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R338_STCS_VA_OFS, reg338.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R33C_STCS_VA_OFS, reg33C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R30_ENGINE_CFA_PATTERN_OFS, reg30.reg);
	sie_platform_spin_unlock(0);
}


void sie_getVaCrp(UINT32 id, SIE_VA_CROP_INFO *pSetting)
{
	T_R338_STCS_VA reg338;
	T_R33C_STCS_VA reg33C;

	reg338.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R338_STCS_VA_OFS);
	reg33C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R33C_STCS_VA_OFS);

	pSetting->uiStX = reg338.bit.VA_CROP_STX;
	pSetting->uiStY = reg338.bit.VA_CROP_STY;

	pSetting->uiSzX = reg33C.bit.VA_CROP_SZX;
	pSetting->uiSzY = reg33C.bit.VA_CROP_SZY;
}



void sie_setVaGma2(UINT32 id, SIE_VA_GMA_INFO *pSetting)
{
	UINT32 i, uiBuf;
	//unsigned long myflags;

	if (pSetting->puiGmaTbl == NULL) {
		return;
	}

	sie_platform_spin_lock(0);
	for (i = 0; i < 32; i += 4) {
		uiBuf  = (pSetting->puiGmaTbl[i + 0] & 0xff) << 0;
		uiBuf |= (pSetting->puiGmaTbl[i + 1] & 0xff) << 8;
		uiBuf |= (pSetting->puiGmaTbl[i + 2] & 0xff) << 16;
		uiBuf |= (pSetting->puiGmaTbl[i + 3] & 0xff) << 24;
		SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R3B8_STCS_GAMMA_OFS + (i), uiBuf);
	}
	// the last one
	uiBuf  = (pSetting->puiGmaTbl[32] & 0xff) << 0;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R3B8_STCS_GAMMA_OFS + (i), uiBuf);
	sie_platform_spin_unlock(0);
}

void sie_setVaFltrG1(UINT32 id, SIE_VA_FLTR_GROUP_INFO *pSetting)
{
	T_R340_STCS_VA reg340;
	T_R344_STCS_VA reg344;
	T_R354_STCS_VA reg354;
	T_R35C_STCS_VA reg35C;

	sie_platform_spin_lock(0);
	reg340.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R340_STCS_VA_OFS);//no need//
	reg344.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R344_STCS_VA_OFS);//no need//
	reg354.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R354_STCS_VA_OFS);//no need//
	reg35C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R35C_STCS_VA_OFS);

	reg340.bit.VDETGH1A         = pSetting->FltrH.uiTapA;
	reg340.bit.VDETGH1B         = pSetting->FltrH.iTapB;
	reg340.bit.VDETGH1C         = pSetting->FltrH.iTapC;
	reg340.bit.VDETGH1D         = pSetting->FltrH.iTapD;
	reg340.bit.VDETGH1_BCD_OP   = pSetting->FltrH.FltrSymm;
	reg340.bit.VDETGH1_FSIZE    = pSetting->FltrH.FltrSize;
	reg340.bit.VDETGH1_EXTEND   = pSetting->bHExtend;
	reg340.bit.VDETGH1_DIV      = pSetting->FltrH.uiDiv;

	reg344.bit.VDETGV1A         = pSetting->FltrV.uiTapA;
	reg344.bit.VDETGV1B         = pSetting->FltrV.iTapB;
	reg344.bit.VDETGV1C         = pSetting->FltrV.iTapC;
	reg344.bit.VDETGV1D         = pSetting->FltrV.iTapD;
	reg344.bit.VDETGV1_BCD_OP   = pSetting->FltrV.FltrSymm;
	reg344.bit.VDETGV1_FSIZE    = pSetting->FltrV.FltrSize;
	reg344.bit.VDETGV1_DIV      = pSetting->FltrV.uiDiv;

	reg354.bit.VA_G1HTHL        = pSetting->FltrH.uiThL;
	reg354.bit.VA_G1HTHH        = pSetting->FltrH.uiThH;
	reg354.bit.VA_G1VTHL        = pSetting->FltrV.uiThL;
	reg354.bit.VA_G1VTHH        = pSetting->FltrV.uiThH;

	reg35C.bit.VA_G1_LINE_MAX_MODE        = pSetting->bLineMax;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R340_STCS_VA_OFS, reg340.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R344_STCS_VA_OFS, reg344.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R354_STCS_VA_OFS, reg354.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R35C_STCS_VA_OFS, reg35C.reg);
	sie_platform_spin_unlock(0);
}

void sie_setVaFltrG2(UINT32 id, SIE_VA_FLTR_GROUP_INFO *pSetting)
{
	T_R348_STCS_VA reg348;
	T_R34C_STCS_VA reg34C;
	T_R358_STCS_VA reg358;
	T_R35C_STCS_VA reg35C;

	sie_platform_spin_lock(0);
	reg348.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R348_STCS_VA_OFS);//no need//
	reg34C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R34C_STCS_VA_OFS);//no need//
	reg358.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R358_STCS_VA_OFS);//no need//
	reg35C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R35C_STCS_VA_OFS);

	reg348.bit.VDETGH2A         = pSetting->FltrH.uiTapA;
	reg348.bit.VDETGH2B         = pSetting->FltrH.iTapB;
	reg348.bit.VDETGH2C         = pSetting->FltrH.iTapC;
	reg348.bit.VDETGH2D         = pSetting->FltrH.iTapD;
	reg348.bit.VDETGH2_BCD_OP   = pSetting->FltrH.FltrSymm;
	reg348.bit.VDETGH2_FSIZE    = pSetting->FltrH.FltrSize;
	reg348.bit.VDETGH2_EXTEND   = pSetting->bHExtend;
	reg348.bit.VDETGH2_DIV      = pSetting->FltrH.uiDiv;

	reg34C.bit.VDETGV2A         = pSetting->FltrV.uiTapA;
	reg34C.bit.VDETGV2B         = pSetting->FltrV.iTapB;
	reg34C.bit.VDETGV2C         = pSetting->FltrV.iTapC;
	reg34C.bit.VDETGV2D         = pSetting->FltrV.iTapD;
	reg34C.bit.VDETGV2_BCD_OP   = pSetting->FltrV.FltrSymm;
	reg34C.bit.VDETGV2_FSIZE    = pSetting->FltrV.FltrSize;
	reg34C.bit.VDETGV2_DIV      = pSetting->FltrV.uiDiv;

	reg358.bit.VA_G2HTHL        = pSetting->FltrH.uiThL;
	reg358.bit.VA_G2HTHH        = pSetting->FltrH.uiThH;
	reg358.bit.VA_G2VTHL        = pSetting->FltrV.uiThL;
	reg358.bit.VA_G2VTHH        = pSetting->FltrV.uiThH;

	reg35C.bit.VA_G2_LINE_MAX_MODE        = pSetting->bLineMax;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R348_STCS_VA_OFS, reg348.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R34C_STCS_VA_OFS, reg34C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R358_STCS_VA_OFS, reg358.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R35C_STCS_VA_OFS, reg35C.reg);
	sie_platform_spin_unlock(0);
}



void sie_setVaWin(UINT32 id, SIE_VA_WIN_INFO *pSetting)
{
	T_R35C_STCS_VA reg35C;
	T_R360_STCS_VA reg360;

	sie_platform_spin_lock(0);
	{
		// VA-Win should not cross VA-Crop
		SIE_VA_CROP_INFO VaCrpInfo;
		sie_getVaCrp(id, &VaCrpInfo);

		if ((pSetting->uiWinSzX + ((pSetting->uiWinNmX - 1) * (pSetting->uiWinSzX + pSetting->uiWinSpX))) > VaCrpInfo.uiSzX) {
			//DBG_ERR("VA Win X out of range !!");
			nvt_dbg(ERR,"VA Win X out of range !!");
		}
		if ((pSetting->uiWinSzY + ((pSetting->uiWinNmY - 1) * (pSetting->uiWinSzY + pSetting->uiWinSpY))) > VaCrpInfo.uiSzY) {
			//DBG_ERR("VA Win Y out of range !!");
	        nvt_dbg(ERR,"VA Win Y out of range !!");
		}
	}

	reg35C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R35C_STCS_VA_OFS);
	reg360.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R360_STCS_VA_OFS);

	reg35C.bit.VA_WIN_SZX    = pSetting->uiWinSzX;
	reg35C.bit.VA_WIN_SZY    = pSetting->uiWinSzY;

	reg360.bit.VA_WIN_NUMX    = pSetting->uiWinNmX - 1;
	reg360.bit.VA_WIN_NUMY    = pSetting->uiWinNmY - 1;
	reg360.bit.VA_WIN_SKIPX   = pSetting->uiWinSpX;
	reg360.bit.VA_WIN_SKIPY   = pSetting->uiWinSpY;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R35C_STCS_VA_OFS, reg35C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R360_STCS_VA_OFS, reg360.reg);
	sie_platform_spin_unlock(0);
}


void sie_getVaWin(UINT32 id, SIE_VA_WIN_INFO *pSetting)
{
	T_R35C_STCS_VA reg35C;
	T_R360_STCS_VA reg360;

	sie_platform_spin_lock(0);
	reg35C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R35C_STCS_VA_OFS);
	reg360.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R360_STCS_VA_OFS);

	pSetting->uiWinSzX = reg35C.bit.VA_WIN_SZX;
	pSetting->uiWinSzY = reg35C.bit.VA_WIN_SZY;

	pSetting->uiWinNmX = reg360.bit.VA_WIN_NUMX + 1;
	pSetting->uiWinNmY = reg360.bit.VA_WIN_NUMY + 1;
	pSetting->uiWinSpX = reg360.bit.VA_WIN_SKIPX;
	pSetting->uiWinSpY = reg360.bit.VA_WIN_SKIPY;
	sie_platform_spin_unlock(0);
}


void sie_setVaIndepWin(UINT32 id, SIE_VA_INDEP_WIN_INFO *pSetting, UINT32 uiIdx)
{
	UINT32 uiAddrOfs;
	T_R364_STCS_VA reg364;
	T_R368_STCS_VA reg368;
	//unsigned long myflags;

	if (uiIdx < 5) {
		uiAddrOfs = uiIdx * 8;
	} else {
		return;
	}

	sie_platform_spin_lock(0);
	reg364.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],(R364_STCS_VA_OFS + uiAddrOfs)); //no need//
	reg368.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],(R368_STCS_VA_OFS + uiAddrOfs)); //no need//

	reg364.bit.WIN0_VAEN    = pSetting->bEn;
	reg364.bit.WIN0_STX    = pSetting->uiWinStX;
	reg364.bit.WIN0_STY    = pSetting->uiWinStY;

	reg368.bit.WIN0_G1_LINE_MAX_MODE    = pSetting->bLineMaxG1;
	reg368.bit.WIN0_G2_LINE_MAX_MODE    = pSetting->bLineMaxG2;
	reg368.bit.WIN0_HSZ    = pSetting->uiWinSzX;
	reg368.bit.WIN0_VSZ    = pSetting->uiWinSzY;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],(R364_STCS_VA_OFS + uiAddrOfs), reg364.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],(R368_STCS_VA_OFS + uiAddrOfs), reg368.reg);
	sie_platform_spin_unlock(0);
}


void sie_getVaIndepWin(UINT32 id, SIE_VA_INDEP_WIN_INFO *pSetting, UINT32 uiIdx)
{
	UINT32 uiAddrOfs;
	T_R364_STCS_VA reg364;
	T_R368_STCS_VA reg368;

	sie_platform_spin_lock(0);
	if (uiIdx < 5) {
		uiAddrOfs = uiIdx * 8;
	} else {
		return;
	}
	reg364.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R364_STCS_VA_OFS + uiAddrOfs);
	reg368.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R368_STCS_VA_OFS + uiAddrOfs);

	pSetting->bEn = reg364.bit.WIN0_VAEN;
	pSetting->uiWinStX = reg364.bit.WIN0_STX;
	pSetting->uiWinStY = reg364.bit.WIN0_STY;

	pSetting->bLineMaxG1 = reg368.bit.WIN0_G1_LINE_MAX_MODE;
	pSetting->bLineMaxG2 = reg368.bit.WIN0_G2_LINE_MAX_MODE;
	pSetting->uiWinSzX = reg368.bit.WIN0_HSZ;
	pSetting->uiWinSzY = reg368.bit.WIN0_VSZ;
	sie_platform_spin_unlock(0);
}

void sie_getVaIndepWinRslt(UINT32 id, SIE_VA_INDEP_WIN_RSLT_INFO *pSetting, UINT32 uiIdx)
{
	UINT32 uiAddrOfs;
	T_R38C_STCS_VA reg38C;
	T_R390_STCS_VA reg390;

	sie_platform_spin_lock(0);
	if (uiIdx < 5) {
		uiAddrOfs = uiIdx * 8;
	} else {
		return;
	}

	reg38C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R38C_STCS_VA_OFS + uiAddrOfs);
	reg390.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R390_STCS_VA_OFS + uiAddrOfs);

	#if 0
	pSetting->uiVaG1H = reg38C.bit.VA_WIN0G1H;
	pSetting->uiVaG1V = reg38C.bit.VA_WIN0G1V;

	pSetting->uiVaG2H = reg390.bit.VA_WIN0G2H;
	pSetting->uiVaG2V = reg390.bit.VA_WIN0G2V;
	#else
	pSetting->uiVaG1H = ((reg38C.bit.VA_WIN0G1H >> 5) & 0x7ff) << ((reg38C.bit.VA_WIN0G1H) & 0x1f);
	pSetting->uiVaG1V = ((reg38C.bit.VA_WIN0G1V >> 5) & 0x7ff) << ((reg38C.bit.VA_WIN0G1V) & 0x1f);
	pSetting->uiVaG2H = ((reg390.bit.VA_WIN0G2H >> 5) & 0x7ff) << ((reg390.bit.VA_WIN0G2H) & 0x1f);
	pSetting->uiVaG2V = ((reg390.bit.VA_WIN0G2V >> 5) & 0x7ff) << ((reg390.bit.VA_WIN0G2V) & 0x1f);

	//nvt_dbg(IND,"%x/%x/%x/%x\r\n",reg38C.bit.VA_WIN0G1H,reg38C.bit.VA_WIN0G1V,reg390.bit.VA_WIN0G2H,reg390.bit.VA_WIN0G2V);
	#endif
	sie_platform_spin_unlock(0);
}


void sie_setEth(UINT32 id, SIE_ETH_INFO *pEth)
{
	T_R3B4_STCS_ETH reg3B4;
	T_R3DC_ADVANCED_ETH reg3dc;

	sie_platform_spin_lock(0);
	reg3B4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R3B4_STCS_ETH_OFS);//no need//
	reg3dc.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R3DC_ADVANCED_ETH_OFS);//no need//

	reg3B4.bit.ETHLOW = pEth->uiEthLow;
	reg3B4.bit.ETHMID = pEth->uiEthMid;
	reg3B4.bit.ETHHIGH = pEth->uiEthHigh;

	reg3dc.bit.ETH_OUTFMT = pEth->bEthOutFmt;
	reg3dc.bit.ETH_OUTSEL = pEth->bEthOutSel;
	reg3dc.bit.ETH_HOUT_SEL = pEth->bEthHOutSel;
	reg3dc.bit.ETH_VOUT_SEL = pEth->bEthVOutSel;


	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R3B4_STCS_ETH_OFS, reg3B4.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R3DC_ADVANCED_ETH_OFS, reg3dc.reg);
	sie_platform_spin_unlock(0);
}

void sie_getEth(UINT32 id, SIE_ETH_INFO *pEth)
{
	T_R3B4_STCS_ETH reg3B4;
	T_R3DC_ADVANCED_ETH reg3dc;

	reg3B4.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R3B4_STCS_ETH_OFS);//no need//
	reg3dc.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R3DC_ADVANCED_ETH_OFS);//no need//

	pEth->uiEthLow = reg3B4.bit.ETHLOW;
	pEth->uiEthMid = reg3B4.bit.ETHMID;
	pEth->uiEthHigh = reg3B4.bit.ETHHIGH;

	pEth->bEthOutFmt = reg3dc.bit.ETH_OUTFMT;
	pEth->bEthOutSel = reg3dc.bit.ETH_OUTSEL;
	pEth->bEthHOutSel = reg3dc.bit.ETH_HOUT_SEL;
	pEth->bEthVOutSel = reg3dc.bit.ETH_VOUT_SEL;
}

void sie_setPFPC(UINT32 id, SIE_PFPC_INFO *pPfpcParam)
{
	UINT32 i, uiBuf;
	T_R3DC_ADVANCED_ETH regPfpc0;

	sie_platform_spin_lock(0);
	regPfpc0.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R3DC_ADVANCED_ETH_OFS);//no need//
	regPfpc0.bit.PFPC_MODE    = pPfpcParam->PfpcMode;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R3DC_ADVANCED_ETH_OFS, regPfpc0.reg);

	for (i = 0; i < 32; i += 4) {
		uiBuf  = (*(pPfpcParam->uiGainTbl + i + 0)) << 0;
		uiBuf |= (*(pPfpcParam->uiGainTbl + i + 1)) << 8;
		uiBuf |= (*(pPfpcParam->uiGainTbl + i + 2)) << 16;
		uiBuf |= (*(pPfpcParam->uiGainTbl + i + 3)) << 24;
		SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R3E0_ADVANCED_PFPC_OFS + (i), uiBuf);
		uiBuf  = (*(pPfpcParam->uiOfsTbl + i + 0)) << 0;
		uiBuf |= (*(pPfpcParam->uiOfsTbl + i + 1)) << 8;
		uiBuf |= (*(pPfpcParam->uiOfsTbl + i + 2)) << 16;
		uiBuf |= (*(pPfpcParam->uiOfsTbl + i + 3)) << 24;
		SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R400_ADVANCED_PFPC_OFS + (i), uiBuf);
	}
	sie_platform_spin_unlock(0);
}

void sie_setObFrm(UINT32 id, SIE_OB_FRAME_INFO *pObFrm)
{
	T_R420_ADVANCED_OBF reg420;
	T_R424_ADVANCED_OBF reg424;
	T_R428_ADVANCED_OBF reg428;
	T_R42C_ADVANCED_OBF reg42C;
	T_R430_ADVANCED_OBF reg430;

	sie_platform_spin_lock(0);
	reg420.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R420_ADVANCED_OBF_OFS);//no need//
	reg424.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R424_ADVANCED_OBF_OFS);//no need//
	reg428.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R428_ADVANCED_OBF_OFS);//no need//
	reg42C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R42C_ADVANCED_OBF_OFS);//no need//
	reg430.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R430_ADVANCED_OBF_OFS);//no need//

	reg420.bit.OBF_TOP_X        = pObFrm->uiTopX;
	reg420.bit.OBF_TOP_Y        = pObFrm->uiTopY;
	reg424.bit.OBF_TOP_INTVL    = pObFrm->uiTopIntvl;
	reg424.bit.OBF_THRES        = pObFrm->uiThresold;
	reg424.bit.OBF_NF           = pObFrm->uiNormFctr;
	reg428.bit.OBF_LEFT_X       = pObFrm->uiLeftX;
	reg428.bit.OBF_LEFT_Y       = pObFrm->uiLeftY;
	reg42C.bit.OBF_LEFT_INTVL   = pObFrm->uiLeftIntvl;
	reg430.bit.OBF_BOT_Y        = pObFrm->uiBotY;
	reg430.bit.OBF_RIGHT_X      = pObFrm->uiRightX;

	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R420_ADVANCED_OBF_OFS, reg420.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R424_ADVANCED_OBF_OFS, reg424.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R428_ADVANCED_OBF_OFS, reg428.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R42C_ADVANCED_OBF_OFS, reg42C.reg);
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R430_ADVANCED_OBF_OFS, reg430.reg);
	sie_platform_spin_unlock(0);
}


void sie_getObFrmRslt(UINT32 id, SIE_OB_FRAME_RSLT_INFO *pObFrmRslt)
{
	T_R434_ADVANCED_OBF reg434;
	T_R438_ADVANCED_OBF reg438;
	T_R43C_ADVANCED_OBF reg43C;
	T_R440_ADVANCED_OBF reg440;
	T_R444_ADVANCED_OBF reg444;
	T_R448_ADVANCED_OBF reg448;
	T_R44C_ADVANCED_OBF reg44C;
	T_R450_ADVANCED_OBF reg450;
	T_R454_ADVANCED_OBF reg454;

	reg434.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R434_ADVANCED_OBF_OFS);
	reg438.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R438_ADVANCED_OBF_OFS);
	reg43C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R43C_ADVANCED_OBF_OFS);
	reg440.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R440_ADVANCED_OBF_OFS);
	reg444.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R444_ADVANCED_OBF_OFS);
	reg448.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R448_ADVANCED_OBF_OFS);
	reg44C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R44C_ADVANCED_OBF_OFS);
	reg450.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R450_ADVANCED_OBF_OFS);
	reg454.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R454_ADVANCED_OBF_OFS);

	pObFrmRslt->uiObfCnt = reg434.bit.OBF_CNT;

	pObFrmRslt->uiTopRslt[0] = (reg438.bit.OBF_TOP0_3 >> 0) & 0xff;
	pObFrmRslt->uiTopRslt[1] = (reg438.bit.OBF_TOP0_3 >> 8) & 0xff;
	pObFrmRslt->uiTopRslt[2] = (reg438.bit.OBF_TOP0_3 >> 16) & 0xff;
	pObFrmRslt->uiTopRslt[3] = (reg438.bit.OBF_TOP0_3 >> 24) & 0xff;
	pObFrmRslt->uiTopRslt[4] = (reg43C.bit.OBF_TOP4_7 >> 0) & 0xff;
	pObFrmRslt->uiTopRslt[5] = (reg43C.bit.OBF_TOP4_7 >> 8) & 0xff;
	pObFrmRslt->uiTopRslt[6] = (reg43C.bit.OBF_TOP4_7 >> 16) & 0xff;
	pObFrmRslt->uiTopRslt[7] = (reg43C.bit.OBF_TOP4_7 >> 24) & 0xff;

	pObFrmRslt->uiLeftRslt[0] = (reg440.bit.OBF_LEFT0_3 >> 0) & 0xff;
	pObFrmRslt->uiLeftRslt[1] = (reg440.bit.OBF_LEFT0_3 >> 8) & 0xff;
	pObFrmRslt->uiLeftRslt[2] = (reg440.bit.OBF_LEFT0_3 >> 16) & 0xff;
	pObFrmRslt->uiLeftRslt[3] = (reg440.bit.OBF_LEFT0_3 >> 24) & 0xff;
	pObFrmRslt->uiLeftRslt[4] = (reg444.bit.OBF_LEFT4_7 >> 0) & 0xff;
	pObFrmRslt->uiLeftRslt[5] = (reg444.bit.OBF_LEFT4_7 >> 8) & 0xff;
	pObFrmRslt->uiLeftRslt[6] = (reg444.bit.OBF_LEFT4_7 >> 16) & 0xff;
	pObFrmRslt->uiLeftRslt[7] = (reg444.bit.OBF_LEFT4_7 >> 24) & 0xff;

	pObFrmRslt->uiBotRslt[0] = (reg448.bit.OBF_BOT0_3 >> 0) & 0xff;
	pObFrmRslt->uiBotRslt[1] = (reg448.bit.OBF_BOT0_3 >> 8) & 0xff;
	pObFrmRslt->uiBotRslt[2] = (reg448.bit.OBF_BOT0_3 >> 16) & 0xff;
	pObFrmRslt->uiBotRslt[3] = (reg448.bit.OBF_BOT0_3 >> 24) & 0xff;
	pObFrmRslt->uiBotRslt[4] = (reg44C.bit.OBF_BOT4_7 >> 0) & 0xff;
	pObFrmRslt->uiBotRslt[5] = (reg44C.bit.OBF_BOT4_7 >> 8) & 0xff;
	pObFrmRslt->uiBotRslt[6] = (reg44C.bit.OBF_BOT4_7 >> 16) & 0xff;
	pObFrmRslt->uiBotRslt[7] = (reg44C.bit.OBF_BOT4_7 >> 24) & 0xff;

	pObFrmRslt->uiRightRslt[0] = (reg450.bit.OBF_RIGHT0_3 >> 0) & 0xff;
	pObFrmRslt->uiRightRslt[1] = (reg450.bit.OBF_RIGHT0_3 >> 8) & 0xff;
	pObFrmRslt->uiRightRslt[2] = (reg450.bit.OBF_RIGHT0_3 >> 16) & 0xff;
	pObFrmRslt->uiRightRslt[3] = (reg450.bit.OBF_RIGHT0_3 >> 24) & 0xff;
	pObFrmRslt->uiRightRslt[4] = (reg454.bit.OBF_RIGHT4_7 >> 0) & 0xff;
	pObFrmRslt->uiRightRslt[5] = (reg454.bit.OBF_RIGHT4_7 >> 8) & 0xff;
	pObFrmRslt->uiRightRslt[6] = (reg454.bit.OBF_RIGHT4_7 >> 16) & 0xff;
	pObFrmRslt->uiRightRslt[7] = (reg454.bit.OBF_RIGHT4_7 >> 24) & 0xff;


}


void sie_setObPln(UINT32 id, SIE_OB_PLANE_INFO *pObPln)
{
	T_R45C_ADVANCED_OBP reg45C;
	UINT32      uiAddr, uiReg, i;

	sie_platform_spin_lock(0);
	reg45C.reg = SIE_GETREG(_SIE_REG_BASE_ADDR_SET[id],R45C_ADVANCED_OBP_OFS);//no need//

	reg45C.bit.OBP_HSCL        = pObPln->uiHSclFctr;
	reg45C.bit.OBP_VSCL        = pObPln->uiVSclFctr;
	reg45C.bit.OBP_SHIFT       = pObPln->uiShift;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],R45C_ADVANCED_OBP_OFS, reg45C.reg);

	if (pObPln->puiObpTbl == NULL) {
		sie_platform_spin_unlock(0);
		return;
	}
	uiAddr = R460_ADVANCED_OBP_OFS;
	for (i = 0; i < (288); i += 4) {
		uiReg  = (*(pObPln->puiObpTbl + i + 0)) << 0;
		uiReg |= (*(pObPln->puiObpTbl + i + 1)) << 8;
		uiReg |= (*(pObPln->puiObpTbl + i + 2)) << 16;
		uiReg |= (*(pObPln->puiObpTbl + i + 3)) << 24;
		SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],uiAddr, uiReg);
		uiAddr += 4;
	}
	// the last one
	uiReg  = (*(pObPln->puiObpTbl + i + 0)) << 0;
	SIE_SETREG(_SIE_REG_BASE_ADDR_SET[id],uiAddr, uiReg);
	sie_platform_spin_unlock(0);
}

#endif

#ifdef __KERNEL__
EXPORT_SYMBOL(sie_getFunction);
EXPORT_SYMBOL(sie_getEngineStatus);
EXPORT_SYMBOL(sie_getIntrStatus);
EXPORT_SYMBOL(sie_getActiveWindow);
EXPORT_SYMBOL(sie_getCropWindow);
EXPORT_SYMBOL(sie_getBurstLength);
EXPORT_SYMBOL(sie_getDVI);
EXPORT_SYMBOL(sie_getObOfs);
//NA//EXPORT_SYMBOL(sie_calcYoutInfo);
EXPORT_SYMBOL(sie_getLaWin);
EXPORT_SYMBOL(sie_getLaWinSum);
EXPORT_SYMBOL(sie_calcBSHScl);
EXPORT_SYMBOL(sie_calcBSVScl);
EXPORT_SYMBOL(sie_getCAResultManual);
EXPORT_SYMBOL(sie_getHisto);
EXPORT_SYMBOL(sie_getMdResult);
EXPORT_SYMBOL(sie_getLAResultManual);
//NA//EXPORT_SYMBOL(sie_calcObPlnScl);
EXPORT_SYMBOL(sie_calcECSScl);
EXPORT_SYMBOL(sie_calcCaLaSize);
EXPORT_SYMBOL(sie_getOutAdd);
EXPORT_SYMBOL(sie_getOutAdd_Flip);
//NA//EXPORT_SYMBOL(sie_getVaIndepWinRslt);
EXPORT_SYMBOL(sie_getMainInput);
EXPORT_SYMBOL(sie_setDramInStart);
#endif

//#endif
//@}
