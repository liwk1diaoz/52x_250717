/*
    NUE2 module driver

    NT98520 NUE2 driver.

    @file       nue2_int.c
    @ingroup    mIIPPNUE2

    Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
*/

#include "nue2_platform.h"
#include "nue2_lib.h"
#include "nue2_reg.h"
#include "nue2_int.h"

#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
static NUE2_OPMODE nue2_mode = NUE2_OPMODE_USERDEFINE;
VOID nue2_set_drvmode(NUE2_OPMODE mode);
#endif

/*
    NUE2 Engine Control API
*/

/**
	NUE2 Set Driver Mode

	NUE2 set driver operation mode.

	@param[in] mode select driver operation mode.

	@return None.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
VOID nue2_set_drvmode(NUE2_OPMODE mode)
{
	nue2_mode = mode;
}

/**
	NUE2 Get Driver Mode

	NUE2 get driver operation mode.

	@return NUE2_OPMODE operation mode.
*/
NUE2_OPMODE nue2_get_drvmode(VOID)
{
	return nue2_mode;
}
#endif

/**
	NUE2 Get Cycle

	NUE2 get current cycle.

	@return UINT32 cycle.
*/
#if NUE2_CYCLE_TEST
UINT32 nue2_getcycle(VOID)
{
	T_NUE2_APP_CYCLE_COUNT_REGISTER_528 reg0_528;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    UINT32 ofs;

    if(nvt_get_chip_id() == CHIP_NA51055) {
		return NUE2_GETDATA(0xbc, NUE2_IOADDR_REG_BASE);
    } else {
        //if (mode == NUE2_CYCLE_OFF) {
        //} else if (mode == NUE2_CYCLE_APP) {
            ofs = NUE2_APP_CYCLE_COUNT_REGISTER_OFS_528;
            reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
            return reg0_528.bit.APP_CYCLE_COUNT;
        //} else if (mode == NUE2_CYCLE_LL) {
        //}
    }
}
#endif

/**
    Enable/Disable NUE2 Interrupt

    NUE2 interrupt enable selection.

    @param[in] enable Decide selected funtions are to be enabled or disabled.
        \n-@b ENABLE: enable selected functions.
        \n-@b DISABLE: disable selected functions.
    @param[in] intr Indicate the function(s).

    @return None.
*/
VOID nue2_enable_int(BOOL enable, UINT32 intr)
{
	T_NUE2_INTERRUPT_ENABLE_REGISTER reg0;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE2_INTERRUPT_ENABLE_REGISTER_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);

	if (enable) {
		reg0.reg |= intr;
	} else {
		reg0.reg &= (~intr);
	}

	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Get Interrupt Enable Status

    Get current NUE2 interrupt Enable status.

    @return NUE2 interrupt Enable status.
*/
UINT32 nue2_get_int_enable(VOID)
{
	T_NUE2_INTERRUPT_ENABLE_REGISTER reg0;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE2_INTERRUPT_ENABLE_REGISTER_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);

	return reg0.reg;
}

/**
    NUE2 Clear Interrupt Status

    Clear NUE2 interrupt status.

    @param[in] uiStatus 1's in this word will clear corresponding interrupt status.

    @return None.
*/
VOID nue2_clr_intr_status(UINT32 status)
{
	T_NUE2_INTERRUPT_STATUS_REGISTER reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_INTERRUPT_STATUS_REGISTER_OFS;
	reg0.reg = status;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Get Interrupt Status

    Get current NUE2 interrupt status.

    @return NUE2 interrupt status readout.
*/
UINT32 nue2_get_intr_status(VOID)
{
	T_NUE2_INTERRUPT_STATUS_REGISTER reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_INTERRUPT_STATUS_REGISTER_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	return reg0.reg;
}

/**
    NUE2 Reset

    Enable/disable NUE2 SW reset.

    @param[in] bReset.
        \n-@b TRUE: enable reset.
        \n-@b FALSE: disable reset.

    @return None.
*/
VOID nue2_clr(BOOL reset)
{
	//T_NUE2_CONTROL_REGISTER reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_CONTROL_REGISTER_OFS;
	//reg0.reg = NUE2_GETDATA(ofs, base_addr);
	//reg0.bit.NUE2_RST    = reset;
	NUE2_SETDATA(ofs, (UINT32)reset, base_addr);
}

/**
    NUE2 Enable

    Trigger NUE2 HW start.

    @param[in] bStart.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
VOID nue2_enable(BOOL start)
{
	T_NUE2_CONTROL_REGISTER reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_CONTROL_REGISTER_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_START = start;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Linked list Enable

    Trigger NUE2 Linked list HW start.

    @param[in] bStart.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
VOID nue2_ll_enable(BOOL start)
{
	T_NUE2_CONTROL_REGISTER reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_CONTROL_REGISTER_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.LL_FIRE = start;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Linked list Terminate

    Trigger NUE2 Linked list HW terminate.

    @param[in] isterminate.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
VOID nue2_ll_terminate(BOOL isterminate)
{
	T_LL_TERMINATE_RESISTER0 reg0;
	T_LL_TERMINATE_RESISTER0_528 reg0_528;

	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = LL_TERMINATE_RESISTER0_OFS;
	UINT32 ofs_528 = LL_TERMINATE_RESISTER0_OFS_528;

	if(nvt_get_chip_id() == CHIP_NA51055) {
		reg0.reg = NUE2_GETDATA(ofs, base_addr);
		reg0.bit.LL_TERMINATE = isterminate;
		NUE2_SETDATA(ofs, reg0.reg, base_addr);
	} else {
		reg0_528.reg = NUE2_GETDATA(ofs_528, base_addr);
		reg0_528.bit.LL_TERMINATE = isterminate;
		NUE2_SETDATA(ofs_528, reg0_528.reg, base_addr);
	}
}

/**
    NUE2 Check If Engine Is Running

    Check if NUE2 engine is already started.

    @return
        \n-@b ENABLE: busy.
        \n-@b DISABLE: idle.
*/
BOOL nue2_isenable(VOID)
{
    UINT32 ofs;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;

	T_NUE2_CONTROL_REGISTER reg0;
	ofs = NUE2_CONTROL_REGISTER_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);

	if ((reg0.bit.NUE2_START == 1) || (reg0.bit.LL_FIRE == 1)) {
		return ENABLE;
	} else {
		return DISABLE;
	}
}

/*
    NUE2 function API
*/

/**
    NUE2 Set YUV to RGB enable

    @param[in] bEn.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
VOID nue2_setYuv2rgbEnReg(BOOL bEn)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_YUV2RGB_EN = (UINT32)bEn;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}


/**
    NUE2 Get YUVtoRGB enable status

    Get current YUVtoRGB enable status

    @return BOOL enable status.
*/
BOOL nue2_getYuv2rgbEnReg(VOID)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = 0;
	UINT32 ofs0 = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs0, base_addr);

	return (BOOL)reg0.bit.NUE2_YUV2RGB_EN;
}

/**
    NUE2 Set Mean Subtraction enable

    @param[in] bEn.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
VOID nue2_setSubEnReg(BOOL bEn)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_SUB_EN = (UINT32)bEn;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Get Mean Subtraction enable status

    Get current Mean Subtraction enable status

    @return BOOL enable status.
*/
BOOL nue2_getSubEnReg(VOID)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = 0;
	UINT32 ofs0 = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs0, base_addr);

	return (BOOL)reg0.bit.NUE2_SUB_EN;
}

/**
    NUE2 Set Padding enable

    @param[in] bEn.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
VOID nue2_setPadEnReg(BOOL bEn)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_PAD_EN = (UINT32)bEn;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Get Padding enable status

    Get current Padding enable status

    @return BOOL enable status.
*/
BOOL nue2_getPadEnReg(VOID)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = 0;
	UINT32 ofs0 = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs0, base_addr);

	return (BOOL)reg0.bit.NUE2_PAD_EN;
}

/**
    NUE2 Set HSV enable

    @param[in] bEn.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
VOID nue2_setHsvEnReg(BOOL bEn)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_HSV_EN = (UINT32)bEn;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Get HSV enable status

    Get current HSV enable status

    @return BOOL enable status.
*/
BOOL nue2_getHsvEnReg(VOID)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = 0;
	UINT32 ofs0 = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs0, base_addr);

	return (BOOL)reg0.bit.NUE2_HSV_EN;
}

/**
    NUE2 Set Rotate enable

    @param[in] bEn.
        \n-@b TRUE: enable.
        \n-@b FALSE: disable.

    @return None.
*/
VOID nue2_setRotateEnReg(BOOL bEn)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_ROTATE_EN = (UINT32)bEn;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Get Rotate enable status

    Get current Rotate enable status

    @return BOOL enable status.
*/
BOOL nue2_getRotateEnReg(VOID)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = 0;
	UINT32 ofs0 = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs0, base_addr);

	return (BOOL)reg0.bit.NUE2_ROTATE_EN;
}

/**
    NUE2 Set Input format

    Set NUE2 input format.

    @param[in] in_fmt input format.

    @return None.
*/
VOID nue2_set_infmt(NUE2_IN_FMT in_fmt)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);

    if (in_fmt == NUE2_YUV_420) {
        reg0.bit.NUE2_IN_FMT = 0;
    } else if (in_fmt == NUE2_Y_CHANNEL) {
        reg0.bit.NUE2_IN_FMT = 1;
    } else if (in_fmt == NUE2_UV_CHANNEL) {
        reg0.bit.NUE2_IN_FMT = 2;
    } else {
        reg0.bit.NUE2_IN_FMT = 3;
    }

	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Get Input format

    Get current NUE2 input format.

    @return input format status.
*/
NUE2_IN_FMT nue2_get_infmt(VOID)
{
    NUE2_IN_FMT in_fmt;
    T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);

    if (reg0.bit.NUE2_IN_FMT == NUE2_YUV_420) {
        in_fmt = 0;
    } else if (reg0.bit.NUE2_IN_FMT == NUE2_Y_CHANNEL) {
        in_fmt = 1;
    } else if (reg0.bit.NUE2_IN_FMT == NUE2_UV_CHANNEL) {
        in_fmt = 2;
    } else {
        in_fmt = 3;
    }

    return in_fmt;
}

/*
    NUE2 set function enabel driver

    Set all functions' enable and options

    @param[in] FuncEn NUE2 output control structure

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_set_func_en(NUE2_FUNC_EN func_en)
{
	ER erReturn = E_OK;

    BOOL yuv2rgb_en = func_en.yuv2rgb_en;
    BOOL sub_en = func_en.sub_en;
    BOOL pad_en = func_en.pad_en;
    BOOL hsv_en = func_en.hsv_en;
    BOOL rotate_en = func_en.rotate_en;

    BOOL path_1 = (yuv2rgb_en | sub_en | pad_en);
    BOOL path_2 = (hsv_en);
    BOOL path_3 = (rotate_en);
    BOOL path_4 = (sub_en | pad_en);
    NUE2_IN_FMT in_fmt = nue2_get_infmt();

	if ((path_4)&&(path_2)) {
		//DBG_ERR("Error! Function of NUE2_OPMODE_1 & NUE2_OPMODE_2 can not both be enabled.\r\n");
        nvt_dbg(ERR, "Error! Function of NUE2_OPMODE_1 & NUE2_OPMODE_2 can not both be enabled.\r\n");
		return E_PAR;
	}
	if ((path_1)&&(path_3)) {
		//DBG_ERR("Error! Function of NUE2_OPMODE_1 & NUE2_OPMODE_3 can not both be enabled.\r\n");
        nvt_dbg(ERR, "Error! Function of NUE2_OPMODE_1 & NUE2_OPMODE_3 can not both be enabled.\r\n");
		return E_PAR;
	}
    if ((path_2)&&(path_3)) {
		//DBG_ERR("Error! Function of NUE2_OPMODE_2 & NUE2_OPMODE_3 can not both be enabled.\r\n");
        nvt_dbg(ERR, "Error! Function of NUE2_OPMODE_2 & NUE2_OPMODE_3 can not both be enabled.\r\n");
		return E_PAR;
	}

    if ((yuv2rgb_en)&&(in_fmt!=NUE2_YUV_420)) {
        //DBG_ERR("Error! If yuv2rgb enable, input format must be NUE2_YUV_420.\r\n");
        nvt_dbg(ERR, "Error! If yuv2rgb enable, input format must be NUE2_YUV_420.\r\n");
		return E_PAR;
    }
    if (sub_en | pad_en) {
        if ((in_fmt==NUE2_YUV_420)&&(yuv2rgb_en==0)) {
            //DBG_ERR("Error! If sub_en or pad_en, yuv2rgb_en disable, input format not support NUE2_YUV_420.\r\n");
            nvt_dbg(ERR, "Error! If sub_en or pad_en, yuv2rgb_en disable, input format not support NUE2_YUV_420.\r\n");
		    return E_PAR;
        }
    }
    if ((rotate_en)&&((in_fmt==NUE2_YUV_420)||(in_fmt==NUE2_RGB_PLANNER))) {
        //DBG_ERR("Error! If rotate_en, input format not support NUE2_YUV_420 or NUE2_RGB_PLANNER.\r\n");
        nvt_dbg(ERR, "Error! If rotate_en, input format not support NUE2_YUV_420 or NUE2_RGB_PLANNER.\r\n");
	    return E_PAR;
    }
	nue2_setYuv2rgbEnReg(yuv2rgb_en);
	nue2_setSubEnReg(sub_en);
	nue2_setPadEnReg(pad_en);
	nue2_setHsvEnReg(hsv_en);
	nue2_setRotateEnReg(rotate_en);

	return erReturn;
}

/*
    NUE2 get function enabel driver

    Get current all functions' enable and options

    @return function enable status
*/
NUE2_FUNC_EN nue2_get_func_en(VOID)
{
    NUE2_FUNC_EN func_en;
    T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = 0;
	UINT32 ofs0 = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs0, base_addr);

    func_en.yuv2rgb_en = (BOOL)reg0.bit.NUE2_YUV2RGB_EN;
    func_en.sub_en= (BOOL)reg0.bit.NUE2_SUB_EN;
    func_en.pad_en= (BOOL)reg0.bit.NUE2_PAD_EN;
    func_en.hsv_en= (BOOL)reg0.bit.NUE2_HSV_EN;
    func_en.rotate_en= (BOOL)reg0.bit.NUE2_ROTATE_EN;

	return func_en;
}


/**
    NUE2 Set Output format

    Set NUE2 Output format.

    @param[in] out_fmt output format.

    @return None.
*/
VOID nue2_set_outfmt(NUE2_OUT_FMT out_fmt)
{
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);

    if (out_fmt.out_signedness) {
        reg0.bit.NUE2_OUT_SIGNEDNESS = 1;
    } else {
        reg0.bit.NUE2_OUT_SIGNEDNESS = 0;
    }

	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Get Output format

    Get current NUE2 Output format.

    @return Output format status.
*/
NUE2_OUT_FMT nue2_get_outfmt(VOID)
{
    NUE2_OUT_FMT out_fmt;
    T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	UINT32 base_addr = 0;
	UINT32 ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	base_addr = NUE2_IOADDR_REG_BASE;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);

    out_fmt.out_signedness = reg0.bit.NUE2_OUT_SIGNEDNESS;

    return out_fmt;
}

/**
    NUE2 Set Input Size

    Set NUE2 Input Size.

    @param[in] p_size Input size.

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_set_insize(NUE2_IN_SIZE in_size)
{
    ER erReturn = E_OK;
    UINT32 in_width = in_size.in_width;
    UINT32 in_height = in_size.in_height;
    NUE2_IN_FMT in_fmt = nue2_get_infmt();
	T_NUE2_INPUT_SIZE_REGISTER0 reg0;
	T_NUE2_INPUT_SIZE_REGISTER0_528 reg0_528;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs;

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = NUE2_INPUT_SIZE_REGISTER0_OFS;
	} else {
		ofs = NUE2_INPUT_SIZE_REGISTER0_OFS_528;
	}	

    // normal in size upper boundary check
    if (in_width > FOR_USER_98520_NUE2_IN_WIDTH_MAX) {
        //DBG_ERR("Error! input width must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_WIDTH_MAX, in_width, FOR_USER_98520_NUE2_IN_WIDTH_MAX);
        nvt_dbg(ERR, "Error! input width must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_WIDTH_MAX, in_width, FOR_USER_98520_NUE2_IN_WIDTH_MAX);
        return E_PAR;
    }
	if(nvt_get_chip_id() == CHIP_NA51055) {
		if (in_height > FOR_USER_98520_NUE2_IN_HEIGHT_MAX) {
			//DBG_ERR("Error! input height must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_HEIGHT_MAX, in_height, FOR_USER_98520_NUE2_IN_HEIGHT_MAX);
			nvt_dbg(ERR, "Error! input height must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_HEIGHT_MAX, in_height, FOR_USER_98520_NUE2_IN_HEIGHT_MAX);
			return E_PAR;
		}
	} else {
		if (in_height > FOR_USER_98528_NUE2_IN_HEIGHT_MAX) {
			nvt_dbg(ERR, "Error! input height must be <= %d : %d -> %d\r\n", FOR_USER_98528_NUE2_IN_HEIGHT_MAX, in_height, FOR_USER_98528_NUE2_IN_HEIGHT_MAX);
			return E_PAR;
		}
	}
    // rotate enable in size upper boundary check
    if ((nue2_getRotateEnReg())&&(in_width > FOR_USER_98520_NUE2_IN_ROT_WIDTH_MAX)) {
        //DBG_ERR("Error! When Rotate enable, input width must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_ROT_WIDTH_MAX, in_width, FOR_USER_98520_NUE2_IN_ROT_WIDTH_MAX);
        nvt_dbg(ERR, "Error! When Rotate enable, input width must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_ROT_WIDTH_MAX, in_width, FOR_USER_98520_NUE2_IN_ROT_WIDTH_MAX);
        return E_PAR;
    }
    if ((nue2_getRotateEnReg())&&(in_height > FOR_USER_98520_NUE2_IN_ROT_HEIGHT_MAX)) {
        //DBG_ERR("Error! When Rotate enable, input height must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_ROT_HEIGHT_MAX, in_height, FOR_USER_98520_NUE2_IN_ROT_HEIGHT_MAX);
        nvt_dbg(ERR, "Error! When Rotate enable, input height must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_ROT_HEIGHT_MAX, in_height, FOR_USER_98520_NUE2_IN_ROT_HEIGHT_MAX);
        return E_PAR;
    }
    // normal in size lower boundary check
    if (in_width < FOR_USER_98520_NUE2_IN_WIDTH_MIN) {
        //DBG_ERR("Error! input width must be >= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_WIDTH_MIN, in_width, FOR_USER_98520_NUE2_IN_WIDTH_MIN);
        nvt_dbg(ERR, "Error! input width must be >= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_WIDTH_MIN, in_width, FOR_USER_98520_NUE2_IN_WIDTH_MIN);
        return E_PAR;
    }
    if (in_height < FOR_USER_98520_NUE2_IN_HEIGHT_MIN) {
        //DBG_ERR("Error! input height must be >= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_HEIGHT_MIN, in_height, FOR_USER_98520_NUE2_IN_HEIGHT_MIN);
        nvt_dbg(ERR, "Error! input height must be >= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_IN_HEIGHT_MIN, in_height, FOR_USER_98520_NUE2_IN_HEIGHT_MIN);
        return E_PAR;
    }
    // check format size
    if ((in_fmt==NUE2_YUV_420)&&(in_width%2!=0)) {
        //DBG_ERR("Error! Format in YUV-420, input width must be 2-byte aligned : %d\r\n", in_width);
        nvt_dbg(ERR, "Error! Format in YUV-420, input width must be 2-byte aligned : %d\r\n", in_width);
        return E_PAR;
    }
    if ((in_fmt==NUE2_YUV_420)&&(in_height%2!=0)) {
        //DBG_ERR("Error! Format in YUV-420, input height must be 2-byte aligned : %d\r\n", in_height);
        nvt_dbg(ERR, "Error! Format in YUV-420, input height must be 2-byte aligned : %d\r\n", in_height);
        return E_PAR;
    }

	if(nvt_get_chip_id() == CHIP_NA51055) {
		reg0.reg = NUE2_GETDATA(ofs, base_addr);
		reg0.bit.NUE2_IN_WIDTH = in_width;
		reg0.bit.NUE2_IN_HEIGHT = in_height;
		NUE2_SETDATA(ofs, reg0.reg, base_addr);
	} else {
		reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
		reg0_528.bit.NUE2_IN_WIDTH = in_width;
		reg0_528.bit.NUE2_IN_HEIGHT = in_height;
		NUE2_SETDATA(ofs, reg0_528.reg, base_addr);
	}

    return erReturn;
}

/**
    NUE2 Get Input Size

    Get current NUE2 Input Size.

    @return input size status.
*/

NUE2_IN_SIZE nue2_get_insize(VOID)
{
    NUE2_IN_SIZE in_size;
    T_NUE2_INPUT_SIZE_REGISTER0 reg0;
	T_NUE2_INPUT_SIZE_REGISTER0_528 reg0_528;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs;

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = NUE2_INPUT_SIZE_REGISTER0_OFS;
		reg0.reg = NUE2_GETDATA(ofs, base_addr);

    	in_size.in_width = reg0.bit.NUE2_IN_WIDTH;
    	in_size.in_height = reg0.bit.NUE2_IN_HEIGHT;
	} else {
		ofs = NUE2_INPUT_SIZE_REGISTER0_OFS_528;
		reg0_528.reg = NUE2_GETDATA(ofs, base_addr);

    	in_size.in_width = reg0_528.bit.NUE2_IN_WIDTH;
    	in_size.in_height = reg0_528.bit.NUE2_IN_HEIGHT;
	}

    return in_size;
}

/**
    NUE2 Set Scaling parameter

    Set NUE2 Scaling parameter.

    @param[in] scaling parameter including target size.

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_set_scale_parm(NUE2_SCALE_PARM scale_parm)
{
    UINT32 ofs;
    UINT32 cal_h_dnrate, cal_v_dnrate, cal_h_sfact, cal_v_sfact;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    ER erReturn = E_OK;

    BOOL fact_update = scale_parm.fact_update;
    UINT32 h_filtmode = (UINT32)scale_parm.h_filtmode;
    UINT32 v_filtmode = (UINT32)scale_parm.v_filtmode;
    UINT32 h_filtcoef = scale_parm.h_filtcoef;
    UINT32 v_filtcoef = scale_parm.v_filtcoef;
    UINT32 h_scl_size = scale_parm.h_scl_size;
    UINT32 v_scl_size = scale_parm.v_scl_size;
    UINT32 h_dnrate = scale_parm.h_dnrate;
    UINT32 v_dnrate = scale_parm.v_dnrate;
    UINT32 h_sfact = scale_parm.h_sfact;
    UINT32 v_sfact = scale_parm.v_sfact;
#if (NUE2_AI_FLOW == ENABLE)
	UINT32 ini_h_dnrate = 0;
    UINT32 ini_h_sfact = 0;
#else
	UINT32 ini_h_dnrate = scale_parm.ini_h_dnrate;
    UINT32 ini_h_sfact = scale_parm.ini_h_sfact;
#endif
	UINT32 scale_h_mode = scale_parm.scale_h_mode;
	UINT32 scale_v_mode = scale_parm.scale_v_mode;

    NUE2_IN_SIZE in_size = nue2_get_insize();
    NUE2_IN_FMT in_fmt = nue2_get_infmt();
    NUE2_FUNC_EN func_en = nue2_get_func_en();
    NUE2_SCALE_PARM scale_last_parm = nue2_get_scale_parm();
    UINT32 in_width = in_size.in_width;
    UINT32 in_height = in_size.in_height;

    T_SCALE_DOWN_RATE_REGISTER0 reg0;
    T_SCALE_DOWN_RATE_REGISTER1 reg1;
    T_SCALE_DOWN_RATE_REGISTER2 reg2;
    T_SCALING_OUTPUT_SIZE_REGISTER0 reg4;
	T_SCALING_OUTPUT_SIZE_REGISTER0_528 reg4_528;
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg5;

    //DBG_MSG("in_width : %d\r\n", in_width);
    //DBG_MSG("in_height : %d\r\n", in_height);

	cal_h_dnrate=0; 
	cal_v_dnrate=0; 
	cal_h_sfact=0; 
	cal_v_sfact=0;

    //check scale out size
    if ((h_scl_size<FOR_USER_98520_NUE2_OUT_WIDTH_MIN)||(v_scl_size<FOR_USER_98520_NUE2_OUT_HEIGHT_MIN)) {
        //DBG_ERR("Error! Scaling output width or height cannot be zero !\r\n");
        nvt_dbg(ERR, "Error! Scaling output width or height cannot be zero !\r\n");
        return E_PAR;
    }

	if(nvt_get_chip_id() == CHIP_NA51055) {
		if (h_scl_size > in_width) {
			//DBG_ERR("Error! Horizontal output scaling width (%d) must be <= input width (%d)\r\n", h_scl_size, in_width);
			nvt_dbg(ERR, "Error! Horizontal output scaling width (%d) must be <= input width (%d)\r\n", h_scl_size, in_width);
			return E_PAR;
		}
		if (v_scl_size > in_height) {
			//DBG_ERR("Error! Vertical output scaling height (%d) must be <= input height (%d)\r\n", h_scl_size, in_height);
			nvt_dbg(ERR, "Error! Vertical output scaling height (%d) must be <= input height (%d)\r\n", h_scl_size, in_height);
			return E_PAR;
		}
	} else {
		if ((h_scl_size > in_width) && (in_width > FOR_USER_98528_NUE2_IN_SCALE_UP_WIDTH_MAX)) {
			nvt_dbg(ERR, "Error! scaling up case (in_width(%d) < scale_h_out(%d)), the input_width must be <= (%d)\r\n", 
						in_width, h_scl_size, FOR_USER_98528_NUE2_IN_SCALE_UP_WIDTH_MAX);
			return E_PAR;
		}
		if ((v_scl_size > in_height) && (in_height > FOR_USER_98528_NUE2_IN_SCALE_UP_HEIGHT_MAX)) {
			nvt_dbg(ERR, "Error! scaling up case (in_height(%d) < scale_v_out(%d)), the input_height must be <= (%d)\r\n", 
						in_height, v_scl_size, FOR_USER_98528_NUE2_IN_SCALE_UP_HEIGHT_MAX);
			return E_PAR;
		}

		if ((h_scl_size > in_width) && (h_scl_size > FOR_USER_98528_NUE2_OUT_SCALE_UP_WIDTH_MAX)) {
			nvt_dbg(ERR, "Error! scaling up case (in_width(%d) < scale_h_out(%d)), the scale_h_out must be <= (%d)\r\n", 
						in_width, h_scl_size, FOR_USER_98528_NUE2_OUT_SCALE_UP_WIDTH_MAX);
			return E_PAR;
		}
		if ((v_scl_size > in_height) && (v_scl_size > FOR_USER_98528_NUE2_OUT_SCALE_UP_HEIGHT_MAX)) {
			nvt_dbg(ERR, "Error! scaling up case (in_height(%d) < scale_v_out(%d)), the scale_v_out must be <= (%d)\r\n", 
						in_height, v_scl_size, FOR_USER_98528_NUE2_OUT_SCALE_UP_HEIGHT_MAX);
			return E_PAR;
		}

		if ((h_scl_size > in_width) && (scale_h_mode == 0)) {
			nvt_dbg(ERR, "Error! scaling up case (in_width(%d) < scale_h_out(%d)), but scale_h_mode=0.\r\n", in_width, h_scl_size);
			return E_PAR;
		}

		if ((v_scl_size > in_height) && (scale_v_mode == 0)) {
			nvt_dbg(ERR, "Error! scaling up case (in_height(%d) < scale_v_out(%d)), but scale_v_mode=0.\r\n", in_height, v_scl_size);
			return E_PAR;
		}
	}

    if (((in_fmt==NUE2_YUV_420)&&(func_en.yuv2rgb_en==0))&&((h_scl_size!=in_width)||(v_scl_size!=in_height))) {
        //DBG_ERR("Error! Scaling not support NUE2_YUV_420 format.\r\n");
        nvt_dbg(ERR, "Error! Scaling not support NUE2_YUV_420 format.\r\n");
        return E_PAR;
    }

#if 1
    //check dnrate and sfact
	if(nvt_get_chip_id() == CHIP_NA51055) {
    	cal_h_dnrate = (((in_width-1)/(h_scl_size-1))-1);
    	cal_v_dnrate = (((in_height-1)/(v_scl_size-1))-1);
	}
    cal_h_sfact = (((in_width-1)*65536/(h_scl_size-1)) & 0xffff);
    cal_v_sfact = (((in_height-1)*65536/(v_scl_size-1)) & 0xffff);

	if(nvt_get_chip_id() == CHIP_NA51055) {
		if (h_dnrate!=cal_h_dnrate) {
			//DBG_WRN("Warning! Scaling horizontal down rate wrong calculated. Actual value : %d, User set : %d\r\n", cal_h_dnrate, h_dnrate);
			nvt_dbg(WRN, "Warning! Scaling horizontal down rate wrong calculated. Actual value : %d, User set : %d\r\n", cal_h_dnrate, h_dnrate);
			//h_dnrate = cal_h_dnrate;
			return E_PAR;
		}
		if (v_dnrate!=cal_v_dnrate) {
			//DBG_WRN("Warning! Scaling vertical down rate wrong calculated. Actual value : %d, User set : %d\r\n", cal_v_dnrate, v_dnrate);
			nvt_dbg(WRN, "Warning! Scaling vertical down rate wrong calculated. Actual value : %d, User set : %d\r\n", cal_v_dnrate, v_dnrate);
			//v_dnrate = cal_v_dnrate;
			return E_PAR;
		}
	}

    if (h_sfact!=cal_h_sfact) {
        //DBG_WRN("Warning! Scaling horizontal factor wrong calculated. Actual value : %d, User set : %d\r\n", cal_h_sfact, h_sfact);
        nvt_dbg(WRN, "Warning! Scaling horizontal factor wrong calculated. Actual value : %d, User set : %d\r\n", cal_h_sfact, h_sfact);
		nvt_dbg(WRN, "Warning! Scaling horizontal factor wrong calculated. Actual value : %d %d \r\n", in_width, h_scl_size);
        //h_sfact = cal_h_sfact;
		return E_PAR;
    }
    if (v_sfact!=cal_v_sfact) {
        //DBG_WRN("Warning! Scaling vertical factor wrong calculated. Actual value : %d, User set : %d\r\n", cal_v_sfact, v_sfact);
        nvt_dbg(WRN, "Warning! Scaling vertical factor wrong calculated. Actual value : %d, User set : %d\r\n", cal_v_sfact, v_sfact);
		nvt_dbg(WRN, "Warning! Scaling vertical factor wrong calculated. Actual value : %d %d \r\n", in_height, v_scl_size);
        //v_sfact = cal_v_sfact;
		return E_PAR;
    }
#endif

    // if MST, scaling initial factor require to update
    if (fact_update) {
        ini_h_dnrate = scale_last_parm.final_h_dnrate;
        ini_h_sfact = scale_last_parm.final_h_sfact;
    } else {
        ini_h_dnrate = 0;
        ini_h_sfact = 0;
    }

    // set register
	ofs = SCALE_DOWN_RATE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_H_DNRATE = h_dnrate;
    reg0.bit.NUE2_V_DNRATE = v_dnrate;
    reg0.bit.NUE2_H_FILTMODE = h_filtmode;
    reg0.bit.NUE2_H_FILTCOEF = h_filtcoef;
    reg0.bit.NUE2_V_FILTMODE = v_filtmode;
    reg0.bit.NUE2_V_FILTCOEF = v_filtcoef;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);

	ofs = SCALE_DOWN_RATE_REGISTER1_OFS;
	reg1.reg = NUE2_GETDATA(ofs, base_addr);
	reg1.bit.NUE2_H_SFACT = h_sfact;
    reg1.bit.NUE2_V_SFACT = v_sfact;
	NUE2_SETDATA(ofs, reg1.reg, base_addr);

	ofs = SCALE_DOWN_RATE_REGISTER2_OFS;
	reg2.reg = NUE2_GETDATA(ofs, base_addr);
	reg2.bit.NUE2_INI_H_DNRATE = ini_h_dnrate;
    reg2.bit.NUE2_INI_H_SFACT = ini_h_sfact;
	NUE2_SETDATA(ofs, reg2.reg, base_addr);

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = SCALING_OUTPUT_SIZE_REGISTER0_OFS;
		reg4.reg = NUE2_GETDATA(ofs, base_addr);
		reg4.bit.NUE2_H_SCL_SIZE = h_scl_size;
		reg4.bit.NUE2_V_SCL_SIZE = v_scl_size;
		NUE2_SETDATA(ofs, reg4.reg, base_addr);
	} else {
		ofs = SCALING_OUTPUT_SIZE_REGISTER0_OFS_528;
		reg4_528.reg = NUE2_GETDATA(ofs, base_addr);
		reg4_528.bit.NUE2_H_SCL_SIZE = h_scl_size;
		reg4_528.bit.NUE2_V_SCL_SIZE = v_scl_size;
		NUE2_SETDATA(ofs, reg4_528.reg, base_addr);
	}

	ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg5.reg = NUE2_GETDATA(ofs, base_addr);
	reg5.bit.NUE2_SCALE_H_MODE = scale_h_mode;
	reg5.bit.NUE2_SCALE_V_MODE = scale_v_mode;
	NUE2_SETDATA(ofs, reg5.reg, base_addr);

    return erReturn;
}

/**
    NUE2 Get Scaling parameter status

    Get current NUE2 Scaling parameter status.

    @return Scaling parameter status.
*/
NUE2_SCALE_PARM nue2_get_scale_parm(VOID)
{
    NUE2_SCALE_PARM scale_parm = {0};
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    T_SCALE_DOWN_RATE_REGISTER0 reg0;
    T_SCALE_DOWN_RATE_REGISTER1 reg1;
    T_SCALE_DOWN_RATE_REGISTER2 reg2;
    T_SCALE_DOWN_RATE_REGISTER3 reg3;
    T_SCALING_OUTPUT_SIZE_REGISTER0 reg4;
	T_SCALING_OUTPUT_SIZE_REGISTER0_528 reg4_528;

	ofs = SCALE_DOWN_RATE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	scale_parm.h_dnrate = reg0.bit.NUE2_H_DNRATE;
    scale_parm.v_dnrate = reg0.bit.NUE2_V_DNRATE;
    scale_parm.h_filtmode = (BOOL)reg0.bit.NUE2_H_FILTMODE;
    scale_parm.h_filtcoef = reg0.bit.NUE2_H_FILTCOEF;
    scale_parm.v_filtmode = (BOOL)reg0.bit.NUE2_V_FILTMODE;
    scale_parm.v_filtcoef = reg0.bit.NUE2_V_FILTCOEF;

	ofs = SCALE_DOWN_RATE_REGISTER1_OFS;
	reg1.reg = NUE2_GETDATA(ofs, base_addr);
	scale_parm.h_sfact = reg1.bit.NUE2_H_SFACT;
    scale_parm.v_sfact = reg1.bit.NUE2_V_SFACT;

	ofs = SCALE_DOWN_RATE_REGISTER2_OFS;
	reg2.reg = NUE2_GETDATA(ofs, base_addr);
	scale_parm.ini_h_dnrate = reg2.bit.NUE2_INI_H_DNRATE;
    scale_parm.ini_h_sfact = reg2.bit.NUE2_INI_H_SFACT;

	ofs = SCALE_DOWN_RATE_REGISTER3_OFS;
	reg3.reg = NUE2_GETDATA(ofs, base_addr);
	scale_parm.final_h_dnrate = reg3.bit.NUE2_FINAL_H_DNRATE;
    scale_parm.final_h_sfact = reg3.bit.NUE2_FINAL_H_SFACT;

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = SCALING_OUTPUT_SIZE_REGISTER0_OFS;
		reg4.reg = NUE2_GETDATA(ofs, base_addr);
		scale_parm.h_scl_size = reg4.bit.NUE2_H_SCL_SIZE;
		scale_parm.v_scl_size = reg4.bit.NUE2_V_SCL_SIZE;
	} else {
		ofs = SCALING_OUTPUT_SIZE_REGISTER0_OFS_528;
		reg4_528.reg = NUE2_GETDATA(ofs, base_addr);
		scale_parm.h_scl_size = reg4_528.bit.NUE2_H_SCL_SIZE;
		scale_parm.v_scl_size = reg4_528.bit.NUE2_V_SCL_SIZE;
	}
	scale_parm.fact_update = 0;

    return scale_parm;
}

/**
    NUE2 Set Mean Subtraction parameter

    Set NUE2 Mean Subtraction parameter.

    @param[in] Mean Subtraction parameter.

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_set_sub_parm(NUE2_SUB_PARM sub_parm)
{
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    ER erReturn = E_OK;

    UINT32 sub_mode = (UINT32)sub_parm.sub_mode;
    UINT32 sub_in_width = sub_parm.sub_in_width;
    UINT32 sub_in_height = sub_parm.sub_in_height;
    UINT32 sub_coef0 = sub_parm.sub_coef0;
    UINT32 sub_coef1 = sub_parm.sub_coef1;
    UINT32 sub_coef2 = sub_parm.sub_coef2;
    UINT32 sub_dup = (UINT32)sub_parm.sub_dup;
    UINT32 sub_shift = sub_parm.sub_shift;

    NUE2_FUNC_EN func_en = nue2_get_func_en();
    NUE2_SCALE_PARM scale_parm = nue2_get_scale_parm();
    UINT32 h_scl_size = scale_parm.h_scl_size;
    UINT32 v_scl_size = scale_parm.v_scl_size;
    UINT32 h_sub_dup = (1<<sub_dup)*sub_in_width;
    UINT32 v_sub_dup = (1<<sub_dup)*sub_in_height;

    T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
    T_MEAN_SUBTRACTION_REGISTER0 reg1;
    T_MEAN_SUBTRACTION_REGISTER0_528 reg1_528;
    T_MEAN_SUBTRACTION_REGISTER1 reg2;
	T_MEAN_SUBTRACTION_REGISTER1_528 reg2_528;

    if (func_en.sub_en) {
        if (sub_mode) {
			if(nvt_get_chip_id() == CHIP_NA51055) {
				if (sub_in_width > FOR_USER_98520_NUE2_IN_WIDTH_MAX) {
					//DBG_ERR("Error! Mean subtraction (Planer) input width must be <= %d : %d -> %d\r\n", 
								//FOR_USER_98520_NUE2_IN_WIDTH_MAX, sub_in_width, FOR_USER_98520_NUE2_IN_WIDTH_MAX);
					nvt_dbg(ERR, "Error! Mean subtraction (Planer) input width must be <= %d : %d -> %d\r\n", 
												FOR_USER_98520_NUE2_IN_WIDTH_MAX, sub_in_width, FOR_USER_98520_NUE2_IN_WIDTH_MAX);
					return E_PAR;
				}
			} else {
				if (sub_in_width > FOR_USER_98528_NUE2_SUB_IN_WIDTH_MAX) {
					//DBG_ERR("Error! Mean subtraction (Planer) input width must be <= %d : %d -> %d\r\n", 
								//FOR_USER_98528_NUE2_SUB_IN_WIDTH_MAX, sub_in_width, FOR_USER_98520_NUE2_IN_WIDTH_MAX);
					nvt_dbg(ERR, "Error! Mean subtraction (Planer) input width must be <= %d : %d -> %d\r\n", 
												FOR_USER_98528_NUE2_SUB_IN_WIDTH_MAX, sub_in_width, FOR_USER_98528_NUE2_SUB_IN_WIDTH_MAX);
					return E_PAR;
				}
			}
			if(nvt_get_chip_id() == CHIP_NA51055) {
				if (sub_in_height > FOR_USER_98520_NUE2_IN_HEIGHT_MAX) {
					//DBG_ERR("Error! Mean subtraction (Planer) input height must be <= %d : %d -> %d\r\n", 
						//FOR_USER_98520_NUE2_IN_HEIGHT_MAX, sub_in_height, FOR_USER_98520_NUE2_IN_HEIGHT_MAX);
					nvt_dbg(ERR, "Error! Mean subtraction (Planer) input height must be <= %d : %d -> %d\r\n", 
									FOR_USER_98520_NUE2_IN_HEIGHT_MAX, sub_in_height, FOR_USER_98520_NUE2_IN_HEIGHT_MAX);
					return E_PAR;
				}
			} else {
				if (sub_in_height > FOR_USER_98528_NUE2_IN_HEIGHT_MAX) {
					nvt_dbg(ERR, "Error! Mean subtraction (Planer) input height must be <= %d : %d -> %d\r\n", 
									FOR_USER_98528_NUE2_IN_HEIGHT_MAX, sub_in_height, FOR_USER_98528_NUE2_IN_HEIGHT_MAX);
					return E_PAR;
				}
			}
            if (h_scl_size!=h_sub_dup) {
                //DBG_WRN("Warning! Mean subtraction (Planer) input width (%d) != source input width (%d)\r\n", h_sub_dup, h_scl_size);
                nvt_dbg(WRN, "Warning! Mean subtraction (Planer) input width (%d) != source input width (%d)\r\n", h_sub_dup, h_scl_size);
				#if (NUE2_AI_FLOW == ENABLE)
				return E_PAR;
				#endif
            }
            if (v_scl_size!=v_sub_dup) {
                //DBG_WRN("Warning! Mean subtraction (Planer) input height (%d) != source input height (%d)\r\n", v_sub_dup, v_scl_size);
                nvt_dbg(WRN, "Warning! Mean subtraction (Planer) input height (%d) != source input height (%d)\r\n", v_sub_dup, v_scl_size);
				#if (NUE2_AI_FLOW == ENABLE)
				return E_PAR;
				#endif
            }
        } else {
            if (sub_coef0 > FOR_USER_98520_NUE2_SUB_COEF_MAX) {
                //DBG_ERR("Error! Mean subtraction (DC) coefficient-0 value must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_SUB_COEF_MAX, sub_coef0, FOR_USER_98520_NUE2_SUB_COEF_MAX);
                nvt_dbg(ERR, "Error! Mean subtraction (DC) coefficient-0 value must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_SUB_COEF_MAX, sub_coef0, FOR_USER_98520_NUE2_SUB_COEF_MAX);
                return E_PAR;
            }
            if (sub_coef1 > FOR_USER_98520_NUE2_SUB_COEF_MAX) {
                //DBG_ERR("Error! Mean subtraction (DC) coefficient-1 value must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_SUB_COEF_MAX, sub_coef1, FOR_USER_98520_NUE2_SUB_COEF_MAX);
                nvt_dbg(ERR, "Error! Mean subtraction (DC) coefficient-1 value must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_SUB_COEF_MAX, sub_coef1, FOR_USER_98520_NUE2_SUB_COEF_MAX);
                return E_PAR;
            }
            if (sub_coef2 > FOR_USER_98520_NUE2_SUB_COEF_MAX) {
                //DBG_ERR("Error! Mean subtraction (DC) coefficient-2 value must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_SUB_COEF_MAX, sub_coef2, FOR_USER_98520_NUE2_SUB_COEF_MAX);
                nvt_dbg(ERR, "Error! Mean subtraction (DC) coefficient-2 value must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_SUB_COEF_MAX, sub_coef2, FOR_USER_98520_NUE2_SUB_COEF_MAX);
                return E_PAR;
            }
        }

		if(nvt_get_chip_id() == CHIP_NA51055) {
			if (sub_shift > FOR_USER_98520_NUE2_SUB_SHF_MAX) {
				//DBG_ERR("Error! Mean subtraction shifting must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_SUB_SHF_MAX, sub_shift, FOR_USER_98520_NUE2_SUB_SHF_MAX);
				nvt_dbg(ERR, "Error! Mean subtraction shifting must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_SUB_SHF_MAX, sub_shift, FOR_USER_98520_NUE2_SUB_SHF_MAX);
				return E_PAR;
			}
		}
    }

	ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_SUB_MODE = sub_mode;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = MEAN_SUBTRACTION_REGISTER0_OFS;
		reg1.reg = NUE2_GETDATA(ofs, base_addr);
		reg1.bit.NUE2_SUB_IN_WIDTH = sub_in_width;
		reg1.bit.NUE2_SUB_IN_HEIGHT = sub_in_height;
		NUE2_SETDATA(ofs, reg1.reg, base_addr);
	} else {
		ofs = MEAN_SUBTRACTION_REGISTER0_OFS_528;
		reg1_528.reg = NUE2_GETDATA(ofs, base_addr);
		reg1_528.bit.NUE2_SUB_IN_WIDTH = sub_in_width;
		reg1_528.bit.NUE2_SUB_IN_HEIGHT = sub_in_height;
		NUE2_SETDATA(ofs, reg1_528.reg, base_addr);
	}

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = MEAN_SUBTRACTION_REGISTER1_OFS;
		reg2.reg = NUE2_GETDATA(ofs, base_addr);
		reg2.bit.NUE2_SUB_COEF_0 = sub_coef0;
		reg2.bit.NUE2_SUB_COEF_1 = sub_coef1;
		reg2.bit.NUE2_SUB_COEF_2 = sub_coef2;
		reg2.bit.NUE2_SUB_DUP = sub_dup;
		reg2.bit.NUE2_SUB_SHF = sub_shift;
		NUE2_SETDATA(ofs, reg2.reg, base_addr);
	} else {
		ofs = MEAN_SUBTRACTION_REGISTER1_OFS_528;
		reg2_528.reg = NUE2_GETDATA(ofs, base_addr);
		reg2_528.bit.NUE2_SUB_COEF_0 = sub_coef0;
		reg2_528.bit.NUE2_SUB_COEF_1 = sub_coef1;
		reg2_528.bit.NUE2_SUB_COEF_2 = sub_coef2;
		reg2_528.bit.NUE2_SUB_DUP = sub_dup;
		NUE2_SETDATA(ofs, reg2_528.reg, base_addr);
	}

    return erReturn;
}

/**
    NUE2 Get Mean Subtraction parameter status

    Get current NUE2 Mean Subtraction parameter status.

    @return Mean Subtraction parameter status.
*/
NUE2_SUB_PARM nue2_get_sub_parm(VOID)
{
    NUE2_SUB_PARM sub_parm;
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
    T_MEAN_SUBTRACTION_REGISTER0 reg1;
	T_MEAN_SUBTRACTION_REGISTER0_528 reg1_528;
    T_MEAN_SUBTRACTION_REGISTER1 reg2;
	T_MEAN_SUBTRACTION_REGISTER1_528 reg2_528;

	ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	sub_parm.sub_mode = (BOOL)reg0.bit.NUE2_SUB_MODE;

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = MEAN_SUBTRACTION_REGISTER0_OFS;
		reg1.reg = NUE2_GETDATA(ofs, base_addr);
		sub_parm.sub_in_width = reg1.bit.NUE2_SUB_IN_WIDTH;
		sub_parm.sub_in_height = reg1.bit.NUE2_SUB_IN_HEIGHT;
	} else {
		ofs = MEAN_SUBTRACTION_REGISTER0_OFS_528;
		reg1_528.reg = NUE2_GETDATA(ofs, base_addr);
		sub_parm.sub_in_width = reg1_528.bit.NUE2_SUB_IN_WIDTH;
		sub_parm.sub_in_height = reg1_528.bit.NUE2_SUB_IN_HEIGHT;
	}

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = MEAN_SUBTRACTION_REGISTER1_OFS;
		reg2.reg = NUE2_GETDATA(ofs, base_addr);
		sub_parm.sub_coef0 = reg2.bit.NUE2_SUB_COEF_0;
		sub_parm.sub_coef1 = reg2.bit.NUE2_SUB_COEF_1;
		sub_parm.sub_coef2 = reg2.bit.NUE2_SUB_COEF_2;
		sub_parm.sub_dup = (NUE2_SUBDUP_RATE)reg2.bit.NUE2_SUB_DUP;
		sub_parm.sub_shift = reg2.bit.NUE2_SUB_SHF;
	} else {
		ofs = MEAN_SUBTRACTION_REGISTER1_OFS_528;
		reg2_528.reg = NUE2_GETDATA(ofs, base_addr);
		sub_parm.sub_coef0 = reg2_528.bit.NUE2_SUB_COEF_0;
		sub_parm.sub_coef1 = reg2_528.bit.NUE2_SUB_COEF_1;
		sub_parm.sub_coef2 = reg2_528.bit.NUE2_SUB_COEF_2;
		sub_parm.sub_dup = (NUE2_SUBDUP_RATE)reg2_528.bit.NUE2_SUB_DUP;
		sub_parm.sub_shift = 0;
	}

    return sub_parm;
}

/**
    NUE2 Set Padding parameter

    Set NUE2 Padding parameter.

    @param[in] Padding parameter.

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_set_pad_parm(NUE2_PAD_PARM pad_parm)
{
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    ER erReturn = E_OK;

    UINT32 pad_crop_x = pad_parm.pad_crop_x;
    UINT32 pad_crop_y = pad_parm.pad_crop_y;
    UINT32 pad_crop_width = pad_parm.pad_crop_width;
    UINT32 pad_crop_height = pad_parm.pad_crop_height;
    UINT32 pad_crop_out_x = pad_parm.pad_crop_out_x;
    UINT32 pad_crop_out_y = pad_parm.pad_crop_out_y;
    UINT32 pad_crop_out_width = pad_parm.pad_crop_out_width;
    UINT32 pad_crop_out_height = pad_parm.pad_crop_out_height;
    UINT32 pad_val0 = pad_parm.pad_val0;
    UINT32 pad_val1 = pad_parm.pad_val1;
    UINT32 pad_val2 = pad_parm.pad_val2;
    NUE2_FUNC_EN func_en = nue2_get_func_en();
    NUE2_SCALE_PARM scale_parm = nue2_get_scale_parm();
    T_PADDING_REGISTER0 reg0;
	T_PADDING_REGISTER0_528 reg0_528;
    T_PADDING_REGISTER1 reg1;
	T_PADDING_REGISTER1_528 reg1_528;
    T_PADDING_REGISTER2 reg2;
    T_PADDING_REGISTER3 reg3;
    T_PADDING_REGISTER4 reg4;

    if (func_en.pad_en) {
        if (pad_crop_x > scale_parm.h_scl_size) {
            //DBG_ERR("Error! The pad_crop_x must be <= the width size after scaling (%d) : %d -> %d\r\n", scale_parm.h_scl_size, pad_crop_x, scale_parm.h_scl_size);
            nvt_dbg(ERR, "Error! The pad_crop_x must be <= the width size after scaling (%d) : %d -> %d\r\n", scale_parm.h_scl_size, pad_crop_x, scale_parm.h_scl_size);
            return E_PAR;
        }
        if (pad_crop_y > scale_parm.v_scl_size) {
            //DBG_ERR("Error! The pad_crop_y must be <= the height size after scaling (%d) : %d -> %d\r\n", scale_parm.v_scl_size, pad_crop_y, scale_parm.v_scl_size);
            nvt_dbg(ERR, "Error! The pad_crop_y must be <= the height size after scaling (%d) : %d -> %d\r\n", scale_parm.v_scl_size, pad_crop_y, scale_parm.v_scl_size);
            return E_PAR;
        }
        if ((pad_crop_x+pad_crop_width) > scale_parm.h_scl_size) {
            //DBG_ERR("Error! The (pad_crop_x + pad_crop_width) must be <= the width size after scaling (%d) : pad_crop_x = %d, pad_crop_width = %d\r\n", scale_parm.h_scl_size, pad_crop_x, pad_crop_width);
            nvt_dbg(ERR, "Error! The (pad_crop_x + pad_crop_width) must be <= the width size after scaling (%d) : pad_crop_x = %d, pad_crop_width = %d\r\n", scale_parm.h_scl_size, pad_crop_x, pad_crop_width);
            return E_PAR;
        }
        if ((pad_crop_y+pad_crop_height) > scale_parm.v_scl_size) {
            //DBG_ERR("Error! The (pad_crop_y + pad_crop_height) must be <= the height size after scaling (%d) : pad_crop_y = %d, pad_crop_height = %d\r\n", scale_parm.v_scl_size, pad_crop_y, pad_crop_height);
            nvt_dbg(ERR, "Error! The (pad_crop_y + pad_crop_height) must be <= the height size after scaling (%d) : pad_crop_y = %d, pad_crop_height = %d\r\n", scale_parm.v_scl_size, pad_crop_y, pad_crop_height);
	#if (NUE2_AI_FLOW == ENABLE)
			return E_PAR;
	#endif
        }
        if (pad_crop_out_width > FOR_USER_98520_NUE2_OUT_WIDTH_MAX) {
            //DBG_ERR("Error! pad_crop_out_width must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_OUT_WIDTH_MAX, pad_crop_out_width, FOR_USER_98520_NUE2_OUT_WIDTH_MAX);
            nvt_dbg(ERR, "Error! pad_crop_out_width must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_OUT_WIDTH_MAX, pad_crop_out_width, FOR_USER_98520_NUE2_OUT_WIDTH_MAX);
            return E_PAR;
        }
        if (pad_crop_out_height > FOR_USER_98520_NUE2_OUT_HEIGHT_MAX) {
            //DBG_ERR("Error! pad_crop_out_height must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_OUT_HEIGHT_MAX, pad_crop_out_height, FOR_USER_98520_NUE2_OUT_HEIGHT_MAX);
            nvt_dbg(ERR, "Error! pad_crop_out_height must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_OUT_HEIGHT_MAX, pad_crop_out_height, FOR_USER_98520_NUE2_OUT_HEIGHT_MAX);
            return E_PAR;
        }
        if ((pad_crop_out_x+pad_crop_width) > pad_crop_out_width) {
            //DBG_ERR("Error! The (pad_crop_out_x + pad_crop_width) must be <= pad_crop_out_width (%d) : pad_crop_out_x = %d, pad_crop_width = %d\r\n", pad_crop_out_width, pad_crop_out_x, pad_crop_width);
            nvt_dbg(ERR, "Error! The (pad_crop_out_x + pad_crop_width) must be <= pad_crop_out_width (%d) : pad_crop_out_x = %d, pad_crop_width = %d\r\n", pad_crop_out_width, pad_crop_out_x, pad_crop_width);
	#if (NUE2_AI_FLOW == ENABLE)
			return E_PAR;
	#endif
        }
        if ((pad_crop_out_y+pad_crop_height) > pad_crop_out_height) {
            //DBG_ERR("Error! The (pad_crop_out_y + pad_crop_height) must be <= pad_crop_out_height (%d) : pad_crop_out_y = %d, pad_crop_height = %d\r\n", pad_crop_out_height, pad_crop_out_y, pad_crop_height);
            nvt_dbg(ERR, "Error! The (pad_crop_out_y + pad_crop_height) must be <= pad_crop_out_height (%d) : pad_crop_out_y = %d, pad_crop_height = %d\r\n", pad_crop_out_height, pad_crop_out_y, pad_crop_height);
	#if (NUE2_AI_FLOW == ENABLE)
			return E_PAR;
	#endif
        }
        if (pad_val0 > FOR_USER_98520_NUE2_PAD_VAL_MAX) {
            //DBG_ERR("Error! pad_val0 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_PAD_VAL_MAX, pad_val0, FOR_USER_98520_NUE2_PAD_VAL_MAX);
            nvt_dbg(ERR, "Error! pad_val0 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_PAD_VAL_MAX, pad_val0, FOR_USER_98520_NUE2_PAD_VAL_MAX);
            return E_PAR;
        }
        if (pad_val1 > FOR_USER_98520_NUE2_PAD_VAL_MAX) {
            //DBG_ERR("Error! pad_val1 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_PAD_VAL_MAX, pad_val1, FOR_USER_98520_NUE2_PAD_VAL_MAX);
            nvt_dbg(ERR, "Error! pad_val1 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_PAD_VAL_MAX, pad_val1, FOR_USER_98520_NUE2_PAD_VAL_MAX);
            return E_PAR;
        }
        if (pad_val2 > FOR_USER_98520_NUE2_PAD_VAL_MAX) {
            //DBG_ERR("Error! pad_val2 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_PAD_VAL_MAX, pad_val2, FOR_USER_98520_NUE2_PAD_VAL_MAX);
            nvt_dbg(ERR, "Error! pad_val2 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_PAD_VAL_MAX, pad_val2, FOR_USER_98520_NUE2_PAD_VAL_MAX);
            return E_PAR;
        }
    }

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = PADDING_REGISTER0_OFS;
		reg0.reg = NUE2_GETDATA(ofs, base_addr);
		reg0.bit.NUE2_PAD_CROP_X = pad_crop_x;
		reg0.bit.NUE2_PAD_CROP_Y = pad_crop_y;
		NUE2_SETDATA(ofs, reg0.reg, base_addr);
	} else {
		ofs = PADDING_REGISTER0_OFS_528;
        reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
        reg0_528.bit.NUE2_PAD_CROP_X = pad_crop_x;
        reg0_528.bit.NUE2_PAD_CROP_Y = pad_crop_y;
        NUE2_SETDATA(ofs, reg0_528.reg, base_addr);
	}

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = PADDING_REGISTER1_OFS;
		reg1.reg = NUE2_GETDATA(ofs, base_addr);
		reg1.bit.NUE2_PAD_CROP_WIDTH = pad_crop_width;
		reg1.bit.NUE2_PAD_CROP_HEIGHT = pad_crop_height;
		NUE2_SETDATA(ofs, reg1.reg, base_addr);
	} else {
		ofs = PADDING_REGISTER1_OFS_528;
		reg1_528.reg = NUE2_GETDATA(ofs, base_addr);
		reg1_528.bit.NUE2_PAD_CROP_WIDTH = pad_crop_width;
		reg1_528.bit.NUE2_PAD_CROP_HEIGHT = pad_crop_height;
		NUE2_SETDATA(ofs, reg1_528.reg, base_addr);
	}

	ofs = PADDING_REGISTER2_OFS;
	reg2.reg = NUE2_GETDATA(ofs, base_addr);
	reg2.bit.NUE2_PAD_OUT_X = pad_crop_out_x;
    reg2.bit.NUE2_PAD_OUT_Y = pad_crop_out_y;
	NUE2_SETDATA(ofs, reg2.reg, base_addr);

	ofs = PADDING_REGISTER3_OFS;
	reg3.reg = NUE2_GETDATA(ofs, base_addr);
	reg3.bit.NUE2_PAD_OUT_WIDTH = pad_crop_out_width;
    reg3.bit.NUE2_PAD_OUT_HEIGHT = pad_crop_out_height;
	NUE2_SETDATA(ofs, reg3.reg, base_addr);

	ofs = PADDING_REGISTER4_OFS;
	reg4.reg = NUE2_GETDATA(ofs, base_addr);
	reg4.bit.NUE2_PAD_VAL_0 = pad_val0;
    reg4.bit.NUE2_PAD_VAL_1 = pad_val1;
    reg4.bit.NUE2_PAD_VAL_2 = pad_val2;
	NUE2_SETDATA(ofs, reg4.reg, base_addr);

    return erReturn;
}

/**
    NUE2 Get Padding parameter status

    Get current NUE2 Padding parameter status.

    @return Padding parameter status.
*/
NUE2_PAD_PARM nue2_get_pad_parm(VOID)
{
    NUE2_PAD_PARM pad_parm;
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    T_PADDING_REGISTER0 reg0;
	T_PADDING_REGISTER0_528 reg0_528;
    T_PADDING_REGISTER1 reg1;
	T_PADDING_REGISTER1_528 reg1_528;
    T_PADDING_REGISTER2 reg2;
    T_PADDING_REGISTER3 reg3;
    T_PADDING_REGISTER4 reg4;

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = PADDING_REGISTER0_OFS;
		reg0.reg = NUE2_GETDATA(ofs, base_addr);
		pad_parm.pad_crop_x = reg0.bit.NUE2_PAD_CROP_X;
		pad_parm.pad_crop_y = reg0.bit.NUE2_PAD_CROP_Y;
	} else {
		ofs = PADDING_REGISTER0_OFS_528;
		reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
		pad_parm.pad_crop_x = reg0_528.bit.NUE2_PAD_CROP_X;
		pad_parm.pad_crop_y = reg0_528.bit.NUE2_PAD_CROP_Y;
	}

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = PADDING_REGISTER1_OFS;
		reg1.reg = NUE2_GETDATA(ofs, base_addr);
		pad_parm.pad_crop_width = reg1.bit.NUE2_PAD_CROP_WIDTH;
		pad_parm.pad_crop_height = reg1.bit.NUE2_PAD_CROP_HEIGHT;
	} else {
		ofs = PADDING_REGISTER1_OFS_528;
		reg1_528.reg = NUE2_GETDATA(ofs, base_addr);
		pad_parm.pad_crop_width = reg1_528.bit.NUE2_PAD_CROP_WIDTH;
		pad_parm.pad_crop_height = reg1_528.bit.NUE2_PAD_CROP_HEIGHT;
	}

	ofs = PADDING_REGISTER2_OFS;
	reg2.reg = NUE2_GETDATA(ofs, base_addr);
	pad_parm.pad_crop_out_x = reg2.bit.NUE2_PAD_OUT_X;
    pad_parm.pad_crop_out_y = reg2.bit.NUE2_PAD_OUT_Y;

	ofs = PADDING_REGISTER3_OFS;
	reg3.reg = NUE2_GETDATA(ofs, base_addr);
	pad_parm.pad_crop_out_width = reg3.bit.NUE2_PAD_OUT_WIDTH;
    pad_parm.pad_crop_out_height = reg3.bit.NUE2_PAD_OUT_HEIGHT;

	ofs = PADDING_REGISTER4_OFS;
	reg4.reg = NUE2_GETDATA(ofs, base_addr);
	pad_parm.pad_val0 = reg4.bit.NUE2_PAD_VAL_0;
    pad_parm.pad_val1 = reg4.bit.NUE2_PAD_VAL_1;
    pad_parm.pad_val2 = reg4.bit.NUE2_PAD_VAL_2;

    return pad_parm;
}

/**
    NUE2 Set HSV parameter

    Set NUE2 HSV parameter.

    @param[in] HSV parameter.

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_set_hsv_parm(NUE2_HSV_PARM hsv_parm)
{
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    ER erReturn = E_OK;
    UINT32 hsv_out_mode = (UINT32)hsv_parm.hsv_out_mode;
    UINT32 hsv_hue_shift = hsv_parm.hsv_hue_shift;
    T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
    T_HSV_REGISTER0 reg1;

	ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_HSV_OUT_MODE = hsv_out_mode;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);

	ofs = HSV_REGISTER0_OFS;
	reg1.reg = NUE2_GETDATA(ofs, base_addr);
	reg1.bit.NUE2_HUE_SFT = hsv_hue_shift;
	NUE2_SETDATA(ofs, reg1.reg, base_addr);

    return erReturn;
}

/**
    NUE2 Get HSV parameter status

    Get current NUE2 HSV parameter status.

    @return HSV parameter status.
*/
NUE2_HSV_PARM nue2_get_hsv_parm(VOID)
{
    NUE2_HSV_PARM hsv_parm;
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
    T_HSV_REGISTER0 reg1;

	ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	hsv_parm.hsv_out_mode = (BOOL)reg0.bit.NUE2_HSV_OUT_MODE;

	ofs = HSV_REGISTER0_OFS;
	reg1.reg = NUE2_GETDATA(ofs, base_addr);
	hsv_parm.hsv_hue_shift = reg1.bit.NUE2_HUE_SFT;

    return hsv_parm;
}

/**
    NUE2 Set Rotate parameter

    Set NUE2 Rotate parameter.

    @param[in] Rotate parameter.

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_set_rotate_parm(NUE2_ROTATE_PARM rotate_parm)
{
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    ER erReturn = E_OK;
	NUE2_IN_FMT in_fmt = nue2_get_infmt();
	NUE2_FUNC_EN func_en = nue2_get_func_en();
    UINT32 rotate_mode = (UINT32)rotate_parm.rotate_mode;
    T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;

	if (in_fmt==NUE2_RGB_PLANNER) {
		if (func_en.rotate_en) {
			nvt_dbg(ERR, "Error! RGB can't be supported the rotate mode. (to use Y/UV instead of it.)\r\n");
			return E_PAR;
		}
	}	

	ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_ROTATE_MODE = rotate_mode;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);

    return erReturn;
}

/**
    NUE2 Get Rotate parameter status

    Get current NUE2 Rotate parameter status.

    @return Rotate parameter status.
*/
NUE2_ROTATE_PARM nue2_get_rotate_parm(VOID)
{
    NUE2_ROTATE_PARM rotate_parm;
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;

    T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	rotate_parm.rotate_mode = (NUE2_ROT_DEG)reg0.bit.NUE2_ROTATE_MODE;

    return rotate_parm;
}

/**
    NUE2 Set Line-Offset parameter

    Set NUE2 Line-Offset parameter.

    @param[in] Line-Offset parameter.

    @return
        -@b E_OK: setting success
        - Others: Error occured.
*/
ER nue2_set_dmaio_lofs(NUE2_DMAIO_LOFS dmaio_lofs)
{
    UINT32 ofs;
    ER erReturn = E_OK;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;

    UINT32 in0_lofs = dmaio_lofs.in0_lofs;
    UINT32 in1_lofs = dmaio_lofs.in1_lofs;
    UINT32 in2_lofs = dmaio_lofs.in2_lofs;
    UINT32 out0_lofs = dmaio_lofs.out0_lofs;
    UINT32 out1_lofs = dmaio_lofs.out1_lofs;
    UINT32 out2_lofs = dmaio_lofs.out2_lofs;

    NUE2_IN_FMT in_fmt = nue2_get_infmt();
    NUE2_IN_SIZE in_size = nue2_get_insize();
    UINT32 in_width = in_size.in_width;
    NUE2_PAD_PARM pad_parm = nue2_get_pad_parm();
    UINT32 pad_crop_out_width = pad_parm.pad_crop_out_width;
	NUE2_SUB_PARM sub_parm = nue2_get_sub_parm();
	NUE2_FUNC_EN func_en = nue2_get_func_en();
	NUE2_FLIP_PARM flip_parm = nue2_get_flip_parm();

    T_INPUT_LINE_OFFSET_REGISTER0 reg0;
    T_INPUT_LINE_OFFSET_REGISTER1 reg1;
    T_INPUT_LINE_OFFSET_REGISTER2 reg2;
    T_OUTPUT_LINE_OFFSET_REGISTER0 reg3;
    T_OUTPUT_LINE_OFFSET_REGISTER1 reg4;
    T_OUTPUT_LINE_OFFSET_REGISTER2 reg5;

    // check maximum size
    if (in0_lofs > FOR_USER_98520_NUE2_LINE_OFFSET_MAX) {
        //DBG_ERR("Error! Input lineoffset-0 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, in0_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        nvt_dbg(ERR, "Error! Input lineoffset-0 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, in0_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        return E_PAR;
    }
    if (in1_lofs > FOR_USER_98520_NUE2_LINE_OFFSET_MAX) {
        //DBG_ERR("Error! Input lineoffset-1 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, in1_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        nvt_dbg(ERR, "Error! Input lineoffset-1 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, in1_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        return E_PAR;
    }
    if (in2_lofs > FOR_USER_98520_NUE2_LINE_OFFSET_MAX) {
        //DBG_ERR("Error! Input lineoffset-2 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, in2_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        nvt_dbg(ERR, "Error! Input lineoffset-2 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, in2_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        return E_PAR;
    }
    if (out0_lofs > FOR_USER_98520_NUE2_LINE_OFFSET_MAX) {
        //DBG_ERR("Error! Output lineoffset-0 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, out0_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        nvt_dbg(ERR, "Error! Output lineoffset-0 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, out0_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        return E_PAR;
    }
    if (out1_lofs > FOR_USER_98520_NUE2_LINE_OFFSET_MAX) {
        //DBG_ERR("Error! Output lineoffset-1 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, out1_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        nvt_dbg(ERR, "Error! Output lineoffset-1 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, out1_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        return E_PAR;
    }
    if (out2_lofs > FOR_USER_98520_NUE2_LINE_OFFSET_MAX) {
        //DBG_ERR("Error! Output lineoffset-2 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, out2_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        nvt_dbg(ERR, "Error! Output lineoffset-2 must be <= %d : %d -> %d\r\n", FOR_USER_98520_NUE2_LINE_OFFSET_MAX, out2_lofs, FOR_USER_98520_NUE2_LINE_OFFSET_MAX);
        return E_PAR;
    }
    // check format size
    if ((in_fmt==NUE2_UV_CHANNEL)&&(out0_lofs%2!=0)) {
        //DBG_ERR("Error! Format in UV_CHANNEL, input lineoffset-0 must be 2-byte aligned : %d\r\n", out0_lofs);
        nvt_dbg(ERR, "Error! Format in UV_CHANNEL, input lineoffset-0 must be 2-byte aligned : %d\r\n", out0_lofs);
        return E_PAR;
    }
    if ((in_fmt==NUE2_UV_CHANNEL)&&(out1_lofs%2!=0)) {
        //DBG_ERR("Error! Format in UV_CHANNEL, input lineoffset-1 must be 2-byte aligned : %d\r\n", out1_lofs);
        nvt_dbg(ERR, "Error! Format in UV_CHANNEL, input lineoffset-1 must be 2-byte aligned : %d\r\n", out1_lofs);
        return E_PAR;
    }
    if ((in_fmt==NUE2_UV_CHANNEL)&&(out2_lofs%2!=0)) {
        //DBG_ERR("Error! Format in UV_CHANNEL, input lineoffset-2 must be 2-byte aligned : %d\r\n", out2_lofs);
        nvt_dbg(ERR, "Error! Format in UV_CHANNEL, input lineoffset-2 must be 2-byte aligned : %d\r\n", out2_lofs);
        return E_PAR;
    }
    // check the size with width
    if (in_fmt==NUE2_UV_CHANNEL) {
        if ((in0_lofs>0)&&((in0_lofs/2)<in_width)) {
            //DBG_ERR("Error! Format in UV_CHANNEL, Input lineoffset-0 must be >= in_width*2 (%d) : %d -> %d\r\n", in_width, in0_lofs, in_width);
            nvt_dbg(ERR, "Error! Format in UV_CHANNEL, Input lineoffset-0 must be >= in_width*2 (%d) : %d -> %d\r\n", in_width, in0_lofs, in_width);
            return E_PAR;
        }
        if (func_en.sub_en == 1 && sub_parm.sub_mode == 1) {
			if ((in2_lofs>0) && (in2_lofs < sub_parm.sub_in_width * 2)) {
				//DBG_ERR("Error! Format in UV_CHANNEL, Input lineoffset-2 must be >= in_width*2 (%d) : %d -> %d\r\n", in_width, in2_lofs, in_width);
				nvt_dbg(ERR, "Error! Format in UV_CHANNEL, Input lineoffset-2 must be >= sub_in_width (%d)*2 : %d -> %d\r\n", sub_parm.sub_in_width, in2_lofs, sub_parm.sub_in_width);
				return E_PAR;
			}
		}
	} else if (in_fmt == NUE2_Y_CHANNEL) {
		if ((in0_lofs>0) && ((in0_lofs)<in_width)) {
			nvt_dbg(ERR, "Error! Format in Y_CHANNEL, Input lineoffset-0 must be >= in_width (%d) : %d -> %d\r\n", in_width, in0_lofs, in_width);
			return E_PAR;
		}

		if (func_en.sub_en == 1 && sub_parm.sub_mode == 1) {
			if ((in2_lofs>0) && (in2_lofs < sub_parm.sub_in_width)) {
				nvt_dbg(ERR, "Error! Format in Y_CHANNEL, Input lineoffset-2 must be >= sub_in_width (%d) : %d -> %d\r\n", sub_parm.sub_in_width, in2_lofs, sub_parm.sub_in_width);
				return E_PAR;
			}
		}

	} else if(in_fmt == NUE2_YUV_420){
		if ((in0_lofs>0) && (in0_lofs<in_width)) {
			nvt_dbg(ERR, "Error! Format in YUV_420 ,Input lineoffset-0 must be >= in_width (%d) : %d -> %d\r\n", in_width, in0_lofs, in_width);
			return E_PAR;
		}
		if ((in1_lofs>0) && (in1_lofs<in_width)) {
			nvt_dbg(ERR, "Error! Format in YUV_420 ,Input lineoffset-1 must be >= in_width (%d) : %d -> %d\r\n", in_width, in1_lofs, in_width);
			return E_PAR;
		}

		if (func_en.sub_en == 1 && sub_parm.sub_mode == 1) {
			if ((in2_lofs>0) && (in2_lofs < sub_parm.sub_in_width*3)) {
				nvt_dbg(ERR, "Error! Format in YUV_420 ,Input lineoffset-2 must be >= sub_in_width (%d) x 3 : %d -> %d\r\n", sub_parm.sub_in_width, in2_lofs, sub_parm.sub_in_width);
				//return E_PAR;
			}
		}		
    } else {
        if ((in0_lofs>0)&&(in0_lofs<in_width)) {
            //DBG_ERR("Error! Input lineoffset-0 must be >= in_width (%d) : %d -> %d\r\n", in_width, in0_lofs, in_width);
            nvt_dbg(ERR, "Error! Input lineoffset-0 must be >= in_width (%d) : %d -> %d\r\n", in_width, in0_lofs, in_width);
            return E_PAR;
        }
        if ((in1_lofs>0)&&(in1_lofs<in_width)) {
            //DBG_ERR("Error! Input lineoffset-1 must be >= in_width (%d) : %d -> %d\r\n", in_width, in1_lofs, in_width);
            nvt_dbg(ERR, "Error! Input lineoffset-1 must be >= in_width (%d) : %d -> %d\r\n", in_width, in1_lofs, in_width);
            return E_PAR;
        }
        if ((in2_lofs>0)&&(in2_lofs<in_width)) {
            //DBG_ERR("Error! Input lineoffset-2 must be >= in_width (%d) : %d -> %d\r\n", in_width, in2_lofs, in_width);
            nvt_dbg(ERR, "Error! Input lineoffset-2 must be >= in_width (%d) : %d -> %d\r\n", in_width, in2_lofs, in_width);
            return E_PAR;
        }
    }

    if (in_fmt==NUE2_UV_CHANNEL) {
        if ((out0_lofs>0)&&((out0_lofs/2)<pad_crop_out_width)) {
            //DBG_ERR("Error! Format in UV_CHANNEL, Output lineoffset-0 must be >= pad_crop_out_width*2 (%d) : %d -> %d\r\n", pad_crop_out_width, out0_lofs, pad_crop_out_width);
            nvt_dbg(ERR, "Error! Format in UV_CHANNEL, Output lineoffset-0 must be >= pad_crop_out_width*2 (%d) : %d -> %d\r\n", pad_crop_out_width, out0_lofs, pad_crop_out_width);
            return E_PAR;
        }
        if ((out1_lofs>0)&&((out1_lofs/2)<pad_crop_out_width)) {
            //DBG_ERR("Error! Format in UV_CHANNEL, Output lineoffset-1 must be >= pad_crop_out_width*2 (%d) : %d -> %d\r\n", pad_crop_out_width, out1_lofs, pad_crop_out_width);
            nvt_dbg(ERR, "Error! Format in UV_CHANNEL, Output lineoffset-1 must be >= pad_crop_out_width*2 (%d) : %d -> %d\r\n", pad_crop_out_width, out1_lofs, pad_crop_out_width);
            return E_PAR;
        }
        if ((out2_lofs>0)&&((out2_lofs/2)<pad_crop_out_width)) {
            //DBG_ERR("Error! Format in UV_CHANNEL, Output lineoffset-2 must be >= pad_crop_out_width*2 (%d) : %d -> %d\r\n", pad_crop_out_width, out2_lofs, pad_crop_out_width);
            nvt_dbg(ERR, "Error! Format in UV_CHANNEL, Output lineoffset-2 must be >= pad_crop_out_width*2 (%d) : %d -> %d\r\n", pad_crop_out_width, out2_lofs, pad_crop_out_width);
            return E_PAR;
        }
    } else {
        if ((out0_lofs>0)&&(out0_lofs<pad_crop_out_width)) {
            //DBG_ERR("Error! Output lineoffset-0 must be >= pad_crop_out_width (%d) : %d -> %d\r\n", pad_crop_out_width, out0_lofs, pad_crop_out_width);
            nvt_dbg(ERR, "Error! Output lineoffset-0 must be >= pad_crop_out_width (%d) : %d -> %d\r\n", pad_crop_out_width, out0_lofs, pad_crop_out_width);
            return E_PAR;
        }
        if ((out1_lofs>0)&&(out1_lofs<pad_crop_out_width)) {
            //DBG_ERR("Error! Output lineoffset-1 must be >= pad_crop_out_width (%d) : %d -> %d\r\n", pad_crop_out_width, out1_lofs, pad_crop_out_width);
            nvt_dbg(ERR, "Error! Output lineoffset-1 must be >= pad_crop_out_width (%d) : %d -> %d\r\n", pad_crop_out_width, out1_lofs, pad_crop_out_width);
            return E_PAR;
        }
        if ((out2_lofs>0)&&(out2_lofs<pad_crop_out_width)) {
            //DBG_ERR("Error! Output lineoffset-2 must be >= pad_crop_out_width (%d) : %d -> %d\r\n", pad_crop_out_width, out2_lofs, pad_crop_out_width);
            nvt_dbg(ERR, "Error! Output lineoffset-2 must be >= pad_crop_out_width (%d) : %d -> %d\r\n", pad_crop_out_width, out2_lofs, pad_crop_out_width);
            return E_PAR;
        }
    }

	if (in_fmt==NUE2_UV_CHANNEL) {
	  	if ((in0_lofs % 2) != 0) {
		  	nvt_dbg(ERR, "Error! UV mode(0x2) can't be supported byte-align, it must be 2-bytes align. input lineoffset-0 = 0x%x\r\n", in0_lofs);
		  	return E_PAR;
	  	}
	  	//SUB MODE
	  	if(sub_parm.sub_mode == 1){ //planner
		  	if ((in2_lofs % 2) != 0) {
			  	nvt_dbg(ERR, "Error! UV mode(0x2) can't be supported byte-alig (SUB_MODE=1), \
							   it must be 2-bytes align. input lineoffset-2 = 0x%x\r\n", in2_lofs);
			  	return E_PAR;
		  	}
	  	}
	}

	if(nvt_get_chip_id() == CHIP_NA51055) {
		//do not thing
	} else {
		if (in_fmt==NUE2_YUV_420) {
			 if ((in1_lofs % 2) != 0) {
				nvt_dbg(ERR, "Error! yuv420-channel2 can't be supported byte-align, it must be 2-bytes align. input lineoffset-1 = 0x%x\r\n", in0_lofs);
				return E_PAR;
			 }
		}
		if (func_en.hsv_en) {
			if (out0_lofs == 0) {
				nvt_dbg(ERR, "Error! HSV can't be supported the output lineoffset-0 = 0.\r\n");
				return E_PAR;
			}
		}
		if (flip_parm.flip_mode && (func_en.hsv_en == 0) && (func_en.rotate_en == 0)) {
			if (in_fmt==NUE2_YUV_420) {
				if (out0_lofs == 0) {
					nvt_dbg(ERR, "Error! Flip can't be supported the output lineoffset-0 = 0.\r\n");
					return E_PAR;
				}
				if (out1_lofs == 0) {
					nvt_dbg(ERR, "Error! Flip can't be supported the output lineoffset-1 = 0.\r\n");
					return E_PAR;
				}
				if (out2_lofs == 0) {
					nvt_dbg(ERR, "Error! Flip can't be supported the output lineoffset-2 = 0.\r\n");
					return E_PAR;
				}
			} else if (in_fmt==NUE2_Y_CHANNEL) {
				if (out0_lofs == 0) {
					nvt_dbg(ERR, "Error! Flip can't be supported the output lineoffset-0 = 0.\r\n");
					return E_PAR;
				}
			} else if (in_fmt==NUE2_UV_CHANNEL) {
				if (out0_lofs == 0) {
					nvt_dbg(ERR, "Error! Flip can't be supported the output lineoffset-0 = 0.\r\n");
					return E_PAR;
				}
			} else if (in_fmt==NUE2_RGB_PLANNER) {
				if (out0_lofs == 0) {
					nvt_dbg(ERR, "Error! Flip can't be supported the output lineoffset-0 = 0.\r\n");
					return E_PAR;
				}
				if (out1_lofs == 0) {
					nvt_dbg(ERR, "Error! Flip can't be supported the output lineoffset-1 = 0.\r\n");
					return E_PAR;
				}
				if (out2_lofs == 0) {
					nvt_dbg(ERR, "Error! Flip can't be supported the output lineoffset-2 = 0.\r\n");
					return E_PAR;
				}
			}
		}
	}

    // Input line offset
	ofs = INPUT_LINE_OFFSET_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
    reg0.bit.DRAM_OFSI0 = in0_lofs;
    NUE2_SETDATA(ofs, reg0.reg, base_addr);

	ofs = INPUT_LINE_OFFSET_REGISTER1_OFS;
	reg1.reg = NUE2_GETDATA(ofs, base_addr);
    reg1.bit.DRAM_OFSI1 = in1_lofs;
    NUE2_SETDATA(ofs, reg1.reg, base_addr);

	ofs = INPUT_LINE_OFFSET_REGISTER2_OFS;
	reg2.reg = NUE2_GETDATA(ofs, base_addr);
    reg2.bit.DRAM_OFSI2 = in2_lofs;
    NUE2_SETDATA(ofs, reg2.reg, base_addr);

    // Output line offset
	ofs = OUTPUT_LINE_OFFSET_REGISTER0_OFS;
	reg3.reg = NUE2_GETDATA(ofs, base_addr);
    reg3.bit.DRAM_OFSO0 = out0_lofs;
    NUE2_SETDATA(ofs, reg3.reg, base_addr);

	ofs = OUTPUT_LINE_OFFSET_REGISTER1_OFS;
	reg4.reg = NUE2_GETDATA(ofs, base_addr);
    reg4.bit.DRAM_OFSO1 = out1_lofs;
    NUE2_SETDATA(ofs, reg4.reg, base_addr);

	ofs = OUTPUT_LINE_OFFSET_REGISTER2_OFS;
	reg5.reg = NUE2_GETDATA(ofs, base_addr);
    reg5.bit.DRAM_OFSO2 = out2_lofs;
    NUE2_SETDATA(ofs, reg5.reg, base_addr);

    return erReturn;
}

/**
    NUE2 Get Line-Offset parameter status

    Get current NUE2 Line-Offset parameter status.

    @return Line-Offset parameter status.
*/
NUE2_DMAIO_LOFS nue2_get_dmaio_lofs(VOID)
{
    NUE2_DMAIO_LOFS dmaio_lofs;
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;

    T_INPUT_LINE_OFFSET_REGISTER0 reg0;
    T_INPUT_LINE_OFFSET_REGISTER1 reg1;
    T_INPUT_LINE_OFFSET_REGISTER2 reg2;
    T_OUTPUT_LINE_OFFSET_REGISTER0 reg3;
    T_OUTPUT_LINE_OFFSET_REGISTER1 reg4;
    T_OUTPUT_LINE_OFFSET_REGISTER2 reg5;

    // Input line offset
	ofs = INPUT_LINE_OFFSET_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
    dmaio_lofs.in0_lofs = reg0.bit.DRAM_OFSI0;

	ofs = INPUT_LINE_OFFSET_REGISTER1_OFS;
	reg1.reg = NUE2_GETDATA(ofs, base_addr);
    dmaio_lofs.in1_lofs = reg1.bit.DRAM_OFSI1;

	ofs = INPUT_LINE_OFFSET_REGISTER2_OFS;
	reg2.reg = NUE2_GETDATA(ofs, base_addr);
    dmaio_lofs.in2_lofs = reg2.bit.DRAM_OFSI2;

    // Output line offset
	ofs = OUTPUT_LINE_OFFSET_REGISTER0_OFS;
	reg3.reg = NUE2_GETDATA(ofs, base_addr);
    dmaio_lofs.out0_lofs = reg3.bit.DRAM_OFSO0;

	ofs = OUTPUT_LINE_OFFSET_REGISTER1_OFS;
	reg4.reg = NUE2_GETDATA(ofs, base_addr);
    dmaio_lofs.out1_lofs = reg4.bit.DRAM_OFSO1;

	ofs = OUTPUT_LINE_OFFSET_REGISTER2_OFS;
	reg5.reg = NUE2_GETDATA(ofs, base_addr);
    dmaio_lofs.out2_lofs = reg5.bit.DRAM_OFSO2;

    return dmaio_lofs;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
    NUE2 Set DMA input address

    Set NUE2 DMA input address.

    @param[in] DMA input address.

    @return none.
*/
VOID nue2_set_dmain_addr(UINT32 addr0, UINT32 addr1, UINT32 addr2)
{
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;

    T_DMA_TO_NUE2_REGISTER0 reg0;
    T_DMA_TO_NUE2_REGISTER1 reg1;
    T_DMA_TO_NUE2_REGISTER2 reg2;

	ofs = DMA_TO_NUE2_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
    reg0.bit.DRAM_SAI0 = addr0;
    NUE2_SETDATA(ofs, reg0.reg, base_addr);

	ofs = DMA_TO_NUE2_REGISTER1_OFS;
	reg1.reg = NUE2_GETDATA(ofs, base_addr);
    reg1.bit.DRAM_SAI1 = addr1;
    NUE2_SETDATA(ofs, reg1.reg, base_addr);

	ofs = DMA_TO_NUE2_REGISTER2_OFS;
	reg2.reg = NUE2_GETDATA(ofs, base_addr);
    reg2.bit.DRAM_SAI2 = addr2;
    NUE2_SETDATA(ofs, reg2.reg, base_addr);
}

/**
    NUE2 Get DMA input address status

    Get current NUE2 DMA input address status.

    @return DMA input address status.
*/
UINT32 nue2_get_dmain_addr(NUE2_IN_BUFID bufid)
{
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;

    T_DMA_TO_NUE2_REGISTER0 reg0;
    T_DMA_TO_NUE2_REGISTER1 reg1;
    T_DMA_TO_NUE2_REGISTER2 reg2;
	UINT32 ofs0 = DMA_TO_NUE2_REGISTER0_OFS;
    UINT32 ofs1 = DMA_TO_NUE2_REGISTER1_OFS;
    UINT32 ofs2 = DMA_TO_NUE2_REGISTER2_OFS;
    UINT32 addr = 0, phy_addr = 0;

    switch (bufid) {
	case NUE2_IN0_BUF:
    	reg0.reg = NUE2_GETDATA(ofs0, base_addr);
        phy_addr = reg0.bit.DRAM_SAI0;
        break;
    case NUE2_IN1_BUF:
    	reg1.reg = NUE2_GETDATA(ofs1, base_addr);
        phy_addr = reg1.bit.DRAM_SAI1;
        break;
    case NUE2_IN2_BUF:
    	reg2.reg = NUE2_GETDATA(ofs2, base_addr);
        phy_addr = reg2.bit.DRAM_SAI2;
        break;
    default:
        //DBG_ERR("Wrong input biffer ID: %d\r\n", bufid);
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
    NUE2 Set DMA output address

    Set NUE2 DMA output address.

    @param[in] DMA output address.

    @return none.
*/
VOID nue2_set_dmaout_addr(UINT32 addr0, UINT32 addr1, UINT32 addr2)
{
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;

    T_NUE2_TO_DMA_RESULT_REGISTER0 reg0;
    T_NUE2_TO_DMA_RESULT_REGISTER1 reg1;
    T_NUE2_TO_DMA_RESULT_REGISTER2 reg2;

	ofs = NUE2_TO_DMA_RESULT_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
    reg0.bit.DRAM_SAO0 = addr0;
    NUE2_SETDATA(ofs, reg0.reg, base_addr);

	ofs = NUE2_TO_DMA_RESULT_REGISTER1_OFS;
	reg1.reg = NUE2_GETDATA(ofs, base_addr);
    reg1.bit.DRAM_SAO1 = addr1;
    NUE2_SETDATA(ofs, reg1.reg, base_addr);

	ofs = NUE2_TO_DMA_RESULT_REGISTER2_OFS;
	reg2.reg = NUE2_GETDATA(ofs, base_addr);
    reg2.bit.DRAM_SAO2 = addr2;
    NUE2_SETDATA(ofs, reg2.reg, base_addr);
}

/**
    NUE2 Get DMA output address status

    Get current NUE2 DMA output address status.

    @return DMA output address status.
*/
UINT32 nue2_get_dmaout_addr(NUE2_OUT_BUFID bufid)
{
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;

    T_NUE2_TO_DMA_RESULT_REGISTER0 reg0;
    T_NUE2_TO_DMA_RESULT_REGISTER1 reg1;
    T_NUE2_TO_DMA_RESULT_REGISTER2 reg2;
	UINT32 ofs0 = NUE2_TO_DMA_RESULT_REGISTER0_OFS;
    UINT32 ofs1 = NUE2_TO_DMA_RESULT_REGISTER1_OFS;
    UINT32 ofs2 = NUE2_TO_DMA_RESULT_REGISTER2_OFS;
    UINT32 addr = 0, phy_addr = 0;

    switch (bufid) {
	case NUE2_OUT0_BUF:
    	reg0.reg = NUE2_GETDATA(ofs0, base_addr);
        phy_addr = reg0.bit.DRAM_SAO0;
        break;
    case NUE2_OUT1_BUF:
    	reg1.reg = NUE2_GETDATA(ofs1, base_addr);
        phy_addr = reg1.bit.DRAM_SAO1;
        break;
    case NUE2_OUT2_BUF:
    	reg2.reg = NUE2_GETDATA(ofs2, base_addr);
        phy_addr = reg2.bit.DRAM_SAO2;
        break;
    default:
        //DBG_ERR("Wrong output biffer ID: %d\r\n", bufid);
        nvt_dbg(ERR, "Wrong output biffer ID: %d\r\n", bufid);
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
    NUE2 Set DMA input linked-list address

    Set NUE2 DMA input linked-list address.

    @param[in] DMA input linked-list address.

    @return none.
*/
VOID nue2_set_dmain_lladdr(UINT32 addr0)
{
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    T_DMA_TO_NUE2_REGISTER3 reg0;
	UINT32 ofs = DMA_TO_NUE2_REGISTER3_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
    reg0.bit.DRAM_SAILL = addr0;
    NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Get DMA input linked-list address status

    Get current NUE2 DMA input linked-list address status.

    @return DMA input linked-list address status.
*/
UINT32 nue2_get_dmain_lladdr(VOID)
{
    UINT32 addr0;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    T_DMA_TO_NUE2_REGISTER3 reg0;
	UINT32 ofs = DMA_TO_NUE2_REGISTER3_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
    addr0 = reg0.bit.DRAM_SAILL;

    return addr0;
}

/**
    NUE2 Set DMA IN/OUT address

    Set NUE2 DMA IN/OUT address.

    @param[in] DMA IN/OUT address.

    @return none.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER nue2_set_dmaio_addr(NUE2_DMAIO_ADDR dma_addr)
{
    NUE2_FUNC_EN func_en;
    NUE2_IN_SIZE in_size;
    NUE2_IN_FMT in_fmt;
    NUE2_DMAIO_LOFS dmaio_lofs;

    NUE2_SUB_PARM sub_parm;
#if (NUE2_AI_FLOW == ENABLE)
#else
    NUE2_PAD_PARM pad_parm;
    NUE2_SCALE_PARM scale_parm;
	NUE2_ROTATE_PARM rotate_parm;
#endif

    NUE2_DMAIO_ADDR *p_addr = &dma_addr;
    UINT32 flushsize = 0;

#if (NUE2_DMA_CACHE_HANDLE == 1)
	UINT32 addr0, addr1, addr2;
	UINT32 phy_addr0, phy_addr1, phy_addr2;
#if (NUE2_AI_FLOW == ENABLE)
#else
	UINT32 real_height, real_width;
#endif

    // Info for flushsize calculation
    func_en = nue2_get_func_en();
    in_size = nue2_get_insize();
    in_fmt = nue2_get_infmt();
    dmaio_lofs = nue2_get_dmaio_lofs();
	
    sub_parm = nue2_get_sub_parm();
#if (NUE2_AI_FLOW == ENABLE)
#else
    pad_parm = nue2_get_pad_parm();
	scale_parm = nue2_get_scale_parm();
	rotate_parm = nue2_get_rotate_parm();
#endif
    
	// Input Dram DMA/Cache Handle
	addr0 = p_addr->addr_in0;
	addr1 = p_addr->addr_in1;
	addr2 = p_addr->addr_in2;

    //nvt_dbg(IND, "NUE2 do addr_in fmem_dcache_sync.\r\n");

    if (addr0 != 0) {
        if (dmaio_lofs.in0_lofs != 0) {
            flushsize = dmaio_lofs.in0_lofs * in_size.in_height;
        } else {
            if (in_fmt==NUE2_UV_CHANNEL) {
                flushsize = in_size.in_width * in_size.in_height * 2;
            } else {
                flushsize = in_size.in_width * in_size.in_height;
            }
        }
		if (p_addr->dma_do_not_sync == 0) {
        	nue2_pt_dma_flush_mem2dev(addr0, flushsize);
		}
    }
    if (addr1 != 0) {
        if (dmaio_lofs.in1_lofs != 0) {
            flushsize = dmaio_lofs.in1_lofs * in_size.in_height;
        } else {
            flushsize = in_size.in_width * in_size.in_height;
        }
        if (in_fmt==NUE2_YUV_420) {
            flushsize = (flushsize>>1);
        }
		if (p_addr->dma_do_not_sync == 0) {
        	nue2_pt_dma_flush_mem2dev(addr1, flushsize);
		}
    }
    if (addr2 != 0) {
        if (func_en.sub_en && sub_parm.sub_mode == 1) { //planner
            if (dmaio_lofs.in2_lofs != 0) {
                flushsize = dmaio_lofs.in2_lofs * sub_parm.sub_in_height;
            } else {
                flushsize = sub_parm.sub_in_width * sub_parm.sub_in_height;
            }
        } else {
            if (dmaio_lofs.in2_lofs != 0) {
                flushsize = dmaio_lofs.in2_lofs * in_size.in_height;
            } else {
                flushsize = in_size.in_width * in_size.in_height;
            }
        }
		if (p_addr->dma_do_not_sync == 0) {
        	nue2_pt_dma_flush_mem2dev(addr2, flushsize);
		}
    }

#if 0
	phy_addr0 = (addr0 != 0) ? dma_getPhyAddr(addr0) : 0;
	phy_addr1 = (addr1 != 0) ? dma_getPhyAddr(addr1) : 0;
	phy_addr2 = (addr2 != 0) ? dma_getPhyAddr(addr2) : 0;
#else
	if (p_addr->is_pa == 1) {
		phy_addr0 = addr0;
		phy_addr1 = addr1;
		phy_addr2 = addr2;
	} else {
		phy_addr0 = (addr0 != 0) ? nue2_pt_va2pa(addr0) : 0;
		phy_addr1 = (addr1 != 0) ? nue2_pt_va2pa(addr1) : 0;
		phy_addr2 = (addr2 != 0) ? nue2_pt_va2pa(addr2) : 0;
	}
#endif

	nue2_set_dmain_addr(phy_addr0, phy_addr1, phy_addr2);
#if 0
	nvt_dbg(INFO, "addr0:%08x\r\n", addr0);
	nvt_dbg(INFO, "addr1:%08x\r\n", addr1);
	nvt_dbg(INFO, "addr2:%08x\r\n", addr2);

	nvt_dbg(INFO, "phy_addr0:%08x\r\n", phy_addr0);
	nvt_dbg(INFO, "phy_addr1:%08x\r\n", phy_addr1);
	nvt_dbg(INFO, "phy_addr2:%08x\r\n", phy_addr2);
#endif

	// Output Dram DMA/Cache Handle
	addr0 = p_addr->addr_out0;
    addr1 = p_addr->addr_out1;
    addr2 = p_addr->addr_out2;

    //nvt_dbg(IND, "NUE2 do addr_out fmem_dcache_sync.\r\n");

#if (NUE2_AI_FLOW == ENABLE)
#else
    if (addr0 != 0) {
        if (func_en.pad_en) {
			if (dmaio_lofs.out0_lofs > pad_parm.pad_crop_out_width) {
				flushsize = dmaio_lofs.out0_lofs * pad_parm.pad_crop_out_height;
			} else {
				flushsize = pad_parm.pad_crop_out_width * pad_parm.pad_crop_out_height;
			}
		} else {
				
			if (func_en.rotate_en) {
				if (rotate_parm.rotate_mode == 2) {
					real_width = scale_parm.h_scl_size;
					real_height = scale_parm.v_scl_size;
				} else {
					real_width = scale_parm.v_scl_size;
					real_height = scale_parm.h_scl_size;
				}

				if (dmaio_lofs.out0_lofs != 0) {
					flushsize = dmaio_lofs.out0_lofs * real_height;
				} else {
					flushsize = real_width * real_height;
				}


			} else {
				if (dmaio_lofs.out0_lofs != 0) {
					flushsize = dmaio_lofs.out0_lofs * scale_parm.v_scl_size;
				} else {
					flushsize = scale_parm.h_scl_size * scale_parm.v_scl_size;
				}
			}
			
			if (in_fmt==NUE2_UV_CHANNEL) {
				flushsize = flushsize << 1;
			}
		}
		if (p_addr->dma_do_not_sync == 0) {
        	nue2_pt_dma_flush(addr0, flushsize);
		}
    }
#endif

#if (NUE2_AI_FLOW == ENABLE)
#else
    if (addr1 != 0) {
        if (func_en.pad_en) {
            if (dmaio_lofs.out1_lofs != 0) {
                flushsize = dmaio_lofs.out1_lofs * pad_parm.pad_crop_out_height;
            } else {
                flushsize = pad_parm.pad_crop_out_width * pad_parm.pad_crop_out_height;
            }
        } else {
            if (dmaio_lofs.out1_lofs != 0) {
                flushsize = dmaio_lofs.out1_lofs * scale_parm.v_scl_size;
            } else {
                flushsize = scale_parm.h_scl_size * scale_parm.v_scl_size;
            }
        }
        if ((in_fmt==NUE2_YUV_420)&&(!func_en.yuv2rgb_en)) {
            flushsize = (flushsize>>1);
        }
		if (p_addr->dma_do_not_sync == 0) {
        	nue2_pt_dma_flush(addr1, flushsize);
		}
    }
#endif

#if (NUE2_AI_FLOW == ENABLE)
#else
    if (addr2 != 0) {
        if (func_en.pad_en) {
            if (dmaio_lofs.out2_lofs != 0) {
                flushsize = dmaio_lofs.out2_lofs * pad_parm.pad_crop_out_height;
            } else {
                flushsize = pad_parm.pad_crop_out_width * pad_parm.pad_crop_out_height;
            }
        } else {
            if (dmaio_lofs.out2_lofs != 0) {
                flushsize = dmaio_lofs.out2_lofs * scale_parm.v_scl_size;
            } else {
                flushsize = scale_parm.h_scl_size * scale_parm.v_scl_size;
            }
        }
		if (p_addr->dma_do_not_sync == 0) {
        	nue2_pt_dma_flush(addr2, flushsize);
		}
    }
#endif

#if 0
	phy_addr0 = (addr0 != 0) ? dma_getPhyAddr(addr0) : 0;
    phy_addr1 = (addr1 != 0) ? dma_getPhyAddr(addr1) : 0;
    phy_addr2 = (addr2 != 0) ? dma_getPhyAddr(addr2) : 0;
#else
	if (p_addr->is_pa == 1) {
		phy_addr0 = addr0;
		phy_addr1 = addr1;
		phy_addr2 = addr2;
	} else {
		phy_addr0 = (addr0 != 0) ? nue2_pt_va2pa(addr0) : 0;
		phy_addr1 = (addr1 != 0) ? nue2_pt_va2pa(addr1) : 0;
		phy_addr2 = (addr2 != 0) ? nue2_pt_va2pa(addr2) : 0;
	}
#endif
	nue2_set_dmaout_addr(phy_addr0, phy_addr1, phy_addr2);

#else
	nue2_set_dmain_addr(p_addr->addr_in0, p_addr->addr_in1, p_addr->addr_in2);
    nue2_set_dmaout_addr(p_addr->addr_out0, p_addr->addr_out1, p_addr->addr_out2);
#endif


	return E_OK;
}

/**
    NUE2 Get DMA IN/OUT address status

    Get current NUE2 DMA IN/OUT address status.

    @return DMA IN/OUT address status.
*/
NUE2_DMAIO_ADDR nue2_get_dmaio_addr(VOID)
{
    NUE2_DMAIO_ADDR dma_addr = {0};
    dma_addr.addr_in0 = nue2_get_dmain_addr(NUE2_IN0_BUF);
    dma_addr.addr_in1 = nue2_get_dmain_addr(NUE2_IN1_BUF);
    dma_addr.addr_in2 = nue2_get_dmain_addr(NUE2_IN2_BUF);
    dma_addr.addr_out0 = nue2_get_dmaout_addr(NUE2_OUT0_BUF);
    dma_addr.addr_out1 = nue2_get_dmaout_addr(NUE2_OUT1_BUF);
    dma_addr.addr_out2 = nue2_get_dmaout_addr(NUE2_OUT2_BUF);

    return dma_addr;
}

/**
    NUE2 Set flip parameter status

    Set current NUE2 flip parameter status.

    @return flip parameter status.
*/
ER nue2_set_flip_parm(NUE2_FLIP_PARM flip_parm)
{
	UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;
	ER erReturn = E_OK;
	NUE2_FUNC_EN func_en = nue2_get_func_en();

	if (func_en.rotate_en || func_en.hsv_en) {
		if (flip_parm.flip_mode) {
			nvt_dbg(ERR, "Error! HSV or Rotate can't be supported FLIP(%d) mode", flip_parm.flip_mode);
			return E_PAR;
		}
	}
   
	ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.NUE2_FLIP_MODE = flip_parm.flip_mode;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);

    return erReturn;
}

/**
    NUE2 Get flip parameter status

    Get current NUE2 flip parameter status.

    @return flip parameter status.
*/
NUE2_FLIP_PARM nue2_get_flip_parm(VOID)
{
    NUE2_FLIP_PARM flip_parm;
	UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	T_NUE2_FUNCTION_ENABLE_REGISTER0 reg0;

   
	ofs = NUE2_FUNCTION_ENABLE_REGISTER0_OFS;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	flip_parm.flip_mode = reg0.bit.NUE2_FLIP_MODE;

    return flip_parm;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
    NUE2 Set linked-list base address

    Set NUE2 Linked list base address.

    @param[in] ll_base_addr.

    @return None.
*/
VOID nue2_set_ll_base_addr(UINT32 ll_base_addr)
{
	T_NUE2_LL_BASE_ADDRESS_REGISTER_528 reg0;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs = NUE2_LL_BASE_ADDRESS_REGISTER_OFS_528;
	reg0.reg = NUE2_GETDATA(ofs, base_addr);
	reg0.bit.LL_BASE_ADDR = ll_base_addr;
	NUE2_SETDATA(ofs, reg0.reg, base_addr);
}

/**
    NUE2 Get linked-list base address

    Get NUE2 Linked list base address.

    @param[in] None.

    @return None.
*/
UINT32 nue2_get_ll_base_addr(VOID)
{
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    T_NUE2_LL_BASE_ADDRESS_REGISTER_528 reg0;


    ofs = NUE2_LL_BASE_ADDRESS_REGISTER_OFS_528;
    reg0.reg = NUE2_GETDATA(ofs, base_addr);

    return (UINT32) reg0.bit.LL_BASE_ADDR;
}

/**
    NUE2 Set Mean shift parameter status

    Set current NUE2 Mean shift parameter status.

    @return Mean shift parameter status.
*/
#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)
#else
ER nue2_set_mean_shift_parm(NUE2_MEAN_SHIFT_PARM mean_shift_parm)
{
	UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	T_MEAN_SHIFT_REGISTER0_528 reg0_528;
	ER erReturn = E_OK;
   
	ofs = MEAN_SHIFT_REGISTER0_OFS_528;
	reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
	reg0_528.bit.MEAN_SHIFT_DIR = mean_shift_parm.mean_shift_dir;
	reg0_528.bit.MEAN_SHIFT = mean_shift_parm.mean_shift;
	reg0_528.bit.MEAN_SCALE = mean_shift_parm.mean_scale;
	NUE2_SETDATA(ofs, reg0_528.reg, base_addr);

    return erReturn;
}

/**
    NUE2 Get Mean shift parameter status

    Get current NUE2 Mean shift parameter status.

    @return Mean shift parameter status.
*/
NUE2_MEAN_SHIFT_PARM nue2_get_mean_shift_parm(VOID)
{
    NUE2_MEAN_SHIFT_PARM mean_shift_parm;
	UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	T_MEAN_SHIFT_REGISTER0_528 reg0_528;

   
	ofs = MEAN_SHIFT_REGISTER0_OFS_528;
	reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
	mean_shift_parm.mean_shift_dir = reg0_528.bit.MEAN_SHIFT_DIR;
	mean_shift_parm.mean_shift = reg0_528.bit.MEAN_SHIFT;
	mean_shift_parm.mean_scale = reg0_528.bit.MEAN_SCALE;

    return mean_shift_parm;
}

/**
    NUE2 Set burst mode

    Set NUE2 burst mode

    @param[in] IN mode

    @param[in] OUT mode

    @return None.
*/
ER nue2_set_burst(UINT8 in_burst_mode, UINT8 out_burst_mode)
{
    ER erReturn = E_OK;
    T_NUE2_DEBUG_DESIGN_REGISTER reg0;
	T_NUE2_DEBUG_DESIGN_REGISTER_528 reg0_528;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs;

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = NUE2_DEBUG_DESIGN_REGISTER_OFS;
		reg0.reg = NUE2_GETDATA(ofs, base_addr);
		reg0.bit.INDATA_BURST_MODE = in_burst_mode;
		reg0.bit.OUTRST_BURST_MODE = out_burst_mode;
		NUE2_SETDATA(ofs, reg0.reg, base_addr);
	} else {
		ofs = NUE2_DEBUG_DESIGN_REGISTER_OFS_528;
		reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
		reg0_528.bit.INDATA_BURST_MODE = in_burst_mode;
		reg0_528.bit.OUTRST_BURST_MODE = out_burst_mode;
		NUE2_SETDATA(ofs, reg0_528.reg, base_addr);
	}

    return erReturn;
}
#endif //#if (KDRV_AI_MINI_FOR_FASTBOOT == 2 || KDRV_AI_MINI_FOR_FASTBOOT == 1)

/**
    NUE2 Set dma disable parameter status

    Set current NUE2 dma parameter status.

    @return none.
*/
VOID nue2_set_dma_disable(UINT8 is_en)
{
    UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    T_DMA_DISABLE_REGISTER0_528 reg0_528;
	T_DMA_DISABLE_REGISTER0 reg0;

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = DMA_DISABLE_REGISTER0_OFS;
		reg0.reg = NUE2_GETDATA(ofs, base_addr);
		reg0.bit.DMA_DISABLE = is_en;
		NUE2_SETDATA(ofs, reg0.reg, base_addr);
	} else {
		ofs = DMA_DISABLE_REGISTER0_OFS_528;
		reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
		reg0_528.bit.DMA_DISABLE = is_en;
		NUE2_SETDATA(ofs, reg0_528.reg, base_addr);
	}

    return;
}

BOOL nue2_get_dma_disable(VOID)
{
 	UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    T_DMA_DISABLE_REGISTER0_528 reg0_528;
	T_DMA_DISABLE_REGISTER0 reg0;
	BOOL is_dma_disable;

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = DMA_DISABLE_REGISTER0_OFS;
		reg0.reg = NUE2_GETDATA(ofs, base_addr);
		is_dma_disable = reg0.bit.DMA_DISABLE;
	} else {
		ofs = DMA_DISABLE_REGISTER0_OFS_528;
		reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
		is_dma_disable = reg0_528.bit.DMA_DISABLE;
	}

    return is_dma_disable;
}

BOOL nue2_get_engine_idle(VOID)
{
 	UINT32 ofs;
    UINT32 base_addr = NUE2_IOADDR_REG_BASE;
    T_DMA_DISABLE_REGISTER0_528 reg0_528;
	T_DMA_DISABLE_REGISTER0 reg0;
	BOOL is_nue2_idle;

	if(nvt_get_chip_id() == CHIP_NA51055) {
		ofs = DMA_DISABLE_REGISTER0_OFS;
		reg0.reg = NUE2_GETDATA(ofs, base_addr);
		is_nue2_idle = reg0.bit.NUE2_IDLE;
	} else {
		ofs = DMA_DISABLE_REGISTER0_OFS_528;
		reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
		is_nue2_idle = reg0_528.bit.NUE2_IDLE;
	}

    return is_nue2_idle;
}

/**
    NUE2 Set burst mode

    Set NUE2 burst mode

    @param[in] IN mode

    @param[in] OUT mode

    @return None.
*/
VOID nue2_set_cycle_en(NUE2_ENGINE_CYCLE_MODE mode)
{
	T_NUE2_DEBUG_DESIGN_REGISTER_528 reg0_528;
	UINT32 base_addr = NUE2_IOADDR_REG_BASE;
	UINT32 ofs;

	if(nvt_get_chip_id() == CHIP_NA51055) {
	} else {
		if (mode == NUE2_CYCLE_OFF) {
			ofs = NUE2_DEBUG_DESIGN_REGISTER_OFS_528;
			reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
			reg0_528.bit.CYCLE_EN_0 = 0;
			reg0_528.bit.CYCLE_EN_1 = 0;
			NUE2_SETDATA(ofs, reg0_528.reg, base_addr);
		} else if (mode == NUE2_CYCLE_APP) {
			ofs = NUE2_DEBUG_DESIGN_REGISTER_OFS_528;
			reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
			reg0_528.bit.CYCLE_EN_0 = 1;
			reg0_528.bit.CYCLE_EN_1 = 0;
			NUE2_SETDATA(ofs, reg0_528.reg, base_addr);
		} else if (mode == NUE2_CYCLE_LL) {
			ofs = NUE2_DEBUG_DESIGN_REGISTER_OFS_528;
			reg0_528.reg = NUE2_GETDATA(ofs, base_addr);
			reg0_528.bit.CYCLE_EN_0 = 1;
			reg0_528.bit.CYCLE_EN_1 = 1;
			NUE2_SETDATA(ofs, reg0_528.reg, base_addr);
		}
	}

    return;
}


