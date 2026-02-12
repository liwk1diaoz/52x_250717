/*
    NUE module driver

    NT98313 NUE driver.

    @file       nue_int.c
    @ingroup    mIIPPNUE

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "nue_platform.h"
#include "nue_lib.h"
#include "nue_reg.h"
#include "nue_int.h"

/**
    @addtogroup mIIPPNUE
*/
//@{

#define NUE_FCD_VLC_REG_CNT		12
#define NUE_FCD_VLC_TBL_SIZE	23
#define NUE_RESULTS_REG_CNT		16

static NUE_OPMODE nue_mode = NUE_OPMODE_USERDEFINE;
#if 0
static BOOL nue_is_reshape_8b_w_shf = TRUE;
#endif

/**
	NUE Get Cycle

	NUE get current cycle.

	@return UINT32 cycle.
*/
UINT32 nue_getcycle(VOID)
{
	return NUE_GETDATA(0x1f0, NUE_IOADDR_REG_BASE);
}

VOID nue_cycle_en(BOOL enable)
{
	T_DEBUG_DESIGN_REGISTER reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = DEBUG_DESIGN_REGISTER_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
    reg1.bit.CYCLE_EN_528 = enable;
	reg1.bit.CYCLE_EN_520 = enable;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
	NUE Get Driver Mode

	NUE get driver operation mode.

	@return NUE_OPMODE operation mode.
*/
NUE_OPMODE nue_get_drvmode(VOID)
{
	return nue_mode;
}

/**
	NUE Set Driver Mode

	NUE set driver operation mode.

	@param[in] mode select driver operation mode.

	@return None.
*/
VOID nue_set_drvmode(NUE_OPMODE mode)
{
	nue_mode = mode;
}

/**
    Enable/Disable NUE Interrupt

    NUE interrupt enable selection.

    @param[in] enable Decide selected funtions are to be enabled or disabled.
        \n-@b ENABLE: enable selected functions.
        \n-@b DISABLE: disable selected functions.
    @param[in] intr Indicate the function(s).

    @return None.
*/
VOID nue_enable_int(BOOL enable, UINT32 intr)
{
	T_NUE_INTERRUPT_ENABLE_REGISTER reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_INTERRUPT_ENABLE_REGISTER_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	if (enable) {
		reg1.reg |= intr;
	} else {
		reg1.reg &= (~intr);
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Get Interrupt Enable Status

    Get current NUE interrupt Enable status.

    @return NUE interrupt Enable status.
*/
UINT32 nue_get_int_enable(VOID)
{
	T_NUE_INTERRUPT_ENABLE_REGISTER reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_INTERRUPT_ENABLE_REGISTER_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return reg1.reg;
}

/**
    NUE Clear Interrupt Status

    Clear NUE interrupt status.

    @param[in] uiStatus 1's in this word will clear corresponding interrupt status.

    @return None.
*/
VOID nue_clr_intr_status(UINT32 status)
{
	T_NUE_INTERRUPT_STATUS_REGISTER reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_INTERRUPT_STATUS_REGISTER_OFS;
	reg1.reg = status;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Get Interrupt Status

    Get current NUE interrupt status.

    @return NUE interrupt status readout.
*/
UINT32 nue_get_intr_status(VOID)
{
	T_NUE_INTERRUPT_STATUS_REGISTER reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_INTERRUPT_STATUS_REGISTER_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	return reg1.reg;
}

//@}

/**
    NUE Reset

    Enable/disable NUE SW reset.

    @param[in] bReset.
        \n-@b TRUE: enable reset.
        \n-@b FALSE: disable reset.

    @return None.
*/
VOID nue_clr(BOOL reset)
{
	T_NUE_CONTROL_REGISTER reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_CONTROL_REGISTER_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.NUE_RST    = reset;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Enable

    Trigger NUE HW start.

    @param[in] bStart.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
VOID nue_enable(BOOL start)
{
	T_NUE_CONTROL_REGISTER reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_CONTROL_REGISTER_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.NUE_START = start;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Linked list Enable

    Trigger NUE Linked list HW start.

    @param[in] bStart.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
VOID nue_ll_enable(BOOL start)
{
	T_NUE_CONTROL_REGISTER reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_CONTROL_REGISTER_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.LL_FIRE = start;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Linked list Terminate

    Trigger NUE Linked list HW terminate.

    @param[in] isterminate.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
VOID nue_ll_terminate(BOOL isterminate)
{
	T_NUE_LL_TERMINATE_REGISTER reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_LL_TERMINATE_REGISTER_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.LL_TERMINATE = isterminate;
	if (isterminate) {
		reg1.bit.LL_TERMINATE = 0;
		reg1.bit.LL_TERMINATE = isterminate;
	}
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Set Engine Mode

    Set NUE HW mode.

    @param[in] mode input nue mode.

    @return None.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER nue_set_engmode(NUE_MODE_TYPE mode)
{
	T_NUE_MODE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	switch (mode) {
	case NUE_SVM:
		reg1.bit.NUE_MODE = 0;
		break;
	case NUE_ROIPOOLING:
		reg1.bit.NUE_MODE = 1;
		break;
	case NUE_REORGANIZATION:
		reg1.bit.NUE_MODE = 3;
		break;
	case NUE_PERMUTE:
		reg1.bit.NUE_MODE = 4;
		break;
	case NUE_ANCHOR:
		reg1.bit.NUE_MODE = 5;
		break;
	case NUE_SOFTMAX:
		reg1.bit.NUE_MODE = 6;
		break;
	default:
		nvt_dbg(ERR, "Unknown engine mode: %d!\r\n", mode);
		return E_PAR;
	}
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
    NUE Get Engine type

    Get current NUE engine mode.

    @return NUE engine mode.
*/
NUE_MODE_TYPE nue_get_engmode(VOID)
{
	T_NUE_MODE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	return (NUE_MODE_TYPE)reg1.bit.NUE_MODE;
}

/**
    NUE Set Input type

    Set NUE input type.

    @param[in] in_type input type.

    @return None.
*/
VOID nue_set_intype(NUE_IO_TYPE in_type)
{
	T_NUE_MODE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	if ((in_type == NUE_INT8) || (in_type == NUE_INT16)) {
		reg1.bit.IN_SIGNEDNESS = 1;
	} else {
		reg1.bit.IN_SIGNEDNESS = 0;
	}

	if ((in_type == NUE_UINT16) || (in_type == NUE_INT16)) {
		reg1.bit.IN_BIT_DEPTH = 1;
	} else {
		reg1.bit.IN_BIT_DEPTH = 0;
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Set Output type

    Set NUE output type.

    @param[in] out_type input type.

    @return None.
*/
VOID nue_set_outtype(NUE_IO_TYPE out_type)
{
	T_NUE_MODE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	if ((out_type == NUE_INT8) || (out_type == NUE_INT16)) {
		reg1.bit.OUT_SIGNEDNESS = 1;
	} else {
		reg1.bit.OUT_SIGNEDNESS = 0;
	}

	if ((out_type == NUE_UINT16) || (out_type == NUE_INT16)) {
		reg1.bit.OUT_BIT_DEPTH = 1;
	} else {
		reg1.bit.OUT_BIT_DEPTH = 0;
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Get Input type

    Get current NUE input type.

    @return NUE input type.
*/
NUE_IO_TYPE nue_get_intype(VOID)
{
	T_NUE_MODE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	if ((reg1.bit.IN_SIGNEDNESS == 1) && (reg1.bit.IN_BIT_DEPTH == 1)) {
		return NUE_INT16;
	} else if ((reg1.bit.IN_SIGNEDNESS == 1) && (reg1.bit.IN_BIT_DEPTH == 0)) {
		return NUE_INT8;
	} else if ((reg1.bit.IN_SIGNEDNESS == 0) && (reg1.bit.IN_BIT_DEPTH == 1)) {
		return NUE_UINT16;
	} else {
		return NUE_UINT8;
	}
}

/**
    NUE Get Output type

    Get current NUE output type.

    @return NUE output type.
*/
NUE_IO_TYPE nue_get_outtype(VOID)
{
	T_NUE_MODE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	if ((reg1.bit.OUT_SIGNEDNESS == 1) && (reg1.bit.OUT_BIT_DEPTH == 1)) {
		return NUE_INT16;
	} else if ((reg1.bit.OUT_SIGNEDNESS == 1) && (reg1.bit.OUT_BIT_DEPTH == 0)) {
		return NUE_INT8;
	} else if ((reg1.bit.OUT_SIGNEDNESS == 0) && (reg1.bit.OUT_BIT_DEPTH == 1)) {
		return NUE_UINT16;
	} else {
		return NUE_UINT8;
	}
}

/**
    NUE Set Kernel 1 Mode

    NUE kernel 1 function selection.

    @param[in] type Kernel 1 type to select function.

    @return None.
*/
ER nue_set_kerl1mode(NUE_SVMKER1_TYPE type)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	switch (type) {
	case NUE_SVMKER1_DOT:
		reg1.bit.SVM_KERNEL1_MODE = 0;
		break;
	case NUE_SVMKER1_RBF:
		reg1.bit.SVM_KERNEL1_MODE = 1;
		break;
	case NUE_SVMKER1_INTER:
		reg1.bit.SVM_KERNEL1_MODE = 2;
		break;
	default:
		nvt_dbg(ERR, "Unknown kernel 1 type: %d!\r\n", type);
		return E_PAR;
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
    NUE Get Kernel 1 Mode

    Get current NUE kernel 1 mode.

    @return NUE kernel 1 mode.
*/
NUE_SVMKER1_TYPE nue_get_kerl1mode(VOID)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return (NUE_SVMKER1_TYPE)reg1.bit.SVM_KERNEL1_MODE;
}

/**
    NUE Set Kernel 2 Mode

    NUE kernel 2 function selection.

    @param[in] type Kernel 2 type to select function.

    @return None.
*/
ER nue_set_kerl2mode(NUE_SVMKER2_TYPE type)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	switch (type) {
	case NUE_SVMKER2_POLY:
		reg1.bit.SVM_KERNEL2_MODE = 0;
		break;
	case NUE_SVMKER2_SIGMOID:
		reg1.bit.SVM_KERNEL2_MODE = 1;
		break;
	default:
		nvt_dbg(ERR, "Unknown kernel 2 type: %d!\r\n", type);
		return E_PAR;
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
    NUE Get Kernel 2 Mode

    Get current NUE kernel 2 mode.

    @return NUE kernel 2 mode.
*/
NUE_SVMKER2_TYPE nue_get_kerl2mode(VOID)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return (NUE_SVMKER2_TYPE)reg1.bit.SVM_KERNEL2_MODE;
}

/**
    NUE Set Result Mode

    NUE result function selection.

    @param[in] type result type to select function.

    @return None.
*/
ER nue_set_rstmode(NUE_SVMRST_TYPE type)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	switch (type) {
	case NUE_SVMRST_FULL:
		reg1.bit.SVM_RESULT_MODE = 0;
		break;
	case NUE_SVMRST_SUB_RHO:
		reg1.bit.SVM_RESULT_MODE = 1;
		break;
	case NUE_SVMRST_ACCRST_SUBRHO:
		reg1.bit.SVM_RESULT_MODE = 2;
		break;
	default:
		nvt_dbg(ERR, "Unknown result output type: %d!\r\n", type);
		return E_PAR;
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
    NUE Get Result Mode

    Get current NUE result mode.

    @return NUE result mode.
*/
NUE_SVMRST_TYPE nue_get_rstmode(VOID)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return (NUE_SVMRST_TYPE)reg1.bit.SVM_RESULT_MODE;
}

/**
    NUE Set Permute mode

    @param[in] mode Decide permute mode.
        \n-@b NUE_PERMUTE_CH_PRIOR: permute with order channel -> width -> height.
        \n-@b NUE_PERMUTE_HEIGHT_PRIOR: permute with order height -> width -> channel.

    @return None.
*/
ER nue_set_permute_mode(NUE_PERMUTE_MODE mode)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	if (mode == NUE_PERMUTE_CH_PRIOR)
		reg1.bit.PERMUTE_MODE = 0;
	else if (mode == NUE_PERMUTE_HEIGHT_PRIOR)
		reg1.bit.PERMUTE_MODE = 1;
	else {
		nvt_dbg(ERR, "Unknown permute mode: %d!\r\n", mode);
		return E_PAR;
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
    NUE Get Permute Mode Status

    Get current NUE Permute Mode status.

    @return NUE Permute Mode status.
*/
NUE_PERMUTE_MODE nue_get_permute_mode(VOID)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return (NUE_PERMUTE_MODE)reg1.bit.PERMUTE_MODE;
}

/**
    NUE Set Permute mode

    @param[in] mode Decide permute mode.
        \n-@b NUE_PERMUTE_CH_PRIOR: permute with order channel -> width -> height.
        \n-@b NUE_PERMUTE_HEIGHT_PRIOR: permute with order height -> width -> channel.

    @return None.
*/
VOID nue_set_permute_shf(INT32 shift)
{
	T_PERMUTE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = PERMUTE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
		
	if (shift < 0) {
		reg1.bit.PERMUTE_SHIFT_SIGNEDNESS = 1;
		reg1.bit.PERMUTE_SHIFT = -shift;
	} else {
		reg1.bit.PERMUTE_SHIFT_SIGNEDNESS = 0;
		reg1.bit.PERMUTE_SHIFT = shift;
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Get Permute Mode Status

    Get current NUE Permute Mode status.

    @return NUE Permute Mode status.
*/
INT32 nue_get_permute_shf(VOID)
{
	T_PERMUTE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = PERMUTE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	if (reg1.bit.PERMUTE_SHIFT_SIGNEDNESS)
		return (-1*reg1.bit.PERMUTE_SHIFT);
	else
		return reg1.bit.PERMUTE_SHIFT;
}

/**
    NUE Set Roipooling mode

    @param[in] mode Decide roipooling mode.
        \n-@b NUE_NORMAL_ROI: normal roipooling.
        \n-@b NUE_PS_ROI: PSROI pooling.

    @return None.
*/
ER nue_set_roipool_mode(NUE_ROI_MODE mode)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	if (mode == NUE_NORMAL_ROI)
		reg1.bit.ROIPOOLING_MODE = 0;
	else if (mode == NUE_PS_ROI)
		reg1.bit.ROIPOOLING_MODE = 1;
	else {
		nvt_dbg(ERR, "Unknown roipooling mode: %d!\r\n", mode);
		return E_PAR;
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
    NUE Get Roipooling Mode Status

    Get current NUE Roipooling Mode status.

    @return NUE Roipooling Mode status.
*/
NUE_ROI_MODE nue_get_roipool_mode(VOID)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return (NUE_ROI_MODE)reg1.bit.ROIPOOLING_MODE;
}

/**
    NUE Set DRAM Output Enable/Disable

    NUE ouput to DRAM enable selection.

    @param[in] enable Decide results output to DRAM.
        \n-@b ENABLE: enable ouput to DRAM.
        \n-@b DISABLE: disable ouput to DRAM.

    @return None.
*/
VOID nue_set_dmao_en(BOOL enable)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	reg1.bit.SVM_DMAOEN = enable;

	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Get DRAM Output Enable Status

    Get current NUE DRAM output enable status.

    @return NUE DRAM output enable status.
*/
BOOL nue_get_dmao_en(VOID)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return reg1.bit.SVM_DMAOEN;
}

/**
    NUE Set DRAM Output Path

    NUE output path to DRAM.

    @param[in] path DRAM output path is selected to output result to DRAM.

    @return None.
*/
ER nue_set_dmao_path(NUE_DMAO_PATH_TYPE path)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	switch (path) {
	case NUE_OUT_RST:
		reg1.bit.SVM_DMAO_PATH = 0;
		break;
	case NUE_OUT_INTERMEDIATE:
		reg1.bit.SVM_DMAO_PATH = 1;
		break;
	case NUE_OUT_KERL2:
		reg1.bit.SVM_DMAO_PATH = 2;
		break;
	case NUE_OUT_KERL1:
		reg1.bit.SVM_DMAO_PATH = 3;
		break;
	default:
		nvt_dbg(ERR, "Unknown DRAM output path: %d!\r\n", path);
		return E_PAR;
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
    NUE Get DRAM Output Path

    Get current NUE DRAM output path.

    @return NUE DRAM output path.
*/
NUE_DMAO_PATH_TYPE nue_get_dmao_path(VOID)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return (NUE_DMAO_PATH_TYPE)reg1.bit.SVM_DMAO_PATH;
}

/**
    NUE Set Kernel Enable/Disable

    NUE kernel enable selection.

    @param[in] enable Decide selected funtions are to be enabled or disabled.
        \n-@b ENABLE: enable selected functions.
        \n-@b DISABLE: disable selected functions.
    @param[in] kerl Indicate the function(s).

    @return None.
*/
VOID nue_set_kerl_en(BOOL enable, UINT32 kerl)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	if (enable) {
		reg1.reg |= kerl;
	} else {
		reg1.reg &= (~kerl);
	}

	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Get Kernel Enable Status

    Get current NUE kernel enable status.

    @return NUE kernel enable status.
*/
UINT32 nue_get_kerl_en(VOID)
{
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	return (NUE_GETDATA(ofs, base_addr) & NUE_ALL_EN);
}

/**
    NUE Set Kernel

    NUE kernel selection.

    @param[in] ker_type nue kernel type selected.

    @return ER Error unknown kernel type.
*/
ER nue_set_ker(NUE_SVMKER_TYPE ker_type)
{
	ER er_code;
	switch (ker_type) {
	case NUE_SVMKER_LINEAR:
		nue_set_kerl_en(DISABLE, (NUE_SVM_KER2_EN | NUE_RELU_EN));
		nue_set_kerl_en(ENABLE, NUE_SVM_CAL_EN);
		nue_set_kerl1mode(NUE_SVMKER1_DOT);
		er_code = E_OK;
		break;
	case NUE_SVMKER_POLY:
		nue_set_kerl_en(DISABLE, NUE_RELU_EN);
		nue_set_kerl_en(ENABLE, (NUE_SVM_KER2_EN | NUE_SVM_CAL_EN));
		nue_set_kerl1mode(NUE_SVMKER1_DOT);
		nue_set_kerl2mode(NUE_SVMKER2_POLY);
		er_code = E_OK;
		break;
	case NUE_SVMKER_RBF:
		nue_set_kerl_en(DISABLE, (NUE_SVM_KER2_EN | NUE_RELU_EN));
		nue_set_kerl_en(ENABLE, NUE_SVM_CAL_EN);
		nue_set_kerl1mode(NUE_SVMKER1_RBF);
		er_code = E_OK;
		break;
	case NUE_SVMKER_SIGMOID:
		nue_set_kerl_en(DISABLE, NUE_RELU_EN);
		nue_set_kerl_en(ENABLE, (NUE_SVM_KER2_EN | NUE_SVM_CAL_EN));
		nue_set_kerl1mode(NUE_SVMKER1_DOT);
		nue_set_kerl2mode(NUE_SVMKER2_SIGMOID);
		er_code = E_OK;
		break;
	case NUE_SVMKER_INTER:
		nue_set_kerl_en(DISABLE, (NUE_SVM_KER2_EN | NUE_RELU_EN));
		nue_set_kerl_en(ENABLE, NUE_SVM_CAL_EN);
		nue_set_kerl1mode(NUE_SVMKER1_INTER);
		er_code = E_OK;
		break;
	default:
		er_code = E_OBJ;
		nvt_dbg(ERR, "Unknown kernel type!\r\n");
		break;
	}

	return er_code;
}

/**
    NUE Get Kernel

    NUE get current kernel type.

    @return NUE_SVMKER_TYPE current NUE kernel type.
*/
NUE_SVMKER_TYPE nue_get_ker(VOID)
{
	NUE_SVMKER_TYPE type = -1;
	BOOL ker2_en = (nue_get_kerl_en() & NUE_SVM_KER2_EN);
	NUE_SVMKER1_TYPE ker1 = nue_get_kerl1mode();
	NUE_SVMKER2_TYPE ker2 = nue_get_kerl2mode();

	if (nue_get_drvmode() != NUE_OPMODE_SVM) {
		nvt_dbg(WRN, "Only support SVM mode: %d!\r\n", nue_get_drvmode());
		return type;
	}

	if ((ker2_en) && (ker1 == NUE_SVMKER1_DOT) && (ker2 == NUE_SVMKER2_POLY)) {
		type = NUE_SVMKER_POLY;
	} else if ((ker2_en) && (ker1 == NUE_SVMKER1_DOT) && (ker2 == NUE_SVMKER2_SIGMOID)) {
		type = NUE_SVMKER_SIGMOID;
	} else if ((!ker2_en) && (ker1 == NUE_SVMKER1_DOT)) {
		type = NUE_SVMKER_LINEAR;
	} else if ((!ker2_en) && (ker1 == NUE_SVMKER1_RBF)) {
		type = NUE_SVMKER_RBF;
	} else if ((!ker2_en) && (ker1 == NUE_SVMKER1_INTER)) {
		type = NUE_SVMKER_INTER;
	} else {
		nvt_dbg(ERR, "Wrong kernel enable setting: %x!\r\n", (unsigned int)nue_get_kerl_en());
	}

	return type;
}

/**
    NUE Set Input Starting Addresses

    Set two DMA input starting addresses.

    @param[in] addr0 DMA input buffer (DMA to NUE) starting address 0.
    @param[in] addr1 DMA input buffer (DMA to NUE) starting address 1.

    @return None.
*/
VOID nue_set_dmain_addr(UINT32 addr0, UINT32 addr1)
{
	T_DMA_TO_NUE_REGISTER0 localreg0;
	T_DMA_TO_NUE_REGISTER1 localreg1;
	UINT32 base_addr = 0;
	UINT32 ofs0 = DMA_TO_NUE_REGISTER0_OFS;
	UINT32 ofs1 = DMA_TO_NUE_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	localreg0.reg = NUE_GETDATA(ofs0, base_addr);
	localreg0.bit.DRAM_SAI0 = addr0;

	localreg1.reg = NUE_GETDATA(ofs1, base_addr);
	localreg1.bit.DRAM_SAI1 = addr1 >> 2;

	NUE_SETDATA(ofs0, localreg0.reg, base_addr);
	NUE_SETDATA(ofs1, localreg1.reg, base_addr);
}

/**
    NUE Set SVM Starting Addresses

    Set three DMA input starting addresses.

    @param[in] addr0 DMA input sv buffer (DMA to NUE) starting address 0.
    @param[in] addr1 DMA input alpha buffer (DMA to NUE) starting address 1.
    @param[in] addr2 DMA input rho buffer (DMA to NUE) starting address 2.

    @return None.
*/
VOID nue_set_dmain_svmaddr(UINT32 addr0, UINT32 addr1, UINT32 addr2)
{
	T_DMA_TO_NUE_REGISTER2 localreg0;
	T_DMA_TO_NUE_REGISTER3 localreg1;
	T_DMA_TO_NUE_REGISTER4 localreg2;
	UINT32 base_addr = 0;
	UINT32 ofs0 = DMA_TO_NUE_REGISTER2_OFS;
	UINT32 ofs1 = DMA_TO_NUE_REGISTER3_OFS;
	UINT32 ofs2 = DMA_TO_NUE_REGISTER4_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	localreg0.reg = NUE_GETDATA(ofs0, base_addr);
	localreg0.bit.DRAM_SAISV = addr0 >> 2;

	localreg1.reg = NUE_GETDATA(ofs1, base_addr);
	localreg1.bit.DRAM_SAIALPHA = addr1 >> 2;

	localreg2.reg = NUE_GETDATA(ofs2, base_addr);
	localreg2.bit.DRAM_SAIRHO = addr2 >> 2;

	NUE_SETDATA(ofs0, localreg0.reg, base_addr);
	NUE_SETDATA(ofs1, localreg1.reg, base_addr);
	NUE_SETDATA(ofs2, localreg2.reg, base_addr);
}

/**
    NUE Set ROI Starting Addresses

    Set one DMA input starting addresses.

    @param[in] addr0 DMA input roi buffer (DMA to NUE) starting address 0.

    @return None.
*/
VOID nue_set_dmain_roiaddr(UINT32 addr0)
{
	T_DMA_TO_NUE_REGISTER5 localreg0;
	UINT32 base_addr = 0;
	UINT32 ofs = DMA_TO_NUE_REGISTER5_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	localreg0.reg = NUE_GETDATA(ofs, base_addr);
	localreg0.bit.DRAM_SAIROI = addr0 >> 2;

	NUE_SETDATA(ofs, localreg0.reg, base_addr);
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
    NUE Set DMA input Linked List Starting Addresses

    Set one DMA input starting addresses.

    @param[in] addr0 DMA input linked list buffer (DMA to NUE) starting address 0.

    @return None.
*/
VOID nue_set_dmain_lladdr(UINT32 addr0)
{
	T_DMA_TO_NUE_REGISTER6 localreg0;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = DMA_TO_NUE_REGISTER6_OFS;

	localreg0.reg = NUE_GETDATA(ofs, base_addr);
	localreg0.bit.DRAM_SAILL = addr0 >> 2;

	NUE_SETDATA(ofs, localreg0.reg, base_addr);
}

VOID nue_set_dmain_lladdr_base(UINT32 addr0)
{
	T_NUE_LL_BASE_ADDR_REGISTER localreg0;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_LL_BASE_ADDR_REGISTER_OFS;

	localreg0.reg = NUE_GETDATA(ofs, base_addr);
	localreg0.bit.LL_BASE_ADDR = addr0;

	NUE_SETDATA(ofs, localreg0.reg, base_addr);
}

/**
   	NUE Get DMA input Linked List Starting Addresses

    Get one DMA input starting addresses.

    @return specifed DMA input linked list starting address.
*/
UINT32 nue_get_dmain_lladdr(VOID)
{
	UINT32 addr = 0, phy_addr = 0;
	T_DMA_TO_NUE_REGISTER6 localreg0;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = DMA_TO_NUE_REGISTER6_OFS;

	localreg0.reg = NUE_GETDATA(ofs, base_addr);
	phy_addr = localreg0.bit.DRAM_SAILL << 2;

#if 0   //(NUE_DMA_CACHE_HANDLE == 1)
	addr = dma_getNonCacheAddr(phy_addr);
#else
	addr = phy_addr;
#endif
	return addr;
}

UINT32 nue_get_dmain_lladdr_base(VOID)
{
	UINT32 addr=0, phy_addr=0;
	T_NUE_LL_BASE_ADDR_REGISTER localreg0;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_LL_BASE_ADDR_REGISTER_OFS;

	localreg0.reg = NUE_GETDATA(ofs, base_addr);
	phy_addr = localreg0.bit.LL_BASE_ADDR;

	#if 0
	#else
	addr = phy_addr;
	#endif

	return addr;
}

/**
    NUE Set Kmeans Quantization Starting Addresses

    Set one DMA input starting addresses.

    @param[in] addr0 DMA input kmeans quantization buffer (DMA to NUE) starting address 0.

    @return None.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
VOID nue_set_dmain_kqaddr(UINT32 addr0)
{
	T_DMA_TO_NUE_REGISTER7 localreg0;
	UINT32 base_addr = 0;
	UINT32 ofs = DMA_TO_NUE_REGISTER7_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	localreg0.reg = NUE_GETDATA(ofs, base_addr);
	localreg0.bit.DRAM_SAIKQ = addr0 >> 2;

	NUE_SETDATA(ofs, localreg0.reg, base_addr);
}

/**
    NUE Get DMA input starting address

    Get specifed DMA input buffer starting address.

    @param[in] bufId input ppb ID.

    @return specifed DMA input buffer starting address.
*/
UINT32 nue_get_dmain_addr(NUE_IN_BUFID bufid)
{
	T_DMA_TO_NUE_REGISTER0 localreg0;
	T_DMA_TO_NUE_REGISTER1 localreg1;
	T_DMA_TO_NUE_REGISTER2 localreg2;
	T_DMA_TO_NUE_REGISTER3 localreg3;
	T_DMA_TO_NUE_REGISTER4 localreg4;
	T_DMA_TO_NUE_REGISTER5 localreg5;
	T_DMA_TO_NUE_REGISTER7 localreg7;
	UINT32 base_addr = 0;
	UINT32 ofs0 = DMA_TO_NUE_REGISTER0_OFS;
	UINT32 ofs1 = DMA_TO_NUE_REGISTER1_OFS;
	UINT32 ofs2 = DMA_TO_NUE_REGISTER2_OFS;
	UINT32 ofs3 = DMA_TO_NUE_REGISTER3_OFS;
	UINT32 ofs4 = DMA_TO_NUE_REGISTER4_OFS;
	UINT32 ofs5 = DMA_TO_NUE_REGISTER5_OFS;
	UINT32 ofs7 = DMA_TO_NUE_REGISTER7_OFS;
	UINT32 addr = 0, phy_addr = 0;
	base_addr = NUE_IOADDR_REG_BASE;

	switch (bufid) {
	case NUE_IN0_BUF:
		localreg0.reg   = NUE_GETDATA(ofs0, base_addr);
		phy_addr       = localreg0.bit.DRAM_SAI0;
		break;
	case NUE_IN1_BUF:
		localreg1.reg   = NUE_GETDATA(ofs1, base_addr);
		phy_addr       = localreg1.bit.DRAM_SAI1 << 2;
		break;
	case NUE_SV_BUF:
		localreg2.reg   = NUE_GETDATA(ofs2, base_addr);
		phy_addr       = localreg2.bit.DRAM_SAISV << 2;
		break;
	case NUE_ALPHA_BUF:
		localreg3.reg   = NUE_GETDATA(ofs3, base_addr);
		phy_addr       = localreg3.bit.DRAM_SAIALPHA << 2;
		break;
	case NUE_RHO_BUF:
		localreg4.reg   = NUE_GETDATA(ofs4, base_addr);
		phy_addr       = localreg4.bit.DRAM_SAIRHO << 2;
		break;
	case NUE_ROI_BUF:
		localreg5.reg   = NUE_GETDATA(ofs5, base_addr);
		phy_addr       = localreg5.bit.DRAM_SAIROI << 2;
		break;
	case NUE_KQ_BUF:
		localreg7.reg   = NUE_GETDATA(ofs7, base_addr);
		phy_addr       = localreg7.bit.DRAM_SAIKQ << 2;
		break;
	default:
		nvt_dbg(ERR, "Wrong input biffer ID: %d\r\n", bufid);
		return E_CTX;
	}

#if 0   //(NUE_DMA_CACHE_HANDLE == 1)
	addr = dma_getNonCacheAddr(phy_addr);
#else
	addr = phy_addr;
#endif

	return addr;
}

/**
    NUE Set Output Starting Addresses

    Set one DMA output starting addresses.

    @param[in] addr0 DMA output result (NUE to DMA) starting address 0.

    @return None.
*/
VOID nue_set_dmaout_addr(UINT32 addr0)
{
	T_NUE_TO_DMA_RESULT_REGISTER0 localreg0;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_TO_DMA_RESULT_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	localreg0.reg = NUE_GETDATA(ofs, base_addr);
	localreg0.bit.DRAM_SAOR = addr0;

	NUE_SETDATA(ofs, localreg0.reg, base_addr);
}

/**
    NUE Get DMA output starting address

    Get specifed DMA output result (NUE to DMA) starting address.

    @return specifed DMA output result (NUE to DMA) starting address.
*/
UINT32 nue_get_dmaout_addr(VOID)
{
	T_NUE_TO_DMA_RESULT_REGISTER0 localreg0;
	UINT32 addr, phy_addr;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_TO_DMA_RESULT_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	localreg0.reg = NUE_GETDATA(ofs, base_addr);
	phy_addr = localreg0.bit.DRAM_SAOR;

#if 0   //(NUE_DMA_CACHE_HANDLE == 1)
	addr = dma_getNonCacheAddr(phy_addr);
#else
	addr = phy_addr;
#endif

	return addr;
}

/**
    NUE Set DMA input line offset

    Set one DMA input line offset.

    @param[in] ilofs0 DMA input line offset.

    @return None.
*/
VOID nue_set_dmain_lofs(UINT32 ilofs0)
{
	T_INPUT_FEATURE_LINE_OFFSET_REGISTER localreg0;
	UINT32 base_addr = 0;
	UINT32 ofs = INPUT_FEATURE_LINE_OFFSET_REGISTER_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	localreg0.reg = NUE_GETDATA(ofs, base_addr);
	localreg0.bit.DRAM_OFSI = ilofs0 >> 2;

	NUE_SETDATA(ofs, localreg0.reg, base_addr);
}

/**
    NUE Get DMA input line offset

    Get specifed DMA input line offset.

    @return specifed DMA input line offset.
*/
UINT32 nue_get_dmain_lofs(VOID)
{
	T_INPUT_FEATURE_LINE_OFFSET_REGISTER localreg0;
	UINT32 base_addr = 0;
	UINT32 ofs = INPUT_FEATURE_LINE_OFFSET_REGISTER_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	localreg0.reg = NUE_GETDATA(ofs, base_addr);

	return (localreg0.bit.DRAM_OFSI << 2);
}

/**
    NUE Set DMA output line offset

    Set one DMA output line offset.

    @param[in] olofs0 DMA output line offset.

    @return None.
*/
VOID nue_set_dmaout_lofs(UINT32 olofs0)
{
	T_OUTPUT_FEATURE_LINE_OFFSET_REGISTER localreg0;
	UINT32 base_addr = 0;
	UINT32 ofs = OUTPUT_FEATURE_LINE_OFFSET_REGISTER_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	localreg0.reg = NUE_GETDATA(ofs, base_addr);
	localreg0.bit.DRAM_OFSO = olofs0 ;

	NUE_SETDATA(ofs, localreg0.reg, base_addr);
}

/**
    NUE Get DMA output line offset

    Get specifed DMA output line offset.

    @return specifed DMA output line offset.
*/
UINT32 nue_get_dmaout_lofs(VOID)
{
	T_OUTPUT_FEATURE_LINE_OFFSET_REGISTER localreg0;
	UINT32 base_addr = 0;
	UINT32 ofs = OUTPUT_FEATURE_LINE_OFFSET_REGISTER_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	localreg0.reg = NUE_GETDATA(ofs, base_addr);

	return (localreg0.bit.DRAM_OFSO << 2);
}

/**
    NUE Set Svm Size

    NUE set svm input size.

    @param[in] p_size svm input size.

    @return ER Error size is out of range.
*/
ER nue_set_svmsize(NUE_SVM_IN_SIZE *p_size)
{
	T_INPUT_SIZE_REGISTER0 reg1;
	T_INPUT_SIZE_REGISTER1 reg2;
	T_INPUT_SIZE_REGISTER2 reg3;
	UINT32 base_addr = 0;
	UINT32 ofs0 = INPUT_SIZE_REGISTER0_OFS;
	UINT32 ofs1 = INPUT_SIZE_REGISTER1_OFS;
	UINT32 ofs2 = INPUT_SIZE_REGISTER2_OFS;
	UINT32 sv_w, sv_h, insize, channel, obj_num, kerl;
	NUE_SVMRST_TYPE rst_type;

	if (p_size == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	sv_w    	= p_size->sv_w;
	sv_h   		= p_size->sv_h;
	insize     	= p_size->insize;
	channel    	= p_size->channel;
	obj_num     = p_size->objnum;
	kerl        = nue_get_kerl_en();
	rst_type 	= nue_get_rstmode();

	base_addr = NUE_IOADDR_REG_BASE;

	if (nue_get_engmode() != NUE_SVM) {
		nvt_dbg(ERR, "This function only suppoer svm mode\r\n");
		return E_CTX;
	}

	if ((sv_w % FOR_USER_98520_NUE_SVM_WIDTH_UNIT) != 0) {
		UINT32 tmp_offset = 0xFFFFFFFF & (~(FOR_USER_98520_NUE_SVM_WIDTH_UNIT-1));
		UINT32 idea_sv_w = (sv_w & tmp_offset) < FOR_USER_98520_NUE_SVM_WIDTH_UNIT ? FOR_USER_98520_NUE_SVM_WIDTH_UNIT : (sv_w & tmp_offset);
		nvt_dbg(WRN, "SV width %d must be %d\r\n", (int)sv_w, (int)idea_sv_w);
		return E_PAR;
	} else if (sv_w < FOR_USER_98520_NUE_SV_WIDTH_MIN) {
		nvt_dbg(WRN, "SV width %d must > %d\r\n", (int)sv_w, FOR_USER_98520_NUE_SV_WIDTH_MIN);
		return E_PAR;
	} else if (sv_w > FOR_USER_98520_NUE_SV_WIDTH_MAX) {
		nvt_dbg(WRN, "SV width %d must < %d\r\n", (int)sv_w, FOR_USER_98520_NUE_SV_WIDTH_MAX);
		return E_PAR;
	}

	if ((sv_h > FOR_USER_98520_NUE_SV_HEIGHT_MAX2) && ((kerl & NUE_SVM_CAL_EN) || (rst_type == NUE_SVMRST_SUB_RHO))) {
		nvt_dbg(WRN, "If you use alpha or rho, SV height %d must < %d\r\n", (int)sv_h, FOR_USER_98520_NUE_SV_HEIGHT_MAX2);
		return E_PAR;
	} else if (sv_h < FOR_USER_98520_NUE_SV_HEIGHT_MIN) {
		nvt_dbg(WRN, "SV height %d must > %d\r\n", (int)sv_h, FOR_USER_98520_NUE_SV_HEIGHT_MIN);
		return E_PAR;
	} else if (sv_h > FOR_USER_98520_NUE_SV_HEIGHT_MAX1) {
		nvt_dbg(WRN, "SV height %d must < %d\r\n", (int)sv_h, FOR_USER_98520_NUE_SV_HEIGHT_MAX1);
		return E_PAR;
	}

	if (channel < FOR_USER_98520_NUE_SVM_CHANNEL_MIN) {
		nvt_dbg(WRN, "channel %d must > %d\r\n", (int)channel, FOR_USER_98520_NUE_SVM_CHANNEL_MIN);
		return E_PAR;
	} else if (channel > FOR_USER_98520_NUE_SVM_CHANNEL_MAX) {
		nvt_dbg(WRN, "channel %d must < %d\r\n", (int)channel, FOR_USER_98520_NUE_SVM_CHANNEL_MAX);
		return E_PAR;
	}

	if ((insize * channel) != sv_w && ((nue_get_kerl_en() & NUE_SVM_INTERLACE_EN) > 0)) {
		nvt_dbg(ERR, "sv width(%d) must be = input data size(%d) * channel(%d)!\r\n", (int)sv_w, (int)insize, (int)channel);
		return E_CTX;
	}

	if (obj_num < FOR_USER_98520_NUE_OBJ_NUM_MIN) {
		nvt_dbg(WRN, "Object number %d must be > %d\r\n", (int)obj_num, FOR_USER_98520_NUE_OBJ_NUM_MIN);
		return E_PAR;
	} else if (obj_num > FOR_USER_98520_NUE_OBJ_NUM_MAX) {
		nvt_dbg(WRN, "Object number %d must be < %d\r\n", (int)obj_num, FOR_USER_98520_NUE_OBJ_NUM_MAX);
		return E_PAR;
	}

	reg1.reg = NUE_GETDATA(ofs0, base_addr);
	reg1.bit.NUE_WIDTH  = sv_w;
	reg1.bit.NUE_HEIGHT = sv_h;

	reg2.reg = NUE_GETDATA(ofs1, base_addr);
	reg2.bit.NUE_SVM_INSIZE = insize;
	reg2.bit.NUE_CHANNEL    = channel;

	reg3.reg = NUE_GETDATA(ofs2, base_addr);
	reg3.bit.OBJ_NUM = obj_num;

	NUE_SETDATA(ofs0, reg1.reg, base_addr);
	NUE_SETDATA(ofs1, reg2.reg, base_addr);
	NUE_SETDATA(ofs2, reg3.reg, base_addr);

	return E_OK;
}

/**
    NUE Get svm Size

    Get current svm input size.

    @return NUE_SVM_IN_SIZE svm input size.
*/
NUE_SVM_IN_SIZE nue_get_svmsize(VOID)
{
	NUE_SVM_IN_SIZE in_size;
	T_INPUT_SIZE_REGISTER0 reg1;
	T_INPUT_SIZE_REGISTER1 reg2;
	T_INPUT_SIZE_REGISTER2 reg3;
	UINT32 base_addr = 0;
	UINT32 ofs0 = INPUT_SIZE_REGISTER0_OFS;
	UINT32 ofs1 = INPUT_SIZE_REGISTER1_OFS;
	UINT32 ofs2 = INPUT_SIZE_REGISTER2_OFS;

	base_addr = NUE_IOADDR_REG_BASE;

	reg1.reg = NUE_GETDATA(ofs0, base_addr);
	in_size.sv_w    = reg1.bit.NUE_WIDTH;
	in_size.sv_h   = reg1.bit.NUE_HEIGHT;

	reg2.reg = NUE_GETDATA(ofs1, base_addr);
	in_size.insize     = reg2.bit.NUE_SVM_INSIZE;
	in_size.channel    = reg2.bit.NUE_CHANNEL;

	reg3.reg = NUE_GETDATA(ofs2, base_addr);
	in_size.objnum     = reg3.bit.OBJ_NUM;
	return in_size;
}

/**
    NUE Set Input Size

    NUE set input Size.

    @param[in] p_size input size.

    @return ER Error size is out of range.
*/
ER nue_set_insize(NUE_SIZE *p_size)
{
	T_INPUT_SIZE_REGISTER0 reg1;
	T_INPUT_SIZE_REGISTER1 reg2;
	UINT32 base_addr = 0;
	UINT32 ofs0 = INPUT_SIZE_REGISTER0_OFS;
	UINT32 ofs1 = INPUT_SIZE_REGISTER1_OFS;
	UINT32 width, height, channel;
	UINT32 permute_mode;

	if (p_size == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	width   = p_size->width;
	height  = p_size->height;
	channel = p_size->channel;

	base_addr = NUE_IOADDR_REG_BASE;
	if (nue_get_engmode() == NUE_SVM) {
		nvt_dbg(ERR, "This function not suppoer svm mode\r\n");
		return E_CTX;
	}

	switch (nue_get_engmode()) {
	case NUE_ROIPOOLING:
		if (width < FOR_USER_98520_NUE_ROI_WIDTH_MIN) {
			nvt_dbg(WRN, "roi-pooling width %d must >= %d\r\n", (int)width, FOR_USER_98520_NUE_ROI_WIDTH_MIN);
			return E_PAR;
		} else if (width > FOR_USER_98520_NUE_ROI_WIDTH_MAX) {
			nvt_dbg(WRN, "roi-pooling width %d must <= %d\r\n", (int)width, FOR_USER_98520_NUE_ROI_WIDTH_MAX);
			return E_PAR;
		}

		if (height < FOR_USER_98520_NUE_ROI_HEIGHT_MIN) {
			nvt_dbg(WRN, "roi-pooling height %d must >= %d\r\n", (int)height, FOR_USER_98520_NUE_ROI_HEIGHT_MIN);
			return E_PAR;
		} else if (height > FOR_USER_98520_NUE_ROI_HEIGHT_MAX) {
			nvt_dbg(WRN, "roi-pooling height %d must <= %d\r\n", (int)height, FOR_USER_98520_NUE_ROI_HEIGHT_MAX);
			return E_PAR;
		}

		if (channel < FOR_USER_98520_NUE_ROI_CHANNEL_MIN) {
			nvt_dbg(WRN, "roi-pooling channel %d must >= %d\r\n", (int)channel, FOR_USER_98520_NUE_ROI_CHANNEL_MIN);
			return E_PAR;
		} else if (channel > FOR_USER_98520_NUE_ROI_CHANNEL_MAX) {
			nvt_dbg(WRN, "roi-pooling channel %d must <= %d\r\n", (int)channel, FOR_USER_98520_NUE_ROI_CHANNEL_MAX);
			return E_PAR;
		}
		break;
	case NUE_REORGANIZATION:
		if (width < FOR_USER_98520_NUE_REORG_WIDTH_MIN) {
			nvt_dbg(WRN, "reorganize width %d must >= %d\r\n", (int)width, FOR_USER_98520_NUE_REORG_WIDTH_MIN);
			return E_PAR;
		} else if (width > FOR_USER_98520_NUE_REORG_WIDTH_MAX) {
			nvt_dbg(WRN, "reorganize width %d must <= %d\r\n", (int)width, FOR_USER_98520_NUE_REORG_WIDTH_MAX);
			return E_PAR;
		}

		if (height < FOR_USER_98520_NUE_REORG_HEIGHT_MIN) {
			nvt_dbg(WRN, "reorganize height %d must >= %d\r\n", (int)height, FOR_USER_98520_NUE_REORG_HEIGHT_MIN);
			return E_PAR;
		} else if (height > FOR_USER_98520_NUE_REORG_HEIGHT_MAX) {
			nvt_dbg(WRN, "reorganize height %d must <= %d\r\n", (int)height, FOR_USER_98520_NUE_REORG_HEIGHT_MAX);
			return E_PAR;
		}

		if (channel < FOR_USER_98520_NUE_REORG_CHANNEL_MIN) {
			nvt_dbg(WRN, "reorganize channel %d must >= %d\r\n", (int)channel, FOR_USER_98520_NUE_REORG_CHANNEL_MIN);
			return E_PAR;
		} else if (channel > FOR_USER_98520_NUE_REORG_CHANNEL_MAX) {
			nvt_dbg(WRN, "reorganize channel %d must <= %d\r\n", (int)channel, FOR_USER_98520_NUE_REORG_CHANNEL_MAX);
			return E_PAR;
		}

		if (width*height >= FOR_USER_98520_NUE_CH_SIZE_MAX) {
			nvt_dbg(ERR, "reorganize, width * height should < %d\r\n", FOR_USER_98520_NUE_CH_SIZE_MAX);
			return E_CTX;
		}

		if ((width % 2) || (height % 2)) {
			nvt_dbg(WRN, "reorganize input size must be 2x: (w=%d, h=%d)\r\n", (int)width, (int)height);
			return E_PAR;
		}
		break;
	case NUE_PERMUTE:
		permute_mode = nue_get_permute_mode();
		if (permute_mode == NUE_PERMUTE_CH_PRIOR) {
			if (width == 1 && height == 1) {
				nvt_dbg(ERR, "permute channel prior mode, width & height cannot be 1 at same time\r\n");
				//DBG_ERR("permute channel prior mode, width & height cannot be 1 at same time\r\n");
				return E_CTX;
			} 
			if (width*height >= FOR_USER_98520_NUE_CH_SIZE_MAX) {
				nvt_dbg(ERR, "permute channel prior mode, width * height should < %d\r\n", FOR_USER_98520_NUE_CH_SIZE_MAX);
				return E_CTX;
			}
		}
		else {
			if (height == 1 || width == 1) {
				nvt_dbg(ERR, "permute height prior mode, width & height should > 1\r\n");
				//DBG_ERR("permute height prior mode, height should > 1\r\n");
				return E_CTX;
			}
			/*if (height == 1) {
				nvt_dbg(ERR, "permute height prior mode, height should > 1\r\n");
				//DBG_ERR("permute height prior mode, height should > 1\r\n");
				return E_CTX;
			} else if (channel == 1 && width == 1) {
				nvt_dbg(ERR, "permute height prior mode, width & channel cannot be 1 at same time\r\n");
				//DBG_ERR("permute height prior mode, width & channel cannot be 1 at same time\r\n");
				return E_CTX;
			}*/
		}
		if (width < FOR_USER_98520_NUE_PERMUTE_WIDTH_MIN) {
			nvt_dbg(WRN, "permute width %d must >= %d\r\n", (int)width, FOR_USER_98520_NUE_PERMUTE_WIDTH_MIN);
			return E_PAR;
		} else if (width > FOR_USER_98520_NUE_PERMUTE_WIDTH_MAX) {
			nvt_dbg(WRN, "permute width %d must <= %d\r\n", (int)width, FOR_USER_98520_NUE_PERMUTE_WIDTH_MAX);
			return E_PAR;
		}

		if (height < FOR_USER_98520_NUE_PERMUTE_HEIGHT_MIN) {
			nvt_dbg(WRN, "permute height %d must >= %d\r\n", (int)height, FOR_USER_98520_NUE_PERMUTE_HEIGHT_MIN);
			return E_PAR;
		} else if (height > FOR_USER_98520_NUE_PERMUTE_HEIGHT_MAX) {
			nvt_dbg(WRN, "permute height %d must <= %d\r\n", (int)height, FOR_USER_98520_NUE_PERMUTE_HEIGHT_MAX);
			return E_PAR;
		}
		if ((width == FOR_USER_98520_NUE_PERMUTE_WIDTH_MIN) &&(height == FOR_USER_98520_NUE_PERMUTE_HEIGHT_MIN)) {
			nvt_dbg(WRN, "permute cannot set width = 1 and height = 1 at the same time\r\n");
			return E_PAR;
		}
		if (channel < FOR_USER_98520_NUE_PERMUTE_CHANNEL_MIN) {
			nvt_dbg(WRN, "permute channel %d must be >= %d\r\n", (int)channel, FOR_USER_98520_NUE_PERMUTE_CHANNEL_MIN);
			return E_PAR;
		} 
		if (permute_mode == 0) {
			if (channel > FOR_USER_98520_NUE_PERMUTE_MODE0_CHANNEL_MAX) {
				nvt_dbg(WRN, "permute mode 0 channel %d must be <= %d\r\n", (int)channel, FOR_USER_98520_NUE_PERMUTE_MODE0_CHANNEL_MAX);
				return E_PAR;
			}
		}
		else {
			if (channel > FOR_USER_98520_NUE_PERMUTE_MODE1_CHANNEL_MAX) {
				nvt_dbg(WRN, "permute mode 1 channel %d must be <= %d\r\n", (int)channel, FOR_USER_98520_NUE_PERMUTE_MODE1_CHANNEL_MAX);
				return E_PAR;
			}
		}
		break;
	case NUE_ANCHOR:
		if (width < FOR_USER_98520_NUE_ANCHOR_WIDTH_MIN) {
			nvt_dbg(WRN, "anchor width %d must be >= %d\r\n", (int)width, FOR_USER_98520_NUE_ANCHOR_WIDTH_MIN);
			return E_PAR;
		} else if (width > FOR_USER_98520_NUE_ANCHOR_WIDTH_MAX) {
			nvt_dbg(WRN, "anchor width %d must be <= %d\r\n", (int)width, FOR_USER_98520_NUE_ANCHOR_WIDTH_MAX);
			return E_PAR;
		}

		if (height < FOR_USER_98520_NUE_ANCHOR_HEIGHT_MIN) {
			nvt_dbg(WRN, "anchor height %d must be >= %d\r\n", (int)height, FOR_USER_98520_NUE_ANCHOR_HEIGHT_MIN);
			return E_PAR;
		} else if (height > FOR_USER_98520_NUE_ANCHOR_HEIGHT_MAX) {
			nvt_dbg(WRN, "anchor height %d must be <= %d\r\n", (int)height, FOR_USER_98520_NUE_ANCHOR_HEIGHT_MAX);
			return E_PAR;
		}

		if (channel < FOR_USER_98520_NUE_ANCHOR_CHANNEL_MIN) {
			nvt_dbg(WRN, "anchor channel %d must be >= %d\r\n", (int)channel, FOR_USER_98520_NUE_ANCHOR_CHANNEL_MIN);
			return E_PAR;
		} else if (channel > FOR_USER_98520_NUE_ANCHOR_CHANNEL_MAX) {
			nvt_dbg(WRN, "anchor channel %d must be <= %d\r\n", (int)channel, FOR_USER_98520_NUE_ANCHOR_CHANNEL_MAX);
			return E_PAR;
		}
		
		if ((channel % 4) > 0) {
			nvt_dbg(WRN, "anchor channel %d must be multiple of 4\r\n", (int)channel);
			return E_PAR;
		}

		if (width*height >= FOR_USER_98520_NUE_CH_SIZE_MAX) {
			nvt_dbg(ERR, "anchor, width * height should < %d\r\n", FOR_USER_98520_NUE_CH_SIZE_MAX);
			return E_CTX;
		}

		break;
	case NUE_SOFTMAX:
		if (width < FOR_USER_98520_NUE_SOFTMAX_WIDTH_MIN) {
			nvt_dbg(WRN, "softmax width %d must be >= %d\r\n", (int)width, FOR_USER_98520_NUE_SOFTMAX_WIDTH_MIN);
			return E_PAR;
		} else if (width > FOR_USER_98520_NUE_SOFTMAX_WIDTH_MAX) {
			nvt_dbg(WRN, "softmax width %d must be <= %d\r\n", (int)width, FOR_USER_98520_NUE_SOFTMAX_WIDTH_MAX);
			return E_PAR;
		}

		if (height < FOR_USER_98520_NUE_SOFTMAX_HEIGHT_MIN) {
			nvt_dbg(WRN, "softmax height %d must be >= %d\r\n", (int)height, FOR_USER_98520_NUE_SOFTMAX_HEIGHT_MIN);
			return E_PAR;
		} else if (height > FOR_USER_98520_NUE_SOFTMAX_HEIGHT_MAX) {
			nvt_dbg(WRN, "softmax height %d must be <= %dr\n", (int)height, FOR_USER_98520_NUE_SOFTMAX_HEIGHT_MAX);
			return E_PAR;
		}

		if (channel < FOR_USER_98520_NUE_SOFTMAX_CHANNEL_MIN) {
			nvt_dbg(WRN, "softmax channel %d must be >= %d\r\n", (int)channel, FOR_USER_98520_NUE_SOFTMAX_CHANNEL_MIN);
			return E_PAR;
		} else if (channel > FOR_USER_98520_NUE_SOFTMAX_CHANNEL_MAX) {
			nvt_dbg(WRN, "softmax channel %d must be <= %d\r\n", (int)channel, FOR_USER_98520_NUE_SOFTMAX_CHANNEL_MAX);
			return E_PAR;
		}

		if (width*height >= FOR_USER_98520_NUE_CH_SIZE_MAX) {
			nvt_dbg(ERR, "softmax, width * height should < %d\r\n", FOR_USER_98520_NUE_CH_SIZE_MAX);
			return E_CTX;
		}

		break;
	default:
		nvt_dbg(ERR, "Unknown NUE engine mode: %d\r\n", nue_get_engmode());
		return E_CTX;
	}

	reg1.reg = NUE_GETDATA(ofs0, base_addr);
	reg1.bit.NUE_WIDTH      = width;
	reg1.bit.NUE_HEIGHT     = height;

	reg2.reg = NUE_GETDATA(ofs1, base_addr);
	reg2.bit.NUE_CHANNEL    = channel;

	NUE_SETDATA(ofs0, reg1.reg, base_addr);
	NUE_SETDATA(ofs1, reg2.reg, base_addr);

	return E_OK;
}

/**
    NUE Get input Size

    Get current input size.

    @param[out] p_size input size.

    @return None.
*/
NUE_SIZE nue_get_insize(VOID)
{
	NUE_SIZE in_size;
	T_INPUT_SIZE_REGISTER0 reg1;
	T_INPUT_SIZE_REGISTER1 reg2;
	UINT32 base_addr = 0;
	UINT32 ofs0 = INPUT_SIZE_REGISTER0_OFS;
	UINT32 ofs1 = INPUT_SIZE_REGISTER1_OFS;

	base_addr = NUE_IOADDR_REG_BASE;

	reg1.reg = NUE_GETDATA(ofs0, base_addr);
	in_size.width      = reg1.bit.NUE_WIDTH;
	in_size.height     = reg1.bit.NUE_HEIGHT;
	reg2.reg = NUE_GETDATA(ofs1, base_addr);
	in_size.channel    = reg2.bit.NUE_CHANNEL;
	return in_size;
}

/**
    NUE Set roi Number

    NUE set roi number in roi-pooling mode.

    @param[in] uiNum number of roi.

    @return None.
*/
ER nue_set_roinum(UINT32 roi_num)
{
	T_INPUT_SIZE_REGISTER2 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = INPUT_SIZE_REGISTER2_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	if (roi_num > FOR_USER_98520_NUE_ROI_NUM_MAX) {
		nvt_dbg(WRN, "ROI number %d is illegal! max = %d\r\n", (int)roi_num, FOR_USER_98520_NUE_ROI_NUM_MAX);
		return E_PAR;
	} else if (roi_num < FOR_USER_98520_NUE_ROI_NUM_MIN) {
		nvt_dbg(WRN, "ROI number %d is illegal! min = %d\r\n", (int)roi_num, FOR_USER_98520_NUE_ROI_NUM_MIN);
		return E_PAR;
	}

	reg1.bit.ROI_NUM   = roi_num;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
    NUE Get roi Number

    Get number of roi in roi-pooling mode.

    @return UINT32 number of roi.
*/
UINT32 nue_get_roinum(VOID)
{
	T_INPUT_SIZE_REGISTER2 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = INPUT_SIZE_REGISTER2_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return reg1.bit.ROI_NUM;
}

/**
    NUE Set Input Refresh

    NUE set svm input to refresh.

    @param[in] enable decide the svm input to refresh.

    @return None.
*/
VOID nue_set_in_rfh(BOOL enable)
{
	T_INPUT_SIZE_REGISTER2 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = INPUT_SIZE_REGISTER2_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	reg1.bit.IN_RFH   = enable;

	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Get Input Refresh

    Get svm input refresh status.

    @return BOOL svm input refresh status.
*/
BOOL nue_get_in_rfh(VOID)
{
	T_INPUT_SIZE_REGISTER2 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = INPUT_SIZE_REGISTER2_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return reg1.bit.IN_RFH;
}

/**
    NUE Set Parameteres

    NUE set svm kernel parameters.

    @param[in] p_parm svm kernel parameters.

    @return ER Error svm parameters are out of range.
*/
ER nue_set_parm(NUE_SVMKERL_PARM *p_parm)
{
	T_KERNEL_REGISTER0 reg1;
	T_KERNEL_REGISTER1 reg2;
	T_RESULT_REGISTER0 reg3;
	UINT32 base_addr = 0;
	UINT32 ofs1 = KERNEL_REGISTER0_OFS;
	UINT32 ofs2 = KERNEL_REGISTER1_OFS;
	UINT32 ofs3 = RESULT_REGISTER0_OFS;

	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	base_addr = NUE_IOADDR_REG_BASE;

	if (p_parm->ft_shf > FOR_USER_98520_NUE_KER_FT_SHIFT_MAX) {
		nvt_dbg(WRN, "Feature shift %d is illegal! max = %d\r\n", (int)p_parm->ft_shf, FOR_USER_98520_NUE_KER_FT_SHIFT_MAX);
		return E_PAR;
	}
	if (p_parm->gv > FOR_USER_98520_NUE_KER_GV_MAX) {
		nvt_dbg(WRN, "Gamma value %d is illegal! max = %d\r\n", (int)p_parm->gv, FOR_USER_98520_NUE_KER_GV_MAX);
		return E_PAR;
	}
	if (p_parm->gv_shf > FOR_USER_98520_NUE_KER_gv_shf_MAX) {
		nvt_dbg(WRN, "Gamma shift %d is illegal! max = %d\r\n", (int)p_parm->gv_shf, FOR_USER_98520_NUE_KER_gv_shf_MAX);
		return E_PAR;
	}
	if (p_parm->degree < FOR_USER_98520_NUE_KER_DEG_MIN) {
		nvt_dbg(WRN, "Degree %d is illegal! min = %d\r\n", (int)p_parm->degree, FOR_USER_98520_NUE_KER_DEG_MIN);
		return E_PAR;
	} else if (p_parm->degree > FOR_USER_98520_NUE_KER_DEG_MAX) {
		nvt_dbg(WRN, "Degree %d is illegal! max = %d\r\n", (int)p_parm->degree, FOR_USER_98520_NUE_KER_DEG_MAX);
		return E_PAR;
	}
	if (p_parm->coef < FOR_USER_98520_NUE_KER_COEF_MIN) {
		nvt_dbg(WRN, "Coefficient %d is illegal! min = %d\r\n", (int)p_parm->coef, FOR_USER_98520_NUE_KER_COEF_MIN);
		return E_PAR;
	} else if (p_parm->degree > FOR_USER_98520_NUE_KER_COEF_MAX) {
		nvt_dbg(WRN, "Coefficient %d is illegal! max = %d\r\n", (int)p_parm->coef, FOR_USER_98520_NUE_KER_COEF_MAX);
		return E_PAR;
	}
	if (p_parm->rho < FOR_USER_98520_NUE_SVMRST_RHO_MIN) {
		nvt_dbg(WRN, "Rho %d is illegal! min = %d\r\n", (int)p_parm->rho, FOR_USER_98520_NUE_SVMRST_RHO_MIN);
		return E_PAR;
	} else if (p_parm->rho > FOR_USER_98520_NUE_SVMRST_RHO_MAX) {
		nvt_dbg(WRN, "Rho %d is illegal! max = %d\r\n", (int)p_parm->rho, FOR_USER_98520_NUE_SVMRST_RHO_MAX);
		return E_PAR;
	}
	if (p_parm->alpha_shf > FOR_USER_98520_NUE_SVMRST_ALPHA_SHIFT_MAX) {
		nvt_dbg(WRN, "Alpha shift %d is illegal! max = %d!\r\n", (int)p_parm->alpha_shf, FOR_USER_98520_NUE_SVMRST_ALPHA_SHIFT_MAX);
		return E_PAR;
	}
	if ((p_parm->rho_fmt < FOR_USER_98520_NUE_SVMRST_RHO_FMT_MIN)) {
		nvt_dbg(WRN, "Rho format %d is illegal! min = %d\r\n", (int)p_parm->rho_fmt, FOR_USER_98520_NUE_SVMRST_RHO_FMT_MIN);
		return E_PAR;
	} else if ((p_parm->rho_fmt > FOR_USER_98520_NUE_SVMRST_RHO_FMT_MAX)) {
		nvt_dbg(WRN, "Rho format %d is illegal! max = %d\r\n", (int)p_parm->rho_fmt, FOR_USER_98520_NUE_SVMRST_RHO_FMT_MAX);
		return E_PAR;
	}

	reg1.reg = NUE_GETDATA(ofs1, base_addr);
	reg2.reg = NUE_GETDATA(ofs2, base_addr);
	reg3.reg = NUE_GETDATA(ofs3, base_addr);

	reg1.bit.KER1_GV        = p_parm->gv;
	reg1.bit.KER1_GV_SHIFT  = p_parm->gv_shf;
	reg1.bit.KER2_DEGREE    = p_parm->degree;

	reg2.bit.KER1_COEF      = p_parm->coef;
	reg2.bit.KER1_FT_SHIFT  = p_parm->ft_shf;

	reg3.bit.RHO_REG        = p_parm->rho;
	reg3.bit.RHO_FMT        = p_parm->rho_fmt - FOR_USER_98520_NUE_SVMRST_RHO_FMT_MIN;
	reg3.bit.ALPHA_SHIFT    = p_parm->alpha_shf;

	NUE_SETDATA(ofs1, reg1.reg, base_addr);
	NUE_SETDATA(ofs2, reg2.reg, base_addr);
	NUE_SETDATA(ofs3, reg3.reg, base_addr);

	return E_OK;
}

/**
    NUE Get Parameters

    Get current svm kernel parameters.

    @return NUE_SVMKERL_PARM svm kernel parameters.
*/
NUE_SVMKERL_PARM nue_get_parm(VOID)
{
	NUE_SVMKERL_PARM parm;
	T_KERNEL_REGISTER0 reg1;
	T_KERNEL_REGISTER1 reg2;
	T_RESULT_REGISTER0 reg3;
	UINT32 base_addr = 0;
	UINT32 ofs1 = KERNEL_REGISTER0_OFS;
	UINT32 ofs2 = KERNEL_REGISTER1_OFS;
	UINT32 ofs3 = RESULT_REGISTER0_OFS;

	base_addr = NUE_IOADDR_REG_BASE;

	reg1.reg = NUE_GETDATA(ofs1, base_addr);
	reg2.reg = NUE_GETDATA(ofs2, base_addr);
	reg3.reg = NUE_GETDATA(ofs3, base_addr);

	parm.ft_shf         = reg2.bit.KER1_FT_SHIFT;
	parm.gv             = reg1.bit.KER1_GV;
	parm.gv_shf         = reg1.bit.KER1_GV_SHIFT;
	parm.coef           = nue_tran_bitval(reg2.bit.KER1_COEF, FOR_USER_98520_NUE_KER_COEF_BIT);
	parm.degree         = reg1.bit.KER2_DEGREE;
	parm.rho            = nue_tran_bitval(reg3.bit.RHO_REG, FOR_USER_98520_NUE_KER_RHO_BIT);
	parm.rho_fmt        = reg3.bit.RHO_FMT;
	parm.alpha_shf      = reg3.bit.ALPHA_SHIFT;
	return parm;
}

/**
    NUE set FCD Enable

    set NUE FCD function enable.

    @param[in] p_parm nue fcd parameters.

    @return ER Error nue fcd parameters are out of range.
*/
static ER nue_set_fcd_en(NUE_FCD_PARM *p_parm)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;

	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	base_addr = NUE_IOADDR_REG_BASE;

	if ((p_parm->fcd_quanti_en == FALSE) && (p_parm->fcd_quanti_kmean_en == TRUE)) {
		nvt_dbg(ERR, "When QUANTIZATION_EN disable, QUANTIZATION_KMEANS_EN not allow to be enable !\r\n");
		return E_CTX;
	}
	if ((p_parm->fcd_vlc_en == FALSE) && (p_parm->fcd_quanti_kmean_en == TRUE)) {
		nvt_dbg(ERR, "When VLCMAN_EN disable, QUANTIZATION_KMEANS_EN not allow to be enable !\r\n");
		return E_CTX;
	}

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.FCD_VLC_EN = p_parm->fcd_vlc_en;
	reg1.bit.FCD_QUANTIZATION_EN = p_parm->fcd_quanti_en;
	reg1.bit.FCD_SPARSE_EN = p_parm->fcd_sparse_en;
	reg1.bit.FCD_QUANTIZATION_KMEANS = p_parm->fcd_quanti_kmean_en;

	NUE_SETDATA(ofs, reg1.reg, base_addr);

	return E_OK;
}

/**
    NUE Get FCD Enable

    get NUE FCD function enable.

    @param[out] p_parm nue fcd parameters.

    @return ER Error nue fcd parameters are out of range.
*/
static ER nue_get_fcd_en(NUE_FCD_PARM *p_parm)
{
	T_NUE_MODE_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_MODE_REGISTER1_OFS;

	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	p_parm->fcd_vlc_en          = reg1.bit.FCD_VLC_EN;
	p_parm->fcd_quanti_en       = reg1.bit.FCD_QUANTIZATION_EN;
	p_parm->fcd_sparse_en       = reg1.bit.FCD_SPARSE_EN;
	p_parm->fcd_quanti_kmean_en = reg1.bit.FCD_QUANTIZATION_KMEANS;
	return E_OK;
}

/**
    NUE Set FCD Encode Bit Length

    set NUE FCD encode bit length.

    @param[in] p_parm nue fcd parameters.

    @return ER Error nue fcd parameters are out of range.
*/
static ER nue_set_fcd_encbitlength(NUE_FCD_PARM *p_parm)
{
	T_COMPRESSION_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = COMPRESSION_REGISTER0_OFS;

	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	base_addr = NUE_IOADDR_REG_BASE;
	if (p_parm->fcd_enc_bit_length > FOR_USER_98520_NUE_FCD_ENC_BIT_LENGTH_MAX) {
		nvt_dbg(ERR, "fcd encode bit length is out of range : %d !\r\n", (int)p_parm->fcd_enc_bit_length);
		return E_CTX;
	}

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.FCD_ENC_BIT_LENGTH = p_parm->fcd_enc_bit_length;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	return E_OK;
}

/**
    NUE Get FCD Encode Bit Length

    get NUE FCD encode bit length.

    @param[out] p_parm nue fcd parameters.

    @return ER Error nue fcd parameters are out of range.
*/
static ER nue_get_fcd_encbitlength(NUE_FCD_PARM *p_parm)
{
	T_COMPRESSION_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = COMPRESSION_REGISTER0_OFS;

	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	p_parm->fcd_enc_bit_length = reg1.bit.FCD_ENC_BIT_LENGTH;
	return E_OK;
}

/**
    NUE Set FCD Canonical VLC Table

    NUE set FCD canonical VLC table (code).

    @param[in] p_parm nue fcd parameters.

    @return ER Error nue fcd parameters are out of range.
*/
static ER nue_set_fcd_vlc_table_code(NUE_FCD_PARM *p_parm)
{
	UINT32 i = 0;
	T_COMPRESSION_REGISTER1 reg[NUE_FCD_VLC_TBL_SIZE];
	UINT32 base_addr = 0;
	UINT32 ofs[NUE_FCD_VLC_TBL_SIZE];
	UINT32 ofs_gap = COMPRESSION_REGISTER2_OFS - COMPRESSION_REGISTER1_OFS;

	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = COMPRESSION_REGISTER1_OFS;
	for (i = 1; i < NUE_FCD_VLC_TBL_SIZE; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}
	base_addr = NUE_IOADDR_REG_BASE;

	for (i = 0; i < NUE_FCD_VLC_TBL_SIZE; i++) {
		if (p_parm->fcd_vlc_code[i] > FOR_USER_98520_NUE_FCD_VLC_CODE_MAX) {
			nvt_dbg(WRN, "fcd_vlc_code[%d] %d must be < %d\r\n", (int)i, (int)p_parm->fcd_vlc_code[i], FOR_USER_98520_NUE_FCD_VLC_CODE_MAX);
			return E_PAR;
		}
		if (p_parm->fcd_vlc_valid[i] > FOR_USER_98520_NUE_FCD_VLC_VALID_MAX) {
			nvt_dbg(WRN, "fcd_vlc_valid[%d] %d must be < %d\r\n", (int)i, (int)p_parm->fcd_vlc_valid[i], FOR_USER_98520_NUE_FCD_VLC_VALID_MAX);
			return E_PAR;
		}
	}

	for (i = 0; i < NUE_FCD_VLC_TBL_SIZE; i++) {		
		reg[i].reg = NUE_GETDATA(ofs[i], base_addr);
		reg[i].bit.FCD_VLC_CODE0  = p_parm->fcd_vlc_code[i];
		reg[i].bit.FCD_VLC_VALID0 = p_parm->fcd_vlc_valid[i];
		NUE_SETDATA(ofs[i], reg[i].reg, base_addr);
	}

	return E_OK;
}


/**
    NUE Get FCD Canonical VLC Table

    NUE get FCD canonical VLC table (code).

    @param[out] p_parm nue fcd parameters.

    @return ER Error nue fcd parameters are out of range.
*/
static ER nue_get_fcd_vlc_table_code(NUE_FCD_PARM *p_parm)
{
	UINT32 i = 0;
	T_COMPRESSION_REGISTER1 reg[NUE_FCD_VLC_TBL_SIZE];
	UINT32 base_addr = 0;
	UINT32 ofs[NUE_FCD_VLC_TBL_SIZE];
	UINT32 ofs_gap = COMPRESSION_REGISTER2_OFS - COMPRESSION_REGISTER1_OFS;

	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = COMPRESSION_REGISTER1_OFS;
	for (i = 1; i < NUE_FCD_VLC_TBL_SIZE; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}
	base_addr = NUE_IOADDR_REG_BASE;
	for (i = 0; i < NUE_FCD_VLC_TBL_SIZE; i++) {
		reg[i].reg = NUE_GETDATA(ofs[i], base_addr);
		p_parm->fcd_vlc_code[i]     = reg[i].bit.FCD_VLC_CODE0;
		p_parm->fcd_vlc_valid[i]    = reg[i].bit.FCD_VLC_VALID0;
	}
	return E_OK;
}

/**
    NUE Set FCD Canonical VLC Table

    NUE set FCD canonical VLC table (offset).

    @param[in] p_parm nue fcd parameters.

    @return ER Error nue fcd parameters are out of range.
*/
static ER nue_set_fcd_vlc_table_ofs(NUE_FCD_PARM *p_parm)
{
	UINT32 i = 0;
	T_COMPRESSION_REGISTER24 reg[NUE_FCD_VLC_REG_CNT];
	UINT32 base_addr = 0;
	UINT32 ofs_gap = COMPRESSION_REGISTER25_OFS - COMPRESSION_REGISTER24_OFS;
	UINT32 ofs[NUE_FCD_VLC_REG_CNT];

	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = COMPRESSION_REGISTER24_OFS;
	for (i = 1; i < NUE_FCD_VLC_REG_CNT; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}
	base_addr = NUE_IOADDR_REG_BASE;

	for (i = 0; i < NUE_FCD_VLC_TBL_SIZE; i++) {
		if (p_parm->fcd_vlc_ofs[i] > FOR_USER_98520_NUE_FCD_VLC_OFFSET_MAX) {
			nvt_dbg(WRN, "fcd_vlc_ofs[%d] %d must be < %d\r\n", (int)i, (int)p_parm->fcd_vlc_ofs[i], FOR_USER_98520_NUE_FCD_VLC_OFFSET_MAX);
			return E_PAR;
		}
	}


	for (i = 0; i < NUE_FCD_VLC_REG_CNT; i++) {
		reg[i].reg = NUE_GETDATA(ofs[i], base_addr);
		reg[i].bit.FCD_VLC_OFFSET0 = p_parm->fcd_vlc_ofs[2 * i];
		if (i < ((NUE_FCD_VLC_TBL_SIZE - 1) >> 1)) {
			reg[i].bit.FCD_VLC_OFFSET1 = p_parm->fcd_vlc_ofs[2 * i + 1];
		}
		NUE_SETDATA(ofs[i], reg[i].reg, base_addr);
	}

	return E_OK;
}

/**
    NUE Get FCD Canonical VLC Table

    NUE get FCD canonical VLC table (offset).

    @param[out] p_parm nue fcd parameters.

    @return ER Error nue fcd parameters are out of range.
*/
static ER nue_get_fcd_vlc_table_ofs(NUE_FCD_PARM *p_parm)
{
	UINT32 i = 0;
	T_COMPRESSION_REGISTER24 reg[NUE_FCD_VLC_REG_CNT];
	UINT32 base_addr = 0;
	UINT32 ofs[NUE_FCD_VLC_REG_CNT];
	UINT32 ofs_gap = COMPRESSION_REGISTER25_OFS - COMPRESSION_REGISTER24_OFS;

	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = COMPRESSION_REGISTER24_OFS;
	for (i = 1; i < NUE_FCD_VLC_REG_CNT; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}
	base_addr = NUE_IOADDR_REG_BASE;

	for (i = 0; i < NUE_FCD_VLC_REG_CNT; i++) {
		reg[i].reg = NUE_GETDATA(ofs[i], base_addr);
		p_parm->fcd_vlc_ofs[2 * i]         = reg[i].bit.FCD_VLC_OFFSET0;
		if (i < ((NUE_FCD_VLC_TBL_SIZE - 1) >> 1)) {
			p_parm->fcd_vlc_ofs[2 * i + 1]   = reg[i].bit.FCD_VLC_OFFSET1;
		}
	}

	return E_OK;
}

/**
    NUE Set FCD Parameters

    nue set fcd parameters.

    @param[in] p_parm nue fcd parameters.

    @return ER Error nue fcd parameters are out of range.
*/
ER nue_set_fcd_parm(NUE_FCD_PARM *p_parm)
{
	ER er_code = E_OK;
	if (p_parm == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	er_code = nue_set_fcd_en(p_parm);
	if (er_code != E_OK) {
		return er_code;
	}
	er_code = nue_set_fcd_encbitlength(p_parm);
	if (er_code != E_OK) {
		return er_code;
	}
	er_code = nue_set_fcd_vlc_table_code(p_parm);
	if (er_code != E_OK) {
		return er_code;
	}
	er_code = nue_set_fcd_vlc_table_ofs(p_parm);
	if (er_code != E_OK) {
		return er_code;
	}
	return er_code;
}

/**
    NUE Get FCD Parameters

    nue get fcd parameters.

    @return p_parm nue fcd parameters.
*/
NUE_FCD_PARM nue_get_fcd_parm(VOID)
{
	NUE_FCD_PARM parm;
	ER er_code = E_OK;
	er_code = nue_get_fcd_en(&parm);
	if (er_code != E_OK) {
		nvt_dbg(WRN, "nue_get_fcd_en fail...\r\n");
	}
	er_code = nue_get_fcd_encbitlength(&parm);
	if (er_code != E_OK) {
		nvt_dbg(WRN, "nue_get_fcd_encbitlength fail...\r\n");
	}
	er_code = nue_get_fcd_vlc_table_code(&parm);
	if (er_code != E_OK) {
		nvt_dbg(WRN, "nue_get_fcd_vlc_table_code fail...\r\n");
	}
	er_code = nue_get_fcd_vlc_table_ofs(&parm);
	if (er_code != E_OK) {
		nvt_dbg(WRN, "nue_get_fcd_vlc_table_ofs fail...\r\n");
	}
	return parm;
}


/**
    NUE Get Result

    Get current ouput results in registers.

    @param[out] p_rsts register results.

    @return ER Error results are null.
*/
ER nue_get_rsts(NUE_SVMRSTS *p_rsts)
{
	UINT32 i = 0;
	T_SVM_RESULT_REGISTER0 reg[NUE_RESULTS_REG_CNT];
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs[NUE_RESULTS_REG_CNT];
	UINT32 ofs_gap = SVM_RESULT_REGISTER1_OFS - SVM_RESULT_REGISTER0_OFS;

	if (p_rsts == NULL) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}

	ofs[0] = SVM_RESULT_REGISTER0_OFS;
	for (i = 1; i < NUE_RESULTS_REG_CNT; i++) {
		ofs[i] = ofs[i - 1] + ofs_gap;
	}

	for (i = 0; i < NUE_RESULTS_REG_CNT; i++) {
		reg[i].reg = NUE_GETDATA(ofs[i], base_addr);
		p_rsts->rslts[2 * i]      = reg[i].bit.Result0;
		p_rsts->rslts[2 * i + 1]    = reg[i].bit.Result1;
	}
	return E_OK;
}

/**
    NUE Set ReLU Leaky

    NUE set Leaky ReLU parameters.

    @param[in] value input leaky value.
    @param[in] shf input leaky shift.

    @return ER Error relu leaky parameters are out of range.
*/
ER nue_set_relu_leaky(INT32 value, UINT32 shf)
{
	T_RELU_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = RELU_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	if (value > FOR_USER_98520_NUE_RELU_LEAKY_VAL_MAX) {
		nvt_dbg(WRN, "In ReLU, leaky value %d > %d!\r\n", (int)value, FOR_USER_98520_NUE_RELU_LEAKY_VAL_MAX);
		return E_PAR;
	} else if (value < FOR_USER_98520_NUE_RELU_LEAKY_VAL_MIN) {
		nvt_dbg(WRN, "In ReLU, leaky value %d < %d!\r\n", (int)value, FOR_USER_98520_NUE_RELU_LEAKY_VAL_MIN);
		return E_PAR;
	}
	if (shf > FOR_USER_98520_NUE_RELU_LEAKY_SHF_MAX) {
		nvt_dbg(WRN, "In ReLU, leaky shift %d > %d!\r\n", (int)shf, FOR_USER_98520_NUE_RELU_LEAKY_SHF_MAX);
		return E_PAR;
	} else if (shf < FOR_USER_98520_NUE_RELU_LEAKY_SHF_MIN) {
		nvt_dbg(WRN, "In ReLU, leaky shift %d < %d!\r\n", (int)shf, FOR_USER_98520_NUE_RELU_LEAKY_SHF_MIN);
		return E_PAR;
	}
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.RELU_LEAKY_VAL     = value;
	reg1.bit.RELU_LEAKY_SHIFT   = shf - FOR_USER_98520_NUE_RELU_LEAKY_SHF_MIN;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
    NUE Get ReLU Leaky

    NUE get Leaky ReLU parameters.

    @param[out] p_value output leaky value.
    @param[out] p_shf output leaky shift.

    @return ER Error relu leaky parameters are out of range.
*/
ER nue_get_relu_leaky(INT32 *p_value, UINT32 *p_shf)
{
	T_RELU_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = RELU_REGISTER0_OFS;
	if ((p_value == NULL) || (p_shf == NULL)) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	*p_value = nue_tran_bitval(reg1.bit.RELU_LEAKY_VAL, FOR_USER_98520_NUE_RELU_LEAKY_BIT);
	*p_shf = reg1.bit.RELU_LEAKY_SHIFT + FOR_USER_98520_NUE_RELU_LEAKY_SHF_MIN;
	return E_OK;
}

/**
    NUE Set ReLU Shift

	NUE set relu shift.

	@param[in] shift relu shift.

    @return None.
*/
ER nue_set_relu_shf(INT32 shift)
{
	T_RELU_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = RELU_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	if (shift > FOR_USER_98520_NUE_RELU_SHIFT_MAX) {
		nvt_dbg(WRN, "In ReLU, output shift %d > %d!\r\n", (int)shift, FOR_USER_98520_NUE_RELU_SHIFT_MAX);
		return E_PAR;
	} else if (shift < FOR_USER_98520_NUE_RELU_SHIFT_MIN) {
		nvt_dbg(WRN, "In ReLU, output shift %d < %d!\r\n", (int)shift, FOR_USER_98520_NUE_RELU_SHIFT_MIN);
		return E_PAR;
	}

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if (shift < 0) {
		reg1.bit.RELU_SHIFT_SIGNEDNESS = 1;
		reg1.bit.RELU_SHIFT = -shift;
	} else {
		reg1.bit.RELU_SHIFT_SIGNEDNESS = 0;
		reg1.bit.RELU_SHIFT = shift;
	}
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
    NUE Get ReLU Shift.

	NUE get relu shift.

    @return INT32 relu shift.
*/
INT32 nue_get_relu_shf(VOID)
{
	T_RELU_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = RELU_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if (reg1.bit.RELU_SHIFT_SIGNEDNESS) {
		return -(reg1.bit.RELU_SHIFT);
	} else {
		return (reg1.bit.RELU_SHIFT);
	}
}

/**
    NUE Set ROI-pooling Kernel Size

    NUE set roi-pooling kernel size.

	@param[in] size roi-pooling kernel size.

    @return None.
*/
VOID nue_set_roipool_kersize(NUE_ROI_POOL_SIZE size)
{
	T_ROIPOOLING_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ROIPOOLING_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if ((UINT32)size > 2)
		reg1.bit.ROIPOOL_PSROI_WH = (UINT32)size - 3;
	else
		reg1.bit.ROIPOOL_STANDARD_WH = (UINT32)size;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
    NUE Get standard ROI-pooling Kernel Size

    NUE get standard roi-pooling kernel size.

    @return NUE_ROI_POOL_SIZE roi-pooling kernel size.
*/
NUE_ROI_POOL_SIZE nue_get_roipool_standard_kersize(VOID)
{
	T_ROIPOOLING_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ROIPOOLING_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	return (NUE_ROI_POOL_SIZE)reg1.bit.ROIPOOL_STANDARD_WH;
}

/**
    NUE Get PS ROI-pooling Kernel Size

    NUE get ps roi-pooling kernel size.

    @return NUE_ROI_POOL_SIZE roi-pooling kernel size.
*/
NUE_ROI_POOL_SIZE nue_get_roipool_psroi_kersize(VOID)
{
	T_ROIPOOLING_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ROIPOOLING_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	return (NUE_ROI_POOL_SIZE)(reg1.bit.ROIPOOL_PSROI_WH+3);
}

/**
	NUE Set ROI-pooling Ratio

	nue set roi-pooling ratio.

	@param[in] mul multiple value of ratio.
	@param[in] shf shift value of ratio.

	@return None.
*/
ER nue_set_roipool_ratio(UINT32 mul, UINT32 shf)
{
	T_ROIPOOLING_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ROIPOOLING_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	if (mul > FOR_USER_98520_NUE_ROI_RATIO_MUL_MAX) {
		nvt_dbg(WRN, "In ROI pooling, multiple of scale ratio %d > %d!\r\n", (int)mul, FOR_USER_98520_NUE_ROI_RATIO_MUL_MAX);
		return E_PAR;
	}
	if (shf > FOR_USER_98520_NUE_ROI_RATIO_SHF_MAX) {
		nvt_dbg(WRN, "In ROI pooling, shift of scale ratio %d > %d!\r\n", (int)shf, FOR_USER_98520_NUE_ROI_RATIO_SHF_MAX);
		return E_PAR;
	}

	if (mul > (UINT32)(1 << shf)) {
		nvt_dbg(WRN, "ratio should be <= 1; mul=%d, shf=%d\r\n", (int)mul, (int)shf);
		return E_PAR;
	}

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.ROIPOOL_RATIO_MUL = mul;
	reg1.bit.ROIPOOL_RATIO_SHIFT = shf;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
	NUE Get ROI-pooling Ratio

	nue get roi-pooling ratio.

	@param[out] p_mul multiple value of ratio.
	@param[out] p_shf shift value of ratio.

	@return ER Error eltwise parameters are null.
*/
ER nue_get_roipool_ratio(UINT32 *p_mul, UINT32 *p_shf)
{
	T_ROIPOOLING_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ROIPOOLING_REGISTER0_OFS;
	if ((p_mul == NULL) || (p_shf == NULL)) {
		nvt_dbg(ERR, "null input...\r\n");
		return E_NOEXS;
	}
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	*p_mul = reg1.bit.ROIPOOL_RATIO_MUL;
	*p_shf = reg1.bit.ROIPOOL_RATIO_SHIFT;
	
	return E_OK;
}

/**
	NUE Set ROI-pooling Shift

	NUE set roi-pooling shift.

	@param[in] shift roi-pooling shift.

	@return None.
*/
ER nue_set_roipool_shf(INT32 shift)
{
	T_ROIPOOLING_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ROIPOOLING_REGISTER0_OFS;
	INT32 upper_bound = 0, lower_bound = 0;
	base_addr = NUE_IOADDR_REG_BASE;

	if ((nue_get_outtype() == NUE_INT16) || (nue_get_outtype() == NUE_UINT16)) {
		upper_bound = FOR_USER_98520_NUE_ROI_SHIFT_MAX1;
		lower_bound = FOR_USER_98520_NUE_ROI_SHIFT_MIN1;
	} else {
		upper_bound = FOR_USER_98520_NUE_ROI_SHIFT_MAX2;
		lower_bound = FOR_USER_98520_NUE_ROI_SHIFT_MIN2;
	}
	if (shift > upper_bound) {
		nvt_dbg(WRN, "In ROI pooling, output shift %d > %d!\r\n", (int)shift, (int)upper_bound);
		return E_PAR;
	} else if (shift < lower_bound) {
		nvt_dbg(WRN, "In ROI pooling, output shift %d < %d!\r\n", (int)shift, (int)lower_bound);
		return E_PAR;
	}

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if (shift < 0) {
		reg1.bit.ROIPOOL_SHIFT_SIGNEDNESS = 1;
		reg1.bit.ROIPOOL_SHIFT = -shift;
	} else {
		reg1.bit.ROIPOOL_SHIFT_SIGNEDNESS = 0;
		reg1.bit.ROIPOOL_SHIFT = shift;
	}
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
	NUE Get ROI-pooling Shift

	NUE get roi-pooling shift.

	@return INT32 roi-pooling shift.
*/
INT32 nue_get_roipool_shf(VOID)
{
	T_ROIPOOLING_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ROIPOOLING_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if (reg1.bit.ROIPOOL_SHIFT_SIGNEDNESS) {
		return -(reg1.bit.ROIPOOL_SHIFT);
	} else {
		return (reg1.bit.ROIPOOL_SHIFT);
	}
}

/**
	NUE Set Reorganize Shift

	NUE set reorganize shift.

	@param[in] shift reorganize shift.

	@return None.
*/
ER nue_set_reorgshf(INT32 shift)
{
	T_REORGANIZE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = REORGANIZE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	if (shift > FOR_USER_98520_NUE_REORG_SHF_MAX) {
		nvt_dbg(WRN, "In Reorganize, output shift %d > %d!\r\n", (int)shift, FOR_USER_98520_NUE_REORG_SHF_MAX);
		return E_PAR;
	} else if (shift < FOR_USER_98520_NUE_REORG_SHF_MIN) {
		nvt_dbg(WRN, "In Reorganize, output shift %d < %d!\r\n", (int)shift, FOR_USER_98520_NUE_REORG_SHF_MIN);
		return E_PAR;
	}

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if (shift < 0) {
		reg1.bit.REORGANIZE_SHIFT_SIGNEDNESS = 1;
		reg1.bit.REORGANIZE_SHIFT = -shift;
	} else {
		reg1.bit.REORGANIZE_SHIFT_SIGNEDNESS = 0;
		reg1.bit.REORGANIZE_SHIFT = shift;
	}
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
	NUE Get Reorganize Shift

	NUE get reorganize shift.

	@return INT32 reorganize shift.
*/
INT32 nue_get_reorgshf(VOID)
{
	T_REORGANIZE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = REORGANIZE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if (reg1.bit.REORGANIZE_SHIFT_SIGNEDNESS) {
		return -(reg1.bit.REORGANIZE_SHIFT);
	} else {
		return (reg1.bit.REORGANIZE_SHIFT);
	}
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
	NUE Set Anchor Shift

	NUE set anchor shift.

	@param[in] shift anchor shift.

	@return None.
*/
ER nue_set_anchor_shf(UINT32 shift)
{
	T_ANCHOR_TRANSFORM_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ANCHOR_TRANSFORM_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	if (shift > FOR_USER_98520_NUE_ANCHOR_SHF_MAX) {
		nvt_dbg(WRN, "In Anchor, shift %d > %d!\r\n", (int)shift, FOR_USER_98520_NUE_ANCHOR_SHF_MAX);
		return E_PAR;
	}

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.AT_W_SHIFT = shift;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
	NUE Get Anchor Shift

	NUE get anchor shift.

	@return INT32 anchor shift.
*/
UINT32 nue_get_anchor_shf(VOID)
{
	T_ANCHOR_TRANSFORM_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ANCHOR_TRANSFORM_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return reg1.bit.AT_W_SHIFT;
}

/**
	NUE Set Anchor Table Update

	NUE set anchor table update.

	@param[in] update anchor table update.

	@return None.
*/
VOID nue_set_anchor_table_update(BOOL update)
{
	T_ANCHOR_TRANSFORM_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ANCHOR_TRANSFORM_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.AT_TABLE_UPDATE = update;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
	NUE Get Anchor Table Update

	NUE get anchor table update.

	@return BOOL anchor table update.
*/
BOOL nue_get_anchor_table_update(VOID)
{
	T_ANCHOR_TRANSFORM_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = ANCHOR_TRANSFORM_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return reg1.bit.AT_TABLE_UPDATE;
}

/**
	NUE Set Softmax Input Shift

	NUE set softmax input shift.

	@param[in] shift softmax input shift.

	@return None.
*/
ER nue_set_softmax_in_shf(INT32 shift)
{
	T_SOFTMAX_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SOFTMAX_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	if (shift > FOR_USER_98520_NUE_SOFTMAX_SHF_MAX) {
		nvt_dbg(WRN, "In Softmax, shift %d > %d!\r\n", (int)shift, FOR_USER_98520_NUE_SOFTMAX_SHF_MAX);
		return E_PAR;
	} else if (shift < FOR_USER_98520_NUE_SOFTMAX_SHF_MIN) {
		nvt_dbg(WRN, "In Softmax, shift %d < %d!\r\n", (int)shift, FOR_USER_98520_NUE_SOFTMAX_SHF_MIN);
		return E_PAR;
	}

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if (shift < 0) {
		reg1.bit.SF_IN_SHIFT_SIGNEDNESS = 1;
		reg1.bit.SF_IN_SHIFT = -shift;
	} else {
		reg1.bit.SF_IN_SHIFT_SIGNEDNESS = 0;
		reg1.bit.SF_IN_SHIFT = shift;
	}
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
	NUE Get Softmax Input Shift

	NUE get softmax input shift.

	@return INT32 input shift.
*/
INT32 nue_get_softmax_in_shf(VOID)
{
	T_SOFTMAX_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SOFTMAX_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	if (reg1.bit.SF_IN_SHIFT_SIGNEDNESS) {
		return -(reg1.bit.SF_IN_SHIFT);
	} else {
		return (reg1.bit.SF_IN_SHIFT);
	}
}

/**
	NUE Set Softmax Output Shift

	NUE set softmax output shift.

	@param[in] shift softmax output shift.

	@return None.
*/
ER nue_set_softmax_out_shf(INT32 shift)
{
	T_SOFTMAX_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SOFTMAX_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	if (shift > FOR_USER_98520_NUE_SOFTMAX_SHF_MAX) {
		nvt_dbg(WRN, "In Softmax, shift %d > %d!\r\n", (int)shift, FOR_USER_98520_NUE_SOFTMAX_SHF_MAX);
		return E_PAR;
	} else if (shift < FOR_USER_98520_NUE_SOFTMAX_SHF_MIN) {
		nvt_dbg(WRN, "In Softmax, shift %d < %d!\r\n", (int)shift, FOR_USER_98520_NUE_SOFTMAX_SHF_MIN);
		return E_PAR;
	}

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if (shift < 0) {
		reg1.bit.SF_OUT_SHIFT_SIGNEDNESS = 1;
		reg1.bit.SF_OUT_SHIFT = -shift;
	} else {
		reg1.bit.SF_OUT_SHIFT_SIGNEDNESS = 0;
		reg1.bit.SF_OUT_SHIFT = shift;
	}
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
	NUE Get Softmax Output Shift

	NUE get softmax output shift.

	@return INT32 softmax output shift.
*/
INT32 nue_get_softmax_out_shf(VOID)
{
	T_SOFTMAX_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SOFTMAX_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	if (reg1.bit.SF_OUT_SHIFT_SIGNEDNESS) {
		return -(reg1.bit.SF_OUT_SHIFT);
	} else {
		return (reg1.bit.SF_OUT_SHIFT);
	}
}

/**
	NUE Set Softmax Group Number

	NUE set softmax group number.

	@param[in] group_num softmax group number.

	@return None.
*/
ER nue_set_softmax_group_num(UINT32 group_num)
{
	T_SOFTMAX_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SOFTMAX_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	if (group_num > FOR_USER_98520_NUE_SOFTMAX_GROUP_MAX) {
		nvt_dbg(WRN, "In Softmax, group_num %d > %d!\r\n", (int)group_num, FOR_USER_98520_NUE_SOFTMAX_GROUP_MAX);
		return E_PAR;
	}

	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.SF_GROUP_NUM = group_num;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
	
	return E_OK;
}

/**
	NUE Get Softmax Group Number

	NUE get softmax group number.

	@return UINT32 softmax group number.
*/
UINT32 nue_get_softmax_group_num(VOID)
{
	T_SOFTMAX_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SOFTMAX_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	return reg1.bit.SF_GROUP_NUM;
}

/**
	NUE Set Softmax Set Number

	NUE set softmax set number.

	@param[in] num softmax set number.

	@return None.
*/
VOID nue_set_softmax_setnum(UINT32 num)
{
	T_SOFTMAX_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SOFTMAX_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.SF_SET_NUM = num;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
	NUE Get Softmax Set Number

	NUE get softmax set number.

	@return UINT32 softmax set number.
*/
UINT32 nue_get_softmax_setnum(VOID)
{
	T_SOFTMAX_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SOFTMAX_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	return reg1.bit.SF_SET_NUM;
}


/**
	NUE Set permute stripepermute stripe

	NUE set permute stripe en.

	@param[in] permute stripe en.

	@return None.
*/

VOID nue_set_permute_stripe(BOOL stripe_en)
{
	T_DEBUG_DESIGN_REGISTER reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = DEBUG_DESIGN_REGISTER_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.PERMUTE_STRIPE_EN = stripe_en;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
	NUE Get permute stripe

	NUE get permute stripe en.

	@return BOOL permute stripe en.
*/
BOOL nue_get_permute_stripe(VOID)
{
	T_DEBUG_DESIGN_REGISTER reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = DEBUG_DESIGN_REGISTER_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	return reg1.bit.PERMUTE_STRIPE_EN;
}

/**
	NUE Set Input Shift

	NUE set input shift.

	@param[in] shift shift value.

	@return None.
*/
VOID nue_set_in_shf(INT32 shift)
{
	T_SCALE_SHIFT_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SCALE_SHIFT_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);

	if (shift < 0) {
		reg1.bit.IN_SHIFT_DIR = 1;
		reg1.bit.IN_SHIFT = -shift;
	} else {
		reg1.bit.IN_SHIFT_DIR = 0;
		reg1.bit.IN_SHIFT = shift;
	}
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
	NUE Get Input Shift

	NUE get input shift.

	@return INT32 shift value.
*/
INT32 nue_get_in_shf(VOID)
{
	T_SCALE_SHIFT_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SCALE_SHIFT_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	if (reg1.bit.IN_SHIFT_DIR) {
		return -(reg1.bit.IN_SHIFT);
	} else {
		return (reg1.bit.IN_SHIFT);
	}
}

/**
	NUE Set Input Scale

	NUE set input scale.

	@param[in] scale value.

	@return None.
*/
VOID nue_set_in_scale(UINT32 scale)
{
	T_SCALE_SHIFT_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SCALE_SHIFT_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.IN_SCALE = scale;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
	NUE Get Input Scale

	NUE get input scale.

	@return INT32 scale value.
*/
UINT32 nue_get_in_scale(VOID)
{
	T_SCALE_SHIFT_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = SCALE_SHIFT_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	return (UINT32)reg1.bit.IN_SCALE;
}

/**
	NUE Set Input clamp Threshold 0

	NUE set input clamp threshold 0

	@param[in] threshold

	@return None.
*/
VOID nue_set_clamp_th0(UINT32 threshold)
{
	T_CLAMP_THRESHOLD_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = CLAMP_THRESHOLD_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.CLAMP_THRES0 = threshold;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
	NUE Get Input Clamp Threshold 0

	NUE get input clamp threshold 0

	@return INT32 clamp threshold 0.
*/
UINT32 nue_get_clamp_th0(VOID)
{
	T_CLAMP_THRESHOLD_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = CLAMP_THRESHOLD_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	return (UINT32)reg1.bit.CLAMP_THRES0;
}

/**
	NUE Set Input clamp Threshold 1

	NUE set input clamp threshold 1

	@param[in] threshold

	@return None.
*/
VOID nue_set_clamp_th1(UINT32 threshold)
{
	T_CLAMP_THRESHOLD_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = CLAMP_THRESHOLD_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.CLAMP_THRES1 = threshold;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
	NUE Get Input Clamp Threshold 1

	NUE get input clamp threshold 1

	@return INT32 clamp threshold 1.
*/
UINT32 nue_get_clamp_th1(VOID)
{
	T_CLAMP_THRESHOLD_REGISTER1 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = CLAMP_THRESHOLD_REGISTER1_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	return (UINT32)reg1.bit.CLAMP_THRES1;
}

/**
	NUE Set DMA Disable

	NUE set dma disable

	@param[in] disable

	@return None.
*/
VOID nue_set_dma_disable(BOOL disable)
{
	T_NUE_DMA_DISABLE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_DMA_DISABLE_REGISTER0_OFS;
	
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.DMA_DISABLE = disable;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}

/**
	NUE Get DMA Disable

	NUE get dma disable

	@return BOOL dma_disable.
*/
BOOL nue_get_dma_disable(VOID)
{
	T_NUE_DMA_DISABLE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_DMA_DISABLE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	return (BOOL)reg1.bit.DMA_DISABLE;
}

/**
	NUE Get Engine Idle

	NUE get engine idle

	@return BOOL nue_idle.
*/
BOOL nue_get_engine_idle(VOID)
{
	T_NUE_DMA_DISABLE_REGISTER0 reg1;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE_DMA_DISABLE_REGISTER0_OFS;
	base_addr = NUE_IOADDR_REG_BASE;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	
	return (BOOL)reg1.bit.NUE_IDLE;
}

/**
    NUE Set DRAM Input and Output Addresses

    NUE set four DMA input and one output starting addresses.

    @param[in] p_addr DMA input and output buffer starting addresses.

    @return ER Error address formats are wrong or null.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER nue_set_dmaio_addr(NUE_DMAIO_ADDR dma_addr)
{
	NUE_DMAIO_ADDR *p_addr = &dma_addr;
#if (NUE_DMA_CACHE_HANDLE == 1)
	UINT32 addr0, addr1, addr2, addr3, addr4, addr5;
	UINT32 phy_addr0, phy_addr1, phy_addr2, phy_addr3, phy_addr4, phy_addr5;
	NUE_SVM_IN_SIZE in_svmsize;
	NUE_SIZE insize;
	NUE_FCD_PARM fcd_para;
	UINT32 kmeans_table_byte = 0;
	UINT32 inbyte, outbyte;
	UINT32 flushsize = 0;

	fcd_para = nue_get_fcd_parm();

	//--- Set to registers
	in_svmsize = nue_get_svmsize();
	insize = nue_get_insize();
	if ((nue_get_intype() == NUE_INT8) || (nue_get_intype() == NUE_UINT8)) {
		inbyte = 1;
	} else {
		inbyte = 2;
	}
	if ((nue_get_outtype() == NUE_INT8) || (nue_get_outtype() == NUE_UINT8)) {
		outbyte = 1;
	} else {
		outbyte = 2;
	}

	switch (nue_get_engmode()) {
	case NUE_SVM:
		// Input Dram DMA/Cache Handle
		addr0 = p_addr->addrin0;
		addr1 = p_addr->addrin_sv;
		addr2 = p_addr->addrin_alpha;
		addr3 = p_addr->addrin_rho;
		addr4 = p_addr->addrin_kq;
		addr5 = p_addr->addrin1;

		if (addr0 != 0) {
			flushsize = (insize.width * in_svmsize.objnum * inbyte);
			//dma_flushWriteCache(addr0, flushsize);
			if (p_addr->drv_dma_not_sync[0] == 0) {
				nue_pt_dma_flush_mem2dev(addr0, flushsize);
			}
		}
		if (addr1 != 0) {
			//flushsize = (insize.width*insize.height*3>>1);
			flushsize = ((fcd_para.fcd_enc_bit_length + 7) >> 3);
			//dma_flushWriteCache(addr1, flushsize);
			if (p_addr->drv_dma_not_sync[2] == 0) {
				nue_pt_dma_flush_mem2dev(addr1, flushsize);
			}
		}
		if (addr2 != 0) {
			flushsize = (insize.height * 3 >> 1);
			//dma_flushWriteCache(addr2, flushsize);
			if (p_addr->drv_dma_not_sync[3] == 0) {
				nue_pt_dma_flush_mem2dev(addr2, flushsize);
			}
		}
		if (addr3 != 0) {
			flushsize = (insize.height * 3 >> 1);
			//dma_flushWriteCache(addr3, flushsize);
			if (p_addr->drv_dma_not_sync[4] == 0) {
				nue_pt_dma_flush_mem2dev(addr3, flushsize);
			}
		}

		//Kmeans Table
		if ((fcd_para.fcd_vlc_en == TRUE) && (fcd_para.fcd_quanti_en == TRUE)) {
			// vlc symbol (16-bit packed) + kmeans (16-bit packed)
			kmeans_table_byte = (256 + 256) * 2;
		} else if ((fcd_para.fcd_vlc_en == TRUE) && (fcd_para.fcd_quanti_en == FALSE)) {
			// vlc symbol (16-bit packed)
			kmeans_table_byte = 4096 * 2;
		} else {
			// no table need
			kmeans_table_byte = 0;
		}
		//dma_flushWriteCache(addr4, (kmeans_table_byte));
		if (addr4 != 0) {
			if (p_addr->drv_dma_not_sync[6] == 0) {
				nue_pt_dma_flush_mem2dev(addr4, (kmeans_table_byte));
			}
		}
		//nvt_dbg(INFO, "Kmeans\r\n");

		//phy_addr0 = dma_getPhyAddr(addr0);
		//phy_addr1 = dma_getPhyAddr(addr1);
		//phy_addr2 = dma_getPhyAddr(addr2);
		//phy_addr3 = dma_getPhyAddr(addr3);
		#if 0
		phy_addr0 = (addr0 != 0) ? frm_va2pa(addr0) : 0;
		phy_addr1 = (addr1 != 0) ? frm_va2pa(addr1) : 0;
		phy_addr2 = (addr2 != 0) ? frm_va2pa(addr2) : 0;
		phy_addr3 = (addr3 != 0) ? frm_va2pa(addr3) : 0;
		phy_addr4 = (addr4 != 0) ? frm_va2pa(addr4) : 0;
		#else
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		phy_addr1 = (addr1 != 0) ? nue_pt_va2pa(addr1) : 0;
		phy_addr2 = (addr2 != 0) ? nue_pt_va2pa(addr2) : 0;
		phy_addr3 = (addr3 != 0) ? nue_pt_va2pa(addr3) : 0;
		phy_addr4 = (addr4 != 0) ? nue_pt_va2pa(addr4) : 0;
		phy_addr5 = (addr5 != 0) ? nue_pt_va2pa(addr5) : 0;
		#endif

		nue_set_dmain_addr(phy_addr0, phy_addr5);
		nue_set_dmain_svmaddr(phy_addr1, phy_addr2, phy_addr3);
		nue_set_dmain_kqaddr(phy_addr4);

#if 0
		nvt_dbg(INFO, "addr0:%08x\r\n", addr0);
		nvt_dbg(INFO, "addr1:%08x\r\n", addr1);
		nvt_dbg(INFO, "addr2:%08x\r\n", addr2);
		nvt_dbg(INFO, "addr3:%08x\r\n", addr3);


		nvt_dbg(INFO, "phy_addr0:%08x\r\n", phy_addr0);
		nvt_dbg(INFO, "phy_addr1:%08x\r\n", phy_addr1);
		nvt_dbg(INFO, "phy_addr2:%08x\r\n", phy_addr2);
		nvt_dbg(INFO, "phy_addr3:%08x\r\n", phy_addr3);
#endif

		// Output Dram DMA/Cache Handle
		addr0 = p_addr->addr_out;
		if (addr0 != 0) {
			//flushsize = (insize.height * in_svmsize.objnum * outbyte);
			//dma_flushReadCache(addr0, flushsize);
#if (NUE_AI_FLOW == ENABLE)
#else
			//nue_pt_dma_flush(addr0, flushsize);

#endif
		}
		//phy_addr0 = dma_getPhyAddr(addr0);
		#if 0
		phy_addr0 = (addr0 != 0) ? frm_va2pa(addr0) : 0;
		#else
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		#endif
		nue_set_dmaout_addr(phy_addr0);
		break;
	case NUE_ROIPOOLING:
		// Input Dram DMA/Cache Handle
		addr0 = p_addr->addrin0;
		addr1 = p_addr->addrin_roi;
		if (addr0 != 0) {
			flushsize = (insize.width * insize.height * insize.channel * inbyte);
			//dma_flushWriteCache(addr0, flushsize);
			if (p_addr->drv_dma_not_sync[0] == 0) {
				nue_pt_dma_flush_mem2dev(addr0, flushsize);
			}
		}
		if (addr1 != 0) {
			flushsize = (nue_get_roinum() * sizeof(UINT16) * 4);
			//dma_flushWriteCache(addr1, flushsize);
			if (p_addr->drv_dma_not_sync[1] == 0) {
				nue_pt_dma_flush_mem2dev(addr1, flushsize);
			}
		}

		//phy_addr0 = dma_getPhyAddr(addr0);
		//phy_addr1 = dma_getPhyAddr(addr1);
		#if 0
		phy_addr0 = (addr0 != 0) ? frm_va2pa(addr0) : 0;
		phy_addr1 = (addr1 != 0) ? frm_va2pa(addr1) : 0;
		#else
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		phy_addr1 = (addr1 != 0) ? nue_pt_va2pa(addr1) : 0;
		#endif
		nue_set_dmain_addr(phy_addr0, phy_addr0);
		nue_set_dmain_roiaddr(phy_addr1);

		// Output Dram DMA/Cache Handle
		addr0 = p_addr->addr_out;
		if (addr0 != 0) {
			UINT32 pool_w = 0, pool_h = 0;
			if (nue_get_roipool_mode() == NUE_NORMAL_ROI) {
				if (nue_get_roipool_standard_kersize() == NUE_ROI_POOL_SIZE_6) {
					pool_w = 6;
					pool_h = 6;
				} else if (nue_get_roipool_standard_kersize() == NUE_ROI_POOL_SIZE_7) {
					pool_w = 7;
					pool_h = 7;
				} else if (nue_get_roipool_standard_kersize() == NUE_ROI_POOL_SIZE_14) {
					pool_w = 14;
					pool_h = 14;
				} else {
					nvt_dbg(WRN, "Standard roipooling kernel size unknown!\r\n");
					pool_w = 6;
					pool_h = 6;
				}
			} else if (nue_get_roipool_mode() == NUE_PS_ROI) {
				if (nue_get_roipool_psroi_kersize() == NUE_PSROI_POOL_SIZE_1) {
					pool_w = 1;
					pool_h = 1;
				} else if (nue_get_roipool_psroi_kersize() == NUE_PSROI_POOL_SIZE_2) {
					pool_w = 2;
					pool_h = 2;
				} else if (nue_get_roipool_psroi_kersize() == NUE_PSROI_POOL_SIZE_3) {
					pool_w = 3;
					pool_h = 3;
				} else if (nue_get_roipool_psroi_kersize() == NUE_PSROI_POOL_SIZE_4) {
					pool_w = 4;
					pool_h = 4;
				} else if (nue_get_roipool_psroi_kersize() == NUE_PSROI_POOL_SIZE_5) {
					pool_w = 5;
					pool_h = 5;
				} else if (nue_get_roipool_psroi_kersize() == NUE_PSROI_POOL_SIZE_6) {
					pool_w = 6;
					pool_h = 6;
				} else if (nue_get_roipool_psroi_kersize() == NUE_PSROI_POOL_SIZE_7) {
					pool_w = 7;
					pool_h = 7;
				} else {
					nvt_dbg(WRN, "Psroipooling kernel size unknown!\r\n");
					pool_w = 1;
					pool_h = 1;
				}
			} else {
				nvt_dbg(WRN, "Roipooling mode unknown!\r\n");
				pool_w = 1;
				pool_h = 1;
			}
			flushsize = ((pool_w * pool_h * insize.channel) * nue_get_roinum() * outbyte);
			if (0) {
				nue_pt_dma_flush_mem2dev(addr0, flushsize);
			}
#if (NUE_AI_FLOW == ENABLE)
#else
			//nue_pt_dma_flush(addr0, flushsize);
#endif
		}
		//phy_addr0 = dma_getPhyAddr(addr0);
		#if 0
		phy_addr0 = (addr0 != 0) ? frm_va2pa(addr0) : 0;
		#else
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		#endif
		nue_set_dmaout_addr(phy_addr0);
		break;
	case NUE_REORGANIZATION:
		// Input Dram DMA/Cache Handle
		addr0 = p_addr->addrin0;
		if (addr0 != 0) {
			flushsize = (insize.width * insize.height * insize.channel * inbyte);
			//dma_flushWriteCache(addr0, flushsize);
			if (p_addr->drv_dma_not_sync[0] == 0) {
				nue_pt_dma_flush_mem2dev(addr0, flushsize);
			}
		}
		//phy_addr0 = dma_getPhyAddr(addr0);
		#if 0
		phy_addr0 = (addr0 != 0) ? frm_va2pa(addr0) : 0;
		#else
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		#endif
		nue_set_dmain_addr(phy_addr0, phy_addr0);

		// Output Dram DMA/Cache Handle
		addr0 = p_addr->addr_out;
		if (addr0 != 0) {
			//flushsize = (insize.width * insize.height * insize.channel * outbyte);
			//dma_flushReadCache(addr0, flushsize);
#if (NUE_AI_FLOW == ENABLE)
#else
			//nue_pt_dma_flush(addr0, flushsize);
#endif
		}
		//phy_addr0 = dma_getPhyAddr(addr0);
		#if 0
		phy_addr0 = (addr0 != 0) ? frm_va2pa(addr0) : 0;
		#else
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		#endif
		nue_set_dmaout_addr(phy_addr0);
		break;
	case NUE_PERMUTE:
		// Input Dram DMA/Cache Handle
		addr0 = p_addr->addrin0;
		if (addr0 != 0) {
			flushsize = (insize.width * insize.height * insize.channel * outbyte);
			//dma_flushWriteCache(addr0, flushsize);
			if (p_addr->drv_dma_not_sync[0] == 0) {
				nue_pt_dma_flush_mem2dev(addr0, flushsize);
			}
		}
		//phy_addr0 = dma_getPhyAddr(addr0);
		#if 0
		phy_addr0 = (addr0 != 0) ? frm_va2pa(addr0) : 0;
		#else
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		#endif
		nue_set_dmain_addr(phy_addr0, phy_addr0);

		// Output Dram DMA/Cache Handle
		addr0 = p_addr->addr_out;
		if (addr0 != 0) {
			//flushsize = (insize.width * insize.height * insize.channel * outbyte);
			//dma_flushReadCache(addr0, flushsize);
#if (NUE_AI_FLOW == ENABLE)
#else
			//nue_pt_dma_flush(addr0, flushsize);
#endif
		}
		//phy_addr0 = dma_getPhyAddr(addr0);
		#if 0
		phy_addr0 = (addr0 != 0) ? frm_va2pa(addr0) : 0;
		#else
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		#endif
		nue_set_dmaout_addr(phy_addr0);
		break;
	case NUE_ANCHOR:
		// Input Dram DMA/Cache Handle
		addr0 = p_addr->addrin0;
		addr1 = p_addr->addrin_sv;
		addr2 = p_addr->addrin_alpha;
		addr3 = p_addr->addrin_rho;
		
		if (addr0 != 0) {
			flushsize = (insize.width * insize.height * insize.channel * inbyte);
			if (p_addr->drv_dma_not_sync[0] == 0) {
				nue_pt_dma_flush_mem2dev(addr0, flushsize);
			}
		}
		if (addr1 != 0) {
			flushsize = insize.width * insize.height * 12; // 12 bits per pixel
			if (flushsize % 32 > 0)
				flushsize += (32 - (flushsize % 32));
			flushsize *= (insize.channel/2);
			flushsize /= 8;
			if (p_addr->drv_dma_not_sync[2] == 0) {
				nue_pt_dma_flush_mem2dev(addr1, flushsize);
			}
		}
		if (addr2 != 0) {
			flushsize = 256*2;
			if (p_addr->drv_dma_not_sync[3] == 0) {
				nue_pt_dma_flush_mem2dev(addr2, flushsize);
			}
		}
		if (addr3 != 0) {
			flushsize = insize.width * insize.height * 12; // 12 bits per pixel
			if (flushsize % 32 > 0)
				flushsize += (32 - (flushsize % 32));
			flushsize *= (insize.channel/2);
			flushsize /= 8;
			if (p_addr->drv_dma_not_sync[4] == 0) {
				nue_pt_dma_flush_mem2dev(addr3, flushsize);
			}
		}
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		phy_addr1 = (addr1 != 0) ? nue_pt_va2pa(addr1) : 0;
		phy_addr2 = (addr2 != 0) ? nue_pt_va2pa(addr2) : 0;
		phy_addr3 = (addr3 != 0) ? nue_pt_va2pa(addr3) : 0;


		nue_set_dmain_addr(phy_addr0, phy_addr0);
		nue_set_dmain_svmaddr(phy_addr1, phy_addr2, phy_addr3);

		// Output Dram DMA/Cache Handle
		addr0 = p_addr->addr_out;
		if (addr0 != 0) {
			//flushsize = (insize.width * insize.height * insize.channel * outbyte);
			//dma_flushReadCache(addr0, flushsize);
#if (NUE_AI_FLOW == ENABLE)
#else
			//nue_pt_dma_flush(addr0, flushsize);
#endif
		}
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		nue_set_dmaout_addr(phy_addr0);
		break;
	case NUE_SOFTMAX:
		// Input Dram DMA/Cache Handle
		addr0 = p_addr->addrin0;
		if (addr0 != 0) {
			flushsize = (insize.width * insize.height * insize.channel * inbyte);
			//dma_flushWriteCache(addr0, flushsize);
			if (p_addr->drv_dma_not_sync[0] == 0) {
				nue_pt_dma_flush_mem2dev(addr0, flushsize);
			}
		}
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		nue_set_dmain_addr(phy_addr0, phy_addr0);

		// Output Dram DMA/Cache Handle
		addr0 = p_addr->addr_out;
		if (addr0 != 0) {
			//flushsize = (insize.width * insize.height * insize.channel * outbyte);
#if (NUE_AI_FLOW == ENABLE)
#else
			//nue_pt_dma_flush(addr0, flushsize);
#endif
		}
		phy_addr0 = (addr0 != 0) ? nue_pt_va2pa(addr0) : 0;
		nue_set_dmaout_addr(phy_addr0);
		break;
	default:
		nvt_dbg(ERR, "Error unknown NUE engine mode: %d\r\n", nue_get_engmode());
		return E_CTX;
	}
	//linked list addr not flush here
	//nue_set_dmain_lladdr(p_addr->addrin_ll);
	//nue_set_dmain_lladdr_base(p_addr->addrin_ll_base);
#else
	nue_set_dmain_addr(p_addr->addrin0, p_addr->addrin1);
	nue_set_dmain_svmaddr(p_addr->addrin_sv, p_addr->addrin_alpha, p_addr->addrin_rho);
	nue_set_dmain_roiaddr(p_addr->addrin_roi);
	nue_set_dmain_lladdr(p_addr->addrin_ll);
	nue_set_dmain_lladdr_base(p_addr->addrin_ll_base);
	nue_set_dmain_kqaddr(p_addr->addrin_kq);
	nue_set_dmaout_addr(p_addr->addr_out);
#endif
	return E_OK;
}

/**
    NUE Get DRAM Input and Output Addresses

    NUE get four DMA input and one output starting addresses.

    @return NUE_DMAIO_ADDR DMA input and output buffer starting addresses.
*/
NUE_DMAIO_ADDR nue_get_dmaio_addr(VOID)
{
	NUE_DMAIO_ADDR dma_addr = { 0 };
	dma_addr.addrin0        = nue_get_dmain_addr(NUE_IN0_BUF);
	dma_addr.addrin1        = nue_get_dmain_addr(NUE_IN1_BUF);
	dma_addr.addrin_sv      = nue_get_dmain_addr(NUE_SV_BUF);
	dma_addr.addrin_alpha   = nue_get_dmain_addr(NUE_ALPHA_BUF);
	dma_addr.addrin_rho     = nue_get_dmain_addr(NUE_RHO_BUF);
	dma_addr.addrin_roi     = nue_get_dmain_addr(NUE_ROI_BUF);
	//dma_addr.addrin_ll      = nue_get_dmain_lladdr();
	//dma_addr.addrin_ll_base = nue_get_dmain_lladdr_base();
	dma_addr.addrin_kq      = nue_get_dmain_addr(NUE_KQ_BUF);
	dma_addr.addr_out       = nue_get_dmaout_addr();
	return dma_addr;
}

/**
    NUE Check If Engine Is Running

    Check if NUE engine is already started.

    @return
        \n-@b ENABLE: busy.
        \n-@b DISABLE: idle.
*/
BOOL nue_isenable(VOID)
{
	T_NUE_CONTROL_REGISTER reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_CONTROL_REGISTER_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if ((reg1.bit.NUE_START == 1) || (reg1.bit.LL_FIRE == 1)) {
		return ENABLE;
	} else {
		return DISABLE;
	}
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
    NUE Check If Engine(Linked list) Is Terminated

    Check if NUE engine(linked list) is already terminated.

    @return
        \n-@b ENABLE: stop.
        \n-@b DISABLE: idle or run.
*/
BOOL nue_is_linklist_terminated(VOID)
{
	T_NUE_LL_TERMINATE_REGISTER reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	// TODO: fix the ll terminate register ofs
	UINT32 ofs = NUE_LL_TERMINATE_REGISTER_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	if (reg1.bit.LL_TERMINATE == 1) {
		return ENABLE;
	} else {
		return DISABLE;
	}
}

/**
	NUE Set AXI Disable

	set NUE AXI bus disable/enable.

    @param[in] is_dis Decide AXI bus is enabled or disabled.
        \n-@b TRUE: disable AXI bus.
        \n-@b FALSE: enable AXI bus.

    @return None.
*/
/*
VOID nue_set_axidis(BOOL is_dis)
{
	T_NUE_AXI_REGISTER0 reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_AXI_REGISTER0_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	reg1.bit.AXI_CH_DIS = is_dis;
	NUE_SETDATA(ofs, reg1.reg, base_addr);
}
*/

/**
	NUE Get AXI Disable

	get NUE AXI bus disable/enable.

    @return
        \n-@b TRUE: disable axi bus.
        \n-@b FALSE: enable axi bus.
*/
/*
BOOL nue_get_axidis(VOID)
{
	T_NUE_AXI_REGISTER0 reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_AXI_REGISTER0_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	return reg1.bit.AXI_CH_DIS;
}
*/

/**
	NUE Get AXI Idle

	get NUE AXI idle status.

    @return
        \n-@b TRUE: axi is idle mode.
        \n-@b FALSE: axi is not idle mode.
*/
/*
BOOL nue_get_axiidle(VOID)
{
	T_NUE_AXI_REGISTER0 reg1;
	UINT32 base_addr = NUE_IOADDR_REG_BASE;
	UINT32 ofs = NUE_AXI_REGISTER0_OFS;
	reg1.reg = NUE_GETDATA(ofs, base_addr);
	return reg1.bit.AXI_CH_IDLE;
}
*/

/**
    NUE Transform Integer Value

    NUE transform the int value to the smaller bit size value.

    @param[in] value input the 32 bits signed integer value.
    @param[in] bits bit size of the value.

    @return unsigned integer value with finite bit size.
*/
UINT32 nue_tran_intval(INT32 value, UINT32 bits)
{
	UINT32 outVal;
	if (value < 0) {
		outVal = (UINT32)((INT32)(1 << (bits)) + value);
	} else {
		outVal = (UINT32)value;
	}
	return outVal;
}

/**
    NUE Transform Bit Value

    NUE transform the small bit size value to the signed 32 bits value.

    @param[in] value input the unsigned integer value with finite bit size.
    @param[in] bits bit size of the value.

    @return 32 bits signed integer value.
*/
INT32 nue_tran_bitval(UINT32 value, UINT32 bits)
{
	if (bits < 1) {
		nvt_dbg(ERR, "bits should higher than 0: %d\r\n", (int)bits);
		return 0;
	}

	if (value < (UINT32)(1 << (bits - 1))) {
		return (INT32)value;
	} else {
		return (INT32)value - (INT32)(1 << bits);
	}
}

/**
    NUE check kernel 1 over flow

    NUE check kernel 1 whether output value is over s32 bits.

    @return
        \n-@b TRUE: overflow.
        \n-@b FALSE: not overflow.
*/
BOOL nue_chk_ker1_overflow(VOID)
{
#if 0
	NUE_SVM_IN_SIZE size;
	NUE_SVMKERL_PARM parm;
	INT64 width, gv, gv_shf, coef;
	INT64 pred_val = 0;
	const INT64 up_bnd = ((INT64)1 << 31) - 1;
	const INT64 lo_bnd = -((INT64)1 << 31);

	size = nue_get_svmsize();
	parm = nue_get_parm();
	width   = size.sv_w;
	gv      = parm.gv;
	gv_shf  = parm.gv_shf;
	coef    = parm.coef;

	if (coef < 0) {
		pred_val = (-(1 << 17));
	} else {
		pred_val = ((1 << 17) - 1);
	}
	pred_val = ((pred_val * width * gv) >> gv_shf) + coef;

	if ((pred_val > up_bnd) || (pred_val < lo_bnd)) {
		nvt_dbg(WRN, "kernel 1 output probably overflow, please adjust gv(%d) and gv_shf(%d)\r\n", (int)gv, (int)gv_shf);
		return TRUE;
	} else {
		return FALSE;
	}
#endif
	return FALSE;
}


//interrupt
EXPORT_SYMBOL(nue_get_int_enable);
EXPORT_SYMBOL(nue_clr_intr_status);
EXPORT_SYMBOL(nue_get_intr_status);
//basic function
EXPORT_SYMBOL(nue_set_dmain_lladdr);
EXPORT_SYMBOL(nue_get_dmain_lladdr);
EXPORT_SYMBOL(nue_set_dmain_lladdr_base);
EXPORT_SYMBOL(nue_get_dmain_lladdr_base);
//Engine
//EXPORT_SYMBOL(nue_get_eltin_mode);
//EXPORT_SYMBOL(nue_get_eltin_sign);
//EXPORT_SYMBOL(nue_get_eltout_mode);
//Eltwise
//EXPORT_SYMBOL(nue_get_eltshf);
//EXPORT_SYMBOL(nue_get_eltcoef);
//Pooling
//EXPORT_SYMBOL(nue_get_poolkertype);
//EXPORT_SYMBOL(nue_get_localpool_parm);
//EXPORT_SYMBOL(nue_get_globalpool_parm);
//check
EXPORT_SYMBOL(nue_chk_ker1_overflow);
// dma disable & idle
EXPORT_SYMBOL(nue_set_dma_disable);
EXPORT_SYMBOL(nue_get_dma_disable);
EXPORT_SYMBOL(nue_get_engine_idle);

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
EXPORT_SYMBOL(nue_set_in_rfh);
EXPORT_SYMBOL(nue_get_ker);
EXPORT_SYMBOL(nue_get_engmode);
EXPORT_SYMBOL(nue_get_kerl_en);
EXPORT_SYMBOL(nue_isenable);
EXPORT_SYMBOL(nue_get_outtype);
EXPORT_SYMBOL(nue_get_intype);
EXPORT_SYMBOL(nue_get_dmao_en);
EXPORT_SYMBOL(nue_get_svmsize);
EXPORT_SYMBOL(nue_get_dmaio_addr);
EXPORT_SYMBOL(nue_get_roinum);
EXPORT_SYMBOL(nue_get_insize);
EXPORT_SYMBOL(nue_get_in_rfh);
//ReLU
EXPORT_SYMBOL(nue_get_relu_leaky);
EXPORT_SYMBOL(nue_get_relu_shf);
//ROI Pooling
EXPORT_SYMBOL(nue_get_roipool_standard_kersize);
EXPORT_SYMBOL(nue_get_roipool_psroi_kersize);
EXPORT_SYMBOL(nue_get_roipool_ratio);
EXPORT_SYMBOL(nue_get_roipool_shf);
//SVM
EXPORT_SYMBOL(nue_get_rsts);
EXPORT_SYMBOL(nue_get_parm);
//Reorg
EXPORT_SYMBOL(nue_get_reorgshf);
#endif

//@}
